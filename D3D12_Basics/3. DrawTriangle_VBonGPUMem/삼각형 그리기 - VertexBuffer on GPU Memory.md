# 02. 삼각형 그리기 - VertexBuffer on GPU Memory

## 삼각형 그리기 - 이번엔 GPU다
- UploadBuffer
	- D3D12 는 비동기 API 이므로 CPU 메모리와 GPU 메모리를 별도로 관
	- D3D12 에서의 Create - Update - Draw
		- CreateCommittedResource() - GPUBuffer
		- CreateCommittedResource() - UploadBuffer
		- ResourceBarrier() - Copy Dest
		- CopyResource(UploadBuffer -> GPUBuffer)
		- ResourceBarrier() - VertexBuffer
		- Draw()
		- Excute()

## 예제 분석
0. 이번에도 Root Signature 는 사용하지 않음. 또 Descriptor 를 사용하지 않기 때문
	- 다만 필요는 하므로 이전과 마찬가지로 빈 껍데기로만 만
1. ```CBasicMeshObject::CreateMesh()```
```
pResourceManager->CreateVertexBuffer(sizeof(BasicVertex), (DWORD)_countof(Vertices), &m_VertexBufferView, &m_pVertexBuffer, Vertices))
```
	- 위 부분이 변경됨
	- 원래 바로 CreateCommittedResource() 를 호출해서 바로 리소스를 생성하고 Map/Unmap을 했지만 이제 CD3D12ResourceManager 를 이용하여 GPU에 리소스를 생성함
2. ```CD3D12ResourceManager::CreateVertexBuffer(UINT SizePerVertex, DWORD dwVertexNum, D3D12_VERTEX_BUFFER_VIEW* pOutVertexBufferView, ID3D12Resource **ppOutBuffer, void* pInitData)```
```
HRESULT CD3D12ResourceManager::CreateVertexBuffer(UINT SizePerVertex, DWORD dwVertexNum, D3D12_VERTEX_BUFFER_VIEW* pOutVertexBufferView, ID3D12Resource **ppOutBuffer, void* pInitData)
{
	HRESULT hr = S_OK;

	D3D12_VERTEX_BUFFER_VIEW	VertexBufferView = {};
	ID3D12Resource*	pVertexBuffer = nullptr;
	ID3D12Resource*	pUploadBuffer = nullptr;
	UINT		VertexBufferSize = SizePerVertex * dwVertexNum;

	// create vertexbuffer for rendering
	hr = m_pD3DDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(VertexBufferSize),
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&pVertexBuffer));

	if (FAILED(hr))
	{
		__debugbreak();
		goto lb_return;
	}
	if (pInitData)
	{
		if (FAILED(m_pCommandAllocator->Reset()))
			__debugbreak();

		if (FAILED(m_pCommandList->Reset(m_pCommandAllocator, nullptr)))
			__debugbreak();

		hr = m_pD3DDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(VertexBufferSize),
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(&pUploadBuffer));

		if (FAILED(hr))
		{
			__debugbreak();
			goto lb_return;
		}
		
		// Copy the triangle data to the vertex buffer.
		UINT8* pVertexDataBegin = nullptr;
		CD3DX12_RANGE writeRange(0, 0);        // We do not intend to read from this resource on the CPU.

		hr = pUploadBuffer->Map(0, &writeRange, reinterpret_cast<void**>(&pVertexDataBegin));
		if (FAILED(hr))
		{
			__debugbreak();
			goto lb_return;
		}
		memcpy(pVertexDataBegin, pInitData, VertexBufferSize);
		pUploadBuffer->Unmap(0, nullptr);

		m_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pVertexBuffer, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
		m_pCommandList->CopyBufferRegion(pVertexBuffer, 0, pUploadBuffer, 0, VertexBufferSize);
		m_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pVertexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

		m_pCommandList->Close();

		ID3D12CommandList* ppCommandLists[] = { m_pCommandList };
		m_pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
		
		Fence();
		WaitForFenceValue();
	}
	

	// Initialize the vertex buffer view.
	VertexBufferView.BufferLocation = pVertexBuffer->GetGPUVirtualAddress();
	VertexBufferView.StrideInBytes = SizePerVertex;
	VertexBufferView.SizeInBytes = VertexBufferSize;

	*pOutVertexBufferView = VertexBufferView;
	*ppOutBuffer = pVertexBuffer;

lb_return:
	if (pUploadBuffer)
	{
		pUploadBuffer->Release();
		pUploadBuffer = nullptr;
	}
	return hr;
}
```
	- 이전 강의때의 설명대로 &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD)를 이용하여 GPU 메모리에 리소스를 만듬
	- pInitData가 있을때 이를 바로 GPU 메모리에 생성하는 법은 D3D12에는 없음
		- 대신 시스템 메모리에만들어서 GPU 메모리로 전송
			- 이전처럼 D3D12_HEAP_TYPE_UPLOAD 로 시스템 메모리에 리소스를 생성하는 것을 볼 수 있음
			- 마찬가지로 Map/Unmap 까지도 동일하게 수행
			- 아래 코드로 CPU -> GPU 로 전송을 수행함
```
		m_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pVertexBuffer, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
		m_pCommandList->CopyBufferRegion(pVertexBuffer, 0, pUploadBuffer, 0, VertexBufferSize);
		m_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pVertexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));
		
		m_pCommandList->Close();
```
			- 복사를 위해 D3D12_RESOURCE_STATE_COPY_DEST 로 Transition
			- CopyBufferRegion()으로 복사
				- 역시 바로 복사가 아닌 그냥 커맨드 리스트 입력
			- 복사가 끝나면 D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER 로 Transition함
	- ResourceManager 안의 별도의 Command Queue 와 Command List 가 존재하고 리소스 갱신만을 위해 Renderer의 Command 들과는 별도로 관리
		- Fence를 사용하는것도 마찬가지
	- 업로드를 위해 만든 시스템 메모리의 버퍼는 더이상 필요 없으므로 Release

3. 나머지는 이전과 거의 동일
	- 종료시 CD3D12ResourceManager 의 CleanUp을 수행

## 추가 샘플 코드 - 04_IndexedDrawTriangle_GpuMem

- 삼각형 그리기와 다르게 Index Buffer까지 이용하여 사각형을 그리는 예제
- D3D12_INDEX_BUFFER_VIEW 를 이용하여 생성
- D3D11 시절과 비슷하게 진행되고 삼각형 그리기와 큰 차이점은 없음
	- 다만 Index Buffer 를 위한 몇줄의 코드가 추가됨
