#include <functional>
#include <unordered_map>
#include <vector>
#include <string>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <cctype>
#include <iostream>
#include <malloc.h>
#define NOMINMAX
#include <Windows.h>

inline void _PUT(std::string &code, unsigned int v)
{
	code += (char) v;
}

inline void _PUT2(std::string &code, unsigned int v2)
{
	_PUT(code, v2 & 0xFF);
	_PUT(code, v2 >> 8);
}

inline void _PUT4(std::string &code, unsigned int v4)
{
	_PUT2(code, v4 & 0xFFFF);
	_PUT2(code, v4 >> 16);
}

inline void _PUTUTF8(std::string &code, const std::string &str)
{
	_PUT4(code, str.length());
	for (int i = 0; i < str.length(); i++)
		_PUT(code, str.at(i));
}

inline void _PUTS(std::string &code, const std::string &str)
{
	for (int i = 0; i < str.length(); i++)
		_PUT(code, str.at(i));
}

inline void _PUTWS(std::string &code, const std::wstring &str)
{
	for (int i = 0; i < str.length(); i++)
		_PUT2(code, str.at(i));
}

inline void _PUTAT(std::string &code, unsigned int v, unsigned int pos)
{
	code[pos] = v;
}

inline void _PUT2AT(std::string &code, unsigned int v2, unsigned int pos)
{
	_PUTAT(code, v2 & 0xFF, pos);
	_PUTAT(code, v2 >> 8, pos + 1);
}

inline void _PUT4AT(std::string &code, unsigned int v4, unsigned int pos)
{
	_PUT2AT(code, v4 & 0xFFFF, pos);
	_PUT2AT(code, v4 >> 16, pos + 2);
}

inline void _PUTALIGN(std::string &code, unsigned int align)
{
	unsigned int padding_bytes = (align - code.size() % align) % align;
	while (padding_bytes--)
		_PUT(code, 0);
}

#define PUT(v) _PUT(code, v)
#define PUT2(v2) _PUT2(code, v2)
#define PUT4(v4) _PUT4(code, v4)
#define PUTS(s) _PUTS(code, s)
#define PUTUTF8(s) _PUTUTF8(code, s)
#define PUTAT(v, pos) _PUTAT(code, v, pos)
#define PUT2AT(v, pos) _PUT2AT(code, v, pos)
#define PUT4AT(v, pos) _PUT4AT(code, v, pos)

void FPUT(unsigned int v)
{
	putchar(v);
}

void FPUT2(unsigned int v2)
{
	FPUT(v2 & 0xFF);
	FPUT(v2 >> 8);
}

void FPUT4(unsigned int v4)
{
	FPUT2(v4 & 0xFFFF);
	FPUT2(v4 >> 16);
}

void FPUTS(const std::string &str)
{
	for (int i = 0; i < str.length(); i++)
		FPUT(str[i]);
}

void FPUTT(const std::vector<std::string> &table)
{
	for (int i = 0; i < table.size(); i++)
		FPUTS(table[i]);
}

void FPUTIT(const std::vector<int> &index_table)
{
	for (int i = 0; i < index_table.size(); i++)
		FPUT4(index_table[i]);
}

std::vector<int> internal_string_index_table;
std::vector<int> string_index_table;
std::vector<int> type_index_table;

std::vector<std::string> field_ref_table;
std::vector<std::string> method_ref_table;
std::vector<std::string> class_def_table;
std::vector<std::string> field_def_table;
std::vector<std::string> method_def_table;

std::string internal_string_heap;
std::string string_heap;
std::string type_heap;
std::string code_heap;

std::unordered_map<std::string, int> internal_string_hash;
std::unordered_map<std::wstring, int> string_hash;
std::unordered_map<std::string, int> type_hash;
std::unordered_map<std::string, int> field_hash;
std::unordered_map<std::string, int> method_hash;

/* Heap data objects */
int new_internal_string(const std::string &string)
{
	if (internal_string_hash.count(string))
		return internal_string_hash[string];
	else
	{
		_PUTALIGN(internal_string_heap, 4);
		int index = internal_string_index_table.size();
		internal_string_index_table.push_back(internal_string_heap.size());
		_PUT4(internal_string_heap, string.size());
		_PUTS(internal_string_heap, string);
		internal_string_hash.insert(std::make_pair(string, index));
		return index;
	}
}

int new_string(const std::wstring &string)
{
	if (string_hash.count(string))
		return string_hash[string];
	else
	{
		_PUTALIGN(string_heap, 4);
		int index = string_index_table.size();
		string_index_table.push_back(string_heap.size());
		_PUT4(string_heap, string.size());
		_PUTWS(string_heap, string);
		string_hash.insert(std::make_pair(string, index));
		return index;
	}
}

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
#define TYPE_CLASS		16
#define TYPE_FUNCTION	17
#define TYPE_ARRAY		18
int new_type_ref_with_signature(const std::string &type)
{
	if (type_hash.count(type))
		return type_hash[type];
	else
	{
		_PUTALIGN(type_heap, 4);
		int index = type_index_table.size();
		type_index_table.push_back(type_heap.size());
		_PUTS(type_heap, type);
		type_hash.insert(std::make_pair(type, index));
		return index;
	}
}

