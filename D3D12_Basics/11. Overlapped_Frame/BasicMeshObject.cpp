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

void BasicMeshObject::Draw(ComPtr<ID3D12GraphicsCommandList> pCommandList, const XMMATRIX& pMatWorld, const D3D12_CPU_DESCRIPTOR_HANDLE& srv)
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

	if (!refDescriptorPool->AllocDescriptorTable(cpuDescriptorTable, gpuDescriptorTable, DESCRIPTOR_COUNT_FOR_DRAW))
	{
		__debugbreak();
		return;
	}

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


	// Root Signature 설정
	pCommandList->SetGraphicsRootSignature(m_pRootSignature.Get());

	// 텍스쳐가 있는 Descriptor Heap 을 설정
	pCommandList->SetDescriptorHeaps(1, refDescriptorHeap.GetAddressOf());

	// 이번에 사용할 constantBuffer 의 Descriptor 를 렌더링용(SHADER_VISIBLE 한) Descriptor Table 에 복사
	CD3DX12_CPU_DESCRIPTOR_HANDLE cbvDest(cpuDescriptorTable, (INT)BASIC_MESH_DESCRIPTOR_INDEX::BASIC_MESH_DESCRIPTOR_INDEX_CBV, srvDescriptorSize);
	prefD3DDevice->CopyDescriptorsSimple(1, cbvDest, pCB->CBVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// srv(texture) 의 Descriptor 를 렌더링용 Descriptor Table 에 복사
	if (srv.ptr)
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE srvDest(cpuDescriptorTable, (INT)BASIC_MESH_DESCRIPTOR_INDEX::BASIC_MESH_DESCRIPTOR_INDEX_TEX, srvDescriptorSize);
		prefD3DDevice->CopyDescriptorsSimple(1, srvDest, srv, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	pCommandList->SetGraphicsRootDescriptorTable(0, gpuDescriptorTable);

	pCommandList->SetPipelineState(m_pPipelineState.Get());
	pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pCommandList->IASetVertexBuffers(0, 1, &m_VertexBufferView);
	pCommandList->IASetIndexBuffer(&m_IndexBufferView);
	pCommandList->DrawIndexedInstanced(6, 1, 0, 0, 0);
}

BOOL BasicMeshObject::CreateMesh()
{
	BOOL bResult = FALSE;
	ComPtr<ID3D12Device5>& pD3DDevice = m_pRenderer->GetDevice();
	std::shared_ptr<D3D12ResourceManager>& pResourceManager = m_pRenderer->GetResourceManager();

	BasicVertex Vertices[] =
	{
		{ { -0.25f, 0.25f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, {0.f, 0.f} },
		{ { 0.25f, 0.25f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, {1.f, 0.f} },
		{ { 0.25f, -0.25f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, {1.f, 1.f} },
		{ { -0.25f, -0.25f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, {0.f, 1.f} }
	};

	WORD Indices[] =
	{
		0, 1, 2,
		0, 2, 3
	};

	const UINT VertexBufferSize = sizeof(Vertices);

	if (FAILED(pResourceManager->CreateVertexBuffer(sizeof(BasicVertex), (DWORD)_countof(Vertices), m_VertexBufferView, m_pVertexBuffer, Vertices)))
	{
		__debugbreak();
		return FALSE;
	}

	if (FAILED(pResourceManager->CreateIndexBuffer((DWORD)_countof(Indices), m_IndexBufferView, m_pIndexBuffer, Indices)))
	{
		__debugbreak();
		return FALSE;
	}

	bResult = TRUE;

	return bResult;
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

	CD3DX12_DESCRIPTOR_RANGE ranges[2] = {};
	ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
	ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	CD3DX12_ROOT_PARAMETER rootParameters[1] = {};
	rootParameters[0].InitAsDescriptorTable(_countof(ranges), ranges, D3D12_SHADER_VISIBILITY_ALL);

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
	// rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);	// 이전의 코드. 비어있는 Root Signature 가 나옴
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
