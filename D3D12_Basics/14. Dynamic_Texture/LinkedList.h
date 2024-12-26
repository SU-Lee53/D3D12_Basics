#pragma once

struct SORT_LINK
{
	SORT_LINK*		pPrv;
	SORT_LINK*		pNext;
	std::shared_ptr<void>			pItem;
};

void LinkToLinkedList(SORT_LINK** ppHead, SORT_LINK** ppTail, SORT_LINK* pNew);
void LinkToLinkedListFIFO(SORT_LINK** ppHead, SORT_LINK** ppTail, SORT_LINK* pNew);
void UnLinkFromLinkedList(SORT_LINK** ppHead, SORT_LINK** ppTail, SORT_LINK* pDel);
