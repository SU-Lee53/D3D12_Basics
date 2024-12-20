#include "pch.h"
#include "BasicMeshObject.h"
#include <d3dcompiler.h>
#include "D3DUtil.h"
#include "D3D12Renderer.h"
#include "D3D12ResourceManager.h"
#include "Typedef.h"

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

void BasicMeshObject::Draw(ComPtr<ID3D12GraphicsCommandList> pCommandList, const XMFLOAT2& pPos)
{
	m_pSysConstBufferDefault->offset.x = pPos.x;
	m_pSysConstBufferDefault->offset.y = pPos.y;

	// Root Signature 설정
	pCommandList->SetGraphicsRootSignature(m_pRootSignature.Get());

	// 텍스쳐가 있는 Descriptor Heap 을 설정
	pCommandList->SetDescriptorHeaps(1, m_pDescriptorHeap.GetAddressOf());

	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuDescriptorTable(m_pDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
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
		{ { -0.25f, 0.25f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f }, {0.f, 0.f} },
		{ { 0.25f, 0.25f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f }, {1.f, 0.f} },
		{ { 0.25f, -0.25f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f }, {1.f, 1.f} },
		{ { -0.25f, -0.25f, 0.0f }, { 0.0f, 1.0f, 1.0f, 1.0f }, {0.f, 1.f} }
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

	// 격자 무늬 텍스쳐 생성
	const UINT texWidth = 16;
	const UINT texHeight = 16;
	DXGI_FORMAT texFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

	BYTE* pImage = (BYTE*)malloc(texWidth * texHeight * 4);
	memset(pImage, 0, texWidth * texHeight * 4);

	BOOL bFirstColorIsWhite = TRUE;

	for (UINT y = 0; y < texHeight; y++)
	{
		for (UINT x = 0; x < texWidth; x++)
		{
			RGBA* pDest = (RGBA*)(pImage + (x + y * texWidth) * 4);
		
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
	pResourceManager->CreateTexture(m_pTexResource, texWidth, texHeight, texFormat, pImage);

	free(pImage);

	CreateDescriptorTable();

	// 텍스쳐를 위한 SRV를 생성
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
		SRVDesc.Format = texFormat;
		SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		SRVDesc.Texture2D.MipLevels = 1;

		CD3DX12_CPU_DESCRIPTOR_HANDLE srv(m_pDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), (INT)BASIC_MESH_DESCRIPTOR_INDEX::BASIC_MESH_DESCRIPTOR_INDEX_TEX, m_srvDescriptorSize);
		pD3DDevice->CreateShaderResourceView(m_pTexResource.Get(), &SRVDesc, srv);
	}
	
	// Constant Buffer 생성
	{
		// Constant Buffer 는 256바이트 정렬이 되어있어야 함
		const UINT constantBufferSize = (UINT)D3DUtils::AlignConstantBuffersize(sizeof(CONSTANT_BUFFER_DEFAULT));

		if (FAILED(pD3DDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(constantBufferSize),
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(m_pConstantBuffer.GetAddressOf()))))
		{
			__debugbreak();
			return FALSE;
		}

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = m_pConstantBuffer->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = constantBufferSize;
		CD3DX12_CPU_DESCRIPTOR_HANDLE cbv(m_pDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), (INT)BASIC_MESH_DESCRIPTOR_INDEX::BASIC_MESH_DESCRIPTOR_INDEX_CBV, m_srvDescriptorSize);
		pD3DDevice->CreateConstantBufferView(&cbvDesc, cbv);

		CD3DX12_RANGE writeRange(0, 0);
		if (SUCCEEDED(m_pConstantBuffer->Map(0, &writeRange, reinterpret_cast<void**>(&m_pSysConstBufferDefault))))
		{
			m_pSysConstBufferDefault->offset.x = 0.0f;
			m_pSysConstBufferDefault->offset.y = 0.0f;
		}
		else
		{
			__debugbreak();
			return FALSE;
		}
		// 여기서 Unmap 을 하지 않음. 
		// 계속 CPU가 m_pSysConstBufferDefault 에다가 내용을 써넣어야 하기 때문에 매번 Unmap을 하는것은 손해임.
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
	psoDesc.DepthStencilState.DepthEnable = FALSE;
	psoDesc.DepthStencilState.StencilEnable = FALSE;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count = 1;
	if (FAILED(pD3DDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(m_pPipelineState.GetAddressOf()))))
	{
		__debugbreak();
		return FALSE;
	}

	return TRUE;
}

BOOL BasicMeshObject::CreateDescriptorTable()
{
	BOOL bResult = FALSE;
	ComPtr<ID3D12Device5>& pD3DDevice = m_pRenderer->GetDevice();

	// 렌더링 시에 Descriptor Table 로 사용할 Descriptor Heap
	// 이번에는 | SRV | CBV | 로 2개가 들어감
	
	// SRV 가 Descriptor 에서 차지하는 크기를 가져옴
	// enum 을 보면 CBV 와 SRV 가 차지하는 크기가 동일한 것으로 보임
	m_srvDescriptorSize = pD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// Descriptor Heap 생성
	D3D12_DESCRIPTOR_HEAP_DESC commonHeapDesc = {};
	commonHeapDesc.NumDescriptors = DESCRIPTOR_COUNT_FOR_DRAW;
	commonHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	commonHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	if (FAILED(pD3DDevice->CreateDescriptorHeap(&commonHeapDesc, IID_PPV_ARGS(m_pDescriptorHeap.GetAddressOf()))))
	{
		__debugbreak();
		return bResult;
	}

	bResult = TRUE;

	return bResult;
}
