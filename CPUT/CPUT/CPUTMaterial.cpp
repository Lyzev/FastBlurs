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
#include "CPUT.h"
#include "CPUTAssetLibrary.h"

#ifdef CPUT_FOR_DX11
#include "CPUTMaterialDX11.h"
#elif (defined(CPUT_FOR_OGL) || defined(CPUT_FOR_OGLES))
#include "CPUTMaterialOGL.h"
#else    
#error "No gfx APi defined"
#endif

//--------------------------------------------------------------------------------------
CPUTMaterial *CPUTMaterial::CreateMaterial(
    const cString   &absolutePathAndFilename,
    const CPUTModel *pModel,
          int        meshIndex,
    const char     **pShaderMacros, // Note: this is honored only on first load.  Subsequent GetMaterial calls will return the material with shaders as compiled with original macros.
          int        numSystemMaterials,
          cString   *pSystemMaterialNames,
          int        externalCount,
          cString   *pExternalName,
          float4    *pExternals,
          int       *pExternalOffset,
          int       *pExternalSize
){
    // Create the material and load it from file.
#ifdef CPUT_FOR_DX11
    CPUTMaterial *pMaterial = new CPUTMaterialDX11();
#elif (defined(CPUT_FOR_OGL) || defined(CPUT_FOR_OGLES))
    CPUTMaterial *pMaterial = new CPUTMaterialOGL();
#else    
    #error You must supply a target graphics API (ex: #define CPUT_FOR_DX11), or implement the target API for this file.
#endif
    pMaterial->mpSubMaterials = NULL;
    CPUTResult result = pMaterial->LoadMaterial( absolutePathAndFilename, pModel, meshIndex, pShaderMacros, numSystemMaterials, pSystemMaterialNames, externalCount, pExternalName, pExternals, pExternalOffset, pExternalSize );
    ASSERT( CPUTSUCCESS(result), _L("\nError - CPUTAssetLibrary::GetMaterial() - Error in material file: '")+absolutePathAndFilename+_L("'") );
    UNREFERENCED_PARAMETER(result);

    // Add the material to the asset library.
    if( pModel && pMaterial->MaterialRequiresPerModelPayload() )
    {
        CPUTAssetLibrary::GetAssetLibrary()->AddMaterial( absolutePathAndFilename, _L(""), _L(""), pMaterial, pShaderMacros, pModel, meshIndex );
    } else
    {
        CPUTAssetLibrary::GetAssetLibrary()->AddMaterial( absolutePathAndFilename, _L(""), _L(""), pMaterial, pShaderMacros );
    }

    return pMaterial;
}
