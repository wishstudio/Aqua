#pragma once

#include "VMTypes.h"

String *CreateString(int32 size);
String *CreateString(int32 size, const char *data);
String *CreateString(int32 size, const uint16 *data);

template<typename T>
Array<T, 1> *CreateArray(int32 size)
{
	Array<T, 1> *array = (Array<T, 1> *) malloc(sizeof(Array<T, 1>) + sizeof(T) * size);
	array->size[0] = size;
	return array;
}
