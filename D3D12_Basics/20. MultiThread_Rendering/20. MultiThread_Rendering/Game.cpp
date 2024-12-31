#include "pch.h"
#include "Game.h"
#include "GameObject.h"
#include "D3D12Renderer.h"

using namespace std;

Game::Game()
{
}

Game::~Game()
{
	CleanUp();
}

BOOL Game::Initialize(HWND hWnd, BOOL bEnableDebugLayer, BOOL bEnableGBV)
{
	m_pRenderer = make_shared<D3D12Renderer>();
	m_pRenderer->Initialize(hWnd, bEnableDebugLayer, bEnableGBV);
	m_hWnd = hWnd;

	// Create Font
	m_pFontObj = m_pRenderer->CreateFontObject(L"Tahoma", 18.0f);

	// Create Texture for Text
	m_TextImageWidth = 512;
	m_TextImageHeight = 256;
	m_pTextImage = (BYTE*)malloc(m_TextImageWidth * m_TextImageHeight * 4);
	m_pTextTexTexHandle = m_pRenderer->CreateDynamicTexture(m_TextImageWidth, m_TextImageHeight);
	memset(m_pTextImage, 0, m_TextImageWidth * m_TextImageHeight * 4);

	m_pSpriteObjCommon = m_pRenderer->CreateSpriteObject();

	const DWORD GAME_OBJ_COUNT = 1000;
	for (DWORD i = 0; i < GAME_OBJ_COUNT; i++)
	{
		shared_ptr<GameObject> pGameObj = CreateGameObject();
		if (pGameObj)
		{
			float x = (float)((rand() % 21) - 10); // -10 ~ 10
			float y = 0.f;
			float z = (float)((rand() % 21) - 10); // -10 ~ 10
			pGameObj->SetPosition(x, y, z);
			float rad = (rand() % 181) * (3.1415f / 180.f);	// 0 ~ 180 -> 라디안 변환
			pGameObj->SetRotationY(rad);
		}
	}
	m_pRenderer->SetCameraPos(0.f, 0.f, -10.f);

	return TRUE;
}

void Game::Run()
{
	m_FrameCount++;

	// Begin
	ULONGLONG CurTick = GetTickCount64();

	// Game Logic
	Update(CurTick);
	Render();

	// 1초에 1번 프레임 갱신하여 윈도우 창에 표시
	if (CurTick - m_PrvFrameCheckTick > 1000)
	{
		m_PrvFrameCheckTick = CurTick;

		WCHAR wchTxt[64];
		m_FPS = m_FrameCount;
		m_dwCommandListCount = m_pRenderer->GetCommandListCount();
		swprintf_s(wchTxt, L"FPS : %u, CommandList : %u", m_FPS, m_dwCommandListCount);
		SetWindowText(m_hWnd, wchTxt);

		m_FrameCount = 0;
	}

}

BOOL Game::Update(ULONGLONG CurTick)
{
	// 0.016 초당 갱신 1번 == 60FPS
	if (CurTick - m_PrvUpdateTick < 16)
	{
		return FALSE;
	}
	m_PrvUpdateTick = CurTick;

	// Update Camera
	if (m_CamOffsetX != 0.f || m_CamOffsetY != 0.f || m_CamOffsetZ != 0.f)
	{
		m_pRenderer->MoveCamera(m_CamOffsetX, m_CamOffsetY, m_CamOffsetZ);
	}

	// Update GameObjects
	for (auto& obj : m_pGameObjList)
	{
		obj->Run();
	}

	// Draw text
	int iTextWidth = 0;
	int iTextHeight = 0;
	WCHAR	wchTxt[64] = {};
	DWORD	dwTxtLen = swprintf_s(wchTxt, L"FrameRate: %u, Commandlist : %u", m_FPS, m_dwCommandListCount);

	if (wcscmp(m_wchText, wchTxt))
	{
		m_testTextUpdatedCount++;
		// 텍스트가 변경된 경우
		memset(m_pTextImage, 0, m_TextImageWidth * m_TextImageHeight * 4);
		
		// 16번째 업데이트에서 무조건 1번 메모리가 튄다...? 
		m_pRenderer->WriteTextToBitmap(m_pTextImage, m_TextImageWidth, m_TextImageHeight, m_TextImageWidth * 4, iTextWidth, iTextHeight, m_pFontObj, wchTxt, dwTxtLen);
		m_pRenderer->UpdateTextureWithImage(m_pTextTexTexHandle, m_pTextImage, m_TextImageWidth, m_TextImageHeight);
		wcscpy_s(m_wchText, wchTxt);
	}

	return TRUE;
}

