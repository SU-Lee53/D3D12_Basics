#pragma once

const UINT SWAP_CHAIN_FRAME_COUNT = 2;

class D3D12Renderer
{
public:
	D3D12Renderer();
	~D3D12Renderer();

public:
	BOOL Initialize(HWND hWnd, BOOL bEnableDebugLayer, BOOL bEnableGBV);
	void BeginRender();
	void EndRender();
	void Present();

private:
	BOOL CreateDescripterHeap();
	BOOL CreateCommandList();
	BOOL CreateFence();

private:
	UINT64 Fence();
	void WaitForFenceValue();

private:
	HWND m_hWnd = nullptr;
	ComPtr<ID3D12Device5> m_pD3dDevice = nullptr;
	
	// Com for Command Queue
	ComPtr<ID3D12CommandQueue>			m_pCommandQueue = nullptr;
	ComPtr<ID3D12CommandAllocator>		m_pCommandAllocator = nullptr;
	ComPtr<ID3D12GraphicsCommandList>	m_pCommandList = nullptr;
	UINT64								m_ui64FenceValue;

	// DXGI
	D3D_FEATURE_LEVEL		m_featureLevel = D3D_FEATURE_LEVEL_11_0;
	DXGI_ADAPTER_DESC1		m_adapterDesc = {};
	ComPtr<IDXGISwapChain3>	m_pSwapChain = nullptr;
	DWORD					m_dwSwapChainFlags = 0;

	// Resources
	std::array<ComPtr<ID3D12Resource>, SWAP_CHAIN_FRAME_COUNT> m_pRenderTargets;
	ComPtr<ID3D12DescriptorHeap>	m_pRTVHeap = nullptr;
	ComPtr<ID3D12DescriptorHeap>	m_pDSVHeap = nullptr;
	ComPtr<ID3D12DescriptorHeap>	m_pSRVHeap = nullptr;
	ComPtr<ID3D12Fence>				m_pFence = nullptr;
	HANDLE							m_hFenceEvent = nullptr;
	UINT							m_uiRTVDescriptorSize = 0;
	UINT							m_uiRenderTargetIndex = 0;
	DWORD							_dwCurContextIndex = 0;

};

