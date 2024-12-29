#pragma once

enum RENDER_ITEM_TYPE
{
	RENDER_ITEM_TYPE_MESH_OBJ,
	RENDER_ITEM_TYPE_SPRITE
};

struct RENDER_MESH_OBJ_PARAM
{
	XMMATRIX matWorld;
};
struct RENDER_SPRITE_PARAM
{
	int iPosX;
	int iPosY;
	float fScaleX;
	float fScaleY;
	RECT Rect;
	BOOL bUseRect;
	float Z;
	std::shared_ptr<void> pTexHandle = nullptr;
};

struct RENDER_ITEM
{
	RENDER_ITEM_TYPE Type;
	std::shared_ptr<void> pObjHandle = nullptr;
	std::variant<RENDER_MESH_OBJ_PARAM, RENDER_SPRITE_PARAM> Param;
};

class D3D12Renderer;

class RenderQueue
{
public:
	RenderQueue();
	~RenderQueue();

	BOOL Initialize(std::shared_ptr<D3D12Renderer> pRenderer, DWORD dwMaxItemNum);
	BOOL Add(const std::shared_ptr<RENDER_ITEM> pItem);
	DWORD Process(ComPtr<ID3D12GraphicsCommandList>& prefCommandList);
	void Reset();


private:
	const std::shared_ptr<RENDER_ITEM> Dispatch();
	void CleanUp();

private:
	std::weak_ptr<D3D12Renderer> m_pRenderer;
	std::vector<std::shared_ptr<RENDER_ITEM>> m_pBuffer = {};
	DWORD m_dwMaxBufferNum = 0;
	DWORD m_dwAllocatedNum = 0;
	DWORD m_dwReadBufferIndex = 0;



};

