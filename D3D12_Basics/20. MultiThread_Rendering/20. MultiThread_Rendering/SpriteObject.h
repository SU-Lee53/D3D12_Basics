#pragma once

enum SPRITE_DESCRIPTOR_INDEX
{
	SPRITE_DESCRIPTOR_INDEX_CBV = 0,
	SPRITE_DESCRIPTOR_INDEX_TEX = 1
};

class D3D12Renderer;

class SpriteObject
{
public:
	const static UINT DESCRIPTOR_COUNT_FOR_DRAW = 2;

public:
	SpriteObject();
	~SpriteObject();
	BOOL Initialize(std::shared_ptr<D3D12Renderer> pRenderer);
	BOOL Initialize(std::shared_ptr<D3D12Renderer> pRenderer, const WCHAR* wchFilename, const RECT* pRect);

private:
	// Init Internal
	BOOL InitCommonResources();
	BOOL InitRootSignature();
	BOOL InitPipelineState();
	BOOL InitMesh();

public:
	// Draw
	void DrawWithTex(ComPtr<ID3D12GraphicsCommandList>& prefCommandList, const XMFLOAT2& refPos, const XMFLOAT2& refScale, const RECT* pRect, float Z, const TEXTURE_HANDLE* pTexHandle);
	void Draw(ComPtr<ID3D12GraphicsCommandList>& prefCommandList, const XMFLOAT2& refPos, const XMFLOAT2& refScale, float z);

private:
	void CleanUp();
	void CleanUpSharedResources();

private:

	// 모든 SpriteObject 객체들이 아래 3가지 + Vertex/Index Buffer 
	static ComPtr<ID3D12RootSignature> m_pRootSignature;
	static ComPtr<ID3D12PipelineState> m_pPipelineState;
	static DWORD m_dwInitRefCount;

	// Vertex
	static ComPtr<ID3D12Resource> m_pVertexBuffer;
	static D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;

	// Index
	static ComPtr<ID3D12Resource> m_pIndexBuffer;
	static D3D12_INDEX_BUFFER_VIEW m_IndexBufferView;

	std::weak_ptr<TEXTURE_HANDLE> m_pTexHandle;
	std::weak_ptr<D3D12Renderer> m_pRenderer;
	RECT m_Rect = {};
	XMFLOAT2 m_Scale = { 1.f, 1.f };


};

