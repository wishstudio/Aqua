#include <cassert>
#include <cstring>
#include <cstdlib>
#include <malloc.h>
#include <varargs.h>
#include <Windows.h>

#include "Object.h"
#include "Resolve.h"

InternHash<InternalString> internalStringHash;
InternHash<String> stringHash;
InternHash<Type> typeHash;

PointerHash1<void, Class> classHash;
PointerHash2<Class, InternalString, Field> fieldHash;
PointerHash3<Class, InternalString, Type, Method> methodHash;

Class *objectClass;
Class *stringClass;

uint32 getTypeSize(Type *type)
{
	switch (type->type)
	{
	case TYPE_INT8: return 1;
	case TYPE_UINT8: return 1;
	case TYPE_INT16: return 2;
	case TYPE_UINT16: return 2;
	case TYPE_INT32: return 4;
	case TYPE_UINT32: return 4;
	case TYPE_INT64: return 8;
	case TYPE_UINT64: return 8;
	case TYPE_FLOAT32: return 4;
	case TYPE_FLOAT64: return 8;
	case TYPE_INT: return 4;
	case TYPE_UINT: return 4;
	case TYPE_BOOL: return 1;
	case TYPE_CHAR: return 2;
	case TYPE_CLASS: return sizeof(pointer);
	case TYPE_ARRAY: return sizeof(pointer);
	default: assert(("Unexpected type.", 0)); return 0;
	}
}

