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
//
// Created by: filip.strugar@intel.com
//--------------------------------------------------------------------------------------
//
// This project is using GLSW OpenGL Shader Wrangler from http://prideout.net/blog/?p=11
//
//////////////////////////////////////////////////////////////////////////

#include "GaussianBlur.h"
#include "CPUTScene.h"
#include "CPUTRenderStateBlockOGL.h"
#include "CPUT_OGL.h"
#include "CPUTGuiControllerOGL.h"

#include "CPUTTextureOGL.h"
#include "CPUTRenderTargetOGL.h"

#include "SampleFastBlurs.h"

#include "SFBRenderTargetOGL.h"

#include "middleware/glsw/glsw.h"

#include "Utility.h"

#include <time.h>

// UI controls
const CPUTControlID ID_MAIN_PANEL = 10;
const CPUTControlID ID_FPS_PANEL = 20;

const CPUTControlID ID_BUTTON_MODE_BASE             = 100;
const CPUTControlID ID_BUTTON_MODE_BASE_10_RESERVED = 109;

const CPUTControlID ID_BUTTON_MODE_SHOW_GRAPH       = 110;
const CPUTControlID ID_BUTTON_MODE_RUN_BENCHMARK    = 111;

const CPUTControlID ID_DROPDOWN_SCENE               = 120;
const CPUTControlID ID_DROPDOWN_BLUR_RESOLUTION     = 121;
const CPUTControlID ID_DROPDOWN_KERNEL_SIZE_PRESET  = 122;

const CPUTControlID ID_FPS_INFO_TEXT                = 1000;
const CPUTControlID ID_OTHER_INFO_TEXT              = 1001;
const CPUTControlID ID_BENCHMARK_TEST_INFO          = 1002;
const CPUTControlID ID_TECHNIQUE_INFO_TEXT          = 1003;
const CPUTControlID ID_IGNORE_CONTROL_ID            = -1;

// 16 and 32 do well on BYT, anything in between or below is bad, values above were not thoroughly tested; 32 seems to do well on laptop/desktop Windows Intel and on NVidia/AMD as well
// (further hardware-specific tuning probably needed for optimal performance)
static const int CS_THREAD_GROUP_SIZE = 32;


const static cString c_EffectNames[] = { _L("Off"), _L("Downsample Only"), _L("Gaussian Blur"), _L("Kawase Blur"), _L("Moving Average CS Blur") };

const static cString c_TexNames[] = { _L("testimg_1280x720.png"), _L("testimg_1600x900.png"), _L("testimg_1920x1080.png"), _L("testimg_2560x1600.png") };

#ifndef GL_FRAMEBUFFER_SRGB
#define GL_FRAMEBUFFER_SRGB               0x8DB9
#endif

// sRGB behavior not consistent between platforms (various desktop and android ones) so not using it for now, pending further investigation. Does not affect performance.
bool c_sRGB = false;

const int               c_InnerLoopsDuringBenchmark         = 8;


// OpenGL / GLES related globals.
static GLuint           g_ProgramHandleFullscreenQuad       = 0;
static GLuint           g_ProgramHandleAnimatedTriangles    = 0;
static GLuint           g_ProgramHandleDownsample2x2        = 0;
static GLuint           g_ProgramHandleDownsample4x4        = 0;
static GLuint           g_ProgramHandleKawaseBlurPass       = 0;
static GLuint           g_ProgramHandleGaussianH            = 0;
static GLuint           g_ProgramHandleGaussianV            = 0;
static GLuint           g_ProgramHandleComputeShaderBlurH   = 0;
static GLuint           g_ProgramHandleComputeShaderBlurV   = 0;
static GLuint           g_ProgramHandleApply                = 0;
static GLuint           g_ProgramHandleDebugGraphDrawInput  = 0;
static GLuint           g_ProgramHandleDebugGraphDraw       = 0;
static ShaderUniforms   g_Uniforms;
static GLsizei          g_FSIndexCount                      = 0;
static const GLuint     g_FSPositionSlot                    = 0;
static GLuint           g_FSVAO                             = 0;

static const int        g_ATCount                           = 32;
static GLsizei          g_ATIndexCount                      = 0;
static const GLuint     g_ATPositionSlot                    = 0;
static const GLuint     g_ATColorSlot                       = 1;
static GLuint           g_ATVAO                             = 0;
static GLuint           g_ATVAOVertices                     = 0;
static ATVertex         g_ATVerts[ g_ATCount * 3 ];
//
static GLuint           g_SamplerLinear                     = 0;
static GLuint           g_SamplerPoint                      = 0;
//
static GLfloat          g_FinalScreenSize[2];
static GLfloat          g_RTSizeSubRTSize[4];
//
static std::string      g_LastGaussKernelShader;


#ifdef CPUT_OS_ANDROID

class SFBTimerAndroid : CPUTTimer
{
public:
    SFBTimerAndroid()
    {
        ResetTimer();
    }

    virtual void   StartTimer()
    {
        if( !m_counting )
        {
            ResetTimer();
            m_counting = true;

            m_startTime = GetCurrentTime();
        }
    }

    virtual double StopTimer()
    {
        if( m_counting )
        {
            m_counting = false;

            m_lastMeasured = GetCurrentTime();
        }

        return m_lastMeasured - m_startTime;
    }

    virtual double GetTotalTime()
    {
        if( m_counting ) 
        {
            double currentTime = GetCurrentTime();
            return currentTime - m_startTime;
        }

        return m_lastMeasured - m_startTime;
    }

    virtual double GetElapsedTime()
    {
        double prevTime = m_lastMeasured;
        if( m_counting )
        {
            m_lastMeasured = GetCurrentTime();
        }
        return m_lastMeasured - prevTime;
    }

    virtual void   ResetTimer()
    {
        m_counting        = false;
        m_startTime       = 0;
        m_lastMeasured    = 0;
    }

private:
    double GetCurrentTime( )
    {
        struct timeval Time;
        gettimeofday( &Time, NULL );

        return Time.tv_sec + Time.tv_usec * 1.0 / 1000000.0;
    }

private:
    bool        m_counting;

    double      m_startTime;
    double      m_lastMeasured;

};

#endif

static void CreateFullscreenPassGeometry()
{
    DEBUG_PRINT( _L("CreateFullscreenPassGeometry") );

    const int Faces[] = {
        0, 1, 2,
        2, 1, 3 
    };

    const float Verts[] = {
         -1.000f,  -1.000f,
          1.000f,  -1.000f,
         -1.000f,   1.000f,
          1.000f,   1.000f,
    };

    g_FSIndexCount = sizeof(Faces) / sizeof(Faces[0]);

    // Create the VAO:
    DEBUG_PRINT( _L("        - Create the VAO") );
    GL_CHECK(glGenVertexArrays(1, &g_FSVAO));
    GL_CHECK(glBindVertexArray(g_FSVAO));

    // Create the VBO for positions:
    DEBUG_PRINT( _L("        - Create the VBO pos") );
    GLuint positions;
    GLsizei stride = 2 * sizeof(float);
    GL_CHECK( glGenBuffers(1, &positions) );
    GL_CHECK( glBindBuffer(GL_ARRAY_BUFFER, positions) );
    GL_CHECK( glBufferData(GL_ARRAY_BUFFER, sizeof(Verts), Verts, GL_STATIC_DRAW) );
    GL_CHECK( glEnableVertexAttribArray(g_FSPositionSlot) );
    GL_CHECK( glVertexAttribPointer(g_FSPositionSlot, 2, GL_FLOAT, GL_FALSE, stride, 0) );

    // Create the VBO for indices:
    DEBUG_PRINT( _L("        - Create the VBO ind") );
    GLuint indices;
    GL_CHECK( glGenBuffers(1, &indices) );
    GL_CHECK( glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices) );
    GL_CHECK( glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Faces), Faces, GL_STATIC_DRAW) );

    GL_CHECK(glBindVertexArray( 0 ));
}

static void CreateAnimatedTrianglesGeometry()
{
    DEBUG_PRINT( _L("CreateAnimatedTrianglesGeometry") );

    int Faces[ g_ATCount * 3 ];

    for( int i = 0; i < g_ATCount * 3; i += 3 )
    {
        Faces[ i + 0 ] = i + 0;
        Faces[ i + 1 ] = i + 1;
        Faces[ i + 2 ] = i + 2;
    }

    srand( 0 );

    for( int i = 0; i < g_ATCount * 3; i++ )
    {
        unsigned char r = rand() & 0xFF | 0x40;
        unsigned char g = rand() & 0xFF | 0x40;
        unsigned char b = rand() & 0xFF | 0x40;
        unsigned char a = rand() & 0xFF | 0xA0;
        float x = (rand() / (float)RAND_MAX) * 2.0f - 1.0f;
        float y = (rand() / (float)RAND_MAX) * 2.0f - 1.0f;


        float ha = (rand() / (float)RAND_MAX) * 1.0f;               // triangle shape
        float hb = (rand() / (float)RAND_MAX) * 0.5f - 0.25f;       // triangle width
        float hc = (rand() / (float)RAND_MAX) * 2.0f - 1.0f;        // triangle speed x
        float hd = (rand() / (float)RAND_MAX) * 2.0f - 1.0f;        // triangle speed y

        g_ATVerts[ i ] = ATVertex( float2( x, y ), (a<<24) | (b<<16) | (g<<8) | (r<<0), float4( ha, hb, hc, hd ) );
    }

    g_ATIndexCount = sizeof(Faces) / sizeof(Faces[0]);

    // Create the VAO:
    DEBUG_PRINT( _L("        - Create the VAO") );
    GL_CHECK(glGenVertexArrays(1, &g_ATVAO));
    GL_CHECK(glBindVertexArray(g_ATVAO));

    // Create the VBO for positions:
    DEBUG_PRINT( _L("        - Create the VBO pos") );
    GLsizei stride = sizeof( ATVertex );
    GL_CHECK( glGenBuffers(1, &g_ATVAOVertices) );
    GL_CHECK( glBindBuffer(GL_ARRAY_BUFFER, g_ATVAOVertices) );
    GL_CHECK( glBufferData(GL_ARRAY_BUFFER, sizeof(g_ATVerts), g_ATVerts, GL_DYNAMIC_DRAW) );
    GL_CHECK( glEnableVertexAttribArray(g_ATPositionSlot) );
    GL_CHECK( glVertexAttribPointer(g_ATPositionSlot, 2, GL_FLOAT, GL_FALSE, stride, 0) );
    GL_CHECK( glEnableVertexAttribArray(g_ATColorSlot) );
    GL_CHECK( glVertexAttribPointer( g_ATColorSlot, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, (void*)(2*sizeof(float)) ) );

    // Create the VBO for indices:
    DEBUG_PRINT( _L("        - Create the VBO ind") );
    GLuint indices;
    GL_CHECK( glGenBuffers(1, &indices) );
    GL_CHECK( glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices) );
    GL_CHECK( glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Faces), Faces, GL_STATIC_DRAW) );

    GL_CHECK(glBindVertexArray( 0 ));
}


extern bool g_CPUT_GL_EGL_COMPUTE_SHADER_SUPPORTED;
extern bool g_CPUT_GL_IS_INTEL_VENDOR;
#ifdef CPUT_FOR_OGLES
extern EGLint g_CPUT_EGL_CONTEXT_MAJOR_VERSION_KHR;
extern EGLint g_CPUT_EGL_CONTEXT_MINOR_VERSION_KHR;
#else
extern bool g_CPUT_GL_IS_AMD_VENDOR;
#endif

