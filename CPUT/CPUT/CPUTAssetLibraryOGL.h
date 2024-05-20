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
#ifndef __CPUTASSETLIBRARYOGL_H__
#define __CPUTASSETLIBRARYOGL_H__

#include "CPUTAssetLibrary.h"
#include "CPUTConfigBlock.h"
#include <vector>
class CPUTAssetSet;
class CPUTMaterial;
class CPUTModel;
class CPUTNullNode;
class CPUTCamera;
class CPUTRenderStateBlock;
class CPUTLight;
class CPUTTexture;
class CPUTShaderOGL;

//-----------------------------------------------------------------------------
struct CPUTSRGBLoadFlags
{
    bool bInterpretInputasSRGB;
    bool bWritetoSRGBOutput;
};

//-----------------------------------------------------------------------------
class CPUTAssetLibraryOGL:public CPUTAssetLibrary
{
protected:
    static CPUTAssetListEntry  *mpPixelShaderList;
    static CPUTAssetListEntry  *mpComputeShaderList;
    static CPUTAssetListEntry  *mpVertexShaderList;
    static CPUTAssetListEntry  *mpGeometryShaderList;
    static CPUTAssetListEntry  *mpHullShaderList;
    static CPUTAssetListEntry  *mpDomainShaderList;
    
    static CPUTAssetListEntry  *mpPixelShaderListTail;
    static CPUTAssetListEntry  *mpVertexShaderListTail;
    static CPUTAssetListEntry  *mpComputeShaderListTail;
    static CPUTAssetListEntry  *mpGeometryShaderListTail;
    static CPUTAssetListEntry  *mpHullShaderListTail;
    static CPUTAssetListEntry  *mpDomainShaderListTail;

public:
    CPUTAssetLibraryOGL(){}
    virtual ~CPUTAssetLibraryOGL()
    {
        ReleaseAllLibraryLists();
    }

    virtual void ReleaseAllLibraryLists();
    void ReleaseIunknownList( CPUTAssetListEntry *pList );
    
    void AddVertexShader(   const cString &name, CPUTShaderOGL *pShader) { AddAsset( _L(""), name, _L(""), pShader, &mpVertexShaderList, &mpVertexShaderListTail ); }
    void AddComputeShader(   const cString &name, CPUTShaderOGL *pShader) { AddAsset( _L(""), name, _L(""), pShader, &mpComputeShaderList, &mpComputeShaderListTail ); }
    void AddPixelShader(    const cString &name, CPUTShaderOGL *pShader) { AddAsset( _L(""), name, _L(""), pShader, &mpPixelShaderList, &mpPixelShaderListTail ); }

    void AddHullShader( const cString &name, CPUTShaderOGL *pShader) { AddAsset( _L(""), name, _L(""), pShader, &mpHullShaderList, &mpHullShaderListTail ); }
    void AddDomainShader( const cString &name, CPUTShaderOGL *pShader) { AddAsset( _L(""), name, _L(""), pShader, &mpDomainShaderList, &mpDomainShaderListTail ); }
    void AddGeometryShader( const cString &name, CPUTShaderOGL *pShader) { AddAsset( _L(""), name, _L(""), pShader, &mpGeometryShaderList, &mpGeometryShaderListTail ); }

	CPUTShaderOGL   *FindVertexShader(   const cString &name, /*const cString &decoration,*/ bool nameIsFullPathAndFilename=false ) { return   (CPUTShaderOGL   *)FindAsset( name, /*decoration,*/ mpVertexShaderList,   nameIsFullPathAndFilename ); }
	CPUTShaderOGL   *FindComputeShader(   const cString &name, /*const cString &decoration,*/ bool nameIsFullPathAndFilename=false ) { return   (CPUTShaderOGL   *)FindAsset( name, /*decoration,*/ mpComputeShaderList,   nameIsFullPathAndFilename ); }
    CPUTShaderOGL   *FindPixelShader(    const cString &name, /*const cString &decoration,*/ bool nameIsFullPathAndFilename=false ) { return   (CPUTShaderOGL   *)FindAsset( name, /*decoration,*/ mpPixelShaderList,    nameIsFullPathAndFilename ); }
	CPUTShaderOGL   *FindHullShader(     const cString &name, /*const cString &decoration,*/ bool nameIsFullPathAndFilename=false ) { return   (CPUTShaderOGL   *)FindAsset( name, /*decoration,*/ mpHullShaderList,     nameIsFullPathAndFilename ); }
    CPUTShaderOGL   *FindDomainShader(   const cString &name, /*const cString &decoration,*/ bool nameIsFullPathAndFilename=false ) { return   (CPUTShaderOGL   *)FindAsset( name, /*decoration,*/ mpDomainShaderList,   nameIsFullPathAndFilename ); }
    CPUTShaderOGL   *FindGeometryShader(   const cString &name, /*const cString &decoration,*/ bool nameIsFullPathAndFilename=false ) { return   (CPUTShaderOGL   *)FindAsset( name, /*decoration,*/ mpGeometryShaderList,   nameIsFullPathAndFilename ); }

