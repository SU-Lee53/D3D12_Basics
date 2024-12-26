#pragma once



struct D3DUtils
{
	static void SetDebugLayerInfo(ComPtr<ID3D12Device> pD3DDevice);
	static void SetDefaultSamplerDesc(D3D12_STATIC_SAMPLER_DESC* pOutSamplerDesc, UINT RegisterIndex);
	static inline size_t AlignConstantBuffersize(size_t size)
	{
		size_t alligned_size = (size + 255) & (~255);
		return alligned_size;
	}

	static void UpdateTexture(ComPtr<ID3D12Device5>& pD3DDevice, ComPtr<ID3D12GraphicsCommandList>& pCommandList, ComPtr<ID3D12Resource>& pDestTexResource, ComPtr <ID3D12Resource>& pSrcTexResource);
};