int new_type_ref(int type)
{
	std::string signature;
	_PUT2(signature, type);
	return new_type_ref_with_signature(signature);
}

int new_type_ref(int type, int basetype)
{
	std::string signature;
	_PUT2(signature, type);
	_PUT2(signature, basetype);
	return new_type_ref_with_signature(signature);
}

int new_class_ref(const std::string &classname)
{
	return new_type_ref(TYPE_CLASS, new_internal_string(classname));
}

int new_array_ref(int element_type)
{
	return new_type_ref(TYPE_ARRAY, element_type);
}

int new_proto_ref(int type, const std::vector<int> &param_types, const std::vector<int> &return_types)
{
	std::string signature;
	_PUT2(signature, type);
	_PUT2(signature, param_types.size());
	_PUT2(signature, return_types.size());
	for (int i = 0; i < param_types.size(); i++)
		_PUT2(signature, param_types[i]);
	for (int i = 0; i < return_types.size(); i++)
		_PUT2(signature, return_types[i]);
	return new_type_ref_with_signature(signature);
}

/* Table data objects */
int new_field_ref(int class_ref, const std::string &field_name)
{
	std::string data;
	_PUT2(data, class_ref);
	_PUT2(data, new_internal_string(field_name));
	if (field_hash.count(data))
		return field_hash[data];
	else
	{
		int index = field_ref_table.size();
		field_ref_table.push_back(data);
		field_hash.insert(std::make_pair(data, index));
		return index;
	}
}

int new_method_ref(int class_ref, const std::string &method_name, int method_type)
{
	std::string data;
	_PUT2(data, class_ref);
	_PUT2(data, new_internal_string(method_name));
	_PUT2(data, method_type);
	_PUT2(data, 0); /* padding */
	if (method_hash.count(data))
		return method_hash[data];
	else
	{
		int index = method_ref_table.size();
		method_ref_table.push_back(data);
		method_hash.insert(std::make_pair(data, index));
		return index;
	}
}

enum TokenType
{
	tkEOF,
	tkNum,
	tkString,
	tkRegister,
	tkIdent,
	tkColon,
	tkDoubleColon,
	tkComma,
	tkArrow,
	tkPLeft,
	tkPRight,
	tkSLeft,
	tkSRight,
	tkArray,
	tkBLeft,
	tkBRight
} tt;

int tn;
std::string ti;
std::wstring ts;
char ch;
int maxRegister;
int currentLine;

void getCh()
{
	ch = getchar();
}

void getToken()
{
	while (ch == ' ' || ch == 9 || ch == 10 || ch == 13)
	{
		/*if (ch == 13)
			printf("%d,", currentLine++);*/
		getCh();
	}
	if (isdigit(ch) || ch == '-' || ch == '+')
	{
		tt = tkNum;
		tn = 0;
		char prefix = ch;
		bool neg = prefix == '-';
		if (prefix == '+' || prefix == '-')
			getCh();
		if (prefix == '-' && ch == '>')
		{
			tt = tkArrow;
			getCh();
			return;
		}
		while (isdigit(ch))
		{
			tn = tn * 10 + ch - '0';
			getCh();
		}
		if (neg)
			tn = -tn;
		return;
	}
	else if (isalpha(ch) || ch == '.' || ch == '_')
	{
		tt = tkIdent;
		ti = ch;
		getCh();
		while (isalpha(ch) || isdigit(ch) || ch == '.' || ch == '_')
		{
			ti += ch;
			getCh();
		}
		return;
	}
	switch (ch)
	{
	case EOF:
		tt = tkEOF;
		return;

	case '"':
	{
		tt = tkString;
		getCh();
		ti = "";
		while (ch != '"')
		{
			if (ch == '\\')
			{
				getCh();
				switch (ch)
				{
				case 'n': ti = ti + '\n'; break;
				case 'r': ti = ti + '\r'; break;
				case 't': ti = ti + '\t'; break;
				default: ti = ti + ch; break;
				}
			}
			else
				ti = ti + ch;
			getCh();
		}
		const char *bytes = ti.c_str();
		int size = MultiByteToWideChar(CP_ACP, 0, bytes, ti.length(), NULL, 0);
		wchar_t *buffer = (wchar_t *) alloca(size * sizeof(wchar_t));
		MultiByteToWideChar(CP_ACP, 0, bytes, ti.length(), buffer, size);
		ts = L"";
		for (int i = 0; i < size; i++)
			ts = ts + buffer[i];
		getCh();
		return;
	}

	case '$':
		tt = tkRegister;
		tn = 0;
		getCh();
		while (isdigit(ch))
		{
			tn = tn * 10 + ch - '0';
			getCh();
		}
		maxRegister = std::max(maxRegister, tn);
		return;

	case ':':
		tt = tkColon;
		getCh();
		if (ch == ':')
		{
			tt = tkDoubleColon;
			getCh();
		}
		return;

	case ',':
		tt = tkComma;
		getCh();
		return;

	case '(':
		tt = tkPLeft;
		getCh();
		return;

	case ')':
		tt = tkPRight;
		getCh();
		return;

	case '[':
		getCh();
		if (ch == ']')
		{
			getCh();
			tt = tkArray;
			return;
		}
		tt = tkSLeft;
		return;

	case ']':
		tt = tkSRight;
		getCh();
		return;

	case '{':
		tt = tkBLeft;
		getCh();
		return;

	case '}':
		tt = tkBRight;
		getCh();
		return;

	case '#': // single line comment
		getCh();
		while (ch != 10)
			getCh();
		getCh();
		getToken();
		return;

	case '/': // comments
		getCh();
		if (ch == '/') // single line comment
		{
			getCh();
			while (ch != 10)
				getCh();
			getCh();
			getToken();
		}
		else
		{
			assert(ch == '*'); // multi line comment
			getCh();
			char b = ch;
			getCh();
			while (b != '*' || ch != '/')
			{
				b = ch;
				getCh();
			}
			getCh();
			getToken();
		}
	}
}

