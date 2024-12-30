#include "pch.h"
#include "DescriptorPool.h"

DescriptorPool::DescriptorPool()
{
}

DescriptorPool::~DescriptorPool()
{
	CleanUp();
}

BOOL DescriptorPool::Initialize(ComPtr<ID3D12Device5>& pD3DDevice, UINT MaxDescriptorCount)
{
	BOOL bResult = FALSE;
	m_pD3DDevice = pD3DDevice;

	m_MaxDescriptorCount = MaxDescriptorCount;
	m_srvDescriptorSize = m_pD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// Descriptor Heap ����
	D3D12_DESCRIPTOR_HEAP_DESC commonHeapDesc = {};
	commonHeapDesc.NumDescriptors = m_MaxDescriptorCount;
	commonHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	commonHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;	// Descriptor Pool �� Descriptor ������ �������������� ��. Constant Buffer Pool �� Descriptor �� ����� �����
	if (FAILED(m_pD3DDevice->CreateDescriptorHeap(&commonHeapDesc, IID_PPV_ARGS(m_pDescriptorHeap.GetAddressOf()))))
	{
		__debugbreak();
		return bResult;
	}

	m_CpuDescriptorHandle = m_pDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	m_GpuDescriptorHandle = m_pDescriptorHeap->GetGPUDescriptorHandleForHeapStart();

	bResult = TRUE;

	return bResult;
}

BOOL DescriptorPool::AllocDescriptorTable(D3D12_CPU_DESCRIPTOR_HANDLE& refOutCPUDescriptor, D3D12_GPU_DESCRIPTOR_HANDLE& refOutGPUDescriptor, UINT DescriptorCount)
{
	BOOL bResult = FALSE;
	if (m_AllocatedDescriptorCount + DescriptorCount > m_MaxDescriptorCount)
	{
		// DescriptorPool �� ũ�⸦ ����ٸ� �ٷ� FALSE ����
		__debugbreak();
		return bResult;
	}

	//UINT offset = m_AllocatedDescriptorCount + DescriptorCount;

	refOutCPUDescriptor = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_CpuDescriptorHandle, m_AllocatedDescriptorCount, m_srvDescriptorSize);
	refOutGPUDescriptor = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_GpuDescriptorHandle, m_AllocatedDescriptorCount, m_srvDescriptorSize);
	m_AllocatedDescriptorCount += DescriptorCount;

	bResult = TRUE;

	return bResult;
}

void DescriptorPool::Reset()
{
	// ���� �������� ������������ �׸��� ���� �������� �׷��������� ��ٸ�
	// �׷���, ���� �����ӿ� ���� Descriptor ���� ��������Ƿ� �׳� Count �� 0���� ������ ó������ Descriptor �� �������� ������
	m_AllocatedDescriptorCount = 0;
}

void DescriptorPool::CleanUp()
{
	if (m_pDescriptorHeap)
	{
		m_pDescriptorHeap.Reset();
	}
}
