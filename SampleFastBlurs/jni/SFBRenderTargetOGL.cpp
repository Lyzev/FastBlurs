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

#include "SFBRenderTargetOGL.h"
#include "CPUTTextureOGL.h"

SFBFramebufferOGL::SFBFramebufferOGL() 
        : mFramebuffer(0), mpColor(NULL), mpDepth(NULL), mpStencil(NULL)
{
    memset( mPrevViewport, 0, sizeof(mPrevViewport) );
    mIsActive = false;
}

SFBFramebufferOGL::~SFBFramebufferOGL()
{
    assert( !mIsActive );
    SAFE_RELEASE(mpColor);
    SAFE_RELEASE(mpDepth);
    SAFE_RELEASE(mpStencil);
    if(mFramebuffer == 0)
    {
        glDeleteFramebuffers(1, &mFramebuffer);
    }
}

SFBFramebufferOGL::SFBFramebufferOGL(CPUTTextureOGL *pColor,
        CPUTTextureOGL *pDepth,
        CPUTTextureOGL *pStencil)
{
    memset( mPrevViewport, 0, sizeof(mPrevViewport) );
    mIsActive = false;

    if( (pColor != NULL) && (pDepth != NULL) )
        assert( (pColor->mWidth == pDepth->mWidth) && (pColor->mHeight == pDepth->mHeight) );

    GLuint framebuffer = 0;
    GL_CHECK( glGenFramebuffers(1, &framebuffer) );

    GL_CHECK( glBindFramebuffer(GL_FRAMEBUFFER, framebuffer) );

    if( pColor != NULL )
        GL_CHECK( glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D, pColor->GetTexture(), 0) );
   
    if( pDepth != NULL )
        GL_CHECK( glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D, pDepth->GetTexture(), 0) );

    GLenum pDrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
    GL_CHECK( glDrawBuffers(1, pDrawBuffers); );

    GLuint status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if( status != GL_FRAMEBUFFER_COMPLETE )
    {
        DEBUG_PRINT( _L(" error creating FBO: %d"), status );
    }
    ASSERT(status == GL_FRAMEBUFFER_COMPLETE, _L("Failed creating Framebuffer"));
    GL_CHECK( glBindFramebuffer(GL_FRAMEBUFFER, 0) );

    mFramebuffer = framebuffer;
    mpColor = pColor;
    if(pColor != NULL)
        pColor->AddRef();
    if(pDepth != NULL)
        pDepth->AddRef();
    mpDepth = pDepth;
    if(pStencil != NULL)
        pStencil->AddRef();
    mpStencil = pStencil;

    return;
} 

void SFBFramebufferOGL::SetActive()
{
    assert( !mIsActive );
    mIsActive = true;

    glGetIntegerv( GL_VIEWPORT, mPrevViewport );

    glBindFramebuffer(GL_FRAMEBUFFER, mFramebuffer);
    if( mpColor != NULL )
        glViewport( 0, 0, mpColor->mWidth, mpColor->mHeight );
    else
        if( mpDepth != NULL )
            glViewport( 0, 0, mpDepth->mWidth, mpDepth->mHeight );
        else 
            assert( false );
}

void SFBFramebufferOGL::UnSetActive()
{
    assert( mIsActive );
    mIsActive = false;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport( mPrevViewport[0], mPrevViewport[1], mPrevViewport[2], mPrevViewport[3] );
}

GLuint SFBFramebufferOGL::GetFramebufferName() { return mFramebuffer;}
CPUTTextureOGL* SFBFramebufferOGL::GetColorTexture() { return mpColor;}
CPUTTextureOGL* SFBFramebufferOGL::GetDepthTexture() { return mpDepth;}
