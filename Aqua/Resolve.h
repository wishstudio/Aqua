#pragma once

#include "Hash.h"
#include "PlatformTypes.h"
#include "VMTypes.h"

extern InternHash<InternalString> internalStringHash;
extern InternHash<Type> typeHash;

extern PointerHash1<void, Class> classHash;
extern PointerHash2<Class, InternalString, Field> fieldHash;
extern PointerHash3<Class, InternalString, Type, Method> methodHash;

extern Class *objectClass;
extern Class *stringClass;

uint32 sizeOf(Type *type);

InternalString *resolveInternalString(const char *string);
InternalString *resolveInternalString(BytecodeFile *bytecodeFile, uint16 id);

String *resolveString(BytecodeFile *bytecodeFile, uint16 id);

Type *resolvePrimitiveType(uint16 type);
ClassType *resolveClassType(Class *classObject);
FunctionType *resolveFunctionType(uint16 paramCount, uint16 returnCount, ...);
DerivedType *resolveDerivedType(uint16 type, Type *baseType);
ArrayType *resolveArrayType(Type *elementType);
Type *resolveType(BytecodeFile *bytecodeFile, uint16 id);

Class *resolveClass(InternalString *className);
Class *resolveClass(BytecodeFile *bytecodeFile, uint16 typeId);

Field *resolveField(BytecodeFile *bytecodeFile, uint16 id);

Method *resolveMethod(BytecodeFile *bytecodeFile, uint16 id);
void registerInternalMethod(Class *classObject, InternalString *name, Type *type, InternalMethod *internalMethod);

void resolveBytecodeFile(const wchar_t *fileName);
void initializeResolveCache();
