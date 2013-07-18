#include "CoreLibraryInternal.h"
#include "PlatformTypes.h"
#include "Object.h"
#include "Resolve.h"
#include "VMTypes.h"

/* static Core.String.AllocateString(int32 length) -> Core.String */
static void Core_String_AllocateString(reg *stack)
{
	int32 size = INT32(stack[0]);
	POINTER(stack[0]) = (pointer) CreateString(size);
}

/* static Core.Native.Memory.CopyMemory(int destination, int source, int32 size) */
static void Core_Native_Memory_CopyMemory(reg *stack)
{
	void *destination = POINTER(stack[0]);
	void *source = POINTER(stack[1]);
	int32 size = INT32(stack[2]);
	memcpy(destination, source, size);
}

/* static Core.Native.Memory.MoveMemory(int destination, int source, int32 size) */
static void Core_Native_Memory_MoveMemory(reg *stack)
{
	void *destination = POINTER(stack[0]);
	void *source = POINTER(stack[1]);
	int32 size = INT32(stack[2]);
	memmove(destination, source, size);
}

/* static Core.Native.Memory.AllocateMemory(int32 size) -> int data */
static void Core_Native_Memory_AllocateMemory(reg *stack)
{
	int32 size = INT32(stack[0]);
	POINTER(stack[0]) = (pointer) malloc(size);
}

/* static Core.Native.Memory.FreeMemory(int data) */
static void Core_Native_Memory_FreeMemory(reg *stack)
{
	free(POINTER(stack[0]));
}

void initializeCoreLibraryInternal()
{
	registerInternalMethod(
		stringClass,
		resolveString("AllocateString"),
		resolveFunctionType(1, 1, resolvePrimitiveType(TYPE_INT32), resolveClassType(stringClass)),
		Core_String_AllocateString
	);

	Class *nativeConverterClass = resolveClass(resolveString("Core.Native.Converter"));

	Class *nativeMemoryClass = resolveClass(resolveString("Core.Native.Memory"));
	registerInternalMethod(
		nativeMemoryClass,
		resolveString("CopyMemory"),
		resolveFunctionType(3, 0, resolvePrimitiveType(TYPE_INT), resolvePrimitiveType(TYPE_INT), resolvePrimitiveType(TYPE_INT32)),
		Core_Native_Memory_CopyMemory
	);
	
	registerInternalMethod(
		nativeMemoryClass,
		resolveString("MoveMemory"),
		resolveFunctionType(3, 0, resolvePrimitiveType(TYPE_INT), resolvePrimitiveType(TYPE_INT), resolvePrimitiveType(TYPE_INT32)),
		Core_Native_Memory_MoveMemory
	);

	registerInternalMethod(
		nativeMemoryClass,
		resolveString("AllocateMemory"),
		resolveFunctionType(1, 1, resolvePrimitiveType(TYPE_INT32), resolvePrimitiveType(TYPE_INT)),
		Core_Native_Memory_AllocateMemory
	);

	registerInternalMethod(
		nativeMemoryClass,
		resolveString("FreeMemory"),
		resolveFunctionType(1, 0, resolvePrimitiveType(TYPE_INT)),
		Core_Native_Memory_FreeMemory
	);
}
