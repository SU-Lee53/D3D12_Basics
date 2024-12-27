#pragma once

/*
	- SORT_LIST 와 Hash Table 은 일단 그냥 생포인터로 진행
		- 이유 : 많은 포인터 연산을 활용하므로 스마트 포인터로 교체가 어려울듯
		- 하라면 vector 등을 이용해서 가능은 하겠지만 지금은 시간이 별로 없음
		- 내부적으로 레퍼런스 카운터를 잘 관리 하므로 마지막에 제거만 잘해주면 문제 없을것 같음

	- 나중에 개인적으로 엔진을 만들때는 그냥 std::map 을 이용해보려고 함
		- 여기 방법이 빠르고 좋긴 하지만 메모리 관리가 너무 빡셀거같음

*/


struct BUCKET_DESC
{
	SORT_LINK*					pBucketLinkHead;
	SORT_LINK*					pBucketLinkTail;
	DWORD						dwLinkNum;
};

struct VB_BUCKET
{
	std::shared_ptr<const void>			pItem;
	BUCKET_DESC*		pBucketDesc;
	SORT_LINK			sortLink;
	DWORD				dwSize;
	char				pKeyData[1];
};


class HashTable
{

	BUCKET_DESC*	m_pBucketDescTable = nullptr;
	DWORD	m_dwMaxBucketNum = 0;
	DWORD	m_dwMaxKeyDataSize = 0;
	DWORD	m_dwItemNum = 0;

	DWORD		CreateKey(const void* pData, DWORD dwSize, DWORD dwBucketNum);

public:
	BOOL	Initialize(DWORD dwMaxBucketNum, DWORD dwMaxKeySize, DWORD dwMaxItemNum);
	DWORD	Select(std::shared_ptr<void>* ppOutItemList, DWORD dwMaxItemNum, const void* pKeyData, DWORD dwSize);
	void*	Insert(std::shared_ptr<const void> pItem, const void* pKeyData, DWORD dwSize);
	void	Delete(const void* pSearchHandle);
	DWORD	GetMaxBucketNum() const;
	void	DeleteAll();
	DWORD	GetAllItems(std::shared_ptr<void>* ppOutItemList, DWORD dwMaxItemNum, BOOL* pbOutInsufficient) const;
	DWORD	GetKeyPtrAndSize(void** ppOutKeyPtr, const void* pSearchHandle) const;
	DWORD	GetItemNum() const;
	void	ResourceCheck() const;

	void	Cleanup();

	HashTable();
	~HashTable();


};