void MySample::RecreateShaders()
{
    DEBUG_PRINT( _L("SampleFastBlurs: Loading shaders") );
    GLint compileSuccess, linkSuccess;
    GLchar compilerSpew[256];

    static bool alreadyBeenHere = false;
    if( alreadyBeenHere )
    {
        glswShutdown();
        glDeleteProgram( g_ProgramHandleFullscreenQuad      );
        glDeleteProgram( g_ProgramHandleAnimatedTriangles   );
        glDeleteProgram( g_ProgramHandleDownsample2x2       );
        glDeleteProgram( g_ProgramHandleDownsample4x4       );
        glDeleteProgram( g_ProgramHandleKawaseBlurPass      );
        glDeleteProgram( g_ProgramHandleGaussianH           );
        glDeleteProgram( g_ProgramHandleGaussianV           );
        glDeleteProgram( g_ProgramHandleComputeShaderBlurH  );
        glDeleteProgram( g_ProgramHandleComputeShaderBlurV  );
        glDeleteProgram( g_ProgramHandleApply               );
        glDeleteProgram( g_ProgramHandleDebugGraphDrawInput );
        glDeleteProgram( g_ProgramHandleDebugGraphDraw      );
    }
    glswInit();
    alreadyBeenHere = true;

    //TODO: Use CPUT path searching here
#ifdef CPUT_OS_WINDOWS
    glswSetPath("../media/shader/", ".glsl");
#else
    glswSetPath("shader/", ".glsl");
#endif

#ifdef CPUT_FOR_OGLES
    assert( g_CPUT_EGL_CONTEXT_MAJOR_VERSION_KHR == 3 );

    if( g_CPUT_GL_EGL_COMPUTE_SHADER_SUPPORTED )
    {
        glswAddDirectiveToken("*", "#version 310 es");
        DEBUG_PRINT( _L(" SHADERS FOR #version 310 es") );
    }
    else
    {
        glswAddDirectiveToken("*", "#version 300 es");
        DEBUG_PRINT( _L(" SHADERS FOR #version 300 es") );
    }
#else
    if( g_CPUT_GL_EGL_COMPUTE_SHADER_SUPPORTED )
    {
        glswAddDirectiveToken("*", "#version 430");
        DEBUG_PRINT( _L(" SHADERS FOR #version 430") );
    }
    else
    {
        glswAddDirectiveToken("*", "#version 330");
        DEBUG_PRINT( _L(" SHADERS FOR #version 330") );
    }
#endif
    
    std::string globalShaderConstants;

    const int shaderKernel[] = { 0, 1, 1, 2, 3 };
    mSettings.KawaseBlurPasses = _countof( shaderKernel );
    memcpy( mSettings.KawaseBlurPassKernelSize, shaderKernel, _countof(shaderKernel) * sizeof(*shaderKernel) );

    int computeShaderKernelSize = 7;

    switch( mSettings.BlurKernelSizePreset )
    {
    case SampleSettings::KernelSizeVerySmall:
        {
            mSettings.GaussKernelSize   = 7;
            computeShaderKernelSize     = 7;
            const int shaderKernel[] = { 0, 0 };
            mSettings.KawaseBlurPasses = _countof( shaderKernel );
            memcpy( mSettings.KawaseBlurPassKernelSize, shaderKernel, _countof(shaderKernel) * sizeof(*shaderKernel) );
        } break;
    case SampleSettings::KernelSizeSmall:
        {
            mSettings.GaussKernelSize   = 15;
            computeShaderKernelSize     = 15;
            const int shaderKernel[] = { 0, 1, 1 };
            mSettings.KawaseBlurPasses = _countof( shaderKernel );
            memcpy( mSettings.KawaseBlurPassKernelSize, shaderKernel, _countof(shaderKernel) * sizeof(*shaderKernel) );
        } break;
    case SampleSettings::KernelSizeMedium:
        {
            mSettings.GaussKernelSize   = 23;
            computeShaderKernelSize     = 23;
            const int shaderKernel[] = { 0, 1, 1, 2 };
            mSettings.KawaseBlurPasses = _countof( shaderKernel );
            memcpy( mSettings.KawaseBlurPassKernelSize, shaderKernel, _countof(shaderKernel) * sizeof(*shaderKernel) );
        } break;
    case SampleSettings::KernelSizeLarge:
        {
            mSettings.GaussKernelSize   = 35;
            computeShaderKernelSize     = 35;
            const int shaderKernel[] = { 0, 1, 2, 2, 3 };
            mSettings.KawaseBlurPasses = _countof( shaderKernel );
            memcpy( mSettings.KawaseBlurPassKernelSize, shaderKernel, _countof(shaderKernel) * sizeof(*shaderKernel) );
        } break;
    case SampleSettings::KernelSizeVeryLarge:
        {
            mSettings.GaussKernelSize   = 63;
            computeShaderKernelSize     = 63;
            const int shaderKernel[] = { 0, 1, 2, 3, 4, 4, 5 };
            mSettings.KawaseBlurPasses = _countof( shaderKernel );
            memcpy( mSettings.KawaseBlurPassKernelSize, shaderKernel, _countof(shaderKernel) * sizeof(*shaderKernel) );
        } break;
    case SampleSettings::KernelSizeHumonguous:
        {
            mSettings.GaussKernelSize   = 127;
            computeShaderKernelSize     = 127;
            const int shaderKernel[] = { 0, 1, 2, 3, 4, 5, 7, 8, 9, 10 };
            mSettings.KawaseBlurPasses = _countof( shaderKernel );
            memcpy( mSettings.KawaseBlurPassKernelSize, shaderKernel, _countof(shaderKernel) * sizeof(*shaderKernel) );
        } break;
    default:
        break;
    }

    // has to be odd number to begin with but verify anyway...
    assert( (mSettings.GaussKernelSize % 2) == 1 );
    
    // ...and then half of it, since we're double-passing, and reduce a bit to match apparent visual blurriness of Gauss...
    computeShaderKernelSize = (mSettings.GaussKernelSize*10) / 27;

    // ...and it has to be odd number!!
    if( (computeShaderKernelSize % 2) != 1 )
        computeShaderKernelSize++;
    assert( (computeShaderKernelSize % 2) == 1 );

    int kernelSizeForDebugGraph = mSettings.GaussKernelSize;
    if( ((int)g_RTSizeSubRTSize[0] / (int)g_RTSizeSubRTSize[2]) == 1 )
        kernelSizeForDebugGraph = (mSettings.GaussKernelSize+1) / 2;
    if( ((int)g_RTSizeSubRTSize[0] / (int)g_RTSizeSubRTSize[2]) == 4 )
        kernelSizeForDebugGraph = mSettings.GaussKernelSize * 2;

    // adjust for nice debugging view
    kernelSizeForDebugGraph = (kernelSizeForDebugGraph * 7 + 1) / 10;

    // define shader constants

    globalShaderConstants = cStringFormatA( "#define cRTPixelSize vec4( %.7f, %.7f, %.7f, %.7f ) \n", 1.0f / g_RTSizeSubRTSize[0], 1.0f / g_RTSizeSubRTSize[1], 1.0f / g_RTSizeSubRTSize[2], 1.0f / g_RTSizeSubRTSize[3] );
    glswAddDirectiveToken( "*", globalShaderConstants.c_str() );

    globalShaderConstants = cStringFormatA( "#define cRTScreenSizeI ivec4( %d, %d, %d, %d ) \n", (int)g_RTSizeSubRTSize[0], (int)g_RTSizeSubRTSize[1], (int)g_RTSizeSubRTSize[2], (int)g_RTSizeSubRTSize[3] );
    glswAddDirectiveToken( "*", globalShaderConstants.c_str() );

    globalShaderConstants = cStringFormatA( "#define cRTScreenSize vec4( %.7f, %.7f, %.7f, %.7f ) \n", g_RTSizeSubRTSize[0], g_RTSizeSubRTSize[1], g_RTSizeSubRTSize[2], g_RTSizeSubRTSize[3] );
    glswAddDirectiveToken( "*", globalShaderConstants.c_str() );

    globalShaderConstants = cStringFormatA( "#define CS_THREAD_GROUP_SIZE %d\n", CS_THREAD_GROUP_SIZE );
    glswAddDirectiveToken( "*", globalShaderConstants.c_str() );

    globalShaderConstants = cStringFormatA( "#define KERNEL_SIZE_PRESET %d\n", mSettings.BlurKernelSizePreset );
    glswAddDirectiveToken( "*", globalShaderConstants.c_str() );
    
    globalShaderConstants = cStringFormatA( "#define KERNEL_SIZE %d\n", mSettings.GaussKernelSize );
    glswAddDirectiveToken( "*", globalShaderConstants.c_str() );

    globalShaderConstants = cStringFormatA( "#define KERNEL_SIZE_FOR_DEBUG_GRAPH %d\n", kernelSizeForDebugGraph );
    glswAddDirectiveToken( "*", globalShaderConstants.c_str() );

    globalShaderConstants = cStringFormatA( "#define COMPUTE_SHADER_KERNEL_SIZE %d\n", computeShaderKernelSize );
    glswAddDirectiveToken( "*", globalShaderConstants.c_str() );

    // generate Gauss blur shader code for the specified kernel size
    bool workaroundForNoCLikeArrayInitialization = false;
#ifdef CPUT_FOR_OGLES
    workaroundForNoCLikeArrayInitialization = g_CPUT_EGL_CONTEXT_MINOR_VERSION_KHR == 0;
#endif
    std::string gaussShaderCode = GenerateGaussFunctionCode( mSettings.GaussKernelSize, workaroundForNoCLikeArrayInitialization );
    glswAddDirectiveToken( "*", gaussShaderCode.c_str() );

    g_LastGaussKernelShader = gaussShaderCode;

    // FullScreenQuad
    {
        DEBUG_PRINT( _L(" loading shaders: FullScreenQuad") );
        const char* vsSource = glswGetShader("fullscreenquad.Vertex");
        const char* fsSource = glswGetShader("fullscreenquad.Fragment");
        const char* msg = "Can't find %s shader.  Does path exist?\n";

        PezCheckCondition(vsSource != 0, msg, "vertex");
        PezCheckCondition(fsSource != 0, msg, "fragment");

        GLuint vsHandle = glCreateShader(GL_VERTEX_SHADER);
        GLuint fsHandle = glCreateShader(GL_FRAGMENT_SHADER);

        GL_CHECK( glShaderSource(vsHandle, 1, &vsSource, 0) );
        GL_CHECK( glCompileShader(vsHandle) );
        GL_CHECK( glGetShaderiv(vsHandle, GL_COMPILE_STATUS, &compileSuccess) );
        GL_CHECK( glGetShaderInfoLog(vsHandle, sizeof(compilerSpew), 0, compilerSpew) );
        PezCheckCondition(compileSuccess, "Vertex Shader Errors:\n%s", compilerSpew);

        GL_CHECK( glShaderSource(fsHandle, 1, &fsSource, 0) );
        GL_CHECK( glCompileShader(fsHandle) );
        GL_CHECK( glGetShaderiv(fsHandle, GL_COMPILE_STATUS, &compileSuccess) );
        GL_CHECK( glGetShaderInfoLog(fsHandle, sizeof(compilerSpew), 0, compilerSpew) );
        PezCheckCondition(compileSuccess, "Fragment Shader Errors:\n%s", compilerSpew);
         
        g_ProgramHandleFullscreenQuad = glCreateProgram();
        GL_CHECK( glAttachShader(g_ProgramHandleFullscreenQuad, vsHandle) );
        GL_CHECK( glAttachShader(g_ProgramHandleFullscreenQuad, fsHandle) );
        GL_CHECK( glBindAttribLocation(g_ProgramHandleFullscreenQuad, g_FSPositionSlot, "Position") );
        GL_CHECK( glLinkProgram(g_ProgramHandleFullscreenQuad) );
        GL_CHECK( glGetProgramiv(g_ProgramHandleFullscreenQuad, GL_LINK_STATUS, &linkSuccess) );
        GL_CHECK( glGetProgramInfoLog(g_ProgramHandleFullscreenQuad, sizeof(compilerSpew), 0, compilerSpew) );
        PezCheckCondition(linkSuccess, "Shader Link Errors:\n%s", compilerSpew);
	
        g_Uniforms.DrawFullscreenQuad_Tex0   = glGetUniformLocation(g_ProgramHandleFullscreenQuad, "uTex0");
    }
    
    // DrawScene
    {
        DEBUG_PRINT( _L(" loading shaders: DrawScene") );
        const char* vsSource = glswGetShader("drawscene.Vertex");
        const char* fsSource = glswGetShader("drawscene.Fragment");
        const char* msg = "Can't find %s shader.  Does path exist?\n";

        PezCheckCondition(vsSource != 0, msg, "vertex");
        PezCheckCondition(fsSource != 0, msg, "fragment");

        GLuint vsHandle = glCreateShader(GL_VERTEX_SHADER);
        GLuint fsHandle = glCreateShader(GL_FRAGMENT_SHADER);

        GL_CHECK( glShaderSource(vsHandle, 1, &vsSource, 0) );
        GL_CHECK( glCompileShader(vsHandle) );
        GL_CHECK( glGetShaderiv(vsHandle, GL_COMPILE_STATUS, &compileSuccess) );
        GL_CHECK( glGetShaderInfoLog(vsHandle, sizeof(compilerSpew), 0, compilerSpew) );
        PezCheckCondition(compileSuccess, "Vertex Shader Errors:\n%s", compilerSpew);

        GL_CHECK( glShaderSource(fsHandle, 1, &fsSource, 0) );
        GL_CHECK( glCompileShader(fsHandle) );
        GL_CHECK( glGetShaderiv(fsHandle, GL_COMPILE_STATUS, &compileSuccess) );
        GL_CHECK( glGetShaderInfoLog(fsHandle, sizeof(compilerSpew), 0, compilerSpew) );
        PezCheckCondition(compileSuccess, "Fragment Shader Errors:\n%s", compilerSpew);

        g_ProgramHandleAnimatedTriangles = glCreateProgram();
        GL_CHECK( glAttachShader(g_ProgramHandleAnimatedTriangles, vsHandle) );
        GL_CHECK( glAttachShader(g_ProgramHandleAnimatedTriangles, fsHandle) );
        GL_CHECK( glBindAttribLocation(g_ProgramHandleAnimatedTriangles, g_ATPositionSlot, "Position") );
        GL_CHECK( glBindAttribLocation(g_ProgramHandleAnimatedTriangles, g_ATColorSlot, "Color") );
        GL_CHECK( glLinkProgram(g_ProgramHandleAnimatedTriangles) );
        GL_CHECK( glGetProgramiv(g_ProgramHandleAnimatedTriangles, GL_LINK_STATUS, &linkSuccess) );
        GL_CHECK( glGetProgramInfoLog(g_ProgramHandleAnimatedTriangles, sizeof(compilerSpew), 0, compilerSpew) );
        PezCheckCondition(linkSuccess, "Shader Link Errors:\n%s", compilerSpew);
    }

    // Downsample
    {
        DEBUG_PRINT( _L(" loading shaders: Downsample") );
        const char* vsSource = glswGetShader("blur_downsample.Vertex");
        const char* fsSource2x2 = glswGetShader("blur_downsample.Fragment2x2");
        const char* fsSource4x4 = glswGetShader("blur_downsample.Fragment4x4");
        const char* msg = "Can't find %s shader.  Does path exist?\n";

        PezCheckCondition(vsSource != 0, msg, "vertex");
        PezCheckCondition(fsSource2x2 != 0, msg, "fragment2x2");
        PezCheckCondition(fsSource4x4 != 0, msg, "fragment4x4");

        GLuint vsHandle = glCreateShader(GL_VERTEX_SHADER);
        GLuint fsHandle2x2 = glCreateShader(GL_FRAGMENT_SHADER);
        GLuint fsHandle4x4 = glCreateShader(GL_FRAGMENT_SHADER);

        GL_CHECK( glShaderSource(vsHandle, 1, &vsSource, 0) );
        GL_CHECK( glCompileShader(vsHandle) );
        GL_CHECK( glGetShaderiv(vsHandle, GL_COMPILE_STATUS, &compileSuccess) );
        GL_CHECK( glGetShaderInfoLog(vsHandle, sizeof(compilerSpew), 0, compilerSpew) );
        PezCheckCondition(compileSuccess, "Vertex Shader Errors:\n%s", compilerSpew);

        GL_CHECK( glShaderSource(fsHandle2x2, 1, &fsSource2x2, 0) );
        GL_CHECK( glCompileShader(fsHandle2x2) );
        GL_CHECK( glGetShaderiv(fsHandle2x2, GL_COMPILE_STATUS, &compileSuccess) );
        GL_CHECK( glGetShaderInfoLog(fsHandle2x2, sizeof(compilerSpew), 0, compilerSpew) );
        PezCheckCondition(compileSuccess, "Fragment Shader Errors:\n%s", compilerSpew);

        g_ProgramHandleDownsample2x2 = glCreateProgram();
        GL_CHECK( glAttachShader(g_ProgramHandleDownsample2x2, vsHandle) );
        GL_CHECK( glAttachShader(g_ProgramHandleDownsample2x2, fsHandle2x2) );
        GL_CHECK( glBindAttribLocation(g_ProgramHandleDownsample2x2, g_FSPositionSlot, "Position") );
        GL_CHECK( glLinkProgram(g_ProgramHandleDownsample2x2) );
        GL_CHECK( glGetProgramiv(g_ProgramHandleDownsample2x2, GL_LINK_STATUS, &linkSuccess) );
        GL_CHECK( glGetProgramInfoLog(g_ProgramHandleDownsample2x2, sizeof(compilerSpew), 0, compilerSpew) );
        PezCheckCondition(linkSuccess, "Shader Link Errors:\n%s", compilerSpew);

        g_Uniforms.DownSample2x2_Tex0              = glGetUniformLocation( g_ProgramHandleDownsample2x2, "uTex0" );

        GL_CHECK( glShaderSource(fsHandle4x4, 1, &fsSource4x4, 0) );
        GL_CHECK( glCompileShader(fsHandle4x4) );
        GL_CHECK( glGetShaderiv(fsHandle4x4, GL_COMPILE_STATUS, &compileSuccess) );
        GL_CHECK( glGetShaderInfoLog(fsHandle4x4, sizeof(compilerSpew), 0, compilerSpew) );
        PezCheckCondition(compileSuccess, "Fragment Shader Errors:\n%s", compilerSpew);

        g_ProgramHandleDownsample4x4 = glCreateProgram();
        GL_CHECK( glAttachShader(g_ProgramHandleDownsample4x4, vsHandle) );
        GL_CHECK( glAttachShader(g_ProgramHandleDownsample4x4, fsHandle4x4) );
        GL_CHECK( glBindAttribLocation(g_ProgramHandleDownsample4x4, g_FSPositionSlot, "Position") );
        GL_CHECK( glLinkProgram(g_ProgramHandleDownsample4x4) );
        GL_CHECK( glGetProgramiv(g_ProgramHandleDownsample4x4, GL_LINK_STATUS, &linkSuccess) );
        GL_CHECK( glGetProgramInfoLog(g_ProgramHandleDownsample4x4, sizeof(compilerSpew), 0, compilerSpew) );
        PezCheckCondition(linkSuccess, "Shader Link Errors:\n%s", compilerSpew);

        g_Uniforms.DownSample4x4_Tex0              = glGetUniformLocation( g_ProgramHandleDownsample4x4, "uTex0" );
    }

    // Kawase blur pass
    {
        DEBUG_PRINT( _L(" loading shaders: KawaseBlurPass") );
        const char* vsSource = glswGetShader("blur_kawase_pass.Vertex");
        const char* fsSource = glswGetShader("blur_kawase_pass.Fragment");
        const char* msg = "Can't find %s shader.  Does path exist?\n";

        PezCheckCondition(vsSource != 0, msg, "vertex");
        PezCheckCondition(fsSource != 0, msg, "fragment");

        GLuint vsHandle = glCreateShader(GL_VERTEX_SHADER);
        GLuint fsHandle = glCreateShader(GL_FRAGMENT_SHADER);

        GL_CHECK( glShaderSource(vsHandle, 1, &vsSource, 0) );
        GL_CHECK( glCompileShader(vsHandle) );
        GL_CHECK( glGetShaderiv(vsHandle, GL_COMPILE_STATUS, &compileSuccess) );
        GL_CHECK( glGetShaderInfoLog(vsHandle, sizeof(compilerSpew), 0, compilerSpew) );
        PezCheckCondition(compileSuccess, "Vertex Shader Errors:\n%s", compilerSpew);

        GL_CHECK( glShaderSource(fsHandle, 1, &fsSource, 0) );
        GL_CHECK( glCompileShader(fsHandle) );
        GL_CHECK( glGetShaderiv(fsHandle, GL_COMPILE_STATUS, &compileSuccess) );
        GL_CHECK( glGetShaderInfoLog(fsHandle, sizeof(compilerSpew), 0, compilerSpew) );
        PezCheckCondition(compileSuccess, "Fragment Shader Errors:\n%s", compilerSpew);

        g_ProgramHandleKawaseBlurPass = glCreateProgram();
        GL_CHECK( glAttachShader(g_ProgramHandleKawaseBlurPass, vsHandle) );
        GL_CHECK( glAttachShader(g_ProgramHandleKawaseBlurPass, fsHandle) );
        GL_CHECK( glBindAttribLocation(g_ProgramHandleKawaseBlurPass, g_FSPositionSlot, "Position") );
        GL_CHECK( glLinkProgram(g_ProgramHandleKawaseBlurPass) );
        GL_CHECK( glGetProgramiv(g_ProgramHandleKawaseBlurPass, GL_LINK_STATUS, &linkSuccess) );
        GL_CHECK( glGetProgramInfoLog(g_ProgramHandleKawaseBlurPass, sizeof(compilerSpew), 0, compilerSpew) );
        PezCheckCondition(linkSuccess, "Shader Link Errors:\n%s", compilerSpew);

        g_Uniforms.KawaseBlurPass_Tex0                            = glGetUniformLocation( g_ProgramHandleKawaseBlurPass, "uTex0" );
        g_Uniforms.KawaseBlurPass_xyPixelSize_zIteration_wDummy   = glGetUniformLocation( g_ProgramHandleKawaseBlurPass, "u_xyPixelSize_zIteration_wDummy" );
    }

    // Gaussian blur pass
    {
        DEBUG_PRINT( _L(" loading shaders: gaussian blur") );
        const char* vsSource = glswGetShader("blur_gaussian.Vertex");
        const char* fsHSource = glswGetShader("blur_gaussian.FragHoriz");
        const char* fsVSource = glswGetShader("blur_gaussian.FragVert");
        const char* msg = "Can't find %s shader.  Does path exist?\n";

        PezCheckCondition(vsSource != 0, msg, "vertex");
        PezCheckCondition(fsHSource != 0, msg, "fragmentH");
        PezCheckCondition(fsVSource != 0, msg, "fragmentV");

        GLuint vsHandle = glCreateShader(GL_VERTEX_SHADER);
        GLuint fsHHandle = glCreateShader(GL_FRAGMENT_SHADER);
        GLuint fsVHandle = glCreateShader(GL_FRAGMENT_SHADER);

        GL_CHECK( glShaderSource(vsHandle, 1, &vsSource, 0) );
        GL_CHECK( glCompileShader(vsHandle) );
        GL_CHECK( glGetShaderiv(vsHandle, GL_COMPILE_STATUS, &compileSuccess) );
        GL_CHECK( glGetShaderInfoLog(vsHandle, sizeof(compilerSpew), 0, compilerSpew) );
        PezCheckCondition(compileSuccess, "Vertex Shader Errors:\n%s", compilerSpew);

        GL_CHECK( glShaderSource(fsHHandle, 1, &fsHSource, 0) );
        GL_CHECK( glCompileShader(fsHHandle) );
        GL_CHECK( glGetShaderiv(fsHHandle, GL_COMPILE_STATUS, &compileSuccess) );
        GL_CHECK( glGetShaderInfoLog(fsHHandle, sizeof(compilerSpew), 0, compilerSpew) );
        PezCheckCondition(compileSuccess, "Fragment Shader Errors:\n%s", compilerSpew);

        GL_CHECK( glShaderSource(fsVHandle, 1, &fsVSource, 0) );
        GL_CHECK( glCompileShader(fsVHandle) );
        GL_CHECK( glGetShaderiv(fsVHandle, GL_COMPILE_STATUS, &compileSuccess) );
        GL_CHECK( glGetShaderInfoLog(fsVHandle, sizeof(compilerSpew), 0, compilerSpew) );
        PezCheckCondition(compileSuccess, "Fragment Shader Errors:\n%s", compilerSpew);

        g_ProgramHandleGaussianH = glCreateProgram();
        GL_CHECK( glAttachShader(g_ProgramHandleGaussianH, vsHandle) );
        GL_CHECK( glAttachShader(g_ProgramHandleGaussianH, fsHHandle) );
        GL_CHECK( glBindAttribLocation(g_ProgramHandleGaussianH, g_FSPositionSlot, "Position") );
        GL_CHECK( glLinkProgram(g_ProgramHandleGaussianH) );
        GL_CHECK( glGetProgramiv(g_ProgramHandleGaussianH, GL_LINK_STATUS, &linkSuccess) );
        GL_CHECK( glGetProgramInfoLog(g_ProgramHandleGaussianH, sizeof(compilerSpew), 0, compilerSpew) );
        PezCheckCondition(linkSuccess, "Shader Link Errors:\n%s", compilerSpew);

        g_ProgramHandleGaussianV = glCreateProgram();
        GL_CHECK( glAttachShader(g_ProgramHandleGaussianV, vsHandle) );
        GL_CHECK( glAttachShader(g_ProgramHandleGaussianV, fsVHandle) );
        GL_CHECK( glBindAttribLocation(g_ProgramHandleGaussianV, g_FSPositionSlot, "Position") );
        GL_CHECK( glLinkProgram(g_ProgramHandleGaussianV) );
        GL_CHECK( glGetProgramiv(g_ProgramHandleGaussianV, GL_LINK_STATUS, &linkSuccess) );
        GL_CHECK( glGetProgramInfoLog(g_ProgramHandleGaussianV, sizeof(compilerSpew), 0, compilerSpew) );
        PezCheckCondition(linkSuccess, "Shader Link Errors:\n%s", compilerSpew);

        g_Uniforms.GaussianH_Tex0                       = glGetUniformLocation( g_ProgramHandleGaussianH, "uTex0" );
        g_Uniforms.GaussianV_Tex0                       = glGetUniformLocation( g_ProgramHandleGaussianV, "uTex0" );
        g_Uniforms.GaussianH_RTPixelSizePixelSizeHalf   = glGetUniformLocation( g_ProgramHandleGaussianH, "uRTPixelSizePixelSizeHalf" );
        g_Uniforms.GaussianV_RTPixelSizePixelSizeHalf   = glGetUniformLocation( g_ProgramHandleGaussianV, "uRTPixelSizePixelSizeHalf" );
    }

    // Compute shader blur
    if( g_CPUT_GL_EGL_COMPUTE_SHADER_SUPPORTED )
    {
        DEBUG_PRINT( _L(" loading shaders: blur_computeshader") );

        {
            const char* csSource = glswGetShader("blur_computeshader.ComputeH");
            const char* msg = "Can't find %s shader.  Does path exist?\n";

            PezCheckCondition(csSource != 0, msg, "computeH");

            GLuint csHandle = glCreateShader(GL_COMPUTE_SHADER);

            GL_CHECK( glShaderSource(csHandle, 1, &csSource, 0) );
            GL_CHECK( glCompileShader(csHandle) );
            GL_CHECK( glGetShaderiv(csHandle, GL_COMPILE_STATUS, &compileSuccess) );
            GL_CHECK( glGetShaderInfoLog(csHandle, sizeof(compilerSpew), 0, compilerSpew) );
            PezCheckCondition(compileSuccess, "Compute Shader Errors:\n%s", compilerSpew);

            g_ProgramHandleComputeShaderBlurH = glCreateProgram();
            GL_CHECK( glAttachShader( g_ProgramHandleComputeShaderBlurH, csHandle) );
            GL_CHECK( glLinkProgram( g_ProgramHandleComputeShaderBlurH) );
            GL_CHECK( glGetProgramiv( g_ProgramHandleComputeShaderBlurH, GL_LINK_STATUS, &linkSuccess) );
            GL_CHECK( glGetProgramInfoLog( g_ProgramHandleComputeShaderBlurH, sizeof(compilerSpew), 0, compilerSpew) );
            PezCheckCondition(linkSuccess, "Compute Shader Link Errors:\n%s", compilerSpew);
        }

        {
            const char* csSource = glswGetShader("blur_computeshader.ComputeV");
            const char* msg = "Can't find %s shader.  Does path exist?\n";

            PezCheckCondition(csSource != 0, msg, "computeH");

            GLuint csHandle = glCreateShader(GL_COMPUTE_SHADER);

            GL_CHECK( glShaderSource(csHandle, 1, &csSource, 0) );
            GL_CHECK( glCompileShader(csHandle) );
            GL_CHECK( glGetShaderiv(csHandle, GL_COMPILE_STATUS, &compileSuccess) );
            GL_CHECK( glGetShaderInfoLog(csHandle, sizeof(compilerSpew), 0, compilerSpew) );
            PezCheckCondition(compileSuccess, "Compute Shader Errors:\n%s", compilerSpew);

            g_ProgramHandleComputeShaderBlurV = glCreateProgram();
            GL_CHECK( glAttachShader( g_ProgramHandleComputeShaderBlurV, csHandle) );
            GL_CHECK( glLinkProgram( g_ProgramHandleComputeShaderBlurV) );
            GL_CHECK( glGetProgramiv( g_ProgramHandleComputeShaderBlurV, GL_LINK_STATUS, &linkSuccess) );
            GL_CHECK( glGetProgramInfoLog( g_ProgramHandleComputeShaderBlurV, sizeof(compilerSpew), 0, compilerSpew) );
            PezCheckCondition(linkSuccess, "Compute Shader Link Errors:\n%s", compilerSpew);
        }
    }

    // Apply
    {
        DEBUG_PRINT( _L(" loading shaders: Apply") );
        const char* vsSource = glswGetShader("blur_apply.Vertex");
        const char* fsSource = glswGetShader("blur_apply.Fragment");
        const char* msg = "Can't find %s shader.  Does path exist?\n";

        PezCheckCondition(vsSource != 0, msg, "vertex");
        PezCheckCondition(fsSource != 0, msg, "fragment");

        GLuint vsHandle = glCreateShader(GL_VERTEX_SHADER);
        GLuint fsHandle = glCreateShader(GL_FRAGMENT_SHADER);

        GL_CHECK( glShaderSource(vsHandle, 1, &vsSource, 0) );
        GL_CHECK( glCompileShader(vsHandle) );
        GL_CHECK( glGetShaderiv(vsHandle, GL_COMPILE_STATUS, &compileSuccess) );
        GL_CHECK( glGetShaderInfoLog(vsHandle, sizeof(compilerSpew), 0, compilerSpew) );
        PezCheckCondition(compileSuccess, "Vertex Shader Errors:\n%s", compilerSpew);

        GL_CHECK( glShaderSource(fsHandle, 1, &fsSource, 0) );
        GL_CHECK( glCompileShader(fsHandle) );
        GL_CHECK( glGetShaderiv(fsHandle, GL_COMPILE_STATUS, &compileSuccess) );
        GL_CHECK( glGetShaderInfoLog(fsHandle, sizeof(compilerSpew), 0, compilerSpew) );
        PezCheckCondition(compileSuccess, "Fragment Shader Errors:\n%s", compilerSpew);

        g_ProgramHandleApply = glCreateProgram();
        GL_CHECK( glAttachShader(g_ProgramHandleApply, vsHandle) );
        GL_CHECK( glAttachShader(g_ProgramHandleApply, fsHandle) );
        GL_CHECK( glBindAttribLocation(g_ProgramHandleApply, g_FSPositionSlot, "Position") );
        GL_CHECK( glLinkProgram(g_ProgramHandleApply) );
        GL_CHECK( glGetProgramiv(g_ProgramHandleApply, GL_LINK_STATUS, &linkSuccess) );
        GL_CHECK( glGetProgramInfoLog(g_ProgramHandleApply, sizeof(compilerSpew), 0, compilerSpew) );
        PezCheckCondition(linkSuccess, "Shader Link Errors:\n%s", compilerSpew);

        g_Uniforms.Apply_Tex0                   = glGetUniformLocation( g_ProgramHandleApply, "uTex0" );
        g_Uniforms.Apply_Params                 = glGetUniformLocation( g_ProgramHandleApply, "uApplyParams" );
        g_Uniforms.Apply_FinalPixelSize         = glGetUniformLocation( g_ProgramHandleApply, "uFinalPixelSize" );
    }

    // Debug graph programs
    {
        DEBUG_PRINT( _L(" loading shaders: DebugGraph") );
        const char* vsSource = glswGetShader("blur_debuggraph.Vertex");
        const char* fsFragDrawInputSource = glswGetShader("blur_debuggraph.FragDrawInput");
        const char* fsFragDrawGraphSource = glswGetShader("blur_debuggraph.FragDrawGraph");
        const char* msg = "Can't find %s shader.  Does path exist?\n";

        PezCheckCondition(vsSource != 0, msg, "vertex");
        PezCheckCondition(fsFragDrawInputSource != 0, msg, "FragDrawInput");
        PezCheckCondition(fsFragDrawGraphSource != 0, msg, "FragDrawGraph");

        GLuint vsHandle = glCreateShader(GL_VERTEX_SHADER);
        GLuint fsFragDrawInputHandle = glCreateShader(GL_FRAGMENT_SHADER);
        GLuint fsFragDrawGraphHandle = glCreateShader(GL_FRAGMENT_SHADER);

        GL_CHECK( glShaderSource(vsHandle, 1, &vsSource, 0) );
        GL_CHECK( glCompileShader(vsHandle) );
        GL_CHECK( glGetShaderiv(vsHandle, GL_COMPILE_STATUS, &compileSuccess) );
        GL_CHECK( glGetShaderInfoLog(vsHandle, sizeof(compilerSpew), 0, compilerSpew) );
        PezCheckCondition(compileSuccess, "Vertex Shader Errors:\n%s", compilerSpew);

        GL_CHECK( glShaderSource(fsFragDrawInputHandle, 1, &fsFragDrawInputSource, 0) );
        GL_CHECK( glCompileShader(fsFragDrawInputHandle) );
        GL_CHECK( glGetShaderiv(fsFragDrawInputHandle, GL_COMPILE_STATUS, &compileSuccess) );
        GL_CHECK( glGetShaderInfoLog(fsFragDrawInputHandle, sizeof(compilerSpew), 0, compilerSpew) );
        PezCheckCondition(compileSuccess, "Fragment Shader Errors:\n%s", compilerSpew);

        GL_CHECK( glShaderSource(fsFragDrawGraphHandle, 1, &fsFragDrawGraphSource, 0) );
        GL_CHECK( glCompileShader(fsFragDrawGraphHandle) );
        GL_CHECK( glGetShaderiv(fsFragDrawGraphHandle, GL_COMPILE_STATUS, &compileSuccess) );
        GL_CHECK( glGetShaderInfoLog(fsFragDrawGraphHandle, sizeof(compilerSpew), 0, compilerSpew) );
        PezCheckCondition(compileSuccess, "Fragment Shader Errors:\n%s", compilerSpew);

        g_ProgramHandleDebugGraphDrawInput = glCreateProgram();
        GL_CHECK( glAttachShader(g_ProgramHandleDebugGraphDrawInput, vsHandle) );
        GL_CHECK( glAttachShader(g_ProgramHandleDebugGraphDrawInput, fsFragDrawInputHandle) );
        GL_CHECK( glBindAttribLocation(g_ProgramHandleDebugGraphDrawInput, g_FSPositionSlot, "Position") );
        GL_CHECK( glLinkProgram(g_ProgramHandleDebugGraphDrawInput) );
        GL_CHECK( glGetProgramiv(g_ProgramHandleDebugGraphDrawInput, GL_LINK_STATUS, &linkSuccess) );
        GL_CHECK( glGetProgramInfoLog(g_ProgramHandleDebugGraphDrawInput, sizeof(compilerSpew), 0, compilerSpew) );
        PezCheckCondition(linkSuccess, "Shader Link Errors:\n%s", compilerSpew);

        g_ProgramHandleDebugGraphDraw = glCreateProgram();
        GL_CHECK( glAttachShader(g_ProgramHandleDebugGraphDraw, vsHandle) );
        GL_CHECK( glAttachShader(g_ProgramHandleDebugGraphDraw, fsFragDrawGraphHandle) );
        GL_CHECK( glBindAttribLocation(g_ProgramHandleDebugGraphDraw, g_FSPositionSlot, "Position") );
        GL_CHECK( glLinkProgram(g_ProgramHandleDebugGraphDraw) );
        GL_CHECK( glGetProgramiv(g_ProgramHandleDebugGraphDraw, GL_LINK_STATUS, &linkSuccess) );
        GL_CHECK( glGetProgramInfoLog(g_ProgramHandleDebugGraphDraw, sizeof(compilerSpew), 0, compilerSpew) );
        PezCheckCondition(linkSuccess, "Shader Link Errors:\n%s", compilerSpew);

        g_Uniforms.DebugGraphDraw_Tex0                      = glGetUniformLocation( g_ProgramHandleDebugGraphDraw, "uTex0" );
        g_Uniforms.DebugGraphDraw_Tex1                      = glGetUniformLocation( g_ProgramHandleDebugGraphDraw, "uTex1" );
    }

    DEBUG_PRINT( _L(" shaders loaded OK!!!") );

    mFPSLogLastIndex        = 0;
    mBenchmarkTextInfo->SetText( _L("") );
    mBenchmarkTextInfoLN2->SetText( _L("") );
}

