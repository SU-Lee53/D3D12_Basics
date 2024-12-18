#pragma once

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


private:
	// ��� BasicMeshObject �Ʒ� 3������ ��ü���� ������
	static ComPtr<ID3D12RootSignature>	m_pRootSignature;
	static ComPtr<ID3D12PipelineState>	m_pPipelineState;
	static DWORD						m_dwInitRefCount;

	std::shared_ptr<D3D12Renderer> m_pRenderer = nullptr;

	ComPtr<ID3D12Resource> m_pVertexBuffer = nullptr;
	D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView = {};



};

