#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <Windows.h>

#include "CoreLibraryInternal.h"
#include "PlatformTypes.h"
#include "Hash.h"
#include "Object.h"
#include "Resolve.h"
#include "VMTypes.h"

static void invokeNativeMethod(Method *method, reg *reg)
{
	_asm
	{
		; eax: method
		; ecx: current parameter id
		; ebx: reg
		mov eax, method
		mov ecx, [eax]Method.type
		movzx ecx, [ecx]FunctionType.paramCount
		mov ebx, reg

		; Push arguments
		cmp ecx, 0
		jz L2
L1:
		push [ebx + ecx * 8 - 8]
		sub ecx, 1
		jnz L1

L2:
		mov eax, [eax]Method.nativeMethod
		call (eax)

		; Save result
		; If the function has no return value, it is still okay to save eax to reg[0]
		; since reg[0] will not be accessed in that case
		mov ebx, reg ; ebx may be modified by callee, load it again
		mov [ebx], eax
	}
}

/* Helpers */
#define OP_END() goto MAIN_DISPATCH
#define OP(opcode, work) case opcode: work; OP_END();
#define OPCODE (cd & 0xFF)
#define OP_A ((cd >> 8) & 0xFF)
#define OP_B ((cd >> 16) & 0xFF)
#define OP_C (cd >> 24)
#define OP_BC (cd >> 16)
#define REG(i) (base[i])
#define rA REG(OP_A)
#define rB REG(OP_B)
#define rC REG(OP_C)
#define _imm16 typedValue(code[pc], uint16)
#define _imm32 typedValue(code[pc], uint32)
#define _imm64 typedValue(code[pc], uint64)
#define imm32 code[pc++] /* WARNING: Notice this ++!!!!! */
#define imm64 code[(pc += 2) - 2] /* WARNING: Notice this += 2!!!!! */
#define BC (cd >> 16)

/* Arithmetic */
#define UNARY_AB(op) INT32(rA) = op INT32(rB)
#define UNARYL_AB(op) INT64(rA) = op INT64(rB)
#define UNARYF_AB(op) FLOAT32(rA) = op FLOAT32(rB)
#define UNARYFL_AB(op) FLOAT64(rA) = op FLOAT64(rB)

#define BINOPI_ABC(op) INT32(rA) = INT32(rB) op INT32(rC)
#define BINOPI_ABI(op) INT32(rA) = INT32(rB) op INT32(imm32)
#define BINOPI_AIB(op) INT32(rA) = INT32(imm32) op UINT32(rB)
#define BINOPU_ABC(op) UINT32(rA) = UINT32(rB) op UINT32(rC)
#define BINOPU_ABI(op) UINT32(rA) = UINT32(rB) op UINT32(imm32)
#define BINOPU_AIB(op) UINT32(rA) = UINT32(imm32) op UINT32(rB)

#define BINOPL_ABC(op) INT64(rA) = INT64(rB) op INT64(rC)
#define BINOPL_ABI(op) INT64(rA) = INT64(rB) op INT64(imm64)
#define BINOPL_AIB(op) INT64(rA) = INT64(imm64) op INT64(rB)
#define BINOPUL_ABC(op) UINT64(rA) = UINT64(rB) op UINT64(rC)
#define BINOPUL_ABI(op) UINT64(rA) = UINT64(rB) op UINT64(imm64)
#define BINOPUL_AIB(op) UINT64(rA) = UINT64(imm64) op UINT64(rB)

#define BINOPN_ABC(op) INT(rA) = INT(rB) op INT(rC)
#define BINOPNI_ABC(op) INT(rA) = INT(rB) op INT32(rC)

#define BINOPF_ABC(op) FLOAT32(rA) = FLOAT32(rB) op FLOAT32(rC)
#define BINOPFL_ABC(op) FLOAT64(rA) = FLOAT64(rB) op FLOAT64(rC)

/* Branch */
#define UNARYN_JMP(cond) pc = (POINTER(rA) cond)? pc + INT32(_imm32): pc + 1