static void CreateSamplers()
{
    DEBUG_PRINT( _L("SampleFastBlurs: Creating samplers...") );

    GL_CHECK( glGenSamplers( 1, &g_SamplerLinear));
    GL_CHECK( glSamplerParameteri( g_SamplerLinear, GL_TEXTURE_MIN_FILTER,   GL_LINEAR ) );
    GL_CHECK( glSamplerParameteri( g_SamplerLinear, GL_TEXTURE_MAG_FILTER,   GL_LINEAR ) );
    GL_CHECK( glSamplerParameteri( g_SamplerLinear, GL_TEXTURE_WRAP_S,       GL_CLAMP_TO_EDGE ) );
    GL_CHECK( glSamplerParameteri( g_SamplerLinear, GL_TEXTURE_WRAP_T,       GL_CLAMP_TO_EDGE ) );
    GL_CHECK( glSamplerParameteri( g_SamplerLinear, GL_TEXTURE_WRAP_R,       GL_CLAMP_TO_EDGE ) );
#ifndef CPUT_FOR_OGLES
    GL_CHECK( glSamplerParameterf( g_SamplerLinear, GL_TEXTURE_LOD_BIAS,     0.0f ) );
#endif

    GL_CHECK( glGenSamplers( 1, &g_SamplerPoint));
    GL_CHECK( glSamplerParameteri( g_SamplerPoint, GL_TEXTURE_MIN_FILTER,   GL_NEAREST ) );
    GL_CHECK( glSamplerParameteri( g_SamplerPoint, GL_TEXTURE_MAG_FILTER,   GL_NEAREST ) );
    GL_CHECK( glSamplerParameteri( g_SamplerPoint, GL_TEXTURE_WRAP_S,       GL_CLAMP_TO_EDGE ) );
    GL_CHECK( glSamplerParameteri( g_SamplerPoint, GL_TEXTURE_WRAP_T,       GL_CLAMP_TO_EDGE ) );
    GL_CHECK( glSamplerParameteri( g_SamplerPoint, GL_TEXTURE_WRAP_R,       GL_CLAMP_TO_EDGE ) );
#ifndef CPUT_FOR_OGLES
    GL_CHECK( glSamplerParameterf( g_SamplerPoint, GL_TEXTURE_LOD_BIAS,     0.0f ) );
#endif

    DEBUG_PRINT( _L(" samplers created OK!!!") );
}

