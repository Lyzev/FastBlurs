//--------------------------------------------------------------------------------------
// Copyright 2011 Intel Corporation
// All Rights Reserved
//
// Permission is granted to use, copy, distribute and prepare derivative works of this
// software for any purpose and without fee, provided, that the above copyright notice
// and this statement appear in all copies.  Intel makes no representations about the
// suitability of this software for any purpose.  THIS SOFTWARE IS PROVIDED "AS IS."
// INTEL SPECIFICALLY DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED, AND ALL LIABILITY,
// INCLUDING CONSEQUENTIAL AND OTHER INDIRECT DAMAGES, FOR THE USE OF THIS SOFTWARE,
// INCLUDING LIABILITY FOR INFRINGEMENT OF ANY PROPRIETARY RIGHTS, AND INCLUDING THE
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  Intel does not
// assume any responsibility for any errors which may appear in this software nor any
// responsibility to update it.
//--------------------------------------------------------------------------------------
#include "CPUT_OGL.h"
#include "CPUTRenderStateBlockOGL.h"
#include "CPUTGuiControllerOGL.h"
//#include "CPUTBufferOGL.h"
//#include "CPUTTextureOGL.h"

#ifdef CPUT_OS_WINDOWS
#define OSSleep Sleep
#else
#include <unistd.h>
#define OSSleep sleep
#endif
#ifdef CPUT_FOR_OGLES3_COMPAT
CPUTOglES3CompatFuncPtrs gOGLESCompatFPtrs;
#endif

PFNGLPATCHPARAMETERIPROC glPatchParameteri = NULL;
PFNGLPATCHPARAMETERFVPROC glPatchParameterfv = NULL;
PFNGLMEMORYBARRIERPROC glMemoryBarrier = NULL;


PFNGLDISPATCHCOMPUTEPROC glDispatchCompute = NULL;
PFNGLSHADERSTORAGEBLOCKBINDINGPROC glShaderStorageBlockBinding = NULL;
PFNGLBINDIMAGETEXTUREPROC glBindImageTexture = NULL;

PFNGLDELETEPERFQUERYPROC  glDeletePerfQueryINTEL = NULL;
PFNGLCREATEPERFQUERYINTELPROC glCreatePerfQueryINTEL = NULL;

PFNGLGETFIRSTPERFQUERYINTELPROC glGetFirstPerfQueryIdINTEL = NULL;
PFNGLGETNEXTPERFQUERYINTELPROC glGetNextPerfQueryIdINTEL = NULL;

PFNGLGETPERFQUERYINFOINTELPROC glGetPerfQueryInfoINTEL = NULL;
PFNGLGETPERFCOUNTERINFOINTELPROC glGetPerfCounterInfoINTEL = NULL;
PFNGLGETPERFQUERYDATAINTELPROC glGetPerfQueryDataINTEL = NULL;

PFNGLBEGINPERFQUERYINTELPROC glBeginPerfQueryINTEL = NULL;
PFNGLENDPERFQUERYINTELPROC glEndPerfQueryINTEL = NULL;
PFNGLGETQUERYOBJECTUI64VPROC glGetQueryObjectui64v = NULL;


PFNGLQUERYCOUNTERPROC glQueryCounter = NULL;
PFNGLGETQUERYOBJECTIVPROC glGetQueryObjectiv = NULL;

CPUTResult CPUT_OGL::DestroyOGLContext(void)
{
     // Disabling and deleting all rendering contexts
	if(display!= EGL_NO_DISPLAY)
	{
        eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		if(context != EGL_NO_CONTEXT)
		{
			eglDestroyContext(display,context);
		}
		if(surface != EGL_NO_SURFACE)
		{
			eglDestroySurface(display,surface);
		}
		eglTerminate(display);
	}
	return CPUT_SUCCESS;
}

#ifdef CPUT_OS_ANDROID
#ifndef _countof
#define _countof(array) (sizeof(array)/sizeof(array[0]))
#endif
#endif

EGLint g_CPUT_EGL_CONTEXT_MAJOR_VERSION_KHR = 0;
EGLint g_CPUT_EGL_CONTEXT_MINOR_VERSION_KHR = 0;
bool g_CPUT_GL_EGL_COMPUTE_SHADER_SUPPORTED = false;
bool g_CPUT_GL_IS_INTEL_VENDOR = false;


