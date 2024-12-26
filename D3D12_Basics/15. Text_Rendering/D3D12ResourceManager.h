#pragma once

class D3D12ResourceManager
{
public:
	D3D12ResourceManager();
	~D3D12ResourceManager();

public:
	BOOL Initialize(ComPtr<ID3D12Device5>& pD3DDevice);
	HRESULT CreateVertexBuffer(UINT SizePerVertex, DWORD dwVertexNum, D3D12_VERTEX_BUFFER_VIEW& refOutVertexBufferView, ComPtr<ID3D12Resource>& prefOutBuffer, void* pInitData);
	HRESULT CreateIndexBuffer(DWORD dwIndexNum, D3D12_INDEX_BUFFER_VIEW& refOutIndexBufferView, ComPtr<ID3D12Resource>& prefOutBuffer, void* pInitData);

	BOOL CreateTexture(ComPtr<ID3D12Resource>& prefOutResource, UINT width, UINT height, DXGI_FORMAT format, BYTE* pInitImage);
	BOOL CreateTextureFromFile(ComPtr<ID3D12Resource>& prefOutResource, D3D12_RESOURCE_DESC& refOutDesc, const WCHAR* wchFilename);
	BOOL CreateTexturePair(ComPtr<ID3D12Resource>& prefOutResource, ComPtr<ID3D12Resource>& prefOutUploadBuffer, UINT width, UINT height, DXGI_FORMAT format);
	void UpdateTextureForWrite(ComPtr<ID3D12Resource> pDescTexResource, ComPtr<ID3D12Resource> pSrcTexResource);

public:
	void CreateFence();
	void CreateCommandList();

public:
	UINT64 Fence();
	void WaitForFenceValue();

private:
	ComPtr<ID3D12Device5>					m_pD3DDevice = nullptr;
	ComPtr<ID3D12CommandQueue>				m_pCommandQueue = nullptr;
	ComPtr<ID3D12CommandAllocator>			m_pCommandAllocator = nullptr;
	ComPtr<ID3D12GraphicsCommandList>		m_pCommandList = nullptr;

	HANDLE					m_hFenceEvent = nullptr;
	ComPtr<ID3D12Fence>		m_pFence = nullptr;
	UINT64					m_ui64FenceValue = 0;

};

