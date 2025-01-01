# 19. D3D12 Renderer Architecture - Virtual Interface

- 이번 내용은 17. SimpleGameFramework 의 확장판 같은 내용임

## Virtual Interface
- 목적
	- Content Layer(CGame) 에서 Renderer 의 내부를 알 수 없도록 함
- Virtual Interface 
	- 기존에 객체를 void 포인터로 넘겨준 이유
		- Content Layer 를 이용하는 개발자가 엔진 내부를 모르도록 하기 위함
		- 차별이 아니라 서로의 영역을 분리하여 본인 개발에 집중하게 만들고 조금 더 단순하게 만들어 사고를 방지하기 위함
	- 이제 void 포인터가 아닌 필요한 인터페이스만 노출하는 객체를 만들어 Content Layer 에 넘겨주는것이 목표

## Virtual Interface 코드 구현  
1. CD3D12Renderer 의 변경
```
class CD3D12Renderer : public IRenderer
```
- Renderer 가 IRenderer 를 상속
	- IRenderer 는 나중에 
```
public:
	// Derived from IUnknown
	STDMETHODIMP			QueryInterface(REFIID, void** ppv);
	STDMETHODIMP_(ULONG)	AddRef();
	STDMETHODIMP_(ULONG)	Release();

	// Derived from IRenderer
	BOOL	__stdcall Initialize(HWND hWnd, BOOL bEnableDebugLayer, BOOL bEnableGBV);
	void	__stdcall BeginRender();
	void	__stdcall EndRender();
	void	__stdcall Present();
	BOOL	__stdcall UpdateWindowSize(DWORD dwBackBufferWidth, DWORD dwBackBufferHeight);

	IMeshObject* __stdcall CreateBasicMeshObject();

	ISprite* __stdcall CreateSpriteObject();
	ISprite* __stdcall CreateSpriteObject(const WCHAR* wchTexFileName, int PosX, int PosY, int Width, int Height);

	void*	__stdcall CreateTiledTexture(UINT TexWidth, UINT TexHeight, DWORD r, DWORD g, DWORD b);
	void*	__stdcall CreateDynamicTexture(UINT TexWidth, UINT TexHeight);
	void*	__stdcall CreateTextureFromFile(const WCHAR* wchFileName);
	void	__stdcall DeleteTexture(void* pTexHandle);

	void*	__stdcall CreateFontObject(const WCHAR* wchFontFamilyName, float fFontSize);
	void	__stdcall DeleteFontObject(void* pFontHandle);
	BOOL	__stdcall WriteTextToBitmap(BYTE* pDestImage, UINT DestWidth, UINT DestHeight, UINT DestPitch, int* piOutWidth, int* piOutHeight, void* pFontObjHandle, const WCHAR* wchString, DWORD dwLen);

	void	__stdcall RenderMeshObject(IMeshObject* pMeshObj, const XMMATRIX* pMatWorld);
	void	__stdcall RenderSpriteWithTex(void* pSprObjHandle, int iPosX, int iPosY, float fScaleX, float fScaleY, const RECT* pRect, float Z, void* pTexHandle);
	void	__stdcall RenderSprite(void* pSprObjHandle, int iPosX, int iPosY, float fScaleX, float fScaleY, float Z);
	void	__stdcall UpdateTextureWithImage(void* pTexHandle, const BYTE* pSrcBits, UINT SrcWidth, UINT SrcHeight);

	void	__stdcall SetCameraPos(float x, float y, float z);
	void	__stdcall MoveCamera(float x, float y, float z);
	void	__stdcall GetCameraPos(float* pfOutX, float* pfOutY, float* pfOutZ);
	void	__stdcall SetCamera(const XMVECTOR* pCamPos, const XMVECTOR* pCamDir, const XMVECTOR* pCamUp);
	DWORD	__stdcall GetCommandListCount();
```
- 위에서부터 설명
```
	STDMETHODIMP			QueryInterface(REFIID, void** ppv);
	STDMETHODIMP_(ULONG)	AddRef();
	STDMETHODIMP_(ULONG)	Release();
```
- STDMETHODIMP 
	- _stdcall 매크로
	- STDMETHODIMP_(ULONG) 는 ULONG 을 리턴하는 _stdcall 함수
	- 전형적인 COM 객체의 인터페이스를 구현하는 방법임
		- == COM 객체로 만들어 버리겠다 이말임
- 나머지 함수들
	- CD3D12Renderer 가 외부로 노출할 인터페이스들임
- COM 인터페이스의 실제 구현부
```
STDMETHODIMP CD3D12Renderer::QueryInterface(REFIID refiid, void** ppv)
{

	return E_NOINTERFACE;


}
STDMETHODIMP_(ULONG) CD3D12Renderer::AddRef()
{
	m_dwRefCount++;
	return m_dwRefCount;

}
STDMETHODIMP_(ULONG) CD3D12Renderer::Release()
{
	DWORD	ref_count = --m_dwRefCount;
	if (!m_dwRefCount)
		delete this;

	return ref_count;
}
```
- QueryInterface 는 기능이 없고 구색만 맞춰놓음
- AddRef, Release 는 내부의 Ref Count 를 관리
- 그 외 노출된 인터페이스는 기존의 구현에서 _stdcall 만 붙어있음

