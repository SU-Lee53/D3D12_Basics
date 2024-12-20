#pragma once

const UINT SWAP_CHAIN_FRAME_COUNT = 2;

class D3D12ResourceManager;
class DescriptorPool;
class SimpleConstantBufferPool;

class D3D12Renderer : public std::enable_shared_from_this<D3D12Renderer>
{
	const static UINT MAX_DRAW_COUNT_PER_FRAME = 256;

public:
	D3D12Renderer();
	~D3D12Renderer();

public:
	BOOL Initialize(HWND hWnd, BOOL bEnableDebugLayer, BOOL bEnableGBV);
	void BeginRender();
	void EndRender();
	void Present();
	BOOL UpdateWindowSize(DWORD dwBackBufferWidth, DWORD dwBackBufferHeight);

private:
	BOOL CreateDescripterHeap();
	BOOL CreateCommandList();
	BOOL CreateFence();

private:
	UINT64 Fence();
	void WaitForFenceValue();

public:
	std::shared_ptr<void> CreateBasicMeshObject();
	void DeleteBasicMeshObject(std::shared_ptr<void>& pMeshObjHandle);
	void RenderMeshObject(std::shared_ptr<void>& pMeshObjHandle, float x_offset, float y_offset);


public:
	// Getter
	ComPtr<ID3D12Device5>& GetDevice() { return m_pD3DDevice; }
	std::shared_ptr<D3D12ResourceManager>& GetResourceManager() { return m_pResourceManager; }
	std::shared_ptr<DescriptorPool>& GetDescriptorPool() { return m_pDescriptorPool; }
	std::shared_ptr<SimpleConstantBufferPool>& GetConstantBufferPool() { return m_pConstantBufferPool; }
	UINT& GetSrvDescriptorSize() { return m_uiSRVDescriptorSize; }

private:
	HWND m_hWnd = nullptr;
	ComPtr<ID3D12Device5> m_pD3DDevice = nullptr;
	
	// Com for Command Queue
	ComPtr<ID3D12CommandQueue>			m_pCommandQueue = nullptr;
	ComPtr<ID3D12CommandAllocator>		m_pCommandAllocator = nullptr;
	ComPtr<ID3D12GraphicsCommandList>	m_pCommandList = nullptr;
	UINT64								m_ui64FenceValue = 0;

	// DXGI
	D3D_FEATURE_LEVEL		m_FeatureLevel = D3D_FEATURE_LEVEL_11_0;
	DXGI_ADAPTER_DESC1		m_AdapterDesc = {};
	ComPtr<IDXGISwapChain3>	m_pSwapChain = nullptr;
	DWORD					m_dwSwapChainFlags = 0;

	// Viewport / Scissor Rect
	D3D12_VIEWPORT	m_ViewPort = {};
	D3D12_RECT		m_ScissorRect = {};
	DWORD			m_dwWidth = 0;
	DWORD			m_dwHeight = 0;

	// Resources
	std::array<ComPtr<ID3D12Resource>, SWAP_CHAIN_FRAME_COUNT> m_pRenderTargets;
	ComPtr<ID3D12DescriptorHeap>	m_pRTVHeap = nullptr;
	ComPtr<ID3D12DescriptorHeap>	m_pDSVHeap = nullptr;	// 아직 미사용
	ComPtr<ID3D12DescriptorHeap>	m_pSRVHeap = nullptr;
	ComPtr<ID3D12Fence>				m_pFence = nullptr;
	HANDLE							m_hFenceEvent = nullptr;
	UINT							m_uiRTVDescriptorSize = 0;
	UINT							m_uiSRVDescriptorSize = 0;
	UINT							m_uiDSVDescriptorSize = 0;	// 아직 미사용
	UINT							m_uiRenderTargetIndex = 0;
	DWORD							_dwCurContextIndex = 0;

	std::shared_ptr<D3D12ResourceManager>		m_pResourceManager = nullptr;
	std::shared_ptr<DescriptorPool>				m_pDescriptorPool = nullptr;
	std::shared_ptr<SimpleConstantBufferPool>	m_pConstantBufferPool = nullptr;
};