/* INTERNAL CALL */
static Class *loadClassObject(Class *classObject)
{
	BytecodeFile *bytecodeFile = classObject->bytecodeFile;
	ClassDef *classDef = &bytecodeFile->classDefTable[classObject->classId];
	if (classObject == objectClass) /* Core.Object does not have a base class */
		classObject->baseClass = nullptr;
	else
		classObject->baseClass = resolveClass(classObject->bytecodeFile, classDef->baseClassRef);
	classObject->staticSize = 0;
	classObject->staticSize = 0;
	if (classObject->baseClass == nullptr)
		classObject->instanceSize = sizeof(VTable *);
	else
		classObject->instanceSize = classObject->baseClass->instanceSize;
	classObject->fieldCount = classDef->fieldCount;
	classObject->fields = (Field *) malloc(sizeof(Field) * classDef->fieldCount);
	for (uint16 i = 0; i < classDef->fieldCount; i++)
	{
		Field *field = &classObject->fields[i];
		FieldDef *fieldDef = &bytecodeFile->fieldDefTable[classDef->fieldStartIndex + i];
		field->classObject = classObject;
		field->name = resolveInternalString(bytecodeFile, fieldDef->name);
		field->type = resolveType(bytecodeFile, fieldDef->type);
		field->modifier = fieldDef->modifier;
		if (field->modifier & MODIFIER_STATIC)
		{
			/* Static field */
			field->offset = classObject->staticSize;
			classObject->staticSize += getTypeSize(field->type); /* FIXME */
		}
		else
		{
			/* Instance field */
			field->offset = classObject->instanceSize;
			classObject->instanceSize += getTypeSize(field->type); /* FIXME */
		}

		fieldHash.insert(classObject, field->name, field);
	}
	/* Allocate static field */
	classObject->staticData = (char *) malloc(classObject->staticSize);
	memset(classObject->staticData, 0, classObject->staticSize);
	
	uint16 currentSlot;
	if (classObject->baseClass == NULL)
		currentSlot = classObject->vtableSlotCount = 0;
	else
		currentSlot = classObject->vtableSlotCount = classObject->baseClass->vtableSlotCount;
	classObject->methodCount = classDef->methodCount;
	classObject->methods = (Method *) malloc(sizeof(Method) * classDef->methodCount);
	for (uint16 i = 0; i < classDef->methodCount; i++)
	{
		Method *method = &classObject->methods[i];
		MethodDef *methodDef = &bytecodeFile->methodDefTable[classDef->methodStartIndex + i];
		method->classObject = classObject;
		method->name = resolveInternalString(bytecodeFile, methodDef->name);
		method->type = resolveType(bytecodeFile, methodDef->type);
		method->modifier = methodDef->modifier;
		if (method->modifier & MODIFIER_NATIVE)
		{
			HMODULE library = LoadLibraryA(resolveInternalString(bytecodeFile, methodDef->dllName)->data);
			method->callingConvention = methodDef->callingConvention;
			method->nativeMethod = GetProcAddress(library, resolveInternalString(bytecodeFile, methodDef->originalName)->data);
		}
		else
		{
			method->registerCount = methodDef->registerCount;
			method->code = (uint32 *) (bytecodeFile->codeHeap + methodDef->codeDataOffset);
			method->codeSlotCount = methodDef->codeSlotCount;
			method->exceptionClauseCount = methodDef->exceptionClauseCount;
			ExceptionClauseDef *exceptionClauseDef = (ExceptionClauseDef *) (bytecodeFile->codeHeap + methodDef->codeDataOffset + methodDef->codeSlotCount * 4);
			method->exceptionClause = (ExceptionClause *) malloc(sizeof(ExceptionClause) * methodDef->exceptionClauseCount);
			for (uint32 i = 0; i < methodDef->exceptionClauseCount; i++)
			{
				method->exceptionClause[i].type = exceptionClauseDef[i].type;
				method->exceptionClause[i].tryStart = exceptionClauseDef[i].tryStart;
				method->exceptionClause[i].tryEnd = exceptionClauseDef[i].tryEnd;
				method->exceptionClause[i].handlerStart = exceptionClauseDef[i].handlerStart;
				method->exceptionClause[i].handlerEnd = exceptionClauseDef[i].handlerEnd;
				method->exceptionClause[i].catchRegister = exceptionClauseDef[i].catchRegister;
				method->exceptionClause[i].catchClass = resolveClass(bytecodeFile, exceptionClauseDef[i].catchClassRef);
			}
			method->vtableSlot = 0;
			if (method->modifier & MODIFIER_VIRTUAL)
				classObject->vtableSlotCount++;
		}

		methodHash.insert(classObject, method->name, method->type, method);
	}
	/* Now deal with virtual/overriden methods */
	classObject->vtable = (VTable *) malloc(sizeof(VTable) + sizeof(Method *) * classObject->vtableSlotCount);
	classObject->vtable->classObject = classObject;
	if (classObject->baseClass)
	{
		/* Copy vtable of base class */
		memcpy(
			classObject->vtable + sizeof(VTable),
			classObject->baseClass->vtable + sizeof(VTable),
			sizeof(Method *) * classObject->baseClass->vtableSlotCount
		);
	}
	for (uint16 i = 0; i < classDef->methodCount; i++)
	{
		Method *method = &classObject->methods[i];
		if (method->modifier & MODIFIER_OVERRIDE)
		{
			Class *c;
			for (c = classObject->baseClass; c; c = c->baseClass)
			{
				Method *baseMethod = methodHash.find(c, method->name, method->type);
				if (baseMethod)
				{
					method->vtableSlot = baseMethod->vtableSlot;
					classObject->vtable->methods[method->vtableSlot] = method;
					break;
				}
			}
			assert(("Overriding unexisting virtual method.", c));
		}
		else if (method->modifier & MODIFIER_VIRTUAL)
		{
			method->vtableSlot = currentSlot;
			classObject->vtable->methods[currentSlot] = method;
			currentSlot++;
		}
	}

	/* Update load state */
	classObject->loadState = Class::LoadState::Loaded;
	return classObject;
}

/* Resolve an internal string using c style string */
InternalString *resolveInternalString(const char *string)
{	
	/* NOTICE: Make sure internal string is null-terminated */
	uint32 len = strlen(string);
	InternalString *s = (InternalString *) malloc(sizeof(InternalString) + len + 1);
	s->length = len;
	memcpy(s->data, string, len + 1);
	InternalString *si = internalStringHash.findOrCreate(s);
	if (si != s)
		free(s);
	return si;
}

InternalString *resolveInternalString(BytecodeFile *bytecodeFile, uint16 id)
{
	if (bytecodeFile->internalStringTable[id])
		return bytecodeFile->internalStringTable[id];

	InternalString *string = (InternalString *) (bytecodeFile->internalStringHeap + bytecodeFile->internalStringIndexTable[id].offset);
	return (bytecodeFile->internalStringTable[id] = internalStringHash.findOrCreate(string, [](InternalString *string) -> InternalString * {
		/* Make internal string null-terminated */
		InternalString *ret = (InternalString *) malloc(sizeof(InternalString) + string->length + 1);
		memcpy(ret, string, sizeof(InternalString) + string->length);
		ret->data[string->length] = 0;
		return ret;
	}));
}

