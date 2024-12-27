#include "pch.h"
#include <d3dcompiler.h>
#include "SpriteObject.h"
#include "D3D12ResourceManager.h"
#include "SimpleConstantBufferPool.h"
#include "SingleDescriptorAllocator.h"
#include "DescriptorPool.h"
#include "D3D12Renderer.h"
#include "D3DUtil.h"

using namespace DirectX;

ComPtr<ID3D12RootSignature> SpriteObject::m_pRootSignature = nullptr;
ComPtr<ID3D12PipelineState> SpriteObject::m_pPipelineState = nullptr;

ComPtr<ID3D12Resource> SpriteObject::m_pVertexBuffer = nullptr;
D3D12_VERTEX_BUFFER_VIEW SpriteObject::m_VertexBufferView = {};

ComPtr<ID3D12Resource> SpriteObject::m_pIndexBuffer = nullptr;
D3D12_INDEX_BUFFER_VIEW SpriteObject::m_IndexBufferView = {};

DWORD SpriteObject::m_dwInitRefCount = 0;

SpriteObject::SpriteObject()
{
}

SpriteObject::~SpriteObject()
{
}

BOOL SpriteObject::Initialize(std::shared_ptr<D3D12Renderer> pRenderer)
{
	m_pRenderer = pRenderer;

	BOOL bResult = InitCommonResources();
	return bResult;
}

BOOL SpriteObject::Initialize(std::shared_ptr<D3D12Renderer> pRenderer, const WCHAR* wchFilename, const RECT* pRect)
{
	m_pRenderer = pRenderer;

	BOOL bResult = (InitCommonResources() != 0);
	if (bResult)
	{
		UINT TexWidth = 1;
		UINT TexHeight = 1;
		m_pTexHandle = std::static_pointer_cast<TEXTURE_HANDLE>(m_pRenderer->CreateTextureFromFile(wchFilename));

		if (m_pTexHandle)
		{
			D3D12_RESOURCE_DESC desc = m_pTexHandle->pTexResource->GetDesc();
			TexWidth = desc.Width;
			TexHeight = desc.Height;
		}

		if (pRect)
		{
			m_Rect = *pRect;
			m_Scale.x = (float)(m_Rect.right - m_Rect.left) / (float)TexWidth;
			m_Scale.y = (float)(m_Rect.bottom - m_Rect.top) / (float)TexHeight;
		}
		else
		{
			if (m_pTexHandle)
			{
				D3D12_RESOURCE_DESC desc = m_pTexHandle->pTexResource->GetDesc();
				m_Rect.left = 0;
				m_Rect.top = 0;
				m_Rect.right = desc.Width;
				m_Rect.bottom = desc.Height;
			}
		}

	}

	return bResult;
}

BOOL SpriteObject::InitCommonResources()
{
	if (m_dwInitRefCount)
	{
		InitRootSignature();
		InitPipelineState();
		InitMesh();
	}

	m_dwInitRefCount++;
	return m_dwInitRefCount;
}

BOOL SpriteObject::InitRootSignature()
{
	ComPtr<ID3D12Device5>& prefD3DDevice = m_pRenderer->GetDevice();

	CD3DX12_DESCRIPTOR_RANGE ranges[2] = {};
	ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
	ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	CD3DX12_ROOT_PARAMETER rootParameters[1] = {};
	rootParameters[0].InitAsDescriptorTable(_countof(ranges), ranges, D3D12_SHADER_VISIBILITY_ALL);

	// Default Sampler
	D3D12_STATIC_SAMPLER_DESC sampler = {};
	D3DUtils::SetDefaultSamplerDesc(&sampler, 0);
	sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;

	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init(_countof(rootParameters), rootParameters, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> pSignature = nullptr;
	ComPtr<ID3DBlob> pError = nullptr;

	if (FAILED(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &pSignature, &pError)))
	{
		std::string errMsg = (const char*)pError->GetBufferPointer();
		MessageBoxA(NULL, errMsg.c_str(), "Serialize Root Signature Failed", MB_OK);
		__debugbreak();
		return FALSE;
	}

	if (FAILED(prefD3DDevice->CreateRootSignature(0, pSignature->GetBufferPointer(), pSignature->GetBufferSize(), IID_PPV_ARGS(m_pRootSignature.GetAddressOf()))))
	{
		__debugbreak();
		return FALSE;
	}

	/*
		Release Coms if not using ComPtr
	*/

	return TRUE;
}

