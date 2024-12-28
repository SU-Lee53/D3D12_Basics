#include "pch.h"
#include "SimpleConstantBufferPool.h"

SimpleConstantBufferPool::SimpleConstantBufferPool()
{
}

SimpleConstantBufferPool::~SimpleConstantBufferPool()
{
}

BOOL SimpleConstantBufferPool::Initialize(ComPtr<ID3D12Device5>& pD3DDevice, CONSTANT_BUFFER_TYPE type, UINT SizePerCBV, UINT MaxCBVNum)
{
	// 여기서는 Constant Buffer Pool 과 Descriptor Heap 을 쌍으로 생성함
	// 여기서 생성되는 Descriptor Heap 은 파이프라인에 들어가지 않고 Descriptor Pool 에서 가져온 Descriptor 에다가 복사하는 Copy Src 일 뿐임

	m_ConstantBufferType = type;
	m_MaxCBVNum = MaxCBVNum;
	m_SizePerCBV = SizePerCBV;
	UINT ByteWidth = SizePerCBV * m_MaxCBVNum;

	// Constant Buffer 생성
	if (FAILED(pD3DDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(ByteWidth),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_pResource.GetAddressOf())
	)))
	{
		__debugbreak();
		return FALSE;
	}

	// Descriptor Heap 생성
	// 위에 설명대로 파이프라인에 들어가지 않으므로 Flag 는 NONE 임
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = m_MaxCBVNum;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	if (FAILED(pD3DDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(m_pCBVHeap.GetAddressOf()))))
	{
		__debugbreak();
		return FALSE;
	}

	// m_pResource 를 뚜껑을 따서(Map) m_pCBContainerList 를 이용해 쪼개서 담아둔다
	// 이전처럼 여기는 계속 갱신될 예정이므로 Unmap 은 하지않는다
	CD3DX12_RANGE writeRange(0, 0);
	m_pResource->Map(0, &writeRange, reinterpret_cast<void**>(&m_pSystemMemAddr));

	m_pCBContainerList.resize(m_MaxCBVNum);

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = m_pResource->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = m_SizePerCBV;

	UINT8* pSystemMemPtr = m_pSystemMemAddr;
	CD3DX12_CPU_DESCRIPTOR_HANDLE heapHandle(m_pCBVHeap->GetCPUDescriptorHandleForHeapStart());

	UINT DescriptorSize = pD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	for (DWORD i = 0; i < m_MaxCBVNum; i++)
	{
		pD3DDevice->CreateConstantBufferView(&cbvDesc, heapHandle);

		m_pCBContainerList[i].CBVHandle = heapHandle;
		m_pCBContainerList[i].pGPUMemAddr = cbvDesc.BufferLocation;
		m_pCBContainerList[i].pSysMemAddr = pSystemMemPtr;

		heapHandle.Offset(1, DescriptorSize);
		cbvDesc.BufferLocation += m_SizePerCBV;
		pSystemMemPtr += m_SizePerCBV;
	}

	return TRUE;
}

CB_CONTAINER* SimpleConstantBufferPool::Alloc()
{
	CB_CONTAINER* pCB = nullptr;

	if (m_AllocatedCBVNum >= m_MaxCBVNum)
		return pCB;

	pCB = &m_pCBContainerList[m_AllocatedCBVNum];
	m_AllocatedCBVNum++;

	return pCB;
}

void SimpleConstantBufferPool::Reset()
{
	m_AllocatedCBVNum = 0;
}

void SimpleConstantBufferPool::CleanUp()
{
	if (m_pResource)
	{
		m_pResource.Reset();
	}
	if (m_pCBVHeap)
	{
		m_pCBVHeap.Reset();
	}
}
