#include "pch.h"
#include "D3D12Renderer.h"
#include "D3D12ResourceManager.h"
#include "BasicMeshObject.h"
#include "DescriptorPool.h"
#include "SimpleConstantBufferPool.h"
#include "SingleDescriptorAllocator.h"
#include "ConstantBufferManager.h"
#include "SpriteObject.h"
#include "FontManager.h"
#include "TextureManager.h"
#include "RenderQueue.h"
#include "CommandListPool.h"
#include <dxgidebug.h>

using namespace std;

D3D12Renderer::D3D12Renderer()
{
}

D3D12Renderer::~D3D12Renderer()
{
	CleanUp();
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
	m_fDPI = ::GetDpiForWindow(hWnd);

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
				pDebugController5.Reset();
			}
		}
	}

	// 1-2 DXGIFactory
	ComPtr<IDXGIFactory4> pFactory = nullptr;
	ComPtr<IDXGIAdapter1> pAdapter = nullptr;
	DXGI_ADAPTER_DESC1 adapterDesc = {};

	CreateDXGIFactory2(dwCreateFactoryFlags, IID_PPV_ARGS(pFactory.GetAddressOf()));

	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_12_2,
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0
	};


	/*
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
				m_FeatureLevel = featureLevels[featureLevelIndex];
				pAdapter->GetDesc1(&m_AdapterDesc);
				break;
			}
		}
	}
	*/

	DWORD featureLevelsCount = _countof(featureLevels);

	for (DWORD featerLevelIndex = 0; featerLevelIndex < featureLevelsCount; featerLevelIndex++)
	{
		UINT adapterIndex = 0;
		while (DXGI_ERROR_NOT_FOUND != pFactory->EnumAdapters1(adapterIndex, &pAdapter))
		{
			pAdapter->GetDesc1(&adapterDesc);

			if (SUCCEEDED(D3D12CreateDevice(pAdapter.Get(), featureLevels[featerLevelIndex], IID_PPV_ARGS(m_pD3DDevice.GetAddressOf()))))
			{
				break;
			}
			pAdapter.Reset();
			adapterIndex++;
		}

		if (m_pD3DDevice.Get())
			break;
	}


	// check if m_pD3DDevice is created
	if (!m_pD3DDevice.Get())
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
	CreateDescripterHeapForRTV();

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
	m_uiSRVDescriptorSize = m_pD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// Depth Stencil Buffer 생성
	CreateDescripterHeapForDSV();
	CreateDepthStencil(m_dwWidth, m_dwHeight);

	// 2-5. Command List -> CommandList Pool 이 관리하므로 SKIP
	//CreateCommandList();

	// 2-6. Fence
	CreateFence();

	m_pFontManager = make_shared<FontManager>();
	m_pFontManager->Initialize(shared_from_this(), m_pCommandQueue, 1024, 256, bEnableDebugLayer);

	m_pResourceManager = make_shared<D3D12ResourceManager>();
	m_pResourceManager->Initialize(m_pD3DDevice);

	m_pTextureManager = make_shared<TextureManager>();
	m_pTextureManager->Initialize(shared_from_this(), 1024 / 16, 1024);

	for (DWORD i = 0; i < MAX_PENDING_FRAME_COUNT; i++)
	{
		m_pCommandListPools[i] = make_shared<CommandListPool>();
		m_pCommandListPools[i]->Initialize(m_pD3DDevice, D3D12_COMMAND_LIST_TYPE_DIRECT, 256); 

		m_pDescriptorPools[i] = make_shared<DescriptorPool>();
		m_pDescriptorPools[i]->Initialize(m_pD3DDevice, MAX_DRAW_COUNT_PER_FRAME * BasicMeshObject::MAX_DESCRIPTOR_COUNT_FOR_DRAW);

		m_pConstantBufferManagers[i] = make_shared<ConstantBufferManager>();
		m_pConstantBufferManagers[i]->Initialize(m_pD3DDevice, MAX_DRAW_COUNT_PER_FRAME);
	}
	m_pSingleDescriptorAllocator = make_shared<SingleDescriptorAllocator>();
	m_pSingleDescriptorAllocator->Initialize(m_pD3DDevice, MAX_DESCRIPTOR_COUNT, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

	m_pRenderQueue = make_shared<RenderQueue>();
	m_pRenderQueue->Initialize(shared_from_this(), 8192);

	// 카메라 생성
	InitCamera();

	bResult = TRUE;
	if (pDebugController)
	{
		pDebugController.Reset();
	}
	if (pAdapter)
	{
		pAdapter.Reset();
	}
	if (pFactory)
	{
		pFactory.Reset();
	}
	return bResult;
}