#define UNARY_JMP(cond) pc = (INT32(rA) cond)? pc + INT32(_imm32): pc + 1
#define UNARYL_JMP(cond) pc = (INT64(rA) cond)? pc + INT32(_imm32): pc + 1
#define UNARYF_JMP(cond) pc = (FLOAT32(rA) cond)? pc + INT32(_imm32): pc + 1
#define UNARYFL_JMP(cond) pc = (FLOAT64(rA) cond)? pc + INT32(_imm32): pc + 1

#define BINOP_JMP(op) pc = (INT32(rA) op INT32(rB))? pc + INT32(_imm32): pc + 1
#define BINOPU_JMP(op) pc = (UINT32(rA) op UINT32(rB))? pc + INT32(_imm32): pc + 1
#define BINOPL_JMP(op) pc = (INT64(rA) op INT64(rB))? pc + INT32(_imm32): pc + 1
#define BINOPUL_JMP(op) pc = (UINT64(rA) op UINT64(rB))? pc + INT32(_imm32): pc + 1
#define BINOPF_JMP(op) pc = (FLOAT32(rA) op FLOAT32(rB))? pc + INT32(_imm32): pc + 1
#define BINOPFL_JMP(op) pc = (FLOAT64(rA) op FLOAT64(rB))? pc + INT32(_imm32): pc + 1

#define JINST() pc = (instanceOf(POINTER(rA), resolveClass(bytecodeFile, OP_BC)))? pc + INT32(_imm32): pc + 1
#define JNINST() pc = (!instanceOf(POINTER(rA), resolveClass(bytecodeFile, OP_BC)))? pc + INT32(_imm32): pc + 1

/* Load/store */
#define LDF(type, regtype) \
	{ \
		uint16 fieldRef = UINT16(_imm32); \
		Field *field = resolveField(bytecodeFile, fieldRef); \
		typedValue(rA, regtype) = typedValue(POINTER(rB)[field->offset], type); \
		pc += 1; \
	}

#define STF(type, regtype) \
	{ \
		uint16 fieldRef = UINT16(_imm32); \
		Field *field = resolveField(bytecodeFile, fieldRef); \
		typedValue(POINTER(rB)[field->offset], type) = typedValue(rA, regtype); \
		pc += 1; \
	}

#define LAF() \
	{ \
		uint16 fieldRef = UINT16(_imm32); \
		Field *field = resolveField(bytecodeFile, fieldRef); \
		POINTER(rA) = POINTER(rB) + field->offset; \
		pc += 1; \
	}

#define LDS(type, regtype) \
	{ \
		uint16 fieldRef = OP_BC; \
		Field *field = resolveField(bytecodeFile, fieldRef); \
		typedValue(rA, regtype) = typedValue(field->classObject->staticData[field->offset], type); \
	}

#define STS(type, regtype) \
	{ \
		uint16 fieldRef = OP_BC; \
		Field *field = resolveField(bytecodeFile, fieldRef); \
		typedValue(field->classObject->staticData[field->offset], type) = typedValue(rA, regtype); \
	}

#define LAS() \
	{ \
		uint16 fieldRef = OP_BC; \
		Field *field = resolveField(bytecodeFile, fieldRef); \
		POINTER(rA) = field->classObject->staticData + field->offset; \
	}

#define LDI(type, regtype) \
	typedValue(rA, regtype) = *typedValue(rB, type *);

#define STI(type, regtype) \
	*typedValue(rB, type *) = typedValue(rA, regtype);

#define LAR() \
	POINTER(rA) = (pointer) &rB;

#define LDE(type, regtype) \
	typedValue(rA, regtype) = *((type *) (POINTER(rB) + sizeof(VTable *) + sizeof(int32) + sizeof(type) * INT32(rC)));

#define STE(type, regtype) \
	*((type *) (POINTER(rB) + sizeof(VTable *) + sizeof(int32) + sizeof(type) * INT32(rC))) = typedValue(rA, regtype);

#define LAE() \
	{ \
		pointer object = POINTER(rB); \
		Type *elementType = *(reinterpret_cast<Type **>((VTable **) rB)); \
		POINTER(rA) = (pointer) (object + sizeof(VTable *) + sizeof(int32) + INT32(rC) * getTypeSize(elementType)); \
	}

