#pragma once


class DescriptorPool
{
public:
	DescriptorPool();
	~DescriptorPool();

	BOOL Initialize(ComPtr<ID3D12Device5>& pD3DDevice, UINT MaxDescriptorCount);

	BOOL AllocDescriptorTable(D3D12_CPU_DESCRIPTOR_HANDLE& refOutCPUDescriptor, D3D12_GPU_DESCRIPTOR_HANDLE& refOutGPUDescriptor, UINT DescriptorCount);
	void Reset();
	
	ComPtr<ID3D12DescriptorHeap>& GetDescriptorHeap() { return m_pDescriptorHeap; }

private:
	void CleanUp();

private:
	ComPtr<ID3D12Device5> m_pD3DDevice = nullptr;

	UINT m_AllocatedDescriptorCount = 0;
	UINT m_MaxDescriptorCount = 0;
	UINT m_srvDescriptorSize = 0;

	ComPtr<ID3D12DescriptorHeap> m_pDescriptorHeap = nullptr;

	D3D12_CPU_DESCRIPTOR_HANDLE	m_CpuDescriptorHandle = {};
	D3D12_GPU_DESCRIPTOR_HANDLE	m_GpuDescriptorHandle = {};

};

