#pragma once

#include "Renderer_typedef.h"

class D3D12ResourceManager;
class DescriptorPool;
class SimpleConstantBufferPool;
class SingleDescriptorAllocator;
class ConstantBufferManager;
class FontManager;
class TextureManager;
class RenderQueue;
class CommandListPool;

#define USE_MULTIPLE_COMMAND_LIST

class D3D12Renderer : public std::enable_shared_from_this<D3D12Renderer>
{
	const static UINT MAX_DRAW_COUNT_PER_FRAME = 1024;
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

	BOOL CreateFence();
	BOOL CreateDepthStencil(UINT width, UINT height);

public:
	// Camera Functions
	void InitCamera();
	void SetCamera(const XMVECTOR& refCamPos, const XMVECTOR& refCamDir, const XMVECTOR& refCamUp);
	void GetCameraPos(float& reffOutX, float& reffOutY, float& reffOutZ);
	void MoveCamera(float x, float y, float z);
	void SetCameraPos(float x, float y, float z);

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
	std::shared_ptr<void> CreateTextureFromFile(const std::wstring& wchFilename);
	// 종료시가 아닌 런타임에 임의로 텍스쳐를 제거할 수도 있음
	void DeleteTexture(std::shared_ptr<void>& pHandle);

	std::shared_ptr<void> CreateDynamicTexture(UINT TexWidth, UINT TexHeight);
	void UpdateTextureWithImage(std::shared_ptr<void>& pTexHandle, const BYTE* pSrcBits, UINT srcWidth, UINT srcHeight);

	// Text Functions
	std::shared_ptr<void> CreateFontObject(const std::wstring& wchFontFamilyName, float fFontSize);
	void	DeleteFontObject(std::shared_ptr<void>& pFontHandle);
	BOOL	WriteTextToBitmap(BYTE* pDestImage, UINT DestWidth, UINT DestHeight, UINT DestPitch, int& piOutWidth, int& piOutHeight, std::shared_ptr<void>& pFontObjHandle, const WCHAR* wchString, DWORD dwLen);

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
	DWORD GetDPI() { return m_fDPI; }
	
	DWORD GetCommandListCount();

private:
	void	CleanUp();
	void	CleanupFence();
	void	CleanupDescriptorHeapForRTV();
	void	CleanupDescriptorHeapForDSV();



private:
	HWND m_hWnd = nullptr;
	ComPtr<ID3D12Device5> m_pD3DDevice = nullptr;
	
	// Com for Command Queue
	ComPtr<ID3D12CommandQueue>			m_pCommandQueue = nullptr;

	// Resource for Rendering
	
	// Commdand Allocator-List 쌍은 이제 CommandListPool 이 관리
	//ComPtr<ID3D12CommandAllocator>		m_pCommandAllocators[MAX_PENDING_FRAME_COUNT] = {};
	//ComPtr<ID3D12GraphicsCommandList>		m_pCommandLists[MAX_PENDING_FRAME_COUNT] = {};
	std::shared_ptr<CommandListPool>		m_pCommandListPools[MAX_PENDING_FRAME_COUNT] = {};
	std::shared_ptr<DescriptorPool>			m_pDescriptorPools[MAX_PENDING_FRAME_COUNT] = {};
	std::shared_ptr<ConstantBufferManager>	m_pConstantBufferManagers[MAX_PENDING_FRAME_COUNT] = {};

	UINT64 m_ui64LastFenceValues[MAX_PENDING_FRAME_COUNT] = {};
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
	float			m_fDPI = 96.f;

	// Resources
	std::array<ComPtr<ID3D12Resource>, SWAP_CHAIN_FRAME_COUNT> m_pRenderTargets;
	ComPtr<ID3D12Resource>			m_pDepthStencil = nullptr;
	ComPtr<ID3D12DescriptorHeap>	m_pRTVHeap = nullptr;
	ComPtr<ID3D12DescriptorHeap>	m_pDSVHeap = nullptr;
	ComPtr<ID3D12DescriptorHeap>	m_pSRVHeap = nullptr;
	ComPtr<ID3D12Fence>				m_pFence = nullptr;
	HANDLE							m_hFenceEvent = nullptr;
	UINT							m_uiRTVDescriptorSize = 0;
	UINT							m_uiSRVDescriptorSize = 0;
	UINT							m_uiDSVDescriptorSize = 0;
	UINT							m_uiRenderTargetIndex = 0;
	DWORD							m_dwCurContextIndex = 0;

	// Camera Variables
	XMMATRIX m_matView = {};
	XMMATRIX m_matProj = {};

	XMVECTOR m_CamPos = {};
	XMVECTOR m_CamDir = {};


	std::shared_ptr<D3D12ResourceManager>		m_pResourceManager = nullptr;
	std::shared_ptr<SingleDescriptorAllocator>	m_pSingleDescriptorAllocator = nullptr;
	std::shared_ptr<FontManager>				m_pFontManager = nullptr;
	std::shared_ptr<TextureManager>				m_pTextureManager = nullptr;
	std::shared_ptr<RenderQueue>				m_pRenderQueue = nullptr;
};

