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
#ifndef __CPUTASSETLIBRARY_H__
#define __CPUTASSETLIBRARY_H__

#include "CPUT.h"
#include "CPUTOSServices.h" // TODO: why is this windows-specific?

// Global Asset Library
//
// The purpose of this library is to keep a copy of all loaded assets and
// provide a one-stop-loading system.  All assets that are loaded into the
// system via the Getxxx() operators stays in the library.  Further Getxxx()
// operations on an already loaded object will addref and return the previously
// loaded object.
//-----------------------------------------------------------------------------
// node that holds a single library object
class CPUTModel;
struct CPUTAssetListEntry
{
    UINT                hash;
    cString             name;
	cString             fileName; // potentially non-unique
    const CPUTModel    *pModel;
    int                 meshIndex;
    void               *pData;
    char              **pShaderMacros;
    CPUTAssetListEntry *pNext;
};
#define SAFE_RELEASE_LIST(list) ReleaseList(list);(list)=NULL;
class CPUTRenderNode;
class CPUTAssetSet;
class CPUTNullNode;
class CPUTModel;
class CPUTMaterial;
class CPUTLight;
class CPUTCamera;
class CPUTTexture;
class CPUTBuffer;
class CPUTRenderStateBlock;
class CPUTFont;
class CPUTNodeAnimation;
class CPUTAnimation;

class CPUTAssetLibrary
{
protected:
    static CPUTAssetLibrary *mpAssetLibrary;

    // simple linked lists for now, but if we want to optimize or load blocks
    // we can change these to dynamically re-sizing arrays and then just do
    // memcopies into the structs.
    // Note: No camera, light, or NullNode directory names - they don't have payload files (i.e., they are defined completely in the .set file)
    cString  mMediaDirectoryName;
    cString  mAssetSetDirectoryName;
    cString  mModelDirectoryName;
    cString  mMaterialDirectoryName;
    cString  mTextureDirectoryName;
    cString  mShaderDirectoryName;
    cString  mFontDirectoryName;
    cString  mSystemDirectoryName;
	cString  mAnimationSetDirectoryName;
public: // TODO: temporary for debug.
    // TODO: Make these lists static.  Share assets (e.g., texture) across all requests for this process.
    static CPUTAssetListEntry  *mpAssetSetList;
    static CPUTAssetListEntry  *mpNullNodeList;
    static CPUTAssetListEntry  *mpModelList;
    static CPUTAssetListEntry  *mpCameraList;
    static CPUTAssetListEntry  *mpLightList;
    static CPUTAssetListEntry  *mpMaterialList;
    static CPUTAssetListEntry  *mpTextureList;
    static CPUTAssetListEntry  *mpBufferList;
    static CPUTAssetListEntry  *mpConstantBufferList;
    static CPUTAssetListEntry  *mpRenderStateBlockList;
    static CPUTAssetListEntry  *mpFontList;
	static CPUTAssetListEntry  *mpAnimationSetList;

    static CPUTAssetListEntry  *mpAssetSetListTail;
    static CPUTAssetListEntry  *mpNullNodeListTail;
    static CPUTAssetListEntry  *mpModelListTail;
    static CPUTAssetListEntry  *mpCameraListTail;
    static CPUTAssetListEntry  *mpLightListTail;
    static CPUTAssetListEntry  *mpMaterialListTail;
    static CPUTAssetListEntry  *mpTextureListTail;
    static CPUTAssetListEntry  *mpBufferListTail;
    static CPUTAssetListEntry  *mpConstantBufferListTail;
    static CPUTAssetListEntry  *mpRenderStateBlockListTail;
    static CPUTAssetListEntry  *mpFontListTail;
	static CPUTAssetListEntry  *mpAnimationSetListTail;

    static void RebindTexturesAndBuffers();
    static void ReleaseTexturesAndBuffers();

public:
    static CPUTAssetLibrary *GetAssetLibrary();
    static void              DeleteAssetLibrary();
    void PrintAssetLibrary();

