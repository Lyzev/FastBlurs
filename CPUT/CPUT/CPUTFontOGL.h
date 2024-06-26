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
#ifndef __CPUTFONTOGL_H__
#define __CPUTFONTOGL_H__

#include "CPUT.h"
#include "CPUTRefCount.h"
#include "CPUTFont.h"

class CPUTTextureOGL;



class CPUTFontOGL : public CPUTFont
{
protected:
    ~CPUTFontOGL(); // Destructor is not public.  Must release instead of delete.

public:
    CPUTFontOGL();
    static CPUTFont *CreateFont( cString FontName, cString AbsolutePathAndFilename);

    CPUTTextureOGL *GetAtlasTexture();
//    ID3D11ShaderResourceView *GetAtlasTextureResourceView();


private:
    CPUTTextureOGL            *mpTextAtlas;
//    ID3D11ShaderResourceView   *mpTextAtlasView;

    
    CPUTResult LoadGlyphMappingFile(const cString fileName);



    

};

#endif // #ifndef __CPUTFONTOGL_H__