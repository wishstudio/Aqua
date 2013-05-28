#pragma once

#include "PlatformTypes.h"

uint32 utf8ToUtf16(const char *data, uint32 length, uint16 *outdata);
uint32 utf16ToUtf8(const uint16 *data, uint32 length, char *outdata);
uint32 hashString(const void *string, uint32 length);
