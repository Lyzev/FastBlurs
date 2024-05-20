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
#include "CPUTMaterialDX11.h"
#include "CPUT_DX11.h"
#include "CPUTRenderStateBlockDX11.h"
#include "D3DCompiler.h"
#include "CPUTTextureDX11.h"
#include "CPUTBufferDX11.h"
#include "CPUTVertexShaderDX11.h"
#include "CPUTPixelShaderDX11.h"
#include "CPUTComputeShaderDX11.h"
#include "CPUTGeometryShaderDX11.h"
#include "CPUTDomainShaderDX11.h"
#include "CPUTHullShaderDX11.h"

#define OUTPUT_BINDING_DEBUG_INFO(x)
// #define OUTPUT_BINDING_DEBUG_INFO(x) OutputDebugString(x)

CPUTConfigBlock CPUTMaterial::mGlobalProperties;

// Note: These initial values shouldn't really matter.  We call ResetStateTracking() before we render (and it performs these initializations)
void *CPUTMaterialDX11::mpLastVertexShader   = (void*)-1;
void *CPUTMaterialDX11::mpLastPixelShader    = (void*)-1;
void *CPUTMaterialDX11::mpLastComputeShader  = (void*)-1;
void *CPUTMaterialDX11::mpLastGeometryShader = (void*)-1;
void *CPUTMaterialDX11::mpLastHullShader     = (void*)-1;
void *CPUTMaterialDX11::mpLastDomainShader   = (void*)-1;
void *CPUTMaterialDX11::mpLastVertexShaderViews[CPUT_MATERIAL_MAX_TEXTURE_SLOTS]                     = {0};
void *CPUTMaterialDX11::mpLastPixelShaderViews[CPUT_MATERIAL_MAX_TEXTURE_SLOTS]                      = {0};
void *CPUTMaterialDX11::mpLastComputeShaderViews[CPUT_MATERIAL_MAX_TEXTURE_SLOTS]                    = {0};
void *CPUTMaterialDX11::mpLastGeometryShaderViews[CPUT_MATERIAL_MAX_TEXTURE_SLOTS]                   = {0};
void *CPUTMaterialDX11::mpLastHullShaderViews[CPUT_MATERIAL_MAX_TEXTURE_SLOTS]                       = {0};
void *CPUTMaterialDX11::mpLastDomainShaderViews[CPUT_MATERIAL_MAX_TEXTURE_SLOTS]                     = {0};
void *CPUTMaterialDX11::mpLastVertexShaderConstantBuffers[CPUT_MATERIAL_MAX_CONSTANT_BUFFER_SLOTS]   = {0};
void *CPUTMaterialDX11::mpLastPixelShaderConstantBuffers[CPUT_MATERIAL_MAX_CONSTANT_BUFFER_SLOTS]    = {0};
void *CPUTMaterialDX11::mpLastComputeShaderConstantBuffers[CPUT_MATERIAL_MAX_CONSTANT_BUFFER_SLOTS]  = {0};
void *CPUTMaterialDX11::mpLastGeometryShaderConstantBuffers[CPUT_MATERIAL_MAX_CONSTANT_BUFFER_SLOTS] = {0};
void *CPUTMaterialDX11::mpLastHullShaderConstantBuffers[CPUT_MATERIAL_MAX_CONSTANT_BUFFER_SLOTS]     = {0};
void *CPUTMaterialDX11::mpLastDomainShaderConstantBuffers[CPUT_MATERIAL_MAX_CONSTANT_BUFFER_SLOTS]   = {0};
void *CPUTMaterialDX11::mpLastComputeShaderUAVs[CPUT_MATERIAL_MAX_UAV_SLOTS]                         = {0};
void *CPUTMaterialDX11::mpLastRenderStateBlock  = (void*)-1;

//-----------------------------------------------------------------------------
CPUTShaderParameters::~CPUTShaderParameters()
{
    for(int ii=0; ii<CPUT_MATERIAL_MAX_TEXTURE_SLOTS; ii++)
    {
        SAFE_RELEASE(mppBindViews[ii]);
        SAFE_RELEASE(mpTexture[ii]);
    }
    for(int ii=0; ii<CPUT_MATERIAL_MAX_BUFFER_SLOTS; ii++)
    {
        SAFE_RELEASE(mpBuffer[ii]);
    }
    for(int ii=0; ii<CPUT_MATERIAL_MAX_UAV_SLOTS; ii++)
    {
        SAFE_RELEASE(mppBindUAVs[ii]);
        SAFE_RELEASE(mpUAV[ii]);
    }
    for(int ii=0; ii<CPUT_MATERIAL_MAX_CONSTANT_BUFFER_SLOTS; ii++)
    {
        SAFE_RELEASE(mppBindConstantBuffers[ii]);
        SAFE_RELEASE(mpConstantBuffer[ii]);
    }
    SAFE_DELETE_ARRAY(mpTextureParameterName);
    SAFE_DELETE_ARRAY(mpTextureParameterBindPoint);
    SAFE_DELETE_ARRAY(mpSamplerParameterName);
    SAFE_DELETE_ARRAY(mpSamplerParameterBindPoint);
    SAFE_DELETE_ARRAY(mpBufferParameterName);
    SAFE_DELETE_ARRAY(mpBufferParameterBindPoint);
    SAFE_DELETE_ARRAY(mpUAVParameterName);
    SAFE_DELETE_ARRAY(mpUAVParameterBindPoint);
    SAFE_DELETE_ARRAY(mpConstantBufferParameterName);
    SAFE_DELETE_ARRAY(mpConstantBufferDesc);
    SAFE_DELETE_ARRAY(mpConstantBufferReflection);
    SAFE_DELETE_ARRAY(mpConstantBufferParameterBindPoint)
}

//-----------------------------------------------------------------------------
CPUTMaterialDX11::CPUTMaterialDX11() :
    mpPixelShader(NULL),
    mpComputeShader(NULL),
    mpVertexShader(NULL),
    mpGeometryShader(NULL),
    mpHullShader(NULL),
    mpDomainShader(NULL),
    mpExternalsConstantBuffer(NULL),
    mExternalsConstantBufferDirty(true),
    mpExternalsConstantBufferReflection(NULL),
    mRequiresPerModelPayload(-1),
    mpExternalName(NULL),
    mpExternalValue(NULL),
    mpExternalOffset(NULL),
    mpExternalSize(NULL),
    mpExternalCount(NULL)
{
    // TODO: Is there a better/safer way to initialize this list?
    mpShaderParametersList[0] =  &mPixelShaderParameters,
    mpShaderParametersList[1] =  &mComputeShaderParameters,
    mpShaderParametersList[2] =  &mVertexShaderParameters,
    mpShaderParametersList[3] =  &mGeometryShaderParameters,
    mpShaderParametersList[4] =  &mHullShaderParameters,
    mpShaderParametersList[5] =  &mDomainShaderParameters,
    mpShaderParametersList[6] =  NULL;
}

//-----------------------------------------------------------------------------
CPUTMaterialDX11::~CPUTMaterialDX11()
{
    SAFE_RELEASE(mpPixelShader);
    SAFE_RELEASE(mpComputeShader);
    SAFE_RELEASE(mpVertexShader);
    SAFE_RELEASE(mpGeometryShader);
    SAFE_RELEASE(mpHullShader);
    SAFE_RELEASE(mpDomainShader);
    SAFE_RELEASE(mpRenderStateBlock);
    SAFE_RELEASE(mpExternalsConstantBuffer);

    if( mpExternalValue )
    {
        for( int ii=0; ii<mSubMaterialCount; ii++ )
        {
            SAFE_DELETE_ARRAY( mpExternalName[ii] );
            SAFE_DELETE_ARRAY( mpExternalValue[ii] );
            SAFE_DELETE_ARRAY( mpExternalOffset[ii] );
            SAFE_DELETE_ARRAY( mpExternalSize[ii] );
        }
    }
    SAFE_DELETE_ARRAY( mpExternalName );
    SAFE_DELETE_ARRAY( mpExternalValue );
    SAFE_DELETE_ARRAY( mpExternalOffset );
    SAFE_DELETE_ARRAY( mpExternalSize );
    SAFE_DELETE_ARRAY( mpExternalCount );

    // SAFE_DELETE(mpExternalsConstantBufferReflection); // Don't delete.  We don't allocate.
    
    CPUTMaterial::~CPUTMaterial();
}

