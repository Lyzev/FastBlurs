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
#ifndef __CPUT_OGL_H__
#define __CPUT_OGL_H__

#include <stdio.h>
// include base headers we'll need
#include "CPUT.h"
#include "CPUTMath.h"
#include "CPUTEventHandler.h"
#include "CPUTGuiController.h"
#include "CPUTBufferOGL.h"

#include <vector>
#include <string>
using namespace std;

#define CPUT_SUPPORT_TESSELLATION
#define CPUT_SUPPORT_IMAGE_STORE
#ifdef CPUT_SUPPORT_IMAGE_STORE
#define CPUT_OPENGL_MAJOR 4
#define CPUT_OPENGL_MINOR 2
#else
#define CPUT_OPENGL_MAJOR 4
#define CPUT_OPENGL_MINOR 0
#endif 
#ifdef CPUT_OS_LINUX
#include <GL/glew.h>
#endif

//current bug with GUI on android, binding GUI constant buffer causing crash. investigating.
//#ifndef CPUT_OS_ANDROID
#define ENABLE_GUI
//#endif

// CPUT objects
#ifdef CPUT_OS_WINDOWS
#include "CPUTWindowWin.h"
#include "CPUTTimerWin.h"
#elif defined CPUT_OS_LINUX
#include "CPUTWindowX.h"
#include "CPUTTimerLinux.h"
#elif defined CPUT_OS_ANDROID
#include "CPUTWindowAndroid.h"
#include "CPUTTimerLinux.h"
#else
#error "OS not defined"
#endif

#include "CPUTMeshOGL.h"
#include "CPUTModelOGL.h"
#include "CPUTAssetSetOGL.h"
#include "CPUTAssetLibraryOGL.h"
#include "CPUTCamera.h"
#include "CPUTLight.h"
#include "CPUTMaterialOGL.h"
#include "CPUTGPUTimerGL.h"

#define CPUT_STRINGIFY2(a) #a
#define CPUT_STRINGIFY(a) CPUT_STRINGIFY2(a)
#define CPUT_PASTE2(a, b) a##b
#define CPUT_PASTE(a, b) CPUT_PASTE2(a, b)

void CheckOpenGLError(const char* stmt, const char* fname, int line);
#ifdef DEBUG
    #define GL_CHECK(stmt) do { \
            stmt; \
            CheckOpenGLError(#stmt, __FILE__, __LINE__); \
        } while (false)
#else
    #define GL_CHECK(stmt) stmt
#endif

// Types of message boxes you can create
enum CPUT_MESSAGE_BOX_TYPE
{
    CPUT_MB_OK = 0,//MB_OK | MB_ICONINFORMATION,
    CPUT_MB_ERROR,// = MB_OK | MB_ICONEXCLAMATION,
    CPUT_MB_WARNING //= MB_OK | MB_ICONWARNING
};


#define INTEL_PERFQUERIES_NONBLOCK  0x83FA
#define INTEL_PERFQUERIES_BLOCK     0x83FB

#define INTEL_PERFQUERIES_TYPE_UNSIGNED_INT    0x9402
#define INTEL_PERFQUERIES_TYPE_UNSIGNED_INT64  0x9403
#define INTEL_PERFQUERIES_TYPE_FLOAT           0x9404
#define INTEL_PERFQUERIES_TYPE_BOOL            0x9406

#define INTEL_PERFQUERIES_CATEGORY_MISC1             0x9407
#define INTEL_PERFQUERIES_CATEGORY_TIMEFRACTION      0x9408
#define INTEL_PERFQUERIES_CATEGORY_CYCLESPERTHREAD   0x9409
#define INTEL_PERFQUERIES_CATEGORY_THROUGHPUT        0x940A
#define INTEL_PERFQUERIES_CATEGORY_MISC2             0x940B



#if  defined( CPUT_FOR_OGLES )
typedef void (GL_APIENTRY * PFNGLDISPATCHCOMPUTEPROC) (GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z);
typedef void (GL_APIENTRY * PFNGLBINDIMAGETEXTUREPROC) (GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format);
typedef void (GL_APIENTRY * PFNGLPATCHPARAMETERFVPROC) (GLenum pname, const GLfloat* values);
typedef void (GL_APIENTRY * PFNGLPATCHPARAMETERIPROC) (GLenum pname, GLint value);
typedef void (GL_APIENTRY *  PFNGLSHADERSTORAGEBLOCKBINDINGPROC) (GLuint program, GLuint storageBlockIndex, GLuint storageBlockBinding);
typedef void (GL_APIENTRY * PFNGLMEMORYBARRIERPROC) (GLbitfield barriers);

