#include "pch.h"
#include "RenderQueue.h"
#include "D3D12Renderer.h"
#include "BasicMeshObject.h"
#include "SpriteObject.h"

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

DWORD RenderQueue::Process(ComPtr<ID3D12GraphicsCommandList>& prefCommandList)
{
	ComPtr<ID3D12Device5> pD3DDevice = m_pRenderer.lock()->GetDevice();

	DWORD dwItemCount = 0;
	shared_ptr<RENDER_ITEM> pItem = nullptr;

	while (pItem = Dispatch())
	{
		switch (pItem->Type)
		{
		case RENDER_ITEM_TYPE_MESH_OBJ:
		{
			shared_ptr<BasicMeshObject> pMeshObj = static_pointer_cast<BasicMeshObject>(pItem->pObjHandle);
			RENDER_MESH_OBJ_PARAM spriteParam = std::get<RENDER_MESH_OBJ_PARAM>(pItem->Param);
			pMeshObj->Draw(prefCommandList, spriteParam.matWorld);
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
						D3DUtils::UpdateTexture(pD3DDevice, prefCommandList, pTextureHandle->pTexResource, pTextureHandle->pUploadBuffer);
					}
					pTextureHandle->bUpdated = FALSE;
				}
				pSpriteObj->DrawWithTex(prefCommandList, pos, scale, pRect, Z, pTextureHandle.get());

			}
			else
			{
				shared_ptr<SpriteObject> pSpriteObj = static_pointer_cast<SpriteObject>(pItem->pObjHandle);
				XMFLOAT2 pos = { (float)spriteParam.iPosX, (float)spriteParam.iPosY };
				XMFLOAT2 scale = { spriteParam.fScaleX, spriteParam.fScaleY };

				pSpriteObj->Draw(prefCommandList, pos, scale, Z);
			}
		}
		break;


		default:
			__debugbreak();
		}

		dwItemCount++;
	}

	return dwItemCount;
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
