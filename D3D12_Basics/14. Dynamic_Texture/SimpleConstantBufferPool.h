#pragma once

// Constant Buffer 를 다루기 위한 정보들의 구조체
struct CB_CONTAINER
{
	D3D12_CPU_DESCRIPTOR_HANDLE CBVHandle;
	D3D12_GPU_VIRTUAL_ADDRESS	pGPUMemAddr;
	UINT8*		pSysMemAddr;
};

class SimpleConstantBufferPool
{
public:
	SimpleConstantBufferPool();
	~SimpleConstantBufferPool();

	BOOL Initialize(ComPtr<ID3D12Device5>& pD3DDevice, CONSTANT_BUFFER_TYPE type, UINT SizePerCBV, UINT MaxCBVNum);

	CB_CONTAINER* Alloc();
	void Reset();

private:
	std::vector<CB_CONTAINER> m_pCBContainerList = {};

	ComPtr<ID3D12DescriptorHeap>	m_pCBVHeap = nullptr;
	ComPtr<ID3D12Resource>			m_pResource = nullptr;
	CONSTANT_BUFFER_TYPE			m_ConstantBufferType;
	UINT8*	m_pSystemMemAddr = nullptr;
	UINT	m_SizePerCBV = 0;
	UINT	m_MaxCBVNum = 0;
	UINT	m_AllocatedCBVNum = 0;



};