    CPUTResult GetVertexShader(    const cString &name, const cString &shaderMain, const cString &shaderProfile, CPUTShaderOGL   **ppShader, bool nameIsFullPathAndFilename=false, CPUT_SHADER_MACRO *pShaderMacros=NULL);
    CPUTResult GetVertexShader(    const std::vector<cString> &name, const cString &shaderMain, const cString &shaderProfile, CPUTShaderOGL   **ppShader, bool nameIsFullPathAndFilename=false, CPUT_SHADER_MACRO *pShaderMacros=NULL);
    CPUTResult GetPixelShader(     const cString &name, const cString &shaderMain, const cString &shaderProfile, CPUTShaderOGL   **ppShader, bool nameIsFullPathAndFilename=false, CPUT_SHADER_MACRO *pShaderMacros=NULL);
    CPUTResult GetPixelShader(     const std::vector<cString> &name, const cString &shaderMain, const cString &shaderProfile, CPUTShaderOGL       **ppShader, bool nameIsFullPathAndFilename=false, CPUT_SHADER_MACRO *pShaderMacros=NULL);

    CPUTResult GetComputeShader(    const cString &name, const cString &shaderMain, const cString &shaderProfile, CPUTShaderOGL   **ppShader, bool nameIsFullPathAndFilename=false, CPUT_SHADER_MACRO *pShaderMacros=NULL);
    CPUTResult GetComputeShader(    const std::vector<cString> &name, const cString &shaderMain, const cString &shaderProfile, CPUTShaderOGL   **ppShader, bool nameIsFullPathAndFilename=false, CPUT_SHADER_MACRO *pShaderMacros=NULL);

	CPUTResult GetHullShader(    const cString &name, const cString &shaderMain, const cString &shaderProfile, CPUTShaderOGL   **ppShader, bool nameIsFullPathAndFilename=false, CPUT_SHADER_MACRO *pShaderMacros=NULL);
    CPUTResult GetHullShader(    const std::vector<cString> &name, const cString &shaderMain, const cString &shaderProfile, CPUTShaderOGL   **ppShader, bool nameIsFullPathAndFilename=false, CPUT_SHADER_MACRO *pShaderMacros=NULL);
    CPUTResult GetDomainShader(  const cString &name, const cString &shaderMain, const cString &shaderProfile, CPUTShaderOGL   **ppShader, bool nameIsFullPathAndFilename=false, CPUT_SHADER_MACRO *pShaderMacros=NULL);
    CPUTResult GetDomainShader(  const std::vector<cString> &name, const cString &shaderMain, const cString &shaderProfile, CPUTShaderOGL       **ppShader, bool nameIsFullPathAndFilename=false, CPUT_SHADER_MACRO *pShaderMacros=NULL);

	CPUTResult GetGeometryShader(    const cString &name, const cString &shaderMain, const cString &shaderProfile, CPUTShaderOGL   **ppShader, bool nameIsFullPathAndFilename=false, CPUT_SHADER_MACRO *pShaderMacros=NULL);
    CPUTResult GetGeometryShader(    const std::vector<cString> &name, const cString &shaderMain, const cString &shaderProfile, CPUTShaderOGL   **ppShader, bool nameIsFullPathAndFilename=false, CPUT_SHADER_MACRO *pShaderMacros=NULL);
};

#endif // #ifndef __CPUTASSETLIBRARYDX11_H__
