// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <macros.h>
#include <minxcptypes.h>

// Provides a simple non-localized character classification.  This is only
// useful in the type converters like float and color where the values are all
// in US English.
class CCharTypes
{
public:
    CCharTypes()    {}
   ~CCharTypes()    {}

static const XINT32 s_types[128];
};

// These now match the equivalent bit values in crt\ctype.h with all
// additional fields coming after 0x0100

#define XCLASS_UPPER    XINT32(0x0001)
#define XCLASS_LOWER    XINT32(0x0002)
#define XCLASS_DIGIT    XINT32(0x0004)
#define XCLASS_SPACE    XINT32(0x0008)
#define XCLASS_PUNCT    XINT32(0x0010)
#define XCLASS_CONTROL  XINT32(0x0020)
#define XCLASS_XDIGIT   XINT32(0x0080)
#define XCLASS_FLEADING XINT32(0x0200)
#define XCLASS_NAME     XINT32(0x0400)
#define XCLASS_JSNAME  XINT32(0x0800)

XINT32 xisspace(_In_ WCHAR ch);
XINT32 xisdigit(_In_ WCHAR ch);
XINT32 xisxdigit(_In_ WCHAR ch);
XINT32 xisfleading(_In_ WCHAR ch);
XINT32 xisupper(_In_ WCHAR ch);
XINT32 xislower(_In_ WCHAR ch);
XINT32 xisalpha(_In_ WCHAR ch);
XINT32 xisalnum(_In_ WCHAR ch);
XINT32 xisname(_In_ WCHAR ch);
XINT32 xisname0(_In_ WCHAR ch);