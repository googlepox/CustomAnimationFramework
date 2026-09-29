#pragma once
#include "NiNodes.h"

void Install();

// Credits to lStewieAl
[[nodiscard]] UInt32 __stdcall DetourVtable(UInt32 addr, UInt32 dst);