//-----------------------------------------------------------------------------
CPUTResult CPUT_OGL::CreateOGLContext(CPUTContextCreation ContextParams )
{
    CPUTResult result = CPUT_ERROR;
    
    
    // Get a matching FB config
    const EGLint attribs[] =
    {
        EGL_SURFACE_TYPE,     EGL_WINDOW_BIT,
        EGL_BLUE_SIZE,        8,
        EGL_GREEN_SIZE,       8,
        EGL_RED_SIZE,         8,
        EGL_ALPHA_SIZE,       8,
        EGL_DEPTH_SIZE,       24,
        EGL_STENCIL_SIZE,     8,
        //EGL_SAMPLE_BUFFERS  , 1,
        //EGL_SAMPLES         , 4,
        EGL_NONE
    };

#ifndef EGL_CONTEXT_MINOR_VERSION_KHR
	#define EGL_CONTEXT_MINOR_VERSION_KHR 0x30FB
#endif

#ifndef EGL_CONTEXT_MAJOR_VERSION_KHR
	#define EGL_CONTEXT_MAJOR_VERSION_KHR EGL_CONTEXT_CLIENT_VERSION
#endif

//    const EGLint contextAttribs[] =
//    {
//#ifdef CPUT_FOR_OGLES2
//        EGL_CONTEXT_CLIENT_VERSION, 2,
//#elif defined CPUT_FOR_OGLES3
//#if defined CPUT_FOR_OGLES3_1
//        EGL_CONTEXT_MAJOR_VERSION_KHR, 3,
//        EGL_CONTEXT_MINOR_VERSION_KHR, 1,
//#else
//        EGL_CONTEXT_CLIENT_VERSION, 3,
//#endif
//#else
//#error "No ES version specified"
//#endif
//        EGL_NONE
//    };

    
    EGLint format;
    EGLint numConfigs;
    EGLConfig config;
    EGLBoolean success = EGL_TRUE;
    
    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display == EGL_NO_DISPLAY) 
    {
        return result;
    }

    success = eglInitialize( display, 0, 0 );
    if (!success)
    {
        DEBUG_PRINT_ALWAYS((_L("Failed to initialise EGL")));
        return result;
    }

    /* Here, the application chooses the configuration it desires. In this
     * sample, we have a very simplified selection process, where we pick
     * the first EGLConfig that matches our criteria */
    success = eglChooseConfig(display, attribs, &config, 1, &numConfigs);
    if (!success)
    {
        DEBUG_PRINT_ALWAYS((_L("Failed to choose config")));
        return result;
    }

    /* EGL_NATIVE_VISUAL_ID is an attribute of the EGLConfig that is
     * guaranteed to be accepted by ANativeWindow_setBuffersGeometry().
     * As soon as we picked a EGLConfig, we can safely reconfigure the
     * ANativeWindow buffers to match, using EGL_NATIVE_VISUAL_ID. */
    eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);

#ifdef CPUT_OS_ANDROID
    ANativeWindow_setBuffersGeometry(mpWindow->GetHWnd(), 0, 0, format);
