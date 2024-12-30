#pragma once

struct COMMAND_LIST
{
	ComPtr<ID3D12CommandAllocator> pDirectCommadAllcator;
	ComPtr<ID3D12GraphicsCommandList> pDirectCommandList;
	BOOL bClosed;
};

class CommandListPool
{
public:
	CommandListPool();
	~CommandListPool();

	BOOL Initialize(ComPtr<ID3D12Device> pDevice, D3D12_COMMAND_LIST_TYPE type, DWORD dwMaxCommandListNum);
	ComPtr<ID3D12GraphicsCommandList> GetCurrentCommandList();
	void Close();
	void CloseAndExecute(ComPtr<ID3D12CommandQueue>& pCommandQueue);
	void Reset();

public:
	// Getter
	DWORD GetTotalCmdListNum() const { return m_dwTotalCmdNum; }
	DWORD GetAllocatedCmdListNum() const { return m_dwAllocatedCmdNum; }
	DWORD GetAvailableCmdListNum() const { return m_dwAvailableCmdNum; }
	ComPtr<ID3D12Device>& GetD3DDevice() { return m_pD3DDevice; }

private:
	BOOL AddCmdList();
	std::shared_ptr<COMMAND_LIST> AllocCmdList();

private:
	void CleanUp();


private:
	ComPtr<ID3D12Device> m_pD3DDevice = nullptr;
	D3D12_COMMAND_LIST_TYPE m_CommandListType = D3D12_COMMAND_LIST_TYPE_DIRECT;
	
	DWORD m_dwAllocatedCmdNum = 0;
	DWORD m_dwAvailableCmdNum = 0;
	DWORD m_dwTotalCmdNum = 0;
	DWORD m_dwMaxCmdListNum = 0;
	std::shared_ptr<COMMAND_LIST> m_pCurCmdList = nullptr;
	
	std::list<std::shared_ptr<COMMAND_LIST>> m_AllocatedCmdList = {};
	std::list<std::shared_ptr<COMMAND_LIST>> m_AvailableCmdList = {};

};

