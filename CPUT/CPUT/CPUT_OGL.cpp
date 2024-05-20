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
#include "CPUTGPUTimerGL.h"
//#include "CPUTBufferOGL.h"
//#include "CPUTTextureOGL.h"

//#ifdef CPUT_FOR_OGLES
//#ifdef CPUT_FOR_OGLES3_1
//#define CPUT_GLSL_VERSION "#version 310 es \n"
//#else
//#define CPUT_GLSL_VERSION "#version 300 es \n"
//#endif
//#else
//#ifdef CPUT_SUPPORT_IMAGE_STORE
//#define CPUT_GLSL_VERSION "#version 430 \n"
//#else
//#define CPUT_GLSL_VERSION "#version 400 \n"
//#endif
//#endif

extern bool g_CPUT_GL_EGL_COMPUTE_SHADER_SUPPORTED;
extern bool g_CPUT_GL_IS_INTEL_VENDOR;
#ifdef CPUT_FOR_OGLES
extern EGLint g_CPUT_EGL_CONTEXT_MAJOR_VERSION_KHR;
extern EGLint g_CPUT_EGL_CONTEXT_MINOR_VERSION_KHR;
#else
extern bool g_CPUT_GL_IS_AMD_VENDOR;
#endif


// static initializers
CPUT_OGL *gpSample;

void CheckOpenGLError(const char* stmt, const char* fname, int line)
{
    GLenum err = glGetError();
    if (err != GL_NO_ERROR)
    {
#ifdef CPUT_OS_ANDROID
        LOGW("OpenGL error %08x, at %s:%i - for %s\n", err, fname, line, stmt);
#else
        DEBUG_PRINT(_L("OpenGL error %08x, at %s:%i - for %s\n"), err, fname, line, stmt);
#endif
        abort();
    }
}

//char* CPUT_OGL::GLSL_VERSION = NULL;
CPUT_SHADER_MACRO* CPUT_OGL::DEFAULT_MACROS = NULL;

char* CPUT_OGL::GetGLSL_VERSION()
{
            //GLSL_VERSION = CPUT_GLSL_VERSION;
#ifdef CPUT_FOR_OGLES
        if( g_CPUT_EGL_CONTEXT_MINOR_VERSION_KHR >= 1 )
        {
            return "#version 310 es \n";
        }
        else
        {
            return "#version 300 es \n";
        }
#else
    #ifdef CPUT_SUPPORT_IMAGE_STORE
        return "#version 430 \n";
    #else
        return "#version 400 \n";
    #endif
#endif

}

CPUT_OGL::CPUT_OGL() :
        mpWindow(NULL),
        mbShutdown(false),
        mpPerFrameConstantBuffer(NULL),
        mpPerModelConstantBuffer(NULL)
    {
        DEFAULT_MACROS = NULL;

        gpSample = this;
#if defined CPUT_OS_LINUX
        mpTimer = (CPUTTimer*) new CPUTTimerLinux();
#elif defined CPUT_OS_ANDROID
        mpTimer = (CPUTTimer*) new CPUTTimerLinux();
#elif defined CPUT_OS_WINDOWS
        mpTimer = (CPUTTimer*) new CPUTTimerWin();
#else
#error "No OS defined"
#endif
    }
 


// Destructor
//-----------------------------------------------------------------------------
CPUT_OGL::~CPUT_OGL()
{
    // all previous shutdown tasks should have happened in CPUTShutdown()

	SAFE_DELETE(mpTimer);
    

}




// initialize the CPUT system
//-----------------------------------------------------------------------------

CPUTResult CPUT_OGL::CPUTInitialize(const cString pCPUTResourcePath)
{
    // set where CPUT will look for it's button images, fonts, etc
    return SetCPUTResourceDirectory(pCPUTResourcePath);
}


// Set where CPUT will look for it's button images, fonts, etc
//-----------------------------------------------------------------------------
CPUTResult CPUT_OGL::SetCPUTResourceDirectory(const cString ResourceDirectory)
{
    CPUTResult result = CPUT_SUCCESS;

    // resolve the directory to a full path
    cString fullPath;
    result = CPUTFileSystem::ResolveAbsolutePathAndFilename(ResourceDirectory, &fullPath);
    if (CPUTFAILED(result)) {
        return result;
    }
	
    // check existence of directory
    result = CPUTFileSystem::DoesDirectoryExist(fullPath);

    if(CPUTFAILED(result)) {
        return result;
    }

    // set the resource directory (absolute path)
    mResourceDirectory = fullPath;



    // tell the gui system where to look for it's resources
    // todo: do we want to force a flush/reload of all resources (i.e. change control graphics)
//    result = CPUTGuiController::GetController()->SetResourceDirectory(ResourceDirectory);

    return result;
}

// Handle keyboard events
//-----------------------------------------------------------------------------
CPUTEventHandledCode CPUT_OGL::CPUTHandleKeyboardEvent(CPUTKey key, CPUTKeyState state)
{
    CPUTEventHandledCode handleCode;
    
    // dispatch event to GUI to handle GUI triggers (if any) #### no GUI for now
    //CPUTEventHandledCode handleCode = CPUTGuiController::GetController()->HandleKeyboardEvent(key);

    // dispatch event to users HandleMouseEvent() method
    HEAPCHECK;
    handleCode = HandleKeyboardEvent(key, state);
    HEAPCHECK;

    return handleCode;
}

