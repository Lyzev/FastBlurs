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

#include "CPUTShaderOGL.h"
#include "CPUTConfigBlock.h"
#include "CPUTMaterial.h"
#include "CPUTOSServices.h"
#include "CPUT_OGL.h"

cString GenerateName(const std::vector<cString> &fileNames)
{
    return fileNames[0];
}

CPUTShaderOGL *CPUTShaderOGL::CreateShaderFromFiles(
        const std::vector<cString> &fileNames,
        GLuint shaderType,
        char* glslVersion,
        CPUT_SHADER_MACRO *pGlobalMacros,
        CPUT_SHADER_MACRO *pShaderMacros
    )
{
    GenerateName(fileNames);
    DEBUG_PRINT(_L("creating shader: %s"), fileNames[0].c_str());

    GLuint shader = 0;
    int maxLength = 0;
    int nBytes = 0;
    char *infoLog = NULL;
    //NOTE: This will specify shader version and shader type as first two lines of shader
    //TODO: Maybe use the PixelShaderProfile to determine version number or some other mechanism
    unsigned int files = fileNames.size();
    
    const int MACRO_FILES = 4; //one for version, one for system, one for material

    //fixme allocate dynamically
    char* source[20];
    source[0] = glslVersion;
    //FIXME: remove these defines - support separate defines for separate shaders.
    if(shaderType == GL_FRAGMENT_SHADER)
    {
        source[1] = "\n#define GLSL_FRAGMENT_SHADER\n";
    }
    else if(shaderType == GL_VERTEX_SHADER)
    {
        source[1] = "\n#define GLSL_VERTEX_SHADER\n";
    }
    else if(shaderType == GL_TESS_CONTROL_SHADER)
	{
		source[1] = "\n#define GLSL_TESS_CONTROL_SHADER\n";
	}
	else if(shaderType == GL_TESS_EVALUATION_SHADER)
	{
		source[1] = "\n#define GLSL_TESS_EVALUATION_SHADER\n";
	}
	else if(shaderType == GL_GEOMETRY_SHADER)
	{
		source[1] = "\n#define GLSL_GEOMETRY_SHADER\n";
	}
	else if(shaderType == GL_COMPUTE_SHADER)
	{
		source[1] = "\n#define GLSL_COMPUTE_SHADER\n";
	}
	else
	{
        source[1] = NULL;
    }
    source[2] = ConvertShaderMacroToChar(pGlobalMacros);
    source[3] = ConvertShaderMacroToChar(pShaderMacros);
    // Read our shaders into the appropriate buffers
    for(int i = 0; i < (int)files; i++)
    {
        CPUTResult result = CPUTFileSystem::ReadFileContents(fileNames[i].c_str(), (UINT *)&nBytes, (void **)&source[i+MACRO_FILES], true);
        if(CPUTFAILED(result))
        {
            DEBUG_PRINT(_L("Failed to read file %s"), fileNames[i].c_str());
		    ASSERT(false, _L("Fragment shader failed to compile"));
            return NULL;
        }
    }
    // Create an empty fragment shader handle
    shader = glCreateShader(shaderType);
	DEBUG_PRINT(_L("Created Shader: %d"), shader);
    // Send the fragment shader source code to GL
    // Note that the source code is NULL character terminated.
    // GL will automatically detect that therefore the length info can be 0 in this case (the last parameter)
    glShaderSource(shader, files+MACRO_FILES, (const GLchar**)&source, 0);
 
    SAFE_DELETE(source[2]);
    SAFE_DELETE(source[3]);
    for(int i = 0; i < (int)files; i++)
    {
        delete source[i+MACRO_FILES];
    } 
    // Compile the fragment shader
    GL_CHECK(glCompileShader(shader));
    GLint isCompiled;
    GL_CHECK(glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled));
    if(isCompiled == false)
    {
        GL_CHECK(glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength));
 
        // The maxLength includes the NULL character
        infoLog = (char *)malloc(maxLength*40);
 
        glGetShaderInfoLog(shader, maxLength, &maxLength, infoLog);
 
        // Handle the error in an appropriate way such as displaying a message or writing to a log file.
        // In this simple program, we'll just leave
        DEBUG_PRINT_ALWAYS((_L("Failed to compile shader:\n%s\n"), infoLog));
		ASSERT(false, _L("Compile frag shader failed"));

        free(infoLog);
    }


    CPUTShaderOGL *pNewCPUTShader = new CPUTShaderOGL( shader );
    
    // add shader to library
    CPUTAssetLibraryOGL *pAssetLibrary = (CPUTAssetLibraryOGL*)CPUTAssetLibrary::GetAssetLibrary();
    cString libName;
    for(unsigned int i = 0; i < fileNames.size(); i++)
    {
        libName += fileNames[i];
    }
    pAssetLibrary->AddPixelShader(libName, pNewCPUTShader);
    return pNewCPUTShader;

}

CPUTShaderOGL *CPUTShaderOGL::CreateShaderFromMemory(
        const std::vector<char*>     &source,
        const cString     &shaderProfile,
        CPUT_SHADER_MACRO *pShaderMacros
    )
{
    return NULL;
}

//-----------------------------------------------------------------------------
bool CPUTShaderOGL::ShaderRequiresPerModelPayload( CPUTConfigBlock &properties )
{
    return false;
}

char* ConvertShaderMacroToChar(CPUT_SHADER_MACRO *pShaderMacros)
{
	char* pOutput = NULL;
	char* pDefineText = "#define ";
	CPUT_SHADER_MACRO* pCurrentMacro = pShaderMacros;
	//first find string length
	int length = 0;
	while(pShaderMacros != NULL && pCurrentMacro->Name != NULL)
	{
		length += strlen(pDefineText);
		length += strlen(pCurrentMacro->Name);
		length += strlen(pCurrentMacro->Definition);
		length += 2; // for " " and "\n"
		pCurrentMacro++;
	}
	length += 1; // for NULL terminator
	pOutput = new char[length];
	char* pEnd = pOutput;
	pCurrentMacro = pShaderMacros;
	while(pShaderMacros != NULL && pCurrentMacro->Name != NULL)
	{
		strcpy(pEnd, pDefineText);
		pEnd += strlen(pDefineText);
		strcpy(pEnd, pCurrentMacro->Name);
		pEnd += strlen(pCurrentMacro->Name);
		*pEnd = ' ';
		pEnd++;
		strcpy(pEnd, pCurrentMacro->Definition);
		pEnd += strlen(pCurrentMacro->Definition);
		*pEnd = '\n';
		pEnd++;
		pCurrentMacro++;
	}
	*pEnd = 0;
	return pOutput;
}