/* Conversion */
#define _C(dstt, dstst, dst, srct, src) \
	case (dstt << 4) | srct: \
		typedValue(rA, dstst) = (dstst) (dst) typedValue(rB, src); \
		OP_END();

#define _CROW(dstt, dstst, dst) \
	_C(dstt, dstst, dst, TYPE_INT8, int32) \
	_C(dstt, dstst, dst, TYPE_UINT8, uint32) \
	_C(dstt, dstst, dst, TYPE_INT16, int32) \
	_C(dstt, dstst, dst, TYPE_UINT16, uint32) \
	_C(dstt, dstst, dst, TYPE_INT32, int32) \
	_C(dstt, dstst, dst, TYPE_UINT32, uint32) \
	_C(dstt, dstst, dst, TYPE_INT64, int64) \
	_C(dstt, dstst, dst, TYPE_UINT64, uint64) \
	_C(dstt, dstst, dst, TYPE_FLOAT32, float32) \
	_C(dstt, dstst, dst, TYPE_FLOAT64, float64) \
	_C(dstt, dstst, dst, TYPE_INT, nativeint) \
	_C(dstt, dstst, dst, TYPE_UINT, nativeuint)

#define CONV() \
	switch (OP_C) \
	{ \
		_CROW(TYPE_INT8, int32, int8) \
		_CROW(TYPE_UINT8, uint32, uint8) \
		_CROW(TYPE_INT16, int32, int16) \
		_CROW(TYPE_UINT16, uint32, uint16) \
		_CROW(TYPE_INT32, int32, int32) \
		_CROW(TYPE_UINT32, uint32, uint32) \
		_CROW(TYPE_INT64, int64, int64) \
		_CROW(TYPE_UINT64, uint64, uint64) \
		_CROW(TYPE_FLOAT32, float32, float32) \
		_CROW(TYPE_FLOAT64, float64, float64) \
		_CROW(TYPE_INT, nativeint, nativeint) \
		_CROW(TYPE_UINT, nativeuint, nativeuint) \
	}

/* Frame helpers */
#define ISLASTFRAME() (currentFrame == startFrame)
#define ENTERFRAME() \
	{ \
		bytecodeFile = currentFrame->method->classObject->bytecodeFile; \
		base = currentFrame->registerBase; \
		code = currentFrame->method->code; \
		pc = currentFrame->nextpc; \
	}

#define RET() \
	{ \
		if (ISLASTFRAME()) \
			return; \
		else \
		{ \
			currentFrame--; \
			ENTERFRAME(); \
		} \
	}

#define CALL(method, basereg) \
	{ \
		if (method->modifier & MODIFIER_INTERNAL) \
			(*method->internalMethod)(&REG(basereg)); \
		else if (method->modifier & MODIFIER_NATIVE) \
			invokeNativeMethod(method, &REG(basereg)); \
		else \
		{ \
			currentFrame->nextpc = pc; \
			currentFrame++; \
			currentFrame->registerBase = base + basereg; \
			currentFrame->method = method; \
			bytecodeFile = method->classObject->bytecodeFile; \
			base += basereg; \
			code = method->code; \
			pc = 0; \
		} \
	}

#define THROW(x, throwpc) \
	{ \
		currentException = (x); \
		currentFrame->nextExceptionClause = 0; \
		pc = (throwpc); \
		goto HANDLE_EXCEPTION; \
	}

#define DEFAULT_REGISTER_STACK_SIZE  1048576
#define DEFAULT_FRAME_STACK_SIZE     1048576
struct Frame
{
	reg *registerBase;
	Method *method;
	uint32 nextpc;
	uint32 nextExceptionClause; /* Next exception clause to handle */
};

static inline bool instanceOf(pointer object, Class *classObject)
{
	Class *c = VTABLE(object)->classObject;
	while (c != classObject && c != nullptr)
		c = c->baseClass;
	return c == classObject;
}

static pointer currentException;
static uint32 currentLeaveTarget;

