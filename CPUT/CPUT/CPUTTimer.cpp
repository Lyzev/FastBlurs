//--------------------------------------------------------------------------------------
// Copyright 2013 Intel Corporation
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
#include "CPUTTimer.h"
#ifndef CPUT_OS_ANDROID
	#include "CPUTTimerWin.h"
#else
	#include "CPUTTimerLinux.h"
#endif




//-----------------------------------------------------------------------------
CPUTTimer *CPUTTimer::CreateTimer(  )
{
    // TODO: accept DX11/OGL param to control which platform we generate.
    // TODO: be sure to support the case where we want to support only one of them
#ifndef CPUT_OS_ANDROID
    return (CPUTTimer*) new CPUTTimerWin();
#else
	   return (CPUTTimer*)  new CPUTTimerLinux();
#endif

assert(0);
return NULL;
}