#pragma once

#include <cstdlib>
#include <cstring>

#include "PlatformTypes.h"
#include "String.h"

#pragma warning(disable: 4200)

#define TYPE_UNKNOWN	0
#define TYPE_INT8		1
#define TYPE_UINT8		2
#define TYPE_INT16		3
#define TYPE_UINT16		4
#define TYPE_INT32		5
#define TYPE_UINT32		6
#define TYPE_INT64		7
#define TYPE_UINT64		8
#define TYPE_FLOAT32	9
#define TYPE_FLOAT64	10
#define TYPE_INT		11
#define TYPE_UINT		12
#define TYPE_BOOL		13
#define TYPE_CHAR		14
#define TYPE_PRI_MAX	15
#define TYPE_CLASS		16
#define TYPE_FUNCTION	17
#define TYPE_ARRAY		18
#define TYPE_POINTER	19

#define MODIFIER_PUBLIC		0x0001
#define MODIFIER_PRIVATE	0x0002
#define MODIFIER_PROTECTED	0x0004
#define MODIFIER_STATIC		0x0008
#define MODIFIER_VIRTUAL	0x0010
#define MODIFIER_OVERRIDE	0x0020
#define MODIFIER_ABSTRACT	0x0040
#define MODIFIER_INTERNAL	0x0100
#define MODIFIER_NATIVE		0x0200
#define MODIFIER_INTERFACE	0x1000
#define MODIFIER_VALUETYPE	0x2000
#define MODIFIER_STATICCTOR	0x8000

#define EXCEPTION_CLAUSE_CATCH		0
#define EXCEPTION_CLAUSE_FINALLY	1

#define PROPERTY_GETTER		1
#define PROPERTY_SETTER		2

struct BytecodeHeader
{
	/* signature */
	uint32 magic;
	uint32 version;

	/* table sizes */
	uint16 internalStringCount;
	uint16 stringCount;
	uint16 typeCount;
	uint16 fieldRefCount;
	uint16 methodRefCount;
	uint16 classDefCount;
	uint16 fieldDefCount;
	uint16 methodDefCount;
	uint16 propertyDefCount;
	uint16 _padding;

	/* heap sizes */
	uint32 internalStringHeapSize;
	uint32 stringHeapSize;
	uint32 typeHeapSize;
	uint32 codeHeapSize;
};

struct InternalStringIndex
{
	uint32 offset;
};

struct StringIndex
{
	uint32 offset;
};

struct TypeIndex
{
	uint32 offset;
};

struct FieldRef
{
	uint16 classRef;
	uint16 name;
};

struct MethodRef
{
	uint16 classRef;
	uint16 name;
	uint16 type;
	uint16 __padding;
};

struct ClassDef
{
	uint16 name;
	uint16 modifier;
	uint16 baseClassRef;
	uint16 fieldCount;
	uint16 fieldStartIndex;
	uint16 methodCount;
	uint16 methodStartIndex;
	uint16 propertyCount;
	uint16 propertyStartIndex;
	uint16 __padding;
};

struct FieldDef
{
	uint16 name;
	uint16 type;
	uint16 modifier;
	uint16 __padding;
};

struct ExceptionClauseDef
{
	uint32 type;
	uint32 tryStart, tryEnd;
	uint32 handlerStart, handlerEnd;
	uint16 catchClassRef;
	uint16 catchRegister;
};

#pragma pack(push)
#pragma pack(1)
/* VC does not pack the union correctly, use #pragma pack to cheat it */
struct MethodDef
{
	uint16 name;
	uint16 type;
	uint16 modifier;
	union
	{
		struct /* Aqua method */
		{
			uint16 registerCount;
			uint32 codeDataOffset;
			uint32 codeSlotCount;
			uint32 exceptionClauseCount;
		};
		struct /* Native method */
		{
			uint16 callingConvention;
			uint16 dllName;
			uint16 originalName;
			uint32 _padding;
			uint32 _padding2;
		};
	};
};
#pragma pack(pop)

struct PropertyDef
{
	uint16 name;
	uint16 type;
	uint16 modifier;
	uint16 flags;
	uint16 getter;
	uint16 setter;
};

struct TypeRef
{
	uint16 type;
};

struct DerivedTypeRef: public TypeRef
{
	uint16 baseType;
};

struct ClassTypeRef: public TypeRef
{
	uint16 className;
};

struct FunctionTypeRef: public TypeRef
{
	uint16 paramCount;
	uint16 returnCount;
	uint16 types[]; /* first param types, then return types */
};

struct InternalString
{
	uint32 length;
	char data[];

	static inline uint32 hash(InternalString *string)
	{
		return hashString(string->data, string->length);
	}

	static inline bool isEqual(InternalString *left, InternalString *right)
	{
		return left->length == right->length && !memcmp(left->data, right->data, left->length);
	}
};

struct VTable;
struct String
{
	VTable *vtable;
	int32 length; /* In characters */
	uint16 data[];