String *resolveString(BytecodeFile *bytecodeFile, uint16 id)
{
	if (bytecodeFile->stringTable[id])
		return bytecodeFile->stringTable[id];

	/* String value in bytecode does not contain VTable pointer */
	String *string = (String *) (bytecodeFile->stringHeap + bytecodeFile->stringIndexTable[id].offset - sizeof(VTable *));
	return (bytecodeFile->stringTable[id] = stringHash.findOrCreate(string, [](String *string) -> String * {
		return CreateString(string->length, string->data);
	}));
}

/* Resolve or generate a primitive type */
Type *resolvePrimitiveType(uint16 type)
{
	Type primitiveType;
	primitiveType.type = type;
	return typeHash.findOrCreate(&primitiveType, [](Type *type) -> Type * {
		Type *ret = (Type *) malloc(sizeof(Type));
		ret->type = type->type;
		return ret;
	});
}

/* Resolve or generate a class type */
ClassType *resolveClassType(Class *classObject)
{
	ClassType classType;
	classType.type = TYPE_CLASS;
	classType.classObject = classObject;
	return (ClassType *) typeHash.findOrCreate(&classType, [](Type *type) -> Type * {
		ClassType *ret = (ClassType *) malloc(sizeof(ClassType));
		ret->type = ((ClassType *) type)->type;
		ret->classObject = ((ClassType *) type)->classObject;
		return ret;
	});
}

/* INTERNAL CALL */
static FunctionType *_resolveFunctionType(FunctionType *functionType)
{
	return (FunctionType *) typeHash.findOrCreate(functionType, [](Type *type) -> Type * {
		uint16 count = ((FunctionType *) type)->paramCount + ((FunctionType *) type)->returnCount;
		uint32 size = sizeof(FunctionType) + count * sizeof(Type *);
		FunctionType *ret = (FunctionType *) malloc(size);
		memcpy(ret, type, size);
		return ret;
	});
}
								  
/* Resolve or generate a function type */
FunctionType *resolveFunctionType(uint16 paramCount, uint16 returnCount, ...)
{
	FunctionType *functionType = (FunctionType *) alloca(sizeof(FunctionType) + sizeof(Type *) * (paramCount + returnCount));
	functionType->type = TYPE_FUNCTION;
	functionType->paramCount = paramCount;
	functionType->returnCount = returnCount;
	va_list va;
	va_start(va, returnCount);
	for (uint16 i = 0; i < paramCount + returnCount; i++)
		functionType->types[i] = va_arg(va, Type *);
	va_end(va);
	return _resolveFunctionType(functionType);
}

/* Resolve or generate a derived type */
DerivedType *resolveDerivedType(uint16 type, Type *baseType)
{
	DerivedType derivedType;
	derivedType.type = type;
	derivedType.baseType = baseType;
	return (DerivedType *) typeHash.findOrCreate(&derivedType, [](Type *type) -> Type * {
		DerivedType *ret = (DerivedType *) malloc(sizeof(DerivedType));
		ret->type = ((DerivedType *) type)->type;
		ret->baseType = ((DerivedType *) type)->baseType;
		return ret;
	});
}

/* Resolve or generate an array type */
ArrayType *resolveArrayType(Type *elementType)
{
	ArrayType arrayType;
	arrayType.type = TYPE_ARRAY;
	arrayType.elementType = elementType;
	arrayType.arrayClass = NULL;
	return (ArrayType *) typeHash.findOrCreate(&arrayType, [](Type *type) -> Type * {
		ArrayType *ret = (ArrayType *) malloc(sizeof(ArrayType));
		ret->type = TYPE_ARRAY;
		ret->elementType = ((ArrayType *) type)->elementType;
		ret->arrayClass = NULL; /* TODO */
		return ret;
	});
}

