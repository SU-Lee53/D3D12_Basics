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
	// 1. m_lAllocatedCount에서 1을 뺀다.
	// 2. m_lAllocatedCount-1위치에 dwIndex를 써넣는다.
	// 이 두가지 액션이 필요한데 1과 2사이에 다른 스레드가 Alloc을 호출하면 이미 할당된 인덱스를 얻어가는 일이 발생한다.
	// 따라서 Alloc과 Free양쪽 다 스핀락으로 막아야한다.

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
		// 0이면 Free 못함
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
