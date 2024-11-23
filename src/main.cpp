#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <MinHook.h>
#include <string>
#include <gl/GL.h>
#include <iostream>
#pragma comment(lib, "opengl32.lib")

// These are defined in glext.h, but its kinda waste to include just for these 3
#define GL_MAX_TEXTURE_UNITS              0x84E2
#define GL_MAX_TEXTURE_IMAGE_UNITS        0x8872
#define GL_FRAGMENT_PROGRAM_ARB           0x8804

// Settings
bool fixMiscBugs = false;
bool enableNVonATI = false;
bool flickerWorkaround = false;

// Buffer returned for glGetString(GL_EXTENSIONS)
GLubyte ret[1024];

// Original functions
void (WINAPI* glGetIntegerv_ori)(GLenum pname, GLint* data);
const GLubyte* (WINAPI* glGetString_ori)(GLenum name);
PROC (WINAPI* wglGetProcAddress_ori)(LPCSTR procName);

// OpenGL ARB functions
typedef void (APIENTRY *PFNGLPROGRAMSTRINGARBPROC)(GLenum target, GLenum format, GLsizei len, const void *string);
PFNGLPROGRAMSTRINGARBPROC glProgramStringARB = NULL;

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

const GLubyte* WINAPI glGetString_hook(GLenum name)
{
	if (name == GL_EXTENSIONS)
	{
		// This will contain our extensions
		std::string extList;

		// Get vendor
		std::string vendorstr = reinterpret_cast<const char*>(glGetString_ori(GL_VENDOR));

		// Check vendor
		size_t pos = vendorstr.find("NVIDIA");
		if (pos != std::string::npos)
		{
			extList = "GL_ARB_FRAGMENT_PROGRAM GL_ARB_MULTITEXTURE GL_ARB_TEXTURE_COMPRESSION GL_ARB_TEXTURE_CUBE_MAP GL_ARB_TEXTURE_ENV_COMBINE GL_ARB_TEXTURE_ENV_DOT3 "
					"GL_ARB_VERTEX_BUFFER_OBJECT GL_ARB_VERTEX_PROGRAM GL_EXT_BGRA GL_EXT_SECONDARY_COLOR GL_EXT_STENCIL_TWO_SIDE GL_EXT_STENCIL_WRAP GL_EXT_TEXTURE_COMPRESSION_S3TC "
					"GL_EXT_TEXTURE_FILTER_ANISOTROPIC GL_EXT_TEXTURE_LOD_BIAS GL_NV_COPY_DEPTH_TO_COLOR GL_NV_FENCE GL_NV_FRAGMENT_PROGRAM GL_NV_FRAGMENT_PROGRAM_OPTION "
					"GL_NV_FRAGMENT_PROGRAM2 GL_NV_OCCLUSION_QUERY GL_NV_REGISTER_COMBINERS GL_NV_REGISTER_COMBINERS2 GL_NV_TEXTURE_SHADER GL_NV_VERTEX_ARRAY_RANGE GL_NV_VERTEX_PROGRAM";
		}
		else if ((pos = vendorstr.find("ATI")) != std::string::npos) // AMD still returns ATI Technologies Inc.
		{
			// Return extra extensions so we have 2.0++ option enabled
			// This works because while AMD doesn't expose GL_NV_FRAGMENT_PROGRAM2 (and GL_NV_vertex_program3) it actually works.
			if (enableNVonATI)
			{
				extList = "GL_ARB_FRAGMENT_PROGRAM GL_ARB_MULTITEXTURE GL_ARB_TEXTURE_COMPRESSION GL_ARB_TEXTURE_CUBE_MAP GL_ARB_TEXTURE_ENV_COMBINE GL_ARB_TEXTURE_ENV_DOT3 "
					"GL_ARB_VERTEX_BUFFER_OBJECT GL_ARB_VERTEX_PROGRAM GL_EXT_BGRA GL_EXT_SECONDARY_COLOR GL_EXT_STENCIL_TWO_SIDE GL_EXT_STENCIL_WRAP GL_EXT_TEXTURE_COMPRESSION_S3TC "
					"GL_EXT_TEXTURE_FILTER_ANISOTROPIC GL_EXT_TEXTURE_LOD_BIAS GL_NV_COPY_DEPTH_TO_COLOR GL_NV_FENCE GL_NV_FRAGMENT_PROGRAM GL_NV_FRAGMENT_PROGRAM_OPTION "
					"GL_NV_FRAGMENT_PROGRAM2 GL_NV_OCCLUSION_QUERY GL_NV_TEXTURE_SHADER GL_NV_VERTEX_ARRAY_RANGE GL_NV_VERTEX_PROGRAM";
			}
			else
			{
				extList = "GL_ARB_FRAGMENT_PROGRAM GL_ARB_MULTITEXTURE GL_ARB_TEXTURE_COMPRESSION GL_ARB_TEXTURE_CUBE_MAP GL_ARB_TEXTURE_ENV_COMBINE GL_ARB_TEXTURE_ENV_DOT3 "
					"GL_ARB_VERTEX_BUFFER_OBJECT GL_ARB_VERTEX_PROGRAM GL_EXT_BGRA GL_EXT_SECONDARY_COLOR GL_EXT_STENCIL_WRAP GL_EXT_TEXTURE_COMPRESSION_S3TC "
					"GL_EXT_TEXTURE_FILTER_ANISOTROPIC GL_EXT_TEXTURE_LOD_BIAS ";
			}
		}

		memcpy(ret, extList.c_str(), 1024);

		return ret;
	}

	const GLubyte* ret = glGetString_ori(name);

	return ret;
}