// **********************************
// **** Set Shader resources if they changed
// **********************************
#define SET_SHADER_RESOURCES( SHADER, SHADER_TYPE ) \
    /* If the shader changed ... */ \
    if( mpLast##SHADER##Shader != mp##SHADER##Shader ) \
    { \
        mpLast##SHADER##Shader = mp##SHADER##Shader; \
        pContext->##SHADER_TYPE##SetShader( mp##SHADER##Shader ? mp##SHADER##Shader->GetNative##SHADER##Shader() : NULL, NULL, 0 ); \
    } \
    /* Spend time checking shader resources only if a shader is bound ... */ \
    if( mp##SHADER##Shader ) \
    { \
        if( m##SHADER##ShaderParameters.mTextureCount || m##SHADER##ShaderParameters.mBufferCount ) \
        { \
            same = true; \
            /* If all of the texture slots we need are already bound to our textures, then skip setting the SRVs... */\
            for( UINT ii=0; ii < m##SHADER##ShaderParameters.mTextureCount; ii++ ) \
            { \
                UINT bindPoint = m##SHADER##ShaderParameters.mpTextureParameterBindPoint[ii]; \
                if(mpLast##SHADER##ShaderViews[bindPoint] != m##SHADER##ShaderParameters.mppBindViews[bindPoint] ) \
                { \
                    mpLast##SHADER##ShaderViews[bindPoint] = m##SHADER##ShaderParameters.mppBindViews[bindPoint]; \
                    same = false; \
                } \
            } \
            for( UINT ii=0; ii < m##SHADER##ShaderParameters.mBufferCount; ii++ ) \
            { \
                UINT bindPoint = m##SHADER##ShaderParameters.mpBufferParameterBindPoint[ii]; \
                if(mpLast##SHADER##ShaderViews[bindPoint] != m##SHADER##ShaderParameters.mppBindViews[bindPoint] ) \
                { \
                    mpLast##SHADER##ShaderViews[bindPoint] = m##SHADER##ShaderParameters.mppBindViews[bindPoint]; \
                    same = false; \
                } \
            } \
            if( !same ) \
            { \
                int min   = m##SHADER##ShaderParameters.mBindViewMin; \
                int max   = m##SHADER##ShaderParameters.mBindViewMax; \
                int count = max - min + 1; \
                pContext->SHADER_TYPE##SetShaderResources( min, count, &m##SHADER##ShaderParameters.mppBindViews[min] ); \
            } \
        } \
        if( m##SHADER##ShaderParameters.mConstantBufferCount ) \
        { \
            same = true; \
            /* If all of the constant buffer slots we need are already bound to our constant buffers, then skip setting the SRVs... */\
            for( UINT ii=0; ii<m##SHADER##ShaderParameters.mConstantBufferCount; ii++ ) \
            { \
                UINT bindPoint = m##SHADER##ShaderParameters.mpConstantBufferParameterBindPoint[ii]; \
                if(mpLast##SHADER##ShaderConstantBuffers[bindPoint] != m##SHADER##ShaderParameters.mppBindConstantBuffers[bindPoint] ) \
                { \
                    mpLast##SHADER##ShaderConstantBuffers[bindPoint] = m##SHADER##ShaderParameters.mppBindConstantBuffers[bindPoint]; \
                    same = false; \
                } \
            } \
            if(!same) \
            { \
                int min   = m##SHADER##ShaderParameters.mBindConstantBufferMin; \
                int max   = m##SHADER##ShaderParameters.mBindConstantBufferMax; \
                int count = max - min + 1; \
                pContext->SHADER_TYPE##SetConstantBuffers(min, count,    &m##SHADER##ShaderParameters.mppBindConstantBuffers[min] ); \
            } \
        } \
    }

//-----------------------------------------------------------------------------
void CPUTMaterialDX11::SetRenderStates( CPUTRenderParameters &renderParams )
{
    ID3D11DeviceContext *pContext = ((CPUTRenderParametersDX*)&renderParams)->mpContext;

    bool same = true;

    if( mpExternalCount && mpExternalsConstantBuffer && mExternalsConstantBufferDirty )
    {
        // Note: Need to set this dirty flag if external values change (or if constant buffer shared between materials, etc.)
        mExternalsConstantBufferDirty = false;
        D3D11_MAPPED_SUBRESOURCE msr = mpExternalsConstantBuffer->MapBuffer( renderParams, CPUT_MAP_WRITE_DISCARD );
        for( int ii=0; ii<mpExternalCount[0]; ii++ )
        {
            float *pData = (float*)&((char*)msr.pData)[mpExternalOffset[0][ii]];
            memcpy(pData,&(mpExternalValue[0][ii]),mpExternalSize[0][ii]);
        }
        mpExternalsConstantBuffer->UnmapBuffer( renderParams );
    }

    SET_SHADER_RESOURCES( Vertex,    VS );
    SET_SHADER_RESOURCES( Pixel,     PS );
    SET_SHADER_RESOURCES( Compute,   CS );
    SET_SHADER_RESOURCES( Geometry,  GS );
    SET_SHADER_RESOURCES( Hull,      HS );
    SET_SHADER_RESOURCES( Domain,    DS );

    // Only compute shaders and pixel shaders can have UAVs.
    same = true;
    for( UINT ii=0; ii<mComputeShaderParameters.mUAVCount; ii++ )
    {
        UINT bindPoint = mComputeShaderParameters.mpUAVParameterBindPoint[ii];
        if(mpLastComputeShaderUAVs[ii] != mComputeShaderParameters.mppBindUAVs[bindPoint] )
        {
            mpLastComputeShaderUAVs[ii] = mComputeShaderParameters.mppBindUAVs[bindPoint];
            same = false;
        }
    }
    // Note that pixel shaders can have UAVs too, but DX requires setting those when setting RTV(s).
    if( mPixelShaderParameters.mUAVCount )
    {
        ID3D11RenderTargetView *pRTVs[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT];
        ID3D11DepthStencilView *pDSV;
        pContext->OMGetRenderTargets( D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, &pRTVs[0], &pDSV );
        // Find the last set RTV and set RTVs 0 through that RTV's index.
        UINT rtvCount;
        for( rtvCount=D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT-1; rtvCount>0; rtvCount-- )
        {
            if( 0 != pRTVs[rtvCount] ) break;
        }
        rtvCount+=1;
        pContext->OMSetRenderTargetsAndUnorderedAccessViews(
            rtvCount,
            pRTVs,
            pDSV,
            rtvCount,
            CPUT_MATERIAL_MAX_UAV_SLOTS-rtvCount,
            &mPixelShaderParameters.mppBindUAVs[rtvCount],
            NULL
        );
        SAFE_RELEASE(pDSV);
        for( UINT ii=0; ii<D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; ii++ )
        {
            SAFE_RELEASE( pRTVs[ii] );
        }
    }

    if(mComputeShaderParameters.mUAVCount && !same)
    {
        int min   = mComputeShaderParameters.mBindUAVMin;
        int max   = mComputeShaderParameters.mBindUAVMax;
        int count = max - min + 1;
        pContext->CSSetUnorderedAccessViews(min, count, &mComputeShaderParameters.mppBindUAVs[min], NULL );
    }

    // Set the render state block if it changed
    if( mpLastRenderStateBlock != mpRenderStateBlock )
    {
        mpLastRenderStateBlock = mpRenderStateBlock;
        if( mpRenderStateBlock )
        {
            // We know we have a DX11 class.  Does this correctly bypass the virtual?
            // Should we move it to the DX11 class.
            ((CPUTRenderStateBlockDX11*)mpRenderStateBlock)->SetRenderStates(renderParams);
        }
        else
        {
            CPUTRenderStateBlock::GetDefaultRenderStateBlock()->SetRenderStates(renderParams);
        }
    }
}

//-----------------------------------------------------------------------------
void CPUTMaterialDX11::ReadShaderSamplersAndTextures( ID3DBlob *pBlob, CPUTShaderParameters *pShaderParameter )
{
    // ***************************
    // Use shader reflection to get texture and sampler names.  We use them later to bind .mtl texture-specification to shader parameters/variables.
    // TODO: Currently do this only for PS.  Do for other shader types too.
    // TODO: Generalize, so easy to call for different shader types
    // ***************************
    ID3D11ShaderReflection *pReflector = NULL;
    D3D11_SHADER_INPUT_BIND_DESC desc;

    D3DReflect( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), IID_ID3D11ShaderReflection, (void**)&pReflector);
    // Walk through the shader input bind descriptors.  Find the samplers and textures.
    int ii=0;
    HRESULT hr = pReflector->GetResourceBindingDesc( ii++, &desc );
    while( SUCCEEDED(hr) )
    {
        switch( desc.Type )
        {
        case D3D_SIT_TEXTURE:
            pShaderParameter->mTextureParameterCount++;
            break;
        case D3D_SIT_SAMPLER:
            pShaderParameter->mSamplerParameterCount++;
            break;
        case D3D_SIT_CBUFFER:
            pShaderParameter->mConstantBufferParameterCount++;
            break;

        case D3D_SIT_TBUFFER:
        case D3D_SIT_STRUCTURED:
        case D3D_SIT_BYTEADDRESS:
            pShaderParameter->mBufferParameterCount++;
            break;

        case D3D_SIT_UAV_RWTYPED:
        case D3D_SIT_UAV_RWSTRUCTURED:
        case D3D_SIT_UAV_RWBYTEADDRESS:
        case D3D_SIT_UAV_APPEND_STRUCTURED:
        case D3D_SIT_UAV_CONSUME_STRUCTURED:
        case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
            pShaderParameter->mUAVParameterCount++;
            break;
        }
        hr = pReflector->GetResourceBindingDesc( ii++, &desc );
    }

    pShaderParameter->mpTextureParameterName              = new cString[pShaderParameter->mTextureParameterCount];
    pShaderParameter->mpTextureParameterBindPoint         = new UINT[   pShaderParameter->mTextureParameterCount];
    pShaderParameter->mpSamplerParameterName              = new cString[pShaderParameter->mSamplerParameterCount];
    pShaderParameter->mpSamplerParameterBindPoint         = new UINT[   pShaderParameter->mSamplerParameterCount];
    pShaderParameter->mpBufferParameterName               = new cString[pShaderParameter->mBufferParameterCount];
    pShaderParameter->mpBufferParameterBindPoint          = new UINT[   pShaderParameter->mBufferParameterCount];
    pShaderParameter->mpUAVParameterName                  = new cString[pShaderParameter->mUAVParameterCount];
    pShaderParameter->mpUAVParameterBindPoint             = new UINT[   pShaderParameter->mUAVParameterCount];
    pShaderParameter->mpConstantBufferParameterName       = new cString[pShaderParameter->mConstantBufferParameterCount];
    pShaderParameter->mpConstantBufferParameterBindPoint  = new UINT[   pShaderParameter->mConstantBufferParameterCount];
    pShaderParameter->mpConstantBufferDesc                = new D3D11_SHADER_BUFFER_DESC[pShaderParameter->mConstantBufferParameterCount];
    pShaderParameter->mpConstantBufferReflection          = new ID3D11ShaderReflectionConstantBuffer*[pShaderParameter->mConstantBufferParameterCount];

    // Start over.  This time, copy the names.
    ii=0;
    UINT textureIndex = 0;
    UINT samplerIndex = 0;
    UINT bufferIndex = 0;
    UINT uavIndex = 0;
    UINT constantBufferIndex = 0;
    hr = pReflector->GetResourceBindingDesc( ii++, &desc );

    while( SUCCEEDED(hr) )
    {
        switch( desc.Type )
        {
        case D3D_SIT_TEXTURE:
            pShaderParameter->mpTextureParameterName[textureIndex] = s2ws(desc.Name);
            pShaderParameter->mpTextureParameterBindPoint[textureIndex] = desc.BindPoint;
            textureIndex++;
            break;
        case D3D_SIT_SAMPLER:
            pShaderParameter->mpSamplerParameterName[samplerIndex] = s2ws(desc.Name);
            pShaderParameter->mpSamplerParameterBindPoint[samplerIndex] = desc.BindPoint;
            samplerIndex++;
            break;
        case D3D_SIT_CBUFFER:
            {
                pShaderParameter->mpConstantBufferParameterName[constantBufferIndex] = s2ws(desc.Name);
                pShaderParameter->mpConstantBufferParameterBindPoint[constantBufferIndex] = desc.BindPoint;
                pShaderParameter->mpConstantBufferReflection[constantBufferIndex] = pReflector->GetConstantBufferByIndex(constantBufferIndex);
                pShaderParameter->mpConstantBufferReflection[constantBufferIndex]->GetDesc(&pShaderParameter->mpConstantBufferDesc[constantBufferIndex]);
                constantBufferIndex++;
            }
            break;
        case D3D_SIT_TBUFFER:
        case D3D_SIT_STRUCTURED:
        case D3D_SIT_BYTEADDRESS:
            pShaderParameter->mpBufferParameterName[bufferIndex] = s2ws(desc.Name);
            pShaderParameter->mpBufferParameterBindPoint[bufferIndex] = desc.BindPoint;
            bufferIndex++;
            break;
        case D3D_SIT_UAV_RWTYPED:
        case D3D_SIT_UAV_RWSTRUCTURED:
        case D3D_SIT_UAV_RWBYTEADDRESS:
        case D3D_SIT_UAV_APPEND_STRUCTURED:
        case D3D_SIT_UAV_CONSUME_STRUCTURED:
        case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
            pShaderParameter->mpUAVParameterName[uavIndex] = s2ws(desc.Name);
            pShaderParameter->mpUAVParameterBindPoint[uavIndex] = desc.BindPoint;
            uavIndex++;
            break;
        }
        hr = pReflector->GetResourceBindingDesc( ii++, &desc );
    }
}

//-----------------------------------------------------------------------------
int CPUTMaterialDX11::GetExternalOffset( cString name )
{
    if( !mpExternalsConstantBufferReflection )
    {
        return -1; // No externals constant buffer reflection (most-likely because no externals constant buffer for this material)
    }
    char *pName = ws2s(name); // Unfortunately, ws2s allocates memory.  We need to free it.
    ID3D11ShaderReflectionVariable *pVariable = mpExternalsConstantBufferReflection->GetVariableByName( pName );
    delete pName;
    if( !pVariable )
    {
        return -1; // Not found.
    }
    D3D11_SHADER_VARIABLE_DESC desc;
    HRESULT hr = pVariable->GetDesc( &desc );
    if( !SUCCEEDED(hr) )
    {
        return -1; //  Not found
    }

    return desc.StartOffset;
}

//-----------------------------------------------------------------------------
int CPUTMaterialDX11::GetExternalSize( cString name )
{
    if( !mpExternalsConstantBufferReflection )
    {
        return -1; // No externals constant buffer reflection (most-likely because no externals constant buffer for this material)
    }
    char *pName = ws2s(name); // Unfortunately, ws2s allocates memory.  We need to free it.
    ID3D11ShaderReflectionVariable *pVariable = mpExternalsConstantBufferReflection->GetVariableByName( pName );
    delete pName;
    if( !pVariable )
    {
        return -1; // Not found.
    }
    D3D11_SHADER_VARIABLE_DESC desc;
    HRESULT hr = pVariable->GetDesc( &desc );
    if( !SUCCEEDED(hr) )
    {
        return -1; //  Not found
    }

    return desc.Size;
}

// TODO: these "Bind*" functions are almost identical, except they use different params.  Can we combine?
//-----------------------------------------------------------------------------
void CPUTMaterialDX11::BindTextures( CPUTShaderParameters &params, const CPUTModel *pModel, int meshIndex )
{
    CPUTAssetLibraryDX11 *pAssetLibrary = (CPUTAssetLibraryDX11*)CPUTAssetLibrary::GetAssetLibrary();

    for(params.mTextureCount=0; params.mTextureCount < params.mTextureParameterCount; params.mTextureCount++)
    {
        cString textureName;
        UINT textureCount = params.mTextureCount;
        cString tagName = params.mpTextureParameterName[textureCount];
        CPUTConfigEntry *pValue = mConfigBlock.GetValueByName(tagName);
        if( !pValue->IsValid() )
        {
            // We didn't find our property in the file.  Is it in the global config block?
            pValue = mGlobalProperties.GetValueByName(tagName);
        }
        ASSERT( pValue->IsValid(), L"Can't find texture '" + tagName + L"'." ); //  TODO: fix message
        textureName = pValue->ValueAsString();
        // If the texture name not specified.  Load default.dds instead
        if( 0 == textureName.length() ) { textureName = _L("default.dds"); }

        UINT bindPoint = params.mpTextureParameterBindPoint[textureCount];
        ASSERT( bindPoint < CPUT_MATERIAL_MAX_TEXTURE_SLOTS, _L("Texture bind point out of range.") );

        params.mBindViewMin = std::min( params.mBindViewMin, bindPoint );
        params.mBindViewMax = std::max( params.mBindViewMax, bindPoint );

        if( textureName[0] == '@' )
        {
            // This is a per-mesh value.  Add to per-mesh list.
            textureName += ptoc(pModel) + itoc(meshIndex);
        } else if( textureName[0] == '#' )
        {
            // This is a per-mesh value.  Add to per-mesh list.
            textureName += ptoc(pModel);
        }

        // Get the sRGB flag (default to true)
        cString SRGBName = tagName+_L("sRGB");
        CPUTConfigEntry *pSRGBValue = mConfigBlock.GetValueByName(SRGBName);
        bool loadAsSRGB = pSRGBValue->IsValid() ?  loadAsSRGB = pSRGBValue->ValueAsBool() : true;

        if( !params.mpTexture[textureCount] )
        {
            params.mpTexture[textureCount] = pAssetLibrary->GetTexture( textureName, false, loadAsSRGB );
            ASSERT( params.mpTexture[textureCount], _L("Failed getting texture ") + textureName);
        }

        // The shader file (e.g. .fx) can specify the texture bind point (e.g., t0).  Those specifications
        // might not be contiguous, and there might be gaps (bind points without assigned textures)
        // TODO: Warn about missing bind points?
        params.mppBindViews[bindPoint] = ((CPUTTextureDX11*)params.mpTexture[textureCount])->GetShaderResourceView();
        params.mppBindViews[bindPoint]->AddRef();

        OUTPUT_BINDING_DEBUG_INFO( (itoc(bindPoint) + _L(" : ") + params.mpTexture[textureCount]->GetName() + _L("\n")).c_str() );
    }
}

//-----------------------------------------------------------------------------
void CPUTMaterialDX11::BindBuffers( CPUTShaderParameters &params, const CPUTModel *pModel, int meshIndex)
{
    CPUTConfigEntry *pValue;
    if( !params.mBufferParameterCount ) { return; }
    OUTPUT_BINDING_DEBUG_INFO( _L("Bound Buffers") );

    CPUTAssetLibraryDX11 *pAssetLibrary = (CPUTAssetLibraryDX11*)CPUTAssetLibrary::GetAssetLibrary();
    for(params.mBufferCount=0; params.mBufferCount < params.mBufferParameterCount; params.mBufferCount++)
    {
        cString bufferName;
        UINT bufferCount = params.mBufferCount;
        cString tagName = params.mpBufferParameterName[bufferCount];
        {
            pValue = mConfigBlock.GetValueByName(tagName);
            if( !pValue->IsValid() )
            {
                // We didn't find our property in the file.  Is it in the global config block?
                pValue = mGlobalProperties.GetValueByName(tagName);
            }
            ASSERT( pValue->IsValid(), L"Can't find buffer '" + tagName + L"'." ); //  TODO: fix message
            bufferName = pValue->ValueAsString();
        }
        UINT bindPoint = params.mpBufferParameterBindPoint[bufferCount];
        ASSERT( bindPoint < CPUT_MATERIAL_MAX_BUFFER_SLOTS, _L("Buffer bind point out of range.") );

        params.mBindViewMin = std::min( params.mBindViewMin, bindPoint );
        params.mBindViewMax = std::max( params.mBindViewMax, bindPoint );

        const CPUTModel *pWhichModel = NULL;
        int              whichMesh   = -1;
        if( bufferName[0] == '@' )
        {
            pWhichModel = pModel;
            whichMesh   = meshIndex;
        } else if( bufferName[0] == '#' )
        {
            pWhichModel = pModel;
        }
        if( !params.mpBuffer[bufferCount] )
        {
            params.mpBuffer[bufferCount] = pAssetLibrary->GetBuffer( bufferName, pWhichModel, whichMesh );
            ASSERT( params.mpBuffer[bufferCount], _L("Failed getting buffer ") + bufferName);
        }

        params.mppBindViews[bindPoint]   = ((CPUTBufferDX11*)params.mpBuffer[bufferCount])->GetShaderResourceView();
        if( params.mppBindViews[bindPoint] )  { params.mppBindViews[bindPoint]->AddRef();}

        OUTPUT_BINDING_DEBUG_INFO( (itoc(bindPoint) + _L(" : ") + params.mpBuffer[bufferCount]->GetName() + _L("\n")).c_str() );
    }
    OUTPUT_BINDING_DEBUG_INFO( _L("\n") );
}

//-----------------------------------------------------------------------------
void CPUTMaterialDX11::BindUAVs( CPUTShaderParameters &params, const CPUTModel *pModel, int meshIndex )
{
    CPUTConfigEntry *pValue;
    if( !params.mUAVParameterCount ) { return; }
    OUTPUT_BINDING_DEBUG_INFO( _L("Bound UAVs") );

    CPUTAssetLibraryDX11 *pAssetLibrary = (CPUTAssetLibraryDX11*)CPUTAssetLibrary::GetAssetLibrary();
    memset( params.mppBindUAVs, 0, sizeof(params.mppBindUAVs) );
    for(params.mUAVCount=0; params.mUAVCount < params.mUAVParameterCount; params.mUAVCount++)
    {
        cString uavName;
        UINT uavCount = params.mUAVCount;

        cString tagName = params.mpUAVParameterName[uavCount];
        {
            pValue = mConfigBlock.GetValueByName(tagName);
            if( !pValue->IsValid() )
            {
                // We didn't find our property in the file.  Is it in the global config block?
                pValue = mGlobalProperties.GetValueByName(tagName);
            }
            ASSERT( pValue->IsValid(), L"Can't find UAV '" + tagName + L"'." ); //  TODO: fix message
            uavName = pValue->ValueAsString();
        }
        UINT bindPoint = params.mpUAVParameterBindPoint[uavCount];
        ASSERT( bindPoint < CPUT_MATERIAL_MAX_UAV_SLOTS, _L("UAV bind point out of range.") );

        params.mBindUAVMin = std::min( params.mBindUAVMin, bindPoint );
        params.mBindUAVMax = std::max( params.mBindUAVMax, bindPoint );

        const CPUTModel *pWhichModel = NULL;
        int              whichMesh   = -1;
        if( uavName[0] == '@' )
        {
            pWhichModel = pModel;
            whichMesh   = meshIndex;
        } else if( uavName[0] == '#' )
        {
            pWhichModel = pModel;
        }
        if( !params.mpUAV[uavCount] )
        {
            params.mpUAV[uavCount] = pAssetLibrary->GetBuffer( uavName );
            ASSERT( params.mpUAV[uavCount], _L("Failed getting UAV ") + uavName);
        }

        // If has UAV, then add to mppBindUAV
        params.mppBindUAVs[bindPoint]   = ((CPUTBufferDX11*)params.mpUAV[uavCount])->GetUnorderedAccessView();
        if( params.mppBindUAVs[bindPoint] )  { params.mppBindUAVs[bindPoint]->AddRef();}

        OUTPUT_BINDING_DEBUG_INFO( (itoc(bindPoint) + _L(" : ") + params.mpUAV[uavCount]->GetName() + _L("\n")).c_str() );
    }
    OUTPUT_BINDING_DEBUG_INFO( _L("\n") );
}

//-----------------------------------------------------------------------------
void CPUTMaterialDX11::BindConstantBuffers( CPUTShaderParameters &params, const CPUTModel *pModel, int meshIndex )
{
    CPUTConfigEntry *pValue;
    if( !params.mConstantBufferParameterCount ) { return; }
    OUTPUT_BINDING_DEBUG_INFO( _L("Bound Constant Buffers\n") );

    CPUTAssetLibraryDX11 *pAssetLibrary = (CPUTAssetLibraryDX11*)CPUTAssetLibrary::GetAssetLibrary();
    for(params.mConstantBufferCount=0; params.mConstantBufferCount < params.mConstantBufferParameterCount; params.mConstantBufferCount++)
    {
        cString constantBufferName;
        UINT constantBufferCount = params.mConstantBufferCount;

        cString tagName = params.mpConstantBufferParameterName[constantBufferCount];
        {
            pValue = mConfigBlock.GetValueByName(tagName);
            if( pValue->IsValid() )
            {
                constantBufferName = pValue->ValueAsString();
            } else
            {
                // We didn't find our property in the file.  Is it in the global config block?
                pValue = mGlobalProperties.GetValueByName( tagName );
                if( pValue->IsValid() )
                {
                    constantBufferName = pValue->ValueAsString();
                } else
                {
                    // Not specified in the .mtl file.  Also not in the global properties.  Is it our "externals" constant buffer?
                    // If the shader expects cbExternals, but the .mtl didn't supply it, and the global properties didn't either,
                    // then assume it's a per-model, per-mesh constant buffer.  In other words, the .mtl file (or global properties) can override
                    // this behavior.
                    if( tagName == _L( "cbExternals" ) )
                    {
                        mpExternalsConstantBufferReflection = params.mpConstantBufferReflection[params.mConstantBufferCount];

                        // Create the cbExternals constant buffer and add it to the asset library
                        D3D11_BUFFER_DESC bd = {0};
                        memcpy( &mExternalsConstantBufferDesc, &params.mpConstantBufferDesc[params.mConstantBufferCount], sizeof( mExternalsConstantBufferDesc ) );
                        bd.ByteWidth = mExternalsConstantBufferDesc.Size;
                        bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
                        bd.Usage = D3D11_USAGE_DYNAMIC;
                        bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
                        ID3D11Buffer *pExternalsConstantBuffer;
                        HRESULT hr = (CPUT_DX11::GetDevice())->CreateBuffer( &bd, NULL, &pExternalsConstantBuffer );
                        ASSERT( !FAILED( hr ), _L("Error creating constant buffer.") );
                        CPUTSetDebugName( pExternalsConstantBuffer, _L("Externals Constant buffer") );
                        mpExternalsConstantBuffer = new CPUTBufferDX11( constantBufferName, pExternalsConstantBuffer );

                        constantBufferName = _L("$cbExternals.") + mMaterialName + ptoc(pModel) + itoc(meshIndex) + ptoc(mpExternalsConstantBuffer);
                        CPUTAssetLibrary::GetAssetLibrary()->AddConstantBuffer( constantBufferName, _L(""), _L(""), mpExternalsConstantBuffer );
                        SAFE_RELEASE(pExternalsConstantBuffer); // We're done with it.  The CPUTBuffer now owns it.
                    } else
                    {
                        ASSERT( 0, L"Can't find constant buffer '" + tagName + L"'." ); //  TODO: fix message
                    }
                }
            }
        }
        UINT bindPoint = params.mpConstantBufferParameterBindPoint[constantBufferCount];
        ASSERT( bindPoint < CPUT_MATERIAL_MAX_CONSTANT_BUFFER_SLOTS, _L("Constant buffer bind point out of range.") );

        params.mBindConstantBufferMin = std::min( params.mBindConstantBufferMin, bindPoint );
        params.mBindConstantBufferMax = std::max( params.mBindConstantBufferMax, bindPoint );

        const CPUTModel *pWhichModel = NULL;
        int              whichMesh   = -1;
        if( constantBufferName[0] == '@' )
        {
            pWhichModel = pModel;
            whichMesh   = meshIndex;
        } else if( constantBufferName[0] == '#' )
        {
            pWhichModel = pModel;
        }
        if( !params.mpConstantBuffer[constantBufferCount] )
        {
            params.mpConstantBuffer[constantBufferCount] = pAssetLibrary->GetConstantBuffer( constantBufferName, pWhichModel, whichMesh );
            ASSERT( params.mpConstantBuffer[constantBufferCount], _L("Failed getting constant buffer ") + constantBufferName);
        }

        // If has constant buffer, then add to mppBindConstantBuffer
        params.mppBindConstantBuffers[bindPoint]   = ((CPUTBufferDX11*)params.mpConstantBuffer[constantBufferCount])->GetNativeBuffer();
        if( params.mppBindConstantBuffers[bindPoint] )  { params.mppBindConstantBuffers[bindPoint]->AddRef();}

        OUTPUT_BINDING_DEBUG_INFO( (itoc(bindPoint) + _L(" : ") + params.mpConstantBuffer[constantBufferCount]->GetName() + _L("\n")).c_str() );
    }
    OUTPUT_BINDING_DEBUG_INFO( _L("\n") );
}

//-----------------------------------------------------------------------------
void ReadMacrosFromConfigBlock(
    CPUTConfigBlock   *pMacrosBlock,
    CPUT_SHADER_MACRO  *pShaderMacros,
    CPUT_SHADER_MACRO **pFinalShaderMacros
){
    int numUserSpecifiedMacros = pMacrosBlock->ValueCount();

    // Count the number of macros passed in
    D3D_SHADER_MACRO *pMacro = (D3D_SHADER_MACRO*)pShaderMacros;
    int numPassedInMacros = 0;
    if( pMacro )
    {
        while( pMacro->Name )
        {
            ++numPassedInMacros;
            ++pMacro;
        }
    }

    // Allocate an array of macros large enough to contain the passed-in macros plus those specified in the .mtl file.
    *pFinalShaderMacros = new CPUT_SHADER_MACRO[numUserSpecifiedMacros + numPassedInMacros + 1];

    // Copy the passed-in macro pointers to the final array
    int jj;
    for( jj=0; jj<numPassedInMacros; jj++ )
    {
        (*pFinalShaderMacros)[jj] = *(CPUT_SHADER_MACRO*)&pShaderMacros[jj];
    }

    // Create a D3D_SHADER_MACRO for each of the macros specified in the .mtl file.
    // And, add their pointers to the final array.
    for( int kk=0; kk<numUserSpecifiedMacros; kk++, jj++ )
    {
        CPUTConfigEntry *pValue = pMacrosBlock->GetValue(kk);
        (*pFinalShaderMacros)[jj].Name       = ws2s(pValue->NameAsString());
        (*pFinalShaderMacros)[jj].Definition = ws2s(pValue->ValueAsString());
    }
    (*pFinalShaderMacros)[jj].Name = NULL;
    (*pFinalShaderMacros)[jj].Definition = NULL;
}

//-----------------------------------------------------------------------------
void CPUTMaterialDX11::ReadExternalsFromConfigBlock(
    CPUTConfigBlock *pExternalsBlock,
    int              externalCount,
    cString         *pExternalName,
    float4          *pExternals, // Original list
    int             *pExternalOffset,
    int             *pExternalSize,
    int             *pFinalExternalCount,
    cString        **pFinalExternalName,
    float4         **pFinalExternalValue, // List after combining originals and values from file
    int            **pFinalExternalOffset,
    int            **pFinalExternalSize
){
    if( !pExternalsBlock )
    {
        // No externals specified.  Copy any passed-in externals.
        *pFinalExternalCount = externalCount;
        if( externalCount )
        {
            *pFinalExternalValue  = new float4[externalCount];
            *pFinalExternalOffset = new int[externalCount];
            *pFinalExternalSize   = new int[externalCount];
            memcpy( *pFinalExternalValue, pExternals, externalCount * sizeof(float4) );
            memcpy( *pFinalExternalOffset, pExternalOffset, externalCount * sizeof(int) );
            memcpy( *pFinalExternalSize, pExternalSize, externalCount * sizeof(int) );
        } else
        {
            *pFinalExternalValue = NULL;
            *pFinalExternalOffset = NULL;
            *pFinalExternalSize   = NULL;
        }
        return;
    }
    int count = pExternalsBlock->ValueCount();
    int finalExternalCount = count + externalCount;
    *pFinalExternalCount   = finalExternalCount;
    *pFinalExternalName    = new cString[finalExternalCount];
    *pFinalExternalValue   = new float4[finalExternalCount];
    *pFinalExternalOffset  = new int[finalExternalCount];
    *pFinalExternalSize    = new int[finalExternalCount];

    // Copy any passed-in externals
    if( externalCount )
    {
        memcpy( *pFinalExternalValue, pExternals, externalCount * sizeof(float4) );
        for( int ii=0; ii<externalCount; ii++ )
        {
            // memcpy( *pFinalExternalOffset, pExternalOffset, externalCount * sizeof(int) );
            (*pFinalExternalName)[ii]   = pExternalName[ii];
            (*pFinalExternalOffset)[ii] = GetExternalOffset( pExternalName[ii] );
            (*pFinalExternalSize)[ii]   = GetExternalSize( pExternalName[ii] );  //TODO: Add above for this
        }
    }

    for( int ii=0; ii<count; ii++, externalCount++ )
    {
        CPUTConfigEntry *pEntry = pExternalsBlock->GetValue(ii);
        ASSERT( pEntry->IsValid(), _L("external value not valid") );
        (*pFinalExternalName)[externalCount] = pEntry->NameAsString();
        pEntry->ValueAsFloatArray((float*)&(*pFinalExternalValue)[externalCount], 4 );
        (*pFinalExternalOffset)[externalCount] = GetExternalOffset( (*pFinalExternalName)[externalCount] );
        ASSERT( (*pFinalExternalOffset)[externalCount] != -1, _L("External '") + (*pFinalExternalName)[externalCount] + _L("' not found.") );
        (*pFinalExternalSize)[externalCount] = GetExternalSize( (*pFinalExternalName)[externalCount] );;
    }
}

//-----------------------------------------------------------------------------
void CPUTMaterialDX11::ReadExternalsFromMasterConfigBlock(
    CPUTConfigBlock *pExternalsBlock,
    int             *pFinalExternalCount,
    cString        **pFinalExternalName,
    float4         **pFinalExternalValue, // List after combining originals and values from file
    int            **pFinalExternalOffset,
    int            **pFinalExternalSize
){                 
    if( !pExternalsBlock )
    {
        // No externals specified.  Copy any passed-in externals.
        *pFinalExternalCount = 0;
        *pFinalExternalName = NULL;
        *pFinalExternalValue = NULL;
        *pFinalExternalOffset = NULL;
        *pFinalExternalSize = NULL;
        return;
    }
    int count = pExternalsBlock->ValueCount();
    *pFinalExternalCount   = count;
    *pFinalExternalName    = new cString[count];
    *pFinalExternalValue   = new float4[count];
    *pFinalExternalOffset  = new int[count];
    *pFinalExternalSize    = new int[count];

    for( int ii=0; ii<count; ii++ )
    {
        CPUTConfigEntry *pEntry = pExternalsBlock->GetValue(ii);
        ASSERT( pEntry->IsValid(), _L("external value not valid") );
        (*pFinalExternalName)[ii] = pEntry->NameAsString();
        pEntry->ValueAsFloatArray((float*)&(*pFinalExternalValue)[ii], 4 );
        // Note: This is the master material.  It doesn't have any real shaders bound, etc.  So, there aren't any offsets to get.
        (*pFinalExternalOffset)[ii] = -1; // Set to -1 for now.  This list will eventually be passed to a real material and it will fill in it's values to match it's shaders
        (*pFinalExternalSize)[ii] = -1;
    }
}

//-----------------------------------------------------------------------------
CPUTResult CPUTMaterialDX11::LoadMaterial(
    const cString   &fileName,
    const CPUTModel *pModel,
          int        meshIndex,
    const char     **pShaderMacros,
          int        systemMaterialCount,
          cString   *pSystemMaterialNames,
          int        externalCount,
          cString   *pExternalName,
          float4    *pExternals,
          int       *pExternalOffset,
          int       *pExternalSize
){
    CPUTResult result = CPUT_SUCCESS;

    mMaterialName = fileName;
    mMaterialNameHash = CPUTComputeHash( mMaterialName );

    // Open/parse the file
    CPUTConfigFile file;
    result = file.LoadFile(fileName);
    if(CPUTFAILED(result))
    {
        return result;
    }

    // Make a local copy of all the parameters
    CPUTConfigBlock *pBlock = file.GetBlock(0);
    ASSERT( pBlock, _L("Error getting parameter block") );
    if( !pBlock )
    {
        return CPUT_ERROR_PARAMETER_BLOCK_NOT_FOUND;
    }
    mConfigBlock = *pBlock;

    // get necessary device and AssetLibrary pointers
    CPUTAssetLibraryDX11 *pAssetLibrary = (CPUTAssetLibraryDX11*)CPUTAssetLibrary::GetAssetLibrary();

    // TODO:  The following code is very repetitive.  Consider generalizing so we can call a function instead.
    // see if there are any pixel/vertex/geo shaders to load
    CPUTConfigEntry *pValue;
    CPUT_SHADER_MACRO *pFinalShaderMacros;

    if( mConfigBlock.GetValueByName( L"MultiMaterial" )->ValueAsBool() )
    {
        // Count materials;
        for(;;)
        {
            CPUTConfigEntry *pValue = mConfigBlock.GetValueByName( _L("Material") + itoc(mSubMaterialCount) );
            if( pValue->IsValid() )
            {
                ++mSubMaterialCount;
            } else
            {
                break;
            }
        }
        ASSERT(mSubMaterialCount != 0, _L("MultiMaterial specified, but no sub materials given.") );

        // Reserve space for "authored" materials plus system materials
        mpSubMaterialNames = new cString[mSubMaterialCount+systemMaterialCount];
        int ii;
        for( ii=0; ii<mSubMaterialCount; ii++ )
        {
            CPUTConfigEntry *pValue = mConfigBlock.GetValueByName( _L("Material") + itoc(ii) );
            mpSubMaterialNames[ii] = pValue->ValueAsString();
        }
        // The real material count includes the system material count
        mSubMaterialCount += systemMaterialCount;
        for( ii=0; ii<systemMaterialCount; ii++ )
        {
            // System materials "grow" from the end; the 1st system material is at the end of the list.
            mpSubMaterialNames[mSubMaterialCount-systemMaterialCount+ii] = pSystemMaterialNames[ii];
        }
        mpSubMaterials   = new CPUTMaterial*[mSubMaterialCount+1];
        mpExternalName   = new cString*[mSubMaterialCount];
        mpExternalValue  = new float4*[mSubMaterialCount];
        mpExternalOffset = new int*[mSubMaterialCount];
        mpExternalSize = new int*[mSubMaterialCount];
        mpExternalCount  = new int[mSubMaterialCount];

        for( int ii=0; ii<mSubMaterialCount; ii++ )
        {
            // Read additional macros from .mtl file
            cString macroBlockName = _L("defines") + itoc(ii);
            CPUTConfigBlock *pMacrosBlock = file.GetBlockByName(macroBlockName);
            int numUserSpecifiedMacros = 0;
            if( pMacrosBlock )
            {
                // This material specifies macros/defines.  Allocate a list big enough to hold the "master" macros + those specified by the material.
                // Copy the master macros to the new list, and also read the material's macros into the list.
                ReadMacrosFromConfigBlock( pMacrosBlock, (CPUT_SHADER_MACRO*)pShaderMacros, &pFinalShaderMacros );
            } else
            {
                // The material doesn't specify any additional macros.  Just use the "master" list.
                pFinalShaderMacros = (CPUT_SHADER_MACRO*)pShaderMacros;
            }

            // Read externals from .mtl file
            cString externalsBlockName = _L("externals") + itoc(ii);
            CPUTConfigBlock *pExternalsBlock = file.GetBlockByName(externalsBlockName);
            ReadExternalsFromMasterConfigBlock( pExternalsBlock, &mpExternalCount[ii], &mpExternalName[ii], &mpExternalValue[ii], &mpExternalOffset[ii],&mpExternalSize[ii] );
            mpSubMaterials[ii] = pAssetLibrary->GetMaterial( mpSubMaterialNames[ii], false, pModel, meshIndex, (const char**)pFinalShaderMacros, 0, NULL, mpExternalName[ii], mpExternalValue[ii], mpExternalOffset[ii], mpExternalSize[ii],mpExternalCount[ii] );
            // ASSERT( !mpSubMaterials[ii]->IsMultiMaterial(), L"Multi-material cannot have a sub material that is also a multi-material (hierarchy not supported at the moment)" );

            // If we allocated pFinalShaderMacros, then delete them
            if( pFinalShaderMacros != (CPUT_SHADER_MACRO*)pShaderMacros )
            {
                for( CPUT_SHADER_MACRO *pCur = pFinalShaderMacros; NULL != pCur->Name; pCur++ )
                {
                    SAFE_DELETE(pCur->Name);
                    SAFE_DELETE(pCur->Definition);
                }
                SAFE_DELETE_ARRAY( pFinalShaderMacros );
            }
        }
        mpSubMaterials[mSubMaterialCount] = NULL;

        return result; // This material is a multi-material, so we're done.
    } // is multi-material
    else
    {
        mSubMaterialCount     = 1 + systemMaterialCount;
        mpSubMaterialNames    = new cString[mSubMaterialCount+1]; // +1 for NULL terminator
        mpSubMaterials        = new CPUTMaterial*[mSubMaterialCount+1];
        mpSubMaterialNames[0] = mMaterialName;
        mpSubMaterials[0]     = this;
        int ii;
        for( ii=0; ii<systemMaterialCount; ii++ )
        {
            mpSubMaterialNames[ii+1] = pSystemMaterialNames[ii];
            mpSubMaterials[ii+1]     = pAssetLibrary->GetMaterial( mpSubMaterialNames[ii+1], false, pModel, meshIndex, pShaderMacros );
        }
        mpSubMaterials[ii+1] = NULL;

        // Read additional macros from .mtl file
        cString macroBlockName = _L("defines");
        CPUTConfigBlock *pMacrosBlock = file.GetBlockByName(macroBlockName);
        int numUserSpecifiedMacros = 0;
        if( pMacrosBlock )
        {
            // This material specifies macros/defines.  Allocate a list big enough to hold the "master" macros + those specified by the material.
            // Copy the master macros to the new list, and also read the material's macros into the list.
            ReadMacrosFromConfigBlock( pMacrosBlock, (CPUT_SHADER_MACRO*)pShaderMacros, &pFinalShaderMacros );
        } else
        {
            // The material doesn't specify any additional macros.  Just use the "master" list.
            pFinalShaderMacros = (CPUT_SHADER_MACRO*)pShaderMacros;
        }

        CPUTConfigEntry *pEntryPointName, *pProfileName;
        pValue   = mConfigBlock.GetValueByName(_L("VertexShaderFile"));
        if( pValue->IsValid() )
        {
            pEntryPointName = mConfigBlock.GetValueByName(_L("VertexShaderMain"));
            pProfileName    = mConfigBlock.GetValueByName(_L("VertexShaderProfile"));
            pAssetLibrary->GetVertexShader(
                pValue->ValueAsString(),
                pEntryPointName->ValueAsString(),
                pProfileName->ValueAsString(),
                &mpVertexShader,
                false,
                pFinalShaderMacros
            );
            ReadShaderSamplersAndTextures( mpVertexShader->GetBlob(), &mVertexShaderParameters );
        }

        // load and store the pixel shader if it was specified
        pValue  = mConfigBlock.GetValueByName(_L("PixelShaderFile"));
        if( pValue->IsValid() )
        {
            pEntryPointName = mConfigBlock.GetValueByName(_L("PixelShaderMain"));
            pProfileName    = mConfigBlock.GetValueByName(_L("PixelShaderProfile"));
            pAssetLibrary->GetPixelShader(
                pValue->ValueAsString(),
                pEntryPointName->ValueAsString(),
                pProfileName->ValueAsString(),
                &mpPixelShader,
                false,
                pFinalShaderMacros
            );
            ReadShaderSamplersAndTextures( mpPixelShader->GetBlob(), &mPixelShaderParameters );
        }

        // load and store the compute shader if it was specified
        pValue = mConfigBlock.GetValueByName(_L("ComputeShaderFile"));
        if( pValue->IsValid() )
        {
            pEntryPointName = mConfigBlock.GetValueByName(_L("ComputeShaderMain"));
            pProfileName = mConfigBlock.GetValueByName(_L("ComputeShaderProfile"));
            pAssetLibrary->GetComputeShader(
                pValue->ValueAsString(),
                pEntryPointName->ValueAsString(),
                pProfileName->ValueAsString(),
                &mpComputeShader,
                false,
                pFinalShaderMacros
            );
            ReadShaderSamplersAndTextures( mpComputeShader->GetBlob(), &mComputeShaderParameters );
        }

        // load and store the geometry shader if it was specified
        pValue = mConfigBlock.GetValueByName(_L("GeometryShaderFile"));
        if( pValue->IsValid() )
        {
            pEntryPointName = mConfigBlock.GetValueByName(_L("GeometryShaderMain"));
            pProfileName = mConfigBlock.GetValueByName(_L("GeometryShaderProfile"));
            pAssetLibrary->GetGeometryShader(
                pValue->ValueAsString(),
                pEntryPointName->ValueAsString(),
                pProfileName->ValueAsString(),
                &mpGeometryShader,
                false,
                pFinalShaderMacros
            );
            ReadShaderSamplersAndTextures( mpGeometryShader->GetBlob(), &mGeometryShaderParameters );
        }

        // load and store the hull shader if it was specified
        pValue = mConfigBlock.GetValueByName(_L("HullShaderFile"));
        if( pValue->IsValid() )
        {
            pEntryPointName = mConfigBlock.GetValueByName(_L("HullShaderMain"));
            pProfileName = mConfigBlock.GetValueByName(_L("HullShaderProfile"));
            pAssetLibrary->GetHullShader(
                pValue->ValueAsString(),
                pEntryPointName->ValueAsString(),
                pProfileName->ValueAsString(),
                &mpHullShader,
                false,
                pFinalShaderMacros
            );
            ReadShaderSamplersAndTextures( mpHullShader->GetBlob(), &mHullShaderParameters );
        }

        // load and store the domain shader if it was specified
        pValue = mConfigBlock.GetValueByName(_L("DomainShaderFile"));
        if( pValue->IsValid() )
        {
            pEntryPointName = mConfigBlock.GetValueByName(_L("DomainShaderMain"));
            pProfileName = mConfigBlock.GetValueByName(_L("DomainShaderProfile"));
            pAssetLibrary->GetDomainShader(
                pValue->ValueAsString(),
                pEntryPointName->ValueAsString(),
                pProfileName->ValueAsString(),
                &mpDomainShader,
                false,
                pFinalShaderMacros
            );
            ReadShaderSamplersAndTextures( mpDomainShader->GetBlob(), &mDomainShaderParameters );
        }

        // load and store the render state file if it was specified
        pValue = mConfigBlock.GetValueByName(_L("RenderStateFile"));
        if( pValue->IsValid() )
        {
            mpRenderStateBlock = pAssetLibrary->GetRenderStateBlock(pValue->ValueAsString());
        }

        OUTPUT_BINDING_DEBUG_INFO( (_L("Bindings for : ") + mMaterialName + _L("\n")).c_str() );
        cString pShaderTypeNameList[] = {
            _L("Pixel shader"),
            _L("Compute shader"),
            _L("Vertex shader"),
            _L("Geometry shader"),
            _L("Hull shader"),
            _L("Domain shader"),
        };
        cString *pShaderTypeName = pShaderTypeNameList;

        void *pShaderList[] = {
            mpPixelShader,
            mpComputeShader,
            mpVertexShader,
            mpGeometryShader,
            mpHullShader,
            mpDomainShader
        };
        void **pShader = pShaderList;

        // For each of the shader stages, bind shaders and buffers
        for( CPUTShaderParameters **pCur = mpShaderParametersList; *pCur; pCur++ ) // Bind textures and buffersfor each shader stage
        {
            // Clear the bindings to reduce "resource still bound" errors, and to avoid leaving garbage pointers between valid pointers.
            // We bind resources as arrays.  We bind from the min required bind slot to the max-required bind slot.
            // Any slots in between will get bound.  It isn't clear if D3D will simply ignore them, or not.
            // But, they could be garbage, or valid resources from a previous binding.
            memset( (*pCur)->mppBindViews,           0, sizeof((*pCur)->mppBindViews) );
            memset( (*pCur)->mppBindUAVs,            0, sizeof((*pCur)->mppBindUAVs) );
            memset( (*pCur)->mppBindConstantBuffers, 0, sizeof((*pCur)->mppBindConstantBuffers) );

            if( !*pShader++ )
            {
                pShaderTypeName++; // Increment the name pointer to remain coherent.
                continue;          // This shader not bound.  Don't waste time binding to it.
            }

            OUTPUT_BINDING_DEBUG_INFO( (*(pShaderTypeName++)  + _L("\n")).c_str() );
            BindTextures(        **pCur, pModel, meshIndex );
            BindBuffers(         **pCur, pModel, meshIndex );
            BindUAVs(            **pCur, pModel, meshIndex );
            BindConstantBuffers( **pCur, pModel, meshIndex );

            OUTPUT_BINDING_DEBUG_INFO( _L("\n") );
        }

        // Safe to read externals now that we've read in the shaders.
        cString externalsBlockName = _L("externals");
        CPUTConfigBlock *pExternalsBlock = file.GetBlockByName(externalsBlockName);
        if( pExternalsBlock )
        {
            // This material wasn't authored as a multi-material.  But, if the caller specified system materials, then it is still a multi material.
            // So, allocate room for all sub materials.  In either case, the authored material is at index 0.
            mpExternalName   = new cString*[mSubMaterialCount];
            mpExternalValue  = new float4*[mSubMaterialCount];
            mpExternalOffset = new int*[mSubMaterialCount];
            mpExternalSize = new int*[mSubMaterialCount];
            mpExternalCount  = new int[mSubMaterialCount];
            // Initialize to NULL so the destructor's SAFE_DELETE_ARRAY() calls don't crash.
            memset( mpExternalName,   0, mSubMaterialCount * sizeof(cString*) );
            memset( mpExternalValue,  0, mSubMaterialCount * sizeof(float4*) );
            memset( mpExternalOffset, 0, mSubMaterialCount * sizeof(int*) );
            memset( mpExternalSize,   0, mSubMaterialCount * sizeof(int*));
            ReadExternalsFromConfigBlock( pExternalsBlock, externalCount, pExternalName, pExternals, pExternalOffset,pExternalSize, &mpExternalCount[0], &mpExternalName[0], &mpExternalValue[0], &mpExternalOffset[0],&mpExternalSize[0] );
        }

        // If we allocated pFinalShaderMacros, then delete them
        if( pFinalShaderMacros != (CPUT_SHADER_MACRO*)pShaderMacros )
        {
            for( CPUT_SHADER_MACRO *pCur = pFinalShaderMacros; NULL != pCur->Name; pCur++ )
            {
                SAFE_DELETE(pCur->Name);
                SAFE_DELETE(pCur->Definition);
            }
            SAFE_DELETE_ARRAY( pFinalShaderMacros );
        }

        return result;
    } // not multi-material
}

//-----------------------------------------------------------------------------
CPUTMaterial *CPUTMaterialDX11::CloneMaterial(const cString &fileName, const CPUTModel *pModel, int meshIndex)
{
    CPUTMaterialDX11 *pMaterial = new CPUTMaterialDX11();

    // TODO: move texture to base class.  All APIs have textures.
    pMaterial->mpPixelShader    = mpPixelShader;    if(mpPixelShader)    mpPixelShader->AddRef();
    pMaterial->mpComputeShader  = mpComputeShader;  if(mpComputeShader)  mpComputeShader->AddRef();
    pMaterial->mpVertexShader   = mpVertexShader;   if(mpVertexShader)   mpVertexShader->AddRef();
    pMaterial->mpGeometryShader = mpGeometryShader; if(mpGeometryShader) mpGeometryShader->AddRef();
    pMaterial->mpHullShader     = mpHullShader;     if(mpHullShader)     mpHullShader->AddRef();
    pMaterial->mpDomainShader   = mpDomainShader;   if(mpDomainShader)   mpDomainShader->AddRef();

    mPixelShaderParameters.CloneShaderParameters(    &pMaterial->mPixelShaderParameters );
    mComputeShaderParameters.CloneShaderParameters(  &pMaterial->mComputeShaderParameters );
    mVertexShaderParameters.CloneShaderParameters(   &pMaterial->mVertexShaderParameters );
    mGeometryShaderParameters.CloneShaderParameters( &pMaterial->mGeometryShaderParameters );
    mHullShaderParameters.CloneShaderParameters(     &pMaterial->mHullShaderParameters );
    mDomainShaderParameters.CloneShaderParameters(   &pMaterial->mDomainShaderParameters );

    pMaterial->mpShaderParametersList[0] =  &pMaterial->mPixelShaderParameters,
    pMaterial->mpShaderParametersList[1] =  &pMaterial->mComputeShaderParameters,
    pMaterial->mpShaderParametersList[2] =  &pMaterial->mVertexShaderParameters,
    pMaterial->mpShaderParametersList[3] =  &pMaterial->mGeometryShaderParameters,
    pMaterial->mpShaderParametersList[4] =  &pMaterial->mHullShaderParameters,
    pMaterial->mpShaderParametersList[5] =  &pMaterial->mDomainShaderParameters,
    pMaterial->mpShaderParametersList[6] =  NULL;

    pMaterial->mpRenderStateBlock = mpRenderStateBlock; if( mpRenderStateBlock ) mpRenderStateBlock->AddRef();

    pMaterial->mMaterialName      = mMaterialName + ptoc(pModel) + itoc(meshIndex);
    pMaterial->mMaterialNameHash  = CPUTComputeHash(pMaterial->mMaterialName);
    pMaterial->mConfigBlock       = mConfigBlock;

    pMaterial->mpExternalCount  = new int[mSubMaterialCount];
    pMaterial->mpExternalName   = new cString*[mSubMaterialCount];
    pMaterial->mpExternalValue  = new float4*[mSubMaterialCount];
    pMaterial->mpExternalOffset = new int*[mSubMaterialCount];
    pMaterial->mpExternalSize = new int*[mSubMaterialCount];

    // Clone the submaterials
    pMaterial->mSubMaterialCount = mSubMaterialCount;
    if( mSubMaterialCount > 0 )
    {
        pMaterial->mpSubMaterials = new CPUTMaterial*[mSubMaterialCount+1];

        for( int ii=0; ii<mSubMaterialCount; ii++ )
        {
            pMaterial->mpExternalCount[ii]  = mpExternalCount[ii];
            pMaterial->mpExternalName[ii]   = mpExternalName[ii];
            pMaterial->mpExternalValue[ii]  = mpExternalValue[ii];
            pMaterial->mpExternalOffset[ii] = mpExternalOffset[ii];
            pMaterial->mpExternalSize[ii]   = mpExternalSize[ii];
            if( mpSubMaterials[ii] == this )
            {
                pMaterial->mpSubMaterials[ii] = pMaterial; // This material isn't a multi-material.  It's first element points to itself.
            } else
            {
                pMaterial->mpSubMaterials[ii] = mpSubMaterials[ii]->CloneMaterial( mpSubMaterialNames[ii], pModel, meshIndex );
            }
        }
        pMaterial->mpSubMaterials[mSubMaterialCount] = NULL;
    }

    // For each of the shader stages, bind shaders and buffers
    for( CPUTShaderParameters **pCur = pMaterial->mpShaderParametersList; *pCur; pCur++ ) // Bind textures and buffersfor each shader stage
    {
        pMaterial->BindTextures(        **pCur, pModel, meshIndex );
        pMaterial->BindBuffers(         **pCur, pModel, meshIndex );
        pMaterial->BindUAVs(            **pCur, pModel, meshIndex );
        pMaterial->BindConstantBuffers( **pCur, pModel, meshIndex );
    }

    // Append this clone to our clone list
    if( mpMaterialNextClone )
    {
        // Already have a list, so add to the end of it.
        ((CPUTMaterialDX11*)mpMaterialLastClone)->mpMaterialNextClone = pMaterial;
    } else
    {
        // No list yet, so start it with this material.
        mpMaterialNextClone = pMaterial;
        mpMaterialLastClone = pMaterial;
    }
    pMaterial->mpModel    = pModel;
    pMaterial->mMeshIndex = meshIndex;
    return pMaterial;
}

//-----------------------------------------------------------------------------
bool CPUTMaterialDX11::MaterialRequiresPerModelPayload()
{
    if( mRequiresPerModelPayload == -1 )
    {
        mRequiresPerModelPayload =
            (mpPixelShader    && mpPixelShader   ->ShaderRequiresPerModelPayload(mConfigBlock))  ||
            (mpComputeShader  && mpComputeShader ->ShaderRequiresPerModelPayload(mConfigBlock))  ||
            (mpVertexShader   && mpVertexShader  ->ShaderRequiresPerModelPayload(mConfigBlock))  ||
            (mpGeometryShader && mpGeometryShader->ShaderRequiresPerModelPayload(mConfigBlock))  ||
            (mpHullShader     && mpHullShader    ->ShaderRequiresPerModelPayload(mConfigBlock))  ||
            (mpDomainShader   && mpDomainShader  ->ShaderRequiresPerModelPayload(mConfigBlock));
    }
    return mRequiresPerModelPayload != 0;
}

//-----------------------------------------------------------------------------
void CPUTMaterialDX11::RebindTexturesAndBuffers()
{
    for( CPUTShaderParameters **pCur = mpShaderParametersList; *pCur; pCur++ ) // Rebind textures for each shader stage
    {
        for( UINT ii=0; ii<(*pCur)->mTextureCount; ii++ )
        {
            if( (*pCur)->mpTexture[ii] )
            {
                UINT bindPoint = (*pCur)->mpTextureParameterBindPoint[ii];
                SAFE_RELEASE((*pCur)->mppBindViews[bindPoint]);
                (*pCur)->mppBindViews[bindPoint] = ((CPUTTextureDX11*)(*pCur)->mpTexture[ii])->GetShaderResourceView();
                (*pCur)->mppBindViews[bindPoint]->AddRef();
            }
        }
        for( UINT ii=0; ii<(*pCur)->mBufferCount; ii++ )
        {
            if( (*pCur)->mpBuffer[ii] )
            {
                UINT bindPoint = (*pCur)->mpBufferParameterBindPoint[ii];
                SAFE_RELEASE((*pCur)->mppBindViews[bindPoint]);
                (*pCur)->mppBindViews[bindPoint] = ((CPUTBufferDX11*)(*pCur)->mpBuffer[ii])->GetShaderResourceView();
                (*pCur)->mppBindViews[bindPoint]->AddRef();
            }
        }
        for( UINT ii=0; ii<(*pCur)->mUAVCount; ii++ )
        {
            if( (*pCur)->mpUAV[ii] )
            {
                UINT bindPoint = (*pCur)->mpUAVParameterBindPoint[ii];
                SAFE_RELEASE((*pCur)->mppBindUAVs[bindPoint]);
                (*pCur)->mppBindUAVs[bindPoint] = ((CPUTBufferDX11*)(*pCur)->mpUAV[ii])->GetUnorderedAccessView();
                (*pCur)->mppBindUAVs[bindPoint]->AddRef();
            }
        }
        for( UINT ii=0; ii<(*pCur)->mConstantBufferCount; ii++ )
        {
            if( (*pCur)->mpConstantBuffer[ii] )
            {
                UINT bindPoint = (*pCur)->mpConstantBufferParameterBindPoint[ii];
                SAFE_RELEASE((*pCur)->mppBindConstantBuffers[bindPoint]);
                (*pCur)->mppBindConstantBuffers[bindPoint] = ((CPUTBufferDX11*)(*pCur)->mpConstantBuffer[ii])->GetNativeBuffer();
                (*pCur)->mppBindConstantBuffers[bindPoint]->AddRef();
            }
        }
    }
}

//-----------------------------------------------------------------------------
void CPUTMaterialDX11::ReleaseTexturesAndBuffers()
{
    for( CPUTShaderParameters **pCur = mpShaderParametersList; *pCur; pCur++ ) // Release the buffers and views for each shader stage
    {
        if( *pCur )
        {
            for( UINT ii=0; ii<CPUT_MATERIAL_MAX_TEXTURE_SLOTS; ii++ )
            {
                SAFE_RELEASE((*pCur)->mppBindViews[ii]);
            }
            for( UINT ii=0; ii<CPUT_MATERIAL_MAX_UAV_SLOTS; ii++ )
            {
                SAFE_RELEASE((*pCur)->mppBindUAVs[ii]);
            }
            for( UINT ii=0; ii<CPUT_MATERIAL_MAX_CONSTANT_BUFFER_SLOTS; ii++ )
            {
                SAFE_RELEASE((*pCur)->mppBindConstantBuffers[ii]);
            }
        }
    }
}

//-----------------------------------------------------------------------------
void CPUTShaderParameters::CloneShaderParameters( CPUTShaderParameters *pShaderParameter )
{
    pShaderParameter->mpTextureParameterName              = new cString[mTextureParameterCount];
    pShaderParameter->mpTextureParameterBindPoint         = new UINT[   mTextureParameterCount];
    pShaderParameter->mpSamplerParameterName              = new cString[mSamplerParameterCount];
    pShaderParameter->mpSamplerParameterBindPoint         = new UINT[   mSamplerParameterCount];
    pShaderParameter->mpBufferParameterName               = new cString[mBufferParameterCount];
    pShaderParameter->mpBufferParameterBindPoint          = new UINT[   mBufferParameterCount];
    pShaderParameter->mpUAVParameterName                  = new cString[mUAVParameterCount];
    pShaderParameter->mpUAVParameterBindPoint             = new UINT[   mUAVParameterCount];
    pShaderParameter->mpConstantBufferParameterName       = new cString[mConstantBufferParameterCount];
    pShaderParameter->mpConstantBufferParameterBindPoint  = new UINT[   mConstantBufferParameterCount];
    pShaderParameter->mpConstantBufferDesc                = new D3D11_SHADER_BUFFER_DESC[mConstantBufferParameterCount];
    pShaderParameter->mpConstantBufferReflection          = new ID3D11ShaderReflectionConstantBuffer*[mConstantBufferParameterCount];

    pShaderParameter->mTextureCount                 = mTextureCount;
    pShaderParameter->mBufferCount                  = mBufferCount;
    pShaderParameter->mUAVCount                     = mUAVCount;
    pShaderParameter->mConstantBufferCount          = mConstantBufferCount;

    pShaderParameter->mTextureParameterCount        = mTextureParameterCount;
    pShaderParameter->mTextureParameterCount        = mTextureParameterCount;
    pShaderParameter->mBufferParameterCount         = mBufferParameterCount;
    pShaderParameter->mUAVParameterCount            = mUAVParameterCount;
    pShaderParameter->mConstantBufferParameterCount = mConstantBufferParameterCount;

    for(UINT ii=0; ii<mTextureParameterCount; ii++ )
    {
        pShaderParameter->mpTextureParameterName[ii]      = mpTextureParameterName[ii];
        pShaderParameter->mpTextureParameterBindPoint[ii] = mpTextureParameterBindPoint[ii];
    }
    for(UINT ii=0; ii<mSamplerParameterCount; ii++ )
    {
        pShaderParameter->mpSamplerParameterName[ii]      = mpSamplerParameterName[ii];
        pShaderParameter->mpSamplerParameterBindPoint[ii] = mpSamplerParameterBindPoint[ii];
    }
    for(UINT ii=0; ii<mBufferParameterCount; ii++ )
    {
        pShaderParameter->mpBufferParameterName[ii]      = mpBufferParameterName[ii];
        pShaderParameter->mpBufferParameterBindPoint[ii] = mpBufferParameterBindPoint[ii];
    }
    for(UINT ii=0; ii<mUAVParameterCount; ii++ )
    {
        pShaderParameter->mpUAVParameterName[ii]      = mpUAVParameterName[ii];
        pShaderParameter->mpUAVParameterBindPoint[ii] = mpUAVParameterBindPoint[ii];
    }

    for(UINT ii=0; ii<mConstantBufferParameterCount; ii++ )
    {
        pShaderParameter->mpConstantBufferParameterName[ii]      = mpConstantBufferParameterName[ii];
        pShaderParameter->mpConstantBufferParameterBindPoint[ii] = mpConstantBufferParameterBindPoint[ii];
        pShaderParameter->mpConstantBufferDesc[ii]               = mpConstantBufferDesc[ii];
        pShaderParameter->mpConstantBufferReflection[ii]         = mpConstantBufferReflection[ii];
    }
    pShaderParameter->mBindViewMin = mBindViewMin;
    pShaderParameter->mBindViewMax = mBindViewMax;

    pShaderParameter->mBindUAVMin = mBindUAVMin;
    pShaderParameter->mBindUAVMax = mBindUAVMax;

    pShaderParameter->mBindConstantBufferMin = mBindConstantBufferMin;
    pShaderParameter->mBindConstantBufferMax = mBindConstantBufferMax;
}