MySample::MySample()
{
    mpAssetSet              = NULL;
    mpTestTexture           = NULL;
    mpSceneRT               = NULL;
    mpDownsampledRT0        = NULL;
    mpDownsampledRT1        = NULL;

    memset( mFPSLog, 0, sizeof(mFPSLog) );
    mFPSLogLastIndex        = 0;
    mFPSTextInfo            = NULL;

    mOtherTextInfo          = NULL;
    mBenchmarkTextInfo      = NULL;
    mBenchmarkTextInfoLN2   = NULL;
    mBlurResolutionTextInfo = NULL;
    mSceneDropdown          = NULL;
    mBlurResolutionDropdown = NULL;
    mBlurKernelSizePreset   = NULL;

#if defined CPUT_OS_LINUX
    mpFPSTimer = (CPUTTimer*) new CPUTTimerLinux();
#elif defined CPUT_OS_ANDROID
    mpFPSTimer = (CPUTTimer*) new SFBTimerAndroid();
#elif defined CPUT_OS_WINDOWS
    mpFPSTimer = (CPUTTimer*) new CPUTTimerWin();
#else
    #error "No OS defined"
#endif

    mpFPSTimer->StartTimer();

    for( int i = 0; i < SampleSettings::NumberOfModes; i++ )
        mButtonModes[i] = NULL;

    mButtonModeRunBenchmark            = NULL;
    mButtonShowGraph                   = NULL;
    mTextInfo                          = NULL;
}

