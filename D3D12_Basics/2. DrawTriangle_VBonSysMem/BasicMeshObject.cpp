#include "pch.h"
#include "BasicMeshObject.h"
#include <d3dcompiler.h>
#include "../D3D_Util/D3DUtil.h"
#include "D3D12Renderer.h"
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

BOOL BasicMeshObject::Initialize(std::shared_ptr<CD3D12Renderer> pRenderer)
{
	m_pRenderer = pRenderer;

	BOOL bResult = InitCommonResources();
	return bResult;
}

void BasicMeshObject::Draw(ComPtr<ID3D12GraphicsCommandList> pCommandList)
{
	// Root Signature ����
	pCommandList->SetGraphicsRootSignature(m_pRootSignature.Get());

	pCommandList->SetPipelineState(m_pPipelineState.Get());
	pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pCommandList->IASetVertexBuffers(0, 1, &m_VertexBufferView);
	pCommandList->DrawInstanced(3, 1, 0, 0);
}

BOOL BasicMeshObject::CreateMesh()
{
	BOOL bResult = FALSE;
	ComPtr<ID3D12Device5>& pD3DDevice = m_pRenderer->GetDevice();

	BasicVertex Vertices[] =
	{
		{ { 0.0f, 0.25f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
		{ { 0.25f, -0.25f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
		{ { -0.25f, -0.25f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }
	};

	const UINT VertexBufferSize = sizeof(Vertices);

	// �� ������ ������ VertexBuffer on System Memory �� ������ ���⼭ ����
	// Vertex Buffer �� �ý��� �޸𸮿� ���� -> GPU �޸𸮿� ���۾��� �ٷ� ���̴� �ڵ�� �Ѱ��ַ��� ��
	// �����δ� ������ Ÿ�� �ӵ��� �ſ� �����Ƿ� �ǻ�Ȳ������ �������� ����
	if (FAILED(pD3DDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(VertexBufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_pVertexBuffer.GetAddressOf()))))
	{
		__debugbreak();
	}

	// �ﰢ�� �����͸� Vertex Buffer �� ����
	UINT8* pVertexDataBegin = nullptr;
	CD3DX12_RANGE readRange(0, 0);

	if (SUCCEEDED(m_pVertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin))))
	{
		memcpy(pVertexDataBegin, Vertices, sizeof(Vertices));
		m_pVertexBuffer->Unmap(0, nullptr);
	}
	else
	{
		__debugbreak();
	}

	// Vertex Buffer View �ʱ�ȭ
	m_VertexBufferView.BufferLocation = m_pVertexBuffer->GetGPUVirtualAddress();
	m_VertexBufferView.StrideInBytes = sizeof(BasicVertex);
	m_VertexBufferView.SizeInBytes = VertexBufferSize;

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
	return m_dwInitRefCount;	// ?? �ϴ� ���� ���󰡺�
}

BOOL BasicMeshObject::InitRootSignature()
{
	ComPtr<ID3D12Device5>& pD3DDevice = m_pRenderer->GetDevice();

	// ����ִ� Root Signature �� ����
	// ������ ����־ Input Assembler �� ���� �������� �˸��� �÷��״� �־��־��� ��
	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

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


	// ���̴� �ڵ� ������ (./Shaders/shaders.hlsl)
	ComPtr<ID3DBlob> pVertexShader = nullptr;
	ComPtr<ID3DBlob> pPixelShader = nullptr;
	ComPtr<ID3DBlob> pError = nullptr;
#ifdef _DEBUG
	// �׷��� ����� ���� ���� ���̴� ������ �ɼ�
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

	// Vertex Input Layout ����
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
	{
		/*
		{ SemanticName, SemanticIndex, Format, InputSlot, AlignedByteOffset, InputSlotClass, InstanceDataStepRate }
		*/
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	// Pipeline State Object �� �����ϰ� ����
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
