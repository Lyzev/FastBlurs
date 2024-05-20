#pragma once

#ifndef _countof
#define _countof(_Array) (sizeof(_Array) / sizeof(_Array[0]))
#endif

#ifdef __cplusplus
extern "C" {
#endif

void PezCheckCondition(int condition, ...);

int glswInit();
int glswShutdown();
int glswSetPath(const char* pathPrefix, const char* pathSuffix);
const char* glswGetShader(const char* effectKey);
const char* glswGetError();
int glswAddDirectiveToken(const char* token, const char* directive);

#ifdef __cplusplus
}
#endif
