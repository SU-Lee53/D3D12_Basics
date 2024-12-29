#pragma once

enum BASIC_MESH_DESCRIPTOR_INDEX_PER_OBJ
{
	BASIC_MESH_DESCRIPTOR_INDEX_PER_OBJ_CBV = 0
};

enum BASIC_MESH_DESCRIPTOR_INDEX_PER_TRI_GROUP
{
	BASIC_MESH_DESCRIPTOR_INDEX_PER_TRI_GROUP_TEX = 0
};

struct INDEXED_TRI_GROUP
{
	ComPtr<ID3D12Resource> pIndexBuffer = nullptr;
	D3D12_INDEX_BUFFER_VIEW IndexBufferView = {};
	DWORD dwTriCount = 0;
	std::shared_ptr<TEXTURE_HANDLE> pTexHandle = nullptr;
};

class D3D12Renderer;

class BasicMeshObject
{
public:
	const static UINT DESCRIPTOR_COUNT_PER_OBJ = 1;
	const static UINT DESCRIPTOR_COUNT_PER_TRI_GROUP = 1;
	const static UINT MAX_TRI_GROUP_COUNT_PER_OBJ = 8;
	const static UINT MAX_DESCRIPTOR_COUNT_FOR_DRAW = DESCRIPTOR_COUNT_PER_OBJ + (MAX_TRI_GROUP_COUNT_PER_OBJ * DESCRIPTOR_COUNT_PER_TRI_GROUP);

public:
	BasicMeshObject();
	~BasicMeshObject();

	BOOL Initialize(std::shared_ptr<D3D12Renderer> pRenderer);
	void Draw(ComPtr<ID3D12GraphicsCommandList> pCommandList, const XMMATRIX& pMatWorld);
	
public:
	BOOL BeginCreateMesh(const BasicVertex* pVertexList, DWORD dwVertexNum, DWORD dwTriGroupCount);
	BOOL InsertTriGroup(const WORD* pIndexList, DWORD dwTriCount, const WCHAR* wchTexFileName);
	void EndCreateMesh();

private:
	BOOL InitCommonResources();

	BOOL InitRootSignature();
	BOOL InitPipelineState();

private:
	void CleanUp();
	void CleanUpSharedResources();

private:
	// 모든 BasicMeshObject 아래 3가지를 객체들이 공유함
	static ComPtr<ID3D12RootSignature>	m_pRootSignature;
	static ComPtr<ID3D12PipelineState>	m_pPipelineState;
	static DWORD						m_dwInitRefCount;

	std::shared_ptr<D3D12Renderer> m_pRenderer = nullptr;

	// Vertex
	ComPtr<ID3D12Resource> m_pVertexBuffer = nullptr;
	D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView = {};

	std::vector<INDEXED_TRI_GROUP> m_TriGroupList = {};
	DWORD	m_dwTriGroupCount = 0;
	DWORD	m_dwMaxTriGroupCount = 0;
};

