#include "pch.h"
#include "D3DUtils.h"

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
