#include "pch.h"
#include "D3D12Renderer.h"
#include "D3D12ResourceManager.h"
#include "BasicMeshObject.h"
#include "D3DUtil.h"
#include "DescriptorPool.h"
#include "SimpleConstantBufferPool.h"
#include "SingleDescriptorAllocator.h"
#include "ConstantBufferManager.h"
#include "SpriteObject.h"


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
				m_FeatureLevel = featureLevels[featureLevelIndex];
				pAdapter->GetDesc1(&m_AdapterDesc);
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

	// 2-5. Command List
	CreateCommandList();

	// 2-6. Fence
	CreateFence();

	m_pResourceManager = std::make_shared<D3D12ResourceManager>();
	m_pResourceManager->Initialize(m_pD3DDevice);

	for (DWORD i = 0; i < MAX_PENDING_FRAME_COUNT; i++)
	{
		m_pDescriptorPools[i] = std::make_shared<DescriptorPool>();
		m_pDescriptorPools[i]->Initialize(m_pD3DDevice, MAX_DRAW_COUNT_PER_FRAME * BasicMeshObject::MAX_DESCRIPTOR_COUNT_FOR_DRAW);

		m_pConstantBufferManagers[i] = std::make_shared<ConstantBufferManager>();
		m_pConstantBufferManagers[i]->Initialize(m_pD3DDevice, BasicMeshObject::MAX_DESCRIPTOR_COUNT_FOR_DRAW);
	}
	m_pSingleDescriptorAllocator = std::make_shared<SingleDescriptorAllocator>();
	m_pSingleDescriptorAllocator->Initialize(m_pD3DDevice, MAX_DESCRIPTOR_COUNT, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

	// 카메라 생성
	InitCamera();

	bResult = TRUE;

	return bResult;
}

void D3D12Renderer::BeginRender()
{
	// 화면 클리어
	ComPtr<ID3D12CommandAllocator> pCommandAllocator = m_pCommandAllocators[m_dwCurContextIndex];
	ComPtr<ID3D12GraphicsCommandList> pCommandList = m_pCommandLists[m_dwCurContextIndex];

	if (FAILED(pCommandAllocator->Reset()))
	{
		__debugbreak();
	}

	if (FAILED(pCommandList->Reset(pCommandAllocator.Get(), nullptr)))
	{
		__debugbreak();
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pRTVHeap->GetCPUDescriptorHandleForHeapStart(), m_uiRenderTargetIndex, m_uiRTVDescriptorSize);
	
	pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pRenderTargets[m_uiRenderTargetIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
	// 위 코드에서 l-value 에러가 발생하면 프로젝트 설정 -> C/C++ -> Launguage 에서 Conformance Mode 를 Off
	// 생각해보면 안되는게 이해가 안되는건 아니지만 MS의 공식 샘플에서도 사용되는 방식
	// 해당 샘플에서도 오류 발생시 위 방법을 권장함
	// https://stackoverflow.com/questions/65315241/how-can-i-fix-requires-l-value

	// DSV Heap 의 선두번지 가져옴
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_pDSVHeap->GetCPUDescriptorHandleForHeapStart());

	// 커맨드 기록
	XMFLOAT3 color;
	XMStoreFloat3(&color, DirectX::Colors::DarkKhaki);
	//const float BackColor[] = { 0.f,0.f,1.f,1.f };
	pCommandList->ClearRenderTargetView(rtvHandle, (float*)&color, 0, nullptr);
	pCommandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);	// Depth 클리어

	pCommandList->RSSetViewports(1, &m_ViewPort);
	pCommandList->RSSetScissorRects(1, &m_ScissorRect);
	pCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
}

void D3D12Renderer::EndRender()
{
	ComPtr<ID3D12GraphicsCommandList> pCommandList = m_pCommandLists[m_dwCurContextIndex];

	// 지오메트리 렌더링
	pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pRenderTargets[m_uiRenderTargetIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	pCommandList->Close();

	ComPtr<ID3D12CommandList> ppCommandLists[] = { pCommandList };
	m_pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists->GetAddressOf());
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
		__debugbreak();;
	}

	m_uiRenderTargetIndex = m_pSwapChain->GetCurrentBackBufferIndex();

	// 비동기 렌더링을 위해 다음 프레임을 준비
	DWORD dwNextCondextIndex = (m_dwCurContextIndex + 1) % MAX_PENDING_FRAME_COUNT;
	WaitForFenceValue(m_ui64LastFenceValues[dwNextCondextIndex]);

	// 강의에 설명이 없는데 안하면 터짐
	m_pConstantBufferManagers[dwNextCondextIndex]->Reset();
	m_pDescriptorPools[dwNextCondextIndex]->Reset();
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

