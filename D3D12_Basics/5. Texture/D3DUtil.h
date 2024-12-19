#pragma once



struct D3DUtils
{
	static void SetDebugLayerInfo(ComPtr<ID3D12Device> pD3DDevice);
	static void SetDefaultSamplerDesc(D3D12_STATIC_SAMPLER_DESC* pOutSamplerDesc, UINT RegisterIndex);
};