void D3D12Renderer::BeginRender()
{
	// 화면 클리어 + 이번 프레임 렌더링을 위한 자료구조 초기화
	//ComPtr<ID3D12CommandAllocator> pCommandAllocator = m_pCommandAllocators[m_dwCurContextIndex];
	//ComPtr<ID3D12GraphicsCommandList> pCommandList = m_pCommandLists[m_dwCurContextIndex];

	// CommandList 는 이제 CommandListPool 에서 관리
	shared_ptr<CommandListPool> pCommandListPool = m_pCommandListPools[m_dwCurContextIndex];
	ComPtr<ID3D12GraphicsCommandList> pCommandList = pCommandListPool->GetCurrentCommandList();

	pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pRenderTargets[m_uiRenderTargetIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
	
	// RTV, DSV 의 Descriptor 위치 가져옴
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pRTVHeap->GetCPUDescriptorHandleForHeapStart(), m_uiRenderTargetIndex, m_uiRTVDescriptorSize);
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_pDSVHeap->GetCPUDescriptorHandleForHeapStart());

	// 커맨드 기록
	XMFLOAT3 color;
	XMStoreFloat3(&color, DirectX::Colors::DarkKhaki);
	//const float BackColor[] = { 0.f,0.f,1.f,1.f };
	pCommandList->ClearRenderTargetView(rtvHandle, (float*)&color, 0, nullptr);
	pCommandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);	// Depth 클리어

	// 바로 실행
	pCommandListPool->CloseAndExecute(m_pCommandQueue);

	// 아래는 이제 CommandList 를 Process 할때 들어감
	//pCommandList->RSSetViewports(1, &m_ViewPort);
	//pCommandList->RSSetScissorRects(1, &m_ScissorRect);
	//pCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
}

void D3D12Renderer::EndRender()
{
	// 지금 사용할 CommandListPool 을 가져옴
	shared_ptr<CommandListPool> pCommandListPool = m_pCommandListPools[m_dwCurContextIndex];

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pRTVHeap->GetCPUDescriptorHandleForHeapStart(), m_uiRenderTargetIndex, m_uiRTVDescriptorSize);
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_pDSVHeap->GetCPUDescriptorHandleForHeapStart());

	// 렌더링 큐에 쌓여있는 렌더링 요청을 한번에 처리
	// USE_MULTIPLE_COMMAND_LIST 매크로가 선언되어있다면 CommandList 에 커맨드를 400개씩 나눠담아 실행하도록 함

#ifdef USE_MULTIPLE_COMMAND_LIST
	m_pRenderQueue->Process(pCommandListPool, m_pCommandQueue, 400, rtvHandle, dsvHandle, m_ViewPort, m_ScissorRect);
#else
	m_pRenderQueue->Process(pCommandListPool, m_pCommandQueue, (DWORD)(-1), rtvHandle, dsvHandle, m_ViewPort, m_ScissorRect);
#endif

	// Present
	ComPtr<ID3D12GraphicsCommandList> pCommandList = pCommandListPool->GetCurrentCommandList();
	pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pRenderTargets[m_uiRenderTargetIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	pCommandListPool->CloseAndExecute(m_pCommandQueue);
	
	m_pRenderQueue->Reset();
}

