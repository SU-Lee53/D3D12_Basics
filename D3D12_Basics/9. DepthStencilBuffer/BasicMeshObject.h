#pragma once

enum class BASIC_MESH_DESCRIPTOR_INDEX
{
	BASIC_MESH_DESCRIPTOR_INDEX_CBV = 0,
	BASIC_MESH_DESCRIPTOR_INDEX_TEX = 1,

	END
};

struct CONSTANT_BUFFER_DEFAULT;
class D3D12Renderer;

class BasicMeshObject
{
public:
	const static UINT DESCRIPTOR_COUNT_FOR_DRAW = (UINT)BASIC_MESH_DESCRIPTOR_INDEX::END;

public:
	BasicMeshObject();
	~BasicMeshObject();

	BOOL Initialize(std::shared_ptr<D3D12Renderer> pRenderer);
	void Draw(ComPtr<ID3D12GraphicsCommandList> pCommandList, const XMFLOAT2& pPos);
	void Draw(ComPtr<ID3D12GraphicsCommandList> pCommandList, const XMFLOAT2& pPos, const D3D12_CPU_DESCRIPTOR_HANDLE& srv);
	BOOL CreateMesh();

private:
	BOOL InitCommonResources();

	BOOL InitRootSignature();
	BOOL InitPipelineState();

private:
	// 모든 BasicMeshObject 아래 3가지를 객체들이 공유함
	static ComPtr<ID3D12RootSignature>	m_pRootSignature;
	static ComPtr<ID3D12PipelineState>	m_pPipelineState;
	static DWORD						m_dwInitRefCount;

	std::shared_ptr<D3D12Renderer> m_pRenderer = nullptr;

	// Vertex, Index
	ComPtr<ID3D12Resource> m_pVertexBuffer = nullptr;
	D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView = {};
	
	ComPtr<ID3D12Resource> m_pIndexBuffer = nullptr;
	D3D12_INDEX_BUFFER_VIEW m_IndexBufferView = {};
};

