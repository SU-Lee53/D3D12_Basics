#include "pch.h"
#include "D3D12Renderer.h"
#include "D3D12ResourceManager.h"
#include "BasicMeshObject.h"
#include "D3DUtil.h"

D3D12Renderer::D3D12Renderer()
{
}

D3D12Renderer::~D3D12Renderer()
{
	WaitForFenceValue();
	// ComPtr을 사용하므로 별도의 CleanUp 은 필요없을것으로 생각됨
}

BOOL D3D12Renderer::Initialize(HWND hWnd, BOOL bEnableDebugLayer, BOOL bEnableGBV)
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


	size_t bestMemory = 0;
	for (UINT adapterIndex = 0;; ++adapterIndex)
	{
		ComPtr<IDXGIAdapter1> pCurAdapter;
		HRESULT hr = pFactory->EnumAdapters1(adapterIndex, pCurAdapter.GetAddressOf());

		if (hr == DXGI_ERROR_NOT_FOUND)
		{
			break;
		}

		DXGI_ADAPTER_DESC1 curAdapterDesc;
		pCurAdapter->GetDesc1(&curAdapterDesc);

		// VRAM 을 메가바이트 단위로 변환
		size_t memory = curAdapterDesc.DedicatedVideoMemory / (1024 * 1024);

		// VRAM 이 가장 큰 어댑터로 선택
		if (memory > bestMemory)
		{
			bestMemory = memory;
			pAdapter = pCurAdapter;
			adapterDesc = curAdapterDesc;
		}
	}

	for (DWORD featureLevelIndex = 0; featureLevelIndex < featureLevels.size(); featureLevelIndex++)
	{
		if (pAdapter)
		{
			if (SUCCEEDED(D3D12CreateDevice(pAdapter.Get(), featureLevels[featureLevelIndex], IID_PPV_ARGS(m_pD3DDevice.GetAddressOf()))))
			{
				break;
			}
		}
	}

	// check if m_pD3DDevice is created
	if (!m_pD3DDevice)
	{
		__debugbreak();
		return bResult;
	}

	m_AdapterDesc = adapterDesc;
	m_hWnd = hWnd;

	if (pDebugController)
	{
		D3DUtils::SetDebugLayerInfo(m_pD3DDevice);
	}


	// 2. SetUp
	// 2-1. Create Command Queue
	{
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

		hr = m_pD3DDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(m_pCommandQueue.GetAddressOf()));
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

	// 추가됨 : Viewport, ScissorRect
	m_ViewPort.Width = (float)dwWndWidth;
	m_ViewPort.Height = (float)dwWndHeight;
	m_ViewPort.MinDepth = 0.f;
	m_ViewPort.MaxDepth = 1.f;

	m_ScissorRect.left = 0;
	m_ScissorRect.top = 0;
	m_ScissorRect.right = dwWndWidth;
	m_ScissorRect.bottom = dwWndHeight;

	m_dwWidth = dwWndWidth;
	m_dwHeight = dwWndHeight;

	// 2-4. m_pSwapChain 의 RTV 를 m_pRTVHeap으로 연결
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pRTVHeap->GetCPUDescriptorHandleForHeapStart());

	for (UINT n = 0; n < SWAP_CHAIN_FRAME_COUNT; n++)
	{
		m_pSwapChain->GetBuffer(n, IID_PPV_ARGS(m_pRenderTargets[n].GetAddressOf()));
		m_pD3DDevice->CreateRenderTargetView(m_pRenderTargets[n].Get(), nullptr, rtvHandle);
		rtvHandle.Offset(1, m_uiRTVDescriptorSize);
	}

	// 2-5. Command List
	CreateCommandList();

	// 2-6. Fence
	CreateFence();

	m_pResourceManager = std::make_shared<D3D12ResourceManager>();
	m_pResourceManager->Initialize(m_pD3DDevice);

	bResult = TRUE;

	return bResult;
}

void D3D12Renderer::BeginRender()
{
	// 화면 클리어
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
	// 위 코드에서 l-value 에러가 발생하면 프로젝트 설정 -> C/C++ -> Launguage 에서 Conformance Mode 를 Off
	// 생각해보면 안되는게 이해가 안되는건 아니지만 MS의 공식 샘플에서도 사용되는 방식
	// 해당 샘플에서도 오류 발생시 위 방법을 권장함
	// https://stackoverflow.com/questions/65315241/how-can-i-fix-requires-l-value


	// 커맨드 기록
	const float BackColor[] = { 0.f,0.f,1.f,1.f };
	m_pCommandList->ClearRenderTargetView(rtvHandle, BackColor, 0, nullptr);

	m_pCommandList->RSSetViewports(1, &m_ViewPort);
	m_pCommandList->RSSetScissorRects(1, &m_ScissorRect);
	m_pCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
}

void D3D12Renderer::EndRender()
{
	// 지오메트리 렌더링
	m_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pRenderTargets[m_uiRenderTargetIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	m_pCommandList->Close();

	ComPtr<ID3D12CommandList> ppCommandLists[] = { m_pCommandList };
	m_pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists->GetAddressOf());
}