void D3D12Renderer::Present()
{
	Fence();

	// Back buffer 를 Primary buffer 로 전송

	UINT m_SyncInterval = 0;	// V-Sync ON, 0일 경우 OFF

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
		__debugbreak();
	}

	m_uiRenderTargetIndex = m_pSwapChain->GetCurrentBackBufferIndex();

	// 비동기 렌더링을 위해 다음 프레임을 준비
	DWORD dwNextCondextIndex = (m_dwCurContextIndex + 1) % MAX_PENDING_FRAME_COUNT;
	WaitForFenceValue(m_ui64LastFenceValues[dwNextCondextIndex]);

	// 강의에 설명이 없는데 안하면 터짐
	m_pConstantBufferManagers[dwNextCondextIndex]->Reset();
	m_pDescriptorPools[dwNextCondextIndex]->Reset();
	m_pCommandListPools[dwNextCondextIndex]->Reset();
	m_dwCurContextIndex = dwNextCondextIndex;
}

BOOL D3D12Renderer::UpdateWindowSize(DWORD dwBackBufferWidth, DWORD dwBackBufferHeight)
{
	BOOL bResult = FALSE;

	if (!(dwBackBufferWidth * dwBackBufferHeight))
		return FALSE;

	if (m_dwWidth == dwBackBufferWidth && m_dwHeight == dwBackBufferHeight)
		return FALSE;

	// 현재 진행중인 그래픽 커맨드들이 모두 끝날때까지 잠깐 대기
	Fence();

	for (DWORD i = 0; i < MAX_PENDING_FRAME_COUNT; i++)
	{
		WaitForFenceValue(m_ui64LastFenceValues[i]);
	}

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

BOOL D3D12Renderer::CreateDescripterHeapForRTV()
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

BOOL D3D12Renderer::CreateDescripterHeapForDSV()
{
	BOOL bResult = FALSE;

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	HRESULT hr = m_pD3DDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(m_pDSVHeap.GetAddressOf()));
	if (FAILED(hr))
	{
		__debugbreak();
		return bResult;
	}

	m_uiDSVDescriptorSize = m_pD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	bResult = TRUE;

	return bResult;
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

BOOL D3D12Renderer::CreateDepthStencil(UINT width, UINT height)
{
	D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
	depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

	// clear value 를 지정하여 약간의 최적화에 도움을 줄 수 있음
	D3D12_CLEAR_VALUE depthOptimizeClearValue = {};
	depthOptimizeClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	depthOptimizeClearValue.DepthStencil.Depth = 1.f;
	depthOptimizeClearValue.DepthStencil.Stencil = 0;

	D3D12_RESOURCE_DESC depthDesc;
	depthDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthDesc.Alignment = 0;
	depthDesc.Width = width;
	depthDesc.Height = height;
	depthDesc.DepthOrArraySize = 1;
	depthDesc.MipLevels = 1;
	depthDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	depthDesc.SampleDesc.Count = 1;
	depthDesc.SampleDesc.Quality = 0;
	depthDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	if (FAILED(m_pD3DDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&depthDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthOptimizeClearValue,
		IID_PPV_ARGS(m_pDepthStencil.GetAddressOf()))))
	{
		__debugbreak();
		return FALSE;
	}

	m_pDepthStencil->SetName(L"D3D12Renderer::m_pDepthStencil");	// 디버그용 이름 설정

	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_pDSVHeap->GetCPUDescriptorHandleForHeapStart());
	m_pD3DDevice->CreateDepthStencilView(m_pDepthStencil.Get(), &depthStencilDesc, dsvHandle);

	return TRUE;
}

void D3D12Renderer::InitCamera()
{
	// 카메라 파라미터 EYE, AT, UP
	m_CamPos = XMVectorSet(0.f, 0.f, -1.f, 1.f);
	m_CamDir = XMVectorSet(0.f, 0.f, 1.f, 0.f);
	XMVECTOR Up = XMVectorSet(0.f, 1.f, 0.f, 0.f);

	SetCamera(m_CamPos, m_CamDir, Up);


}

void D3D12Renderer::SetCamera(const XMVECTOR& refCamPos, const XMVECTOR& refCamDir, const XMVECTOR& refCamUp)
{
	// View
	m_matView = XMMatrixLookAtLH(refCamPos, refCamDir, refCamUp);

	// 시야각 fovY (라디안)
	float fovY = XM_PIDIV4;	// 90도(위아래 45도)

	// Projection
	float fAspectRatio = (float)m_dwWidth / (float)m_dwHeight;
	float fNear = 0.1f;
	float fFar = 1000.f;

	m_matProj = XMMatrixPerspectiveFovLH(fovY, fAspectRatio, fNear, fFar);
}

