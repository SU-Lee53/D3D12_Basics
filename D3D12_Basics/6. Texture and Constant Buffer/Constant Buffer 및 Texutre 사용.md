# 06. Constant Buffer 및 Texutre 사용

## 이번에는
- 이전 예제에서 ConstantBuffer 를 이용해 Object 를 움직여볼거임
- Constant Buffer
	- 이전 Vertex Buffer 때처럼 굳이 GPU 메모리에 반드시 존재할 필요까지는 없음
	- 다만 Descriptor 를 이용해서 GPU 메모리에 전송하는게 기본이고 나중에 Ray Tracing 을 위해서는 꼭 필요하므로 이번에는 Descriptor 를 이용
	- 원래 업데이트가 잦아 CPU 접근이 많으므로 시스템 메모리에 놓고 다이나믹 버퍼라는 형태로 사용하는 경우가 많음
		- D3D12 의 경우 이를 Upload Heap 이라고 함 

## 예제 코드 분석
- 쉐이더 코드의 추가사항
```
Texture2D texDiffuse : register(t0);
SamplerState samplerDiffuse	: register(s0);

cbuffer CONSTANT_BUFFER_DEFAULT : register(b0)
{
    float4 g_Offset;
};

struct VSInput
{
    float4 Pos : POSITION;
    float4 color : COLOR;
    float2 TexCoord : TEXCOORD0;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 TexCoord : TEXCOORD0;
};

PSInput VSMain(VSInput input)
{
    PSInput result = (PSInput)0;

    result.position = input.Pos;
    result.position.xy += g_Offset.xy;
    result.TexCoord = input.TexCoord;
    result.color = input.color;
    
    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float4	texColor = texDiffuse.Sample(samplerDiffuse, input.TexCoord);
    return texColor * input.color;
}
```
	- Constant Buffer 가 추가됨
	- Constant Buffer 로 받은 offset 만큼 position 을 이동

- ```CBasicMeshObject::InitRootSinagture()``` 의 변경사항
```
BOOL CBasicMeshObject::InitRootSinagture()
{
	ID3D12Device5* pD3DDeivce = m_pRenderer->INL_GetD3DDevice();
	ID3DBlob* pSignature = nullptr;
	ID3DBlob* pError = nullptr;

	CD3DX12_DESCRIPTOR_RANGE ranges[2] = {};
	ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);	// b0 : Constant Buffer View
	ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);	// t0 : texture
	
	CD3DX12_ROOT_PARAMETER rootParameters[1] = {};
	rootParameters[0].InitAsDescriptorTable(_countof(ranges), ranges, D3D12_SHADER_VISIBILITY_ALL);

	// default sampler
	D3D12_STATIC_SAMPLER_DESC sampler = {};
	SetDefaultSamplerDesc(&sampler, 0);
	sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;

	// Allow input layout and deny uneccessary access to certain pipeline stages.
	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

	// Create an empty root signature.
	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	//rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	rootSignatureDesc.Init(_countof(rootParameters), rootParameters, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	if (FAILED(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &pSignature, &pError)))
	{
		__debugbreak();
	}

	if (FAILED(pD3DDeivce->CreateRootSignature(0, pSignature->GetBufferPointer(), pSignature->GetBufferSize(), IID_PPV_ARGS(&m_pRootSignature))))
	{
		__debugbreak();
	}
	if (pSignature)
	{
		pSignature->Release();
		pSignature = nullptr;
	}
	if (pError)
	{
		pError->Release();
		pError = nullptr;
	}
	return TRUE;
}
```
	- Constant Buffer 를 위해 이번에는 2칸짜리 Root Signature 를 만듬