typedef void (GL_APIENTRY * PFNGLDELETEPERFQUERYPROC) (GLuint id);
typedef void (GL_APIENTRY * PFNGLCREATEPERFQUERYINTELPROC) (GLuint queryId, GLuint *id);

typedef void (GL_APIENTRY * PFNGLBEGINPERFQUERYINTELPROC) (GLuint queryHandle);
typedef void (GL_APIENTRY * PFNGLENDPERFQUERYINTELPROC) (GLuint queryHandle);
typedef void (GL_APIENTRY * PFNGLQUERYCOUNTERPROC) (GLuint id, GLenum target);
typedef void (GL_APIENTRY * PFNGLGETQUERYOBJECTIVPROC) (GLuint id, GLenum pname, GLint* params);
typedef void (GL_APIENTRY * PFNGLGETQUERYOBJECTUI64VPROC) (GLuint id, GLenum pname, GLuint64* params);


typedef void (GL_APIENTRY * PFNGLGETFIRSTPERFQUERYINTELPROC) (GLuint* id);
typedef void (GL_APIENTRY * PFNGLGETNEXTPERFQUERYINTELPROC) (GLuint queryId, GLuint *id);

typedef void (GL_APIENTRY * PFNGLGETPERFQUERYDATAINTELPROC)(GLuint queryHandle, GLuint flags, GLsizei dataSize, void *data, GLuint *bytesWritten);

typedef void (GL_APIENTRY * PFNGLGETPERFQUERYINFOINTELPROC) (GLuint queryId,
    GLuint nameMaxLength, char *name,
    GLuint *counterBufferSize, GLuint *numCounters, GLuint *maxQueries,
    GLuint *unknown);

typedef void (GL_APIENTRY * PFNGLGETPERFCOUNTERINFOINTELPROC) (GLuint queryId, GLuint counterId,
    GLuint nameMaxLength, char *name,
    GLuint descMaxLength, char *desc,
    GLuint *offset, GLuint *size, GLuint *usage, GLuint *type,
    GLuint64 *unknown);

extern PFNGLDISPATCHCOMPUTEPROC glDispatchCompute;
extern PFNGLBINDIMAGETEXTUREPROC glBindImageTexture;
extern PFNGLSHADERSTORAGEBLOCKBINDINGPROC glShaderStorageBlockBinding;
extern PFNGLMEMORYBARRIERPROC glMemoryBarrier;
extern PFNGLPATCHPARAMETERIPROC glPatchParameteri;
extern PFNGLPATCHPARAMETERFVPROC glPatchParameterfv;


#define GL_WRITE_ONLY					  0x88B9
#define GL_IMAGE_2D                       0x904D
#define GL_COMPUTE_SHADER				  0x91B9
#define GL_TESS_EVALUATION_SHADER         0x8E87
#define GL_TESS_CONTROL_SHADER            0x8E88
#define GL_GEOMETRY_SHADER                0x8DD9
#define GL_PATCHES                        0x000E
#define GL_PATCH_VERTICES                 0x8E72
#define GL_READ_ONLY                      0x88B8
#define GL_READ_WRITE                     0x88BA
#define GL_SHADER_STORAGE_BUFFER		  0x90D2
#define GL_SHADER_STORAGE_BARRIER_BIT	  0x2000

#define GL_TIMESTAMP					 0x8E28

#else
typedef void (WINAPI * PFNGLDELETEPERFQUERYPROC) (GLuint id);
typedef void (WINAPI * PFNGLCREATEPERFQUERYINTELPROC) (GLuint queryId, GLuint *id);

typedef void (WINAPI * PFNGLBEGINPERFQUERYINTELPROC) (GLuint queryHandle);
typedef void (WINAPI * PFNGLENDPERFQUERYINTELPROC) (GLuint queryHandle);
//typedef void (WINAPI * PFNGLQUERYCOUNTERPROC) (GLuint id, GLenum target);
//typedef void (WINAPI * PFNGLGETQUERYOBJECTIVPROC) (GLuint id, GLenum pname, GLint* params);
//typedef void (WINAPI * PFNGLGETQUERYOBJECTUI64VPROC) (GLuint id, GLenum pname, GLuint64* params);