BOOL D3D12Renderer::CreateCommandList()
{
	HRESULT hr;

	for (DWORD i = 0; i < MAX_PENDING_FRAME_COUNT; i++)
	{
		hr = m_pD3DDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_pCommandAllocators[i].GetAddressOf()));
		if (FAILED(hr))
		{
			__debugbreak();
			return FALSE;
		}

		hr = m_pD3DDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_pCommandAllocators[i].Get(), nullptr, IID_PPV_ARGS(m_pCommandLists[i].GetAddressOf()));
		if (FAILED(hr))
		{
			__debugbreak();
			return FALSE;
		}

		m_pCommandLists[i]->Close();
	}

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
	XMVECTOR eyePos = XMVectorSet(0.f, 0.f, -1.f, 1.f);	// 카메라 위치
	XMVECTOR eyeDir = XMVectorSet(0.f, 0.f, 1.f, 0.f);	// 카메라 방향(정면)
	XMVECTOR upDir = XMVectorSet(0.f, 1.f, 0.f, 0.f);	// 카메라 위쪽 방향

	// View
	m_matView = XMMatrixLookAtLH(eyePos, eyeDir, upDir);

	// 시야각 fovY (라디안)
	float fovY = XM_PIDIV4;	// 90도(위아래 45도)

	// Projection
	float fAspectRatio = (float)m_dwWidth / (float)m_dwHeight;
	float fNear = 0.1f;
	float fFar = 1000.f;

	m_matProj = XMMatrixPerspectiveFovLH(fovY, fAspectRatio, fNear, fFar);

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

std::shared_ptr<void> D3D12Renderer::CreateBasicMeshObject()
{
	std::shared_ptr<BasicMeshObject> pMeshObj = std::make_shared<BasicMeshObject>();
	pMeshObj->Initialize(shared_from_this());
	return pMeshObj;
}

void D3D12Renderer::DeleteBasicMeshObject(std::shared_ptr<void>& pMeshObjHandle)
{
	for (DWORD i = 0; i < MAX_PENDING_FRAME_COUNT; i++)
	{
		WaitForFenceValue(m_ui64LastFenceValues[i]);
	}

	std::shared_ptr<BasicMeshObject> pMeshObj = std::static_pointer_cast<BasicMeshObject>(pMeshObjHandle);
	pMeshObj.reset();
	pMeshObj = nullptr;
}

BOOL D3D12Renderer::BeginCreateMesh(std::shared_ptr<void>& prefMeshObjHandle, const BasicVertex* pVertexList, DWORD dwVertexCount, DWORD dwTriGroupCount)
{
	std::shared_ptr<BasicMeshObject> pMeshObj = std::static_pointer_cast<BasicMeshObject>(prefMeshObjHandle);
	BOOL bResult = pMeshObj->BeginCreateMesh(pVertexList, dwVertexCount, dwTriGroupCount);
	return bResult;
}

BOOL D3D12Renderer::InsertTriGroup(std::shared_ptr<void>& prefMeshObjHandle, const WORD* pIndexList, DWORD dwTriCount, const WCHAR* wchTexFilename)
{
	std::shared_ptr<BasicMeshObject> pMeshObj = std::static_pointer_cast<BasicMeshObject>(prefMeshObjHandle);
	BOOL bResult = pMeshObj->InsertTriGroup(pIndexList, dwTriCount, wchTexFilename);
	return bResult;
}

void D3D12Renderer::EndCreateMesh(std::shared_ptr<void>& prefMeshObjHandle)
{
	std::shared_ptr<BasicMeshObject> pMeshObj = std::static_pointer_cast<BasicMeshObject>(prefMeshObjHandle);
	pMeshObj->EndCreateMesh();
}

void D3D12Renderer::RenderMeshObject(std::shared_ptr<void>& pMeshObjHandle, const XMMATRIX& refMatWorld)
{
	ComPtr<ID3D12GraphicsCommandList> pCommandList = m_pCommandLists[m_dwCurContextIndex];

	D3D12_CPU_DESCRIPTOR_HANDLE srv = {};
	std::shared_ptr<BasicMeshObject> pMeshObj = std::static_pointer_cast<BasicMeshObject>(pMeshObjHandle);
	
	pMeshObj->Draw(pCommandList, refMatWorld);
}

std::shared_ptr<void> D3D12Renderer::CreateSpriteObject()
{
	std::shared_ptr<SpriteObject> pSprObj = std::make_shared<SpriteObject>();
	pSprObj->Initialize(shared_from_this());

	return std::static_pointer_cast<void>(pSprObj);
}

