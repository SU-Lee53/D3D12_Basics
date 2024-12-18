#include "pch.h"
#include "D3D12Renderer.h"
#include "D3DUtils.h"

CD3D12Renderer::CD3D12Renderer()
{
}

CD3D12Renderer::~CD3D12Renderer()
{
	WaitForFenceValue();
	// ComPtr�� ����ϹǷ� ������ CleanUp �� �ʿ���������� ������
}

BOOL CD3D12Renderer::Initialize(HWND hWnd, BOOL bEnableDebugLayer, BOOL bEnableGBV)
{
	BOOL bResult = FALSE;

	HRESULT hr = S_OK;

	// 1. SetUp Begin
	// 1-1. Debug Layer
	ComPtr<ID3D12Debug> pDebugController = nullptr;
	DWORD dwCreateFlags = 0;
	DWORD dwCreateFactoryFlags = 0;

	if (bEnableDebugLayer)
	{
		hr = D3D12GetDebugInterface(IID_PPV_ARGS(pDebugController.GetAddressOf()));
		if (SUCCEEDED(hr))
		{
			pDebugController->EnableDebugLayer();
		}
		dwCreateFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
		if (bEnableGBV)
		{
			ComPtr<ID3D12Debug5> pDebugController5;
			hr = pDebugController->QueryInterface(pDebugController5.GetAddressOf());
			if (SUCCEEDED(hr))
			{
				pDebugController5->SetEnableGPUBasedValidation(TRUE);
				pDebugController5->SetEnableAutoName(TRUE);
			}
		}
	}

	// 1-2 DXGIFactory
	ComPtr<IDXGIFactory4> pFactory = nullptr;
	ComPtr<IDXGIAdapter1> pAdapter = nullptr;
	DXGI_ADAPTER_DESC1 adapterDesc = {};

	CreateDXGIFactory2(dwCreateFactoryFlags, IID_PPV_ARGS(pFactory.GetAddressOf()));

	std::array<D3D_FEATURE_LEVEL, 5> featureLevels =
	{
		D3D_FEATURE_LEVEL_12_2,
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0
	};

	for (DWORD featureLevelIndex = 0; featureLevelIndex < featureLevels.size(); featureLevelIndex++)
	{
		UINT uiAdapterIndex = 0;
		while (DXGI_ERROR_NOT_FOUND != pFactory->EnumAdapters1(uiAdapterIndex, pAdapter.GetAddressOf()))
		{
			pAdapter->GetDesc1(&adapterDesc);

			hr = D3D12CreateDevice(pAdapter.Get(), featureLevels[featureLevelIndex], IID_PPV_ARGS(m_pD3dDevice.GetAddressOf()));
			if (SUCCEEDED(hr))
			{
				m_FeatureLevel = featureLevels[featureLevelIndex];
				break;
			}

		}

		if (m_pD3dDevice)
			break;
	}

	// check if m_pD3dDevice is created
	if (!m_pD3dDevice)
	{
		__debugbreak();
		return bResult;
	}

	m_AdapterDesc = adapterDesc;
	m_hWnd = hWnd;

	if (pDebugController)
	{
		D3DUtils::SetDebugLayerInfo(m_pD3dDevice);
	}


	// 2. SetUp
	// 2-1. Create Command Queue
	{
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

		hr = m_pD3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(m_pCommandQueue.GetAddressOf()));
		if (FAILED(hr))
		{
			__debugbreak();
			return bResult;
		}
	}

	// 2-2. Create Descripter Heap
	CreateDescripterHeap();

	// 2-3. SwapChain
	RECT rect;
	::GetClientRect(hWnd, &rect);
	DWORD dwWndWidth = rect.right - rect.left;
	DWORD dwWndHeight = rect.bottom - rect.top;
	DWORD dwBackBufferWidth = dwWndWidth;
	DWORD dwBackBufferHeight = dwWndHeight;

	{
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		swapChainDesc.Width = dwBackBufferWidth;
		swapChainDesc.Height = dwBackBufferHeight;
		swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = SWAP_CHAIN_FRAME_COUNT;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.Scaling = DXGI_SCALING_NONE;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
		swapChainDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
		m_dwSwapChainFlags = swapChainDesc.Flags;

		DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
		fsSwapChainDesc.Windowed = TRUE;

		ComPtr<IDXGISwapChain1> pSwapChain1;
		hr = pFactory->CreateSwapChainForHwnd(m_pCommandQueue.Get(), hWnd, &swapChainDesc, &fsSwapChainDesc, nullptr, pSwapChain1.GetAddressOf());
		if (FAILED(hr))
		{
			__debugbreak();
			return bResult;
		}
		
		hr = pSwapChain1->QueryInterface(IID_PPV_ARGS(m_pSwapChain.GetAddressOf()));
		if (FAILED(hr))
		{
			__debugbreak();
			return bResult;
		}
		
		m_uiRenderTargetIndex = m_pSwapChain->GetCurrentBackBufferIndex();
	}

	// 2-4. m_pSwapChain �� RTV �� m_pRTVHeap���� ����
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pRTVHeap->GetCPUDescriptorHandleForHeapStart());

	for (UINT n = 0; n < SWAP_CHAIN_FRAME_COUNT; n++)
	{
		m_pSwapChain->GetBuffer(n, IID_PPV_ARGS(m_pRenderTargets[n].GetAddressOf()));
		m_pD3dDevice->CreateRenderTargetView(m_pRenderTargets[n].Get(), nullptr, rtvHandle);
		rtvHandle.Offset(1, m_uiRTVDescriptorSize);
	}

	// 2-5. Command List
	CreateCommandList();

	// 2-6. Fence
	CreateFence();

	bResult = TRUE;

	return bResult;
}

