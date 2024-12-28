#include "pch.h"
#include "TextureManager.h"
#include "D3D12Renderer.h"
#include "D3D12ResourceManager.h"
#include "SingleDescriptorAllocator.h"

using namespace std;

TextureManager::TextureManager()
{
}

TextureManager::~TextureManager()
{
	CleanUp();
}

BOOL TextureManager::Initialize(std::shared_ptr<D3D12Renderer>& pRenderer, DWORD dwMaxBucketNum, DWORD dwMaxFileNum)
{
	m_pRenderer = pRenderer;
	m_pResourceManager = pRenderer->GetResourceManager();

	//m_pHashTable = make_unique<HashTable>();
	//m_pHashTable->Initialize(dwMaxBucketNum, _MAX_PATH * sizeof(WCHAR), dwMaxFileNum);

	return TRUE;
}

std::shared_ptr<TEXTURE_HANDLE> TextureManager::CreateTextureFromFile(const std::wstring& wchFilename)
{
	ComPtr<ID3D12Device5> pD3DDevice = m_pRenderer.lock()->GetDevice();
	shared_ptr<SingleDescriptorAllocator> pSingleDescriptorAllocator = m_pRenderer.lock()->GetSingleDescriptorAllocator();

	ComPtr<ID3D12Resource> pTexResource = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE srv = {};
	D3D12_RESOURCE_DESC desc = {};
	shared_ptr<TEXTURE_HANDLE> pTexHandle = nullptr;

	DWORD dwFileNameLen = wchFilename.size();
	DWORD dwKeySize = dwFileNameLen * sizeof(WCHAR);

	// 문제 발생 가능성 존재 한번 확인할것
	if (m_TextureMap.find(wchFilename) != m_TextureMap.end())
	{
		pTexHandle = m_TextureMap[wchFilename];
		pTexHandle->dwRefCount++;
	}
	else
	{
		if (m_pResourceManager->CreateTextureFromFile(pTexResource, desc, wchFilename.c_str()))
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
			SRVDesc.Format = desc.Format;
			SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			SRVDesc.Texture2D.MipLevels = desc.MipLevels;

			if (pSingleDescriptorAllocator->AllocDescriptorHandle(srv))
			{
				pD3DDevice->CreateShaderResourceView(pTexResource.Get(), &SRVDesc, srv);

				pTexHandle = AllocTextureHandle();
				pTexHandle->pTexResource = pTexResource;
				pTexHandle->bFromFile = TRUE;
				pTexHandle->srv = srv;

				//pTexHandle->pSearchHandle = m_pHashTable->Insert(pTexHandle, wchFilename.c_str(), dwKeySize);
				//if (!pTexHandle->pSearchHandle)
				//	__debugbreak();

				m_TextureMap.insert({ wchFilename, pTexHandle });
				pTexHandle->pSearchKey = wchFilename;
			}
			else
			{
				// Release pTexResource
			}
		}
	}

	return pTexHandle;
}

std::shared_ptr<TEXTURE_HANDLE> TextureManager::CreateDynamicTexture(UINT TexWidth, UINT TexHeight)
{
	ComPtr<ID3D12Device5> pD3DDevice = m_pRenderer.lock()->GetDevice();
	shared_ptr<SingleDescriptorAllocator> pSingleDescriptorAllocator = m_pRenderer.lock()->GetSingleDescriptorAllocator();
	shared_ptr<TEXTURE_HANDLE> pTexHandle = nullptr;

	ComPtr<ID3D12Resource> pTexResource = nullptr;
	ComPtr<ID3D12Resource> pUploadBuffer = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE srv = {};

	DXGI_FORMAT TexFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

	if (m_pResourceManager->CreateTexturePair(pTexResource, pUploadBuffer, TexWidth, TexHeight, TexFormat))
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
		SRVDesc.Format = TexFormat;
		SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		SRVDesc.Texture2D.MipLevels = 1;

		if (pSingleDescriptorAllocator->AllocDescriptorHandle(srv))
		{
			pD3DDevice->CreateShaderResourceView(pTexResource.Get(), &SRVDesc, srv);

			pTexHandle = AllocTextureHandle();
			pTexHandle->pTexResource = pTexResource;
			pTexHandle->pUploadBuffer = pUploadBuffer;
			pTexHandle->srv = srv;

			m_DynamicTextureCount++;
			pTexHandle->pSearchKey = L"Dynamic" + to_wstring(m_DynamicTextureCount);
		}
		else
		{
			// Release pTexResource, pUploadBuffer
		}
	}

	return pTexHandle;
}