    CPUTAssetLibrary() {}
    virtual ~CPUTAssetLibrary() {}

    // Add/get/delete items to specified library
    void *FindAsset(
        const cString      &name,
        CPUTAssetListEntry *pList,
        bool                nameIsFullPathAndFilename=false,
        const char        **pShaderMacros=NULL,
        const CPUTModel    *pModel=NULL,
        int                 meshIndex=-1
    );
    void *FindAssetByName(
        const cString      &name,
        CPUTAssetListEntry *pList
    );

    virtual void ReleaseAllLibraryLists();

    void SetMediaDirectoryName( const std::wstring &directoryName)
    {
        cString localDirectoryName = ws2cs(directoryName);

        mMediaDirectoryName    = localDirectoryName;
#ifndef CPUT_OS_WINDOWS
        mAssetSetDirectoryName 		= localDirectoryName + _L("Asset/");
        mModelDirectoryName    		= localDirectoryName + _L("Asset/");
        mMaterialDirectoryName 		= localDirectoryName + _L("Material/");
        mTextureDirectoryName  		= localDirectoryName + _L("Texture/");
        mShaderDirectoryName   		= localDirectoryName + _L("Shader/");
        mAnimationSetDirectoryName 	= localDirectoryName + _L("Animation/");
#else
        mAssetSetDirectoryName 		= localDirectoryName + _L("Asset/");
        mModelDirectoryName    		= localDirectoryName + _L("Asset/");
        mMaterialDirectoryName 		= localDirectoryName + _L("Material/");
        mTextureDirectoryName  		= localDirectoryName + _L("Texture/");
        mShaderDirectoryName   		= localDirectoryName + _L("Shader/");
        mAnimationSetDirectoryName 	= localDirectoryName + _L("Animation/");
#endif
        // mFontStateBlockDirectoryName   = directoryName + _L("Font\\");
    }
    void SetMediaDirectoryName( const std::string &directoryName)
    {
        cString localDirectoryName = s2cs(directoryName);

        mMediaDirectoryName    = localDirectoryName;
#ifndef CPUT_OS_WINDOWS
        mAssetSetDirectoryName = localDirectoryName + _L("Asset/");
        mModelDirectoryName    = localDirectoryName + _L("Asset/");
        mMaterialDirectoryName = localDirectoryName + _L("Material/");
        mTextureDirectoryName  = localDirectoryName + _L("Texture/");
        mShaderDirectoryName   = localDirectoryName + _L("Shader/");
		mAnimationSetDirectoryName= localDirectoryName + _L("Animation/");
#else
        mAssetSetDirectoryName = localDirectoryName + _L("Asset\\");
        mModelDirectoryName    = localDirectoryName + _L("Asset\\");
        mMaterialDirectoryName = localDirectoryName + _L("Material\\");
        mTextureDirectoryName  = localDirectoryName + _L("Texture\\");
        mShaderDirectoryName   = localDirectoryName + _L("Shader\\");
        mAnimationSetDirectoryName= localDirectoryName + _L("Animation\\");
#endif
        // mFontStateBlockDirectoryName   = directoryName + _L("Font/");
    }
    void SetAssetSetDirectoryName(        const cString &directoryName) { mAssetSetDirectoryName  = directoryName; }
    void SetModelDirectoryName(           const cString &directoryName) { mModelDirectoryName     = directoryName; }
    void SetMaterialDirectoryName(        const cString &directoryName) { mMaterialDirectoryName  = directoryName; }
    void SetTextureDirectoryName(         const cString &directoryName) { mTextureDirectoryName   = directoryName; }
    void SetShaderDirectoryName(          const cString &directoryName) { mShaderDirectoryName    = directoryName; }
    void SetFontDirectoryName(            const cString &directoryName) { mFontDirectoryName      = directoryName; }
	void SetAnimationSetDirectoryName(      const cString &directoryName) { mAnimationSetDirectoryName = directoryName; }
    void SetAllAssetDirectoryNames(       const cString &directoryName) {
            mAssetSetDirectoryName       = directoryName;
            mModelDirectoryName          = directoryName;
            mMaterialDirectoryName       = directoryName;
            mTextureDirectoryName        = directoryName;
            mShaderDirectoryName         = directoryName;
            mFontDirectoryName           = directoryName;
			mAnimationSetDirectoryName	 = directoryName;
    };
    void SetSystemDirectoryName(          const cString &directoryName ) { mSystemDirectoryName   = directoryName; }