MySample::~MySample()
{
    SAFE_DELETE( mpTimer );
    SAFE_DELETE( mpFPSTimer );

    SAFE_RELEASE( mpAssetSet );
    SAFE_RELEASE( mpTestTexture );
    SAFE_DELETE( mpSceneRT );
    SAFE_DELETE( mpDownsampledRT0 );
    SAFE_DELETE( mpDownsampledRT1 );
}

void MySample::Update(double deltaSeconds)
{
    static double totalSeconds = 13.0;
    totalSeconds += deltaSeconds;
    
    // update FPS
    {
        mFPSLogLastIndex = (mFPSLogLastIndex+1) % cFPSLogLength;

        double curFrameTime = mpFPSTimer->GetElapsedTime();
        mFPSLog[ mFPSLogLastIndex ] = (float)curFrameTime;

        if( (mFPSLogLastIndex % 3) == 0 )
        {
            double avgTimeTotal = 0.0f;
            for( int i = 0; i < cFPSLogLength; i++ )
                avgTimeTotal += mFPSLog[ i ];
            mAverageFrameTime = (float)(avgTimeTotal / (double)cFPSLogLength);
            mFPSAverage = (float)(1.0 / mAverageFrameTime);
        }
        if( (mFPSLogLastIndex % 3) == 1 )
        {
            cString FPSData     = cStringFormat( _L("FPS Avg: %.2f"), mFPSAverage );

            cString Resolution;
            {
                int fw = (int)g_RTSizeSubRTSize[0];
                int fh = (int)g_RTSizeSubRTSize[1];
                int bw = (int)g_RTSizeSubRTSize[2];
                int bh = (int)g_RTSizeSubRTSize[3];

                Resolution  = cStringFormat( _L("FullRes [%3dx%3d], BlurRes [%3dx%3d]  "), fw, fh, bw, bh );
            }
            if( mAutoBenchmark.IsActive() )
            {
                FPSData     += _L(" BENCHMARK ACTIVE: ");
                FPSData     += (mAutoBenchmark.RemainingFramesToTestBaseline > 0)?(_L(" profiling baseline")):(_L(" profiling the effect"));
            }

            mFPSTextInfo->SetText( FPSData );
            mOtherTextInfo->SetText( Resolution );

            if( !mAutoBenchmark.IsActive() && (mAutoBenchmark.ModeToTest != SampleSettings::ModeOff) )
            {
                cString text = cStringFormat( _L("Average cost per frame for mode %s is %.3fms"), c_EffectNames[mAutoBenchmark.ModeToTest].c_str(), mAutoBenchmark.AverageCostTestMode*1000.0f );
                mBenchmarkTextInfo->SetText( text );

                int bw = (int)g_RTSizeSubRTSize[2];
                int bh = (int)g_RTSizeSubRTSize[3];
                text = cStringFormat( _L("(profiling effect done off-screen at %3dx%3d)"), bw, bh );
                mBenchmarkTextInfoLN2->SetText( text );
            }
            else
            {
                mBenchmarkTextInfo->SetText( _L("") );
                mBenchmarkTextInfoLN2->SetText( _L("") );
            }
        }
    }

    if( mAutoBenchmark.IsActive() )
    {
        if( mAutoBenchmark.RemainingFramesToTestBaseline > 0 )
        {
            mAutoBenchmark.RemainingFramesToTestBaseline--;
            
            if( mSettings.Mode != mSettings.ModeOff )
            {
                mSettings.Mode = mSettings.ModeOff;
                UpdateControlsBasedOnSettings();
            }

            if( mAutoBenchmark.RemainingFramesToTestBaseline == 0 )
                mAutoBenchmark.AverageFrameTimeBaseline = mAverageFrameTime / (float)c_InnerLoopsDuringBenchmark;
        }
        else if( mAutoBenchmark.RemainingFramesToTestMode > 0 )
        {
            mAutoBenchmark.RemainingFramesToTestMode--;

            if( mSettings.Mode != mAutoBenchmark.ModeToTest )
            {
                mSettings.Mode = mAutoBenchmark.ModeToTest;
                UpdateControlsBasedOnSettings();
            }

            if( mAutoBenchmark.RemainingFramesToTestMode == 0 )
            {
                mAutoBenchmark.AverageFrameTimeTestMode = mAverageFrameTime / (float)c_InnerLoopsDuringBenchmark;
                mAutoBenchmark.AverageCostTestMode = mAutoBenchmark.AverageFrameTimeTestMode - mAutoBenchmark.AverageFrameTimeBaseline;
            }
        }
        else
        {
            assert( false );
        }

        // finished?
        if( !mAutoBenchmark.IsActive() )
            UpdateControlsBasedOnSettings();
    }
}

void MySample::StartAutoBenchmark()
{
    assert( !mAutoBenchmark.IsActive() );
    if( mSettings.Mode == SampleSettings::ModeOff )
        return;

    mAutoBenchmark.ModeToTest = mSettings.Mode;
    mAutoBenchmark.RemainingFramesToTestBaseline    = (int)(cFPSLogLength * 1.3);
    mAutoBenchmark.RemainingFramesToTestMode        = (int)(cFPSLogLength * 1.3);
}