std::shared_ptr<void> D3D12Renderer::CreateSpriteObject(const std::wstring wstrFilename, int PosX, int PosY, int Width, int Height)
{
	std::shared_ptr<SpriteObject> pSprObj = std::make_shared<SpriteObject>();

	RECT rect;
	rect.left = PosX;
	rect.top = PosY;
	rect.right = Width;
	rect.bottom = Height;
	pSprObj->Initialize(shared_from_this(), wstrFilename.c_str(), &rect);

	return std::static_pointer_cast<void>(pSprObj);
}

void D3D12Renderer::DeleteSpriteObject(std::shared_ptr<void>& pSpriteHandle)
{
	for (DWORD i = 0; i < MAX_PENDING_FRAME_COUNT; i++)
	{
		WaitForFenceValue(m_ui64LastFenceValues[i]);
	}

	std::shared_ptr<SpriteObject> pSprObj = std::static_pointer_cast<SpriteObject>(pSpriteHandle);
	pSprObj.reset();
	pSprObj = nullptr;	
}

void D3D12Renderer::RenderSpriteWithTex(std::shared_ptr<void>& pSpriteHandle, int iPosX, int iPosY, float fScaleX, float fScaleY, const RECT& pRect, float Z, std::shared_ptr<void>& pTexHandle)
{
	ComPtr<ID3D12GraphicsCommandList> pCommandList = m_pCommandLists[m_dwCurContextIndex];
	std::shared_ptr<SpriteObject> pSpriteObj = std::static_pointer_cast<SpriteObject>(pSpriteHandle);

	XMFLOAT2 Pos = { (float)iPosX, (float)iPosY };
	XMFLOAT2 Scale = { fScaleX, fScaleY };

	pSpriteObj->DrawWithTex(pCommandList, Pos, Scale, &pRect, Z, std::static_pointer_cast<TEXTURE_HANDLE>(pTexHandle).get());
}

void D3D12Renderer::RenderSprite(std::shared_ptr<void>& pSpriteHandle, int iPosX, int iPosY, float fScaleX, float fScaleY, float Z)
{
	ComPtr<ID3D12GraphicsCommandList> pCommandList = m_pCommandLists[m_dwCurContextIndex];
	std::shared_ptr<SpriteObject> pSpriteObj = std::static_pointer_cast<SpriteObject>(pSpriteHandle);

	XMFLOAT2 Pos = { (float)iPosX, (float)iPosY };
	XMFLOAT2 Scale = { fScaleX, fScaleY };

	pSpriteObj->Draw(pCommandList, Pos, Scale, Z);
}

std::shared_ptr<void> D3D12Renderer::CreateTiledTexture(UINT TexWidth, UINT TexHeight, DWORD r, DWORD g, DWORD b)
{
	std::shared_ptr<TEXTURE_HANDLE> pTexHandle = nullptr;

	BOOL bResult = FALSE;
	ComPtr<ID3D12Resource> pTexResource = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE srv = {};

	DXGI_FORMAT TexFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	BYTE* pImage = (BYTE*)malloc(TexWidth * TexHeight * 4); // RGBA
	std::memset(pImage, 0, TexWidth * TexHeight * 4);

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

	if (m_pResourceManager->CreateTexture(pTexResource, TexWidth, TexHeight, TexFormat, pImage))
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
		SRVDesc.Format = TexFormat;
		SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		SRVDesc.Texture2D.MipLevels = 1;

		if (m_pSingleDescriptorAllocator->AllocDescriptorHandle(srv))
		{
			m_pD3DDevice->CreateShaderResourceView(pTexResource.Get(), &SRVDesc, srv);

			pTexHandle = std::make_shared<TEXTURE_HANDLE>();
			pTexHandle->pTexResource = pTexResource;
			pTexHandle->srv = srv;
			bResult = true;
		}
	}

	free(pImage);
	pImage = nullptr;

	return pTexHandle;
}

std::shared_ptr<void> D3D12Renderer::CreateTextureFromFile(const WCHAR* wchFilename)
{
	std::shared_ptr<TEXTURE_HANDLE> pTexHandle = nullptr;

	ComPtr<ID3D12Resource> pTexResource = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE srv = {};

	DXGI_FORMAT TexFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	D3D12_RESOURCE_DESC desc = {};
	if (m_pResourceManager->CreateTextureFromFile(pTexResource, desc, wchFilename))
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
		SRVDesc.Format = desc.Format;
		SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		SRVDesc.Texture2D.MipLevels = desc.MipLevels;

		if (m_pSingleDescriptorAllocator->AllocDescriptorHandle(srv))
		{
			m_pD3DDevice->CreateShaderResourceView(pTexResource.Get(), &SRVDesc, srv);

			pTexHandle = std::make_shared<TEXTURE_HANDLE>();
			pTexHandle->pTexResource = pTexResource;
			pTexHandle->srv = srv;
		}
	}

	return pTexHandle;
}

