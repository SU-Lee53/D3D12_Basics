#include "pch.h"
#include "SingleDescriptorAllocator.h"

SingleDescriptorAllocator::SingleDescriptorAllocator()
{
}

SingleDescriptorAllocator::~SingleDescriptorAllocator()
{
	CleanUp();
}

BOOL SingleDescriptorAllocator::Initialize(ComPtr<ID3D12Device5>& pDevice, DWORD dwMaxCount, D3D12_DESCRIPTOR_HEAP_FLAGS Flags)
{
	m_pD3DDevice = pDevice;

	D3D12_DESCRIPTOR_HEAP_DESC commonHeapDesc = {};
	commonHeapDesc.NumDescriptors = dwMaxCount;
	commonHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	commonHeapDesc.Flags = Flags;

	if (FAILED(m_pD3DDevice->CreateDescriptorHeap(&commonHeapDesc, IID_PPV_ARGS(m_pHeap.GetAddressOf()))))
	{
		__debugbreak();
		return FALSE;
	}

	m_IndexCreator.Initialize(dwMaxCount);

	m_DescriptorSize = m_pD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	return TRUE;
}

BOOL SingleDescriptorAllocator::AllocDescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE& refOutCPUHandle)
{
	BOOL bResult = FALSE;

	DWORD dwIndex = m_IndexCreator.Alloc();
	if (dwIndex == -1)
	{
		__debugbreak();
		return bResult;
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE DescriptorHandle(m_pHeap->GetCPUDescriptorHandleForHeapStart(), dwIndex, m_DescriptorSize);
	refOutCPUHandle = DescriptorHandle;

	bResult = TRUE;
	return bResult;
}

void SingleDescriptorAllocator::FreeDescriptorHandle(const D3D12_CPU_DESCRIPTOR_HANDLE& crefDescriptorHandle)
{
	D3D12_CPU_DESCRIPTOR_HANDLE base = m_pHeap->GetCPUDescriptorHandleForHeapStart();

#ifdef _DEBUG
	if (base.ptr > crefDescriptorHandle.ptr)
		__debugbreak();
#endif
	DWORD dwIndex = (DWORD)(crefDescriptorHandle.ptr - base.ptr) / m_DescriptorSize;
	m_IndexCreator.Free(dwIndex);
}

BOOL SingleDescriptorAllocator::Check(const D3D12_CPU_DESCRIPTOR_HANDLE& crefDescriptorHandle)
{
	BOOL bResult = FALSE;

	D3D12_CPU_DESCRIPTOR_HANDLE base = m_pHeap->GetCPUDescriptorHandleForHeapStart();
	if (base.ptr > crefDescriptorHandle.ptr)
	{
		__debugbreak();
		return bResult;
	}

	bResult = TRUE;
	return bResult;
}

D3D12_GPU_DESCRIPTOR_HANDLE SingleDescriptorAllocator::GetGPUHandleFromCPUHandle(const D3D12_CPU_DESCRIPTOR_HANDLE& crefCPUHandle)
{
	D3D12_CPU_DESCRIPTOR_HANDLE base = m_pHeap->GetCPUDescriptorHandleForHeapStart();

#ifdef _DEBUG
	if (base.ptr > crefCPUHandle.ptr)
		__debugbreak();
#endif
	DWORD dwIndex = (DWORD)(crefCPUHandle.ptr - base.ptr) / m_DescriptorSize;
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle(m_pHeap->GetGPUDescriptorHandleForHeapStart(), dwIndex, m_DescriptorSize);

	return gpuHandle;

}

void SingleDescriptorAllocator::CleanUp()
{
#ifdef _DEBUG
		m_IndexCreator.Check();
#endif
	if (m_pHeap)
	{
		m_pHeap.Reset();
	}
	if (m_pD3DDevice)
	{
		m_pD3DDevice.Reset();
	}
}