- ```CBasicMeshObject::CreateMesh()``` 의 변경사항
```BOOL CBasicMeshObject::CreateMesh()
{
	// 바깥에서 버텍스데이터와 텍스처를 입력하는 식으로 변경할 것

	BOOL bResult = FALSE;
	ID3D12Device5* pD3DDeivce = m_pRenderer->INL_GetD3DDevice();
	CD3D12ResourceManager*	pResourceManager = m_pRenderer->INL_GetResourceManager();

	// Create the vertex buffer.
	// Define the geometry for a triangle.
	BasicVertex Vertices[] =
	{
		{ { 0.0f, 0.25f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f }, { 0.5f, 0.0f } },
		{ { 0.25f, -0.25f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
		{ { -0.25f, -0.25f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } }
	};

	const UINT VertexBufferSize = sizeof(Vertices);

	if (FAILED(pResourceManager->CreateVertexBuffer(sizeof(BasicVertex), (DWORD)_countof(Vertices), &m_VertexBufferView, &m_pVertexBuffer, Vertices)))
	{
		__debugbreak();
		goto lb_return;
	}
	// Create Texture
	const UINT TexWidth = 16;
	const UINT TexHeight = 16;;
	DXGI_FORMAT TexFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

	BYTE* pImage = (BYTE*)malloc(TexWidth * TexHeight * 4);
	memset(pImage, 0, TexWidth * TexHeight * 4);

	BOOL bFirstColorIsWhite = TRUE;
	
	for (UINT y = 0; y < TexHeight; y++)
	{
		for (UINT x = 0; x < TexWidth; x++)
		{
			
			RGBA* pDest = (RGBA*)(pImage + (x + y * TexWidth) * 4);

			pDest->r = rand() % 255;
			pDest->g = rand() % 255;
			pDest->b = rand() % 255;

			if ((bFirstColorIsWhite + x) % 2)
			{
				pDest->r = 255;
				pDest->g = 255;
				pDest->b = 255;
			}
			else
			{
				pDest->r = 0;
				pDest->g = 0;
				pDest->b = 0;
			}
			pDest->a = 255;
		}
		bFirstColorIsWhite++;
		bFirstColorIsWhite %= 2;
	}
	pResourceManager->CreateTexture(&m_pTexResource, TexWidth, TexHeight, TexFormat, pImage);

	free(pImage);

	CreateDescriptorTable();

	// Create ShaderResource View from TextureResource
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
		SRVDesc.Format = TexFormat;
		SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		SRVDesc.Texture2D.MipLevels = 1;

		CD3DX12_CPU_DESCRIPTOR_HANDLE srv(m_pDescritorHeap->GetCPUDescriptorHandleForHeapStart(), BASIC_MESH_DESCRIPTOR_INDEX_TEX, m_srvDescriptorSize);
		pD3DDeivce->CreateShaderResourceView(m_pTexResource, &SRVDesc, srv);
	}


	// create constant buffer
	{
		// CB size is required to be 256-byte aligned.
		const UINT constantBufferSize = (UINT)AlignConstantBufferSize(sizeof(CONSTANT_BUFFER_DEFAULT));

		if (FAILED(pD3DDeivce->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(constantBufferSize),
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(&m_pConstantBuffer))))
		{
			__debugbreak();
		}

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
        cbvDesc.BufferLocation = m_pConstantBuffer->GetGPUVirtualAddress();
        cbvDesc.SizeInBytes = constantBufferSize;
		CD3DX12_CPU_DESCRIPTOR_HANDLE cbv(m_pDescritorHeap->GetCPUDescriptorHandleForHeapStart(), BASIC_MESH_DESCRIPTOR_INDEX_CBV, m_srvDescriptorSize);
		pD3DDeivce->CreateConstantBufferView(&cbvDesc, cbv);

        // Map and initialize the constant buffer. We don't unmap this until the
        // app closes. Keeping things mapped for the lifetime of the resource is okay.
        CD3DX12_RANGE writeRange(0, 0);        // We do not intend to read from this resource on the CPU.
		if (FAILED(m_pConstantBuffer->Map(0, &writeRange, reinterpret_cast<void**>(&m_pSysConstBufferDefault))))
		{
			__debugbreak();
		}
		m_pSysConstBufferDefault->offset.x = 0.0f;
		m_pSysConstBufferDefault->offset.y = 0.0f;
	}

	bResult = TRUE;

lb_return:
	return bResult;
}
```
	- Constant Buffer 를 만드는 과정이 추가
	- AlignConstantBufferSize()
		- 상수버퍼는 256바이트의 정렬을 맞추어야 함
		- 이 정렬을 맞추기 위해 D3DUtil에 해당 함수가 추가됨
	- 재미있게도 Constant Buffer 의 내용을 쓰기 위해 Map 을 하고 Unmap을 하지 않음
		- D3D12의 권장사항으로 내용을 계속 Cpu가 써서 업로드 하기 때문으로 보임 
	
