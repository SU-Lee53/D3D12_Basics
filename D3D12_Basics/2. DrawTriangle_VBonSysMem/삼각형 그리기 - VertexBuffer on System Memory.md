
# 02. 삼각형 그리기 - VertexBuffer on System Memory

## 삼각형 그리기-필요 리소스
- Shader 코드
- Root Signature
- VertexBuffer
- PipelineState

- D3D12에서 새로운 요소
	- Root Signature
		- 어떤 리소스가 어떻게 파이프라인에 바인드 될지를 정의
		- Resource binding 설정을 기술한 일종의 템플릿
		- Descriptor Table 에서 어떤 리소스가 어느 레지스터로 매핑되는지를 표현함
	- PipelineState
		- 여러가지 모든 상태들을 하나로 묶어서 처리
			- Blend State
			- Depth State
			- Render Target Format
			- Shaders
			- 기타 등등
		- 장점: 기존 Draw 전에 하던 상태설정을 한번에 가능
			- 원래 ID3D11DeviceCondext 객체로 줄줄이 하던 것들이 SetPipelineState()한줄로 끝남
		- 단점: 간단하게 Shader 나 Blend State 하나만 바꾸려고 해도 ID3D12PipelineState 를 통째로 바꿔야 함
			- 중간중간 z-buffering 을 끄거나 와이어프레임으로 샘플링하거나 등등을 하려면 별도의 PipelineState를 만들어주어야 함...
			- 조합의 경우의 수가 미친듯이 폭발해버리지만 어쩔수 없음...

## 예제 분석
1. shaders.hlsl
```
hlsl
struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

PSInput VSMain(float4 position : POSITION, float4 color : COLOR)
{
    PSInput result;

    result.position = position;
    result.color = color;

    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return input.color;
}
```
2. main.cpp 의 추가사항
```
	g_pRenderer = new CD3D12Renderer;
	g_pRenderer->Initialize(g_hMainWindow, TRUE, TRUE);
	g_pMeshObj = g_pRenderer->CreateBasicMeshObject();
```
	- 이번에는 화면에 그릴 삼각형을 만들고 시작함

3. CD3D12Renderer::CreateBasicMeshObject()
```
void* CD3D12Renderer::CreateBasicMeshObject()
{
	CBasicMeshObject* pMeshObj = new CBasicMeshObject;
	pMeshObj->Initialize(this);
	pMeshObj->CreateMesh();
	return pMeshObj;
}
```

4. CBasicMeshObject
- CBasicMeshObject::InitCommonResources()
```
BOOL CBasicMeshObject::InitCommonResources()
{
	if (m_dwInitRefCount)
		goto lb_true;

	InitRootSinagture();
	InitPipelineState();

lb_true:
	m_dwInitRefCount++;
	return m_dwInitRefCount;
}
```
	- CBasicMeshObject::Initialize() 에서 호출함
	- CommonResource 인 이유
		- 본 CBasicMeshObject 객체는 생성시 렌더링 방법이 정해짐
		- 즉, RootSignature 와 PipelineState가 정해지고 시작 
			- 쉐이더를 포함하는 Rendering State의 내용들은 같은 메쉬일 경우 모두 공유하겠다 -> 이것이 CommonResource
			- 때문에 내부적으로 RefCount를 가짐(클래스 안의 mdwInitRefCount)
		- 같은 CBasicMeshObject를 가진 Object라면 굳이 여러 Rendering State를 만들지 않도록 함
		- 또 다른 CBasicMeshObject를 만든다면 아예 다른 메쉬구조를 가진 Object 임을 의미
	- Root Signature 와 PipelineState를 만들었다면 Ref Count를 올림
		- 다음 이 함수가 호출되어도 새로운 Root Signature 와 PipelineState 를 만들지 않음 
		
