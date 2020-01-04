#pragma once

#include "FromZero_intrinsics.h"

struct Memory_Arena
{
	size_t Size;
	uint8 *Base;
	size_t Used;
};

void InitializeArena(Memory_Arena *Arena, size_t Size, uint8 *Base);
void * PushSize(Memory_Arena *Arena, size_t Size);
void * PushArray(Memory_Arena *Arena, size_t Size, uint32 Count);