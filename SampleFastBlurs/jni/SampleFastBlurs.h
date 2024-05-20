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

#ifndef __CPUT_SAMPLEFASTBLURS_H__
#define __CPUT_SAMPLEFASTBLURS_H__

#include <stdio.h>
#include <time.h>
#include "CPUTScene.h"
#include "CPUT_OGL.h"
#include "CPUTSpriteOGL.h"
#include "CPUTTexture.h"

class CPUTTextureOGL;
class SFBFramebufferOGL;

struct ShaderUniforms
{
    GLuint DrawFullscreenQuad_Tex0;

    GLuint DownSample2x2_Tex0;

    GLuint DownSample4x4_Tex0;

    GLuint KawaseBlurPass_Tex0;
    GLuint KawaseBlurPass_xyPixelSize_zIteration_wDummy;

    GLuint GaussianH_Tex0;
    GLuint GaussianV_Tex0;

    GLuint GaussianH_RTPixelSizePixelSizeHalf;
    GLuint GaussianV_RTPixelSizePixelSizeHalf;

    GLuint Apply_Tex0;
    GLuint Apply_Params;
    GLuint Apply_FinalPixelSize;

    GLuint DebugGraphDraw_Tex0;
    GLuint DebugGraphDraw_Tex1;
    GLuint DebugGraphDraw_ScreenSize;
};

struct ATVertex
{
    float2          Position;
    unsigned int    Color;

    float4          ABCD;

    ATVertex() {}

    ATVertex( const float2 & pos, unsigned int col, const float4 & abcd ) : Position(pos), Color(col), ABCD( abcd ) { }
};

struct SampleSettings
{
    enum SampleMode
    {
        ModeOff,
        ModeJustDownsample,
        ModeGaussian,
        ModeKawase,
        ModeComputeShader,

        NumberOfModes
    };

    enum KernelSize
    {
        KernelSizeVerySmall     = 0,
        KernelSizeSmall         = 1,
        KernelSizeMedium        = 2,
        KernelSizeLarge         = 3,
        KernelSizeVeryLarge     = 4,
        KernelSizeHumonguous    = 5,

        NumberOfKernelSizes
    };

    SampleMode          Mode;
    KernelSize          BlurKernelSizePreset;
    int                 SceneContent;
    int                 BlurBufferScale;
    bool                ShowGraph;

    // derived settings (from above)
    int                 GaussKernelSize;
    static const int    KawaseBlurPassesMax = 16;
    int                 KawaseBlurPasses;
    int                 KawaseBlurPassKernelSize[KawaseBlurPassesMax];
};

struct AutoBenchmarkContext
{
    SampleSettings::SampleMode  ModeToTest;

    int                         RemainingFramesToTestBaseline;
    int                         RemainingFramesToTestMode;
    float                       AverageFrameTimeBaseline;
    float                       AverageFrameTimeTestMode;
    float                       AverageCostTestMode;

    AutoBenchmarkContext()      { ModeToTest = SampleSettings::ModeOff; RemainingFramesToTestBaseline = 0; RemainingFramesToTestMode = 0; AverageFrameTimeBaseline = 0.0f; AverageFrameTimeTestMode = 0.0f; AverageCostTestMode = 0.0f; }

    bool                        IsActive()          { return (RemainingFramesToTestBaseline) > 0 || (RemainingFramesToTestMode > 0); }
};

class MySample : public CPUT_OGL
{
private:
    float                   mfElapsedTime;
    CPUTAssetSet *          mpAssetSet;

    CPUTTextureOGL *        mpTestTexture;

    SFBFramebufferOGL *     mpSceneRT;
    SFBFramebufferOGL *     mpDownsampledRT0;
    SFBFramebufferOGL *     mpDownsampledRT1;

    CPUTTimer *             mpFPSTimer;
    static const int        cFPSLogLength = 96;
    float                   mFPSLog[cFPSLogLength];
    int                     mFPSLogLastIndex;
    float                   mFPSAverage;
    float                   mAverageFrameTime;
    CPUTText *              mFPSTextInfo;
    CPUTText *              mOtherTextInfo;
    CPUTText *              mBenchmarkTextInfo;
    CPUTText *              mBenchmarkTextInfoLN2;
    CPUTText *              mBlurResolutionTextInfo;
    CPUTDropdown *          mSceneDropdown;
    CPUTDropdown *          mBlurResolutionDropdown;
    CPUTDropdown *          mBlurKernelSizePreset;
    SampleSettings          mSettings;

    CPUTButton *            mButtonModes[SampleSettings::NumberOfModes];

    CPUTButton *            mButtonModeRunBenchmark;
    CPUTButton *            mButtonShowGraph;
    CPUTText *              mTextInfo;

    AutoBenchmarkContext    mAutoBenchmark;

public:
    MySample();
    virtual ~MySample();

    virtual CPUTEventHandledCode HandleKeyboardEvent(CPUTKey key, CPUTKeyState state)
    {
        return CPUT_EVENT_UNHANDLED;
    }

    virtual CPUTEventHandledCode HandleMouseEvent(int x, int y, int wheel, CPUTMouseState state, CPUTEventID eventID)
    {
        return CPUT_EVENT_UNHANDLED;
    }

    virtual void Create();
    virtual void Render(double deltaSeconds);
    virtual void Update(double deltaSeconds);
    virtual void ResizeWindow(UINT width, UINT height);
    virtual void Shutdown();

private:
    void            DrawAnimatedTriangles( double deltaSeconds, bool messWithTheGraph );

    void            PassDownsample( SFBFramebufferOGL * outputRT, SFBFramebufferOGL * sceneColor );
    void            PassKawaseBlur( SFBFramebufferOGL * outputRT, SFBFramebufferOGL * inputBlur, int iteration );
    void            PassGaussianBlur( SFBFramebufferOGL * outputRT, SFBFramebufferOGL * inputBlur, bool horizontal );
    void            PassComputeShaderBlur( SFBFramebufferOGL * tex0, SFBFramebufferOGL * tex1 );
    void            PassApply( SFBFramebufferOGL * inputBlur );

    void            PassDebugGraphDrawInput( ); //SFBFramebufferOGL * outputRT );
    void            PassDebugGraphDraw( SFBFramebufferOGL * inputBlur );

    virtual void    HandleCallbackEvent( CPUTEventID Event, CPUTControlID ControlID, CPUTControl *pControl );
    void            UpdateControlsBasedOnSettings( );

    void            RecreateShaders();
    void            RecreateTextures();

    void            StartAutoBenchmark( );

};

extern GLuint genComputeProg(GLuint texHandle);
extern GLuint genRenderProg(GLuint texHandle,	GLuint &vertArray);
extern GLuint genTexture();
extern void checkErrors(std::string desc);


#endif // __CPUT_SAMPLEFASTBLURS_H__
