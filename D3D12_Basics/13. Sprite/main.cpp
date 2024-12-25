#include "pch.h"
#include "Resource.h"
#include "D3D12Renderer.h"
#include "VertexUtil.h"
#include <dxgidebug.h>

// .lib files link
#pragma comment(lib, "DXGI.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "D3DCompiler.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

// 지금 예제코드랑 다르게 ARM용 DirectXTex 를 컴파일을 못하겠음...
// 어차피 지금은 안쓰니 그냥 넘어감
//#if defined(_M_ARM64EC) || defined(_M_ARM64)
//#ifdef _DEBUG
//#pragma comment(lib, "../DirectXTex/DirectXTex/Bin/Desktop_2022/ARM64/debug/DirectXTex.lib")
//#else
//#pragma comment(lib, "../DirectXTex/DirectXTex/Bin/Desktop_2022/ARM64/release/DirectXTex.lib")
//#endif
#if defined _M_AMD64
#ifdef _DEBUG
#pragma comment(lib, "../DirectXTex/Bin/Desktop_2022/x64/debug/DirectXTex.lib")
#else
#pragma comment(lib, "../DirectXTex/Bin/Desktop_2022/x64/release/DirectXTex.lib")
#endif
#elif defined _M_IX86
#ifdef _DEBUG
#pragma comment(lib, "../DirectXTex/Bin/Desktop_2022/win32/debug/DirectXTex.lib")
#else
#pragma comment(lib, "../DirectXTex/Bin/Desktop_2022/win32/release/DirectXTex.lib")
#endif
#endif

extern "C" {__declspec(dllexport) DWORD NvOptimuisEnablement = 0x000000001; }

/////////////////////////////////////////////////////////////////////////////////////////////
// D3D12 Agility SDK Runtime 
// 
// 원래 자동으로 환경에 맞게 세팅되나 여기선 수동으로 설정한다

//extern "C" {__declspec(dllexport) extern const UINT D3D12SDKVersion = 614; }
// 강의의 SDK 버전이 현재 솔루션이 SDK 버전과 맞지않음. 확인 필요
// 일단 자동으로 설정되도록 주석처리

#ifdef _M_ARM64EC
extern "C" {__declspec(dllexport) extern const char* D3D12SDKPath = u8".\\D3D12\\arm64"; }
#elif _M_ARM64
extern "C" {__declspec(dllexport) extern const char* D3D12SDKPath = u8".\\D3D12\\arm64"; }
#elif _M_AMD64
extern "C" {__declspec(dllexport) extern const char* D3D12SDKPath = u8".\\D3D12\\x64"; }
#elif _M_IX86
extern "C" {__declspec(dllexport) extern const char* D3D12SDKPath = u8".\\D3D12\\x86"; }
#endif

/////////////////////////////////////////////////////////////////////////////////////////////

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
HWND g_hMainWindow = nullptr;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
HWND                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

// D3D12Renderer
std::shared_ptr<D3D12Renderer> g_pRenderer = nullptr;
std::shared_ptr<void> g_pMeshObj0 = nullptr;
std::shared_ptr<void> g_pMeshObj1 = nullptr;
std::shared_ptr<void> g_pSpriteObjCommon = nullptr;
std::shared_ptr<void> g_pSprite0 = nullptr;
std::shared_ptr<void> g_pSprite1 = nullptr;
std::shared_ptr<void> g_pSprite2 = nullptr;
std::shared_ptr<void> g_pSprite3 = nullptr;
std::shared_ptr<void> g_pTexHandle0 = nullptr;

float g_fRot0 = 0.f;
float g_fRot1 = 0.f;
float g_fRot2 = 0.f;

XMMATRIX g_matWorld0 = {};
XMMATRIX g_matWorld1 = {};
XMMATRIX g_matWorld2 = {};

ULONGLONG g_PrvFrameCheckTick = 0;
ULONGLONG g_PrvUpdateTick = 0;
DWORD g_FrameCount = 0;

void RunGame();
void Update();

std::shared_ptr<void> CreateBoxMeshObject();
std::shared_ptr<void> CreateQuadMesh();

#ifdef _DEBUG
void ComLeakCheck();
#endif

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.
#ifdef _DEBUG
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif _DEBUG

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_MY13SPRITE, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    g_hMainWindow = InitInstance(hInst, nCmdShow);
    if (!g_hMainWindow)
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_MY13SPRITE));

    MSG msg;

    g_pRenderer = std::make_shared<D3D12Renderer>();
#ifdef _DEBUG
    g_pRenderer->Initialize(g_hMainWindow, TRUE, TRUE); // Debug Layer ON / GPU Based Validation ON
#elif _DEBUG
    g_pRenderer->Initialize(g_hMainWindow, FALSE, FALSE);
