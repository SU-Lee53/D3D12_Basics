#include "pch.h"
#include "D3D12ResourceManager.h"
#include "../DirectXTex/dds.h"
#include "../DirectXTex/DirectXTex.h"
#include "DDSTextureLoader12.h"

D3D12ResourceManager::D3D12ResourceManager()
{
}

D3D12ResourceManager::~D3D12ResourceManager()
{
}

BOOL D3D12ResourceManager::Initialize(ComPtr<ID3D12Device5>& pD3DDevice)
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

HRESULT D3D12ResourceManager::CreateIndexBuffer(DWORD dwIndexNum, D3D12_INDEX_BUFFER_VIEW& refOutIndexBufferView, ComPtr<ID3D12Resource>& prefOutBuffer, void* pInitData)
{
	HRESULT hr = S_OK;

	D3D12_INDEX_BUFFER_VIEW IndexBufferView = {};
	ComPtr<ID3D12Resource> pIndexBuffer = nullptr;
	ComPtr<ID3D12Resource> pUploadBuffer = nullptr;
	UINT IndexBufferSize = sizeof(WORD) * dwIndexNum;

	hr = m_pD3DDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(IndexBufferSize),
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(pIndexBuffer.GetAddressOf()));

	if (FAILED(hr))
	{
		__debugbreak();
		return hr;
	}

	if (pInitData)
	{
		hr = m_pCommandAllocator->Reset();
		if (FAILED(hr))
		{
			__debugbreak();
			return hr;
		}

		hr = m_pCommandList->Reset(m_pCommandAllocator.Get(), nullptr);
		if (FAILED(hr))
		{
			__debugbreak();
			return hr;
		}

		hr = m_pD3DDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(IndexBufferSize),
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(pUploadBuffer.GetAddressOf()));

		if (FAILED(hr))
		{
			__debugbreak();
			return hr;
		}

		UINT8* pIndexDataBegin = nullptr;
		CD3DX12_RANGE writeRange(0, 0);

		hr = pUploadBuffer->Map(0, &writeRange, reinterpret_cast<void**>(&pIndexDataBegin));
		if (SUCCEEDED(hr))
		{
			memcpy(pIndexDataBegin, pInitData, IndexBufferSize);
			pUploadBuffer->Unmap(0, nullptr);
		}
		else
		{
			__debugbreak();
			return hr;
		}

		m_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pIndexBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
		m_pCommandList->CopyBufferRegion(pIndexBuffer.Get(), 0, pUploadBuffer.Get(), 0, IndexBufferSize);
		m_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pIndexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER));

		m_pCommandList->Close();

		ID3D12CommandList* ppCommandLists[] = { m_pCommandList.Get() };
		m_pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		Fence();
		WaitForFenceValue();
	}
	
	IndexBufferView.BufferLocation = pIndexBuffer->GetGPUVirtualAddress();
	IndexBufferView.Format = DXGI_FORMAT_R16_UINT;
	IndexBufferView.SizeInBytes = IndexBufferSize;

	refOutIndexBufferView = IndexBufferView;
	prefOutBuffer = pIndexBuffer;

	return hr;
}

BOOL D3D12ResourceManager::CreateTexture(ComPtr<ID3D12Resource>& prefOutResource, UINT width, UINT height, DXGI_FORMAT format, BYTE* pInitImage)
{
	ComPtr<ID3D12Resource> pTexResource = nullptr;
	ComPtr<ID3D12Resource> pUploadBuffer = nullptr;

	D3D12_RESOURCE_DESC textureDesc = {};
	textureDesc.MipLevels = 1;
	textureDesc.Format = format;
	textureDesc.Width = width;
	textureDesc.Height = height;
	textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	textureDesc.DepthOrArraySize = 1;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

	HRESULT hr = S_OK;
	hr = m_pD3DDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&textureDesc,
		D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(pTexResource.GetAddressOf()));

	if (FAILED(hr))
	{
		__debugbreak();
		return FALSE;
	}

	if (pInitImage)
	{
		D3D12_RESOURCE_DESC Desc = pTexResource->GetDesc();
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT FootPrint;
		UINT Rows = 0;
		UINT64 RowSize = 0;
		UINT64 TotalBytes = 0;

		m_pD3DDevice->GetCopyableFootprints(&Desc, 0, 1, 0, &FootPrint, &Rows, &RowSize, &TotalBytes);

		std::shared_ptr<BYTE> pMappedPtr = nullptr;
		CD3DX12_RANGE writeRange(0, 0);

		UINT64 uploadBufferSize = GetRequiredIntermediateSize(pTexResource.Get(), 0, 1);

		hr = m_pD3DDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(pUploadBuffer.GetAddressOf()));
		if (FAILED(hr))
		{
			__debugbreak();
			return FALSE;
		}

		HRESULT hr = pUploadBuffer->Map(0, &writeRange, reinterpret_cast<void**>(&pMappedPtr));
		if (SUCCEEDED(hr))
		{
			const BYTE* pSrc = pInitImage;
			BYTE* pDest = pMappedPtr.get();
			for (UINT y = 0; y < height; y++)
			{
				std::memcpy(pDest, pSrc, width * 4);
				pSrc += (width * 4);
				pDest += FootPrint.Footprint.RowPitch;
			}
		}
		else
		{
			__debugbreak();
			return FALSE;
		}
		pUploadBuffer->Unmap(0, nullptr);

		UpdateTextureForWrite(pTexResource, pUploadBuffer);

	}

	prefOutResource = pTexResource;

	return TRUE;

}