- CBasicMeshObject::CreateDescriptorTable() 의 변경사항 
```
BOOL CBasicMeshObject::CreateDescriptorTable()
{

	BOOL bResult = FALSE;
	ID3D12Device5* pD3DDeivce = m_pRenderer->INL_GetD3DDevice();

	// 렌더링시 Descriptor Table로 사용할 Descriptor Heap - 
	// Descriptor Table
	// | CBV | SRV(TEX) |
	m_srvDescriptorSize = pD3DDeivce->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// create descriptor heap
	D3D12_DESCRIPTOR_HEAP_DESC commonHeapDesc = {};
	commonHeapDesc.NumDescriptors = DESCRIPTOR_COUNT_FOR_DRAW;
	commonHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	commonHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	if (FAILED(pD3DDeivce->CreateDescriptorHeap(&commonHeapDesc, IID_PPV_ARGS(&m_pDescritorHeap))))
	{
		__debugbreak();
		goto lb_return;
	}
	bResult = TRUE;
lb_return:
	return bResult;

}
```
	- Constant Buffer 를 위해 2칸짜리 Descriptor Table 을 만듬
	- 


- main 의 Update() 변경사항
```
void Update()
{
	g_fOffsetX += g_fSpeed;
	if (g_fOffsetX > 0.75f)
	{
		g_fSpeed *= -1.0f;
	}
	if (g_fOffsetX < -0.75f)
	{
		g_fSpeed *= -1.0f;
	}
}
```
	- 여기서 이동할 Offset 을 바꿔줌 

- ```CD3D12Renderer::RenderMeshObject(void* pMeshObjHandle, float x_offset, float y_offset)``` 의 변경사항
```
void CD3D12Renderer::RenderMeshObject(void* pMeshObjHandle, float x_offset, float y_offset)
{
	CBasicMeshObject* pMeshObj = (CBasicMeshObject*)pMeshObjHandle;
	XMFLOAT2	Pos = { x_offset, y_offset };
	pMeshObj->Draw(m_pCommandList, &Pos);
}
```
	- offset 을 pos 로 하여 Draw로 넘김

- ```CBasicMeshObject::Draw(ID3D12GraphicsCommandList* pCommandList, const XMFLOAT2* pPos)``` 의 변경사항
```
void CBasicMeshObject::Draw(ID3D12GraphicsCommandList* pCommandList, const XMFLOAT2* pPos)
{
	m_pSysConstBufferDefault->offset.x = pPos->x;
	m_pSysConstBufferDefault->offset.y = pPos->y;

	// set RootSignature
	pCommandList->SetGraphicsRootSignature(m_pRootSignature);

	pCommandList->SetDescriptorHeaps(1, &m_pDescritorHeap);

	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuDescriptorTable(m_pDescritorHeap->GetGPUDescriptorHandleForHeapStart());
	pCommandList->SetGraphicsRootDescriptorTable(0, gpuDescriptorTable);

	pCommandList->SetPipelineState(m_pPipelineState);
	pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pCommandList->IASetVertexBuffers(0, 1, &m_VertexBufferView);
	pCommandList->DrawInstanced(3, 1, 0, 0);
	

}
```
	- Constant Buffer 를 설정하는 부분이 추가됨 