// Handle mouse events
//-----------------------------------------------------------------------------
CPUTEventHandledCode CPUT_OGL::CPUTHandleMouseEvent(int x, int y, int wheel, CPUTMouseState state, CPUTEventID eventID)
{
//    assert(false);
    CPUTEventHandledCode handleCode = CPUT_EVENT_UNHANDLED;
    
    // dispatch event to GUI to handle GUI triggers (if any)
    handleCode = CPUTGuiControllerOGL::GetController()->HandleMouseEvent(x,y,wheel,state, eventID);

    // dispatch event to users HandleMouseEvent() method if it wasn't consumed by the GUI
    if(CPUT_EVENT_HANDLED != handleCode)
    {
        HEAPCHECK;
        handleCode = HandleMouseEvent(x,y,wheel,state, eventID);
        HEAPCHECK;
    }

    return handleCode;
}


// Call appropriate OS create window call
//-----------------------------------------------------------------------------
CPUTResult CPUT_OGL::MakeWindow(const cString WindowTitle, CPUTWindowCreationParams windowParams)
{
    CPUTResult result;

    HEAPCHECK;

    // if we have a window, destroy it
    if(mpWindow)
    {
        delete mpWindow;
        mpWindow = NULL;
    }

    HEAPCHECK;

	// As OpenGL is multiplatform be sure to call proper function on proper OS to create window for now.
#ifdef CPUT_OS_WINDOWS
	mpWindow = new CPUTWindowWin();
#elif defined CPUT_OS_LINUX
    mpWindow = new CPUTWindowX();
#elif defined CPUT_OS_ANDROID
    mpWindow = new CPUTWindowAndroid();
#else
#error "Need OS Support"
#endif

    result = mpWindow->Create((CPUT*)this, WindowTitle, windowParams);

    HEAPCHECK;

    return result;
}


// Return the current GUI controller
//-----------------------------------------------------------------------------
CPUTGuiController* CPUT_OGL::CPUTGetGuiController()
{
    return CPUTGuiControllerOGL::GetController();
}

bool CPUT_OGL::supportExtension(const string name)
{
	vector<string>::iterator it;

	for(it=extensions.begin() ; it < extensions.end(); it++) {

		if(name == (*it))
		{
	        DEBUG_PRINT(_L("%s == %s"), name.c_str(),(*it).c_str());
			return true;
		}
    }
    return false;
}
/*
CPUTResult CPUT_OGL::CreateOGLContext(CPUTContextCreation ContextParams )
{




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
    CPUTRenderStateBlock *pBlock = new CPUTRenderStateBlockOGL();
//    pBlock->CreateNativeResources();
    CPUTRenderStateBlock::SetDefaultRenderStateBlock( pBlock );
    
    cString name = _L("$cbPerFrameValues");
    mpPerFrameConstantBuffer = new CPUTBufferOGL(name);
    GLuint id = mpPerFrameConstantBuffer->GetBufferID();
#ifndef CPUT_FOR_OGLES2
    GL_CHECK(glBindBuffer(GL_UNIFORM_BUFFER, mpPerFrameConstantBuffer->GetBufferID()));
    GL_CHECK(glBufferData(GL_UNIFORM_BUFFER, sizeof(CPUTFrameConstantBuffer), NULL, GL_DYNAMIC_DRAW)); // NULL to just init buffer size
    GL_CHECK(glBindBuffer(GL_UNIFORM_BUFFER, 0));
    DEBUG_PRINT(_L("bind per frame buffer buffer %d\n"), id);
    GL_CHECK(ES3_COMPAT(glBindBufferBase(GL_UNIFORM_BUFFER, id, id)));
    DEBUG_PRINT(_L("completed - bind buffer %d\n"), id);
    GL_CHECK(glBindBuffer(GL_UNIFORM_BUFFER, 0));
    
#else
#warning "Need to do something with uniform buffers here"
#endif
    CPUTAssetLibrary::GetAssetLibrary()->AddConstantBuffer(_L(""), name, _L(""), mpPerFrameConstantBuffer);
    
    return CPUT_SUCCESS;
}
*/

// This function tests a created DirectX context for specific features required for
// the framework, and possibly sample.  If your sample has specific hw features
// you wish to check for at creation time, you can add them here and have them
// tested at startup time.  If no contexts support your desired features, then
// the system will revert to the DX reference rasterizer, or barring that, 
// pop up a dialog and exit.
//-----------------------------------------------------------------------------
bool CPUT_OGL::TestContextForRequiredFeatures()
{

    return true;
}

// Default creation routine for making the back/stencil buffers
//-----------------------------------------------------------------------------
CPUTResult CPUT_OGL::CreateContext()
{
    int width,height;
    mpWindow->GetClientDimensions(&width, &height);

    // Set default viewport
    glViewport( 0, 0, width, height );
    return CPUT_SUCCESS;
}



