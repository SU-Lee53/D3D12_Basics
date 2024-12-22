#include "pch.h"
#include "BasicMeshObject.h"
#include <d3dcompiler.h>
#include "D3DUtil.h"
#include "D3D12Renderer.h"
#include "D3D12ResourceManager.h"
#include "Typedef.h"
#include "DescriptorPool.h"
#include "SimpleConstantBufferPool.h"

ComPtr<ID3D12RootSignature>	BasicMeshObject::m_pRootSignature = nullptr;
ComPtr<ID3D12PipelineState>	BasicMeshObject::m_pPipelineState = nullptr;
DWORD						BasicMeshObject::m_dwInitRefCount = 0;

BasicMeshObject::BasicMeshObject()
{
}

BasicMeshObject::~BasicMeshObject()
{
	m_dwInitRefCount--;
}

BOOL BasicMeshObject::Initialize(std::shared_ptr<D3D12Renderer> pRenderer)
{
	m_pRenderer = pRenderer;

	BOOL bResult = InitCommonResources();
	return bResult;
}

void BasicMeshObject::Draw(ComPtr<ID3D12GraphicsCommandList> pCommandList, const XMMATRIX& pMatWorld)
{
	// 각각의 draw() 작업의 무결성을 부작하려면 draw() 작업마다 다른 영역의 Descriptor table(SHADER_VISIBLE 한) 과 다른 영역의 CBV 를 사용하여야 함
	// 따라서 draw() 할 때마다 CBV 는 ConstantBufferPool 에서 할당받고, 렌더러용 Descriptor Table 은 DescriptorPool 에서 할당받아와야 함

	ComPtr<ID3D12Device5>& prefD3DDevice = m_pRenderer->GetDevice();
	UINT srvDescriptorSize = m_pRenderer->GetSrvDescriptorSize();
	std::shared_ptr<DescriptorPool>& refDescriptorPool = m_pRenderer->GetDescriptorPool();
	ComPtr<ID3D12DescriptorHeap>& refDescriptorHeap = refDescriptorPool->GetDescriptorHeap();
	std::shared_ptr<SimpleConstantBufferPool>& refConstantBufferPool = m_pRenderer->GetConstantBufferPool();

	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuDescriptorTable = {};
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuDescriptorTable = {};
	DWORD dwRequiredDescriptorCount = DESCRIPTOR_COUNT_PER_OBJ + (m_dwTriGroupCount * DESCRIPTOR_COUNT_PER_TRI_GROUP);

	if (!refDescriptorPool->AllocDescriptorTable(cpuDescriptorTable, gpuDescriptorTable, dwRequiredDescriptorCount))
	{
		__debugbreak();
		return;
	}

	// 각각의 draw() 에 대해 독립적인 constant buffer(내부적으로는 같은 Resource 의 영역) 을 사용함
	CB_CONTAINER* pCB = refConstantBufferPool->Alloc();
	if (!pCB)
	{
		__debugbreak();
		return;
	}

	CONSTANT_BUFFER_DEFAULT* pConstantBufferDefault = (CONSTANT_BUFFER_DEFAULT*)pCB->pSysMemAddr;

	// Constant Buffer 에 내용 기입
	m_pRenderer->GetViewProjMatrix(pConstantBufferDefault->matView, pConstantBufferDefault->matProj);
	pConstantBufferDefault->matWorld = XMMatrixTranspose(pMatWorld);

	// Descriptor Table 을 구성
	// 이번에 사용할 constant buffer 의 descriptor 를 렌더링용(SHADER_VISIBLE) descriptor table 에 복사
	
	// per Obj
	CD3DX12_CPU_DESCRIPTOR_HANDLE Dest(cpuDescriptorTable, BASIC_MESH_DESCRIPTOR_INDEX_PER_OBJ_CBV, srvDescriptorSize);
	prefD3DDevice->CopyDescriptorsSimple(1, Dest, pCB->CBVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	Dest.Offset(1, srvDescriptorSize);

	// per Tri Group
	for (DWORD i = 0; i < m_dwTriGroupCount; i++)
	{
		const INDEXED_TRI_GROUP& TriGroup = m_TriGroupList[i];
		std::shared_ptr<TEXTURE_HANDLE> pTexHandle = TriGroup.pTexHandle;
		
		if (pTexHandle)
		{
			prefD3DDevice->CopyDescriptorsSimple(1, Dest, pTexHandle->srv, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}
		else
		{
			__debugbreak();
		}

		Dest.Offset(1, srvDescriptorSize);
	}


	// Root Signature 설정
	pCommandList->SetGraphicsRootSignature(m_pRootSignature.Get());
	pCommandList->SetDescriptorHeaps(1, refDescriptorHeap.GetAddressOf());

	// 현재 Root Signature 의 구조
	// Descriptor Table : | CBV | SRV[0] | SRV[1] | SRV[2] | SRV[3] | SRV[4] | SRV[5] | 

	pCommandList->SetPipelineState(m_pPipelineState.Get());
	pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pCommandList->IASetVertexBuffers(0, 1, &m_VertexBufferView);

	// Root Parameter[0] 의 Descriptor Table 을 설정함
	pCommandList->SetGraphicsRootDescriptorTable(0, gpuDescriptorTable);

	// gpuDescriptorTable 에서 srvDescriptorSize 만큼 DESCRIPTOR_COUNT_PER_OBJ 을 offset 으로 이동한 D3D12_GPU_DESCRIPTOR_HANDLE -> SRV 시작주소
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandleForTriGroup(gpuDescriptorTable, DESCRIPTOR_COUNT_PER_OBJ, srvDescriptorSize);
	for (DWORD i = 0; i < m_dwTriGroupCount; i++)
	{
		// Root Parameter[1] 의 Descriptor Table 을 설정함
		pCommandList->SetGraphicsRootDescriptorTable(1, gpuDescriptorHandleForTriGroup);
		gpuDescriptorHandleForTriGroup.Offset(1, srvDescriptorSize);	// 다음 SRV로 offset 이동

		const INDEXED_TRI_GROUP& TriGroup = m_TriGroupList[i];
		pCommandList->IASetIndexBuffer(&TriGroup.IndexBufferView);
		pCommandList->DrawIndexedInstanced(TriGroup.dwTriCount * 3, 1, 0, 0, 0);
	}

}

BOOL BasicMeshObject::BeginCreateMesh(const BasicVertex* pVertexList, DWORD dwVertexNum, DWORD dwTriGroupCount)
{
	BOOL bResult = FALSE;
	ComPtr<ID3D12Device5>& refD3DDevice = m_pRenderer->GetDevice();
	std::shared_ptr<D3D12ResourceManager>& prefResourceManager = m_pRenderer->GetResourceManager();

	if (dwTriGroupCount > MAX_TRI_GROUP_COUNT_PER_OBJ)
		__debugbreak();

	HRESULT hr = prefResourceManager->CreateVertexBuffer(sizeof(BasicVertex), dwVertexNum, m_VertexBufferView, m_pVertexBuffer, (void*)pVertexList);
	if (FAILED(hr))
	{
		__debugbreak();
		return bResult;
	}

	m_dwMaxTriGroupCount = dwTriGroupCount;
	m_TriGroupList.resize(m_dwMaxTriGroupCount);

	bResult = TRUE;

	return TRUE;
}

BOOL BasicMeshObject::InsertTriGroup(const WORD* pIndexList, DWORD dwTriCount, const WCHAR* wchTexFileName)
{
	BOOL bResult = FALSE;

	ComPtr<ID3D12Device5>& pD3DDevice = m_pRenderer->GetDevice();
	UINT srvDescriptorSize = m_pRenderer->GetSrvDescriptorSize();
	std::shared_ptr<D3D12ResourceManager>& prefResourceManager = m_pRenderer->GetResourceManager();
	std::shared_ptr<SingleDescriptorAllocator>& prefSingleDescriptorAllocator = m_pRenderer->GetSingleDescriptorAllocator();

	ComPtr<ID3D12Resource> pIndexBuffer = nullptr;
	D3D12_INDEX_BUFFER_VIEW IndexBufferView = {};

	if (m_dwTriGroupCount >= m_dwMaxTriGroupCount)
	{
		__debugbreak();
		return bResult;
	}

	HRESULT hr = prefResourceManager->CreateIndexBuffer(dwTriCount * 3, IndexBufferView, pIndexBuffer, (void*)pIndexList);
	if (FAILED(hr))
	{
		__debugbreak();
		return bResult;
	}

	INDEXED_TRI_GROUP* pTriGroup = &m_TriGroupList[m_dwTriGroupCount];
	pTriGroup->pIndexBuffer = pIndexBuffer;
	pTriGroup->IndexBufferView = IndexBufferView;
	pTriGroup->dwTriCount = dwTriCount;
	pTriGroup->pTexHandle = std::static_pointer_cast<TEXTURE_HANDLE>(m_pRenderer->CreateTextureFromFile(wchTexFileName));
	m_dwTriGroupCount++;
	bResult = TRUE;

	return bResult;
}

void BasicMeshObject::EndCreateMesh()
{
}

BOOL BasicMeshObject::InitCommonResources()
{
	if (m_dwInitRefCount == 0)
	{
		InitRootSignature();
		InitPipelineState();
	}

	m_dwInitRefCount++;
	return m_dwInitRefCount;
}

BOOL BasicMeshObject::InitRootSignature()
{
	ComPtr<ID3D12Device5>& pD3DDevice = m_pRenderer->GetDevice();

	/*
		이번 Root Signature 는 아래같은 형태로 구성됨
		
		Object - CBV - RootParam[0]
		{
			TriGroup[0] - SRV[0] - RootParam[1] - Draw()
			TriGroup[1] - SRV[1] - RootParam[1] - Draw()
			TriGroup[2] - SRV[2] - RootParam[1] - Draw()
			TriGroup[3] - SRV[3] - RootParam[1] - Draw()
			TriGroup[4] - SRV[4] - RootParam[1] - Draw()
			TriGroup[5] - SRV[5] - RootParam[1] - Draw()
		}
		
		이해가 안된다면 렌더링 시 Descriptor Table은 아래처럼 구성됨

			Descriptor Table : | CBV | SRV[0] | SRV[1] | SRV[2] | SRV[3] | SRV[4] | SRV[5] | 
		
		이때 아래에서 Root Signature 의 Root Parameter 는 2개인데 각 면마다 아래처럼 구성되어 렌더링 됨

			Descriptor Table	:	|   CBV    |  SRV[0]  |  SRV[1]  |  SRV[2]  |  SRV[3]  |  SRV[4]  |  SRV[5]  |
			Root Signature		
				Tri Group[0]	:	| Param[0] | Param[1] |
				Tri Group[1]	:	| Param[0] |          | Param[1] |
				Tri Group[2]	:	| Param[0] |                     | Param[1] |
				Tri Group[3]	:	| Param[0] |                                | Param[1] |
				Tri Group[4]	:	| Param[0] |                                           | Param[1] |
				Tri Group[5]	:	| Param[0] |                                                      | Param[1] |

		이런식으로 Root Parameter 가 가리키는 Descriptor 의 Offset 을 옮겨 다니면서 각 면마다 다른 텍스쳐를 그리게 됨
	*/

	CD3DX12_DESCRIPTOR_RANGE rangesPerObj[1] = {};
	rangesPerObj[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);

	CD3DX12_DESCRIPTOR_RANGE rangesPerTriGroup[1] = {};
	rangesPerTriGroup[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	CD3DX12_ROOT_PARAMETER rootParameters[2] = {};
	rootParameters[0].InitAsDescriptorTable(_countof(rangesPerObj), rangesPerObj, D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[1].InitAsDescriptorTable(_countof(rangesPerTriGroup), rangesPerTriGroup, D3D12_SHADER_VISIBILITY_ALL);


	// Default Sampler
	D3D12_STATIC_SAMPLER_DESC sampler = {};
	D3DUtils::SetDefaultSamplerDesc(&sampler, 0);
	sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;

	// 불필요한 파이프라인 단계에서 Root Signature 로의 접근을 막음
	// 굳이? 싶은 부분이지만 하드웨어에 따라 최적화를 할 수도 있음
	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

	// Root Signature 생성
	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init(_countof(rootParameters), rootParameters, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> pSignature = nullptr;
	ComPtr<ID3DBlob> pError = nullptr;

	if (FAILED(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, pSignature.GetAddressOf(), pError.GetAddressOf())))
	{
		std::string errMessage = (const char*)pError->GetBufferPointer();
		MessageBoxA(NULL, errMessage.c_str(), "Serialize Root Signature Failed", MB_OK);
		__debugbreak();
		return FALSE;
	}

	if (FAILED(pD3DDevice->CreateRootSignature(0, pSignature->GetBufferPointer(), pSignature->GetBufferSize(), IID_PPV_ARGS(m_pRootSignature.GetAddressOf()))))
	{
		__debugbreak();
		return FALSE;
	}

	return TRUE;
}

BOOL BasicMeshObject::InitPipelineState()
{
	ComPtr<ID3D12Device5>& pD3DDevice = m_pRenderer->GetDevice();


	// 쉐이더 코드 컴파일 (./Shaders/shaders.hlsl)
	ComPtr<ID3DBlob> pVertexShader = nullptr;
	ComPtr<ID3DBlob> pPixelShader = nullptr;
	ComPtr<ID3DBlob> pError = nullptr;
#ifdef _DEBUG
	// 그래픽 디버깅 툴을 위한 쉐이더 컴파일 옵션
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif

	if (FAILED(D3DCompileFromFile(L"./Shaders/shaders.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, pVertexShader.GetAddressOf(), pError.GetAddressOf())))
	{
		std::string errMessage = (const char*)pError->GetBufferPointer();
		MessageBoxA(NULL, errMessage.c_str(), "Failed to Compile VertexShader", MB_OK);
		__debugbreak();
		return FALSE;
	}

	if (FAILED(D3DCompileFromFile(L"./Shaders/shaders.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, pPixelShader.GetAddressOf(), pError.GetAddressOf())))
	{
		std::string errMessage = (const char*)pError->GetBufferPointer();
		MessageBoxA(NULL, errMessage.c_str(), "Failed to Compile PixelShader", MB_OK);
		__debugbreak();
		return FALSE;
	}

	// Vertex Input Layout 정의
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
	{
		/*
		{ SemanticName, SemanticIndex, Format, InputSlot, AlignedByteOffset, InputSlotClass, InstanceDataStepRate }
		*/
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },	// 0
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },	// 0 + 12 = 12
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 28, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }		// 12 + 16 = 28
	};

	// Pipeline State Object 를 정의하고 생성
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
	psoDesc.pRootSignature = m_pRootSignature.Get();
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(pVertexShader->GetBufferPointer(), pVertexShader->GetBufferSize());
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(pPixelShader->GetBufferPointer(), pPixelShader->GetBufferSize());
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	//psoDesc.DepthStencilState.DepthEnable = FALSE;
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	psoDesc.DepthStencilState.StencilEnable = FALSE;
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;	// 강의에 없는데 회전된 뒷면도 보려면 설정해야함
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	psoDesc.SampleDesc.Count = 1;
	if (FAILED(pD3DDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(m_pPipelineState.GetAddressOf()))))
	{
		__debugbreak();
		return FALSE;
	}

	return TRUE;
}
