#include "pch.h"
#include "FontManager.h"
#include "D3D12Renderer.h"

using namespace D2D1;

FontManager::FontManager()
{
}

FontManager::~FontManager()
{
}

BOOL FontManager::Initialize(std::shared_ptr<D3D12Renderer> pRenderer, ComPtr<ID3D12CommandQueue> pCommandQueue, UINT width, UINT height, BOOL bEnableDebugLayer)
{
	ComPtr<ID3D12Device> pD3DDevice = pRenderer->GetDevice();
	CreateD2D(pD3DDevice, pCommandQueue, bEnableDebugLayer);

	float fDPI = pRenderer->GetDPI();
	CreateDWrite(pD3DDevice, width, height, fDPI);

	return TRUE;
}

std::shared_ptr<FONT_HANDLE> FontManager::CreateFontObject(const std::wstring& wchFontFamilyName, float fFontSize)
{
	ComPtr<IDWriteTextFormat> pTextFormat = nullptr;
	ComPtr<IDWriteFactory5> pDWFactory = m_pDWFactory;
	ComPtr<IDWriteFontCollection1> pFontCollection = nullptr;

	if (pDWFactory)
	{
		if (FAILED(pDWFactory->CreateTextFormat(
			wchFontFamilyName.c_str(),
			pFontCollection.Get(),
			DWRITE_FONT_WEIGHT_REGULAR,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			fFontSize,
			DEFAULT_LOCALE_NAME,
			pTextFormat.GetAddressOf()
		)))
		{
			__debugbreak();
		}
	}
	std::shared_ptr<FONT_HANDLE> pFontHandle = std::make_shared<FONT_HANDLE>();
	wcscpy_s(pFontHandle->wchFontFamilyName, wchFontFamilyName.c_str());
	pFontHandle->fFontSize = fFontSize;

	if (pTextFormat)
	{
		if (FAILED(pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING)))
			__debugbreak();

		if (FAILED(pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR)))
			__debugbreak();
	}

	pFontHandle->pTextFormat = pTextFormat;
	
	return pFontHandle;
}

void FontManager::DeleteFontObject(std::shared_ptr<FONT_HANDLE>& pFontHandle)
{
	if (pFontHandle->pTextFormat)
	{
		pFontHandle->pTextFormat->Release();
		pFontHandle->pTextFormat = nullptr;
	}

	pFontHandle.reset();
}

BOOL FontManager::WriteTextToBitmap(BYTE* pDescImage, UINT DestWidth, UINT DestHeight, UINT DestPitch, int& refiOutWidth, int refiOutHeight, std::shared_ptr<FONT_HANDLE> pFontHandle, const std::wstring& wchString, DWORD dwLen)
{
	int iTextWidth = 0;
	int iTextHeight = 0;

	BOOL bResult = CreateBitmapFromText(iTextWidth, iTextHeight, pFontHandle->pTextFormat, wchString, dwLen);
	
	if (bResult)
	{
		if (iTextWidth > (int)DestWidth)
			iTextWidth = (int)DestWidth;

		if (iTextHeight > (int)DestHeight)
			iTextHeight = (int)DestHeight;

		D2D1_MAPPED_RECT mappedRect;

		if (FAILED(m_pD2DTargetBitmapRead->Map(D2D1_MAP_OPTIONS_READ, &mappedRect)))
		{
			__debugbreak();
			return FALSE;
		}

		BYTE* pDest = pDescImage;
		char* pSrc = (char*)mappedRect.bits;

		for (DWORD y = 0; y < (DWORD)iTextHeight; y++)
		{
			memcpy(pDest, pSrc, iTextWidth * 4);
			pDest += DestPitch;
			pSrc += mappedRect.pitch;
		}
		m_pD2DTargetBitmapRead->Unmap();

	}

	refiOutWidth = iTextWidth;
	refiOutHeight = iTextHeight;
	
	return bResult;
}

BOOL FontManager::CreateD2D(ComPtr<ID3D12Device>& pD3DDevice, ComPtr<ID3D12CommandQueue>& pCommandQueue, BOOL bEnableDebugLayer)
{
	// D3D11 Device / DeviceContext 를 만들고 11on12 로 Wrapping 함
	// 좀 어이가 없지만 어쩔수 없음

	UINT d3d11DeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
	D2D1_FACTORY_OPTIONS d2dFactoryOptions = {};

	ComPtr<ID2D1Factory3> pD2DFactory = nullptr;
	ComPtr<ID3D11Device> pD3D11Device = nullptr;
	ComPtr<ID3D11DeviceContext> pD3D11DeviceContext = nullptr;
	ComPtr<ID3D11On12Device> pD3D11On12Device = nullptr;

	if (bEnableDebugLayer)
	{
		d3d11DeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
	}
	d2dFactoryOptions.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;

	if (FAILED(D3D11On12CreateDevice(
		pD3DDevice.Get(),
		d3d11DeviceFlags,
		nullptr,
		0,
		(IUnknown**)pCommandQueue.GetAddressOf(),
		1,
		0,
		pD3D11Device.GetAddressOf(),
		pD3D11DeviceContext.GetAddressOf(),
		nullptr
	)))
	{
		__debugbreak();
		return FALSE;
	}

	if (FAILED(pD3D11Device->QueryInterface(IID_PPV_ARGS(pD3D11On12Device.GetAddressOf()))))
	{
		__debugbreak();
		return FALSE;
	}


	// D2D / DWrite 컴포넌트 생성
	D2D1_DEVICE_CONTEXT_OPTIONS deviceOptions = D2D1_DEVICE_CONTEXT_OPTIONS_NONE;
	if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory3), &d2dFactoryOptions, (void**)pD2DFactory.GetAddressOf())))
	{
		__debugbreak();
		return FALSE;
	}

	ComPtr<IDXGIDevice> pDXGIDevice = nullptr;
	if (FAILED(pD3D11On12Device->QueryInterface(IID_PPV_ARGS(pDXGIDevice.GetAddressOf()))))
	{
		__debugbreak();
		return FALSE;
	}
	if (FAILED(pD2DFactory->CreateDevice(pDXGIDevice.Get(), m_pD2DDevice.GetAddressOf())))
	{
		__debugbreak();
		return FALSE;
	}
	if (FAILED(m_pD2DDevice->CreateDeviceContext(deviceOptions, m_pD2DDeviceContext.GetAddressOf())))
	{
		__debugbreak();
		return FALSE;
	}

	return TRUE;
}