void D3D12Renderer::GetCameraPos(float& reffOutX, float& reffOutY, float& reffOutZ)
{
	reffOutX = m_CamPos.m128_f32[0];
	reffOutY = m_CamPos.m128_f32[1];
	reffOutZ = m_CamPos.m128_f32[2];
}

void D3D12Renderer::MoveCamera(float x, float y, float z)
{
	m_CamPos.m128_f32[0] += x;
	m_CamPos.m128_f32[1] += y;
	m_CamPos.m128_f32[2] += z;
	XMVECTOR Up = XMVectorSet(0.f, 1.f, 0.f, 0.f);

	SetCamera(m_CamPos, m_CamDir, Up);
}

void D3D12Renderer::SetCameraPos(float x, float y, float z)
{
	m_CamPos.m128_f32[0] = x;
	m_CamPos.m128_f32[1] = y;
	m_CamPos.m128_f32[2] = z;
	XMVECTOR Up = XMVectorSet(0.f, 1.f, 0.f, 0.f);

	SetCamera(m_CamPos, m_CamDir, Up);
}

UINT64 D3D12Renderer::Fence()
{
	// 제출한 작업에 Fence 를 걸어 끝났는지 확인 가능
	m_ui64FenceValue++;
	m_pCommandQueue->Signal(m_pFence.Get(), m_ui64FenceValue);
	m_ui64LastFenceValues[m_dwCurContextIndex] = m_ui64FenceValue;
	return m_ui64FenceValue;
}

void D3D12Renderer::WaitForFenceValue(UINT64 ExpectedFenceValue)
{
	// 원하는 프레임의 작업이 끝날때까지 대기함
	if (m_pFence->GetCompletedValue() < ExpectedFenceValue)
	{
		m_pFence->SetEventOnCompletion(ExpectedFenceValue, m_hFenceEvent);
		WaitForSingleObject(m_hFenceEvent, INFINITE);
	}
}

shared_ptr<void> D3D12Renderer::CreateBasicMeshObject()
{
	shared_ptr<BasicMeshObject> pMeshObj = make_shared<BasicMeshObject>();
	pMeshObj->Initialize(shared_from_this());
	return pMeshObj;
}

void D3D12Renderer::DeleteBasicMeshObject(shared_ptr<void>& pMeshObjHandle)
{
	for (DWORD i = 0; i < MAX_PENDING_FRAME_COUNT; i++)
	{
		WaitForFenceValue(m_ui64LastFenceValues[i]);
	}

	shared_ptr<BasicMeshObject> pMeshObj = static_pointer_cast<BasicMeshObject>(pMeshObjHandle);
	pMeshObj.reset();
}

BOOL D3D12Renderer::BeginCreateMesh(shared_ptr<void>& prefMeshObjHandle, const BasicVertex* pVertexList, DWORD dwVertexCount, DWORD dwTriGroupCount)
{
	shared_ptr<BasicMeshObject> pMeshObj = static_pointer_cast<BasicMeshObject>(prefMeshObjHandle);
	BOOL bResult = pMeshObj->BeginCreateMesh(pVertexList, dwVertexCount, dwTriGroupCount);
	return bResult;
}

BOOL D3D12Renderer::InsertTriGroup(shared_ptr<void>& prefMeshObjHandle, const WORD* pIndexList, DWORD dwTriCount, const WCHAR* wchTexFilename)
{
	shared_ptr<BasicMeshObject> pMeshObj = static_pointer_cast<BasicMeshObject>(prefMeshObjHandle);
	BOOL bResult = pMeshObj->InsertTriGroup(pIndexList, dwTriCount, wchTexFilename);
	return bResult;
}

void D3D12Renderer::EndCreateMesh(shared_ptr<void>& prefMeshObjHandle)
{
	shared_ptr<BasicMeshObject> pMeshObj = static_pointer_cast<BasicMeshObject>(prefMeshObjHandle);
	pMeshObj->EndCreateMesh();
}

