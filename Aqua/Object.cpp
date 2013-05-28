#include "Object.h"
#include "VMTypes.h"
#include "Resolve.h"

String *CreateString(int32 length)
{	
	String *string = (String *) malloc(sizeof(String) + length * 2 + 2);
	string->vtable = stringClass->vtable;
	string->length = length;
	string->data[string->length] = 0;
	return string;
}

String *CreateString(int32 length, const uint16 *data)
{
	String *string = (String *) malloc(sizeof(String) + length * 2 + 2);
	string->vtable = stringClass->vtable;
	string->length = length;
	string->data[string->length] = 0;
	memcpy(string->data, data, string->length * 2);
	return string;
}
