#include "MemoryArena.h"
#include "FromZero_intrinsics.h"

void InitializeArena(Memory_Arena *Arena, size_t Size, uint8 *Base)
{
	Arena->Size = Size;
	Arena->Base = Base;
	Arena->Used = 0;
}

void * PushSize(Memory_Arena *Arena, size_t Size)
{
	assert(Arena->Used + Size <= Arena->Size);
	void *Result = Arena->Base + Arena->Used;
	Arena->Used += Size;

	return Result;
}

void * PushArray(Memory_Arena *Arena, size_t Size, uint32 Count)
{
	return PushSize(Arena, Size * Count);
}