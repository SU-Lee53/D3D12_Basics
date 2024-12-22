#include "pch.h"
#include "D3DUtil.h"

void D3DUtils::SetDebugLayerInfo(ComPtr<ID3D12Device> pD3DDevice)
{
	ComPtr<ID3D12InfoQueue> pInfoQueue;
	pD3DDevice->QueryInterface(IID_PPV_ARGS(pInfoQueue.GetAddressOf()));

	if(pInfoQueue)
	{
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);

		std::array<D3D12_MESSAGE_ID, 4> hide =
		{
			D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
			D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
			D3D12_MESSAGE_ID_EXECUTECOMMANDLISTS_WRONGSWAPCHAINBUFFERREFERENCE,
			D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE
		};
		D3D12_INFO_QUEUE_FILTER filter = {};
		filter.DenyList.NumIDs = hide.size();
		filter.DenyList.pIDList = hide.data();
		pInfoQueue->AddStorageFilterEntries(&filter);
	}
}

void D3DUtils::SetDefaultSamplerDesc(D3D12_STATIC_SAMPLER_DESC* pOutSamplerDesc, UINT RegisterIndex)
{
	pOutSamplerDesc->Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;

	pOutSamplerDesc->AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	pOutSamplerDesc->AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	pOutSamplerDesc->AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	pOutSamplerDesc->MipLODBias = 0.f;
	pOutSamplerDesc->MaxAnisotropy = 16;
	pOutSamplerDesc->ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	pOutSamplerDesc->BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
	pOutSamplerDesc->MinLOD = -FLT_MAX;
	pOutSamplerDesc->MaxLOD = D3D12_FLOAT32_MAX;
	pOutSamplerDesc->ShaderRegister = RegisterIndex;
	pOutSamplerDesc->RegisterSpace = 0;
	pOutSamplerDesc->ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
}