// Toggle the fullscreen mode
// This routine keeps the current desktop resolution.  DougB suggested allowing
// one to go fullscreen in a different resolution
//-----------------------------------------------------------------------------
CPUTResult CPUT_OGL::CPUTToggleFullScreenMode()
{    
    // get the current fullscreen state
    bool bIsFullscreen = CPUTGetFullscreenState();
     
    // toggle the state
    bIsFullscreen = !bIsFullscreen;

    // set the fullscreen state
 //   HRESULT hr = mpSwapChain->SetFullscreenState(bIsFullscreen, NULL);
    ASSERT( SUCCEEDED(CPUT_ERROR), _L("Failed toggling full screen mode.") );

    // trigger resize event so that all buffers can resize
    int x,y,width,height;
    mpWindow->GetClientDimensions(&x, &y, &width, &height);
    ResizeWindow(width,height);

    // trigger a fullscreen mode change call if the sample has decided to handle the mode change
    FullscreenModeChange( bIsFullscreen );

    return CPUT_SUCCESS;
}

// Set the fullscreen mode to a desired state
//-----------------------------------------------------------------------------
void CPUT_OGL::CPUTSetFullscreenState(bool bIsFullscreen)
{
    // get the current fullscreen state
    bool bCurrentFullscreenState = CPUTGetFullscreenState();
    if((bool)bCurrentFullscreenState == bIsFullscreen)
    {
        // no need to call expensive state change, full screen state is already
        // in desired state
        return;
    }

    // set the fullscreen state
//    HRESULT hr = mpSwapChain->SetFullscreenState(bIsFullscreen, NULL);
//   ASSERT( SUCCEEDED(hr), _L("Failed toggling full screen mode.") );

    // trigger resize event so that all buffers can resize
    int x,y,width,height;
    mpWindow->GetClientDimensions(&x, &y, &width, &height);
    ResizeWindow(width,height);



    // trigger a fullscreen mode change call if the sample has decided to handle the mode change
    FullscreenModeChange( bIsFullscreen );
}

// Get a bool indicating whether the system is in full screen mode or not
//-----------------------------------------------------------------------------
bool CPUT_OGL::CPUTGetFullscreenState()
{
	/*
    // get the current fullscreen state
    BOOL bCurrentlyFullscreen;
    IDXGIOutput *pSwapTarget=NULL;
    mpSwapChain->GetFullscreenState(&bCurrentlyFullscreen, &pSwapTarget);
    SAFE_RELEASE(pSwapTarget);
    if(TRUE == bCurrentlyFullscreen )
    {
        return true;
    }
	 * */
    return false;
}


// incoming resize event to be handled and translated
//-----------------------------------------------------------------------------
void CPUT_OGL::ResizeWindow(UINT width, UINT height)
{
    DEBUG_PRINT( _L("ResizeWindow") );

	DEBUG_PRINT(_L("Width %d Height %d"),width, height);
	
	CPUTGuiControllerOGL::GetController()->Resize();
	
    // set the viewport
    glViewport( 0, 0, width, height );

}

// 'soft' resize - just stretch-blit
//-----------------------------------------------------------------------------
void CPUT_OGL::ResizeWindowSoft(UINT width, UINT height)
{
    UNREFERENCED_PARAMETER(width);
    UNREFERENCED_PARAMETER(height);
    // trigger the GUI manager to resize
 //   CPUTGuiControllerDX11::GetController()->Resize();

    InnerExecutionLoop();
}

//-----------------------------------------------------------------------------
void CPUT_OGL::UpdatePerFrameConstantBuffer( CPUTRenderParameters &renderParams, double totalSeconds )
{
    //NOTE: Issue with using the value of the resultant Uniform Block in shader
    if( mpPerFrameConstantBuffer )
    {
        float4x4 view, projection, inverseView, viewProjection;
        float4 eyePosition, lightDir;
        if( renderParams.mpCamera )
        {
            view = *renderParams.mpCamera->GetViewMatrix();
            projection = *renderParams.mpCamera->GetProjectionMatrix();
            inverseView = inverse(*renderParams.mpCamera->GetViewMatrix());
            eyePosition = float4(renderParams.mpCamera->GetPosition(), 0.0f);        
            viewProjection = view * projection;
        }
        if( renderParams.mpShadowCamera )
        {
            lightDir = float4(normalize(renderParams.mpShadowCamera->GetLook()), 0);
        }

        CPUTFrameConstantBuffer cb;

        cb.View           = view;
        cb.InverseView    = inverseView;
        cb.Projection     = projection;
        cb.ViewProjection = viewProjection;
        cb.AmbientColor   = float4(mAmbientColor, 0.0f);
        cb.LightColor     = float4(mLightColor, 0.0f);
        cb.LightDirection = lightDir;
        cb.EyePosition    = eyePosition;
        cb.TotalSeconds   = float4((float)totalSeconds);

#ifndef CPUT_FOR_OGLES2
		GLuint BufferID = mpPerFrameConstantBuffer->GetBufferID();
        GL_CHECK(glBindBuffer(GL_UNIFORM_BUFFER,BufferID ));
        GL_CHECK(glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(CPUTFrameConstantBuffer), &cb));
        GL_CHECK(glBindBuffer(GL_UNIFORM_BUFFER, 0));
#else
#warning "Need to do something with uniform buffers here"
#endif
        
    }
}

