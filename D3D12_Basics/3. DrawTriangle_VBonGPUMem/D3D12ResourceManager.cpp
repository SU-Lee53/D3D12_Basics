#include "pch.h"
#include "D3D12ResourceManager.h"

D3D12ResourceManager::D3D12ResourceManager()
{
}

D3D12ResourceManager::~D3D12ResourceManager()
{
}

BOOL D3D12ResourceManager::Initialize(ComPtr<ID3D12Device5> pD3DDevice)
{
	BOOL bResult = FALSE;

	m_pD3DDevice = pD3DDevice;

	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	HRESULT hr = m_pD3DDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(m_pCommandQueue.GetAddressOf()));
	if (FAILED(hr))
	{
		__debugbreak();
		return FALSE;
	}

	CreateCommandList();

	CreateFence();

	bResult = TRUE;

	return bResult;
}

HRESULT D3D12ResourceManager::CreateVertexBuffer(UINT SizePerVertex, DWORD dwVertexNum, D3D12_VERTEX_BUFFER_VIEW& pOutVertexBufferView, ComPtr<ID3D12Resource>& ppOutBuffer, void* pInitData)
{
	HRESULT hr = S_OK;

	D3D12_VERTEX_BUFFER_VIEW VertexBufferView = {};
	ComPtr<ID3D12Resource> pVertexBuffer = nullptr;
	ComPtr<ID3D12Resource> pUploadBuffer = nullptr;
	UINT VertexBufferSize = SizePerVertex * dwVertexNum;

	// 이번에는 GPU에 VertexBuffer 를 만듬
	// 바로 GPU에 입력은 당연히 안되고 Upload Buffer 에다가 CPU가 쓴 다음 GPU에다 전송함
	
	hr = m_pD3DDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(VertexBufferSize),
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(pVertexBuffer.GetAddressOf()));

	if (FAILED(hr))
	{
		__debugbreak();
		return hr;
	}

	if (pInitData)
	{
		if (FAILED(m_pCommandAllocator->Reset()))
			__debugbreak();

		if (FAILED(m_pCommandList->Reset(m_pCommandAllocator.Get(), nullptr)))
			__debugbreak();

		hr = m_pD3DDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(VertexBufferSize),
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(pUploadBuffer.GetAddressOf()));

		if (FAILED(hr))
		{
			__debugbreak();
			return hr;
		}

		UINT8* pVertexDataBegin = nullptr;
		CD3DX12_RANGE writeRange(0, 0);

		hr = pUploadBuffer->Map(0, &writeRange, reinterpret_cast<void**>(&pVertexDataBegin));
		if (SUCCEEDED(hr))
		{
			memcpy(pVertexDataBegin, pInitData, VertexBufferSize);
			pUploadBuffer->Unmap(0, nullptr);
		}
		else
		{
			__debugbreak();
			return S_FALSE;
		}

		m_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pVertexBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
		{
			m_pCommandList->CopyBufferRegion(pVertexBuffer.Get(), 0, pUploadBuffer.Get(), 0, VertexBufferSize);
		}
		m_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pVertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

		m_pCommandList->Close();

		ID3D12CommandList* ppCommandLists[] = { m_pCommandList.Get()};
		m_pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		Fence();
		WaitForFenceValue();
	}

	VertexBufferView.BufferLocation = pVertexBuffer->GetGPUVirtualAddress();
	VertexBufferView.StrideInBytes = SizePerVertex;
	VertexBufferView.SizeInBytes = VertexBufferSize;

	pOutVertexBufferView = VertexBufferView;
	ppOutBuffer = pVertexBuffer;

	return hr;
}

void D3D12ResourceManager::CreateFence()
{
	if (FAILED(m_pD3DDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_pFence.GetAddressOf()))))
	{
		__debugbreak();
	}

	m_ui64FenceValue = 0;
	m_hFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
}

void D3D12ResourceManager::CreateCommandList()
{
	if (FAILED(m_pD3DDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_pCommandAllocator.GetAddressOf()))))
	{
		__debugbreak();
	}

	if (FAILED(m_pD3DDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_pCommandAllocator.Get(), nullptr, IID_PPV_ARGS(m_pCommandList.GetAddressOf()))))
	{
		__debugbreak();
	}

	m_pCommandList->Close();
}

UINT64 D3D12ResourceManager::Fence()
{
	m_ui64FenceValue++;
	m_pCommandQueue->Signal(m_pFence.Get(), m_ui64FenceValue);
	return m_ui64FenceValue;
}

void D3D12ResourceManager::WaitForFenceValue()
{
	const UINT64 ExpectedFenceValue = m_ui64FenceValue;

	if (m_pFence->GetCompletedValue() < ExpectedFenceValue)
	{
		m_pFence->SetEventOnCompletion(ExpectedFenceValue, m_hFenceEvent);
		WaitForSingleObject(m_hFenceEvent, INFINITE);
	}
}
