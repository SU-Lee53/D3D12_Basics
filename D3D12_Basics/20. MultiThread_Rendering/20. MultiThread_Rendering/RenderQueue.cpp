#include "pch.h"
#include "RenderQueue.h"
#include "D3D12Renderer.h"
#include "BasicMeshObject.h"
#include "SpriteObject.h"
#include "CommandListPool.h"

using namespace std;

RenderQueue::RenderQueue()
{
}

RenderQueue::~RenderQueue()
{
	CleanUp();
}

BOOL RenderQueue::Initialize(std::shared_ptr<D3D12Renderer> pRenderer, DWORD dwMaxItemNum)
{
	m_pRenderer = pRenderer;
	m_dwMaxBufferNum = dwMaxItemNum;
	m_pBuffer.resize(dwMaxItemNum, nullptr);

	return TRUE;
}

BOOL RenderQueue::Add(const std::shared_ptr<RENDER_ITEM> pItem)
{
	BOOL bResult = FALSE;

	if (m_dwAllocatedNum + 1 > m_dwMaxBufferNum)
		return bResult;

	m_pBuffer[m_dwAllocatedNum] = pItem;
	m_dwAllocatedNum++;
	bResult = TRUE;

	return bResult;
}

#pragma region OLD_PROCESS_NO_MULTITHREAD
//DWORD RenderQueue::Process(ComPtr<ID3D12GraphicsCommandList>& prefCommandList)
//{
//	ComPtr<ID3D12Device5> pD3DDevice = m_pRenderer.lock()->GetDevice();
//
//	DWORD dwProcessedCount = 0;
//	shared_ptr<RENDER_ITEM> pItem = nullptr;
//
//	while (pItem = Dispatch())
//	{
//		switch (pItem->Type)
//		{
//		case RENDER_ITEM_TYPE_MESH_OBJ:
//		{
//			shared_ptr<BasicMeshObject> pMeshObj = static_pointer_cast<BasicMeshObject>(pItem->pObjHandle);
//			RENDER_MESH_OBJ_PARAM spriteParam = std::get<RENDER_MESH_OBJ_PARAM>(pItem->Param);
//			pMeshObj->Draw(prefCommandList, spriteParam.matWorld);
//		}
//		break;
//
//		case RENDER_ITEM_TYPE_SPRITE:
//		{
//			shared_ptr<SpriteObject> pSpriteObj = static_pointer_cast<SpriteObject>(pItem->pObjHandle);
//			RENDER_SPRITE_PARAM spriteParam = std::get<RENDER_SPRITE_PARAM>(pItem->Param);
//			shared_ptr<TEXTURE_HANDLE> pTextureHandle = static_pointer_cast<TEXTURE_HANDLE>(spriteParam.pTexHandle);
//			float Z = spriteParam.Z;
//
//			if (pTextureHandle)
//			{
//				XMFLOAT2 pos = { (float)spriteParam.iPosX, (float)spriteParam.iPosY };
//				XMFLOAT2 scale = { spriteParam.fScaleX, spriteParam.fScaleY };
//
//				const RECT* pRect = nullptr;
//				if (spriteParam.bUseRect)
//				{
//					pRect = &spriteParam.Rect;
//				}
//
//				if (pTextureHandle->pUploadBuffer)
//				{
//					if (pTextureHandle->bUpdated)
//					{
//						D3DUtils::UpdateTexture(pD3DDevice, prefCommandList, pTextureHandle->pTexResource, pTextureHandle->pUploadBuffer);
//					}
//					pTextureHandle->bUpdated = FALSE;
//				}
//				pSpriteObj->DrawWithTex(prefCommandList, pos, scale, pRect, Z, pTextureHandle.get());
//
//			}
//			else
//			{
//				shared_ptr<SpriteObject> pSpriteObj = static_pointer_cast<SpriteObject>(pItem->pObjHandle);
//				XMFLOAT2 pos = { (float)spriteParam.iPosX, (float)spriteParam.iPosY };
//				XMFLOAT2 scale = { spriteParam.fScaleX, spriteParam.fScaleY };
//
//				pSpriteObj->Draw(prefCommandList, pos, scale, Z);
//			}
//		}
//		break;
//
//
//		default:
//			__debugbreak();
//		}
//
//		dwProcessedCount++;
//	}
//
//	m_dwItemCount = 0;
//	return dwProcessedCount;
//}
//
//DWORD RenderQueue::Process(shared_ptr<CommandListPool> pCommandListPool, ComPtr<ID3D12CommandQueue> pCommandQueue, DWORD dwProcessCountPerCommandList, D3D12_CPU_DESCRIPTOR_HANDLE rtv, D3D12_CPU_DESCRIPTOR_HANDLE dsv, const D3D12_VIEWPORT& refViewport, const D3D12_RECT& refScissorRect)
//{
//	ComPtr<ID3D12Device5> pD3DDevice = m_pRenderer.lock()->GetDevice();
//
//	// Command List 들을 모아놓음 -> 나중에 한번에 처리
//	ComPtr<ID3D12GraphicsCommandList> pCommandLists[64] = {};
//	DWORD dwCommandListCount = 0;
//
//	ComPtr<ID3D12GraphicsCommandList> pCommandList = nullptr;
//	DWORD dwProcessedCount = 0;
//	DWORD dwProcessedCountPerCommandList = 0;
//	shared_ptr<RENDER_ITEM> pItem = nullptr;
//
//	while (pItem = Dispatch())
//	{
//		pCommandList = pCommandListPool->GetCurrentCommandList();
//		pCommandList->RSSetViewports(1, &refViewport);
//		pCommandList->RSSetScissorRects(1, &refScissorRect);
//		pCommandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
//
//		switch (pItem->Type)
//		{
//		case RENDER_ITEM_TYPE_MESH_OBJ:
//		{
//			shared_ptr<BasicMeshObject> pMeshObj = static_pointer_cast<BasicMeshObject>(pItem->pObjHandle);
//			RENDER_MESH_OBJ_PARAM meshParam = std::get<RENDER_MESH_OBJ_PARAM>(pItem->Param);
//			pMeshObj->Draw(pCommandList, meshParam.matWorld);
//		}
//		break;
//
//		case RENDER_ITEM_TYPE_SPRITE:
//		{
//			shared_ptr<SpriteObject> pSpriteObj = static_pointer_cast<SpriteObject>(pItem->pObjHandle);
//			RENDER_SPRITE_PARAM spriteParam = std::get<RENDER_SPRITE_PARAM>(pItem->Param);
//			shared_ptr<TEXTURE_HANDLE> pTextureHandle = static_pointer_cast<TEXTURE_HANDLE>(spriteParam.pTexHandle);
//			float Z = spriteParam.Z;
//
//			if (pTextureHandle)
//			{
//				XMFLOAT2 pos = { (float)spriteParam.iPosX, (float)spriteParam.iPosY };
//				XMFLOAT2 scale = { spriteParam.fScaleX, spriteParam.fScaleY };
//
//				const RECT* pRect = nullptr;
//				if (spriteParam.bUseRect)
//				{
//					pRect = &spriteParam.Rect;
//				}
//
//				if (pTextureHandle->pUploadBuffer)
//				{
//					if (pTextureHandle->bUpdated)
//					{
//						D3DUtils::UpdateTexture(pD3DDevice, pCommandList, pTextureHandle->pTexResource, pTextureHandle->pUploadBuffer);
//					}
//					pTextureHandle->bUpdated = FALSE;
//				}
//				pSpriteObj->DrawWithTex(pCommandList, pos, scale, pRect, Z, pTextureHandle.get());
//			}
//			else
//			{
//				shared_ptr<SpriteObject> pSpriteObj = static_pointer_cast<SpriteObject>(pItem->pObjHandle);
//				XMFLOAT2 pos = { (float)spriteParam.iPosX, (float)spriteParam.iPosY };
//				XMFLOAT2 scale = { spriteParam.fScaleX, spriteParam.fScaleY };
//
//				pSpriteObj->Draw(pCommandList, pos, scale, Z);
//			}
//		}
//		break;
//
//		default:
//			__debugbreak();
//		}
//
//		dwProcessedCount++;
//		dwProcessedCountPerCommandList++;
//		
//		// 원하는 만큼 커맨드 작성이 끝나면 기존 커맨드 리스트를 배열에 담음
//		// 여기서 pCommandList 를 nullptr 로 만들면 루프 시작때 새로운 CommandList 를 가져옴
//		if (dwProcessedCountPerCommandList > dwProcessCountPerCommandList)
//		{
//			pCommandListPool->Close();
//			pCommandLists[dwCommandListCount] = pCommandList;
//			dwCommandListCount++;
//			pCommandList = nullptr;
//			dwProcessedCountPerCommandList = 0;
//		}
//
//	}
//
//	// 마지막으로 커맨드가 작성되던 커맨드 리스트를 일단 닫고 배열에 담음
//	if (dwProcessedCountPerCommandList)
//	{
//		pCommandListPool->Close();
//		pCommandLists[dwCommandListCount] = pCommandList;
//		dwCommandListCount++;
//		pCommandList = nullptr;
//	}
//
//	if (dwCommandListCount)
//	{
//		// ComPtr 이라 어쩔수 없는듯? 다른방법이 있을까
//		ID3D12CommandList* rawpCommandLists[64];
//		for (DWORD i = 0; i < dwCommandListCount; i++)
//		{
//			rawpCommandLists[i] = std::move(pCommandLists[i].Get());
//		}
//		pCommandQueue->ExecuteCommandLists(dwCommandListCount, (ID3D12CommandList**)rawpCommandLists);
//	}
//
//	m_dwItemCount = 0;
//	return dwProcessedCount;
//
//}
#pragma endregion OLD_PROCESS_NO_MULTITHREAD

