#pragma once

// Window
#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <initguid.h>

// D3D12
#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3d11on12.h>
#include <dwrite.h>
#include <d2d1_3.h>
#include <d3dx12.h>
#include <DirectXMath.h>

// Windows Header Files
#include <windows.h>
#include <wrl.h>
using namespace Microsoft::WRL;

// C librarys
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <assert.h>

// STL
#include <array>
#include <memory>

#include "typedef.h"
#include "D3DUtil.h"

// Debugs
#ifdef _DEBUG

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)

#endif _DEBUG

