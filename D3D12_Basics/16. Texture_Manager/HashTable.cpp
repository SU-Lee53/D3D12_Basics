#include "pch.h"
#include <Windows.h>
#include "LinkedList.h"
#include "HashTable.h"

using namespace std;

HashTable::HashTable()
{
}

BOOL HashTable::Initialize(DWORD dwMaxBucketNum, DWORD dwMaxKeySize, DWORD dwMaxItemNum)
{
	m_dwMaxKeyDataSize = dwMaxKeySize;
	m_dwMaxBucketNum = dwMaxBucketNum;

	m_pBucketDescTable = new BUCKET_DESC[dwMaxBucketNum];
	memset(m_pBucketDescTable, 0, sizeof(BUCKET_DESC) * dwMaxBucketNum);

	return TRUE;
}


DWORD HashTable::CreateKey(const void* pData, DWORD dwSize, DWORD dwBucketNum)
{
	DWORD	dwKeyData = 0;

	const char*	pEntry = (char*)pData;
	if (dwSize & 0x00000001)
	{
		dwKeyData += (DWORD)(*(BYTE*)pEntry);
		pEntry++;
		dwSize--;
	}
	if (!dwSize)
		goto lb_exit;


	if (dwSize & 0x00000002)
	{
		dwKeyData += (DWORD)(*(WORD*)pEntry);
		pEntry += 2;
		dwSize -= 2;
	}
	if (!dwSize)
		goto lb_exit;

	dwSize = (dwSize >> 2);

	for (DWORD i = 0; i < dwSize; i++)
	{
		dwKeyData += *(DWORD*)pEntry;
		pEntry += 4;
	}

lb_exit:
	DWORD	dwIndex = dwKeyData % dwBucketNum;

	return dwIndex;
}
DWORD HashTable::Select(shared_ptr<void>* ppOutItemList, DWORD dwMaxItemNum, const void* pKeyData, DWORD dwSize)
{

	DWORD			dwSelectedItemNum = 0;
	DWORD			dwIndex = CreateKey(pKeyData, dwSize, m_dwMaxBucketNum);
	BUCKET_DESC*	pBucketDesc = m_pBucketDescTable + dwIndex;

	SORT_LINK*		pCur = pBucketDesc->pBucketLinkHead;

	VB_BUCKET*		pBucket;

	while (pCur)
	{
		if (!dwMaxItemNum)
			goto lb_return;

		pBucket = (VB_BUCKET*)pCur->pItem.get();

		if (pBucket->dwSize != dwSize)
			goto lb_next;

		if (memcmp(pBucket->pKeyData, pKeyData, dwSize))
			goto lb_next;


		dwMaxItemNum--;
		ppOutItemList[dwSelectedItemNum] = const_pointer_cast<void>(pBucket->pItem);
		dwSelectedItemNum++;
	lb_next:
		pCur = pCur->pNext;
	}

lb_return:
	return dwSelectedItemNum;
}
void* HashTable::Insert(shared_ptr<const void> pItem, const void* pKeyData, DWORD dwSize)
{
	void*	pSearchHandle = nullptr;

	if (dwSize > m_dwMaxKeyDataSize)
	{
		__debugbreak();
		return pSearchHandle;
	}

	DWORD dwBucketMemSize = (DWORD)(sizeof(VB_BUCKET) - sizeof(char)) + m_dwMaxKeyDataSize;
	shared_ptr<VB_BUCKET> pBucket = make_shared<VB_BUCKET>();

	DWORD			dwIndex = CreateKey(pKeyData, dwSize, m_dwMaxBucketNum);
	BUCKET_DESC*	pBucketDesc = m_pBucketDescTable + dwIndex;

	pBucket->pItem = pItem;
	pBucket->dwSize = dwSize;
	pBucket->pBucketDesc = pBucketDesc;
	pBucket->sortLink.pPrv = nullptr;
	pBucket->sortLink.pNext = nullptr;
	pBucket->sortLink.pItem = pBucket;
	pBucketDesc->dwLinkNum++;

	memcpy(pBucket->pKeyData, pKeyData, dwSize);


	LinkToLinkedListFIFO(&pBucketDesc->pBucketLinkHead, &pBucketDesc->pBucketLinkTail, &pBucket->sortLink);

	m_dwItemNum++;
	pSearchHandle = pBucket.get();

	return pSearchHandle;



}
void HashTable::Delete(const void* pSearchHandle)
{

	VB_BUCKET*		pBucket = (VB_BUCKET*)pSearchHandle;
	BUCKET_DESC*	pBucketDesc = pBucket->pBucketDesc;

	UnLinkFromLinkedList(&pBucketDesc->pBucketLinkHead, &pBucketDesc->pBucketLinkTail, &pBucket->sortLink);

	pBucketDesc->dwLinkNum--;

	free(pBucket);
	m_dwItemNum--;

}


DWORD	HashTable::GetMaxBucketNum() const
{
	return m_dwMaxBucketNum;
}


void HashTable::DeleteAll()
{

	VB_BUCKET*		pBucket;

	for (DWORD i = 0; i < m_dwMaxBucketNum; i++)
	{
		while (m_pBucketDescTable[i].pBucketLinkHead)
		{
			pBucket = (VB_BUCKET*)m_pBucketDescTable[i].pBucketLinkHead->pItem.get();
			Delete(pBucket);
		}
	}
}

DWORD HashTable::GetAllItems(shared_ptr<void>* ppOutItemList, DWORD dwMaxItemNum, BOOL* pbOutInsufficient) const
{
	VB_BUCKET*		pBucket;
	SORT_LINK*		pCur;

	*pbOutInsufficient = FALSE;
	DWORD			dwItemNum = 0;

	for (DWORD i = 0; i < m_dwMaxBucketNum; i++)
	{
		pCur = m_pBucketDescTable[i].pBucketLinkHead;
		while (pCur)
		{
			pBucket = (VB_BUCKET*)pCur->pItem.get();

			if (dwItemNum >= dwMaxItemNum)
			{
				*pbOutInsufficient = TRUE;
				goto lb_return;
			}


			ppOutItemList[dwItemNum] = const_pointer_cast<void>(pBucket->pItem);
			dwItemNum++;

			pCur = pCur->pNext;
		}
	}
lb_return:
	return dwItemNum;
}

DWORD HashTable::GetKeyPtrAndSize(void** ppOutKeyPtr, const void* pSearchHandle) const
{

	*ppOutKeyPtr = ((VB_BUCKET*)pSearchHandle)->pKeyData;
	DWORD	dwSize = ((VB_BUCKET*)pSearchHandle)->dwSize;

	return dwSize;
}



DWORD HashTable::GetItemNum() const
{
	return m_dwItemNum;
}

void HashTable::ResourceCheck() const
{
	if (m_dwItemNum)
		__debugbreak();
}
void HashTable::Cleanup()
{
	ResourceCheck();

	DeleteAll();
	if (m_pBucketDescTable)
	{
		delete[] m_pBucketDescTable;
		m_pBucketDescTable = nullptr;
	}
}
HashTable::~HashTable()
{
	Cleanup();
}