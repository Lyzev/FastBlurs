//--------------------------------------------------------------------------------------
// Copyright 2014 Intel Corporation
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

#ifndef _UTILITY_H
#define _UTILITY_H

#include <stdio.h>
#include <time.h>
#include "CPUTScene.h"

// TODO: MOVE THESE TO CPUT math
inline float4x4 LookAtLH( float3 eyePos, float3 targetPos, float3 upVector )
{
    upVector.normalize();

    float3 look  = (targetPos - eyePos).normalize();
    float3 right = cross3( upVector, look ).normalize();
    float3 up    = cross3(look, right);

    return float4x4(
        right.x,    right.y,    right.z, 0.0f,
        up.x,       up.y,       up.z, 0.0f,
        look.x,     look.y,     look.z, 0.0f,
        eyePos.x,   eyePos.y,   eyePos.z, 1.0f
        );
}

// TODO: MOVE THESE TO CPUT math
inline float4x4 LookAtRH( float3 eyePos, float3 targetPos, float3 upVector )
{
    upVector.normalize();

    float3 look  = (eyePos - targetPos).normalize();
    float3 right = cross3( upVector, look ).normalize();
    float3 up    = cross3(look, right);

    return float4x4(
        right.x,    right.y,    right.z, 0.0f,
        up.x,       up.y,       up.z, 0.0f,
        look.x,     look.y,     look.z, 0.0f,
        eyePos.x,   eyePos.y,   eyePos.z, 1.0f
        );
}

inline std::string cStringFormatA( const char * fmt, ... )
{
    int nSize = 0;
    char buff[4096];
    va_list args;
    va_start(args, fmt);
    nSize = vsnprintf( buff, sizeof(buff) - 1, fmt, args); // C4996
    return std::string( buff );    
}

#if defined (UNICODE) || defined(_UNICODE)
inline cString cStringFormat( const wchar_t * fmt, ... )
{
    int nSize = 0;
    wchar_t buff[4096];
    va_list args;
    va_start( args, fmt );
    nSize = _vsnwprintf( buff, sizeof(buff) - 1, fmt, args);
    return cString( buff );
}
#else
inline cString cStringFormat( const char * fmt, ... )
{
    int nSize = 0;
    char buff[4096];
    va_list args;
    va_start(args, fmt);
    nSize = vsnprintf( buff, sizeof(buff) - 1, fmt, args); // C4996
    return cString( buff );    
}
#endif

#endif // _UTILITY_H