// Call the user's Render() callback (if it exists)
//-----------------------------------------------------------------------------
void CPUT_OGL::InnerExecutionLoop()
{
#ifdef CPUT_GPA_INSTRUMENTATION
    D3DPERF_BeginEvent(D3DCOLOR(0xff0000), L"CPUT User's Render() ");
#endif
    if (!mbShutdown)
    {


		 CPUTGPUTimerGL::OnFrameStart( );
		 CPUTGPUQueryGL::OnFrameStart( );
 
        double deltaSeconds = mpTimer->GetElapsedTime();
        Update(deltaSeconds);

        Render(deltaSeconds);


		CPUTGPUTimerGL::OnFrameEnd( );
		CPUTGPUQueryGL::OnFrameEnd( );

    }
    else
    {
#ifndef _DEBUG
        exit(0);
#endif
     //   Present(); // Need to present, or will leak all references held by previous Render()!
        ShutdownAndDestroy();
    }

#ifdef CPUT_GPA_INSTRUMENTATION
    D3DPERF_EndEvent();
#endif
}

// draw all the GUI controls
//-----------------------------------------------------------------------------
void CPUT_OGL::CPUTDrawGUI()
{
#ifdef CPUT_GPA_INSTRUMENTATION
    D3DPERF_BeginEvent(D3DCOLOR(0xff0000), L"CPUT Draw GUI");
#endif

    // draw all the Gui controls
    HEAPCHECK;
        CPUTGuiControllerOGL::GetController()->Draw();
    HEAPCHECK;

#ifdef CPUT_GPA_INSTRUMENTATION
        D3DPERF_EndEvent();
#endif
}

// Parse the command line for the parameters
// Only parameters that are specified on the command line are updated, if there
// are no parameters for a value, the previous WindowParams settings passed in
// are preserved
//-----------------------------------------------------------------------------
CPUTResult CPUT_OGL::CPUTParseCommandLine(cString commandLine, CPUTWindowCreationParams *pWindowParams, cString *pFilename)
{
    ASSERT( (NULL!=pWindowParams), _L("Required command line parsing parameter is NULL"));
    ASSERT( (NULL!=pFilename), _L("Required command line parsing parameter is NULL"));
   
    // there are no command line parameters, just return
    if(0==commandLine.size())
    {
        return CPUT_SUCCESS;
    }
 
    return CPUT_SUCCESS;
}




// Create a window context
//-----------------------------------------------------------------------------
CPUTResult CPUT_OGL::CPUTCreateWindowAndContext(const cString WindowTitle, CPUTWindowCreationParams windowParams)
{
    // Let's name it simpler in future:
    //
    // CPUTResult CPUT::CreateWindow(const cString WindowTitle, CPUTWindowCreationParams windowParams)
    //
    // This is proposition of new implementation. Let's make it simpler that it is currently.
    // Why so many function calls and separation of window and context creation? They are
    // tightly connected in terms of OpenGL on Windows and connected as well for other API's
    // and OS'es. Let's keep it together.
    CPUTResult result = CPUT_SUCCESS;

    HEAPCHECK;

    // We shouldn't destroy old window if it already exist, 
    // Framework user should do this by himself to be aware
    // of what he is doing.
    if( mpWindow )
    {
        return CPUT_ERROR_WINDOW_ALREADY_EXISTS;
    }

    // In future create unified Window class. For now on, be 
    // sure to create proper class on proper OS. It could be
    // just:
    //
    // mpWindow = new CPUTWindow();
    // assert( mpWindow );
    // mpWindow->Create( this, WindowTitle, windowParameters );
    //

   result = MakeWindow(WindowTitle, windowParams);
   if(CPUTFAILED(result))
   {
       return result;
   }


    HEAPCHECK;

    // create the DX context
    result = CreateOGLContext(windowParams.deviceParams);
    if(CPUTFAILED(result))
    {
        return result;
    }


    HEAPCHECK;

    result = CreateContext();

    CPUTRenderStateBlock *pBlock = new CPUTRenderStateBlockOGL();
    CPUTRenderStateBlock::SetDefaultRenderStateBlock( pBlock );
    
    cString name = _L("$cbPerFrameValues");
    mpPerFrameConstantBuffer = new CPUTBufferOGL(name);
    GLuint id = mpPerFrameConstantBuffer->GetBufferID();
#ifndef CPUT_FOR_OGLES2
    GL_CHECK(glBindBuffer(GL_UNIFORM_BUFFER, mpPerFrameConstantBuffer->GetBufferID()));
    GL_CHECK(glBufferData(GL_UNIFORM_BUFFER, sizeof(CPUTFrameConstantBuffer), NULL, GL_DYNAMIC_DRAW)); // NULL to just init buffer size
    DEBUG_PRINT(_L("bind per frame buffer buffer %d\n"), id);
//FIXME: constant buffer binding
    GL_CHECK(ES3_COMPAT(glBindBufferBase(GL_UNIFORM_BUFFER, id, id)));
    DEBUG_PRINT(_L("completed - bind buffer %d\n"), id);
    GL_CHECK(glBindBuffer(GL_UNIFORM_BUFFER, 0));
#else
#warning "Need to do something with uniform buffers here"
#endif
    CPUTAssetLibrary::GetAssetLibrary()->AddConstantBuffer(_L(""), name, _L(""), mpPerFrameConstantBuffer);
    
    name = _L("$cbPerModelValues");
    mpPerModelConstantBuffer = new CPUTBufferOGL(name, GL_UNIFORM_BUFFER, GL_DYNAMIC_DRAW, sizeof(CPUTModelConstantBuffer), NULL);
    
    id = mpPerModelConstantBuffer->GetBufferID();
#ifndef CPUT_FOR_OGLES2
    DEBUG_PRINT(_L("Bind per model values %d"), id);
    GL_CHECK(ES3_COMPAT(glBindBufferBase(GL_UNIFORM_BUFFER, id, id)));
    DEBUG_PRINT(_L("Completed bind per model values"));
    GL_CHECK(glBindBuffer(GL_UNIFORM_BUFFER, 0));
#else
#warning "Need to do something with uniform buffers here"
#endif
    CPUTAssetLibrary::GetAssetLibrary()->AddConstantBuffer(_L(""), name, _L(""), mpPerModelConstantBuffer);

	// Add our programatic (and global) material parameters
    CPUTMaterial::mGlobalProperties.AddValue( _L("cbPerFrameValues"), _L("$cbPerFrameValues") );
    CPUTMaterial::mGlobalProperties.AddValue( _L("cbPerModelValues"), _L("$cbPerModelValues") );
    CPUTMaterial::mGlobalProperties.AddValue( _L("cbGUIValues"), _L("$cbGUIValues") );

#ifdef ENABLE_GUI
    // initialize the gui controller 
    // Use the ResourceDirectory that was given during the Initialize() function
    // to locate the GUI+font resources
    CPUTGuiControllerOGL *pGUIController = CPUTGuiControllerOGL::GetController();
    cString ResourceDirectory = GetCPUTResourceDirectory();
    result = pGUIController->Initialize(ResourceDirectory);
    if(CPUTFAILED(result))
    {
        return result;
    }
    // register the callback object for GUI events as our sample
    pGUIController->SetCallback(this);
	pGUIController->SetWindow(mpWindow);
#endif
    HEAPCHECK;
#ifdef ENABLE_GUI
    //DrawLoadingFrame();
#endif
    HEAPCHECK;

    // Trigger a post-create user callback event
    Create();

    HEAPCHECK;
    //
    // Start the timer after everything is initialized and assets have been loaded
    //
    mpTimer->StartTimer();

    int x,y,width,height;
    mpWindow->GetClientDimensions(&x, &y, &width, &height);

    ResizeWindow(width,height);

	CPUTGPUTimerGL::OnDeviceAndContextCreated( );
	CPUTGPUQueryGL::OnDeviceAndContextCreated( );

    return result;
}


