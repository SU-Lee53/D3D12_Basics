#pragma once

class D3D12ResourceManager
{
public:
	D3D12ResourceManager();
	~D3D12ResourceManager();

public:
	BOOL Initialize(ComPtr<ID3D12Device5> pD3DDevice);
	HRESULT CreateVertexBuffer(UINT SizePerVertex, DWORD dwVertexNum, D3D12_VERTEX_BUFFER_VIEW& pOutVertexBufferView, ComPtr<ID3D12Resource>& ppOutBuffer, void* pInitData);

public:
	void CreateFence();
	void CreateCommandList();

public:
	UINT64 Fence();
	void WaitForFenceValue();

private:
	ComPtr<ID3D12Device5>			m_pD3DDevice = nullptr;
	ComPtr<ID3D12CommandQueue>		m_pCommandQueue = nullptr;
	ComPtr<ID3D12CommandAllocator>	m_pCommandAllocator = nullptr;
	ComPtr<ID3D12GraphicsCommandList>		m_pCommandList = nullptr;

	HANDLE					m_hFenceEvent = nullptr;
	ComPtr<ID3D12Fence>		m_pFence = nullptr;
	UINT64					m_ui64FenceValue = 0;

};