void MySample::UpdateControlsBasedOnSettings( )
{
    bool guiEnabled = !mAutoBenchmark.IsActive();

    for( int i = 0; i < SampleSettings::NumberOfModes; i++ )
    {
        cString buttonText = cStringFormat( (mSettings.Mode != (SampleSettings::SampleMode)i) ? ( _L("     %s     ") ) : ( _L(">> %s <<") ), c_EffectNames[i].c_str() );
        mButtonModes[i]->SetText( buttonText );
        mButtonModes[i]->SetEnable( guiEnabled );
    }

    mButtonModeRunBenchmark->SetEnable( (!mSettings.ShowGraph) && guiEnabled && (mSettings.Mode != SampleSettings::ModeOff) );

    mButtonShowGraph->SetEnable( guiEnabled );
    mButtonShowGraph->SetText( (mSettings.ShowGraph)?(_L("Hide graph")):(_L("Show graph")) );


    if( !g_CPUT_GL_EGL_COMPUTE_SHADER_SUPPORTED )
    {
        // not available on GLES3.0, needs GLES3.1
        mButtonModes[ SampleSettings::ModeComputeShader ]->SetEnable( false );
    }
}

void MySample::RecreateTextures()
{
    DEBUG_PRINT( _L("SampleFastBlurs: (Re)creating textures and render targets...") );

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////
    // "Loading screen" - should be something more fun but this is good enough for now
    glClearColor(0.0f, 0.1f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    Present();
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////

    SAFE_DELETE( mpSceneRT );
    SAFE_DELETE( mpDownsampledRT0 );
    SAFE_DELETE( mpDownsampledRT1 );

    static int lastLoadedTexture = -1;
    int textureToLoad = 0;
    if( lastLoadedTexture !=(mSettings.SceneContent % _countof(c_TexNames)) )
    {
        lastLoadedTexture = mSettings.SceneContent % _countof(c_TexNames);

        CPUTAssetLibrary *pAssetLibrary = CPUTAssetLibrary::GetAssetLibrary();
        SAFE_RELEASE( mpTestTexture );
        cString texPath = pAssetLibrary->GetSystemDirectoryName() + _L("texture/");
        
        texPath += c_TexNames[ mSettings.SceneContent % _countof(c_TexNames) ];

        mpTestTexture = (CPUTTextureOGL *)pAssetLibrary->GetTexture( texPath, true, c_sRGB );
        if( mpTestTexture == NULL )
        {
            DEBUG_PRINT( _L("ERROR: unable to load %s"), texPath.c_str() );
            abort();
        }
    }

    int width = 0, height = 0;
    mpTestTexture->GetWidthAndHeight(&width, &height);


    GLenum internalFormat = (c_sRGB)?(GL_SRGB8):(GL_RGBA8);
    //GLenum internalFormat = (c_sRGB)?(GL_SRGB8_ALPHA8):(GL_RGB8); // GL_SRGB8_ALPHA8 is the only sRGB format supported on some GLES 3.0!! hmm need to investigate this
    GLenum format = GL_RGBA;

    // Scene RT
    {
        CPUTTextureOGL* pColorTexture = (CPUTTextureOGL*)CPUTTextureOGL::CreateTexture(_L("$SceneRT"), internalFormat, width, height, format, GL_UNSIGNED_BYTE, NULL);
        CPUTTextureOGL* pDepthTexture = NULL;
        mpSceneRT = new SFBFramebufferOGL(pColorTexture, pDepthTexture);
        DEBUG_PRINT( _L("      SceneRT - done") );
        SAFE_RELEASE(pColorTexture);
        SAFE_RELEASE(pDepthTexture);
    }

    const int downsampledRTFactor   = mSettings.BlurBufferScale;
    const int dsWidth               = (width + downsampledRTFactor - 1) / downsampledRTFactor;
    const int dsHeight              = (height + downsampledRTFactor - 1) / downsampledRTFactor;

    // Downsampled RT 0
    {
        CPUTTextureOGL* pColorTexture = (CPUTTextureOGL*)CPUTTextureOGL::CreateTexture(_L("$Scene0RT"), internalFormat, dsWidth, dsHeight, format, GL_UNSIGNED_BYTE, NULL);
        CPUTTextureOGL* pDepthTexture = NULL;
        mpDownsampledRT0 = new SFBFramebufferOGL(pColorTexture, pDepthTexture);
        SAFE_RELEASE(pColorTexture);
        SAFE_RELEASE(pDepthTexture);
    }

    // Downsampled RT 1
    {
        CPUTTextureOGL* pColorTexture = (CPUTTextureOGL*)CPUTTextureOGL::CreateTexture(_L("$Scene1RT"), internalFormat, dsWidth, dsHeight, format, GL_UNSIGNED_BYTE, NULL);
        CPUTTextureOGL* pDepthTexture = NULL;
        mpDownsampledRT1 = new SFBFramebufferOGL(pColorTexture, pDepthTexture);
        SAFE_RELEASE(pColorTexture);
        SAFE_RELEASE(pDepthTexture);
    }
    g_RTSizeSubRTSize[0] = (GLfloat)width;
    g_RTSizeSubRTSize[1] = (GLfloat)height;
    g_RTSizeSubRTSize[2] = (GLfloat)dsWidth;
    g_RTSizeSubRTSize[3] = (GLfloat)dsHeight;   

    mFPSLogLastIndex        = 0;
    mBenchmarkTextInfo->SetText( _L("") );
    mBenchmarkTextInfoLN2->SetText( _L("") );
}

void MySample::Create()
{
	CPUTAssetLibrary *pAssetLibrary = CPUTAssetLibrary::GetAssetLibrary();
    CPUTGuiControllerOGL *pGUI = (CPUTGuiControllerOGL*)CPUTGetGuiController();
    
    //
    // Create controls
    //   
    DEBUG_PRINT( _L("Create controls") );

    pGUI->CreateText( _L("Blur mode:"), ID_IGNORE_CONTROL_ID, ID_MAIN_PANEL );

    for( int i = 0; i < SampleSettings::NumberOfModes; i++ )
    {
        pGUI->CreateButton( _L("Mode"), ID_BUTTON_MODE_BASE+i, ID_MAIN_PANEL, &mButtonModes[i] );
    }

    pGUI->CreateText( _L(" "), ID_IGNORE_CONTROL_ID, ID_MAIN_PANEL );
    pGUI->CreateText( _L("                                                       "), ID_IGNORE_CONTROL_ID, ID_MAIN_PANEL );
    pGUI->CreateText( _L(" "), ID_TECHNIQUE_INFO_TEXT, ID_MAIN_PANEL, &mTextInfo );

    pGUI->CreateText( _L("Scene content: "), ID_IGNORE_CONTROL_ID, ID_MAIN_PANEL );
    pGUI->CreateDropdown( _L(""), ID_DROPDOWN_SCENE, ID_MAIN_PANEL, &mSceneDropdown );
    mSceneDropdown->AddSelectionItem( _L(" Screenshot 1 (1280x720)"), false );       
    mSceneDropdown->AddSelectionItem( _L(" Screenshot 2 (1600x900)"), false );  
    mSceneDropdown->AddSelectionItem( _L(" Screenshot 3 (1920x1080)"), false );
    mSceneDropdown->AddSelectionItem( _L(" Screenshot 4 (2560x1600)"), false );
    mSceneDropdown->AddSelectionItem( _L(" Animated shapes (1280x720)"), true );    mSettings.SceneContent = 4;
    mSceneDropdown->AddSelectionItem( _L(" Animated shapes (1600x900)"), false );   
    mSceneDropdown->AddSelectionItem( _L(" Animated shapes (1920x1080)"), false );
    mSceneDropdown->AddSelectionItem( _L(" Animated shapes (2560x1600)"), false );

    pGUI->CreateText( _L("Blur working texture: "), ID_IGNORE_CONTROL_ID, ID_MAIN_PANEL, &mBlurResolutionTextInfo );
    pGUI->CreateDropdown( _L(""), ID_DROPDOWN_BLUR_RESOLUTION, ID_MAIN_PANEL, &mBlurResolutionDropdown );
    mBlurResolutionDropdown->AddSelectionItem( _L(" Full resolution"), false );
    mBlurResolutionDropdown->AddSelectionItem( _L(" 1/2 x 1/2 res"), true );        mSettings.BlurBufferScale = 2; // BlurBufferScale
    mBlurResolutionDropdown->AddSelectionItem( _L(" 1/4 x 1/4 res"), false );

    pGUI->CreateText( _L(" "), ID_IGNORE_CONTROL_ID, ID_MAIN_PANEL );
    pGUI->CreateText( _L("Blur kernel size: "), ID_IGNORE_CONTROL_ID, ID_MAIN_PANEL );
    pGUI->CreateDropdown( _L(""), ID_DROPDOWN_KERNEL_SIZE_PRESET, ID_MAIN_PANEL, &mBlurKernelSizePreset );
    mBlurKernelSizePreset->AddSelectionItem( _L(" 7x7"),            false );
    mBlurKernelSizePreset->AddSelectionItem( _L(" 15x15"),          false );
    mBlurKernelSizePreset->AddSelectionItem( _L(" 23x23"),          false );                 
    mBlurKernelSizePreset->AddSelectionItem( _L(" 35x35"),          true );               mSettings.BlurKernelSizePreset = SampleSettings::KernelSizeLarge;
    mBlurKernelSizePreset->AddSelectionItem( _L(" 63x63"),          false );
    mBlurKernelSizePreset->AddSelectionItem( _L(" 127x127"),        false );

    pGUI->CreateText( _L(" "), ID_IGNORE_CONTROL_ID, ID_MAIN_PANEL );

    pGUI->CreateButton( _L("Show graph"),     ID_BUTTON_MODE_SHOW_GRAPH, ID_MAIN_PANEL, &mButtonShowGraph );

    pGUI->CreateText( _L(" "), ID_IGNORE_CONTROL_ID, ID_MAIN_PANEL );
    pGUI->CreateText( _L(" "), ID_IGNORE_CONTROL_ID, ID_MAIN_PANEL );

    pGUI->CreateButton( _L("Run Benchmark"),     ID_BUTTON_MODE_RUN_BENCHMARK, ID_MAIN_PANEL, &mButtonModeRunBenchmark );
        

    pGUI->CreateText( _L("FPS: "), ID_FPS_INFO_TEXT, ID_MAIN_PANEL, &mFPSTextInfo );
    mFPSTextInfo->SetAutoArranged( false );
    mFPSTextInfo->SetPosition( 10, 10 );
    
    pGUI->CreateText( _L("FPS: "), ID_OTHER_INFO_TEXT, ID_MAIN_PANEL, &mOtherTextInfo );
    mOtherTextInfo->SetAutoArranged( false );
    mOtherTextInfo->SetPosition( 10, 30 );


    pGUI->CreateText( _L("FPS: "), ID_BENCHMARK_TEST_INFO, ID_MAIN_PANEL, &mBenchmarkTextInfo );
    mBenchmarkTextInfo->SetAutoArranged( false );
    mBenchmarkTextInfo->SetPosition( 10, 50 );

    pGUI->CreateText( _L("FPS: "), ID_BENCHMARK_TEST_INFO, ID_MAIN_PANEL, &mBenchmarkTextInfoLN2 );
    mBenchmarkTextInfoLN2->SetAutoArranged( false );
    mBenchmarkTextInfoLN2->SetPosition( 10, 66 );

    DEBUG_PRINT( _L("Load texture") );

    mpTestTexture = NULL;
  
    int width, height;
    mpWindow->GetClientDimensions(&width, &height);

    // Resize window
    ResizeWindow(width, height);

    CreateFullscreenPassGeometry();
    CreateAnimatedTrianglesGeometry();
    CreateSamplers();

    RecreateTextures();
    RecreateShaders();

    mSettings.ShowGraph = false;

    mSettings.Mode = SampleSettings::ModeGaussian;

    UpdateControlsBasedOnSettings();

#ifdef CPUT_FOR_OGLES
    eglSwapInterval(display, 0);
#endif
}

void MySample::Shutdown()
{
    glswShutdown();
    CPUT_OGL::Shutdown();
}

// Handle resize events
//-----------------------------------------------------------------------------
void MySample::ResizeWindow(UINT width, UINT height)
{
    CPUT_OGL::ResizeWindow( width, height );

    g_FinalScreenSize[0] = (GLfloat)width;
    g_FinalScreenSize[1] = (GLfloat)height; 
}

void MySample::HandleCallbackEvent( CPUTEventID Event, CPUTControlID ControlID, CPUTControl *pControl ) 
{
    if( 
        Event == CPUT_EVENT_CLICK
//        Event == CPUT_EVENT_DOWN  // <- old workaround for Android, seems that it's not needed anymore
        )
    {
        if( (ControlID >= ID_BUTTON_MODE_BASE) && (ControlID < ID_BUTTON_MODE_BASE+SampleSettings::NumberOfModes) )
        {
            SampleSettings::SampleMode newMode = (SampleSettings::SampleMode)(ControlID-ID_BUTTON_MODE_BASE);

#ifdef CPUT_OS_WINDOWS
            static int repeatClickGauss = 0;
            if( (newMode == mSettings.Mode) && (newMode == SampleSettings::ModeGaussian) )
                repeatClickGauss++;
            else
                repeatClickGauss = 0;

            if( repeatClickGauss > 2 )
            {
                repeatClickGauss = -1000000; // prevent recursive calls to this from MessageBox-s own message loop, below
                MessageBoxA( NULL, g_LastGaussKernelShader.c_str(), "Currently used Gauss shader (use Ctrl+C to copy):", MB_OK );
            }
#endif

            mSettings.Mode = newMode;
        }
        else
        {
            switch( ControlID )
            {
            case( ID_BUTTON_MODE_RUN_BENCHMARK ): StartAutoBenchmark(); break;
            case( ID_BUTTON_MODE_SHOW_GRAPH ): mSettings.ShowGraph = !mSettings.ShowGraph; break;
            case( ID_DROPDOWN_BLUR_RESOLUTION ):
                {
                    int newBlurBufferScale = 1;
                    switch( mBlurResolutionDropdown->GetSelectedItemZeroBasedIndex() )
                    {
                    case( 0 ):  newBlurBufferScale =  1; break;
                    case( 1 ):  newBlurBufferScale =  2; break;
                    case( 2 ):  newBlurBufferScale =  4; break;
                    default:    newBlurBufferScale =  1; break;
                    }
        
                    if( newBlurBufferScale != mSettings.BlurBufferScale )
                    {
                        mSettings.BlurBufferScale = newBlurBufferScale;
                        RecreateTextures();
                        RecreateShaders();
                    }
                } break;
            case( ID_DROPDOWN_SCENE ):
                {
                    int newSceneContent = mSceneDropdown->GetSelectedItemZeroBasedIndex();
                    if( newSceneContent != mSettings.SceneContent )
                    {
                        mSettings.SceneContent = newSceneContent;
                        RecreateTextures();
                        RecreateShaders();
                    }
                    
                } break;
            case( ID_DROPDOWN_KERNEL_SIZE_PRESET ):
                {
                    int newKernelSize = mBlurKernelSizePreset->GetSelectedItemZeroBasedIndex();
                    if( newKernelSize != mSettings.BlurKernelSizePreset )
                    {
                        mSettings.BlurKernelSizePreset = (SampleSettings::KernelSize)newKernelSize;
                        //RecreateTextures(); // no need to reload textures
                        RecreateShaders();
                    }

                } break;

            default:
                break;
            }
        }

        UpdateControlsBasedOnSettings( );
    }
}

void MySample::DrawAnimatedTriangles( double deltaSeconds, bool messWithTheGraph )
{
    for( int i = 0; i < g_ATCount * 3; i++ )
    {
        ATVertex & v = g_ATVerts[i];
        if( (i % 3) == 1 )
        {
            ATVertex & vL = g_ATVerts[i-1];
            ATVertex & vR = g_ATVerts[i+1];

            float2 diff = vR.Position - vL.Position;
            float2 diff2 = float2( diff.y, diff.x );

            v.Position = vL.Position + diff * v.ABCD.x + diff2 * v.ABCD.y;
        }
        else
        {
            const float speed = (float)deltaSeconds * 0.05f;

            v.Position.x += v.ABCD.z * speed;
            if( v.Position.x > 1.0f || v.Position.x < -1.0f )
            {
                v.ABCD.z = -v.ABCD.z;
                v.Position.x += v.ABCD.z * speed;
            }
            v.Position.y += v.ABCD.w * speed;
            if( v.Position.y > 1.0f || v.Position.y < -1.0f )
            {
                v.ABCD.w = -v.ABCD.w;
                v.Position.y += v.ABCD.w * speed;
            }
        }
    }

    // gray out the background
    g_ATVerts[ 0 ] = ATVertex( float2( -3.0f, -3.0f ), (0x80U<<24) | (0x00U<<16) | (0x00U<<8) | (0x00U<<0), float4( 0.0f, 0.0f, 0.0f, 0.0f ) );
    g_ATVerts[ 1 ] = ATVertex( float2(  3.0f, -3.0f ), (0x80U<<24) | (0x00U<<16) | (0x00U<<8) | (0x00U<<0), float4( 0.0f, 0.0f, 0.0f, 0.0f ) );
    g_ATVerts[ 2 ] = ATVertex( float2(  0.0f,  3.0f ), (0x80U<<24) | (0x00U<<16) | (0x00U<<8) | (0x00U<<0), float4( 0.0f, 0.0f, 0.0f, 0.0f ) );
    if( messWithTheGraph )
    {
        g_ATVerts[ 0 ].Color = 0;
        g_ATVerts[ 1 ].Color = 0;
        g_ATVerts[ 2 ].Color = 0;
    }

    GL_CHECK( glUseProgram( g_ProgramHandleAnimatedTriangles ) );
    GL_CHECK( glBindVertexArray(g_ATVAO) );
    GL_CHECK( glBindBuffer(GL_ARRAY_BUFFER, g_ATVAOVertices) );
    GL_CHECK( glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(g_ATVerts), g_ATVerts) );
    GL_CHECK( glDrawElements(GL_TRIANGLES, g_ATIndexCount, GL_UNSIGNED_INT, 0) );
    GL_CHECK( glUseProgram(0) );
    GL_CHECK( glBindBuffer(GL_ARRAY_BUFFER, 0) );
    GL_CHECK( glBindVertexArray(0) );
}