// Pop up a message box with specified title/text
//-----------------------------------------------------------------------------
void CPUT_OGL::DrawLoadingFrame()
{
    DEBUG_PRINT(_L("DrawLoadingFrame()"));
    // fill first frame with clear values so render order later is ok
    const float srgbClearColor[] = { 0.0993f, 0.0993f, 0.0993f, 1.0f };
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);


    CPUTGuiControllerOGL *pGUIController = CPUTGuiControllerOGL::GetController();
    CPUTText *pText = NULL;
    pGUIController->CreateText(_L("Just a moment, now loading..."), 999, 0, &pText);

    pGUIController->EnableAutoLayout(false);
    int textWidth, textHeight;
    pText->GetDimensions(textWidth, textHeight);
    int width,height;
    mpWindow->GetClientDimensions(&width, &height);
    pText->SetPosition(width/2-textWidth/2, height/2);

    pGUIController->Draw();
    
    // present loading screen
    Present();

    pGUIController->EnableAutoLayout(true);
    pGUIController->DeleteAllControls();
}

// Pop up a message box with specified title/text
//-----------------------------------------------------------------------------
void CPUT_OGL::CPUTMessageBox(const cString DialogBoxTitle, const cString DialogMessage)
{
    CPUTOSServices::OpenMessageBox(DialogBoxTitle.c_str(), DialogMessage.c_str());
}

// start main message loop
//-----------------------------------------------------------------------------
int CPUT_OGL::CPUTMessageLoop()
{
    return mpWindow->StartMessageLoop();
}

// Window is closing. Shut the system to shut down now, not later.
//-----------------------------------------------------------------------------
void CPUT_OGL::DeviceShutdown()
{
	/*
    if(mpSwapChain)
    {
        // DX requires setting fullscreenstate to false before exit.
        mpSwapChain->SetFullscreenState(false, NULL);
    }
     */
    if (mbShutdown == false) {
        mbShutdown = true;
        ShutdownAndDestroy();
    }
}

// Shutdown the CPUT system
// Destroy all 'global' resource handling objects, all asset handlers,
// the DX context, and everything EXCEPT the window
//-----------------------------------------------------------------------------
void CPUT_OGL::Shutdown()
{
    // release the lock on the mouse (if there was one)
//    CPUTOSServices::GetOSServices()->ReleaseMouse();
    mbShutdown = true;
}