#endif
    surface = eglCreateWindowSurface(display, config, mpWindow->GetHWnd(), NULL);

    if (surface == EGL_NO_SURFACE)
    {
        DEBUG_PRINT_ALWAYS((_L("Failed to create EGLSurface")));
        return result;
    }

    const EGLint context_attribs_31_core[] = {
        EGL_CONTEXT_MAJOR_VERSION_KHR, 3,
        EGL_CONTEXT_MINOR_VERSION_KHR, 1,
        EGL_NONE
    };
    const EGLint context_attribs_30_core[] = {
        EGL_CONTEXT_MAJOR_VERSION_KHR, 3,
        EGL_CONTEXT_MINOR_VERSION_KHR, 0,
        EGL_NONE
    };
    const EGLint context_attribs_20_core[] = {
        EGL_CONTEXT_MAJOR_VERSION_KHR, 2,
        EGL_CONTEXT_MINOR_VERSION_KHR, 0,
        EGL_NONE
    };
    const EGLint context_attribs_legacy[] = {
        EGL_NONE
    };

    const EGLint *context_attribs_array[] = { context_attribs_31_core, context_attribs_30_core, context_attribs_20_core };
    int context_attribs_array_count = _countof( context_attribs_array );

    // pick first working context
    for( int i = 0; i < context_attribs_array_count; i++ )
    {
        context = eglCreateContext( display, config, NULL, context_attribs_array[i] );
        if (context == EGL_NO_CONTEXT)
        {
            DEBUG_PRINT_ALWAYS(( _L("Failed to create EGLContext %d"), i ));
            DEBUG_PRINT( _L("    contextAttribs: %x %x\n"), context_attribs_array[i][0], context_attribs_array[i][1]) ;
            DEBUG_PRINT( _L("    contextAttribs: %x %x\n"), context_attribs_array[i][2], context_attribs_array[i][3]) ;
        }
        else
        {
            DEBUG_PRINT( _L("Context %d created successfully: \n"), i );
            g_CPUT_EGL_CONTEXT_MAJOR_VERSION_KHR = context_attribs_array[i][1];
            g_CPUT_EGL_CONTEXT_MINOR_VERSION_KHR = context_attribs_array[i][3];
            g_CPUT_GL_EGL_COMPUTE_SHADER_SUPPORTED = g_CPUT_EGL_CONTEXT_MINOR_VERSION_KHR >= 1;
            DEBUG_PRINT( _L("    contextAttribs: %x %x\n"), context_attribs_array[i][0], context_attribs_array[i][1]) ;
            DEBUG_PRINT( _L("    contextAttribs: %x %x\n"), context_attribs_array[i][2], context_attribs_array[i][3]) ;
            break;
        }
    }
    if (context == EGL_NO_CONTEXT)
    {
        DEBUG_PRINT_ALWAYS((_L("Failed to create any EGLContext")));
        return result;
    }
    
    if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE) 
    {
        DEBUG_PRINT_ALWAYS((_L("Failed to eglMakeCurrent")));
        return result;
    }
	eglSwapInterval( display, 0 );
 
    eglQuerySurface(display, surface, EGL_WIDTH, &width);
    eglQuerySurface(display, surface, EGL_HEIGHT, &height);
	mpWindow->SetClientDimensions(width, height);
    
    glClearColor ( 0, 0.5, 1, 1 );
    glClear ( GL_COLOR_BUFFER_BIT );
    eglSwapBuffers( display, surface);
    OSSleep( 1 );
 
    glClearColor ( 1, 0.5, 0, 1 );
    glClear ( GL_COLOR_BUFFER_BIT );
    eglSwapBuffers( display, surface);
 
    OSSleep( 1 );

    DEBUG_PRINT((_L("GL Version: %s\n")), glGetString(GL_VERSION));
    
   // Creates table of supported extensions strings
    extensions.clear();
    string tmp;
    sint32 begin, end;
    tmp   = string( (char*)glGetString( GL_EXTENSIONS ) );

    begin = 0;
    end   = tmp.find( ' ', 0 );

    DEBUG_PRINT(_L("Checking Extensions"));
    
    while( end != string::npos )
    {
        DEBUG_PRINT((_L("extension %s")), tmp.substr( begin, end-begin ).c_str());
        extensions.insert( extensions.end(), tmp.substr( begin, end-begin ) );
        begin = end + 1;
        end   = tmp.find( ' ', begin );

    }

	if(supportExtension("GL_INTEL_tessellation"))
	{
		glPatchParameteri = (PFNGLPATCHPARAMETERIPROC)eglGetProcAddress("glPatchParameteri");
        DEBUG_PRINT(_L("%s = %p"), "glPatchParameteri",(void*)glPatchParameteri);
		glPatchParameterfv = (PFNGLPATCHPARAMETERFVPROC)eglGetProcAddress("glPatchParameterfv");
        DEBUG_PRINT(_L("%s = %p"), "glPatchParameterfv",(void*)glPatchParameterfv);
	}
#ifndef  CPUT_FOR_OGLES3_1
	if(supportExtension("GL_INTEL_compute_shader"))
