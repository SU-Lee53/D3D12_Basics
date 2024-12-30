#pragma once


class IndexCreator
{
public:
	IndexCreator();
	~IndexCreator();

	BOOL Initialize(DWORD dwNum);

	DWORD Alloc();
	void Free(DWORD dwIndex);
	void Check();


private:
	std::vector<DWORD>	m_dwIndexVector = {};
	DWORD		m_dwMaxNum = 0;
	DWORD		m_dwAllocatedCount = 0;

};

