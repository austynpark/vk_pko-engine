#pragma once

#include <stdint.h>
// Unsigned int types.
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

// Signed int types.
typedef signed char i8;
typedef signed short i16;
typedef signed int i32;
typedef signed long long i64;

// Floating point types
typedef float f32;
typedef double f64;

// Boolean types
typedef int b32;
typedef bool b8;

// Properly define static assertions.
#if defined(__clang__) || defined(__gcc__)
#define STATIC_ASSERT _Static_assert
#else
#define STATIC_ASSERT static_assert
#endif

// Ensure all types are of the correct size.
STATIC_ASSERT(sizeof(u8) == 1, "Expected u8 to be 1 byte.");
STATIC_ASSERT(sizeof(u16) == 2, "Expected u16 to be 2 bytes.");
STATIC_ASSERT(sizeof(u32) == 4, "Expected u32 to be 4 bytes.");
STATIC_ASSERT(sizeof(u64) == 8, "Expected u64 to be 8 bytes.");

STATIC_ASSERT(sizeof(i8) == 1, "Expected i8 to be 1 byte.");
STATIC_ASSERT(sizeof(i16) == 2, "Expected i16 to be 2 bytes.");
STATIC_ASSERT(sizeof(i32) == 4, "Expected i32 to be 4 bytes.");
STATIC_ASSERT(sizeof(i64) == 8, "Expected i64 to be 8 bytes.");

STATIC_ASSERT(sizeof(f32) == 4, "Expected f32 to be 4 bytes.");
STATIC_ASSERT(sizeof(f64) == 8, "Expected f64 to be 8 bytes.");

#define ENUM_FLAGS_OPERATOR(VALUE_TYPE, ENUM_TYPE) \
	inline ENUM_TYPE operator& (ENUM_TYPE a, ENUM_TYPE b) \
	{return (ENUM_TYPE)((VALUE_TYPE)a & (VALUE_TYPE)b);}	\
	inline ENUM_TYPE operator| (ENUM_TYPE a, ENUM_TYPE b) \
	{return (ENUM_TYPE)((VALUE_TYPE)a | (VALUE_TYPE)b);} \
	inline ENUM_TYPE& operator&= (ENUM_TYPE& a, ENUM_TYPE b) \
	{ENUM_TYPE result = (ENUM_TYPE)((VALUE_TYPE)a & (VALUE_TYPE)b); return result;} \
	inline ENUM_TYPE& operator|= (ENUM_TYPE& a, ENUM_TYPE b) \
	{ENUM_TYPE result = (ENUM_TYPE)((VALUE_TYPE)a | (VALUE_TYPE)b); return result;}

#if defined (_MSC_VER)
#define PKO_EXPORT __declspec(dllexport)
#define PKO_IMPORT __declspec(dllimport)
#elif defined (__GNUC__)
#define PKO_EXPORT __attribute__((visibility("default")))
#define PKO_IMPORT
#endif //_MSC_VER