- CBasicMeshObject::InitRootSinagture()
```
BOOL CBasicMeshObject::InitRootSinagture()
{
	ID3D12Device5* pD3DDeivce = m_pRenderer->INL_GetD3DDevice();

	// Create an empty root signature.
	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ID3DBlob* pSignature = nullptr;
	ID3DBlob* pError = nullptr;

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
	- 하는게 없음. 이번 예제는 Root Signature를 사용하지 않을 예정이기 때문
	- 다음번에 사용할때는 CD3DX12_ROOT_SIGNATURE_DESC 의 내용을 채워주어야 함
	- 같은 CBasicMeshObject 면 하나의 Root Signature 를 공유하므로 m_pRootSignature 는 static 으로 선언됨 

- CBasicMeshObject::InitPipelineState()
```
BOOL CBasicMeshObject::InitPipelineState()
{
	ID3D12Device5* pD3DDeivce = m_pRenderer->INL_GetD3DDevice();

	ID3DBlob* pVertexShader = nullptr;
	ID3DBlob* pPixelShader = nullptr;


#if defined(_DEBUG)
	// Enable better shader debugging with the graphics debugging tools.
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif
	if (FAILED(D3DCompileFromFile(L"./Shaders/shaders.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &pVertexShader, nullptr)))
	{
		__debugbreak();
	}
	if (FAILED(D3DCompileFromFile(L"./Shaders/shaders.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pPixelShader, nullptr)))
	{
		__debugbreak();
	}


	// Define the vertex input layout.
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};


	// Describe and create the graphics pipeline state object (PSO).
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
	psoDesc.pRootSignature = m_pRootSignature;
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(pVertexShader->GetBufferPointer(), pVertexShader->GetBufferSize());
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(pPixelShader->GetBufferPointer(), pPixelShader->GetBufferSize());
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState.DepthEnable = FALSE;
	psoDesc.DepthStencilState.StencilEnable = FALSE;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count = 1;
	if (FAILED(pD3DDeivce->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pPipelineState))))
	{
		__debugbreak();
	}

	if (pVertexShader)
	{
		pVertexShader->Release();
		pVertexShader = nullptr;
	}
	if (pPixelShader)
	{
		pPixelShader->Release();
		pPixelShader = nullptr;
	}
	return TRUE;
}
```
	- D3DCompileFromFile() 은 Shader Model 5.0 이후로는 안된다고 함
		- 대신 DXC 라는 오픈소스 컴파일러로 가능

- CBasicMeshObject::CreateMesh()
```
BOOL CBasicMeshObject::CreateMesh()
{
	// 바깥에서 버텍스데이터와 텍스처를 입력하는 식으로 변경할 것

	BOOL bResult = FALSE;
	ID3D12Device5* pD3DDeivce = m_pRenderer->INL_GetD3DDevice();

	// Create the vertex buffer.
	// Define the geometry for a triangle.
	BasicVertex Vertices[] =
	{
		{ { 0.0f, 0.25f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
		{ { 0.25f, -0.25f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
		{ { -0.25f, -0.25f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }
	};

	const UINT VertexBufferSize = sizeof(Vertices);

	// Note: using upload heaps to transfer static data like vert buffers is not 
	// recommended. Every time the GPU needs it, the upload heap will be marshalled 
	// over. Please read up on Default Heap usage. An upload heap is used here for 
	// code simplicity and because there are very few verts to actually transfer.
	if (FAILED(pD3DDeivce->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(VertexBufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_pVertexBuffer))))
	{
		__debugbreak();
	}

	// Copy the triangle data to the vertex buffer.
	UINT8* pVertexDataBegin = nullptr;
	CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.

	if (FAILED(m_pVertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin))))
	{
		__debugbreak();
	}
	memcpy(pVertexDataBegin, Vertices, sizeof(Vertices));
	m_pVertexBuffer->Unmap(0, nullptr);

	// Initialize the vertex buffer view.
	m_VertexBufferView.BufferLocation = m_pVertexBuffer->GetGPUVirtualAddress();
	m_VertexBufferView.StrideInBytes = sizeof(BasicVertex);
	m_VertexBufferView.SizeInBytes = VertexBufferSize;

	bResult = TRUE;

lb_return:
	return bResult;
}
```
	- 여기서 BasicMesh 는 typedef.h 에 선언됨
	- XMFLOAT3, XMFLOAT4 로 각 12, 16바이트로 선언 <- 16바이트 padding?
		- Shader에서 float4로 position을 받음을 알 수 있음
		- Input Assembler 단계에서 알아서 4바이트를 채워
	- 이번에는 VertexBuffer 를 시스템 메모리에 만듬
		- 모든 리소스들이 시스템 메모리에 만드는것을 허용하지는 않음
			- Constant Buffer, VertexBuffer, IndexBuffer 등 사이즈가 그닥 크지않고, 읽고쓰기가 빈번하지 않다면 허용
			- 텍스쳐처럼 반대의 경우라면 아예 허용되지 않음
		- CreateCommittedResource() 함수의 파라미터로 &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD)를 이용하여 시스템 메모리에 만들어짐 
			- D3D12_HEAP_TYPE_UPLOAD 는 CPU Write, GPU Read
			- 반대는 D3D12_HEAP_TYPE_READBACK 으로 Compute Shader 등을 사용할때 좋음
			- 아예 GPU에 만드려면 D3D12_HEAP_TYPE_DEFAULT 를 사용 
	- Map/Unmap 은 D3D11 시절과 거의 동일함
	- D3D12_VERTEX_BUFFER_VIEW
		- Descriptor 가 아닌 시스템 메모리 이므로 별도로 렌더링 파이프라인에 묶어주어야 함 -> 이때 사용하는것이 D3D12_VERTEX_BUFFER_VIEW

5.  CD3D12Renderer::RenderMeshObject(void* pMeshObjHandle)
```
void CD3D12Renderer::RenderMeshObject(void* pMeshObjHandle)
{
	CBasicMeshObject* pMeshObj = (CBasicMeshObject*)pMeshObjHandle;
	pMeshObj->Draw(m_pCommandList);
}
```
	- 일부러 Content Layer 에서 엔진 내부를 숨기기 위해 void* 를 사용함
		- 엔진 내부는 뭔지 알기때문에 캐스팅이 가능함

6. CBasicMeshObject::Draw(ID3D12GraphicsCommandList* pCommandList)
```
void CBasicMeshObject::Draw(ID3D12GraphicsCommandList* pCommandList)
{
	// set RootSignature
	pCommandList->SetGraphicsRootSignature(m_pRootSignature);
   
	pCommandList->SetPipelineState(m_pPipelineState);
    pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    pCommandList->IASetVertexBuffers(0, 1, &m_VertexBufferView);
    pCommandList->DrawInstanced(3, 1, 0, 0);

}
```
	- 당연히 바로 그리는건 아니고 Command List 에 기록함