void MySample::PassDownsample( SFBFramebufferOGL * outputRT, SFBFramebufferOGL * sceneColor )
{
    outputRT->SetActive();

    if( mSettings.BlurBufferScale == 2 )
    {
        glUseProgram( g_ProgramHandleDownsample2x2 );
        GL_CHECK( glUniform1i( g_Uniforms.DownSample2x2_Tex0, 0 ) );
    }
    else if( mSettings.BlurBufferScale == 4 )
    {
        glUseProgram( g_ProgramHandleDownsample4x4 );
        GL_CHECK( glUniform1i( g_Uniforms.DownSample4x4_Tex0, 0 ) );
    }
    else
    {
        assert( false );
        outputRT->UnSetActive();
        return;
    }

    //glUniform1f(g_Uniforms.TessLevelInner, 5.0f);

    GL_CHECK( glActiveTexture( GL_TEXTURE0 ) );
    GL_CHECK( glBindTexture(GL_TEXTURE_2D, sceneColor->GetColorTexture()->GetTexture() ) );
    GL_CHECK( glBindSampler( 0, g_SamplerLinear ) );


    GL_CHECK(glBindVertexArray(g_FSVAO));
    glDrawElements(GL_TRIANGLES, g_FSIndexCount, GL_UNSIGNED_INT, 0);
    glUseProgram(0);

    outputRT->UnSetActive();
}

void MySample::PassGaussianBlur( SFBFramebufferOGL * outputRT, SFBFramebufferOGL * inputBlur, bool horizontal )
{
    outputRT->SetActive();

    glUseProgram( (horizontal)?(g_ProgramHandleGaussianH):(g_ProgramHandleGaussianV) );

    GL_CHECK( glActiveTexture( GL_TEXTURE0+0 ) );
    GL_CHECK( glBindTexture(GL_TEXTURE_2D, inputBlur->GetColorTexture()->GetTexture() ) );
    GL_CHECK( glBindSampler( 0, g_SamplerLinear ) );

    GL_CHECK( glUniform1i( (horizontal)?(g_Uniforms.GaussianH_Tex0):(g_Uniforms.GaussianV_Tex0), 0 ) );

    GL_CHECK( glUniform4f( (horizontal)?(g_Uniforms.GaussianH_RTPixelSizePixelSizeHalf):(g_Uniforms.GaussianV_RTPixelSizePixelSizeHalf), 1.0f / g_RTSizeSubRTSize[2], 1.0f / g_RTSizeSubRTSize[3], 0.5f / g_RTSizeSubRTSize[2], 0.5f / g_RTSizeSubRTSize[3] ) );

    GL_CHECK(glBindVertexArray(g_FSVAO));
    glDrawElements(GL_TRIANGLES, g_FSIndexCount, GL_UNSIGNED_INT, 0);
    glUseProgram(0);

    outputRT->UnSetActive();
}

void MySample::PassKawaseBlur( SFBFramebufferOGL * outputRT, SFBFramebufferOGL * inputBlur, int iteration )
{
    outputRT->SetActive();

    glUseProgram( g_ProgramHandleKawaseBlurPass );

    GL_CHECK( glActiveTexture( GL_TEXTURE0+0 ) );
    GL_CHECK( glBindTexture(GL_TEXTURE_2D, inputBlur->GetColorTexture()->GetTexture() ) );
    GL_CHECK( glBindSampler( 0, g_SamplerLinear ) );

    GL_CHECK( glUniform1i( g_Uniforms.KawaseBlurPass_Tex0, 0 ) );
    GL_CHECK( glUniform4f( g_Uniforms.KawaseBlurPass_xyPixelSize_zIteration_wDummy, 1.0f / g_RTSizeSubRTSize[2], 1.0f / g_RTSizeSubRTSize[3], (float)iteration, 0.0f ) );

    GL_CHECK(glBindVertexArray(g_FSVAO));
    glDrawElements(GL_TRIANGLES, g_FSIndexCount, GL_UNSIGNED_INT, 0);
    glUseProgram(0);

    outputRT->UnSetActive();
}

#ifndef GL_SHADER_IMAGE_ACCESS_BARRIER_BIT
    #define GL_SHADER_IMAGE_ACCESS_BARRIER_BIT 0x00000020
#endif