void Game::OnKeyDown(UINT nChar, UINT uiScanCode)
{
	switch (nChar)
	{
	case VK_SHIFT:
		m_bShiftKeyDown = TRUE;
		break;
	case 'W':
		if (m_bShiftKeyDown)
		{
			m_CamOffsetY = 0.05f;
		}
		else
		{
			m_CamOffsetZ = 0.05f;
		}
		break;
	case 'S':
		if (m_bShiftKeyDown)
		{
			m_CamOffsetY = -0.05f;
		}
		else
		{
			m_CamOffsetZ = -0.05f;
		}
		break;
	case 'A':
		m_CamOffsetX = -0.05f;
		break;
	case 'D':
		m_CamOffsetX = 0.05f;
		break;
	}
}

void Game::OnKeyUp(UINT nChar, UINT uiScanCode)
{
	switch (nChar)
	{
	case VK_SHIFT:
		m_bShiftKeyDown = FALSE;
		break;
	case 'W':
		m_CamOffsetY = 0.0f;
		m_CamOffsetZ = 0.0f;
		break;
	case 'S':
		m_CamOffsetY = 0.0f;
		m_CamOffsetZ = 0.0f;
		break;
	case 'A':
		m_CamOffsetX = 0.0f;
		break;
	case 'D':
		m_CamOffsetX = 0.0f;
		break;
	}
}

BOOL Game::UpdateWindowSize(DWORD dwBackBufferWidth, DWORD dwBackBufferHeight)
{
	BOOL bResult = FALSE;

	if (m_pRenderer)
	{
		bResult = m_pRenderer->UpdateWindowSize(dwBackBufferWidth, dwBackBufferHeight);
	}

	return bResult;
}

void Game::Render()
{
	//UINT64 startTime;
	//UINT64 beginRenderTime;
	//UINT64 afterRenderTime;
	//UINT64 EndRenderTime;
	//UINT64 PresentTime;

	//QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&startTime));
	m_pRenderer->BeginRender();

	//QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&beginRenderTime));

	DWORD dwObjCount = 0;
	for (auto& obj : m_pGameObjList)
	{
		obj->Render();
		dwObjCount++;
	}

	m_pRenderer->RenderSpriteWithTex(m_pSpriteObjCommon, 512 + 5, 256 + 5 + 256 + 5, 1.f, 1.f, nullptr, 0.f, m_pTextTexTexHandle);

	//QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&afterRenderTime));

	m_pRenderer->EndRender();
	//QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&EndRenderTime));

	m_pRenderer->Present();
	//QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&PresentTime));

	//UINT64 __A_beginRenderDiff = beginRenderTime - startTime;
	//UINT64 __B_renderDiff = afterRenderTime - beginRenderTime;
	//UINT64 __C_endRenderDiff = EndRenderTime - afterRenderTime;
	//UINT64 __D_presentDiff = PresentTime - EndRenderTime;
	//
	//UINT64 __E_FPS = m_FPS;

}

std::shared_ptr<GameObject> Game::CreateGameObject()
{
	shared_ptr<GameObject> pGameObject = make_shared<GameObject>();
	pGameObject->Initialize(shared_from_this());
	m_pGameObjList.push_front(pGameObject);

	return pGameObject;
}

void Game::DeleteGameObject(std::shared_ptr<GameObject> pGameObj)
{
	m_pGameObjList.erase(find(m_pGameObjList.begin(), m_pGameObjList.end(), pGameObj));
	pGameObj.reset();
}

void Game::DeleteAllGameObjects()
{
	for (auto& obj : m_pGameObjList)
	{
		obj.reset();
	}

	m_pGameObjList.clear();
}

void Game::CleanUp()
{
	DeleteAllGameObjects();

	if (m_pTextImage)
	{
		free(m_pTextImage);
		m_pTextImage = nullptr;
	}
	if (m_pRenderer)
	{
		if (m_pFontObj)
		{
			m_pRenderer->DeleteFontObject(m_pFontObj);
			m_pFontObj = nullptr;
		}

		if (m_pTextTexTexHandle)
		{
			m_pRenderer->DeleteTexture(m_pTextTexTexHandle);
			m_pTextTexTexHandle = nullptr;
		}
		if (m_pSpriteObjCommon)
		{
			m_pRenderer->DeleteSpriteObject(m_pSpriteObjCommon);
			m_pSpriteObjCommon = nullptr;
		}

		m_pRenderer.reset();
	}
}
