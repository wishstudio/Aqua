#include "CoreLibraryInternal.h"
#include "PlatformTypes.h"
#include "Object.h"
#include "Resolve.h"
#include "VMTypes.h"

/* Core.Object::get_Name() -> Core.String */
static void Core_Object_get_Name(reg *stack)
{
	pointer object = (pointer) POINTER(stack[0]);
	Class *classObject = VTABLE(object)->classObject;
	int32 length = utf8ToUtf16(classObject->name->data, classObject->name->length + 1, NULL);
	String *string = CreateString(length);
	utf8ToUtf16(classObject->name->data, classObject->name->length + 1, string->data);
	POINTER(stack[0]) = (pointer) string;
}

/* static Core.String.AllocateString(int32 length) -> Core.String */
static void Core_String_AllocateString(reg *stack)
{
	int32 length = INT32(stack[0]);
	POINTER(stack[0]) = (pointer) CreateString(length);
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
		objectClass,
		resolveInternalString(".getName"),
		resolveFunctionType(0, 1, resolveClassType(stringClass)),
		Core_Object_get_Name
	);

	registerInternalMethod(
		stringClass,
		resolveInternalString("AllocateString"),
		resolveFunctionType(1, 1, resolvePrimitiveType(TYPE_INT32), resolveClassType(stringClass)),
		Core_String_AllocateString
	);

	Class *nativeConverterClass = resolveClass(resolveInternalString("Core.Native.Converter"));

	Class *nativeMemoryClass = resolveClass(resolveInternalString("Core.Native.Memory"));
	registerInternalMethod(
		nativeMemoryClass,
		resolveInternalString("CopyMemory"),
		resolveFunctionType(3, 0, resolvePrimitiveType(TYPE_INT), resolvePrimitiveType(TYPE_INT), resolvePrimitiveType(TYPE_INT32)),
		Core_Native_Memory_CopyMemory
	);
	
	registerInternalMethod(
		nativeMemoryClass,
		resolveInternalString("MoveMemory"),
		resolveFunctionType(3, 0, resolvePrimitiveType(TYPE_INT), resolvePrimitiveType(TYPE_INT), resolvePrimitiveType(TYPE_INT32)),
		Core_Native_Memory_MoveMemory
	);

	registerInternalMethod(
		nativeMemoryClass,
		resolveInternalString("AllocateMemory"),
		resolveFunctionType(1, 1, resolvePrimitiveType(TYPE_INT32), resolvePrimitiveType(TYPE_INT)),
		Core_Native_Memory_AllocateMemory
	);

	registerInternalMethod(
		nativeMemoryClass,
		resolveInternalString("FreeMemory"),
		resolveFunctionType(1, 0, resolvePrimitiveType(TYPE_INT)),
		Core_Native_Memory_FreeMemory
	);
}