void APIENTRY glProgramStringARB_hook(GLenum target, GLenum format, GLsizei len, const void* string)
{
	if(target == GL_FRAGMENT_PROGRAM_ARB && len == 9467) // XRShader_FP20SS_SpecNormal.fp
	{
		std::string prog = reinterpret_cast<const char*>(string);
		std::string tofind = "DP4 r0.a, smpacked0, 0.125;";
		size_t pos = prog.find(tofind);
		if (pos != std::string::npos)
		{
			// Kinda hacky but seems to work.
			prog.replace(pos, tofind.length(), "DP4 r0.a, smpacked0, 0.125;\nNRM r0, r0;\nNRM r0, r0;");
			glProgramStringARB(target, format, strlen(prog.c_str()), prog.c_str());
			
			return;
		}
	}

	glProgramStringARB(target, format, len, string);
}

PROC WINAPI wglGetProcAddress_hook(LPCSTR procName)
{
	if(strcmp("glProgramStringARB", procName) == 0)
	{
		// Get the ProcAddress for glProgramStringARB
		glProgramStringARB = (PFNGLPROGRAMSTRINGARBPROC)wglGetProcAddress_ori("glProgramStringARB");
		// Return our hook
		return (PROC)glProgramStringARB_hook;
	}

	return wglGetProcAddress_ori(procName);
}

void Init()
{
	// Read settings from ini
	const char* ini = ".\\TCoRFix.ini";
	fixMiscBugs = GetPrivateProfileIntA("General", "Fix Misc Bugs", 0, ini);
	enableNVonATI = GetPrivateProfileIntA("General", "Enable 2.0++ on AMD/ATI", 0, ini);
	flickerWorkaround = GetPrivateProfileIntA("General", "Eyeshine Flicker Workaround", 0, ini);

	// Preload ogl dll (in case its not loaded yet, so minhook can always work)
	LoadLibraryA("opengl32.dll");

	// Init minhook
	MH_Initialize();
	MH_CreateHookApi(L"opengl32.dll", "glGetString", glGetString_hook, (void**)&glGetString_ori);

	if (fixMiscBugs)
	{
		MH_CreateHookApi(L"opengl32.dll", "glGetIntegerv", glGetIntegerv_hook, (void**)&glGetIntegerv_ori);
	}

	if (flickerWorkaround)
	{
		MH_CreateHookApi(L"opengl32.dll", "wglGetProcAddress", wglGetProcAddress_hook, (void**)&wglGetProcAddress_ori);
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