int getSimpleType()
{
	assert(tt == tkIdent);
	std::string t = ti;
	getToken();
	if (t == "int8")
		return new_type_ref(TYPE_INT8);
	else if (t == "uint8")
		return new_type_ref(TYPE_UINT8);
	else if (t == "int16")
		return new_type_ref(TYPE_INT16);
	else if (t == "uint16")
		return new_type_ref(TYPE_UINT16);
	else if (t == "int32")
		return new_type_ref(TYPE_INT32);
	else if (t == "uint32")
		return new_type_ref(TYPE_UINT32);
	else if (t == "int64")
		return new_type_ref(TYPE_INT64);
	else if (t == "uint64")
		return new_type_ref(TYPE_UINT64);
	else if (t == "float32")
		return new_type_ref(TYPE_FLOAT32);
	else if (t == "float64")
		return new_type_ref(TYPE_FLOAT64);
	else if (t == "int")
		return new_type_ref(TYPE_INT);
	else if (t == "uint")
		return new_type_ref(TYPE_UINT);
	else if (t == "bool")
		return new_type_ref(TYPE_BOOL);
	else if (t == "char")
		return new_type_ref(TYPE_CHAR);
	else
		return new_class_ref(t);
}

int getType()
{
	std::vector<int> param_types, return_types;
	if (tt == tkPLeft)
	{
		/* Prototype */
		getToken();
		if (tt != tkPRight)
		{
			param_types.push_back(getType());
			while (tt == tkComma)
			{
				getToken();
				param_types.push_back(getType());
			}
		}
		assert(tt == tkPRight);
		getToken();
		if (tt == tkArrow)
		{
			getToken();
			return_types.push_back(getType());
			while (tt == tkComma)
			{
				getToken();
				return_types.push_back(getType());
			}
		}
		return new_proto_ref(TYPE_FUNCTION, param_types, return_types);
	}
	else
	{
		int type_ref = getSimpleType();
		while (tt == tkArray)
		{
			getToken();
			type_ref = new_array_ref(type_ref);
		}
		return type_ref;
	}
}

int getPrimitiveType()
{
	assert(tt == tkIdent);
	std::string t = ti;
	getToken();
	if (t == "int8")
		return TYPE_INT8;
	else if (t == "uint8")
		return TYPE_UINT8;
	else if (t == "int16")
		return TYPE_INT16;
	else if (t == "uint16")
		return TYPE_UINT16;
	else if (t == "int32")
		return TYPE_INT32;
	else if (t == "uint32")
		return TYPE_UINT32;
	else if (t == "int64")
		return TYPE_INT64;
	else if (t == "uint64")
		return TYPE_UINT64;
	else if (t == "float32")
		return TYPE_FLOAT32;
	else if (t == "float64")
		return TYPE_FLOAT64;
	else if (t == "int")
		return TYPE_INT;
	else if (t == "uint")
		return TYPE_UINT;
	else
	{
		puts("Unknown primitive type.");
		return 0;
	}
}

int getFieldRef()
{
	assert(tt == tkIdent);
	int class_ref = new_class_ref(ti);
	getToken();
	assert(tt == tkDoubleColon);
	getToken();
	int field_ref = new_field_ref(class_ref, ti);
	getToken();
	return field_ref;
}

int getMethodRef()
{
	assert(tt == tkIdent);
	int class_ref = new_class_ref(ti);
	getToken();
	assert(tt == tkDoubleColon);
	getToken();
	assert(tt == tkIdent);
	std::string method_name = ti;
	getToken();
	return new_method_ref(class_ref, method_name, getType());
}

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
int getModifier()
{
	int modifier = 0;
	bool ok = true;
	while (ok)
	{
		if (ti == "public")
			modifier |= MODIFIER_PUBLIC;
		else if (ti == "private")
			modifier |= MODIFIER_PRIVATE;
		else if (ti == "protected")
			modifier |= MODIFIER_PROTECTED;
		else if (ti == "static")
			modifier |= MODIFIER_STATIC;
		else if (ti == "virtual")
			modifier |= MODIFIER_VIRTUAL;
		else if (ti == "override")
			modifier |= MODIFIER_OVERRIDE;
		else if (ti == "abstract")
			modifier |= MODIFIER_ABSTRACT;
		else if (ti == "internal")
			modifier |= MODIFIER_INTERNAL;
		else if (ti == "native")
			modifier |= MODIFIER_NATIVE;
		else if (ti == "interface")
			modifier |= MODIFIER_INTERFACE;
		else
			ok = false;
		if (ok)
			getToken();
	}
	return modifier;
}

