#pragma once

#include <windows.h>

struct Memory_Arena
{
	size_t Size;
	UINT8 *Base;
	size_t Used;
};

void InitializeArena(Memory_Arena *Arena, size_t Size, UINT8 *Base);
void * PushSize(Memory_Arena *Arena, size_t Size);
void * PushArray(Memory_Arena *Arena, size_t Size, UINT32 Count);