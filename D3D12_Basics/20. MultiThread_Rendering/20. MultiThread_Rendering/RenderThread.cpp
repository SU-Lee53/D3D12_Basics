#include "pch.h"
#include "RenderThread.h"
#include <process.h>
#include "D3D12Renderer.h"

using namespace std;

UINT WINAPI RenderThread(void* pArg)
{
	RENDER_THREAD_DESC* pDesc = (RENDER_THREAD_DESC*)(pArg);
	shared_ptr<D3D12Renderer> pRenderer = pDesc->pRenderer.lock();
	DWORD dwThreadIndex = pDesc->dwThreadIndex;
	const HANDLE* phEventList = pDesc->hEventList;

	while (1)
	{
		DWORD dwEventIndex = WaitForMultipleObjects(RENDER_THREAD_EVENT_TYPE_COUNT, phEventList, FALSE, INFINITE);

		switch (dwEventIndex)
		{
		case RENDER_THREAD_EVENT_TYPE_PROCESS:
			pRenderer->ProcessByThread(dwThreadIndex);
			break;
			
		case RENDER_THREAD_EVENT_TYPE_DESTROY:
			goto lb_exit;	// goto 안쓰려했는데 지금은 안쓰면 쓸데없이 복잡해질듯
			
		default:
			__debugbreak();
		}

	}

lb_exit:
	_endthreadex(0);
	return 0;

}