    const cString &GetMediaDirectoryName()    { return mMediaDirectoryName; }
    const cString &GetAssetSetDirectoryName() { return mAssetSetDirectoryName; }
    const cString &GetModelDirectoryName()    { return mModelDirectoryName; }
    const cString &GetMaterialDirectoryName() { return mMaterialDirectoryName; }
    const cString &GetTextureDirectoryName()  { return mTextureDirectoryName; }
    const cString &GetShaderDirectoryName()   { return mShaderDirectoryName; }
    const cString &GetFontDirectoryName()     { return mFontDirectoryName; }
    const cString &GetSystemDirectoryName()   { return mSystemDirectoryName; }
	const cString &GetAnimationSetDirectoryName(){ return mAnimationSetDirectoryName; }

    void AddAssetSet(        const cString &name, const cString prefixDecoration, const cString suffixDecoration, CPUTAssetSet         *pAssetSet)        { AddAsset( name, prefixDecoration, suffixDecoration, pAssetSet,         &mpAssetSetList,         &mpAssetSetListTail ); }
    void AddNullNode(        const cString &name, const cString prefixDecoration, const cString suffixDecoration, CPUTNullNode         *pNullNode)        { AddAsset( name, prefixDecoration, suffixDecoration, pNullNode,         &mpNullNodeList,         &mpNullNodeListTail ); }
    void AddModel(           const cString &name, const cString prefixDecoration, const cString suffixDecoration, CPUTModel            *pModel)           { AddAsset( name, prefixDecoration, suffixDecoration, pModel,            &mpModelList,            &mpModelListTail    ); }
    void AddLight(           const cString &name, const cString prefixDecoration, const cString suffixDecoration, CPUTLight            *pLight)           { AddAsset( name, prefixDecoration, suffixDecoration, pLight,            &mpLightList,            &mpLightListTail    ); }
    void AddCamera(          const cString &name, const cString prefixDecoration, const cString suffixDecoration, CPUTCamera           *pCamera)          { AddAsset( name, prefixDecoration, suffixDecoration, pCamera,           &mpCameraList,           &mpCameraListTail   ); }
    void AddFont(            const cString &name, const cString prefixDecoration, const cString suffixDecoration, CPUTFont             *pFont)            { AddAsset( name, prefixDecoration, suffixDecoration, pFont,             &mpFontList,             &mpFontListTail     ); }
	void AddAnimationSet(    const cString &name, const cString prefixDecoration, const cString suffixDecoration, CPUTAnimation	       *pAnimationSet)	  { AddAsset( name, prefixDecoration, suffixDecoration, pAnimationSet,     &mpAnimationSetList,		&mpAnimationSetListTail); }
    void AddRenderStateBlock(const cString &name, const cString prefixDecoration, const cString suffixDecoration, CPUTRenderStateBlock *pRenderStateBlock){ AddAsset( name, prefixDecoration, suffixDecoration, pRenderStateBlock, &mpRenderStateBlockList, &mpRenderStateBlockListTail ); }
    void AddMaterial(        const cString &name, const cString prefixDecoration, const cString suffixDecoration, CPUTMaterial *pMaterial, const char **pShaderMacros=NULL, const CPUTModel *pModel=NULL, int meshIndex=-1){ AddAsset( name, prefixDecoration, suffixDecoration, pMaterial, &mpMaterialList,       &mpMaterialListTail,       pShaderMacros, pModel, meshIndex ); }
    void AddTexture(         const cString &name, const cString prefixDecoration, const cString suffixDecoration, CPUTTexture  *pTexture,                                   const CPUTModel *pModel=NULL, int meshIndex=-1){ AddAsset( name, prefixDecoration, suffixDecoration, pTexture,  &mpTextureList,        &mpTextureListTail,        NULL,          pModel, meshIndex ); }
    void AddBuffer(          const cString &name, const cString prefixDecoration, const cString suffixDecoration, CPUTBuffer   *pBuffer,                                    const CPUTModel *pModel=NULL, int meshIndex=-1){ AddAsset( name, prefixDecoration, suffixDecoration, pBuffer,   &mpBufferList,         &mpBufferListTail,         NULL,          pModel, meshIndex ); }
    void AddConstantBuffer(  const cString &name, const cString prefixDecoration, const cString suffixDecoration, CPUTBuffer   *pBuffer,                                    const CPUTModel *pModel=NULL, int meshIndex=-1){ AddAsset( name, prefixDecoration, suffixDecoration, pBuffer,   &mpConstantBufferList, &mpConstantBufferListTail, NULL,          pModel, meshIndex ); }