typedef void (WINAPI * PFNGLGETFIRSTPERFQUERYINTELPROC) (GLuint* id);
typedef void (WINAPI * PFNGLGETNEXTPERFQUERYINTELPROC) (GLuint queryId, GLuint *id);

typedef void (WINAPI * PFNGLGETPERFQUERYDATAINTELPROC)(GLuint queryHandle, GLuint flags, GLsizei dataSize, void *data, GLuint *bytesWritten);

typedef void (WINAPI * PFNGLGETPERFQUERYINFOINTELPROC) (GLuint queryId,
    GLuint nameMaxLength, char *name,
    GLuint *counterBufferSize, GLuint *numCounters, GLuint *maxQueries,
    GLuint *unknown);

typedef void (WINAPI * PFNGLGETPERFCOUNTERINFOINTELPROC) (GLuint queryId, GLuint counterId,
    GLuint nameMaxLength, char *name,
    GLuint descMaxLength, char *desc,
    GLuint *offset, GLuint *size, GLuint *usage, GLuint *type,
    GLuint64 *unknown);
#endif


extern PFNGLDELETEPERFQUERYPROC  glDeletePerfQueryINTEL;
extern PFNGLCREATEPERFQUERYINTELPROC glCreatePerfQueryINTEL;

extern PFNGLGETFIRSTPERFQUERYINTELPROC glGetFirstPerfQueryIdINTEL;
extern PFNGLGETNEXTPERFQUERYINTELPROC glGetNextPerfQueryIdINTEL;

extern PFNGLGETPERFQUERYINFOINTELPROC glGetPerfQueryInfoINTEL;
extern PFNGLGETPERFCOUNTERINFOINTELPROC glGetPerfCounterInfoINTEL;

extern PFNGLGETPERFQUERYDATAINTELPROC glGetPerfQueryDataINTEL;


extern PFNGLBEGINPERFQUERYINTELPROC glBeginPerfQueryINTEL;
extern PFNGLENDPERFQUERYINTELPROC glEndPerfQueryINTEL;

extern PFNGLQUERYCOUNTERPROC glQueryCounter;
extern PFNGLGETQUERYOBJECTIVPROC glGetQueryObjectiv;
extern PFNGLGETQUERYOBJECTUI64VPROC glGetQueryObjectui64v;

//typedef a (GL_APIENTRY * CPUT_PASTE(FPtrType_, b)) c; \
//--------------------------------------------------------------------------------------

#ifdef CPUT_FOR_OGLES3_COMPAT
#define ES3_FUNC(a, b, c) \
    typedef a (GL_APIENTRY * CPUT_PASTE(FPtrType_, b)) c; \
    CPUT_PASTE(FPtrType_, b) b;

struct CPUTOglES3CompatFuncPtrs
{
#include "CPUTOGLES3Compat.h"
};

#undef ES3_FUNC

extern CPUTOglES3CompatFuncPtrs gOGLESCompatFPtrs;

#define ES3_COMPAT(x) gOGLESCompatFPtrs.x   
#else
#define ES3_COMPAT(x) x   
#endif



// OpenGL CPUT layer
//-----------------------------------------------------------------------------
class CPUT_OGL;
extern CPUT_OGL *gpSample;

class CPUT_OGL : public CPUT
{

public:
    static CPUT_SHADER_MACRO *DEFAULT_MACROS; 


protected:

#ifdef CPUT_OS_WINDOWS
    CPUTWindowWin             *mpWindow;
#elif defined  CPUT_OS_LINUX
    CPUTWindowX               *mpWindow;
#elif defined  CPUT_OS_ANDROID
    CPUTWindowAndroid         *mpWindow;

#else
#error "No OS"
#endif

    bool                       mbShutdown;
    cString                    mResourceDirectory;
    CPUTBufferOGL             *mpPerFrameConstantBuffer;
    CPUTBufferOGL             *mpPerModelConstantBuffer;
    vector<string>				extensions;                // Table of supported extensions

#ifdef CPUT_FOR_OGL
	HDC                 mhDC;                      // Application Device Context
    HGLRC               mhRC;                      // OpenGL Rendering Contexts per worker thread
#endif
#ifdef CPUT_FOR_OGLES
    EGLDisplay          display;                   // Display device
    EGLSurface          surface;                   // Drawing surface
    EGLContext          context;                   // OpenGL Rendering Contexts
    int              width;
    int              height;  
#endif
public:
    CPUT_OGL(); 
    virtual ~CPUT_OGL();

