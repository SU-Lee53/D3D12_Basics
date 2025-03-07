#pragma once

const UINT SWAP_CHAIN_FRAME_COUNT = 4;

// Pending : "보류중인" 이라는 뜻
// 화면에 그려지는 전면버퍼 하나 빼고 나머지 후면버퍼들의 갯수를 말함
const UINT MAX_PENDING_FRAME_COUNT = SWAP_CHAIN_FRAME_COUNT - 1;


class D3D12ResourceManager;
class DescriptorPool;
class SimpleConstantBufferPool;
class SingleDescriptorAllocator;
class ConstantBufferManager;

class D3D12Renderer : public std::enable_shared_from_this<D3D12Renderer>
{
	const static UINT MAX_DRAW_COUNT_PER_FRAME = 256;
	static const UINT MAX_DESCRIPTOR_COUNT = 4096;

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
	BOOL CreateDescripterHeapForRTV();
	BOOL CreateDescripterHeapForDSV();

	BOOL CreateCommandList();
	BOOL CreateFence();
	BOOL CreateDepthStencil(UINT width, UINT height);

	void InitCamera();

private:
	UINT64 Fence();
	void WaitForFenceValue(UINT64 ExpectedFenceValue);

public:
	// Basic Mesh Functions
	std::shared_ptr<void> CreateBasicMeshObject();
	void DeleteBasicMeshObject(std::shared_ptr<void>& pMeshObjHandle);

	BOOL BeginCreateMesh(std::shared_ptr<void>& prefMeshObjHandle, const BasicVertex* pVertexList, DWORD dwVertexCount, DWORD dwTriGroupCount);
	BOOL InsertTriGroup(std::shared_ptr<void>& prefMeshObjHandle, const WORD* pIndexList, DWORD dwTriCount, const WCHAR* wchTexFilename);
	void EndCreateMesh(std::shared_ptr<void>& prefMeshObjHandle);

	void RenderMeshObject(std::shared_ptr<void>& pMeshObjHandle, const XMMATRIX& refMatWorld);

	// Sprite Functions
	std::shared_ptr<void> CreateSpriteObject();
	std::shared_ptr<void> CreateSpriteObject(const std::wstring wstrFilename, int PosX, int PosY, int Width, int Height);
	void DeleteSpriteObject(std::shared_ptr<void> pSpriteHandle);
	void RenderSpriteWithTex(std::shared_ptr<void>& pSpriteHandle, int iPosX, int iPosY, float fScaleX, float fScaleY, const RECT* pRect, float Z, std::shared_ptr<void>& pTexHandle);
	void RenderSprite(std::shared_ptr<void>& pSpriteHandle, int iPosX, int iPosY, float fScaleX, float fScaleY, float Z);


	// Texture Functions
	std::shared_ptr<void> CreateTiledTexture(UINT TexWidth, UINT TexHeight, DWORD r, DWORD g, DWORD b);
	std::shared_ptr<void> CreateTextureFromFile(const WCHAR* wchFilename);
	// 종료시가 아닌 런타임에 임의로 텍스쳐를 제거할 수도 있음
	void DeleteTexture(std::shared_ptr<void>& pHandle);

	std::shared_ptr<void> CreateDynamicTexture(UINT TexWidth, UINT TexHeight);
	void UpdateTextureWithImage(std::shared_ptr<void>& pTexHandle, const BYTE* pSrcBits, UINT srcWidth, UINT srcHeight);
	
public:
	// Getter
	ComPtr<ID3D12Device5>& GetDevice() { return m_pD3DDevice; }
	std::shared_ptr<D3D12ResourceManager>& GetResourceManager() { return m_pResourceManager; }

	std::shared_ptr<DescriptorPool>& GetDescriptorPool() { return m_pDescriptorPools[m_dwCurContextIndex]; }
	std::shared_ptr<SimpleConstantBufferPool>& GetConstantBufferPool(CONSTANT_BUFFER_TYPE type);

	UINT& GetSrvDescriptorSize() { return m_uiSRVDescriptorSize; }
	std::shared_ptr<SingleDescriptorAllocator>& GetSingleDescriptorAllocator() { return m_pSingleDescriptorAllocator; }

	void GetViewProjMatrix(XMMATRIX& refOutMatView, XMMATRIX& refOutMatProj)
	{
		refOutMatView = XMMatrixTranspose(m_matView);
		refOutMatProj = XMMatrixTranspose(m_matProj);
	}

	DWORD GetScreenWidth() { return m_dwWidth; }
	DWORD GetScreenHeight() { return m_dwHeight; }

private:
	// Texture Allocate
	std::shared_ptr<TEXTURE_HANDLE> AllocTextureHandle();
	void FreeTextureHandle(std::shared_ptr<TEXTURE_HANDLE>& pTexHandle);


private:
	HWND m_hWnd = nullptr;
	ComPtr<ID3D12Device5> m_pD3DDevice = nullptr;
	
	// Com for Command Queue
	ComPtr<ID3D12CommandQueue>			m_pCommandQueue = nullptr;

	// Resource for Rendering
	std::array<ComPtr<ID3D12CommandAllocator>, MAX_PENDING_FRAME_COUNT>				m_pCommandAllocators = {};
	std::array<ComPtr<ID3D12GraphicsCommandList>, MAX_PENDING_FRAME_COUNT>			m_pCommandLists = {};
	std::array<std::shared_ptr<DescriptorPool>, MAX_PENDING_FRAME_COUNT>			m_pDescriptorPools = {};
	std::array<std::shared_ptr<ConstantBufferManager>, MAX_PENDING_FRAME_COUNT>		m_pConstantBufferManagers = {};
	std::array<UINT64, MAX_PENDING_FRAME_COUNT>										m_ui64LastFenceValues = {};
	UINT64 m_ui64FenceValue = 0;

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
	ComPtr<ID3D12Resource>			m_pDepthStencil = nullptr;
	ComPtr<ID3D12DescriptorHeap>	m_pRTVHeap = nullptr;
	ComPtr<ID3D12DescriptorHeap>	m_pDSVHeap = nullptr;	// 아직 미사용
	ComPtr<ID3D12DescriptorHeap>	m_pSRVHeap = nullptr;
	ComPtr<ID3D12Fence>				m_pFence = nullptr;
	HANDLE							m_hFenceEvent = nullptr;
	UINT							m_uiRTVDescriptorSize = 0;
	UINT							m_uiSRVDescriptorSize = 0;
	UINT							m_uiDSVDescriptorSize = 0;	// 아직 미사용
	UINT							m_uiRenderTargetIndex = 0;
	DWORD							m_dwCurContextIndex = 0;

	XMMATRIX m_matView = {};
	XMMATRIX m_matProj = {};

	std::shared_ptr<D3D12ResourceManager>		m_pResourceManager = nullptr;
	std::shared_ptr<SingleDescriptorAllocator>	m_pSingleDescriptorAllocator = nullptr;

	// Allocated Texture List
	SORT_LINK* m_pTexLinkHead = nullptr;
	SORT_LINK* m_pTexLinkTail = nullptr;


};

