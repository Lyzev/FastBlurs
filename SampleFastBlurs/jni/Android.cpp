/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

//BEGIN_INCLUDE(all)
#include <jni.h>
#include <errno.h>

#include <android/sensor.h>
#include <android/log.h>
#include <android_native_app_glue.h>

#include "CPUTScene.h"
#include "CPUTRenderStateBlockOGL.h"
#include "CPUT_OGL.h"
#include "CPUTGuiControllerOGL.h"
#include "SampleFastBlurs.h"

#define GUI_LOCATION     "gui/"
#define SYSTEM_LOCATION  "system/"
#define TEXTURE_LOCATION "texture/"

static void cput_handle_cmd(struct android_app* app, int32_t cmd)
{
    MySample *pSample = (MySample *)app->userData;

    switch (cmd)
    {
    case APP_CMD_SAVE_STATE:
        break;
    case APP_CMD_INIT_WINDOW:
        if (!pSample->HasWindow())
        {
            //LOGI("Creating window");
            CPUTResult result;

            // window and device parameters
            CPUTWindowCreationParams params;
            params.samples = 1;

            // create the window and device context
            result = pSample->CPUTCreateWindowAndContext(_L("CPUTWindow OpenGLES"), params);
            if (result != CPUT_SUCCESS)
                LOGI("Unable to create window");
        }
        else
        {
            //LOGI("Window already created");
        }
        break;
    case APP_CMD_TERM_WINDOW:
        // Need clear window create and destroy calls
        // The window is being hidden or closed, clean it up.
        if (pSample->HasWindow())
        {
            pSample->DeviceShutdown();
        }
        break;
    case APP_CMD_GAINED_FOCUS:
        break;
    case APP_CMD_LOST_FOCUS:
        break;
	case APP_CMD_WINDOW_RESIZED:
        break;
    }
}

/**
 * This is the main entry point of a native application that is using
 * android_native_app_glue.  It runs in its own thread, with its own
 * event loop for receiving input events and doing other things.
 */
void android_main(struct android_app* state)
{
    // Make sure glue isn't stripped.
    app_dummy();

    // create an instance of my sample
    MySample *pSample = new MySample();
    if (!pSample)
    {
        LOGI("Failed to allocate MySample");
        return;
    }

     // Assign the sample back into the app state
    state->userData = pSample;
    state->onAppCmd = cput_handle_cmd;
    state->onInputEvent = CPUT_OGL::cput_handle_input;

    // We make the assumption we are running from the executable's dir in
    // the CPUT SampleStart directory or it won't be able to use the relative paths to find the default
    // resources    
    cString ResourceDirectory;

    CPUTFileSystem::GetExecutableDirectory(&ResourceDirectory);

    // Different executable and assets locations on different OS'es.
    // Consistency should be maintained in all OS'es and API's.
    ResourceDirectory.append(GUI_LOCATION);

	// Initialize the system and give it the base CPUT resource directory (location of GUI images/etc)
    // For now, we assume it's a relative directory from the executable directory.  Might make that resource
    // directory location an env variable/hardcoded later
    CPUTWindowAndroid::SetAppState(state);
    pSample->CPUTInitialize(ResourceDirectory);

    CPUTFileSystem::GetExecutableDirectory(&ResourceDirectory);
    CPUTAssetLibrary *pAssetLibrary = CPUTAssetLibrary::GetAssetLibrary();

    // Different executable and assets locations on different OS'es.
    // Consistency should be maintained in all OS'es and API's.
    pAssetLibrary->SetMediaDirectoryName(  ResourceDirectory );

    // Different executable and assets locations on different OS'es.
    // Consistency should be maintained in all OS'es and API's.
    CPUTFileSystem::GetExecutableDirectory(&ResourceDirectory);
    ResourceDirectory.append(SYSTEM_LOCATION);
    pAssetLibrary->SetSystemDirectoryName(ResourceDirectory);

    // start the main message loop
    pSample->CPUTMessageLoop();

    // cleanup resources
    SAFE_DELETE(pSample);
    pSample = NULL;

    state->userData = NULL;
}
//END_INCLUDE(all)