std::shared_ptr<TEXTURE_HANDLE> TextureManager::CreateImmutableTexture(UINT TexWidth, UINT TexHeight, DXGI_FORMAT format, const BYTE* pInitImage)
{
	ComPtr<ID3D12Device5> pD3DDevice = m_pRenderer.lock()->GetDevice();
	shared_ptr<SingleDescriptorAllocator> pSingleDescriptorAllocator = m_pRenderer.lock()->GetSingleDescriptorAllocator();
	shared_ptr<TEXTURE_HANDLE> pTexHandle = nullptr;

	ComPtr<ID3D12Resource> pTexResource = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE srv = {};

	if (m_pResourceManager->CreateTexture(pTexResource, TexWidth, TexHeight, format, pInitImage))
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
		SRVDesc.Format = format;
		SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		SRVDesc.Texture2D.MipLevels = 1;

		if (pSingleDescriptorAllocator->AllocDescriptorHandle(srv))
		{
			pD3DDevice->CreateShaderResourceView(pTexResource.Get(), &SRVDesc, srv);

			pTexHandle = AllocTextureHandle();
			pTexHandle->pTexResource = pTexResource;
			pTexHandle->srv = srv;
			pTexHandle->pSearchKey = L"Immutable" + to_wstring(m_ImmutableTextureCount);
		}
		else
		{
			// Release pTexResource
		}
	}

	return pTexHandle;
}

void TextureManager::DeleteTexture(std::shared_ptr<TEXTURE_HANDLE> pTexHandle)
{
	FreeTextureHandle(pTexHandle);
}

std::shared_ptr<TEXTURE_HANDLE> TextureManager::AllocTextureHandle()
{
	shared_ptr<TEXTURE_HANDLE> pTexHandle = make_shared<TEXTURE_HANDLE>();
	pTexHandle->Link.pItem = pTexHandle;
	m_TextureList.push_front(pTexHandle);
	pTexHandle->dwRefCount = 1;
	return pTexHandle;
}

DWORD TextureManager::FreeTextureHandle(std::shared_ptr<TEXTURE_HANDLE> pTexHandle)
{
	ComPtr<ID3D12Device5> pD3DDevice = m_pRenderer.lock()->GetDevice();
	shared_ptr<SingleDescriptorAllocator> pSingleDescriptorAllocator = m_pRenderer.lock()->GetSingleDescriptorAllocator();

	// dwRefCount == 0 은 불가능
	if (!pTexHandle->dwRefCount)
		__debugbreak();

	DWORD ref_count = --pTexHandle->dwRefCount;
	if (!ref_count)
	{
		if (pTexHandle->pTexResource)
		{
			pTexHandle->pTexResource->Release();
			pTexHandle->pTexResource = nullptr;
		}
		if (pTexHandle->pUploadBuffer)
		{
			pTexHandle->pUploadBuffer->Release();
			pTexHandle->pUploadBuffer = nullptr;
		}
		if (pTexHandle->srv.ptr)
		{
			pSingleDescriptorAllocator->FreeDescriptorHandle(pTexHandle->srv);
			pTexHandle->srv = {};
		}

		if (!pTexHandle->pSearchKey.empty())
		{
			//m_pHashTable->Delete(pTexHandle->pSearchHandle);
			//pTexHandle->pSearchHandle = nullptr;
			m_TextureMap.erase(pTexHandle->pSearchKey);
			pTexHandle->pSearchKey = L"";
		}

		//UnLinkFromLinkedList(&m_pTexLinkHead, &m_pTexLinkTail, &pTexHandle->Link);

		m_TextureList.erase(std::find(m_TextureList.begin(), m_TextureList.end(), pTexHandle));

		// shared_ptr 의 reset 은 ref count 만 낮춤
		// 다만 ref count 가 0이되면 소멸자까지 호출해줌
		// 여기서 소멸자가 호출이 되는지는 확인이 필요할듯
		pTexHandle.reset();
	}
	else
	{
		// ref_count 가 0이 아니면 삭제 XXX
	}

	return ref_count;
}

void TextureManager::CleanUp()
{
	//if (m_pTexLinkHead)
	//{
	//	// texture resource leak
	//	__debugbreak();
	//}
	//
	//if (m_pHashTable)
	//{
	//	// unique_ptr 이므로 제거되야 맞음
	//	m_pHashTable.reset();
	//	m_pHashTable = nullptr;
	//}
}
