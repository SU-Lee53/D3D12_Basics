#pragma once

#include "IndexCreator.h"

class SingleDescriptorAllocator
{
public:
	SingleDescriptorAllocator();
	~SingleDescriptorAllocator();

	BOOL Initialize(ComPtr<ID3D12Device5>& pDevice, DWORD dwMaxCount, D3D12_DESCRIPTOR_HEAP_FLAGS Flags);
	BOOL AllocDescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE& refOutCPUHandle);
	void FreeDescriptorHandle(const D3D12_CPU_DESCRIPTOR_HANDLE& crefDescriptorHandle);
	BOOL Check(const D3D12_CPU_DESCRIPTOR_HANDLE& crefDescriptorHandle);
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandleFromCPUHandle(const D3D12_CPU_DESCRIPTOR_HANDLE& crefCPUHandle);
	ComPtr<ID3D12DescriptorHeap>& GetDescriptorHeap() { return m_pHeap; }

private:
	ComPtr<ID3D12Device5>			m_pD3DDevice = nullptr;
	ComPtr<ID3D12DescriptorHeap>	m_pHeap = nullptr;
	IndexCreator					m_IndexCreator;
	UINT							m_DescriptorSize = 0;

};

