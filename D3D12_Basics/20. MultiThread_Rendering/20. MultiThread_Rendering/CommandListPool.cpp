#include "pch.h"
#include "CommandListPool.h"

using namespace std;

CommandListPool::CommandListPool()
{
}

CommandListPool::~CommandListPool()
{
	CleanUp();
}

BOOL CommandListPool::Initialize(ComPtr<ID3D12Device> pDevice, D3D12_COMMAND_LIST_TYPE type, DWORD dwMaxCommandListNum)
{
	if (dwMaxCommandListNum < 2)
		__debugbreak();

	m_dwMaxCmdListNum = dwMaxCommandListNum;

	m_pD3DDevice = pDevice;

	return TRUE;
}

ComPtr<ID3D12GraphicsCommandList> CommandListPool::GetCurrentCommandList()
{
	ComPtr<ID3D12GraphicsCommandList> pCommandList = nullptr;
	if (!m_pCurCmdList)
	{
		m_pCurCmdList = AllocCmdList();
		if (!m_pCurCmdList)
		{
			__debugbreak();
			return nullptr; // nullptr;
		}
	}

	return m_pCurCmdList->pDirectCommandList;
}

void CommandListPool::Close()
{
	// 현재 인덱스의 CommandList 를 Close 만 함(m_pCurCmdList)
	if (!m_pCurCmdList)
		__debugbreak();

	if (m_pCurCmdList->bClosed)
		__debugbreak();

	if (FAILED(m_pCurCmdList->pDirectCommandList->Close()))
		__debugbreak();

	m_pCurCmdList->bClosed = TRUE;
	m_pCurCmdList = nullptr;
}

void CommandListPool::CloseAndExecute(ComPtr<ID3D12CommandQueue>& pCommandQueue)
{
	// 현재 인덱스의 CommandList 를 Close 하고 실행(m_pCurCmdList)
	if (!m_pCurCmdList)
		__debugbreak();

	if (m_pCurCmdList->bClosed)
		__debugbreak();

	if (FAILED(m_pCurCmdList->pDirectCommandList->Close()))
		__debugbreak();

	m_pCurCmdList->bClosed = TRUE;

	pCommandQueue->ExecuteCommandLists(1, (ID3D12CommandList**)m_pCurCmdList->pDirectCommandList.GetAddressOf());
	m_pCurCmdList = nullptr;
}

void CommandListPool::Reset()
{
	for (auto& cmdList : m_AllocatedCmdList)
	{
		if (FAILED(cmdList->pDirectCommadAllcator->Reset()))
		{
			__debugbreak();
		}
		if (FAILED(cmdList->pDirectCommandList->Reset(cmdList->pDirectCommadAllcator.Get(), nullptr)))
		{
			__debugbreak();
		}

		cmdList->bClosed = FALSE;
	}

	// std::list 의 splice() 로 한번에 AllocList 에서 AvailableList 로 옮김
	m_AvailableCmdList.splice(m_AvailableCmdList.begin(), m_AllocatedCmdList);

	m_dwAvailableCmdNum += m_dwAllocatedCmdNum;
	m_dwAllocatedCmdNum = 0;
}

BOOL CommandListPool::AddCmdList()
{
	BOOL bResult = FALSE;
	shared_ptr<COMMAND_LIST> pCmdList = nullptr;

	ComPtr<ID3D12CommandAllocator> pDirectCommandAllocator = nullptr;
	ComPtr<ID3D12GraphicsCommandList> pDirectCommandList = nullptr;

	if (m_dwTotalCmdNum >= m_dwMaxCmdListNum)
	{
#ifdef _DEBUG
		__debugbreak();
#endif
		return bResult;
	}

	if (FAILED(m_pD3DDevice->CreateCommandAllocator(m_CommandListType, IID_PPV_ARGS(pDirectCommandAllocator.GetAddressOf()))))
	{
#ifdef _DEBUG
		__debugbreak();
#endif
		return bResult;
	}

	if (FAILED(m_pD3DDevice->CreateCommandList(0, m_CommandListType, pDirectCommandAllocator.Get(), nullptr, IID_PPV_ARGS(pDirectCommandList.GetAddressOf()))))
	{
#ifdef _DEBUG
		__debugbreak();
#endif
		pDirectCommandAllocator.Reset();
		pDirectCommandAllocator = nullptr;
		return bResult;
	}

	pCmdList = make_shared<COMMAND_LIST>();
	pCmdList->pDirectCommandList = pDirectCommandList;
	pCmdList->pDirectCommadAllcator = pDirectCommandAllocator;
	pCmdList->bClosed = FALSE;
	m_dwTotalCmdNum++;

	m_AvailableCmdList.push_front(pCmdList);
	m_dwAvailableCmdNum++;

	bResult = TRUE;

	return bResult;
}

std::shared_ptr<COMMAND_LIST> CommandListPool::AllocCmdList()
{
	shared_ptr<COMMAND_LIST> pCmdList = nullptr;

	// Available List 에 남은게 없으면 하나 추가
	if (m_AvailableCmdList.empty())
	{
		if (!AddCmdList())
		{
			return pCmdList; // nullptr
		}
	}

	pCmdList = m_AvailableCmdList.front();

	m_AvailableCmdList.pop_front();
	m_dwAvailableCmdNum--;

	m_AllocatedCmdList.push_front(pCmdList);
	m_dwAllocatedCmdNum++;

	return pCmdList;
}

void CommandListPool::CleanUp()
{
	Reset();

	for (auto& cmdList : m_AvailableCmdList)
	{
		cmdList->pDirectCommandList.Reset();
		cmdList->pDirectCommadAllcator.Reset();
	}

	m_AvailableCmdList.clear();
}