BOOL SpriteObject::InitPipelineState()
{
	ComPtr<ID3D12Device5>& prefD3DDevice = m_pRenderer->GetDevice();

	ComPtr<ID3DBlob> pVertexShader = nullptr;
	ComPtr<ID3DBlob> pPixelShader = nullptr;
	ComPtr<ID3DBlob> pError = nullptr;

#ifdef _DEBUG
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif

	if (FAILED(D3DCompileFromFile(L"./Shaders/shSprite.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, pVertexShader.GetAddressOf(), pError.GetAddressOf())))
	{
		std::string errMessage = (const char*)pError->GetBufferPointer();
		MessageBoxA(NULL, errMessage.c_str(), "Failed to Compile VertexShader", MB_OK);
		__debugbreak();
		return FALSE;
	}

	if (FAILED(D3DCompileFromFile(L"./Shaders/shSprite.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, pPixelShader.GetAddressOf(), pError.GetAddressOf())))
	{
		std::string errMessage = (const char*)pError->GetBufferPointer();
		MessageBoxA(NULL, errMessage.c_str(), "Failed to Compile PixelShader", MB_OK);
		__debugbreak();
		return FALSE;
	}

	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,	0, 28,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
	psoDesc.pRootSignature = m_pRootSignature.Get();
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(pVertexShader->GetBufferPointer(), pVertexShader->GetBufferSize());
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(pPixelShader->GetBufferPointer(), pPixelShader->GetBufferSize());
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	psoDesc.DepthStencilState.StencilEnable = FALSE;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	psoDesc.SampleDesc.Count = 1;

	if (FAILED(prefD3DDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(m_pPipelineState.GetAddressOf()))))
	{
		__debugbreak();
		return FALSE;
	}

	return TRUE;
}

BOOL SpriteObject::InitMesh()
{
	BOOL bResult = FALSE;
	ComPtr<ID3D12Device5>& prefeD3DDevice = m_pRenderer->GetDevice();
	UINT srvDescriptorSize = m_pRenderer->GetSrvDescriptorSize();
	std::shared_ptr<D3D12ResourceManager>& prefResourceManager = m_pRenderer->GetResourceManager();
	std::shared_ptr<SingleDescriptorAllocator>& prefSingleDescriptorAllocator = m_pRenderer->GetSingleDescriptorAllocator();

	BasicVertex Vertices[] =
	{
		{ { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } },
		{ { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f } },
		{ { 1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f } },
		{ { 1.0f, 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } },
	};


	WORD Indices[] =
	{
		0, 1, 2,
		0, 2, 3
	};

	const UINT VertexBufferSize = sizeof(Vertices);

	if (FAILED(prefResourceManager->CreateVertexBuffer(sizeof(BasicVertex), (DWORD)_countof(Vertices), m_VertexBufferView, m_pVertexBuffer, Vertices)))
	{
		__debugbreak();
		return bResult;
	}

	if (FAILED(prefResourceManager->CreateIndexBuffer((DWORD)_countof(Indices), m_IndexBufferView, m_pIndexBuffer, Indices)))
	{
		__debugbreak();
		return bResult;
	}

	bResult = TRUE;

	return bResult;

}

void SpriteObject::Draw(ComPtr<ID3D12GraphicsCommandList>& refCommandList, const XMFLOAT2& refPos, const XMFLOAT2& refScale, float z)
{
	XMFLOAT2 Scale = { m_Scale.x * refScale.x, m_Scale.y * refScale.y };
	DrawWithTex(refCommandList, refPos, Scale, &m_Rect, z, m_pTexHandle.get());
}

void SpriteObject::DrawWithTex(ComPtr<ID3D12GraphicsCommandList>& prefCommandList, const XMFLOAT2& refPos, const XMFLOAT2& refScale, const RECT* pRect, float Z, const TEXTURE_HANDLE* pTexHandle)
{
	ComPtr<ID3D12Device5> prefD3DDevice = m_pRenderer->GetDevice();
	UINT srvDescriptorSize = m_pRenderer->GetSrvDescriptorSize();
	std::shared_ptr<DescriptorPool> prefDescriptorPool = m_pRenderer->GetDescriptorPool();
	ComPtr<ID3D12DescriptorHeap> prefDescriptorHeap = prefDescriptorPool->GetDescriptorHeap();
	std::shared_ptr<SimpleConstantBufferPool> prefConstantBufferPool = m_pRenderer->GetConstantBufferPool(CONSTANT_BUFFER_TYPE_SPRITE);

	UINT TexWidth = 0;
	UINT TexHeight = 0;
	D3D12_CPU_DESCRIPTOR_HANDLE srv = {};
	if (pTexHandle)
	{
		D3D12_RESOURCE_DESC desc = pTexHandle->pTexResource->GetDesc();
		TexWidth = desc.Width;
		TexHeight = desc.Height;
		srv = pTexHandle->srv;
	}

	RECT rect;
	if (!pRect)
	{
		rect.left = 0;
		rect.top = 0;
		rect.right = TexWidth;
		rect.bottom = TexHeight;
		pRect = &rect;
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuDescriptorTable = {};
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuDescriptorTable = {};

	if (!prefDescriptorPool->AllocDescriptorTable(cpuDescriptorTable, gpuDescriptorTable, DESCRIPTOR_COUNT_FOR_DRAW))
	{
		__debugbreak();
		return;
	}

	// 각각의 Draw() 에 대해 독립적인 Constant Buffer 를 사용한다
	CB_CONTAINER* pCB = prefConstantBufferPool->Alloc();
	if (!pCB)
	{
		__debugbreak();
		return;
	}

	CONSTANT_BUFFER_SPRITE* pConstantBufferSprite = (CONSTANT_BUFFER_SPRITE*)pCB->pSysMemAddr;

	pConstantBufferSprite->ScreenRes.x = (float)m_pRenderer->GetScreenWidth();
	pConstantBufferSprite->ScreenRes.y = (float)m_pRenderer->GetScreenHeight();
	pConstantBufferSprite->Pos = refPos;
	pConstantBufferSprite->Scale = refScale;
	pConstantBufferSprite->TexSize.x = (float)TexWidth;
	pConstantBufferSprite->TexSize.y = (float)TexHeight;
	pConstantBufferSprite->TexSamplePos.x = (float)pRect->left;
	pConstantBufferSprite->TexSamplePos.y = (float)pRect->top;
	pConstantBufferSprite->TexSampleSize.x = (float)(pRect->right - pRect->left);
	pConstantBufferSprite->TexSampleSize.y = (float)(pRect->bottom - pRect->top);
	pConstantBufferSprite->Z = Z;
	pConstantBufferSprite->Alpha = 1.0f;

	prefCommandList->SetGraphicsRootSignature(m_pRootSignature.Get());
	prefCommandList->SetDescriptorHeaps(1, prefDescriptorHeap.GetAddressOf());

	CD3DX12_CPU_DESCRIPTOR_HANDLE cbvDest(cpuDescriptorTable, SPRITE_DESCRIPTOR_INDEX_CBV, srvDescriptorSize);
	prefD3DDevice->CopyDescriptorsSimple(1, cbvDest, pCB->CBVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	if (srv.ptr)
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE srvDest(cpuDescriptorTable, SPRITE_DESCRIPTOR_INDEX_TEX, srvDescriptorSize);
		prefD3DDevice->CopyDescriptorsSimple(1, srvDest, srv, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	prefCommandList->SetGraphicsRootDescriptorTable(0, gpuDescriptorTable);

	prefCommandList->SetPipelineState(m_pPipelineState.Get());
	prefCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	prefCommandList->IASetVertexBuffers(0, 1, &m_VertexBufferView);
	prefCommandList->IASetIndexBuffer(&m_IndexBufferView);
	prefCommandList->DrawIndexedInstanced(6, 1, 0, 0, 0);

}