DWORD RenderQueue::Process(DWORD dwThreadIndex, std::shared_ptr<CommandListPool> pCommandListPool, ComPtr<ID3D12CommandQueue> pCommandQueue, DWORD dwProcessCountPerCommandList, D3D12_CPU_DESCRIPTOR_HANDLE rtv, D3D12_CPU_DESCRIPTOR_HANDLE dsv, const D3D12_VIEWPORT& refViewport, const D3D12_RECT& refScissorRect)
{
	ComPtr<ID3D12Device5> pD3DDevice = m_pRenderer.lock()->GetDevice();

	// Command List 들을 모아놓음 -> 나중에 한번에 처리
	ComPtr<ID3D12GraphicsCommandList> pCommandLists[64] = {};
	DWORD dwCommandListCount = 0;

	ComPtr<ID3D12GraphicsCommandList> pCommandList = nullptr;
	DWORD dwProcessedCount = 0;
	DWORD dwProcessedCountPerCommandList = 0;
	shared_ptr<RENDER_ITEM> pItem = nullptr;

	while (pItem = Dispatch())
	{
		pCommandList = pCommandListPool->GetCurrentCommandList();
		pCommandList->RSSetViewports(1, &refViewport);
		pCommandList->RSSetScissorRects(1, &refScissorRect);
		pCommandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

		switch (pItem->Type)
		{
		case RENDER_ITEM_TYPE_MESH_OBJ:
		{
			shared_ptr<BasicMeshObject> pMeshObj = static_pointer_cast<BasicMeshObject>(pItem->pObjHandle);
			RENDER_MESH_OBJ_PARAM meshParam = std::get<RENDER_MESH_OBJ_PARAM>(pItem->Param);
			pMeshObj->Draw(dwThreadIndex, pCommandList, meshParam.matWorld);
		}
		break;

		case RENDER_ITEM_TYPE_SPRITE:
		{
			shared_ptr<SpriteObject> pSpriteObj = static_pointer_cast<SpriteObject>(pItem->pObjHandle);
			RENDER_SPRITE_PARAM spriteParam = std::get<RENDER_SPRITE_PARAM>(pItem->Param);
			shared_ptr<TEXTURE_HANDLE> pTextureHandle = static_pointer_cast<TEXTURE_HANDLE>(spriteParam.pTexHandle);
			float Z = spriteParam.Z;

			if (pTextureHandle)
			{
				XMFLOAT2 pos = { (float)spriteParam.iPosX, (float)spriteParam.iPosY };
				XMFLOAT2 scale = { spriteParam.fScaleX, spriteParam.fScaleY };

				const RECT* pRect = nullptr;
				if (spriteParam.bUseRect)
				{
					pRect = &spriteParam.Rect;
				}

				if (pTextureHandle->pUploadBuffer)
				{
					if (pTextureHandle->bUpdated)
					{
						D3DUtils::UpdateTexture(pD3DDevice, pCommandList, pTextureHandle->pTexResource, pTextureHandle->pUploadBuffer);
					}
					pTextureHandle->bUpdated = FALSE;
				}
				pSpriteObj->DrawWithTex(dwThreadIndex, pCommandList, pos, scale, pRect, Z, pTextureHandle.get());
			}
			else
			{
				shared_ptr<SpriteObject> pSpriteObj = static_pointer_cast<SpriteObject>(pItem->pObjHandle);
				XMFLOAT2 pos = { (float)spriteParam.iPosX, (float)spriteParam.iPosY };
				XMFLOAT2 scale = { spriteParam.fScaleX, spriteParam.fScaleY };

				pSpriteObj->Draw(dwThreadIndex, pCommandList, pos, scale, Z);
			}
		}
		break;

		default:
			__debugbreak();
		}

		dwProcessedCount++;
		dwProcessedCountPerCommandList++;

		// 원하는 만큼 커맨드 작성이 끝나면 기존 커맨드 리스트를 배열에 담음
		// 여기서 pCommandList 를 nullptr 로 만들면 루프 시작때 새로운 CommandList 를 가져옴
		if (dwProcessedCountPerCommandList > dwProcessCountPerCommandList)
		{
			pCommandListPool->Close();
			pCommandLists[dwCommandListCount] = pCommandList;
			dwCommandListCount++;
			pCommandList = nullptr;
			dwProcessedCountPerCommandList = 0;
		}

	}

	// 마지막으로 커맨드가 작성되던 커맨드 리스트를 일단 닫고 배열에 담음
	if (dwProcessedCountPerCommandList)
	{
		pCommandListPool->Close();
		pCommandLists[dwCommandListCount] = pCommandList;
		dwCommandListCount++;
		pCommandList = nullptr;
	}

	if (dwCommandListCount)
	{
		// ComPtr 이라 어쩔수 없는듯? 다른방법이 있을까
		ID3D12CommandList* rawpCommandLists[64];
		for (DWORD i = 0; i < dwCommandListCount; i++)
		{
			rawpCommandLists[i] = std::move(pCommandLists[i].Get());
		}
		pCommandQueue->ExecuteCommandLists(dwCommandListCount, (ID3D12CommandList**)rawpCommandLists);
	}

	m_dwItemCount = 0;
	return dwProcessedCount;

}

void RenderQueue::Reset()
{
	m_dwAllocatedNum = 0;
	m_dwReadBufferIndex = 0;
}

const std::shared_ptr<RENDER_ITEM> RenderQueue::Dispatch()
{
	shared_ptr<RENDER_ITEM> pItem = nullptr;
	if (m_dwReadBufferIndex + 1 > m_dwAllocatedNum)
		return pItem;

	pItem = m_pBuffer[m_dwReadBufferIndex];
	m_dwReadBufferIndex++;

	return pItem;
}

void RenderQueue::CleanUp()
{
	if (!m_pBuffer.empty())
	{
		m_pBuffer.clear();
	}
}