void D3D12Renderer::DeleteTexture(std::shared_ptr<void>& pHandle)
{
	// wait for all commands
	for (DWORD i = 0; i < MAX_PENDING_FRAME_COUNT; i++)
	{
		WaitForFenceValue(m_ui64LastFenceValues[i]);
	}

	std::shared_ptr<TEXTURE_HANDLE> pTexHandle = std::static_pointer_cast<TEXTURE_HANDLE>(pHandle);
	ComPtr<ID3D12Resource> pTexResource = pTexHandle->pTexResource;
	D3D12_CPU_DESCRIPTOR_HANDLE srv = pTexHandle->srv;

	pTexResource->Release();
	m_pSingleDescriptorAllocator->FreeDescriptorHandle(srv);

	pTexHandle.reset();
	pTexHandle = nullptr;
}

std::shared_ptr<SimpleConstantBufferPool>& D3D12Renderer::GetConstantBufferPool(CONSTANT_BUFFER_TYPE type)
{
	std::shared_ptr<ConstantBufferManager> pConstantBufferManager = m_pConstantBufferManagers[m_dwCurContextIndex];
	std::shared_ptr<SimpleConstantBufferPool> pConstantBufferPool = pConstantBufferManager->GetConstantBufferPool(type);
	return pConstantBufferPool;
}

void D3D12Renderer::CleanUp()
{
	// 현재 진행중인 그래픽 커맨드들이 모두 끝날때까지 잠깐 대기
	Fence();

	for (DWORD i = 0; i < MAX_PENDING_FRAME_COUNT; i++)
	{
		WaitForFenceValue(m_ui64LastFenceValues[i]);
	}

	for (DWORD i = 0; i < MAX_PENDING_FRAME_COUNT; i++)
	{
		if (m_pConstantBufferManagers[i])
		{
			m_pConstantBufferManagers[i].reset();
			m_pConstantBufferManagers[i] = nullptr;
		}
		if (m_pDescriptorPools[i])
		{
			m_pDescriptorPools[i].reset();
			m_pDescriptorPools[i] = nullptr;
		}
	}

	if (m_pResourceManager)
	{
		m_pResourceManager.reset();
		m_pResourceManager = nullptr;
	}
	if (m_pSingleDescriptorAllocator)
	{
		m_pSingleDescriptorAllocator.reset();
		m_pSingleDescriptorAllocator = nullptr;
	}

	CleanupDescriptorHeapForRTV();
	CleanupDescriptorHeapForDSV();

	for (DWORD i = 0; i < SWAP_CHAIN_FRAME_COUNT; i++)
	{
		if (m_pRenderTargets[i])
		{
			m_pRenderTargets[i]->Release();
		}
	}
	if (m_pDepthStencil)
	{
		m_pDepthStencil->Release();
	}
	if (m_pSwapChain)
	{
		m_pSwapChain->Release();
	}

	if (m_pCommandQueue)
	{
		m_pCommandQueue->Release();
	}

	CleanupCommandList();
	CleanupFence();

	if (m_pD3DDevice)
	{
		ULONG ref_count = m_pD3DDevice->Release();
		if (ref_count)
		{
			//resource leak!!!
			IDXGIDebug1* pDebug = nullptr;
			if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDebug))))
			{
				pDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_DETAIL);
				pDebug->Release();
			}
			__debugbreak();
		}

		m_pD3DDevice = nullptr;

	}
}

void D3D12Renderer::CleanupDescriptorHeapForRTV()
{
	if (m_pRTVHeap)
	{
		m_pRTVHeap->Release();
	}
}

void D3D12Renderer::CleanupDescriptorHeapForDSV()
{
	if (m_pDSVHeap)
	{
		m_pDSVHeap->Release();
	}
}

void D3D12Renderer::CleanupCommandList()
{
	for (DWORD i = 0; i < MAX_PENDING_FRAME_COUNT; i++)
	{
		if (m_pCommandAllocators[i])
		{
			m_pCommandAllocators[i]->Release();
		}
		if (m_pCommandLists[i])
		{
			m_pCommandLists[i]->Release();
		}
	}
}

void D3D12Renderer::CleanupFence()
{
	if (m_hFenceEvent)
	{
		CloseHandle(m_hFenceEvent);
	}
	if (m_pFence)
	{
		m_pFence->Release();
	}
}