#define CC_UNKNOWN			0
#define CC_STDCALL			1
int getCallingConvention()
{
	int cc = 0;
	if (ti == "stdcall")
		cc = CC_STDCALL;
	else
		assert(("Unknown calling convention", false));
	getToken();
	return cc;
}

#define OP(inst, exp) if (opcode == inst) { exp continue; }

#define NOP(opcode) \
	{ \
		PUT(opcode), PUT(0x00), PUT(0x00), PUT(0x00); \
	}

#define SINGLE(opcode) \
	{ \
		assert(tt == tkRegister); \
		PUT(opcode), PUT(tn), PUT(0), PUT(0); \
		getToken(); \
	}

#define BINI(opcode) \
	{ \
		assert(tt == tkRegister); \
		int a = tn; \
		getToken(); \
		assert(tt == tkComma); \
		getToken(); \
		int b = tn; \
		assert(tt == tkRegister); \
		getToken(); \
		assert(tt == tkComma); \
		getToken(); \
		if (tt == tkRegister) \
			PUT(opcode), PUT(a), PUT(b), PUT(tn); \
		else \
		{ \
			assert(tt == tkNum); \
			PUT(opcode + 1), PUT(a), PUT(b), PUT(0), PUT4(tn); \
		} \
		getToken(); \
	}

#define BINADDR(opcode) \
	{ \
		assert(tt == tkRegister); \
		int a = tn; \
		getToken(); \
		assert(tt == tkComma); \
		getToken(); \
		assert(tt == tkRegister); \
		int b = tn; \
		getToken(); \
		assert(tt == tkComma); \
		getToken(); \
		assert(tt == tkIdent); \
		PUT(opcode), PUT(a), PUT(b), PUT(0x00); \
		label_patch_list.push_back(std::make_pair(code.size() / 4, ti)); \
		PUT4(0); \
		getToken(); \
	}

#define UNARYADDR(opcode) \
	{ \
		assert(tt == tkRegister); \
		int a = tn; \
		getToken(); \
		assert(tt == tkComma); \
		getToken(); \
		assert(tt == tkIdent); \
		PUT(opcode), PUT(a), PUT(0x00), PUT(0x00); \
		label_patch_list.push_back(std::make_pair(code.size() / 4, ti)); \
		PUT4(0); \
		getToken(); \
	}

#define JMP(opcode) \
	{ \
		assert(tt == tkIdent); \
		PUT(opcode), PUT(0x00), PUT(0x00), PUT(0x00); \
		label_patch_list.push_back(std::make_pair(code.size() / 4, ti)); \
		PUT4(0); \
		getToken(); \
	}

#define JINST(opcode) \
	{ \
		assert(tt == tkRegister); \
		int a = tn; \
		getToken(); \
		assert(tt == tkComma); \
		getToken(); \
		int type = getType(); \
		assert(tt == tkComma); \
		getToken(); \
		assert(tt == tkIdent); \
		PUT(opcode), PUT(a), PUT2(type); \
		label_patch_list.push_back(std::make_pair(code.size() / 4, ti)); \
		PUT4(0); \
		getToken(); \
	}

#define BIN(opcode) \
	{ \
		assert(tt == tkRegister); \
		int a = tn; \
		getToken(); \
		assert(tt == tkComma); \
		getToken(); \
		assert(tt == tkRegister); \
		int b = tn; \
		getToken(); \
		assert(tt == tkComma); \
		getToken(); \
		assert(tt == tkRegister); \
		PUT(opcode), PUT(a), PUT(b), PUT(tn); \
		getToken(); \
	}
#define UNARY(opcode) \
	{ \
		assert(tt == tkRegister); \
		int a = tn; \
		getToken(); \
		assert(tt == tkComma); \
		getToken(); \
		assert(tt == tkRegister); \
		PUT(opcode), PUT(a), PUT(tn), PUT(0); \
		getToken(); \
	}
#define LDF(opcode_ldf, opcode_stf) \
	{ \
		assert(tt == tkRegister); \
		int o = tn; \
		getToken(); \
		assert(tt == tkComma); \
		getToken(); \
		if (tt == tkRegister) \
		{ \
			/* ldf $a, $b, @field */ \
			int p = tn; \
			getToken(); \
			assert(tt == tkComma); \
			getToken(); \
			int field_ref = getFieldRef(); \
			PUT(opcode_ldf), PUT(o), PUT(p), PUT(0x00), PUT4(field_ref); \
		} \
		else \
		{ \
			/* ldf $b, @field, $a */ \
			int field_ref = getFieldRef(); \
			assert(tt == tkComma); \
			getToken(); \
			assert(tt == tkRegister); \
			int p = tn; \
			PUT(opcode_stf), PUT(p), PUT(o), PUT(0x00), PUT4(field_ref); \
			getToken(); \
		} \
	}
#define LDS(opcode_lds, opcode_sts) \
	{ \
		if (tt == tkRegister) \
		{ \
			/* lds $a, @field */ \
			int a = tn; \
			getToken(); \
			assert(tt == tkComma); \
			getToken(); \
			int field_ref = getFieldRef(); \
			PUT(opcode_lds), PUT(a), PUT2(field_ref); \
		} \
		else \
		{ \
			/* lds @field, $a */ \
			int field_ref = getFieldRef(); \
			assert(tt == tkComma); \
			getToken(); \
			assert(tt == tkRegister); \
			int a = tn; \
			PUT(opcode_sts), PUT(a), PUT2(field_ref); \
			getToken(); \
		} \
	}