void D3D12Renderer::Present()
{
	// Back buffer 를 Primary buffer 로 전송

	UINT m_SyncInterval = 1;	// V-Sync ON, 0일 경우 OFF

	UINT uiSyncInterval = m_SyncInterval;
	UINT uiPresentFlags = 0;

	if (!uiSyncInterval)
	{
		uiPresentFlags = DXGI_PRESENT_ALLOW_TEARING; // 화면 찢어짐 허용, 허용해야 V-Sync 를 완전히 끌 수 있음
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

BOOL D3D12Renderer::UpdateWindowSize(DWORD dwBackBufferWidth, DWORD dwBackBufferHeight)
{
	BOOL bResult = FALSE;

	if (!(dwBackBufferWidth * dwBackBufferHeight))
		return FALSE;

	if (m_dwWidth == dwBackBufferWidth && m_dwHeight == dwBackBufferHeight)
		return FALSE;

	DXGI_SWAP_CHAIN_DESC1 desc;
	if (FAILED(m_pSwapChain->GetDesc1(&desc)))
	{
		__debugbreak();
	}

	for (UINT n = 0; n < SWAP_CHAIN_FRAME_COUNT; n++)
	{
		m_pRenderTargets[n].Reset();
	}

	if (FAILED(m_pSwapChain->ResizeBuffers(SWAP_CHAIN_FRAME_COUNT, dwBackBufferWidth, dwBackBufferHeight, DXGI_FORMAT_R8G8B8A8_UNORM, m_dwSwapChainFlags)))
	{
		__debugbreak();
	}
	m_uiRenderTargetIndex = m_pSwapChain->GetCurrentBackBufferIndex();

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pRTVHeap->GetCPUDescriptorHandleForHeapStart());

	for (UINT n = 0; n < SWAP_CHAIN_FRAME_COUNT; n++)
	{
		m_pSwapChain->GetBuffer(n, IID_PPV_ARGS(m_pRenderTargets[n].GetAddressOf()));
		m_pD3DDevice->CreateRenderTargetView(m_pRenderTargets[n].Get(), nullptr, rtvHandle);
		rtvHandle.Offset(1, m_uiRTVDescriptorSize);
	}

	m_dwWidth = dwBackBufferWidth;
	m_dwHeight = dwBackBufferHeight;
	m_ViewPort.Width = (float)m_dwWidth;
	m_ViewPort.Height = (float)m_dwHeight;
	m_ScissorRect.left = 0;
	m_ScissorRect.top = 0;
	m_ScissorRect.right = m_dwWidth;
	m_ScissorRect.bottom = m_dwHeight;

	return TRUE;
}

BOOL D3D12Renderer::CreateDescripterHeap()
{
	// 렌더 타겟용 디스크립터 힙
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = SWAP_CHAIN_FRAME_COUNT; // 전면 / 후면버퍼 2개
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	
	HRESULT hr = m_pD3DDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(m_pRTVHeap.GetAddressOf()));
	if (FAILED(hr))
	{
		__debugbreak();
		return FALSE;
	}

	m_uiRTVDescriptorSize = m_pD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);	// 32 in this system

	return TRUE;
}

BOOL D3D12Renderer::CreateCommandList()
{
	HRESULT hr;
	hr = m_pD3DDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_pCommandAllocator.GetAddressOf()));
	if (FAILED(hr))
	{
		__debugbreak();
		return FALSE;
	}

	hr = m_pD3DDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_pCommandAllocator.Get(), nullptr, IID_PPV_ARGS(m_pCommandList.GetAddressOf()));
	if (FAILED(hr))
	{
		__debugbreak();
		return FALSE;
	}

	m_pCommandList->Close();

	return TRUE;
}

BOOL D3D12Renderer::CreateFence()
{
	HRESULT hr;
	hr = m_pD3DDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_pFence.GetAddressOf()));
	if (FAILED(hr))
	{
		__debugbreak();
		return FALSE;
	}

	m_ui64FenceValue = 0;

	m_hFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

	return TRUE;
}

UINT64 D3D12Renderer::Fence()
{
	// 제출한 작업에 Fence 를 걸어 끝났는지 확인 가능
	m_ui64FenceValue++;
	m_pCommandQueue->Signal(m_pFence.Get(), m_ui64FenceValue);
	return m_ui64FenceValue;
}

void D3D12Renderer::WaitForFenceValue()
{
	const UINT64 ExpectedFenceValue = m_ui64FenceValue;

	// 이전 프레임의 작업이 끝날때까지 대기함
	if (m_pFence->GetCompletedValue() < ExpectedFenceValue)
	{
		m_pFence->SetEventOnCompletion(ExpectedFenceValue, m_hFenceEvent);
		WaitForSingleObject(m_hFenceEvent, INFINITE);
	}
}

std::shared_ptr<void> D3D12Renderer::CreateBasicMeshObject()
{
	std::shared_ptr<BasicMeshObject> pMeshObj = std::make_shared<BasicMeshObject>();
	pMeshObj->Initialize(shared_from_this());
	pMeshObj->CreateMesh();

	return pMeshObj;
}

void D3D12Renderer::DeleteBasicMeshObject(std::shared_ptr<void>& pMeshObjHandle)
{
	std::shared_ptr<BasicMeshObject> pMeshObj = std::static_pointer_cast<BasicMeshObject>(pMeshObjHandle);
	pMeshObj.reset();
}

void D3D12Renderer::RenderMeshObject(std::shared_ptr<void>& pMeshObjHandle)
{
	std::shared_ptr<BasicMeshObject> pMeshObj = std::static_pointer_cast<BasicMeshObject>(pMeshObjHandle);
	pMeshObj->Draw(m_pCommandList.Get());
}