Type *resolveType(BytecodeFile *bytecodeFile, uint16 id)
{
	if (bytecodeFile->typeTable[id])
		return bytecodeFile->typeTable[id];

	TypeRef *ref = (TypeRef *) (bytecodeFile->typeHeap + bytecodeFile->typeIndexTable[id].offset);
	Type *resolvedType = nullptr;
	if (ref->type <= TYPE_PRI_MAX)
		bytecodeFile->typeTable[id] = resolvePrimitiveType(ref->type);
	else if (ref->type == TYPE_CLASS)
	{
		/* NOTICE: All possible class objects must at least be present at "Unloaded" state at this point */
		InternalString *className = resolveInternalString(bytecodeFile, ((ClassTypeRef *) ref)->className);
		Class *classObject = classHash.find(className);
		bytecodeFile->typeTable[id] = resolveClassType(classObject);
	}
	else if (ref->type == TYPE_FUNCTION)
	{
		uint16 count = ((FunctionTypeRef *) ref)->paramCount + ((FunctionTypeRef *) ref)->returnCount;
		/* Since the type is variable-sized, we use alloca() to allocate it */
		FunctionType *type = (FunctionType *) alloca(sizeof(FunctionType) + count * sizeof(Type *));
		type->type = TYPE_FUNCTION;
		type->paramCount = ((FunctionTypeRef *) ref)->paramCount;
		type->returnCount = ((FunctionTypeRef *) ref)->returnCount;
		for (uint16 i = 0; i < count; i++)
			type->types[i] = resolveType(bytecodeFile, ((FunctionTypeRef *) ref)->types[i]);
		bytecodeFile->typeTable[id] = _resolveFunctionType(type);
	}
	else /* Derived type */
	{
		Type *baseType = resolveType(bytecodeFile, ((DerivedTypeRef *) ref)->baseType);
		bytecodeFile->typeTable[id] = resolveDerivedType(ref->type, baseType);
	}
	assert(("Resolve type failed.", bytecodeFile->typeTable[id]));
	return bytecodeFile->typeTable[id];
}

Class *resolveClass(InternalString *className)
{
	Class *classObject = classHash.find(className);
	assert(("Class does not exist.", classObject));
	if (classObject->loadState == Class::LoadState::Loaded)
		return classObject;
	else
		return loadClassObject(classObject);
}

/* Resolve class object using given type id, load it if currently not loaded */
Class *resolveClass(BytecodeFile *bytecodeFile, uint16 typeId)
{
	ClassType *type = (ClassType *) resolveType(bytecodeFile, typeId);
	Class *classObject = type->classObject;
	if (classObject->loadState == Class::LoadState::Loaded)
		return classObject;
	else
		return loadClassObject(classObject);
}

Field *resolveField(BytecodeFile *bytecodeFile, uint16 id)
{
	if (bytecodeFile->fieldTable[id])
		return bytecodeFile->fieldTable[id];

	Class *classObject = resolveClass(bytecodeFile, bytecodeFile->fieldRefTable[id].classRef);
	InternalString *name = resolveInternalString(bytecodeFile, bytecodeFile->fieldRefTable[id].name);
	return (bytecodeFile->fieldTable[id] = fieldHash.find(classObject, name));
}

Method *resolveMethod(BytecodeFile *bytecodeFile, uint16 id)
{
	if (bytecodeFile->methodTable[id])
		return bytecodeFile->methodTable[id];

	Class *classObject = resolveClass(bytecodeFile, bytecodeFile->methodRefTable[id].classRef);
	InternalString *name = resolveInternalString(bytecodeFile, bytecodeFile->methodRefTable[id].name);
	Type *type = resolveType(bytecodeFile, bytecodeFile->methodRefTable[id].type);
	return (bytecodeFile->methodTable[id] = methodHash.find(classObject, name, type));
}

void registerInternalMethod(Class *classObject, InternalString *name, Type *type, InternalMethod *internalMethod)
{
	Method *method = methodHash.find(classObject, name, type);
	method->internalMethod = internalMethod;
}