#endif

    g_pMeshObj0 = CreateBoxMeshObject();
    g_pMeshObj1 = CreateQuadMesh();

    g_pTexHandle0 = g_pRenderer->CreateTextureFromFile(L"tex_00.dds");
    g_pSpriteObjCommon = g_pRenderer->CreateSpriteObject();

    g_pSprite0 = g_pRenderer->CreateSpriteObject(L"sprite_1024x1024.dds", 0, 0, 512, 512);
    g_pSprite1 = g_pRenderer->CreateSpriteObject(L"sprite_1024x1024.dds", 512, 0, 1024, 512);
    g_pSprite2 = g_pRenderer->CreateSpriteObject(L"sprite_1024x1024.dds", 0, 512, 512, 1024);
    g_pSprite3 = g_pRenderer->CreateSpriteObject(L"sprite_1024x1024.dds", 512, 512, 1024, 1024);


    SetWindowText(g_hMainWindow, L"Sprite");

    // Main message loop:
    while (1)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
                break;

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            // TODO: Run
            RunGame();
        }
    }

    if (g_pRenderer)
        g_pRenderer.reset();


    // for crtcheck test
    //char* byte = new char[100];
    //memset(byte, 'a', 100);

#ifdef _DEBUG
    //_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
    //_CrtDumpMemoryLeaks();
    //_ASSERT(_CrtCheckMemory());
    ComLeakCheck();
#endif _DEBUG

    return (int)msg.wParam;
}


void RunGame()
{
    g_FrameCount++;
    ULONGLONG CurTick = GetTickCount64();

    // Begin Render
    g_pRenderer->BeginRender();

    // Game Logic
    if (CurTick - g_PrvUpdateTick > 16)
    {
        // 60프레임으로 Update()
        Update();
        g_PrvUpdateTick = CurTick;
    }

    // Render
    // 하나의 오브젝트를 여러 컨텍스트로 렌더링
    g_pRenderer->RenderMeshObject(g_pMeshObj0, g_matWorld0);
    g_pRenderer->RenderMeshObject(g_pMeshObj0, g_matWorld1);

    // 다른 오브젝트를 렌더링
    g_pRenderer->RenderMeshObject(g_pMeshObj1, g_matWorld2);

    // 스프라이트
    RECT rect;
    rect.left = 0;
    rect.top = 0;
    rect.right = 256;
    rect.bottom = 256;
    g_pRenderer->RenderSpriteWithTex(g_pSpriteObjCommon, 0, 0, 0.5f, 0.5f, rect, 0.0f, g_pTexHandle0);

    rect.left = 256;
    rect.top = 0;
    rect.right = 512;
    rect.bottom = 256;
    g_pRenderer->RenderSpriteWithTex(g_pSpriteObjCommon, 256 + 5, 0, 0.5f, 0.5f, rect, 0.0f, g_pTexHandle0);

    rect.left = 0;
    rect.top = 256;
    rect.right = 256;
    rect.bottom = 512;
    g_pRenderer->RenderSpriteWithTex(g_pSpriteObjCommon, 0, 256 + 5, 0.5f, 0.5f, rect, 0.0f, g_pTexHandle0);

    rect.left = 256;
    rect.top = 256;
    rect.right = 512;
    rect.bottom = 512;
    g_pRenderer->RenderSpriteWithTex(g_pSpriteObjCommon, 256 + 5, 256 + 5, 0.5f, 0.5f, rect, 0.0f, g_pTexHandle0);

    g_pRenderer->RenderSprite(g_pSprite0, 512 + 10, 0, 0.5f, 0.5f, 1.f);
    g_pRenderer->RenderSprite(g_pSprite1, 512 + 10 + 10 + 256, 0, 0.5f, 0.5f, 1.f);
    g_pRenderer->RenderSprite(g_pSprite2, 512 + 10, 256 + 10, 0.5f, 0.5f, 1.f);
    g_pRenderer->RenderSprite(g_pSprite3, 512 + 10 + 10 + 256, 256 + 10, 0.5f, 0.5f, 0.f);

    // EndRender
    g_pRenderer->EndRender();

    // Present
    g_pRenderer->Present();

    if (CurTick - g_PrvFrameCheckTick > 1000)
    {
        g_PrvFrameCheckTick = CurTick;

        WCHAR wchTxt[64];
        swprintf_s(wchTxt, L"FPS:%u", g_FrameCount);
        SetWindowText(g_hMainWindow, wchTxt);

        g_FrameCount = 0;
    }

}

