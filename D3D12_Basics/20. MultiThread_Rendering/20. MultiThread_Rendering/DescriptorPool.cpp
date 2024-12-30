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

	// Descriptor Heap 생성
	D3D12_DESCRIPTOR_HEAP_DESC commonHeapDesc = {};
	commonHeapDesc.NumDescriptors = m_MaxDescriptorCount;
	commonHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	commonHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;	// Descriptor Pool 의 Descriptor 내용이 파이프라인으로 들어감. Constant Buffer Pool 의 Descriptor 는 여기로 복사됨
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
		// DescriptorPool 의 크기를 벗어났다면 바로 FALSE 리턴
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
	// 현재 렌더러는 다음프레임을 그릴때 이전 프레임이 그려질때까지 기다림
	// 그래서, 이전 프레임에 사용된 Descriptor 들은 쓸모없으므로 그냥 Count 를 0으로 내리고 처음부터 Descriptor 를 가져가도 무방함
	m_AllocatedDescriptorCount = 0;
}

void DescriptorPool::CleanUp()
{
	if (m_pDescriptorHeap)
	{
		m_pDescriptorHeap.Reset();
	}
}