void MySample::PassComputeShaderBlur( SFBFramebufferOGL * tex0, SFBFramebufferOGL * tex1 )
{
    if( !g_CPUT_GL_EGL_COMPUTE_SHADER_SUPPORTED )
        return;

    int width   = (int)g_RTSizeSubRTSize[2];
    int height  = (int)g_RTSizeSubRTSize[3];

    // Horizontal
    {
        glUseProgram( g_ProgramHandleComputeShaderBlurH );
        GL_CHECK( glBindImageTexture( 0, tex0->GetColorTexture()->GetTexture(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8 ) );
        GL_CHECK( glBindImageTexture( 1, tex1->GetColorTexture()->GetTexture(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8 ) );
        glDispatchCompute( (height+CS_THREAD_GROUP_SIZE-1)/CS_THREAD_GROUP_SIZE, 1, 1);

        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
#ifndef CPUT_FOR_OGLES
        if( g_CPUT_GL_IS_AMD_VENDOR )
        {
             glFinish();
        }
#endif

        GL_CHECK( glBindImageTexture( 1, tex0->GetColorTexture()->GetTexture(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8 ) );
        GL_CHECK( glBindImageTexture( 0, tex1->GetColorTexture()->GetTexture(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8 ) );
        glDispatchCompute( (height+CS_THREAD_GROUP_SIZE-1)/CS_THREAD_GROUP_SIZE, 1, 1);

        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
#ifndef CPUT_FOR_OGLES
        if( g_CPUT_GL_IS_AMD_VENDOR )
            glFinish();
#endif
    }
    
    // Vertical
    {
        glUseProgram( g_ProgramHandleComputeShaderBlurV );

        GL_CHECK( glBindImageTexture( 0, tex0->GetColorTexture()->GetTexture(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8 ) );
        GL_CHECK( glBindImageTexture( 1, tex1->GetColorTexture()->GetTexture(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8 ) );
        glDispatchCompute( (width+CS_THREAD_GROUP_SIZE-1)/CS_THREAD_GROUP_SIZE, 1, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
#ifndef CPUT_FOR_OGLES
        if( g_CPUT_GL_IS_AMD_VENDOR )
            glFinish();
#endif

        GL_CHECK( glBindImageTexture( 1, tex0->GetColorTexture()->GetTexture(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8 ) );
        GL_CHECK( glBindImageTexture( 0, tex1->GetColorTexture()->GetTexture(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8 ) );
        glDispatchCompute( (width+CS_THREAD_GROUP_SIZE-1)/CS_THREAD_GROUP_SIZE, 1, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
#ifndef CPUT_FOR_OGLES
        if( g_CPUT_GL_IS_AMD_VENDOR )
            glFinish();
#endif
    }

    GL_CHECK( glBindImageTexture( 0, 0, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R8 ) );
    GL_CHECK( glBindImageTexture( 1, 0, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R8 ) );
    glUseProgram(0);
}

void MySample::PassApply( SFBFramebufferOGL * inputBlur )
{
    glUseProgram( g_ProgramHandleApply );

    GL_CHECK( glActiveTexture( GL_TEXTURE0+0 ) );
    GL_CHECK( glBindTexture(GL_TEXTURE_2D, inputBlur->GetColorTexture()->GetTexture() ) );
    GL_CHECK( glBindSampler( 0, g_SamplerLinear ) );

    GL_CHECK( glUniform1i( g_Uniforms.Apply_Tex0, 0 ) );
    
    float ApplyParams[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    if( mSettings.SceneContent >= _countof(c_TexNames) )
    {
        ApplyParams[0] = 10.0f;
        ApplyParams[1] = 0.60f;

        // just for nicer visuals with animated triangles, no other practical reason
        if( mSettings.Mode > SampleSettings::ModeJustDownsample )
            ApplyParams[1] -= ((float)(int)mSettings.BlurKernelSizePreset) * 0.04f;
    }
    GL_CHECK( glUniform4fv( g_Uniforms.Apply_Params, 1, ApplyParams ) );
    GL_CHECK( glUniform4f( g_Uniforms.Apply_FinalPixelSize, 1.0f / g_FinalScreenSize[0], 1.0f / g_FinalScreenSize[1], 0.0f, 0.0f ) );

    GL_CHECK(glBindVertexArray(g_FSVAO));
    glDrawElements(GL_TRIANGLES, g_FSIndexCount, GL_UNSIGNED_INT, 0);
    glUseProgram(0);
}

void MySample::PassDebugGraphDrawInput( )
{
    glUseProgram( g_ProgramHandleDebugGraphDrawInput );
    GL_CHECK(glBindVertexArray(g_FSVAO));
    glDrawElements(GL_TRIANGLES, g_FSIndexCount, GL_UNSIGNED_INT, 0);
    glUseProgram(0);
}

void MySample::PassDebugGraphDraw( SFBFramebufferOGL * inputBlur )
{
    glUseProgram( g_ProgramHandleDebugGraphDraw );

    GL_CHECK( glActiveTexture( GL_TEXTURE0+0 ) );
    GL_CHECK( glBindTexture( GL_TEXTURE_2D, inputBlur->GetColorTexture()->GetTexture() ) );
    GL_CHECK( glBindSampler( 0, g_SamplerPoint ) );
    GL_CHECK( glUniform1i( g_Uniforms.DebugGraphDraw_Tex0, 0 ) );

    GL_CHECK( glActiveTexture( GL_TEXTURE0+1 ) );
    GL_CHECK( glBindTexture( GL_TEXTURE_2D, inputBlur->GetColorTexture()->GetTexture() ) );
    GL_CHECK( glBindSampler( 1, g_SamplerLinear ) );
    GL_CHECK( glUniform1i( g_Uniforms.DebugGraphDraw_Tex1, 1 ) );

    GL_CHECK(glBindVertexArray(g_FSVAO));
    glDrawElements(GL_TRIANGLES, g_FSIndexCount, GL_UNSIGNED_INT, 0);
    glUseProgram(0);

}

void MySample::Render(double deltaSeconds)
{
    static int frame = 0;

    assert( !c_sRGB ); // SRGB code paths below not functional

    // Intel OpenGL / OpenGLES driver defaults to different values for some reason or CPUT isn't setting this correctly everywhere so hack it in here. Sorry.
    if( !g_CPUT_GL_IS_INTEL_VENDOR )
        glDisable( GL_FRAMEBUFFER_SRGB );

    glDisable( GL_BLEND );

    // some common states
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    const int testLoops = (mAutoBenchmark.IsActive())?(c_InnerLoopsDuringBenchmark):(1);

    GLint   prevViewport[4]; // only needed for benchmarking
    // make benchmark as much resolution independent as possible
    if( mAutoBenchmark.IsActive() )
        glGetIntegerv( GL_VIEWPORT, prevViewport );

    for( int lp = 0; lp < testLoops; lp++ )
    {
        // Draw scene (dummy scene)
        {
            mpSceneRT->SetActive();

            // Clear
            {
                frame = (frame+1) % 512;

                if( frame % 6 < 3 )
                    GL_CHECK(glClearColor ( 0.0, 0.5, 1, 1 ));
                else
                    GL_CHECK(glClearColor ( 1.0, 0.5, 0, 1 ));

                GL_CHECK(glClearDepthf(0.0f));
                GL_CHECK(glClear ( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT ));
            }

            // Draw loaded texture (dummy scene)
            {
                glUseProgram(g_ProgramHandleFullscreenQuad);

                GL_CHECK( glActiveTexture( GL_TEXTURE0 ) );
                GL_CHECK( glBindTexture(GL_TEXTURE_2D, mpTestTexture->GetTexture() ) );
                GL_CHECK( glBindSampler( 0, g_SamplerLinear ) );
                GL_CHECK( glUniform1i( g_Uniforms.DrawFullscreenQuad_Tex0, 0 ) );

                GL_CHECK(glBindVertexArray(g_FSVAO));
                glDrawElements(GL_TRIANGLES, g_FSIndexCount, GL_UNSIGNED_INT, 0);
                glUseProgram(0);
            }

            // Draw dummy source white point surrounded by 'enough' black pixels (where enough is bigger or equal to the biggest blur combined effective kernel)
            // (so we can use it as a source for the graph drawing)
            if( mSettings.ShowGraph )
            {
                PassDebugGraphDrawInput( );
            }

            // animated triangles?
            if( mSettings.SceneContent >= _countof(c_TexNames) )
            {
                glEnable( GL_BLEND );
                DrawAnimatedTriangles( deltaSeconds, mSettings.ShowGraph );
                glDisable( GL_BLEND );
            }

            mpSceneRT->UnSetActive();
        }

        SFBFramebufferOGL * lastBlurRT = mpSceneRT;

        if( mSettings.Mode != SampleSettings::ModeOff )
        {
            if( mSettings.BlurBufferScale > 1 )
            {
                lastBlurRT = mpDownsampledRT0;
                PassDownsample( lastBlurRT, mpSceneRT );                
            }

            if( mSettings.Mode == SampleSettings::ModeGaussian )
            {
                int i = 0;
                {
                    SFBFramebufferOGL * srcBlurRT = lastBlurRT;
                    SFBFramebufferOGL * dstBlurRT = ( (i % 2) == 0 )?( mpDownsampledRT1 ) : ( mpDownsampledRT0 );

                    PassGaussianBlur( dstBlurRT, srcBlurRT, true );
                    lastBlurRT = dstBlurRT;
                }
                i = 1;
                {
                    SFBFramebufferOGL * srcBlurRT = lastBlurRT;
                    SFBFramebufferOGL * dstBlurRT = ( (i % 2) == 0 )?( mpDownsampledRT1 ) : ( mpDownsampledRT0 );

                    PassGaussianBlur( dstBlurRT, srcBlurRT, false );
                    lastBlurRT = dstBlurRT;
                }
            }

            if( mSettings.Mode == SampleSettings::ModeKawase )
            {
                int numberOfPasses = mSettings.KawaseBlurPasses;
                const int * shaderKernel = mSettings.KawaseBlurPassKernelSize;

                for( int i = 0; i < numberOfPasses; i++ )
                {
                    SFBFramebufferOGL * srcBlurRT = lastBlurRT;
                    SFBFramebufferOGL * dstBlurRT = ( (i % 2) == 0 )?( mpDownsampledRT1 ) : ( mpDownsampledRT0 );

                    PassKawaseBlur( dstBlurRT, srcBlurRT, shaderKernel[i] );
                    lastBlurRT = dstBlurRT;
                }
            }

            if( mSettings.Mode == SampleSettings::ModeComputeShader )
            {
                if( mSettings.BlurBufferScale == 1 )
                {
                    // There is an additional fullscreen copy here in the case of the fullscreen res buffer that is not needed but fixing it would make things much more complicated.
                    // Since compute shader kernel has very high initial base cost, the proportional impact is lower than for the effects above.
                    // However, it needs fixing (will require a special first pass that copies from the main texture) but it also reflects the difficulty a developer will experience
                    // so it's not entirely unfair towards compute shader implementation.
                    
                    // this will just do a copy
                    mSettings.BlurBufferScale = 2;
                    lastBlurRT = mpDownsampledRT0;
                    PassDownsample( mpDownsampledRT0, mpSceneRT );
                    mSettings.BlurBufferScale = 1;
                }

                PassComputeShaderBlur( mpDownsampledRT0, mpDownsampledRT1 );
            }

            // make benchmark as much resolution independent as possible
            if( mAutoBenchmark.IsActive() )
                glViewport( 0, 0, 1024, 720 );

            //glEnable( GL_BLEND );
            PassApply( lastBlurRT );
            //glDisable( GL_BLEND );
        }
        else
        {
            // make benchmark as much resolution independent as possible
            if( mAutoBenchmark.IsActive() )
                glViewport( 0, 0, 1024, 720 );

            PassApply( mpSceneRT );
        }
        // Draw blur effect graph
        if( mSettings.ShowGraph )
        {
            PassDebugGraphDraw( lastBlurRT );
        }
    }

    // restore to pre-benchmark viewport
    if( mAutoBenchmark.IsActive() )
        glViewport( prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3] );


    GL_CHECK(glBindVertexArray(0));
    GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, 0));

    glEnable( GL_BLEND );

    // GUI!!
    CPUTMaterialOGL::ResetStateTracking();
    CPUTDrawGUI();

    Present();
}