void Update()
{

    //
    // world matrix 0
    //
    g_matWorld0 = XMMatrixIdentity();

    // rotation 
    XMMATRIX matRot0 = XMMatrixRotationX(g_fRot0);

    // translation
    XMMATRIX matTrans0 = XMMatrixTranslation(-0.5f, 0.0f, 0.25f);

    // rot0 x trans0
    g_matWorld0 = XMMatrixMultiply(matRot0, matTrans0);

    //
    // world matrix 1
    //
    g_matWorld1 = XMMatrixIdentity();

    // world matrix 1
    // rotation 
    XMMATRIX matRot1 = XMMatrixRotationY(g_fRot1);

    // translation
    XMMATRIX matTrans1 = XMMatrixTranslation(0.0f, 0.0f, 0.25f);

    // rot1 x trans1
    g_matWorld1 = XMMatrixMultiply(matRot1, matTrans1);
    
    //
    // world matrix 1
    //
    g_matWorld2 = XMMatrixIdentity();

    // world matrix 1
    // rotation 
    XMMATRIX matRot2 = XMMatrixRotationY(g_fRot1);

    // translation
    XMMATRIX matTrans2 = XMMatrixTranslation(0.5f, 0.0f, 0.25f);

    // rot1 x trans1
    g_matWorld2 = XMMatrixMultiply(matRot2, matTrans2);

    BOOL	bChangeTex = FALSE;
    g_fRot0 += 0.05f;
    if (g_fRot0 > 2.0f * 3.1415f)
    {
        g_fRot0 = 0.0f;
        bChangeTex = TRUE;
    }

    g_fRot1 += 0.1f;
    if (g_fRot1 > 2.0f * 3.1415f)
    {
        g_fRot1 = 0.0f;
    }

    g_fRot2 += 0.1f;
    if (g_fRot2 > 2.0f * 3.1415f)
    {
        g_fRot2 = 0.0f;
    }
}

std::shared_ptr<void> CreateBoxMeshObject()
{
    std::shared_ptr<void> pMeshObj = nullptr;

    // Box Mesh 생성
    WORD pIndexList[36];
    std::vector<BasicVertex> VertexList = {};
    DWORD dwVertexCount = VertexUtil::CreateBoxMesh(VertexList, pIndexList, (DWORD)_countof(pIndexList), 0.25f);

    pMeshObj = g_pRenderer->CreateBasicMeshObject();

    const WCHAR* wchTexFileNameList[6] =
    {
        L"tex_00.dds",
        L"tex_01.dds",
        L"tex_02.dds",
        L"tex_03.dds",
        L"tex_04.dds",
        L"tex_05.dds"
    };

    g_pRenderer->BeginCreateMesh(pMeshObj, VertexList.data(), dwVertexCount, 6);
    for (DWORD i = 0; i < 6; i++)
    {
        g_pRenderer->InsertTriGroup(pMeshObj, pIndexList + i * 6, 2, wchTexFileNameList[i]);
    }
    g_pRenderer->EndCreateMesh(pMeshObj);

    return pMeshObj;
}

std::shared_ptr<void> CreateQuadMesh()
{
    std::shared_ptr<void> pMeshObj = nullptr;
    pMeshObj = g_pRenderer->CreateBasicMeshObject();

    BasicVertex pVertexList[] =
    {
        { { -0.25f, 0.25f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f } },
        { { 0.25f, 0.25f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f } },
        { { 0.25f, -0.25f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } },
        { { -0.25f, -0.25f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } },
    };

    WORD pIndexList[] =
    {
        0, 1, 2,
        0, 2, 3
    };


    g_pRenderer->BeginCreateMesh(pMeshObj, pVertexList, (DWORD)_countof(pVertexList), 1);
    g_pRenderer->InsertTriGroup(pMeshObj, pIndexList, 2, L"tex_06.dds");
    g_pRenderer->EndCreateMesh(pMeshObj);
    return pMeshObj;
}

#ifdef _DEBUG
void ComLeakCheck()
{
    HMODULE DxgiDebugDll = GetModuleHandleW(L"dxgidebug.dll");

    decltype(&DXGIGetDebugInterface) GetDebugInterface = reinterpret_cast<decltype(&DXGIGetDebugInterface)>(GetProcAddress(DxgiDebugDll, "DXGIGetDebugInterface"));

    ComPtr<IDXGIDebug> Debug;

    GetDebugInterface(IID_PPV_ARGS(Debug.GetAddressOf()));

    OutputDebugStringW(L"▼▽▼▽▼▽▼▽▼▽▼▽▼▽▼▽▼▽▼▽▼▽       D3D Object ref count check       ▼▽▼▽▼▽▼▽▼▽▼▽▼▽▼▽▼▽▼▽▼▽\r\n");
    Debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_DETAIL);
    OutputDebugStringW(L"△▲△▲△▲△▲△▲△▲△▲△▲△▲△▲△▲ Objects Shown above are not terminated △▲△▲△▲△▲△▲△▲△▲△▲△▲△▲△▲\r\n");
}

#endif
//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDC_MY13SPRITE));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_MY13SPRITE);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
HWND InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance; // Store instance handle in our global variable

    HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd)
    {
        return nullptr;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return hWnd;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        // Parse the menu selections:
        switch (wmId)
        {
        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    break;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        // TODO: Add any drawing code that uses hdc here...
        EndPaint(hWnd, &ps);
    }
    break;
    case WM_SIZE:
    {
        if (g_pRenderer)
        {
            RECT rect;
            ::GetClientRect(hWnd, &rect);
            DWORD dwWndWidth = rect.right - rect.left;
            DWORD dwWndHeight = rect.bottom - rect.top;
            g_pRenderer->UpdateWindowSize(dwWndWidth, dwWndHeight);
        }
    }
    break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
