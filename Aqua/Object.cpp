#include <Windows.h>

#include "Object.h"
#include "VMTypes.h"
#include "Resolve.h"

String *CreateString(int32 size)
{	
	String *string = (String *) malloc(sizeof(String) + size + 1);
	string->vtable = stringClass->vtable;
	string->size = size;
	string->data[string->size] = 0;
	return string;
}

String *CreateString(int32 size, const char *data)
{
	String *string = (String *) malloc(sizeof(String) + size + 1);
	string->vtable = stringClass->vtable;
	string->size = size;
	string->data[string->size] = 0;
	memcpy(string->data, data, size);
	return string;
}

String *CreateString(int32 size, const uint16 *data)
{
	int32 utf8Size = utf16ToUtf8(data, size, nullptr);
	String *string = (String *) malloc(sizeof(String) + size + 1);
	string->vtable = stringClass->vtable;
	string->size = utf8Size;
	string->data[string->size] = 0;
	utf16ToUtf8(data, size, string->data);
	return string;
}