static char *readFile(const wchar_t *fileName, uint32 *fileSize)
{
	HANDLE hFile = CreateFileW(fileName, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
		return nullptr;
	LARGE_INTEGER size;
	if (GetFileSizeEx(hFile, &size) == 0)
	{
		CloseHandle(hFile);
		return nullptr;
	}
	(*fileSize) = (uint32) size.QuadPart;
	char *content = (char *) malloc(*fileSize);
	DWORD bytesRead; /* XP Requires this */
	if (!ReadFile(hFile, content, (*fileSize), &bytesRead, nullptr))
	{
		free(content);
		content = nullptr;
	}
	CloseHandle(hFile);
	return content;
}

#define RAW_STEP(type, size) (raw += (size), (type *) (raw - (size)))
#define RAW_STEP_TABLE(type, count) RAW_STEP(type, sizeof(type) * (count))
#define RAW_STEP_SINGLE(type) RAW_STEP(type, sizeof(type))
void resolveBytecodeFile(const wchar_t *fileName)
{
	uint32 size;
	char *raw = readFile(fileName, &size);

	BytecodeHeader *header = RAW_STEP_SINGLE(BytecodeHeader);
	/* Verify signature and bytecode version */
	assert(header->magic == 'auqA');
	assert(header->version == 0);

	BytecodeFile *bytecodeFile = new BytecodeFile();
	memset(bytecodeFile, 0, sizeof BytecodeFile);
	bytecodeFile->internalStringCount = header->internalStringCount;
	bytecodeFile->stringCount = header->stringCount;
	bytecodeFile->typeCount = header->typeCount;
	bytecodeFile->fieldRefCount = header->fieldRefCount;
	bytecodeFile->methodRefCount = header->methodRefCount;
	bytecodeFile->classDefCount = header->classDefCount;

	/* Index tables */
	bytecodeFile->internalStringIndexTable = RAW_STEP_TABLE(InternalStringIndex, header->internalStringCount);
	bytecodeFile->stringIndexTable = RAW_STEP_TABLE(StringIndex, header->stringCount);
	bytecodeFile->typeIndexTable = RAW_STEP_TABLE(TypeIndex, header->typeCount);
	
	/* Tables */
	bytecodeFile->fieldRefTable = RAW_STEP_TABLE(FieldRef, header->fieldRefCount);
	bytecodeFile->methodRefTable = RAW_STEP_TABLE(MethodRef, header->methodRefCount);
	bytecodeFile->classDefTable = RAW_STEP_TABLE(ClassDef, header->classDefCount);
	bytecodeFile->fieldDefTable = RAW_STEP_TABLE(FieldDef, header->fieldDefCount);
	bytecodeFile->methodDefTable = RAW_STEP_TABLE(MethodDef, header->methodDefCount);

	/* Heaps */
	bytecodeFile->internalStringHeap = RAW_STEP(char, header->internalStringHeapSize);
	bytecodeFile->stringHeap = RAW_STEP(char, header->stringHeapSize);
	bytecodeFile->typeHeap = RAW_STEP(char, header->typeHeapSize);
	bytecodeFile->codeHeap = RAW_STEP(char, header->codeHeapSize);

	/* Preallocate tables */
	bytecodeFile->internalStringTable = (InternalString **) calloc(bytecodeFile->internalStringCount, sizeof(InternalString *));
	bytecodeFile->stringTable = (String **) calloc(bytecodeFile->stringCount, sizeof(String *));
	bytecodeFile->typeTable = (Type **) calloc(bytecodeFile->typeCount, sizeof(Type *));
	bytecodeFile->fieldTable = (Field **) calloc(bytecodeFile->fieldRefCount, sizeof(Field *));
	bytecodeFile->methodTable = (Method **) calloc(bytecodeFile->methodRefCount, sizeof(Method *));

	/* Preallocate class objects */
	/* Fill in minimum amount of information needed by later resolving */
	bytecodeFile->classTable = (Class *) malloc(sizeof(Class) * bytecodeFile->classDefCount);
	for (uint16 i = 0; i < bytecodeFile->classDefCount; i++)
	{
		InternalString *className = resolveInternalString(bytecodeFile, bytecodeFile->classDefTable[i].name);
		Class *classObject = &bytecodeFile->classTable[i];
		classObject->name = className;
		classObject->loadState = Class::LoadState::Unloaded;
		classObject->bytecodeFile = bytecodeFile;
		classObject->classId = i;

		classHash.insert(className, classObject);
	}
}

void initializeResolveCache()
{
	/* objectClass must be declared before resolving Core.Object */
	/* loadClass needs objectClass to know whether the class does not have a base class */
	objectClass = classHash.find(resolveInternalString("Core.Object"));
	stringClass = resolveClass(resolveInternalString("Core.String"));
}