void D3D12Renderer::RenderMeshObject(shared_ptr<void>& pMeshObjHandle, const XMMATRIX& refMatWorld)
{
	RENDER_MESH_OBJ_PARAM param;
	param.matWorld = refMatWorld;

	RENDER_ITEM item;
	item.Type = RENDER_ITEM_TYPE_MESH_OBJ;
	item.pObjHandle = pMeshObjHandle;
	item.Param = param;

	if (!m_pRenderQueue->Add(make_shared<RENDER_ITEM>(item)))
		__debugbreak();
}

shared_ptr<void> D3D12Renderer::CreateSpriteObject()
{
	shared_ptr<SpriteObject> pSprObj = make_shared<SpriteObject>();
	pSprObj->Initialize(shared_from_this());

	return static_pointer_cast<void>(pSprObj);
}

shared_ptr<void> D3D12Renderer::CreateSpriteObject(const wstring wstrFilename, int PosX, int PosY, int Width, int Height)
{
	shared_ptr<SpriteObject> pSprObj = make_shared<SpriteObject>();

	RECT rect;
	rect.left = PosX;
	rect.top = PosY;
	rect.right = Width;
	rect.bottom = Height;
	pSprObj->Initialize(shared_from_this(), wstrFilename.c_str(), &rect);

	return static_pointer_cast<void>(pSprObj);
}

void D3D12Renderer::DeleteSpriteObject(shared_ptr<void> pSpriteHandle)
{
	for (DWORD i = 0; i < MAX_PENDING_FRAME_COUNT; i++)
	{
		WaitForFenceValue(m_ui64LastFenceValues[i]);
	}

	shared_ptr<SpriteObject> pSprObj = static_pointer_cast<SpriteObject>(pSpriteHandle);
	pSprObj.reset();	// need to check ref_count
}

void D3D12Renderer::RenderSpriteWithTex(shared_ptr<void>& pSpriteHandle, int iPosX, int iPosY, float fScaleX, float fScaleY, const RECT* pRect, float Z, shared_ptr<void>& pTexHandle)
{
	RENDER_SPRITE_PARAM param;
	{
		param.iPosX = iPosX;
		param.iPosY = iPosY;
		param.fScaleX = fScaleX;
		param.fScaleY = fScaleY;
		
		if (pRect)
		{
			param.bUseRect = TRUE;
			param.Rect = *pRect;
		}
		else
		{
			param.bUseRect = FALSE;
			param.Rect = {};
		}

		param.pTexHandle = pTexHandle;
		param.Z = Z;
	}

	RENDER_ITEM item;
	item.Type = RENDER_ITEM_TYPE_SPRITE;
	item.pObjHandle = pSpriteHandle;
	item.Param = param;

	if (!m_pRenderQueue->Add(make_shared<RENDER_ITEM>(item)))
		__debugbreak;

}

void D3D12Renderer::RenderSprite(shared_ptr<void>& pSpriteHandle, int iPosX, int iPosY, float fScaleX, float fScaleY, float Z)
{
	RENDER_SPRITE_PARAM param;
	{
		param.iPosX = iPosX;
		param.iPosY = iPosY;
		param.fScaleX = fScaleX;
		param.fScaleY = fScaleY;
		param.bUseRect = FALSE;
		param.Rect = {};
		param.pTexHandle = nullptr;
		param.Z = Z;
	}

	RENDER_ITEM item;
	item.Type = RENDER_ITEM_TYPE_SPRITE;
	item.pObjHandle = pSpriteHandle;
	item.Param = param;

	if (!m_pRenderQueue->Add(make_shared<RENDER_ITEM>(item)))
		__debugbreak;

}