#define LDI(opcode_ldi, opcode_sti) \
	{ \
		if (tt == tkRegister) \
		{ \
			/* ldi $a, ($b) */ \
			int a = tn; \
			getToken(); \
			assert(tt == tkComma); \
			getToken(); \
			assert(tt == tkPLeft); \
			getToken(); \
			assert(tt == tkRegister); \
			int b = tn; \
			getToken(); \
			assert(tt == tkPRight); \
			PUT(opcode_ldi), PUT(a), PUT(b), PUT(0x00); \
			getToken(); \
		} \
		else \
		{ \
			/* ldi ($b), $a */ \
			assert(tt == tkPLeft); \
			getToken(); \
			assert(tt == tkRegister); \
			int b = tn; \
			getToken(); \
			assert(tt == tkPRight); \
			getToken(); \
			assert(tt == tkComma); \
			getToken(); \
			assert(tt == tkRegister); \
			PUT(opcode_sti), PUT(tn); PUT(b), PUT(0x00); \
			getToken(); \
		} \
	}

#define LDE(opcode_lde, opcode_ste) \
	{ \
		assert(tt == tkRegister); \
		int r = tn; \
		getToken(); \
		if (tt == tkComma) \
		{ \
			/* lde $a, $b($c) */ \
			getToken(); \
			assert(tt == tkRegister); \
			int b = tn; \
			getToken(); \
			assert(tt == tkPLeft); \
			getToken(); \
			assert(tt == tkRegister); \
			int c = tn; \
			getToken(); \
			assert(tt == tkPRight); \
			getToken(); \
			PUT(opcode_lde), PUT(r), PUT(b), PUT(c); \
		} \
		else \
		{ \
			/* lde $b($c), $a */ \
			assert(tt == tkPLeft); \
			getToken(); \
			assert(tt == tkRegister); \
			int c = tn; \
			getToken(); \
			assert(tt == tkPRight); \
			getToken(); \
			assert(tt == tkComma); \
			getToken(); \
			assert(tt == tkRegister); \
			PUT(opcode_ste), PUT(tn), PUT(r), PUT(c); \
			getToken(); \
		} \
	}

#define CALL(opcode) \
	{ \
		assert(tt == tkRegister); \
		int s = tn; \
		getToken(); \
		assert(tt == tkComma); \
		getToken(); \
		int method_ref = getMethodRef(); \
		PUT(opcode), PUT(s), PUT2(method_ref); \
	}

#define EXCEPTION_CLAUSE_CATCH		0
#define EXCEPTION_CLAUSE_FINALLY	1

