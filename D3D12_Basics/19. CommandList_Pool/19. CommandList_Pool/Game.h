#pragma once

class D3D12Renderer;
class GameObject;

class Game : public std::enable_shared_from_this<Game>
{
public:
	Game();
	~Game();

	BOOL Initialize(HWND hWnd, BOOL bEnableDebugLayer, BOOL bEnableGBV);
	void Run();
	BOOL Update(ULONGLONG CurTick);
	void OnKeyDown(UINT nChar, UINT uiScanCode);
	void OnKeyUp(UINT nChar, UINT uiScanCode);
	BOOL UpdateWindowSize(DWORD dwBackBufferWidth, DWORD dwBackBufferHeight);

private:
	void Render();
	std::shared_ptr<GameObject> CreateGameObject();
	void DeleteGameObject(std::shared_ptr<GameObject> pGameObj);
	void DeleteAllGameObjects();
	
	void CleanUp();

public:
	std::shared_ptr<D3D12Renderer> GetRenderer() const { return m_pRenderer; }


private:
	std::shared_ptr<D3D12Renderer> m_pRenderer = nullptr;
	HWND m_hWnd = nullptr;
	std::shared_ptr<void> m_pSpriteObjCommon = nullptr;

	BYTE* m_pTextImage = nullptr;
	UINT m_TextImageWidth = 0;
	UINT m_TextImageHeight = 0;

	std::shared_ptr<void> m_pTextTexTexHandle = nullptr;
	std::shared_ptr<void> m_pFontObj = nullptr;

	BOOL	m_bShiftKeyDown = FALSE;

	float m_CamOffsetX = 0.0f;
	float m_CamOffsetY = 0.0f;
	float m_CamOffsetZ = 0.0f;

	std::list<std::shared_ptr<GameObject>> m_pGameObjList = {};

	ULONGLONG m_PrvFrameCheckTick = 0;
	ULONGLONG m_PrvUpdateTick = 0;
	DWORD m_FrameCount = 0;
	DWORD m_FPS = 0;
	WCHAR m_wchText[64] = {};

	ULONGLONG m_testTextUpdatedCount = 0;

};

