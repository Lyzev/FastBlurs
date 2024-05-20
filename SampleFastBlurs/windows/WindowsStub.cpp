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

#include "..\jni\SampleFastBlurs.h"
#include "CPUTRenderStateBlockOGL.h"
#include "CPUTGuiControllerOGL.h"

#ifdef CPUT_OS_LINUX
const cString GUI_DIR = _L("../media/gui/");
const cString SYSTEM_DIR = _L("../media/system/");
#else
const cString GUI_DIR = _L("../media/gui/");
const cString SYSTEM_DIR = _L("../media/system/");
#endif

const cString WINDOW_TITLE = _L("CPUTWindow Fast Blurs");

int main( int argc, char **argv );
#ifdef CPUT_OS_WINDOWS
#include <stdlib.h>
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
    // Prevent unused parameter compiler warnings
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(nCmdShow);

    return main(__argc, __argv);
}
#endif
// Application entry point.  Execution begins here.
//-----------------------------------------------------------------------------
int main( int argc, char **argv )
{
#ifdef DEBUG
    // tell VS to report leaks at any exit of the program
    _CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
    //http://msdn.microsoft.com/en-us/library/x98tx3cf%28v=vs.100%29.aspx
	//Add a watch for “{,,msvcr100d.dll}_crtBreakAlloc” to the watch window
	//Set the value of the watch to the memory allocation number reported by your sample at exit.
	//Note that the “msvcr100d.dll” is for MSVC2010.  Other versions of MSVC use different versions of this dl; you’ll need to specify the appropriate version.

#endif
    CPUTResult result=CPUT_SUCCESS;
    int returnCode=0;

    // create an instance of my sample
    MySample *pSample = new MySample();
    
    // We make the assumption we are running from the executable's dir in
    // the CPUT SampleStart directory or it won't be able to use the relative paths to find the default
    // resources    
    cString ResourceDirectory;

    CPUTFileSystem::GetExecutableDirectory(&ResourceDirectory);

    // Different executable and assets locations on different OS'es.
    // Consistency should be maintained in all OS'es and API's.
    ResourceDirectory.append(GUI_DIR);

	// Initialize the system and give it the base CPUT resource directory (location of GUI images/etc)
    // For now, we assume it's a relative directory from the executable directory.  Might make that resource
    // directory location an env variable/hardcoded later
    pSample->CPUTInitialize(ResourceDirectory);

    CPUTFileSystem::GetExecutableDirectory(&ResourceDirectory);

    // Different executable and assets locations on different OS'es.
    // Consistency should be maintained in all OS'es and API's.
    ResourceDirectory.append(SYSTEM_DIR);
    CPUTAssetLibrary *pAssetLibrary = CPUTAssetLibrary::GetAssetLibrary();
    pAssetLibrary->SetSystemDirectoryName(ResourceDirectory);


    // window and device parameters
    CPUTWindowCreationParams params;
    params.samples = 1;
    
    // parse out the parameter settings or reset them to defaults if not specified
    std::string AssetFilename;

    result = pSample->CPUTCreateWindowAndContext(WINDOW_TITLE, params);
    ASSERT( CPUTSUCCESS(result), _L("CPUT Error creating window and context.") );

    DEBUG_PRINT( _L("Start CPUTMessageLoop") );

    returnCode = pSample->CPUTMessageLoop();

    pSample->DeviceShutdown();

    // cleanup resources
    SAFE_DELETE(pSample);

    return returnCode;
}


