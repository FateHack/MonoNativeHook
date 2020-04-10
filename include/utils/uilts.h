//
// Created by Fate on 2020-04-10.
//

#ifndef MONONATIVEHOOK_UILTS_H
#define MONONATIVEHOOK_UILTS_H

#include <vector>
#include <mono/metadata/metadata.h>
#include <mono/metadata/class.h>
#include "mono/metadata/assembly.h"
#include <iostream>
#include "mono/metadata/debug-helpers.h"

void split_str(char *str, const char *split, std::vector<char *> &vec);

MonoMethod *get_MonoMethod(std::vector<char *> vector);

std::string getProcName();

#endif //MONONATIVEHOOK_UILTS_H
