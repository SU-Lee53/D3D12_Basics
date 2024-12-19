#pragma once

enum BASIC_MESH_DESCRIPTOR_INDEX
{
	BASIC_MESH_DESCRIPTOR_INDEX_TEX = 0
};


class D3D12Renderer;

class BasicMeshObject
{
public:
	BasicMeshObject();
	~BasicMeshObject();

	BOOL Initialize(std::shared_ptr<D3D12Renderer> pRenderer);
	void Draw(ComPtr<ID3D12GraphicsCommandList> pCommandList);
	BOOL CreateMesh();

private:
	BOOL InitCommonResources();

	BOOL InitRootSignature();
	BOOL InitPipelineState();
	
	BOOL CreateDescriptorTable();

private:
	// 모든 BasicMeshObject 아래 3가지를 객체들이 공유함
	static ComPtr<ID3D12RootSignature>	m_pRootSignature;
	static ComPtr<ID3D12PipelineState>	m_pPipelineState;
	static DWORD						m_dwInitRefCount;

	std::shared_ptr<D3D12Renderer> m_pRenderer = nullptr;

	ComPtr<ID3D12Resource> m_pVertexBuffer = nullptr;
	D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView = {};
	
	ComPtr<ID3D12Resource> m_pIndexBuffer = nullptr;
	D3D12_INDEX_BUFFER_VIEW m_IndexBufferView = {};

	ComPtr<ID3D12Resource> m_pTexResource = nullptr;

	// Descriptor
	UINT m_srvDescriptorSize = 0;
	ComPtr<ID3D12DescriptorHeap> m_pDescriptorHeap = nullptr;
	
	const static UINT DESCRIPTOR_COUNT_FOR_DRAW = 1;

};