std::string compile_method(int *register_count, int *code_size, int *exception_clause_count)
{
	std::unordered_map<std::string, int> labels;
	std::vector<std::pair<int, std::string>> label_patch_list;
	maxRegister = 0;
	std::string code, handler_table;
	int clauseCount = 0;

	std::function<void()> compile_method_internal = [&]() {
		assert(tt == tkBLeft);
		getToken();
		for (;;)
		{
			if (tt == tkBRight)
			{
				getToken();
				return;
			}
			assert(tt == tkIdent);
			std::string opcode = ti;
			getToken();
			while (tt == tkColon)
			{
				labels.insert(std::make_pair(opcode, code.size() / 4));
				getToken();
				opcode = ti;
				getToken();
			}
			if (opcode == ".try")
			{
				int try_start = code.size() / 4;
				compile_method_internal();
				int try_end = code.size() / 4;
				while (tt == tkIdent && (ti == ".catch" || ti == ".finally"))
				{
					int flags;
					int catch_class_ref = 0, catch_reg = 0;
					if (ti == ".catch")
					{
						flags = EXCEPTION_CLAUSE_CATCH;
						getToken();
						assert(tt == tkPLeft);
						getToken();
						assert(tt == tkIdent);
						catch_class_ref = new_class_ref(ti);
						getToken();
						assert(tt == tkRegister);
						catch_reg = tn;
						getToken();
						assert(tt == tkPRight);
						getToken();
					}
					else if (ti == ".finally")
					{
						flags = EXCEPTION_CLAUSE_FINALLY;
						getToken();
					}
					int handler_start = code.size() / 4;
					compile_method_internal();
					int handler_end = code.size() / 4;

					/* Update handler table */
					_PUT4(handler_table, flags);
					_PUT4(handler_table, try_start);
					_PUT4(handler_table, try_end);
					_PUT4(handler_table, handler_start);
					_PUT4(handler_table, handler_end);
					_PUT2(handler_table, catch_class_ref);
					_PUT2(handler_table, catch_reg);
					clauseCount++;
					/* Update try block end for use with next block */
					try_end = code.size() / 4;
				}
				continue;
			}
			OP("addi", BINI(0x00))
			OP("subi", BINI(0x02))
			OP("muli", BINI(0x04))
			OP("mulu", BINI(0x06))
			OP("divi", BINI(0x08))
			OP("divu", BINI(0x0A))
			OP("remi", BINI(0x0C))
			OP("remu", BINI(0x0E))
	
			OP("andi", BINI(0x10))
			OP("ori", BINI(0x12))
			OP("xori", BINI(0x14))
			OP("noti", UNARY(0x16))
			OP("negi", UNARY(0x17))
			OP("sli", BINI(0x18))
			OP("sri", BINI(0x1A))
			OP("sru", BINI(0x1C))
	
			OP("andl", BIN(0x20))
			OP("subl", BIN(0x21))
			OP("mull", BIN(0x22))
			OP("mulul", BIN(0x23))
			OP("divl", BIN(0x24))
			OP("divul", BIN(0x25))
			OP("reml", BIN(0x26))
			OP("remul", BIN(0x27))
			OP("andl", BIN(0x28))
			OP("orl", BIN(0x29))
			OP("xorl", BIN(0x2A))
			OP("notl", BIN(0x2B))
			OP("negl", BIN(0x2C))
			OP("sll", BIN(0x2D))
			OP("srl", BIN(0x2E))
			OP("srul", BIN(0x2F))
	
			OP("addn", BIN(0x30))
			OP("addni", BIN(0x31))
			OP("subn", BIN(0x32))
			OP("subni", BIN(0x33))
	
			OP("jmp", JMP(0x60))
			OP("jn", UNARYADDR(0x61))
			OP("jnn", UNARYADDR(0x62))

			OP("jinst", JINST(0x64))
			OP("jninst", JINST(0x65))
	
			OP("jzi", UNARYADDR(0x70))
			OP("jeqzi", UNARYADDR(0x70))
			OP("jnzi", UNARYADDR(0x71))
			OP("jneqzi", UNARYADDR(0x71))
			OP("jgtzi", UNARYADDR(0x72))
			OP("jgezi", UNARYADDR(0x73))
			OP("jltzi", UNARYADDR(0x74))
			OP("jlezi", UNARYADDR(0x75))
			OP("jeqi", BINADDR(0x76))
			OP("jneqi", BINADDR(0x77))
			OP("jgti", BINADDR(0x78))
			OP("jgtu", BINADDR(0x79))
			OP("jgei", BINADDR(0x7A))
			OP("jgeu", BINADDR(0x7B))
			OP("jlti", BINADDR(0x7C))
			OP("jltu", BINADDR(0x7D))
			OP("jlei", BINADDR(0x7E))
			OP("jleu", BINADDR(0x7F))
	
			OP("ldfb", LDF(0x80, 0x88))
			OP("ldfub", LDF(0x81, 0x89))
			OP("ldfw", LDF(0x82, 0x8A))
			OP("ldfuw", LDF(0x83, 0x8B))
			OP("ldfi", LDF(0x84, 0x8C))
			OP("ldfl", LDF(0x85, 0x8D))
			OP("ldfv", LDF(0x86, 0x8E))
			OP("ldfa", LDF(0x87, 0x8F))
	
			OP("ldsb", LDS(0x90, 0x98))
			OP("ldsub", LDS(0x91, 0x99))
			OP("ldsw", LDS(0x92, 0x9A))
			OP("ldsuw", LDS(0x93, 0x9B))
			OP("ldsi", LDS(0x94, 0x9C))
			OP("ldsl", LDS(0x95, 0x9D))
			OP("ldsv", LDS(0x96, 0x9E))
			OP("ldsa", LDS(0x97, 0x9F))
	
			OP("ldib", LDI(0xA0, 0xA8))
			OP("ldiub", LDI(0xA1, 0xA9))
			OP("ldiw", LDI(0xA2, 0xAA))
			OP("ldiuw", LDI(0xA3, 0xAB))
			OP("ldii", LDI(0xA4, 0xAC))
			OP("ldil", LDI(0xA5, 0xAD))
			OP("ldiv", LDI(0xA6, 0xAE))
			OP("ldia", LDI(0xA7, 0xAF))
	
			OP("ldeb", LDE(0xB0, 0xB8))
			OP("ldeub", LDE(0xB1, 0xB9))
			OP("ldew", LDE(0xB2, 0xBA))
			OP("ldeuw", LDE(0xB3, 0xBB))
			OP("ldei", LDE(0xB4, 0xBC))
			OP("ldel", LDE(0xB5, 0xBD))
			//OP("ldev", LDEV(0xB6, 0xBE))
			OP("ldea", LDE(0xB7, 0xBF))
			if (opcode == "ldi")
			{
				assert(tt == tkRegister);
				int a = tn;
				getToken();
				assert(tt == tkComma);
				getToken();
				if (tt == tkRegister) // register
					PUT(0xC0), PUT(a), PUT(tn), PUT(0x00);
				else // immediate
				{
					assert(tt == tkNum);
					PUT(0xC4), PUT(a), PUT(0x00), PUT(0x00), PUT4(tn);
				}
				getToken();
				continue;
			}
			if (opcode == "lda")
			{
				assert(tt == tkRegister);
				int a = tn;
				getToken();
				assert(tt == tkComma);
				getToken();
				assert(tt == tkRegister);
				PUT(0xC3), PUT(a), PUT(tn), PUT(0);
				getToken();
				continue;
			}
			if (opcode == "ldstr")
			{
				assert(tt == tkRegister);
				int a = tn;
				getToken();
				assert(tt == tkComma);
				getToken();
				assert(tt == tkString);
				PUT(0xC8), PUT(a), PUT2(new_string(ts));
				getToken();
				continue;
			}
			OP("ldlen", UNARY(0xC9))
			OP("ldnull", SINGLE(0xCB))
			OP("laf", LDF(0xCC, 0xCC)) /* FIXME */
			OP("las", LDF(0xCD, 0xCD)) /* FIXME */
			OP("lar", UNARY(0xCE))
			OP("lae", LDE(0xCF, 0xCF)) /* FIXME */
			if (opcode == "ret")
			{
				if (tt == tkRegister)
				{
					int a = tn;
					getToken();
					if (tt == tkComma)
					{
						/* retn */
						getToken();
						assert(tt == tkRegister);
						PUT(0xDA), PUT(a), PUT(tn), PUT(0);
						getToken();
					}
					else /* ret1 */
						PUT(0xD9), PUT(a), PUT(0), PUT(0);
				}
				else /* ret0 */
					PUT(0xD8), PUT(0), PUT(0), PUT(0);
				continue;
			}
			OP("new", CALL(0xD0))
			if (opcode == "newarr")
			{
				/* newarr $a, type($b) */
				assert(tt == tkRegister);
				int a = tn;
				getToken();
				assert(tt == tkComma);
				getToken();
				int type_ref = getType();
				assert(tt == tkPLeft);
				getToken();
				assert(tt == tkRegister);
				int b = tn;
				getToken();
				assert(tt == tkPRight);
				getToken();
				PUT(0xD1), PUT(a), PUT(b), PUT(0);
				PUT2(new_type_ref(TYPE_ARRAY, type_ref)), PUT2(0);
				continue;
			}
			OP("call", CALL(0xDB))
			OP("callv", CALL(0xDC))
			OP("throw", SINGLE(0xE0))
			OP("rethrow", NOP(0xE1))
			OP("leave", JMP(0xE2))
			OP("endfinally", NOP(0xE3))
			if (opcode == "cast")
			{
				/* cast $a, $b, type */
				assert(tt == tkRegister);
				int a = tn;
				getToken();
				assert(tt == tkComma);
				getToken();
				assert(tt == tkRegister);
				int b = tn;
				getToken();
				assert(tt == tkComma);
				getToken();
				int type_ref = getType();
				PUT(0xE7), PUT(a), PUT(b), PUT(0);
				PUT2(type_ref), PUT2(0);
				continue;
			}
			/* conversion */
			if (opcode == "conv")
			{
				/* conv dsttype $a, srctype $b */
				int dsttype = getPrimitiveType();
				assert(tt == tkRegister);
				int a = tn;
				getToken();
				assert(tt == tkComma);
				getToken();
				int srctype = getPrimitiveType();
				assert(tt == tkRegister);
				int b = tn;
				getToken();
				PUT(0xE6), PUT(a), PUT(b), PUT((dsttype << 4) | srctype);
				continue;
			}
			// special opcodes
			if (opcode == "printi")
			{
				assert(tt == tkRegister);
				PUT(0xFF), PUT(0x00), PUT(0x00), PUT(tn);
				getToken();
				continue;
			}
			if (opcode == "printu")
			{
				assert(tt == tkRegister);
				PUT(0xFF), PUT(0x01), PUT(0x00), PUT(tn);
				getToken();
				continue;
			}
			if (opcode == "printl")
			{
				assert(tt == tkRegister);
				PUT(0xFF), PUT(0x02), PUT(0x00), PUT(tn);
				getToken();
				continue;
			}
			if (opcode == "printul")
			{
				assert(tt == tkRegister);
				PUT(0xFF), PUT(0x03), PUT(0x00), PUT(tn);
				getToken();
				continue;
			}
			if (opcode == "prints")
			{
				assert(tt == tkRegister);
				PUT(0xFF), PUT(0x04), PUT(0x00), PUT(tn);
				getToken();
				continue;
			}
			if (opcode == "printc")
			{
				assert(tt == tkRegister);
				PUT(0xFF), PUT(0x05), PUT(0x00), PUT(tn);
				getToken();
				continue;
			}
			if (opcode == "readi")
			{
				assert(tt == tkRegister);
				PUT(0xFF), PUT(0x08), PUT(0x00), PUT(tn);
				getToken();
				continue;
			}
			if (opcode == "readu")
			{
				assert(tt == tkRegister);
				PUT(0xFF), PUT(0x09), PUT(0x00), PUT(tn);
				getToken();
				continue;
			}
			if (opcode == "readl")
			{
				assert(tt == tkRegister);
				PUT(0xFF), PUT(0x0A), PUT(0x00), PUT(tn);
				getToken();
				continue;
			}
			if (opcode == "readul")
			{
				assert(tt == tkRegister);
				PUT(0xFF), PUT(0x0B), PUT(0x00), PUT(tn);
				getToken();
				continue;
			}
			if (opcode == "reads")
			{
				assert(tt == tkRegister);
				PUT(0xFF), PUT(0x0C), PUT(0x00), PUT(tn);
				getToken();
				continue;
			}
			printf("Unknown opcode: %s\n", opcode.c_str());
			assert(("Unknown opcode.", 0));
		}
	};

	compile_method_internal();

	*register_count = maxRegister + 1;
	*exception_clause_count = clauseCount;
	*code_size = code.size() / 4;
	for (int i = 0; i < label_patch_list.size(); i++)
	{
		auto it = labels.find(label_patch_list[i].second);
		assert(it != labels.end());
		PUT4AT(it->second - label_patch_list[i].first, label_patch_list[i].first * 4);
	}
	return code + handler_table;
}

