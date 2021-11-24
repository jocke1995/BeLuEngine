#ifndef CORE_H
#define CORE_H

#include <string>
#include <filesystem>

#include <locale>
#include <codecvt>
#include <vector>
#include <Windows.h>

static std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> strconverter;
inline std::string to_string(std::wstring wstr)
{
	return strconverter.to_bytes(wstr);
}
inline std::wstring to_wstring(std::string str)
{
	return strconverter.from_bytes(str);
}


template <typename T>
inline T Min(T a, T b)
{
	if (a < b)
	{
		return a;
	}
	return b;
}

template <typename T>
inline T Max(T a, T b)
{
	if (a > b)
	{
		return a;
	}
	return b;
}

inline std::string GetFileExtension(const std::string& FileName)
{
	if (FileName.find_last_of(".") != std::string::npos)
	{
		return FileName.substr(FileName.find_last_of(".") + 1);
	}
	return "";
}

#define concat(x,y) concat2(x,y)
#define concat2(x,y) x##y

enum class E_WINDOW_MODE
{
	WINDOWED,
	WINDOWED_FULLSCREEN,
	FULLSCREEN
};

enum class E_TEXTURE_TYPE
{
	UNKNOWN,
	TEXTURE2D,
	TEXTURE2DGUI,
	TEXTURECUBEMAP,
	NUM_TYPES
};

enum class E_TEXTURE2D_TYPE
{
	ALBEDO,
	ROUGHNESS,
	METALLIC,
	NORMAL,
	EMISSIVE,
	OPACITY,
	NUM_TYPES
};

enum E_LIGHT_TYPE
{
	DIRECTIONAL_LIGHT,
	POINT_LIGHT,
	SPOT_LIGHT,
	NUM_LIGHT_TYPES
};

enum E_SHADOW_RESOLUTION
{
	LOW,
	MEDIUM,
	HIGH,
	NUM_SHADOW_RESOLUTIONS,
	UNDEFINED
};

enum class E_SHADER_TYPE
{
	VS = 0,
	PS = 1,
	CS = 2,
	DXR = 3,
	UNSPECIFIED = 4
};

enum class E_CAMERA_TYPE
{
	PERSPECTIVE,
	ORTHOGRAPHIC,
	NUM_CAMERA_TYPES,
	UNDEFINED
};

// this will only call release if an object exists (prevents exceptions calling release on non existant objects)
#define BL_SAFE_RELEASE(p)		\
{								\
	if ((*p))					\
	{							\
		(*p)->Release();		\
		(*p) = nullptr;			\
	}							\
}

#define BL_SAFE_DELETE(p)		\
{								\
	if (p != nullptr)			\
	{							\
		delete p;				\
		p = nullptr;			\
	}							\
}

#define BL_SAFE_DELETE_ARRAY(p)	\
{								\
	if (p != nullptr)			\
	{							\
		delete p[];				\
		p = nullptr;			\
	}							\
}

#define TODO() //DebugBreak();

// Debug
#define SINGLE_THREADED_RENDERER false
#define ENABLE_DEBUGLAYER false
#define ENABLE_VALIDATIONGLAYER false
#define DEVELOPERMODE_DRAWBOUNDINGBOX false
//#define USE_NSIGHT_AFTERMATH

// Common
#define NUM_SWAP_BUFFERS 2
#define BIT(x) (1 << x)
#define MAXNUMBER 10000000.0f

enum F_DRAW_FLAGS
{
	NO_DEPTH = BIT(1),
	DRAW_OPAQUE = BIT(2),
	DRAW_TRANSPARENT = BIT(3),
	GIVE_SHADOW = BIT(4),
	NUM_FLAG_DRAWS = 4,
};

enum F_THREAD_FLAGS
{
	RENDER = BIT(1),
	TEST = BIT(2),
	ASYNC_BLAS = BIT(3),	// TODO
	// CopyTextures
	// PrepareNextScene ..
	// etc
	ALL = BIT(4)
};


// TODO: Make a renderCore.h with renderstuff, and keep Core.h for commonStuff
class Resource;
class UnorderedAccessView;
class ShaderResourceView;
struct Resource_UAV_SRV
{
	Resource* resource;
	UnorderedAccessView* uav;
	ShaderResourceView* srv;
};

#endif