void CD3D12Renderer::BeginRender()
{
	// ȭ�� Ŭ����
	if (FAILED(m_pCommandAllocator->Reset()))
	{
		__debugbreak();
	}

	if (FAILED(m_pCommandList->Reset(m_pCommandAllocator.Get(), nullptr)))
	{
		__debugbreak();
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pRTVHeap->GetCPUDescriptorHandleForHeapStart(), m_uiRenderTargetIndex, m_uiRTVDescriptorSize);
	m_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pRenderTargets[m_uiRenderTargetIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
	// �� �ڵ忡�� l-value ������ �߻��ϸ� ������Ʈ ���� -> C/C++ -> Launguage ���� Conformance Mode �� Off
	// �����غ��� �ȵǴ°� ���ذ� �ȵǴ°� �ƴ����� MS�� ���� ���ÿ����� ���Ǵ� ���
	// �ش� ���ÿ����� ���� �߻��� �� ����� ������
	// https://stackoverflow.com/questions/65315241/how-can-i-fix-requires-l-value


	// Ŀ�ǵ� ���
	const float BackColor[] = { 0.f,0.f,1.f,1.f };
	m_pCommandList->ClearRenderTargetView(rtvHandle, BackColor, 0, nullptr);
}

void CD3D12Renderer::EndRender()
{
	// ������Ʈ�� ������
	m_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pRenderTargets[m_uiRenderTargetIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	m_pCommandList->Close();

	ComPtr<ID3D12CommandList> ppCommandLists[] = { m_pCommandList };
	m_pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists->GetAddressOf());
}

void CD3D12Renderer::Present()
{
	// Back buffer �� Primary buffer �� ����

	UINT m_SyncInterval = 1;	// V-Sync ON, 0�� ��� OFF

	UINT uiSyncInterval = m_SyncInterval;
	UINT uiPresentFlags = 0;

	if (!uiSyncInterval)
	{
		uiPresentFlags = DXGI_PRESENT_ALLOW_TEARING; // ȭ�� ������ ���, ����ؾ� V-Sync �� ������ �� �� ����
	}
	
	HRESULT hr;
	hr = m_pSwapChain->Present(uiSyncInterval, uiPresentFlags);

	if (hr == DXGI_ERROR_DEVICE_REMOVED)
	{
		__debugbreak();;
	}

	m_uiRenderTargetIndex = m_pSwapChain->GetCurrentBackBufferIndex();

	Fence();
	WaitForFenceValue();

}

BOOL CD3D12Renderer::CreateDescripterHeap()
{
	// ���� Ÿ�ٿ� ��ũ���� ��
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = SWAP_CHAIN_FRAME_COUNT; // ���� / �ĸ���� 2��
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	
	HRESULT hr = m_pD3dDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(m_pRTVHeap.GetAddressOf()));
	if (FAILED(hr))
	{
		__debugbreak();
		return FALSE;
	}

	m_uiRTVDescriptorSize = m_pD3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);	// 32 in this system

	return TRUE;
}

BOOL CD3D12Renderer::CreateCommandList()
{
	HRESULT hr;
	hr = m_pD3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_pCommandAllocator.GetAddressOf()));
	if (FAILED(hr))
	{
		__debugbreak();
		return FALSE;
	}

	hr = m_pD3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_pCommandAllocator.Get(), nullptr, IID_PPV_ARGS(m_pCommandList.GetAddressOf()));
	if (FAILED(hr))
	{
		__debugbreak();
		return FALSE;
	}

	m_pCommandList->Close();

	return TRUE;
}

BOOL CD3D12Renderer::CreateFence()
{
	HRESULT hr;
	hr = m_pD3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_pFence.GetAddressOf()));
	if (FAILED(hr))
	{
		__debugbreak();
		return FALSE;
	}

	m_ui64FenceValue = 0;

	m_hFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

	return TRUE;
}

UINT64 CD3D12Renderer::Fence()
{
	// ������ �۾��� Fence �� �ɾ� �������� Ȯ�� ����
	m_ui64FenceValue++;
	m_pCommandQueue->Signal(m_pFence.Get(), m_ui64FenceValue);
	return m_ui64FenceValue;
}

void CD3D12Renderer::WaitForFenceValue()
{
	const UINT64 ExpectedFenceValue = m_ui64FenceValue;

	// ���� �������� �۾��� ���������� �����
	if (m_pFence->GetCompletedValue() < ExpectedFenceValue)
	{
		m_pFence->SetEventOnCompletion(ExpectedFenceValue, m_hFenceEvent);
		WaitForSingleObject(m_hFenceEvent, INFINITE);
	}
}