2. IRenderer
```
interface IRenderer : public IUnknown
{
	virtual BOOL	__stdcall Initialize(HWND hWnd, BOOL bEnableDebugLayer, BOOL bEnableGBV) = 0;
	virtual void	__stdcall BeginRender() = 0;
	virtual void	__stdcall EndRender() = 0;
	virtual void	__stdcall Present() = 0;
	virtual BOOL	__stdcall UpdateWindowSize(DWORD dwBackBufferWidth, DWORD dwBackBufferHeight) = 0;

	virtual IMeshObject* __stdcall CreateBasicMeshObject() = 0;

	virtual ISprite* __stdcall CreateSpriteObject() = 0;
	virtual ISprite* __stdcall CreateSpriteObject(const WCHAR* wchTexFileName, int PosX, int PosY, int Width, int Height) = 0;

	virtual void*	__stdcall CreateTiledTexture(UINT TexWidth, UINT TexHeight, DWORD r, DWORD g, DWORD b) = 0;
	virtual void*	__stdcall CreateDynamicTexture(UINT TexWidth, UINT TexHeight) = 0;
	virtual void*	__stdcall CreateTextureFromFile(const WCHAR* wchFileName) = 0;
	virtual void	__stdcall DeleteTexture(void* pTexHandle) = 0;

	virtual void*	__stdcall CreateFontObject(const WCHAR* wchFontFamilyName, float fFontSize) = 0;
	virtual void	__stdcall DeleteFontObject(void* pFontHandle) = 0;
	virtual BOOL	__stdcall WriteTextToBitmap(BYTE* pDestImage, UINT DestWidth, UINT DestHeight, UINT DestPitch, int* piOutWidth, int* piOutHeight, void* pFontObjHandle, const WCHAR* wchString, DWORD dwLen) = 0;

	virtual void	__stdcall RenderMeshObject(IMeshObject* pMeshObj, const XMMATRIX* pMatWorld) = 0;
	virtual void	__stdcall RenderSpriteWithTex(void* pSprObjHandle, int iPosX, int iPosY, float fScaleX, float fScaleY, const RECT* pRect, float Z, void* pTexHandle) = 0;
	virtual void	__stdcall RenderSprite(void* pSprObjHandle, int iPosX, int iPosY, float fScaleX, float fScaleY, float Z) = 0;
	virtual void	__stdcall UpdateTextureWithImage(void* pTexHandle, const BYTE* pSrcBits, UINT SrcWidth, UINT SrcHeight) = 0;

	virtual void	__stdcall SetCameraPos(float x, float y, float z) = 0;
	virtual void	__stdcall MoveCamera(float x, float y, float z) = 0;
	virtual void	__stdcall GetCameraPos(float* pfOutX, float* pfOutY, float* pfOutZ) = 0;
	virtual void	__stdcall SetCamera(const XMVECTOR* pCamPos, const XMVECTOR* pCamDir, const XMVECTOR* pCamUp) = 0;
	virtual DWORD	__stdcall GetCommandListCount() = 0;
};
```
- 외부로 노출되는 IRenderer 인터페이스
- 순수 가상 함수로 선언되어 구현 없이도 컴파일에 문제가 없음
- IRenderer 포인터는 내부 함수들을 순서대로 나열해서 vtable 에 보관됨
	- 외부에 제공될 API 목록을 템플릿으로 제공하는 느낌
- IUnknown
	- MS 의 COM 객체들이 공통적으로 상속받는 인터페이스 
	- 여기서 상속되는 인터페이스가 QueryInterface, AddRef, Release 임 
	- 굳이 해야하나?
		- 안하고 본인 스타일대로 가도 상관없음
		- 여기서는 COM 스타일로 가기 위해 한것

3. CGame
```
CGame.h 초반부
	IRenderer*	m_pRenderer = nullptr;
	HWND	m_hWnd = nullptr;
	ISprite* m_pSpriteObjCommon = nullptr;
```
- 이제 void 포인터가 아닌 IRenderer, ISprite 등을 이용해 객체들을 받음
	- ISprite 도 IRenderer 와 동일한 방법으로 구현됨
	- IMeshObject 도 있고 마찬가지로 IRenderer와 동일함
```
#include "IRenderer.h"
```
- cpp 구현부에서 CD3D12Renderer.h 가 아닌 인터페이스만있는 IRenderer 를 include 함
	- 내부 구현부를 알수 없고, 인터페이스를 통해 간접 호출로 사용하게 됨
```
객체의 생성과 삭제
	CreateRendererInstance(&m_pRenderer);
	m_pRenderer->Release();
```
- 이제 new - delete 가 아닌 가상 인터페이스를 통해 생성하고 COM 객체처럼 Release 함

4. CGameObject
```
class CGameObject
{
	CGame* m_pGame = nullptr;
	IRenderer* m_pRenderer = nullptr;
	IMeshObject* m_pMeshObj = nullptr;
	
	XMVECTOR m_Scale = {1.0f, 1.0f, 1.0f, 0.0f};
	XMVECTOR m_Pos = {};
	float m_fRotY = 0.0f;

	XMMATRIX m_matScale = {};
	XMMATRIX m_matRot = {};
	XMMATRIX m_matTrans = {};
	XMMATRIX m_matWorld = {};
	BOOL	m_bUpdateTransform = FALSE;

	IMeshObject* CreateBoxMeshObject();
	IMeshObject* CreateQuadMesh();

	void	UpdateTransform();
	void	Cleanup();
public:
	SORT_LINK	m_LinkInGame;
	
	BOOL	Initialize(CGame* pGame);
	void	SetPosition(float x, float y, float z);
	void	SetScale(float x, float y, float z);
	void	SetRotationY(float fRotY);
	void	Run();
	void	Render();
	CGameObject();
	~CGameObject();
};
```
- 여기도 void 포인터 없이 IRenderer 와 IMeshObject 라는 인터페이스로 변경됨

5. 여기까지 Virtual Interface 를 구현하면 Content Layer(CGame) 은 내부 구조를 몰라도 인터페이스를 이용한 간접 호출을 이용해 렌더러를 다룰 수 있음
	- 즉, 사실상 Renderer 와 Content Layer 간의 분리가 이루어짐