BOOL D3D12ResourceManager::CreateTextureFromFile(ComPtr<ID3D12Resource>& prefOutResource, D3D12_RESOURCE_DESC& refOutDesc, const WCHAR* wchFilename)
{
	BOOL bResult = FALSE;

	ComPtr<ID3D12Resource> pTexResource = nullptr;
	ComPtr<ID3D12Resource> pUploadBuffer = nullptr;

	D3D12_RESOURCE_DESC textureDesc = {};

	std::unique_ptr<uint8_t[]> ddsData;
	std::vector<D3D12_SUBRESOURCE_DATA> subResourceData;

	// GPU Heap 은 여기서 생성이 되므로 별도로 pTexResource 를 CreateCommittedResource() 로 만들 필요가 없음
	if (FAILED(LoadDDSTextureFromFile(m_pD3DDevice.Get(), wchFilename, pTexResource.GetAddressOf(), ddsData, subResourceData)))
	{
		__debugbreak();
		return bResult;
	}

	textureDesc = pTexResource->GetDesc();
	UINT subResourceSize = (UINT)subResourceData.size();
	UINT64 uploadBufferSize = GetRequiredIntermediateSize(pTexResource.Get(), 0, subResourceSize);

	if (FAILED(m_pD3DDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&pUploadBuffer))))
	{
		__debugbreak();
		return bResult;
	}

	if (FAILED(m_pCommandAllocator->Reset()))
	{
		__debugbreak();
		return bResult;
	}

	if (FAILED(m_pCommandList->Reset(m_pCommandAllocator.Get(), nullptr)))
	{
		__debugbreak();
		return bResult;
	}

	m_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pTexResource.Get(), D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST));
	{

		// UpdateSubresources 는 아래 UpdateTextureForWrite 에서 하던 CopyTextureRegion 을 수행
		// 이번에는 subResource 가 존재하므로(밉맵) 이 함수를 이용하면 간편하게 Command List 에 복사하는 커맨드를 기록할 수 있음
		UpdateSubresources(m_pCommandList.Get(), pTexResource.Get(), pUploadBuffer.Get(), 0, 0, subResourceSize, subResourceData.data());
	}
	m_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pTexResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE));

	m_pCommandList->Close();

	ID3D12CommandList* ppCommandLists[] = { m_pCommandList.Get()};
	m_pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	Fence();
	WaitForFenceValue();

	prefOutResource = pTexResource;
	refOutDesc = textureDesc;

	bResult = true;

	return bResult;
}

void D3D12ResourceManager::UpdateTextureForWrite(ComPtr<ID3D12Resource> pDestTexResource, ComPtr<ID3D12Resource> pSrcTexResource)
{
	// 이건 왜 있음?
	//	- 사실상 GPU 메모리로 텍스쳐 데이터를 올리는건 여기서 진행됨
	//	- 이전 CreateTexture 는 선형 메모리로 pTexResource 에다가 복사해두었지만 GPU 입장에서는 캐시미스가 잦아서 좋지 못한 구조임
	//	- 여기서는 CopyTextureRegion 를 이용하여 pTexResource 를 하드웨어가 써먹기 좋은 (캐시 히트가 잘나는) 구조로 GPU에 보냄

	const DWORD MAX_SUB_RESOURCE_NUM = 32;	// 최대 MipMap 레벨. 32단계까지는 갈일이 사실상 없겠지만 여유롭게 잡음
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT FootPrint[MAX_SUB_RESOURCE_NUM] = {};
	UINT Rows[MAX_SUB_RESOURCE_NUM] = {};
	UINT64 RowSize[MAX_SUB_RESOURCE_NUM] = {};
	UINT64 TotalBytes = 0;

	D3D12_RESOURCE_DESC desc = pDestTexResource->GetDesc();
	if (desc.MipLevels > (UINT)_countof(FootPrint))	// _countof(FootPrint) == MAX_SUB_RESOURCE_NUM 사실상 허용된 밉맵 레벨이 넘으면 걍 터트리라는 말
	{
		__debugbreak();
		return;
	}

	m_pD3DDevice->GetCopyableFootprints(&desc, 0, desc.MipLevels, 0, FootPrint, Rows, RowSize, &TotalBytes);

	if (FAILED(m_pCommandAllocator->Reset()))
	{
		__debugbreak();
		return;
	}

	if (FAILED(m_pCommandList->Reset(m_pCommandAllocator.Get(), nullptr)))
	{
		__debugbreak();
		return;
	}

	m_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pDestTexResource.Get(), D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST));
	for (DWORD i = 0; i < desc.MipLevels; i++)
	{
		D3D12_TEXTURE_COPY_LOCATION destLocation = {};
		destLocation.PlacedFootprint = FootPrint[i];
		destLocation.pResource = pDestTexResource.Get();
		destLocation.SubresourceIndex = i;
		destLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

		D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
		srcLocation.PlacedFootprint = FootPrint[i];
		srcLocation.pResource = pSrcTexResource.Get();
		srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;

		m_pCommandList->CopyTextureRegion(&destLocation, 0, 0, 0, &srcLocation, nullptr);

	}
	m_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pDestTexResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE));
	m_pCommandList->Close();

	ID3D12CommandList* ppCommandList[] = { m_pCommandList.Get() };
	m_pCommandQueue->ExecuteCommandLists(_countof(ppCommandList), ppCommandList);

	Fence();
	WaitForFenceValue();

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