void compile_class()
{
	int field_def_table_start_index = field_def_table.size();
	int method_def_table_start_index = method_def_table.size();
	int field_count = 0;
	int method_count = 0;

	int class_modifier = getModifier();
	assert(tt == tkIdent);
	std::string name = ti;
	getToken();
	
	int base_class_ref;
	if (tt == tkColon)
	{
		getToken();
		base_class_ref = getType();
	}
	else if (name == "Core.Object")
		base_class_ref = 0;
	else
		base_class_ref = new_class_ref("Core.Object");

	assert(tt == tkBLeft);
	getToken();
	while (tt != tkBRight)
	{
		assert(tt == tkIdent);
		if (ti == ".field")
		{
			getToken();
			std::string data;
			int modifier = getModifier();
			int type = getType();
			assert(tt == tkIdent);
			_PUT2(data, new_internal_string(ti));
			_PUT2(data, type);
			_PUT2(data, modifier);
			_PUT2(data, 0); /* padding */
			field_def_table.push_back(data);
			field_count++;
			getToken();
		}
		else if (ti == ".method")
		{
			getToken();
			std::string data;
			int modifier = getModifier();
			assert(tt == tkIdent && ti == "function");
			getToken();
			_PUT2(data, new_internal_string(ti));
			getToken();
			_PUT2(data, getType());
			_PUT2(data, modifier);
			if (modifier & MODIFIER_INTERNAL || modifier & MODIFIER_ABSTRACT || class_modifier == MODIFIER_INTERFACE)
			{
				/* Internal or abstract method or methods within an interface does not have code */
				/* Paddings */
				_PUT2(data, 0);
				_PUT4(data, 0);
				_PUT4(data, 0);
				_PUT4(data, 0);
			}
			else if (modifier & MODIFIER_NATIVE)
			{
				assert(tt == tkSLeft);
				getToken();
				int callingconvention = getCallingConvention();
				assert(tt == tkComma);
				getToken();
				assert(tt == tkString);
				int dllname = new_internal_string(ti);
				getToken();
				assert(tt == tkComma);
				getToken();
				assert(tt == tkString);
				int originalname = new_internal_string(ti);
				getToken();
				assert(tt == tkSRight);
				getToken();
				_PUT2(data, callingconvention);
				_PUT2(data, dllname);
				_PUT2(data, originalname);
				_PUT4(data, 0); /* padding */
				_PUT4(data, 0); /* padding 2 */
			}
			else
			{
				int register_count, code_size, clause_count;
				std::string code = compile_method(&register_count, &code_size, &clause_count);
				_PUT2(data, register_count);
				_PUT4(data, code_heap.size());
				_PUT4(data, code_size);
				_PUT4(data, clause_count);
				code_heap += code;
			}
			method_def_table.push_back(data);
			method_count++;
		}
	}
	getToken(); /* Skip '}' */
	std::string data;
	_PUT2(data, new_internal_string(name));
	_PUT2(data, base_class_ref); /* padding */
	_PUT2(data, field_count);
	_PUT2(data, field_def_table_start_index);
	_PUT2(data, method_count);
	_PUT2(data, method_def_table_start_index);
	class_def_table.push_back(data);
}

