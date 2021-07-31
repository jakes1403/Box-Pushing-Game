/*
Copyright (c) 2019-2021 Jacob Allen
Released under the MIT license
*/

#pragma once

#include <lua.hpp>
#include <string>

extern lua_State* state;

std::string LI_GetStringVarValue(const char* varName);

void LI_SetStringVarValue(const char* varName, std::string varValue);

void LI_SetBoolVarValue(const char* varName, bool varValue);

void LI_SetFloatVarValue(const char* varName, float varValue);

void LI_SetIntVarValue(const char* varName, int varValue);

bool LI_GetBoolVarValue(const char* varName);

int LI_GetIntVarValue(const char* varName);

float LI_GetFloatVarValue(const char* varName);

void LI_RunFunction(const char* funcName);

void LI_Init();

void LI_ExecuteLua(const char* filename);

void LI_ExecuteString(const char* string);