    CPUTModel     *FindModel(                   const cString &name, bool nameIsFullPathAndFilename=false)																					{ return (CPUTModel*)           FindAsset( name, mpModelList,            nameIsFullPathAndFilename ); }
    CPUTAssetSet  *FindAssetSet(                const cString &name, bool nameIsFullPathAndFilename=false)																					{ return (CPUTAssetSet*)        FindAsset( name, mpAssetSetList,         nameIsFullPathAndFilename ); }
    CPUTTexture   *FindTexture(                 const cString &name, bool nameIsFullPathAndFilename=false)																					{ return (CPUTTexture*)         FindAsset( name, mpTextureList,          nameIsFullPathAndFilename ); }
    CPUTNullNode  *FindNullNode(                const cString &name, bool nameIsFullPathAndFilename=false)																					{ return (CPUTNullNode*)        FindAsset( name, mpNullNodeList,         nameIsFullPathAndFilename ); }
    CPUTLight     *FindLight(                   const cString &name, bool nameIsFullPathAndFilename=false)																					{ return (CPUTLight*)           FindAsset( name, mpLightList,            nameIsFullPathAndFilename ); }
    CPUTCamera    *FindCamera(                  const cString &name, bool nameIsFullPathAndFilename=false)																					{ return (CPUTCamera*)          FindAsset( name, mpCameraList,           nameIsFullPathAndFilename ); }
    CPUTFont      *FindFont(                    const cString &name, bool nameIsFullPathAndFilename=false)																					{ return (CPUTFont*)            FindAsset( name, mpFontList,             nameIsFullPathAndFilename ); }
	CPUTAnimation *FindAnimation(            const cString &name, bool nameIsFullPathAndFilename=false)																					{ return (CPUTAnimation*)       FindAsset( name, mpAnimationSetList,     nameIsFullPathAndFilename ); }
    CPUTRenderStateBlock *FindRenderStateBlock( const cString &name, bool nameIsFullPathAndFilename=false)																					{ return (CPUTRenderStateBlock*)FindAsset( name, mpRenderStateBlockList, nameIsFullPathAndFilename ); }
    CPUTBuffer   *FindBuffer(                   const cString &name, bool nameIsFullPathAndFilename=false, const CPUTModel *pModel=NULL, int meshIndex=-1)									{ return (CPUTBuffer*)          FindAsset( name, mpBufferList,           nameIsFullPathAndFilename, NULL,          pModel, meshIndex ); }
    CPUTBuffer   *FindConstantBuffer(           const cString &name, bool nameIsFullPathAndFilename=false, const CPUTModel *pModel=NULL, int meshIndex=-1)									{ return (CPUTBuffer*)          FindAsset( name, mpConstantBufferList,   nameIsFullPathAndFilename, NULL,          pModel, meshIndex ); }
    CPUTMaterial *FindMaterial(                 const cString &name, bool nameIsFullPathAndFilename=false, const char **pShaderMacros=NULL, const CPUTModel *pModel=NULL, int meshIndex=-1) { return (CPUTMaterial*)        FindAsset( name, mpMaterialList,         nameIsFullPathAndFilename, pShaderMacros, pModel, meshIndex ); }

