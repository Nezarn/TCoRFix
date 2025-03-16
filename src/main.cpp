#include <MinHook.h>

#include <gl/GL.h>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#pragma comment(lib, "opengl32.lib")

// These are defined in glext.h, but its kinda waste to include just for these
#define GL_MAX_TEXTURE_UNITS              0x84E2
#define GL_MAX_TEXTURE_IMAGE_UNITS        0x8872
#define GL_FRAGMENT_PROGRAM_ARB           0x8804
#define GL_SAMPLES_PASSED_ARB             0x8914
#define APIENTRYP APIENTRY *

// Settings
bool bFixMiscBugs = false;
bool bEnable2_0PlusPlus = false;
bool bFlickerWorkaround = false;
bool bTextureSizeFix = false;

// Buffer returned for glGetString(GL_EXTENSIONS)
GLubyte extensionList[1024];

// Original functions
void (WINAPI* glColor4f_ori)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
void (WINAPI* glGetIntegerv_ori)(GLenum pname, GLint* data);
const GLubyte* (WINAPI* glGetString_ori)(GLenum name);
PROC (WINAPI* wglGetProcAddress_ori)(LPCSTR procName);

// OpenGL ARB functions
typedef void (APIENTRYP PFNGLPROGRAMSTRINGARBPROC)(GLenum target, GLenum format, GLsizei len, const void *string);
typedef void (APIENTRYP PFNGLVERTEXATTRIB4FPROC) (GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
typedef void (APIENTRYP PFNGLGENOCCLUSIONQUERIESNVPROC) (GLsizei n, GLuint *ids);
typedef void (APIENTRYP PFNGLDELETEOCCLUSIONQUERIESNVPROC) (GLsizei n, const GLuint *ids);
typedef GLboolean (APIENTRYP PFNGLISOCCLUSIONQUERYNVPROC) (GLuint id);
typedef void (APIENTRYP PFNGLBEGINOCCLUSIONQUERYNVPROC) (GLuint id);
typedef void (APIENTRYP PFNGLENDOCCLUSIONQUERYNVPROC) (void);
typedef void (APIENTRYP PFNGLGETOCCLUSIONQUERYIVNVPROC) (GLuint id, GLenum pname, GLint *params);
typedef void (APIENTRYP PFNGLGETOCCLUSIONQUERYUIVNVPROC) (GLuint id, GLenum pname, GLuint *params);
typedef void (APIENTRYP PFNGLGENQUERIESARBPROC) (GLsizei n, GLuint *ids);
typedef void (APIENTRYP PFNGLDELETEQUERIESARBPROC) (GLsizei n, const GLuint *ids);
typedef GLboolean (APIENTRYP PFNGLISQUERYARBPROC) (GLuint id);
typedef void (APIENTRYP PFNGLBEGINQUERYARBPROC) (GLenum target, GLuint id);
typedef void (APIENTRYP PFNGLENDQUERYARBPROC) (GLenum target);
typedef void (APIENTRYP PFNGLGETQUERYIVARBPROC) (GLenum target, GLenum pname, GLint *params);
typedef void (APIENTRYP PFNGLGETQUERYOBJECTIVARBPROC) (GLuint id, GLenum pname, GLint *params);
typedef void (APIENTRYP PFNGLGETQUERYOBJECTUIVARBPROC) (GLuint id, GLenum pname, GLuint *params);
PFNGLPROGRAMSTRINGARBPROC glProgramStringARB = NULL;
PFNGLVERTEXATTRIB4FPROC glVertexAttrib4f = NULL;
PFNGLGENQUERIESARBPROC glGenQueriesARB = NULL;
PFNGLDELETEQUERIESARBPROC glDeleteQueriesARB = NULL;
PFNGLISQUERYARBPROC glIsQueryARB = NULL;
PFNGLBEGINQUERYARBPROC glBeginQueryARB = NULL;
PFNGLENDQUERYARBPROC glEndQueryARB = NULL;
PFNGLGETQUERYIVARBPROC glGetQueryivARB = NULL;
PFNGLGETQUERYOBJECTIVARBPROC glGetQueryObjectivARB = NULL;
PFNGLGETQUERYOBJECTUIVARBPROC glGetQueryObjectuivARB = NULL;

// WGL
typedef BOOL (WINAPI *PFNWGLCHOOSEPIXELFORMATARBPROC)(HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats);
PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = NULL;
BOOL WINAPI wglChoosePixelFormatARB_hook(HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats);

void patch(void* addr, const char* val, size_t size)
{
	DWORD old = 0;
	VirtualProtect(addr, size, PAGE_EXECUTE_READWRITE, &old);
	memcpy(addr, val, size);
	VirtualProtect(addr, size, old, &old);
}

uint8_t checkGameArch()
{
	wchar_t path[MAX_PATH];
	GetModuleFileNameW(nullptr, path, MAX_PATH);

	if(wcsstr(path, L"Win32_x86_SSE2") != 0)
		return 3;
	else if(wcsstr(path, L"Win32_x86_SSE") != 0)
		return 2;
	else if(wcsstr(path, L"Win32_x86") != 0)
		return 1;
	else if(wcsstr(path, L"Win64_AMD64") != 0)
		return 0;

	return 4;
}

void WINAPI glGetIntegerv_hook(GLenum pname, GLint* data)
{
	// GL_MAX_TEXTURE_UNITS returns GL_INVALID_VALUE on modern GPUs (at least on NVIDIA)
	// so replace with GL_MAX_TEXTURE_IMAGE_UNITS
	if (pname == GL_MAX_TEXTURE_UNITS)
	{
		glGetIntegerv_ori(GL_MAX_TEXTURE_IMAGE_UNITS, data);
		return;
	}

	glGetIntegerv_ori(pname, data);
}

void WINAPI glColor4f_hook(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
	glVertexAttrib4f(3, red, green, blue, alpha);
}

std::vector<std::string>requiredExtensions = 
{
	"GL_ARB_fragment_program",
	"GL_ARB_multitexture",
	"GL_ARB_texture_compression",
	"GL_ARB_texture_cube_map",
	"GL_ARB_texture_env_combine",
	"GL_ARB_texture_env_dot3",
	"GL_ARB_vertex_buffer_object",
	"GL_ARB_vertex_program",
	"GL_EXT_bgra",
	"GL_EXT_secondary_color",
	"GL_EXT_stencil_two_side",
	"GL_EXT_stencil_wrap",
	"GL_EXT_texture_compression_s3tc",
	"GL_EXT_texture_filter_anisotropic",
	"GL_EXT_texture_lod_bias",
	"GL_NV_copy_depth_to_color",
	"GL_NV_fence",
	"GL_NV_fragment_program",
	"GL_NV_fragment_program_option",
	"GL_NV_fragment_program2",
	"GL_NV_occlusion_query",
	"GL_NV_register_combiners",
	"GL_NV_register_combiners2",
	"GL_NV_texture_shader",
	"GL_NV_vertex_array_range",
	"GL_NV_vertex_program"
};

const GLubyte* WINAPI glGetString_hook(GLenum name)
{
	if (name == GL_EXTENSIONS)
	{
		// This will contain our extensions
		std::string extList;

		if(bEnable2_0PlusPlus)
			extList = "GL_NV_fragment_program GL_NV_fragment_program_option GL_NV_fragment_program2 GL_NV_occlusion_query GL_NV_copy_depth_to_color ";

		// Get vendor
		std::string vendorstr = reinterpret_cast<const char*>(glGetString_ori(GL_VENDOR));

		// Get extensions and build our own list with the required ones only
		std::string extensions = reinterpret_cast<const char*>(glGetString_ori(GL_EXTENSIONS));

		for (size_t i = 0; i < requiredExtensions.size(); i++)
		{
			size_t pos = 0;
			// If we have the 2.0++ force enabled, ignore the already included extensions
			if(bEnable2_0PlusPlus && (pos = extList.find(requiredExtensions[i]) != std::string::npos))
				continue;
			pos = extensions.find(requiredExtensions[i]);
			if (pos != std::string::npos)
			{
				extList += requiredExtensions[i] + " ";
			}
		}

		memcpy(extensionList, extList.c_str(), 1024);

		return extensionList;
	}

	const GLubyte* ret = glGetString_ori(name);

	return ret;
}

std::map<std::string, std::string> shaderPatches
{
	{"OPTION NV_fragment_program2;", "TEMP tcorfix;"},
	{"SHORT TEMP", "TEMP"},
	{"SHORT OUTPUT", "OUTPUT"},
	{"MULH", "MUL"},
	{"MADH", "MAD"},
	{"MUL_SAT depth, |depth|, depthtolerance.a;", "ABS tcorfix, depth;\nMUL_SAT depth, tcorfix, depthtolerance.a;\nMOV tcorfix, {0.0, 0.0, 0.0, 0.0};"},
	{"NRMH NormalMapTexel.rgb, NormalMapTexel;", "DP3 tcorfix.a, NormalMapTexel, NormalMapTexel;\nRSQ tcorfix.a, tcorfix.a;\nMUL NormalMapTexel.rgb, NormalMapTexel, tcorfix.a;\nMOV tcorfix, {0.0, 0.0, 0.0, 0.0};"},
	{"NRMH TSLV.rgb, IPTSLV;", "DP3 tcorfix.a, IPTSLV, IPTSLV;\nRSQ tcorfix.a, tcorfix.a;\nMUL TSLV.rgb, IPTSLV, tcorfix.a;\nMOV tcorfix, {0.0, 0.0, 0.0, 0.0};"},
	{"NRMH TSEV.rgb, IPTSEV;", "DP3 tcorfix.a, IPTSEV, IPTSEV;\nRSQ tcorfix.a, tcorfix.a;\nMUL TSEV.rgb, IPTSEV, tcorfix.a;\nMOV tcorfix, {0.0, 0.0, 0.0, 0.0};"}
};

void replaceAll(std::string& str, const std::string& from, const std::string& to)
{
	size_t pos = 0;
	while ((pos = str.find(from, pos)) != std::string::npos)
	{
		str.replace(pos, from.length(), to);
		pos += to.length();
	}
}

void APIENTRY glProgramStringARB_hook(GLenum target, GLenum format, GLsizei len, const void* string)
{
	if (target == GL_FRAGMENT_PROGRAM_ARB)
	{
		std::string prog = reinterpret_cast<const char*>(string);

		if(bEnable2_0PlusPlus)
		{
			for (const auto& sp : shaderPatches)
				replaceAll(prog, sp.first, sp.second);
		}

		// Hack: I'm still not sure what is the root cause, but removing this MUL seems to work on every gpu?
		if (len == 9467 && bFlickerWorkaround) // XRShader_FP20SS_SpecNormal.fp
		{
			replaceAll(prog, "MUL r0.rgb, r0, ShadowMaskTexel.a;", "#MUL r0.rgb, r0, ShadowMaskTexel.a;");
		}

		glProgramStringARB(target, format, strlen(prog.c_str()), prog.c_str());

		return;
	}

	glProgramStringARB(target, format, len, string);
}

void APIENTRY glGenOcclusionQueriesNV_hook(GLsizei n, GLuint *ids)
{
	glGenQueriesARB(n, ids);
}

void APIENTRY glDeleteOcclusionQueriesNV_hook(GLsizei n, const GLuint *ids)
{
	glDeleteQueriesARB(n, ids);
}

GLboolean APIENTRY glIsOcclusionQueryNV_hook(GLuint id)
{
	return glIsQueryARB(id);
}

void APIENTRY glBeginOcclusionQueryNV_hook(GLuint id)
{
	glBeginQueryARB(GL_SAMPLES_PASSED_ARB, id);
}

void APIENTRY glEndOcclusionQueryNV_hook(void)
{
	glEndQueryARB(GL_SAMPLES_PASSED_ARB);
}

void APIENTRY glGetOcclusionQueryivNV_hook(GLuint id, GLenum pname, GLint *params)
{
	// pname is same in both
	glGetQueryObjectivARB(id, pname, params);
}

void APIENTRY glGetOcclusionQueryuivNV_hook(GLuint id, GLenum pname, GLuint *params)
{
	// pname is same in both
	glGetQueryObjectuivARB(id, pname, params);
}

PROC WINAPI wglGetProcAddress_hook(LPCSTR procName)
{
	if(strcmp("glProgramStringARB", procName) == 0)
	{
		// Get the ProcAddress for glProgramStringARB
		glProgramStringARB = (PFNGLPROGRAMSTRINGARBPROC)wglGetProcAddress_ori("glProgramStringARB");

		// also get address for other things game doesn't use but we do
		if(bEnable2_0PlusPlus)
		{
			glVertexAttrib4f = (PFNGLVERTEXATTRIB4FPROC)wglGetProcAddress_ori("glVertexAttrib4f");
		}

		// Return our hook
		return (PROC)glProgramStringARB_hook;
	}
	else if(strcmp("glGenOcclusionQueriesNV", procName) == 0 && bEnable2_0PlusPlus)
	{
		glGenQueriesARB = (PFNGLGENQUERIESARBPROC)wglGetProcAddress_ori("glGenQueriesARB");
		return (PROC)glGenOcclusionQueriesNV_hook;
	}
	else if(strcmp("glDeleteOcclusionQueriesNV", procName) == 0 && bEnable2_0PlusPlus)
	{
		glDeleteQueriesARB = (PFNGLDELETEQUERIESARBPROC)wglGetProcAddress_ori("glDeleteQueriesARB");
		return (PROC)glDeleteOcclusionQueriesNV_hook;
	}
	else if(strcmp("glIsOcclusionQueryNV", procName) == 0 && bEnable2_0PlusPlus)
	{
		glIsQueryARB = (PFNGLISQUERYARBPROC)wglGetProcAddress_ori("glIsQueryARB");
		return (PROC)glIsOcclusionQueryNV_hook;
	}
	else if(strcmp("glBeginOcclusionQueryNV", procName) == 0 && bEnable2_0PlusPlus)
	{
		glBeginQueryARB = (PFNGLBEGINQUERYARBPROC)wglGetProcAddress_ori("glBeginQueryARB");
		return (PROC)glBeginOcclusionQueryNV_hook;
	}
	else if(strcmp("glEndOcclusionQueryNV", procName) == 0 && bEnable2_0PlusPlus)
	{
		glEndQueryARB = (PFNGLENDQUERYARBPROC)wglGetProcAddress_ori("glEndQueryARB");
		return (PROC)glEndOcclusionQueryNV_hook;
	}
	else if(strcmp("glGetOcclusionQueryivNV", procName) == 0 && bEnable2_0PlusPlus)
	{
		glGetQueryObjectivARB = (PFNGLGETQUERYOBJECTIVARBPROC)wglGetProcAddress_ori("glGetQueryObjectivARB");
		return (PROC)glGetOcclusionQueryivNV_hook;
	}
	else if(strcmp("glGetOcclusionQueryuivNV", procName) == 0 && bEnable2_0PlusPlus)
	{
		glGetQueryObjectuivARB = (PFNGLGETQUERYOBJECTUIVARBPROC)wglGetProcAddress_ori("glGetQueryObjectuivARB");
		return (PROC)glGetOcclusionQueryuivNV_hook;
	}

	return wglGetProcAddress_ori(procName);
}

void Init()
{
	// Read settings from ini
	const char* ini = ".\\TCoRFix.ini";
	bFixMiscBugs = GetPrivateProfileIntA("General", "Fix Misc Bugs", 0, ini);
	bEnable2_0PlusPlus = GetPrivateProfileIntA("General", "Enable 2.0++ on any GPU", 0, ini);
	bFlickerWorkaround = GetPrivateProfileIntA("General", "Eyeshine Flicker Workaround", 0, ini);
	bTextureSizeFix = GetPrivateProfileIntA("General", "Fix Texture Size", 0, ini);

	// Preload ogl dll (in case its not loaded yet, so minhook can always work)
	LoadLibraryA("Opengl32.dll");

	if(bTextureSizeFix)
	{
		// Game is hardcoded to max 2048 texture size, which breaks every resolution > 2048
		// This patch removes the limit, and game allows up to what the GPU supports
		// Which fixes the broken resolutions
		const wchar_t* rndrgl = L"RndrGL.dll";
		// Load RndrGL.dll and get its base address
		LoadLibraryW(rndrgl);
		uintptr_t rndrglBase = (uintptr_t)GetModuleHandleW(rndrgl);
		// Dumb, but working way to check which exe we're running
		uint8_t arch = checkGameArch();

		switch (arch)
		{
			case 0: // x64
				patch(reinterpret_cast<void*>(rndrglBase + 0x84B7), "\xEB", 1);
				break;
			case 1: // x86
				patch(reinterpret_cast<void*>(rndrglBase + 0x63A7), "\xEB", 1);
				break;
			case 2: // x86_SSE
				patch(reinterpret_cast<void*>(rndrglBase + 0x63D7), "\xEB", 1);
				break;
			case 3: // x86_SSE2
				patch(reinterpret_cast<void*>(rndrglBase + 0x63B7), "\xEB", 1);
				break;

			default:
				break;
		}
	}

	// Init minhook
	MH_Initialize();
	MH_CreateHookApi(L"opengl32.dll", "glGetString", glGetString_hook, (void**)&glGetString_ori);

	if (bFixMiscBugs)
	{
		MH_CreateHookApi(L"opengl32.dll", "glGetIntegerv", glGetIntegerv_hook, (void**)&glGetIntegerv_ori);
	}

	if (bFlickerWorkaround)
	{
		MH_CreateHookApi(L"opengl32.dll", "wglGetProcAddress", wglGetProcAddress_hook, (void**)&wglGetProcAddress_ori);
	}

	if(bEnable2_0PlusPlus)
	{
		MH_CreateHookApi(L"opengl32.dll", "glColor4f", glColor4f_hook, (void**)&glColor4f_ori);
	}
	
	MH_EnableHook(MH_ALL_HOOKS);
}

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		Init();
		break;
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}