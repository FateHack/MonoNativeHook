#ifndef MONONATIVEHOOK_LIBRARY_H
#define MONONATIVEHOOK_LIBRARY_H

#include "../mono/metadata/appdomain.h"
#include "../mono/metadata/metadata.h"
#include "../mono/metadata/object.h"


void load_dll(const char* dllPath,const char* imageName,const char* nameSpace, const char* className, const char* methodName,
              int delay);

#endif