shared_ptr<void> D3D12Renderer::CreateTiledTexture(UINT TexWidth, UINT TexHeight, DWORD r, DWORD g, DWORD b)
{
	DXGI_FORMAT TexFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	BYTE* pImage = (BYTE*)malloc(TexWidth * TexHeight * 4); // RGBA
	memset(pImage, 0, TexWidth * TexHeight * 4);

	BOOL bFirstColorIsWHite = TRUE;

	for (UINT y = 0; y < TexHeight; y++)
	{
		for (UINT x = 0; x < TexWidth; x++)
		{
			RGBA* pDest = (RGBA*)(pImage + (x + y * TexWidth) * 4);

			if ((bFirstColorIsWHite + x) % 2)
			{
				pDest->r = r;
				pDest->g = g;
				pDest->b = b;
				pDest->a = 255;
			}
			else
			{
				pDest->r = 0;
				pDest->g = 0;
				pDest->b = 0;
			}
			pDest->a = 255;
		}
		bFirstColorIsWHite++;
		bFirstColorIsWHite %= 2;
	}
	shared_ptr<TEXTURE_HANDLE> pTexHandle = m_pTextureManager->CreateImmutableTexture(TexWidth, TexHeight, TexFormat, pImage);

	free(pImage);
	pImage = nullptr;

	return pTexHandle;
}

shared_ptr<void> D3D12Renderer::CreateTextureFromFile(const wstring& wchFilename)
{
	shared_ptr<TEXTURE_HANDLE> pTexHandle = m_pTextureManager->CreateTextureFromFile(wchFilename);
	return pTexHandle;
}

void D3D12Renderer::DeleteTexture(shared_ptr<void>& pHandle)
{
	for (DWORD i = 0; i < MAX_PENDING_FRAME_COUNT; i++)
	{
		WaitForFenceValue(m_ui64LastFenceValues[i]);
	}
	m_pTextureManager->DeleteTexture(static_pointer_cast<TEXTURE_HANDLE>(pHandle));
}

shared_ptr<void> D3D12Renderer::CreateDynamicTexture(UINT TexWidth, UINT TexHeight)
{
	shared_ptr<TEXTURE_HANDLE> pTexHandle = m_pTextureManager->CreateDynamicTexture(TexWidth, TexHeight);
	return pTexHandle;
}

void D3D12Renderer::UpdateTextureWithImage(shared_ptr<void>& pTexHandle, const BYTE* pSrcBits, UINT srcWidth, UINT srcHeight)
{
	shared_ptr<TEXTURE_HANDLE> pTextureHandle = static_pointer_cast<TEXTURE_HANDLE>(pTexHandle);
	ComPtr<ID3D12Resource> pDescTexResource = pTextureHandle->pTexResource;
	ComPtr<ID3D12Resource> pUploadBuffer = pTextureHandle->pUploadBuffer;

	D3D12_RESOURCE_DESC Desc = pDescTexResource->GetDesc();
	if (srcWidth > Desc.Width)
	{
		__debugbreak();
		return;
	}

	if (srcHeight > Desc.Height)
	{
		__debugbreak();
		return;
	}

	D3D12_PLACED_SUBRESOURCE_FOOTPRINT FootPrint;
	UINT Rows = 0;
	UINT64 RowSize = 0;
	UINT64 TotalBytes = 0;

	m_pD3DDevice->GetCopyableFootprints(&Desc, 0, 1, 0, &FootPrint, &Rows, &RowSize, &TotalBytes);

	BYTE* pMappedPtr = nullptr;
	CD3DX12_RANGE writeRange(0, 0);
	
	HRESULT hr = pUploadBuffer->Map(0, &writeRange, reinterpret_cast<void**>(&pMappedPtr));
	if (FAILED(hr))
	{
		__debugbreak();
	}

	const BYTE* pSrc = pSrcBits;
	BYTE* pDest = pMappedPtr;
	for (UINT y = 0; y < srcHeight; y++)
	{
		memcpy(pDest, pSrc, srcWidth * 4);
		pSrc += (srcWidth * 4);
		pDest += FootPrint.Footprint.RowPitch;
	}

	pUploadBuffer->Unmap(0, nullptr);

	pTextureHandle->bUpdated = TRUE;


}

shared_ptr<void> D3D12Renderer::CreateFontObject(const wstring& wchFontFamilyName, float fFontSize)
{
	shared_ptr<FONT_HANDLE> pFontHandle = m_pFontManager->CreateFontObject(wchFontFamilyName, fFontSize);
	return pFontHandle;
}

