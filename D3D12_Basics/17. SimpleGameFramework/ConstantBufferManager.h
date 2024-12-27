#pragma once

class SimpleConstantBufferPool;

class ConstantBufferManager
{
public:
	ConstantBufferManager();
	~ConstantBufferManager();
	
	BOOL Initialize(ComPtr<ID3D12Device5>& pD3DDevice, DWORD dwMaxCBVNum);
	void Reset();

	std::shared_ptr<SimpleConstantBufferPool> GetConstantBufferPool(CONSTANT_BUFFER_TYPE type);

private:
	std::array<std::shared_ptr<SimpleConstantBufferPool>, CONSTANT_BUFFER_TYPE_COUNT> m_pConstantBufferPoolList = {};
};