static void runMethod(Frame *startFrame, Method *startMethod, reg *base)
{
	BytecodeFile *bytecodeFile = startMethod->classObject->bytecodeFile;
	Frame *currentFrame = startFrame;
	currentFrame->method = startMethod;
	currentFrame->registerBase = base;

	uint32 pc = 0;
	uint32 *code = startMethod->code;

	{
MAIN_DISPATCH:
		uint32 cd = code[pc++];
		switch (OPCODE)
		{
		OP(0x00, BINOPI_ABC(+)) // ADDI $a, $b, $c
		OP(0x01, BINOPI_ABI(+)) // ADDI $a, $b, imm32
		OP(0x02, BINOPI_ABC(-)) // SUBI $a, $b, $c
		OP(0x03, BINOPI_ABI(-)) // SUBI $a, $b, imm32
		OP(0x04, BINOPI_ABC(*)) // MULI $a, $b, $c
		OP(0x05, BINOPI_ABI(*)) // MULI $a, $b, imm32
		OP(0x06, BINOPU_ABC(*)) // MULU $a, $b, $c
		OP(0x07, BINOPU_ABI(*)) // MULU $a, $b, imm32
		OP(0x08, BINOPI_ABC(/)) // DIVI $a, $b, $c
		OP(0x09, BINOPI_ABI(/)) // DIVI $a, $b, imm32
		OP(0x0A, BINOPU_ABC(/)) // DIVU $a, $b, $c
		OP(0x0B, BINOPU_ABI(/)) // DIVU $a, $b, imm32
		OP(0x0C, BINOPI_ABC(%)) // REMI $a, $b, $c
		OP(0x0D, BINOPI_ABI(%)) // REMI $a, $b, imm32
		OP(0x0E, BINOPU_ABC(%)) // REMU $a, $b, $c
		OP(0x0F, BINOPU_ABI(%)) // REMU $a, $b, imm32
		
		OP(0x10, BINOPI_ABC(&)) // ANDI $a, $b, $c
		OP(0x11, BINOPI_ABI(&)) // ANDI $a, $b, imm32
		OP(0x12, BINOPI_ABC(|)) // ORI $a, $b, $c
		OP(0x13, BINOPI_ABI(|)) // ORI $a, $b, imm32
		OP(0x14, BINOPI_ABC(^)) // XORI $a, $b, $c
		OP(0x15, BINOPI_ABI(^)) // XORI $a, $b, imm32
		OP(0x16, UNARY_AB(~)) // NOTI $a, $b
		OP(0x17, UNARYL_AB(-)) // NEGI $a, $b
		OP(0x18, BINOPI_ABC(<<)) // SLI $a, $b, $c
		OP(0x19, BINOPI_ABI(<<)) // SLI $a, $b, imm32
		OP(0x1A, BINOPI_ABC(>>)) // SRI $a, $b, $c
		OP(0x1B, BINOPI_ABI(>>)) // SRI $a, $b, imm32
		OP(0x1C, BINOPU_ABC(>>)) // SRU $a, $b, $c
		OP(0x1D, BINOPU_ABI(>>)) // SRU $a, $b, imm32

		OP(0x30, BINOPN_ABC(+)) // ADDN $a, $b, $c
		OP(0x31, BINOPNI_ABC(+)) // ADDNI $a, $b, $c
		OP(0x32, BINOPN_ABC(-)) // SUBN $a, $b, $c
		OP(0x33, BINOPNI_ABC(-)) // SUBNI $a, $b, $c

		OP(0x60, pc += INT32(_imm32)) // JMP addr
		OP(0x61, UNARYN_JMP(== 0)) // JN addr
		OP(0x62, UNARYN_JMP(!= 0)) // JNN addr

		OP(0x64, JINST()) // JINST $a, type, addr
		OP(0x65, JNINST()) // JNINST $a, type, addr

		OP(0x70, UNARY_JMP(== 0)) // JZI $a, addr
		OP(0x71, UNARY_JMP(!= 0)) // JNZI $a, addr
		OP(0x72, UNARY_JMP(> 0)) // JGTZI $a, addr
		OP(0x73, UNARY_JMP(>= 0)) // JGEZI $a, addr
		OP(0x74, UNARY_JMP(< 0)) // JLTZI $a, addr
		OP(0x75, UNARY_JMP(<= 0)) // JLEZI $a, addr
		OP(0x76, BINOP_JMP(==)) // JEQI $a, $b, addr
		OP(0x77, BINOP_JMP(!=)) // JNEQI $a, $b, addr
		OP(0x78, BINOP_JMP(>)) // JGTI $a, $b, addr
		OP(0x79, BINOPU_JMP(>)) // JGTU $a, $b, addr
		OP(0x7A, BINOP_JMP(>=)) // JGEI $a, $b, addr
		OP(0x7B, BINOPU_JMP(>=)) // JGEU $a, $b, addr
		OP(0x7C, BINOP_JMP(<)) // JLTI $a, $b, addr
		OP(0x7D, BINOPU_JMP(<)) // JLTU $a, $b, addr
		OP(0x7E, BINOP_JMP(<=)) // JLEI $a, $b, addr
		OP(0x7F, BINOPU_JMP(<=)) // JLEU $a, $b, addr

		OP(0x80, LDF(int8, int32)) // LDFB $a, $b, @field
		OP(0x81, LDF(uint8, uint32)) // LDFUB $a, $b, @field
		OP(0x82, LDF(int16, int32)) // LDFW $a, $b, @field
		OP(0x83, LDF(uint16, uint32)) // LDFUW $a, $b, @field
		OP(0x84, LDF(int32, int32)) // LDFI $a, $b, @field
		OP(0x85, LDF(int64, int64)) // LDFL $a, $b, @field
		OP(0x87, LDF(pointer, pointer)) // LDFA $a, $b, @field
		
		OP(0x88, STF(int8, int32)) // LDFB $b, @field, $a
		OP(0x89, STF(uint8, uint32)) // LDFUB $b, @field, $a
		OP(0x8A, STF(int16, int32)) // LDFW $b, @field, $a
		OP(0x8B, STF(uint16, uint32)) // LDFUW $b, @field, $a
		OP(0x8C, STF(int32, int32)) // LDFI $b, @field, $a
		OP(0x8D, STF(int64, int64)) // LDFL $b, @field, $a
		OP(0x8F, STF(pointer, pointer)) // LDFA $b, @field, $a

		OP(0x90, LDS(int8, int32)) // LDSB $a, @field
		OP(0x91, LDS(uint8, uint32)) // LDSUB $a, @field
		OP(0x92, LDS(int16, int32)) // LDSW $a, @field
		OP(0x93, LDS(uint16, uint32)) // LDSUW $a, @field
		OP(0x94, LDS(int32, int32)) // LDSI $a, @field
		OP(0x95, LDS(int64 ,int64)) // LDSL $a, @field
		OP(0x97, LDS(pointer, pointer)) // LDSA $a, @field
		
		OP(0x98, STS(int8, int32)) // LDSB @field, $a
		OP(0x99, STS(uint8, uint32)) // LDSUB @field, $a
		OP(0x9A, STS(int16, int32)) // LDSW @field, $a
		OP(0x9B, STS(uint16, uint32)) // LDSUW @field, $a
		OP(0x9C, STS(int32, int32)) // LDSI @field, $a
		OP(0x9D, STS(int64, int64)) // LDSL @field, $a
		OP(0x9F, STS(pointer, pointer)) // LDSA @field, $a

		OP(0xA0, LDI(int8, int32)) // LDIB $a, ($b)
		OP(0xA1, LDI(uint8, uint32)) // LDIUB $a, ($b)
		OP(0xA2, LDI(int16, int32)) // LDIW $a, ($b)
		OP(0xA3, LDI(uint16, uint32)) // LDIUW $a, ($b)
		OP(0xA4, LDI(int32, int32)) // LDII $a, ($b)
		OP(0xA5, LDI(int64, int64)) // LDIL $a, ($b)
		OP(0xA7, LDI(pointer, pointer)) // LDIA $a, ($b)
		
		OP(0xA8, STI(int8, int32)) // LDIB ($b), $a
		OP(0xA9, STI(uint8, uint32)) // LDIUB ($b), $a
		OP(0xAA, STI(int16, int32)) // LDIW ($b), $a
		OP(0xAB, STI(uint16, uint32)) // LDIUW ($b), $a
		OP(0xAC, STI(int32, int32)) // LDII ($b), $a
		OP(0xAD, STI(int64, int64)) // LDIL ($b), $a
		OP(0xAF, STI(pointer, pointer)) // LDIA ($b), $a

		OP(0xB0, LDE(int8, int32)) // LDEB $a, $b($c)
		OP(0xB1, LDE(uint8, uint32)) // LDEUB $a, $b($c)
		OP(0xB2, LDE(int16, int32)) // LDEW $a, $b($c)
		OP(0xB3, LDE(uint16, uint32)) // LDEUW $a, $b($c)
		OP(0xB4, LDE(int32, int32)) // LDEI $a, $b($c)
		OP(0xB5, LDE(int64, int64)) // LDEL $a, $b($c)
		OP(0xB7, LDE(pointer, pointer)) // LDEA $a, $b($c)

		OP(0xB8, STE(int8, int32)) // LDEB $b($c), $a
		OP(0xB9, STE(uint8, uint32)) // LDEUB $b($c), $a
		OP(0xBA, STE(int16, int32)) // LDEW $b($c), $a
		OP(0xBB, STE(uint16, uint32)) // LDEUW $b($c), $a
		OP(0xBC, STE(int32, int32)) // LDEI $b($c), $a
		OP(0xBD, STE(int64, int64)) // LDEL $b($c), $a
		OP(0xBF, STE(pointer, pointer)) // LDEA $b($c), $a

		OP(0xC0, INT32(rA) = INT32(rB)) // LDI $a, $b
		OP(0xC1, INT64(rA) = INT64(rB)) // LDL $a, $b
		OP(0xC3, POINTER(rA) = POINTER(rB)) // LDA $a, $b
		OP(0xC4, INT32(rA) = INT32(imm32)) // LDI $a, imm32
		OP(0xC5, INT64(rA) = INT64(imm64)) // LDL $a, imm64
		OP(0xC6, FLOAT32(rA) = FLOAT32(imm32)) // LDF $a, imm32
		OP(0xC7, FLOAT64(rA) = FLOAT64(imm64)) // LDFL $a, imm64

		OP(0xC8, POINTER(rA) = (pointer) resolveString(bytecodeFile, OP_BC)) // LDSTR $a, "str"
		OP(0xC9, INT32(rA) = *((int32 *) (POINTER(rB) + sizeof(VTable *)))) // LDLEN $a, $b
		
		OP(0xCB, POINTER(rA) = 0) // LDNULL $a
		OP(0xCC, LAF()) // LAF $a, $b, @field
		OP(0xCD, LAS()) // LAF $a, @field
		OP(0xCE, LAR()) // LAR $a, $b
		OP(0xCF, LAE()) // LAE $a, $b($c)

		case 0xD0: // NEW $a, @ctormethod
		{
			Method *method = resolveMethod(bytecodeFile, OP_BC);
			Class *classObject = method->classObject;
			pointer object = (pointer) malloc(classObject->instanceSize);
			VTABLE(object) = classObject->vtable;
			memset(object + sizeof(VTable *), 0, classObject->instanceSize - sizeof(VTable *));

			POINTER(rA) = object;
			POINTER(REG(OP_A + 1)) = object;

			CALL(method, OP_A + 1);
			OP_END();
		}

		case 0xD1: // NEWARR $a, type($b)
		{
			ArrayType *type = (ArrayType *) resolveType(bytecodeFile, _imm32);
			/* FIXME: VTable handling, now we just store the underling type on 'vtable' position of object */
			uint32 size = getTypeSize(type->elementType) * INT32(rB);
			pointer object = (pointer) malloc(sizeof(VTable *) + sizeof(int32) + size);
			*(reinterpret_cast<Type **>((VTable **) object)) = type->elementType;
			*((int32 *) (object + sizeof(VTable *))) = INT32(rB);
			memset(object + sizeof(VTable *) + sizeof(int32), 0, size);

			POINTER(rA) = object;
			pc = pc + 1;
			OP_END();
		}

		case 0xD8: // RET
			RET();
			OP_END();

		case 0xD9: // RET1
			REG(0) = rA;
			RET();
			OP_END();
		
		case 0xDA: // RET $a, $b
			for (uint32 i = OP_A; i <= OP_B; i++)
				REG(i - OP_A) = REG(i);
			RET();
			OP_END();
			
		case 0xDB: // CALL $a, @method
		{
			Method *method = resolveMethod(bytecodeFile, OP_BC);
			CALL(method, OP_A);
			OP_END();
		}

		case 0xDC: // CALLV $a, @method
		{
			uint16 vtableSlot = resolveMethod(bytecodeFile, OP_BC)->vtableSlot;
			Method *method = VTABLE(POINTER(rA))->methods[vtableSlot];
			if (method == nullptr)
			{
				printf("Calling abstract function.\n");
				return;
			}
			CALL(method, OP_A);
			OP_END();
		}

		case 0xE0: // THROW $a
			THROW(POINTER(rA), pc - 1);

		case 0xE2: // LEAVE addr
			currentFrame->nextExceptionClause = 0;
			currentLeaveTarget = pc + _imm32;
			pc = pc - 1;
			goto HANDLE_LEAVE;

		case 0xE3: // ENDFINALLY
			pc = pc - 1;
			if (currentException == nullptr)
				goto HANDLE_LEAVE;
			else
				goto HANDLE_EXCEPTION;

		case 0xE6:
			CONV();
			assert(("Should not come here.", 0));

		case 0xE7: // CAST $a, $b, class
		{
			Class *classObject = resolveClass(bytecodeFile, _imm16);
			if (!instanceOf(POINTER(rB), classObject))
			{
				printf("Not instance of %s!", classObject->name->data);
				return;
			}
			POINTER(rA) = POINTER(rB);
			pc = pc + 1;
			OP_END();
		}

		case 0xFF:
			switch (OP_A)
			{
			case 0x00: // PRINTI rC
				printf("%d\n", INT32(rC));
				OP_END();

			case 0x01: // PRINTU rC
				printf("%u\n", UINT32(rC));
				OP_END();

			case 0x02: // PRINTL rC
				printf("%lld\n", INT64(rC));
				OP_END();

			case 0x03: // PRINTUL rC
				printf("%llu\n", UINT64(rC));
				OP_END();

			case 0x04: // PRINTS rC
			{
				String *string = (String *) POINTER(rC);
				for (int32 i = 0; i < string->length; i++)
					wprintf(L"%c", string->data[i]);
				wprintf(L"\n");
				OP_END();
			}

			case 0x05: // PRINTC rC
				wprintf(L"%c", UINT16(rC));
				OP_END();

			case 0x08: // READI rC
				scanf("%d\n", &INT32(rC));
				OP_END();

			case 0x09: // READU rC
				scanf("%u\n", &UINT32(rC));
				OP_END();

			case 0x0A: // READL rC
				scanf("%lld\n", &INT64(rC));
				OP_END();

			case 0x0B: // READUL rC
				scanf("%ulld\n", &UINT64(rC));
				OP_END();

			case 0x0C: // READS rC
			{
				wchar_t buffer[256];
				uint32 len;
				for (len = 0;; len++)
				{
					wscanf_s(L"%c", &buffer[len]);
					if (buffer[len] == '\n')
						break;
				}

				pointer object = (pointer) malloc(sizeof(String) + len * 2 + 2);
				*((Class **) object) = stringClass;
				String *string = (String *) (object + sizeof(VTable *));
				string->length = len;
				memcpy(string->data, buffer, len * 2);
				string->data[len] = 0;

				POINTER(rC) = (pointer) string;
				OP_END();
			};
			}
			OP_END();
		}
		assert(("Should not come here.", 0));

HANDLE_EXCEPTION: /* Handles throw */
		bool isFrameInvalidated = false; /* Whether we need to reload frame variables */
		for (;;)
		{
			if (currentFrame->nextExceptionClause >= currentFrame->method->exceptionClauseCount)
			{
				/* Exception not handled in this frame, pop frame */
				isFrameInvalidated = true;
				if (ISLASTFRAME())
				{
					/* Unhandled exception */
					printf("Unhandled exception \"%s\" caught.", VTABLE(currentException)->classObject->name->data);
					return;
				}
				currentFrame--;
				currentFrame->nextExceptionClause = 0;
				pc = currentFrame->nextpc - 1; /* Get actual current pc, we ensure all call instructions are exactly 1 slot size */
			}
			else
			{
				ExceptionClause *clause = &currentFrame->method->exceptionClause[currentFrame->nextExceptionClause++];
				if (pc >= clause->tryStart && pc < clause->tryEnd)
				{
					if (clause->type == EXCEPTION_CLAUSE_CATCH)
					{
						if (!instanceOf(currentException, clause->catchClass))
							continue;
						/* Got ya */
						if (isFrameInvalidated)
							ENTERFRAME();
						POINTER(REG(clause->catchRegister)) = currentException;
						currentException = nullptr;
					}
					else /* finally */
						if (isFrameInvalidated)
							ENTERFRAME();
					pc = clause->handlerStart;
					OP_END();
				}
			}
		}
		assert(("Should not come here.", 0));

HANDLE_LEAVE: /* Handles leave */
		for (;;)
		{
			if (currentFrame->nextExceptionClause >= currentFrame->method->exceptionClauseCount)
			{
				/* We are done */
				pc = currentLeaveTarget;
				OP_END();
			}
			else
			{
				ExceptionClause *clause = &currentFrame->method->exceptionClause[currentFrame->nextExceptionClause++];
				if (clause->type == EXCEPTION_CLAUSE_FINALLY && (pc >= clause->tryStart && pc < clause->tryEnd)
					&& !(currentLeaveTarget >= clause->tryStart && currentLeaveTarget < clause->tryEnd))
				{
					/* Enter this clause */
					pc = clause->handlerStart;
					OP_END();
				}
			}
		}
		assert(("Should not come here.", 0));
	}
}