// Frees all resources and removes all assets from asset library
//-----------------------------------------------------------------------------
void CPUT_OGL::RestartCPUT()
{
    //
    // Clear out all CPUT resources
    //
//    CPUTInputLayoutCacheDX11::GetInputLayoutCache()->ClearLayoutCache();
    CPUTAssetLibrary::GetAssetLibrary()->ReleaseAllLibraryLists();
//	CPUTGuiControllerDX11::GetController()->DeleteAllControls();
//	CPUTGuiControllerDX11::GetController()->ReleaseResources();

    //
    // Clear out all DX resources and contexts
    //
    DestroyOGLContext();

    //
    // Signal the window to close
    //
  //  mpWindow->Destroy();

    //
    // Clear out the timer
    //
    mpTimer->StopTimer();
    mpTimer->ResetTimer();
    
    HEAPCHECK;
}
// Actually destroy all 'global' resource handling objects, all asset handlers,
// the DX context, and everything EXCEPT the window
//-----------------------------------------------------------------------------
void CPUT_OGL::ShutdownAndDestroy()
{

	 CPUTGPUTimerGL::OnDeviceAboutToBeDestroyed( );
	 CPUTGPUQueryGL::OnDeviceAboutToBeDestroyed( );
    // make sure no more rendering can happen
    mbShutdown = true;

    // call the user's OnShutdown code
    Shutdown();
	
    CPUTAssetLibrary::DeleteAssetLibrary();
    CPUTGuiControllerOGL::DeleteController();
	CPUTRenderStateBlock::SetDefaultRenderStateBlock( NULL );
	
    SAFE_RELEASE(mpPerFrameConstantBuffer);
    SAFE_RELEASE(mpPerModelConstantBuffer);

    DestroyOGLContext();

    // Tell the window layer to post a close-window message to OS
    // and stop the message pump
    mpWindow->Destroy();
    delete mpWindow;
    mpWindow = NULL;

    HEAPCHECK;
}

//-----------------------------------------------------------------------------
void CPUTSetDebugName( void *pResource, std::wstring name )
{
#ifdef _DEBUG
    //char pCharString[CPUT_MAX_STRING_LENGTH];
    const wchar_t *pWideString = name.c_str();
    //UINT ii;
	/*
    UINT length = min( (UINT)name.length(), (CPUT_MAX_STRING_LENGTH-1));
    for(ii=0; ii<length; ii++)
    {
        pCharString[ii] = (char)pWideString[ii];
    }
    pCharString[ii] = 0; // Force NULL termination
    ((ID3D11DeviceChild*)pResource)->SetPrivateData( WKPDID_D3DDebugObjectName, (UINT)name.length(), pCharString );
	 * */
#endif // _DEBUG
}

#ifdef CPUT_OS_ANDROID

#include "CPUTGestureDetectorAndroid.h"    //Tap/Doubletap/Pinch detector

static CPUTKey ConvertToCPUTKey(int aKey)
{
    if ((aKey >= AKEYCODE_0) && (aKey <= AKEYCODE_9))
        return (CPUTKey)(KEY_0 + aKey - AKEYCODE_0);

    if ((aKey >= AKEYCODE_A) && (aKey <= AKEYCODE_Z))
        return (CPUTKey)(KEY_A + aKey - AKEYCODE_A);

    switch (aKey)
    {
    case AKEYCODE_HOME:
        return KEY_HOME;
    case AKEYCODE_STAR:
        return KEY_STAR;
    case AKEYCODE_POUND:
        return KEY_HASH;
    case AKEYCODE_COMMA:
        return KEY_COMMA;
    case AKEYCODE_PERIOD:
        return KEY_PERIOD;
    case AKEYCODE_ALT_LEFT:
        return KEY_LEFT_ALT;
    case AKEYCODE_ALT_RIGHT:
        return KEY_RIGHT_ALT;
    case AKEYCODE_SHIFT_LEFT:
        return KEY_LEFT_SHIFT;
    case AKEYCODE_SHIFT_RIGHT:
        return KEY_RIGHT_SHIFT;
    case AKEYCODE_TAB:
        return KEY_TAB;
    case AKEYCODE_SPACE:
        return KEY_SPACE;
    case AKEYCODE_ENTER:
        return KEY_ENTER;
    case AKEYCODE_DEL:
        return KEY_DELETE;
    case AKEYCODE_MINUS:
        return KEY_MINUS;
    case AKEYCODE_LEFT_BRACKET:
        return KEY_OPENBRACKET;
    case AKEYCODE_RIGHT_BRACKET:
        return KEY_CLOSEBRACKET;
    case AKEYCODE_BACKSLASH:
        return KEY_BACKSLASH;
    case AKEYCODE_SEMICOLON:
        return KEY_SEMICOLON;
    case AKEYCODE_APOSTROPHE:
        return KEY_SINGLEQUOTE;
    case AKEYCODE_SLASH:
        return KEY_SLASH;
    case AKEYCODE_AT:
        return KEY_AT;
    case AKEYCODE_PLUS:
        return KEY_PLUS;
    case AKEYCODE_PAGE_UP:
        return KEY_PAGEUP;
    case AKEYCODE_PAGE_DOWN:
        return KEY_PAGEDOWN;
    default:
    case AKEYCODE_SOFT_LEFT:
    case AKEYCODE_SOFT_RIGHT:
    case AKEYCODE_BACK:
    case AKEYCODE_CALL:
    case AKEYCODE_ENDCALL:
    case AKEYCODE_DPAD_UP:
    case AKEYCODE_DPAD_DOWN:
    case AKEYCODE_DPAD_LEFT:
    case AKEYCODE_DPAD_RIGHT:
    case AKEYCODE_DPAD_CENTER:
    case AKEYCODE_VOLUME_UP:
    case AKEYCODE_VOLUME_DOWN:
    case AKEYCODE_POWER:
    case AKEYCODE_CAMERA:
    case AKEYCODE_CLEAR:
    case AKEYCODE_SYM:
    case AKEYCODE_EXPLORER:
    case AKEYCODE_ENVELOPE:
    case AKEYCODE_GRAVE:
    case AKEYCODE_EQUALS:
    case AKEYCODE_NUM:
    case AKEYCODE_HEADSETHOOK:
    case AKEYCODE_FOCUS:
    case AKEYCODE_MENU:
    case AKEYCODE_NOTIFICATION:
    case AKEYCODE_SEARCH:
    case AKEYCODE_MEDIA_PLAY_PAUSE:
    case AKEYCODE_MEDIA_STOP:
    case AKEYCODE_MEDIA_NEXT:
    case AKEYCODE_MEDIA_PREVIOUS:
    case AKEYCODE_MEDIA_REWIND:
    case AKEYCODE_MEDIA_FAST_FORWARD:
    case AKEYCODE_MUTE:
    case AKEYCODE_PICTSYMBOLS:
    case AKEYCODE_SWITCH_CHARSET:
    case AKEYCODE_BUTTON_L1:
    case AKEYCODE_BUTTON_R1:
    case AKEYCODE_BUTTON_L2:
    case AKEYCODE_BUTTON_R2:
    case AKEYCODE_BUTTON_THUMBL:
    case AKEYCODE_BUTTON_THUMBR:
    case AKEYCODE_BUTTON_START:
    case AKEYCODE_BUTTON_SELECT:
    case AKEYCODE_BUTTON_MODE:
    case AKEYCODE_UNKNOWN:
        return KEY_NONE;
    }

}