void D3D12Renderer::DeleteFontObject(shared_ptr<void>& pFontHandle)
{
	m_pFontManager->DeleteFontObject(static_pointer_cast<FONT_HANDLE>(pFontHandle));
}

BOOL D3D12Renderer::WriteTextToBitmap(BYTE* pDestImage, UINT DestWidth, UINT DestHeight, UINT DestPitch, int& refiOutWidth, int& refiOutHeight, shared_ptr<void>& pFontObjHandle, const WCHAR* wchString, DWORD dwLen)
{
	BOOL bResult = m_pFontManager->WriteTextToBitmap(
		pDestImage, 
		DestWidth, 
		DestHeight, 
		DestPitch, 
		refiOutWidth, 
		refiOutHeight, 
		static_pointer_cast<FONT_HANDLE>(pFontObjHandle), 
		wchString, 
		dwLen
	);
	
	return bResult;
}

shared_ptr<SimpleConstantBufferPool>& D3D12Renderer::GetConstantBufferPool(CONSTANT_BUFFER_TYPE type)
{
	return m_pConstantBufferManagers[m_dwCurContextIndex]->GetConstantBufferPool(type);
}

DWORD D3D12Renderer::GetCommandListCount()
{
	DWORD dwCommandListCount = 0;
	for (DWORD i = 0; i < MAX_PENDING_FRAME_COUNT; i++)
	{
		dwCommandListCount = m_pCommandListPools[i]->GetTotalCmdListNum();
	}
	return dwCommandListCount;
}

void D3D12Renderer::CleanUp()
{

	Fence();

	for (DWORD i = 0; i < MAX_PENDING_FRAME_COUNT; i++)
	{
		WaitForFenceValue(m_ui64LastFenceValues[i]);
	}
	for (DWORD i = 0; i < MAX_PENDING_FRAME_COUNT; i++)
	{
		if (m_pCommandListPools[i])
		{
			m_pCommandListPools[i].reset();
		}
		if (m_pConstantBufferManagers[i])
		{
			m_pConstantBufferManagers[i].reset();
		}
		if (m_pDescriptorPools[i])
		{
			m_pDescriptorPools[i].reset();
		}
	}
	if (m_pTextureManager)
	{
		m_pTextureManager.reset();
	}
	if (m_pResourceManager)
	{
		m_pResourceManager.reset();
	}
	if (m_pFontManager)
	{
		m_pFontManager.reset();
	}
	if (m_pSingleDescriptorAllocator)
	{
		m_pSingleDescriptorAllocator.reset();
	}

	CleanupDescriptorHeapForRTV();
	CleanupDescriptorHeapForDSV();

	for (DWORD i = 0; i < SWAP_CHAIN_FRAME_COUNT; i++)
	{
		if (m_pRenderTargets[i])
		{
			m_pRenderTargets[i].Reset();
		}
	}
	if (m_pDepthStencil)
	{
		m_pDepthStencil.Reset();
	}
	if (m_pSwapChain)
	{
		m_pSwapChain.Reset();
	}

	if (m_pCommandQueue)
	{
		m_pCommandQueue.Reset();
	}

	CleanupFence();

	if (m_pD3DDevice)
	{
		ULONG ref_count = m_pD3DDevice.Reset();
		if (ref_count)
		{
			//resource leak!!!
			IDXGIDebug1* pDebug = nullptr;
			if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDebug))))
			{
				pDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_SUMMARY);
				pDebug->Release();
			}
			__debugbreak();
		}

		m_pD3DDevice = nullptr;

	}
}
void D3D12Renderer::CleanupFence()
{
	if (m_hFenceEvent)
	{
		CloseHandle(m_hFenceEvent);
		m_hFenceEvent = nullptr;
	}
	if (m_pFence)
	{
		m_pFence.Reset();
	}
}
void D3D12Renderer::CleanupDescriptorHeapForRTV()
{
	if (m_pRTVHeap)
	{
		m_pRTVHeap.Reset();
	}
}
void D3D12Renderer::CleanupDescriptorHeapForDSV()
{
	if (m_pDSVHeap)
	{
		m_pDSVHeap.Reset();
	}

}