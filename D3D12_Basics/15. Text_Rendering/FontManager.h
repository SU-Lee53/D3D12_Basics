#pragma once


class D3D12Renderer;

class FontManager
{
public:
	FontManager();
	~FontManager();

	BOOL Initialize(std::shared_ptr<D3D12Renderer> pRenderer, ComPtr<ID3D12CommandQueue> pCommandQueue, UINT width, UINT height, BOOL bEnableDebugLayer);
	std::shared_ptr<FONT_HANDLE> CreateFontObject(const std::wstring& wchFontFamilyName, float fFontSize);
	void DeleteFontObject(std::shared_ptr<FONT_HANDLE>& pFontHandle);
	BOOL WriteTextToBitmap(BYTE* pDescImage, UINT DestWidth, UINT DestHeight, UINT DestPitch, int& refiOutWidth, int refiOutHeight, std::shared_ptr<FONT_HANDLE> pFontHandle, const std::wstring& wchString, DWORD dwLen);


private:
	BOOL CreateD2D(ComPtr<ID3D12Device>& pD3DDevice, ComPtr<ID3D12CommandQueue>& pCommandQueue, BOOL bEnableDebugLayer);
	BOOL CreateDWrite(ComPtr<ID3D12Device>& pD3DDevice, UINT TexWidth, UINT TexHeight, float fDPI);
	BOOL CreateBitmapFromText(int& refiOutWidth, int& refiOutHeight, ComPtr<IDWriteTextFormat>& pTextFormat, const std::wstring& wchString, DWORD dwLen);
	
private:
	void CleanUp();
	void CleanUpDWrite();
	void CleanUpD2D();

private:
	std::shared_ptr<D3D12Renderer>	m_pRenderer = nullptr;

	ComPtr<ID2D1Device2>			m_pD2DDevice = nullptr;
	ComPtr<ID2D1DeviceContext2>		m_pD2DDeviceContext = nullptr;

	ComPtr<ID2D1Bitmap1>			m_pD2DTargetBitmap = nullptr;
	ComPtr<ID2D1Bitmap1>			m_pD2DTargetBitmapRead = nullptr;
	ComPtr<IDWriteFontCollection1>	m_pFontCollection = nullptr;
	ComPtr<ID2D1SolidColorBrush>	m_pWhiteBrush = nullptr;

	ComPtr<IDWriteFactory5>				m_pDWFactory = nullptr;
	std::vector<DWRITE_LINE_METRICS>	m_pLineMetrics = {};
	DWORD								m_dwMaxLineMetricsNum = 0;
	UINT	m_D2DBitmapWidth = 0;
	UINT	m_D2DBitmapHeight = 0;
	
};