void compile()
{
	while (tt == tkIdent)
	{
		if (ti == ".class")
		{
			getToken();
			compile_class();
		}
		else
			assert(("Unexpected identifier.", false));
	}
}

void output()
{
	// Aqua identifier
	FPUT('A');
	FPUT('q');
	FPUT('u');
	FPUT('a');
	// Bytecode version
	FPUT4(0);

	// Table sizes
	FPUT2(internal_string_index_table.size());
	FPUT2(string_index_table.size());
	FPUT2(type_index_table.size());
	FPUT2(field_ref_table.size());
	FPUT2(method_ref_table.size());
	FPUT2(class_def_table.size());
	FPUT2(field_def_table.size());
	FPUT2(method_def_table.size());
	
	// Heap sizes
	FPUT4(internal_string_heap.size());
	FPUT4(string_heap.size());
	FPUT4(type_heap.size());
	FPUT4(code_heap.size());

	// Index tables
	FPUTIT(internal_string_index_table);
	FPUTIT(string_index_table);
	FPUTIT(type_index_table);
	
	// Tables
	FPUTT(field_ref_table);
	FPUTT(method_ref_table);
	FPUTT(class_def_table);
	FPUTT(field_def_table);
	FPUTT(method_def_table);

	// Heaps
	FPUTS(internal_string_heap);
	FPUTS(string_heap);
	FPUTS(type_heap);
	FPUTS(code_heap);
}

int main(int argc, char **argv)
{
	assert(argc >= 3);
	for (int i = 1; i < argc - 1; i++)
	{
		freopen(argv[i], "rb", stdin);
		printf("Assembling %s...", argv[i]);
		currentLine = 2;
		getCh();
		getToken();
		compile();
		printf("\n");
	}
	puts("Assembler done.");
	freopen(argv[argc - 1], "wb", stdout);
	output();
	return 0;
}
