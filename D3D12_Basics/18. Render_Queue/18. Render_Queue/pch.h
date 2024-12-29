#pragma once

// Window
#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <initguid.h>

// D3D12
#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3d11on12.h>
#include <d2d1_3.h>
#include <d2d1_3helper.h>
#include <dwrite_3.h>
#include <d3dx12.h>
#include <DirectXMath.h>
#include <DirectXColors.h>

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
#include <string>
#include <unordered_map>
#include <list>
#include <variant>
using namespace std::literals;

#include "typedef.h"
#include "../D3D_Util/D3DUtil.h"
#include "../D3D_Util/IndexCreator.h"
#include "../D3D_Util/VertexUtil.h"

// Debugs
#ifdef _DEBUG

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)

#endif _DEBUG

