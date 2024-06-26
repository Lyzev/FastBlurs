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

#include "CPUTScene.h"
#include "CPUTAssetLibrary.h"

//-----------------------------------------------------------------------------
CPUTResult CPUTScene::LoadScene(const cString &sceneFileName, bool nameIsFullPathAndFilename)
{
    CPUTResult result = CPUT_SUCCESS;
    CPUTAssetLibrary *pAssetLibrary = CPUTAssetLibrary::GetAssetLibrary();


    cString absolutePathAndFilename;
    if( !nameIsFullPathAndFilename )
    {
		DEBUG_PRINT(_L("%s"),pAssetLibrary->GetMediaDirectoryName().c_str());

		CPUTFileSystem::ResolveAbsolutePathAndFilename( pAssetLibrary->GetMediaDirectoryName() + sceneFileName, &absolutePathAndFilename);
    }
    else
    {
        absolutePathAndFilename = sceneFileName;
    }

	DEBUG_PRINT(_L("%s"),absolutePathAndFilename.c_str());

	result = mSceneFile.LoadFile(absolutePathAndFilename);
    if (CPUTFAILED(result)) {
        DEBUG_PRINT(_L("Failed to load scene"));
        return result;
    }

    CPUTConfigBlock *pAssetsBlock = mSceneFile.GetBlockByName(_L("Assets"));
    if (pAssetsBlock == NULL) {
        DEBUG_PRINT(_L("Failed to load Assets"));
        return CPUT_WARNING_NO_ASSETS_LOADED;
    }

    int numAssets = pAssetsBlock->ValueCount();
    if (numAssets <= 0) {
        DEBUG_PRINT(_L("Failed to load Assets"));
        return CPUT_WARNING_NO_ASSETS_LOADED;
    }

    CPUTAssetSet *pAssetSet = NULL;
    for (int i = 0; i < numAssets; ++i) {
         CPUTConfigEntry *pEntry = pAssetsBlock->GetValue(i);
         if (pEntry == NULL) {
             continue;
         }

		 cString resolvedAssetNameAndPath(pEntry->NameAsString());
         CPUTFileSystem::ResolveAbsolutePathAndFilename(pAssetLibrary->GetMediaDirectoryName() + pEntry->NameAsString(), &resolvedAssetNameAndPath);

         //
         // Extract the set file name off the end of the path
         //
         cString delimiters = _L("\\/");
         size_t pos = resolvedAssetNameAndPath.find_last_of(delimiters);
         if (pos == std::string::npos) {
             // then there are no directories in the path provided. There should always be at least /asset/
         }
         cString assetFileName = resolvedAssetNameAndPath.substr(pos + 1); // +1 to skip the '/' or '\' character

         //
         // the assetname will always end in /asset/name.set
         //
         pos = resolvedAssetNameAndPath.rfind(_L("asset"));
         if (pos == std::string::npos) {
             pos = resolvedAssetNameAndPath.rfind(_L("Asset"));
         }
         if (pos == std::string::npos) {
             // then the set file is not in the correct folder
         }
         cString assetFilePath = resolvedAssetNameAndPath.substr(0, pos);

         std::string executableDirectory;
         CPUTFileSystem::GetExecutableDirectory(&executableDirectory);

         pAssetLibrary->SetMediaDirectoryName(assetFilePath);

         pAssetSet  = pAssetLibrary->GetAssetSet(resolvedAssetNameAndPath.c_str(), true); // need to state that this is the fully qualified path name so CPUT will not append a .set extension
        if (!pAssetSet)
            DEBUG_PRINT(_L("Failed to load AssetSet"));// %p", pAssetSet);
         ASSERTA( pAssetSet, "Failed loading" + assetFilePath);
         mpAssetSetList[mNumAssetSets] = pAssetSet;
         mNumAssetSets++;

         ASSERTA(mNumAssetSets <= MAX_NUM_ASSETS, "Number of Assets in scene file exceeds MAX_NUM_ASSETS");
    }

    CalculateBoundingBox();

    return result;
}

//-----------------------------------------------------------------------------
void CPUTScene::CalculateBoundingBox()
{
    mMinExtent.x = mMinExtent.y = mMinExtent.z =  FLT_MAX;
    mMaxExtent.x = mMaxExtent.y = mMaxExtent.z = -FLT_MAX;

    for (UINT i = 0; i < mNumAssetSets; ++i) {
        float3 lookAtPoint(0.0f, 0.0f, 0.0f);
        float3 half(1.0f, 1.0f, 1.0f);
        mpAssetSetList[i]->GetBoundingBox( &lookAtPoint, &half );

        mMinExtent = Min( (lookAtPoint - half), mMinExtent );
        mMaxExtent = Max( (lookAtPoint + half), mMaxExtent );
    }

    mSceneBoundingBoxCenter  = (mMaxExtent + mMinExtent) * 0.5f;
    mSceneBoundingBoxHalf    = (mMaxExtent - mMinExtent) * 0.5f;
}

//-----------------------------------------------------------------------------
void CPUTScene::Render(CPUTRenderParameters &renderParameters, int materialIndex)
{
    for (UINT i = 0; i < mNumAssetSets; ++i)
    {
        mpAssetSetList[i]->RenderRecursive(renderParameters, materialIndex);
    }
}
void CPUTScene::Update( float dt )
{
	for (UINT i = 0; i < mNumAssetSets; ++i)
	{
		mpAssetSetList[i]->UpdateRecursive(dt);
	}
}