	// If the asset exists, these 'Get' methods will addref and return it. Otherwise,
	// they will return NULL.
    CPUTAssetSet         *GetAssetSetByName(         const cString &name);
	CPUTModel            *GetModelByName(            const cString &name);
    CPUTTexture          *GetTextureByName(          const cString &name);
    CPUTNullNode         *GetNullNodeByName(         const cString &name);
    CPUTLight            *GetLightByName(            const cString &name);
    CPUTCamera           *GetCameraByName(           const cString &name);
    CPUTFont             *GetFontByName(             const cString &name);
	CPUTAnimation	     *GetAnimationSetByName(     const cString &name);
    CPUTRenderStateBlock *GetRenderStateBlockByName( const cString &name);
    CPUTBuffer           *GetBufferByName(           const cString &name);
    CPUTBuffer           *GetConstantBufferByName(   const cString &name);
    CPUTMaterial         *GetMaterialByName(         const cString &name);

    // If the asset exists, these 'Get' methods will addref and return it.  Otherwise,
    // they will create it and return it.
    CPUTAssetSet         *GetAssetSet(        const cString &name, bool nameIsFullPathAndFilename=false, int numSystemMaterials=0, cString *pSystemMaterialNames=NULL );
    CPUTModel            *GetModel(           const cString &name, bool nameIsFullPathAndFilename=false );
    CPUTTexture          *GetTexture(         const cString &name, bool nameIsFullPathAndFilename=false, bool loadAsSRGB=true );
    CPUTRenderStateBlock *GetRenderStateBlock(const cString &name, bool nameIsFullPathAndFilename=false);
    CPUTBuffer           *GetBuffer(          const cString &name, const CPUTModel *pModel=NULL, int meshIndex=-1 );
    CPUTBuffer           *GetConstantBuffer(  const cString &name, const CPUTModel *pModel=NULL, int meshIndex=-1 );
    CPUTCamera           *GetCamera(          const cString &name );
    CPUTLight            *GetLight(           const cString &name );
    CPUTFont             *GetFont(            const cString &name );
    CPUTMaterial         *GetMaterial(
        const cString   &name,
              bool       nameIsFullPathAndFilename=false,
        const CPUTModel *pModel=NULL,
              int        meshIndex=-1,
        const char     **pShaderMacros=NULL,
              int        numSystemMaterials=0,
              cString   *pSystemMaterialNames=NULL,
              cString   *pExternalNam=NULL,
              float4    *pExternals=NULL,
              int       *pExternalOffset=NULL,
              int       *pExternalSize=NULL,
              int        externalCount=0
    );
	CPUTAnimation	 *GetAnimation(	  const cString &name, bool nameIsFullPathAndFilename=false );

protected:
    // helper functions
    void ReleaseList(CPUTAssetListEntry *pLibraryRoot);
    void AddAsset(         const cString &name, const cString &prefixDecoration, const cString &suffixDecoration, void *pAsset, CPUTAssetListEntry **pHead, CPUTAssetListEntry **pTail, const char **pShaderMacros=NULL, const CPUTModel *pModel=NULL, int meshIndex=-1 );

    UINT CPUTComputeHash( const cString &string )
    {
        size_t length = string.length();
        UINT hash = 0;
        for( size_t ii=0; ii<length; ii++ )
        {
            hash += tolow(string[ii]);
        }
        return hash;
    }
};

#endif //#ifndef __CPUTASSETLIBRARY_H__


