#pragma once

#if !defined(COMPILER_MSVC)
	#define COMPILER_MSVC 0
#endif // !defined(COMPILER_MSVC)

#if !defined(COMPILER_LLVM)
	#define COMPILER_LLVM 0
#endif // !defined(COMPILER_LLVM)

#if !COMPILER_MSVC && !COMPILER_LLVM
	#if _MSC_VER
		#undef COMPILER_MSVC
		#define COMPILER_MSVC 1 
	#else
		#undef COMPILER_LLVM
		#define COMPILER_LLVM 1
	#endif // _MSC_VER
#endif // !COMPILER_MSVC && !COMPILER_LLVM