#endif
	{
		glDispatchCompute = (PFNGLDISPATCHCOMPUTEPROC)eglGetProcAddress("glDispatchCompute");
        DEBUG_PRINT(_L("%s = %p"), "glDispatchCompute",(void*)glDispatchCompute);
		glBindImageTexture = (PFNGLBINDIMAGETEXTUREPROC)eglGetProcAddress("glBindImageTexture");
        DEBUG_PRINT(_L("%s = %p"), "glBindImageTexture",(void*)glBindImageTexture);
		glShaderStorageBlockBinding = (PFNGLSHADERSTORAGEBLOCKBINDINGPROC)eglGetProcAddress("glShaderStorageBlockBinding");
        DEBUG_PRINT(_L("%s = %p"), "glShaderStorageBlockBinding",(void*)glShaderStorageBlockBinding);
		glMemoryBarrier = (PFNGLMEMORYBARRIERPROC)eglGetProcAddress("glMemoryBarrier");
        DEBUG_PRINT(_L("%s = %p"), "glMemoryBarrier",(void*)glMemoryBarrier);
	}

    std::string vendorName = (const char *)glGetString( GL_VENDOR );
    g_CPUT_GL_IS_INTEL_VENDOR = false;
    if( (vendorName == "Intel") || supportExtension("GL_INTEL_performance_queries") )
        g_CPUT_GL_IS_INTEL_VENDOR = true;

	if(supportExtension("GL_INTEL_performance_queries"))
	{
		glDeletePerfQueryINTEL = (PFNGLDELETEPERFQUERYPROC)eglGetProcAddress("glDeletePerfQueryINTEL");
		glCreatePerfQueryINTEL = (PFNGLCREATEPERFQUERYINTELPROC)eglGetProcAddress("glCreatePerfQueryINTEL");

		glGetFirstPerfQueryIdINTEL = (PFNGLGETFIRSTPERFQUERYINTELPROC)eglGetProcAddress("glGetFirstPerfQueryIdINTEL");
		glGetNextPerfQueryIdINTEL = (PFNGLGETNEXTPERFQUERYINTELPROC)eglGetProcAddress("glGetNextPerfQueryIdINTEL");

		glGetPerfQueryInfoINTEL = (PFNGLGETPERFQUERYINFOINTELPROC)eglGetProcAddress("glGetPerfQueryInfoINTEL");
		glGetPerfCounterInfoINTEL = (PFNGLGETPERFCOUNTERINFOINTELPROC)eglGetProcAddress("glGetPerfCounterInfoINTEL");

		glGetPerfQueryDataINTEL = (PFNGLGETPERFQUERYDATAINTELPROC)eglGetProcAddress("glGetPerfQueryDataINTEL");

		glBeginPerfQueryINTEL = (PFNGLBEGINPERFQUERYINTELPROC)eglGetProcAddress("glBeginPerfQueryINTEL");
		glEndPerfQueryINTEL = (PFNGLENDPERFQUERYINTELPROC)eglGetProcAddress("glEndPerfQueryINTEL");

	}
	if(supportExtension("GL_INTEL_timer_query"))
	{
		glQueryCounter =  (PFNGLQUERYCOUNTERPROC)eglGetProcAddress("glQueryCounterINTEL");
		glGetQueryObjectiv =  (PFNGLGETQUERYOBJECTIVPROC)eglGetProcAddress("glGetQueryObjectivINTEL");
		glGetQueryObjectui64v =  (PFNGLGETQUERYOBJECTUI64VPROC)eglGetProcAddress("glGetQueryObjectui64vINTEL");
	}

//	if(supportExtension("GL_INTEL_shader_image_load_store"))
//	{
//		glBindImageTexture = (PFNGLBINDIMAGETEXTUREPROC)eglGetProcAddress("glBindImageTexture");
//        DEBUG_PRINT(_L("%s = %p"), "glBindImageTexture",(void*)glBindImageTexture);
//	}



#ifdef CPUT_FOR_OGLES3_COMPAT
    // Set up the ES3 compat functions
    memset((void *)&gOGLESCompatFPtrs, 0, sizeof(CPUTOglES3CompatFuncPtrs));

#define ES3_FUNC(a, b, c) \
    gOGLESCompatFPtrs.b = (CPUT_PASTE(CPUTOglES3CompatFuncPtrs::FPtrType_, b)) eglGetProcAddress(CPUT_STRINGIFY(b)); \
    DEBUG_PRINT("%s = %p", CPUT_STRINGIFY(b), gOGLESCompatFPtrs.b); \
    if (! gOGLESCompatFPtrs.b) \
        return CPUT_ERROR;
    
#include "CPUTOGLES3Compat.h"
    
#undef ES3_FUNC
#endif  

    return CPUT_SUCCESS;
}