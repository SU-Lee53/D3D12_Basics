#include "pch.h"
#include "ConstantBufferManager.h"
#include "SimpleConstantBufferPool.h"

CONSTANT_BUFFER_PROPERTY g_pConstantBufferPropList[] =
{
	{CONSTANT_BUFFER_TYPE_DEFAULT, sizeof(CONSTANT_BUFFER_DEFAULT)},
	{CONSTANT_BUFFER_TYPE_SPRITE, sizeof(CONSTANT_BUFFER_SPRITE)}
};

ConstantBufferManager::ConstantBufferManager()
{
}

ConstantBufferManager::~ConstantBufferManager()
{
}

BOOL ConstantBufferManager::Initialize(ComPtr<ID3D12Device5>& pD3DDevice, DWORD dwMaxCBVNum)
{
	for (DWORD i = 0; i < CONSTANT_BUFFER_TYPE_COUNT; i++)
	{
		m_pConstantBufferPoolList[i] = std::make_shared<SimpleConstantBufferPool>();
		m_pConstantBufferPoolList[i]->Initialize(pD3DDevice, (CONSTANT_BUFFER_TYPE)i, D3DUtils::AlignConstantBuffersize(g_pConstantBufferPropList[i].Size), dwMaxCBVNum);
	}

	return TRUE;
}

void ConstantBufferManager::Reset()
{
	for (DWORD i = 0; i < CONSTANT_BUFFER_TYPE_COUNT; i++)
	{
		if (m_pConstantBufferPoolList[i])
		{
			m_pConstantBufferPoolList[i]->Reset();
		}
	}
}

std::shared_ptr<SimpleConstantBufferPool> ConstantBufferManager::GetConstantBufferPool(CONSTANT_BUFFER_TYPE type)
{
	if (type >= CONSTANT_BUFFER_TYPE_COUNT)
		__debugbreak();

	return m_pConstantBufferPoolList[type];
}