int main()
{
	wchar_t *commandLine = GetCommandLineW();
	int argc;
	wchar_t **argv = CommandLineToArgvW(commandLine, &argc);
	if (argc < 3)
		return EXIT_FAILURE;
	int p = argc;
	for (int i = 2; i < argc; i++)
		if (argv[i][0] == '-' && argv[i][1] == 0)
		{
			p = i + 1;
			break;
		}
		else
			resolveBytecodeFile(argv[i]);

	initializeResolveCache();
	initializeCoreLibraryInternal();

	/* Find entry class */
	int entryClassNameLengthW = lstrlenW(argv[1]);
	char *entryClassName = (char *) malloc(utf16ToUtf8((const uint16 *) argv[1], entryClassNameLengthW, nullptr) + 1);
	entryClassName[utf16ToUtf8((const uint16 *) argv[1], entryClassNameLengthW, entryClassName)] = 0;
	Class *entryClass = resolveClass(resolveInternalString(entryClassName));
	free(entryClassName);

	/* Find 'Main()' entry point */
	InternalString *entryMethodName = resolveInternalString("Main");
	Type *entryType = resolveFunctionType(0, 0);
	Method *startMethod = methodHash.find(entryClass, entryMethodName, entryType);
	if (startMethod == nullptr)
	{
		entryType = resolveFunctionType(1, 0, resolveArrayType(resolveClassType(stringClass)));
		startMethod = methodHash.find(entryClass, entryMethodName, entryType);
	}
	assert(("Method 'main' not found.", startMethod));

	BytecodeFile *bytecodeFile = entryClass->bytecodeFile;

	/* Create args parameter */
	int argcount = argc - p;
	Array<String *, 1> *args = CreateArray<String *>(argcount);
	for (int i = p; i < argc; i++)
		args->data[i - p] = CreateString(lstrlenW(argv[i]), (const uint16 *) argv[i]);
	
	/* Initialize register stack and frame stack */
	reg *registerStack = (uint64 *) malloc(sizeof(uint64) * DEFAULT_REGISTER_STACK_SIZE);
	Frame *frameStack = (Frame *) malloc(sizeof(Frame) * DEFAULT_FRAME_STACK_SIZE);
	POINTER(registerStack[0]) = (pointer) args;
	runMethod(frameStack, startMethod, registerStack);

	return EXIT_SUCCESS;
}
