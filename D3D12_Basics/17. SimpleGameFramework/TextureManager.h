#pragma once

/*
	- 그냥 STL로 다시 만듬
		- 의도한 대로는 잘 도니까 좋았쓰
*/


class HashTable;
class D3D12Renderer;
class D3D12ResourceManager;

class TextureManager
{
public:
	TextureManager();
	~TextureManager();

	BOOL Initialize(std::shared_ptr<D3D12Renderer>& pRenderer, DWORD dwMaxBucketNum, DWORD dwMaxFileNum);
	std::shared_ptr<TEXTURE_HANDLE> CreateTextureFromFile(const std::wstring& wchFilename);
	std::shared_ptr<TEXTURE_HANDLE> CreateDynamicTexture(UINT TexWidth, UINT TexHeight);
	std::shared_ptr<TEXTURE_HANDLE> CreateImmutableTexture(UINT TexWidth, UINT TexHeight, DXGI_FORMAT format, const BYTE* pInitImage);

	void DeleteTexture(std::shared_ptr<TEXTURE_HANDLE> pTexHandle);

private:
	std::shared_ptr<TEXTURE_HANDLE> AllocTextureHandle();
	DWORD FreeTextureHandle(std::shared_ptr<TEXTURE_HANDLE> pTexHandle);
	void CleanUp();


private:
	std::shared_ptr<D3D12Renderer> m_pRenderer = nullptr;
	std::shared_ptr<D3D12ResourceManager> m_pResourceManager = nullptr;
	//std::unique_ptr<HashTable> m_pHashTable = nullptr;
	
	//SORT_LINK* m_pTexLinkHead = nullptr;
	//SORT_LINK* m_pTexLinkTail = nullptr;

	std::unordered_map<std::wstring, std::shared_ptr<TEXTURE_HANDLE>> m_TextureMap;
	std::list<std::shared_ptr<TEXTURE_HANDLE>> m_TextureList;

	UINT64 m_DynamicTextureCount = 0;
	UINT64 m_ImmutableTextureCount = 0;
public:

};