    // context creation/destruction routines
#ifdef CPUT_OS_ANDROID
    static int32_t cput_handle_input(struct android_app* app, AInputEvent* event);
#else
	HWND CPUT_OGL::CreateDummyWindow(HINSTANCE hInst);
#endif
    CPUTResult CPUTInitialize(const cString ResourceDirectory);
	bool supportExtension(const string name);
    CPUTResult SetCPUTResourceDirectory(const cString ResourceDirectory);
    cString GetCPUTResourceDirectory() { return mResourceDirectory; }
    CPUTResult CPUTParseCommandLine(cString commandLine, CPUTWindowCreationParams *pWindowParams, cString *pFilename);
//    D3D_FEATURE_LEVEL GetFeatureLevel() { return mfeatureLevel; }

    static char* GetGLSL_VERSION();

    int CPUTMessageLoop();
    CPUTResult CPUTCreateWindowAndContext(const cString WindowTitle, CPUTWindowCreationParams windowParams);

    // CPUT interfaces
    virtual void ResizeWindow(UINT width, UINT height);
    virtual void ResizeWindowSoft(UINT width, UINT height);
    void DeviceShutdown();
    void RestartCPUT();

    void UpdatePerFrameConstantBuffer( CPUTRenderParameters &renderParams, double totalSeconds );
    void InnerExecutionLoop();

    // events
    virtual void Update(double deltaSeconds) {}
    virtual void Present() 
    { 
// This can be also divided into separate functions, 
// see proposition of interface design in CPUTWindowWin.h
#if   defined( CPUT_OS_LINUX )
    // linux stuff here from CPUTWindowLin.cpp 
#elif defined( CPUT_OS_WINDOWS )
#if   defined( CPUT_FOR_OGL )
    SwapBuffers( mhDC );		 
#elif defined( CPUT_FOR_DX11 )
    // DX11 display
#elif defined( CPUT_FOR_OGLES3 )
    // DX11 display
	 eglSwapBuffers( display, surface);
#endif
#else
#if   defined( CPUT_FOR_OGL )
    SwapBuffers( mhDC );		 
#elif defined( CPUT_FOR_OGLES3 )
    // DX11 display
	 eglSwapBuffers( display, surface);
#else
#error "Need to add something here"
#endif
#endif
    }
    
    bool HasWindow()
    {
        return (mpWindow != NULL);
    }

    virtual void Render(double deltaSeconds) = 0;
    virtual void Create()=0;
    virtual void Shutdown();
    virtual void FullscreenModeChange(bool bFullscreen) {UNREFERENCED_PARAMETER(bFullscreen);} // fires when CPUT changes to/from fullscreen mode
    virtual void ReleaseSwapChain() {}
    // virtual void ResizeWindow(UINT width, UINT height){UNREFERENCED_PARAMETER(width);UNREFERENCED_PARAMETER(height);}
    virtual CPUTResult CreateContext();

    // GUI
    void CPUTDrawGUI();

    // Event Handling
    CPUTEventHandledCode CPUTHandleKeyboardEvent(CPUTKey key, CPUTKeyState state);
    CPUTEventHandledCode CPUTHandleMouseEvent(int x, int y, int wheel, CPUTMouseState state, CPUTEventID eventID);

    // Utility functions for the sample developer
    CPUTResult CPUTToggleFullScreenMode();
    void CPUTSetFullscreenState(bool bIsFullscreen);
    bool CPUTGetFullscreenState();
    CPUTGuiController* CPUTGetGuiController();

    // Message boxes
    void CPUTMessageBox(const cString DialogBoxTitle, const cString DialogMessage);

protected:
    // private helper functions
    bool TestContextForRequiredFeatures();
    void ShutdownAndDestroy();
    virtual CPUTResult CreateOGLContext(CPUTContextCreation ContextParams);   // allow user to override DirectX context creation
    virtual CPUTResult DestroyOGLContext();  // allow user to override DirectX context destruction
    CPUTResult         CreateWindowAndContext(const cString WindowTitle, CPUTWindowCreationParams windowParams);
    CPUTResult         MakeWindow(const cString WindowTitle, CPUTWindowCreationParams windowParams);
    void               DrawLoadingFrame();

    // TODO: Put this somewhere else
//    bool               FindMatchingInputSlot(const char *pInputSlotName, const ID3DBlob *pVertexShaderBlob);
};

#endif //#ifndef __CPUT_DX11_H__
