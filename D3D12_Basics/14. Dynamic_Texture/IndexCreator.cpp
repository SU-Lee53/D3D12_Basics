#include "pch.h"
#include "IndexCreator.h"

IndexCreator::IndexCreator()
{
}

IndexCreator::~IndexCreator()
{
}

BOOL IndexCreator::Initialize(DWORD dwNum)
{
	m_dwIndexVector.resize(dwNum, 0);
	m_dwMaxNum = dwNum;

	for (DWORD i = 0; i < m_dwMaxNum; i++)
	{
		m_dwIndexVector[i] = i;
	}

	return TRUE;
}

DWORD IndexCreator::Alloc()
{
	// 1. m_lAllocatedCount���� 1�� ����.
	// 2. m_lAllocatedCount-1��ġ�� dwIndex�� ��ִ´�.
	// �� �ΰ��� �׼��� �ʿ��ѵ� 1�� 2���̿� �ٸ� �����尡 Alloc�� ȣ���ϸ� �̹� �Ҵ�� �ε����� ���� ���� �߻��Ѵ�.
	// ���� Alloc�� Free���� �� ���ɶ����� ���ƾ��Ѵ�.

	DWORD dwResult = -1;

	if (m_dwAllocatedCount >= m_dwMaxNum)
		return dwResult;

	dwResult = m_dwIndexVector[m_dwAllocatedCount];
	m_dwAllocatedCount++;

	return dwResult;
}

void IndexCreator::Free(DWORD dwIndex)
{
	if (!m_dwAllocatedCount)
	{
		// 0�̸� Free ����
		__debugbreak();
		return;
	}

	m_dwAllocatedCount--;
	m_dwIndexVector[m_dwAllocatedCount] = dwIndex;
}

void IndexCreator::Check()
{
	if (m_dwAllocatedCount)
		__debugbreak();
}