	static inline uint32 hash(String *string)
	{
		return hashString(string->data, string->length * 2);
	}

	static inline bool isEqual(String *left, String *right)
	{
		return left->length == right->length && !memcmp(left->data, right->data, left->length * 2);
	}
};

template<typename T, int N>
struct Array
{
	VTable *vtable;
	int32 size[N];
	T data[];
};

struct Type
{
	uint32 type;

	static inline uint32 hash(Type *type);
	static inline bool isEqual(Type *left, Type *right);
};

struct DerivedType: public Type
{
	Type *baseType;
};

struct Class;
struct ClassType: public Type
{
	Class *classObject;
};

struct FunctionType: public Type
{
	uint16 paramCount;
	uint16 returnCount;
	Type *types[]; /* first param types, then return types */
};

struct ArrayType: public Type
{
	Type *elementType;
	Class *arrayClass;
};

/* NOTICE: Make sure all Type types have no alignment paddings,
           or the padding fields may not be correctly initialized,
		   that will cause hash() and isEqual() failing */

__forceinline uint32 _getTypeStructSize(Type *type)
{
	if (type->type <= TYPE_PRI_MAX)
		return sizeof(Type);
	else if (type->type == TYPE_CLASS)
		return sizeof(ClassType);
	else if (type->type == TYPE_FUNCTION)
		return sizeof(FunctionType) + sizeof(Type *) * (((FunctionType *) type)->paramCount + ((FunctionType *) type)->returnCount);
	else if (type->type == TYPE_ARRAY)
		/* Don't compare ->arrayClass */
		return sizeof(ArrayType) - sizeof(Class *);
	else
		return sizeof(DerivedType);
}

inline uint32 Type::hash(Type *type)
{
	return hashString(type, _getTypeStructSize(type));
}

inline bool Type::isEqual(Type *left, Type *right)
{
	if (left->type != right->type)
		return false;
	return !memcmp(left, right, _getTypeStructSize(left));
}

struct Field
{
	InternalString *name;
	Type *type;
	Class *classObject;
	uint16 modifier;
	uint32 offset;
};

typedef void InternalMethod(reg *);

struct ExceptionClause
{
	uint32 type;
	uint32 tryStart, tryEnd;
	uint32 handlerStart, handlerEnd;
	Class *catchClass;
	uint16 catchRegister;
};

struct Method
{
	InternalString *name;
	Type *type;
	Class *classObject;
	uint16 modifier;
	uint16 vtableSlot;
	union
	{
		struct /* Aqua method */
		{
			uint16 registerCount;
			uint32 *code;
			uint32 codeSlotCount;
			uint32 exceptionClauseCount;
			ExceptionClause *exceptionClause;
		};
		InternalMethod *internalMethod;
		struct /* Native method */
		{
			uint16 callingConvention;
			void *nativeMethod;
		};
	};
};

struct BytecodeFile;
struct Class;

struct VTable
{
	Class *classObject;
	Method *methods[];
};

struct Class
{
	enum class LoadState: uint32 { Unloaded = 0, Loaded = 1 };
	LoadState loadState;
	BytecodeFile *bytecodeFile;
	uint16 classId; /* id in bytecode file */

	Class *baseClass;

	InternalString *name;
	uint16 modifier;
	uint16 fieldCount;
	Field *fields;
	uint16 methodCount;
	Method *methods;
	/* Including vtable */
	uint32 staticSize, instanceSize;
	char *staticData;
	uint16 vtableSlotCount;

	VTable *vtable;
};

struct BytecodeFile
{
	uint16 internalStringCount;
	InternalStringIndex *internalStringIndexTable;
	InternalString **internalStringTable;
	uint16 stringCount;
	StringIndex *stringIndexTable;
	String **stringTable;
	uint16 typeCount;
	TypeIndex *typeIndexTable;
	Type **typeTable;

	uint16 fieldRefCount;
	FieldRef *fieldRefTable;
	Field **fieldTable;

	uint16 methodRefCount;
	MethodRef *methodRefTable;
	Method **methodTable;

	uint16 classDefCount;
	ClassDef *classDefTable;
	FieldDef *fieldDefTable;
	MethodDef *methodDefTable;
	Class *classTable;

	char *internalStringHeap;
	char *stringHeap;
	char *typeHeap;
	char *codeHeap;
};

#define typedValue(value, type) (* (type *) &(value))
#define INT16(x) typedValue(x, int16)
#define UINT16(x) typedValue(x, uint16)
#define INT32(x) typedValue(x, int32)
#define UINT32(x) typedValue(x, uint32)
#define INT64(x) typedValue(x, int64)
#define UINT64(x) typedValue(x, uint64)
#define FLOAT32(x) typedValue(x, float32)
#define FLOAT64(x) typedValue(x, float64)
#define POINTER(x) typedValue(x, pointer)
#define INT(x) typedValue(x, nativeint)
#define UINT(x) typedValue(x, nativeuint)

#define VTABLE(obj) (*(VTable **) (obj))