BOOL FontManager::CreateDWrite(ComPtr<ID3D12Device>& pD3DDevice, UINT TexWidth, UINT TexHeight, float fDPI)
{
	m_D2DBitmapWidth = TexWidth;
	m_D2DBitmapHeight = TexHeight;

	D2D1_SIZE_U size;
	size.width = TexWidth;
	size.height = TexHeight;

	D2D1_BITMAP_PROPERTIES1 bitmapProperties =
		BitmapProperties1(
			D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
			D2D1::PixelFormat(DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
			fDPI,
			fDPI
		);

	if (FAILED(m_pD2DDeviceContext->CreateBitmap(size, nullptr, 0, &bitmapProperties, m_pD2DTargetBitmap.GetAddressOf())))
	{
		__debugbreak();
		return FALSE;
	}

	bitmapProperties.bitmapOptions = D2D1_BITMAP_OPTIONS_CANNOT_DRAW | D2D1_BITMAP_OPTIONS_CPU_READ;
	if (FAILED(m_pD2DDeviceContext->CreateBitmap(size, nullptr, 0, &bitmapProperties, m_pD2DTargetBitmapRead.GetAddressOf())))
	{
		__debugbreak();
		return FALSE;
	}

	if (FAILED(m_pD2DDeviceContext->CreateSolidColorBrush(ColorF(ColorF::White), m_pWhiteBrush.GetAddressOf())))
	{
		__debugbreak();
		return FALSE;
	}

	if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory5), (IUnknown**)m_pDWFactory.GetAddressOf())))
	{
		__debugbreak();
		return FALSE;
	}

	return TRUE;
}

BOOL FontManager::CreateBitmapFromText(int& refiOutWidth, int& refiOutHeight, ComPtr<IDWriteTextFormat>& pTextFormat, const std::wstring& wchString, DWORD dwLen)
{
	BOOL bResult = FALSE;

	ComPtr<ID2D1DeviceContext> pD2DDeviceContext = m_pD2DDeviceContext;
	ComPtr<IDWriteFactory5> pDWFactory = m_pDWFactory;
	D2D1_SIZE_F max_size = pD2DDeviceContext->GetSize();
	max_size.width = (float)m_D2DBitmapWidth;
	max_size.height = (float)m_D2DBitmapHeight;

	ComPtr<IDWriteTextLayout> pTextLayout = nullptr;
	if (pDWFactory && pTextFormat)
	{
		if (FAILED(pDWFactory->CreateTextLayout(wchString.c_str(), dwLen, pTextFormat.Get(), max_size.width, max_size.height, pTextLayout.GetAddressOf())))
		{
			__debugbreak();
			return bResult;
		}
	}

	DWRITE_TEXT_METRICS metrics = {};
	if (pTextLayout)
	{
		pTextLayout->GetMetrics(&metrics);

		// 타겟 설정
		pD2DDeviceContext->SetTarget(m_pD2DTargetBitmap.Get());

		// 안티 앨리어싱 모드 설정
		pD2DDeviceContext->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);

		// 텍스트 렌더링
		pD2DDeviceContext->BeginDraw();

		pD2DDeviceContext->Clear(D2D1::ColorF(D2D1::ColorF::Black));
		
		// 자체적인 변환 행렬을 사용할 수 있지만 일단 변환되지 않도록 Identity로 설정
		pD2DDeviceContext->SetTransform(D2D1::Matrix3x2F::Identity());

		pD2DDeviceContext->DrawTextLayout(D2D1::Point2F(0.0f, 0.0f), pTextLayout.Get(), m_pWhiteBrush.Get());

		pD2DDeviceContext->EndDraw();
		
		// 여기까지 하면 TargetBitmap에 텍스트가 찍힌 텍스쳐가 만들어 짐

		// 안티 앨리어싱 모드 복구
		pD2DDeviceContext->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_DEFAULT);
		pD2DDeviceContext->SetTarget(nullptr);
	}

	int width = (int)ceil(metrics.width);
	int height = (int)ceil(metrics.height);

	D2D1_POINT_2U destPos = {};
	D2D1_RECT_U srcRect = { 0, 0, width, height };	// L, T, R, B

	// 시스템 메모리로 Read-Back 을 위한 비트맵에다가도 하나 더 복사함
	if (FAILED(m_pD2DTargetBitmapRead->CopyFromBitmap(&destPos, m_pD2DTargetBitmap.Get(), &srcRect)))
	{
		__debugbreak();
		return bResult;
	}

	refiOutWidth = width;
	refiOutHeight = height;

	bResult = TRUE;

	return bResult;
}

void FontManager::CleanUp()
{
}

void FontManager::CleanUpDWrite()
{
}

void FontManager::CleanUpD2D()
{
}