static CPUTKeyState ConvertToCPUTKeyState(int aAction)
{
    switch (aAction)
    {
    case AKEY_EVENT_ACTION_UP:
        return CPUT_KEY_UP;
    case AKEY_EVENT_ACTION_DOWN:
    default:
        return CPUT_KEY_DOWN;
    }
}

int32_t CPUT_OGL::cput_handle_input(struct android_app* app, AInputEvent* event)
{    
    int n;
    CPUT_OGL *pSample = (CPUT_OGL*)app->userData;
    int lEventType = AInputEvent_getType(event);
    static float drag_center_x = 0.0f, drag_center_y = 0.0f;
    static float dist_squared = 0.0f;
    static bool isPanning = false;

    static ndk_helper::TapDetector         sTapDetector;
    static ndk_helper::DoubletapDetector   sDoubletapDetector;
    static ndk_helper::PinchDetector       sPinchDetector;
    static ndk_helper::DragDetector        sDragDetector;
    static float2 sLastMouse;
    switch (lEventType) 
    {
    case AINPUT_EVENT_TYPE_MOTION:
        {
#if 1
            ndk_helper::GESTURE_STATE tapState          = sTapDetector.Detect(event);
            ndk_helper::GESTURE_STATE doubleTapState    = sDoubletapDetector.Detect(event);
            ndk_helper::GESTURE_STATE dragState         = sDragDetector.Detect(event);
            ndk_helper::GESTURE_STATE pinchState        = sPinchDetector.Detect(event);

        //Disable Taps/double taps - GUI doesn't support (yet)        
            if( tapState == ndk_helper::GESTURE_STATE_ACTION )
            {
                // LOGI("GESTURE_STATE_ACTION - tap");
                // float2 v;
                // float x, y;
                // sTapDetector.GetPointer( v );
                // x = v.x;
                // y = v.y;
                // LOGI("     TOUCH POINT: %f, %f", x, y);
                // pSample->CPUTHandleMouseEvent(x, y, 0.0f, CPUT_MOUSE_LEFT_DOWN);
                // //pSample->CPUTHandleMouseEvent(x, y, 0.0f, CPUT_MOUSE_LEFT_UP);
            }
            else
            if( doubleTapState == ndk_helper::GESTURE_STATE_ACTION )
            {
                LOGI("DOUBLE TAP RECEIVED");
            }
            //else
            {
                //Handle drag state
                if( dragState & ndk_helper::GESTURE_STATE_START )
                {
                    if (isPanning == false) {
                        LOGI("GESTURE_STATE_START - drag");
                        sDragDetector.GetPointer( sLastMouse );
                        pSample->CPUTHandleMouseEvent(sLastMouse.x, sLastMouse.y, 0.0f, CPUT_MOUSE_LEFT_DOWN, CPUT_EVENT_DOWN);
                    }
                }
                else if( dragState & ndk_helper::GESTURE_STATE_MOVE )
                {
                    if (isPanning == false) {
                        LOGI("GESTURE_STATE_MOVE - drag");
                        sDragDetector.GetPointer( sLastMouse );
                        pSample->CPUTHandleMouseEvent(sLastMouse.x, sLastMouse.y, 0.0f, CPUT_MOUSE_LEFT_DOWN, CPUT_EVENT_MOVE);
                    }
                }
                else if( dragState & ndk_helper::GESTURE_STATE_END )
                {
                    float2 v = float2(-1.0f, -1.0f);
                    pSample->CPUTHandleMouseEvent(sLastMouse.x, sLastMouse.y, 0.0f, CPUT_MOUSE_NONE, CPUT_EVENT_UP);
                    pSample->HandleKeyboardEvent(KEY_A, CPUT_KEY_UP);
                    pSample->HandleKeyboardEvent(KEY_D, CPUT_KEY_UP);
                    pSample->HandleKeyboardEvent(KEY_E, CPUT_KEY_UP);
                    pSample->HandleKeyboardEvent(KEY_W, CPUT_KEY_UP);
                    pSample->HandleKeyboardEvent(KEY_S, CPUT_KEY_UP);
                    pSample->HandleKeyboardEvent(KEY_Q, CPUT_KEY_UP);
                    isPanning = false;
                    sLastMouse.x = sLastMouse.y = -1.0f;
                    LOGI("GESTURE_STATE_END - drag");
                }
                
                //Handle pinch state
                if( pinchState & ndk_helper::GESTURE_STATE_START )
                {
                    if (isPanning == false) {
                        LOGI("GESTURE_STATE_START - pinch");
                        //Start new pinch
                        float2 v1;
                        float2 v2;
                        float x1, y1, x2, y2;
                        sPinchDetector.GetPointers( v1, v2 );
                        x1 = v1.x;
                        y1 = v1.y;
                        x2 = v2.x;
                        y2 = v2.y;
                        drag_center_x = (x1 + x2) / 2.0f;
                        drag_center_y = (y1 + y2) / 2.0f;
                        dist_squared = ((x2 - x1) * (x2 - x1)) + ((y2 - y1) * (y2 - y1));
                    }
                }
                else if( pinchState & ndk_helper::GESTURE_STATE_MOVE )
                {
                    isPanning = true;
                
                    CPUTKey key = (CPUTKey)0;
                    CPUTKeyState state = (CPUTKeyState)0;
                
                    LOGI("GESTURE_STATE_MOVE - pinch");
                
                    float2 v1;
                    float2 v2;
                    float x1, y1, x2, y2;
                    float new_center_x, new_center_y;
                    float new_dist_squared;
                    float delta_x, delta_y;
                    sPinchDetector.GetPointers( v1, v2 );
                    x1 = v1.x;
                    y1 = v1.y;
                    x2 = v2.x;
                    y2 = v2.y;
                
                    new_center_x = (x1 + x2) / 2.0f;
                    new_center_y = (y1 + y2) / 2.0f;
                
                    new_dist_squared = ((x2 - x1) * (x2 - x1)) + ((y2 - y1) * (y2 - y1));
                
                    delta_x = drag_center_x - new_center_x;
                    delta_y = drag_center_y - new_center_y;
                
                    //
                    // For each direction of movement, the opposite direction is cancelled (KEY_UP)
                    //
                
                    // Handle pinch and zoom actions
                    if (abs(new_dist_squared - dist_squared) > 1000.0f) {
                        if (new_dist_squared < dist_squared) {
                            pSample->HandleKeyboardEvent(KEY_S, CPUT_KEY_UP);
                            pSample->HandleKeyboardEvent(KEY_W, CPUT_KEY_DOWN);
                        } else {
                            pSample->HandleKeyboardEvent(KEY_W, CPUT_KEY_UP);
                            pSample->HandleKeyboardEvent(KEY_S, CPUT_KEY_DOWN);
                        } 
                    } else {
                        pSample->HandleKeyboardEvent(KEY_W, CPUT_KEY_UP);
                        pSample->HandleKeyboardEvent(KEY_S, CPUT_KEY_UP);
                    }
                
                    // handle left and right drag
                    if (delta_x >= 2.0f) {
                        pSample->HandleKeyboardEvent(KEY_A, CPUT_KEY_DOWN);
                        pSample->HandleKeyboardEvent(KEY_D, CPUT_KEY_UP);
                    } else if (delta_x <= -2.0f) {
                        pSample->HandleKeyboardEvent(KEY_D, CPUT_KEY_DOWN);
                        pSample->HandleKeyboardEvent(KEY_A, CPUT_KEY_UP);
                    } else if (delta_x < 2.0 && delta_x > -2.0) {
                        pSample->HandleKeyboardEvent(KEY_A, CPUT_KEY_UP);
                        pSample->HandleKeyboardEvent(KEY_D, CPUT_KEY_UP);
                    }
                
                    // handle up and down drag
                    if (delta_y >= 2.0f) {
                        pSample->HandleKeyboardEvent(KEY_Q, CPUT_KEY_UP);
                        pSample->HandleKeyboardEvent(KEY_E, CPUT_KEY_DOWN);
                    } else if (delta_y <= -2.0f) {
                        pSample->HandleKeyboardEvent(KEY_E, CPUT_KEY_UP);
                        pSample->HandleKeyboardEvent(KEY_Q, CPUT_KEY_DOWN);
                    } else if (delta_y < 2.0 && delta_y > -2.0) {
                        pSample->HandleKeyboardEvent(KEY_E, CPUT_KEY_UP);
                        pSample->HandleKeyboardEvent(KEY_Q, CPUT_KEY_UP);
                    }
                
                    // current values become old values for next frame
                    dist_squared = new_dist_squared;
                    drag_center_x = new_center_x;
                    drag_center_y = new_center_y;
                }
            }
#endif
        } break;
    case AINPUT_EVENT_TYPE_KEY:
        {
            int aKey = AKeyEvent_getKeyCode(event);
            CPUTKey cputKey = ConvertToCPUTKey(aKey);
            int aAction = AKeyEvent_getAction(event);
            CPUTKeyState cputKeyState = ConvertToCPUTKeyState(aAction);
            pSample->CPUTHandleKeyboardEvent(cputKey, cputKeyState);
            return 1;
        }
    default:
        return 0;
    }

    return 0;
}

#endif // #ifdef CPUT_OS_ANDROID
