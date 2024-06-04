// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//  Abstract:
//  Common types used in DWrite code

typedef XUINT8  uint8_t;         typedef XINT8  int8_t;
typedef XUINT16 uint16_t;        typedef XINT16 int16_t;
typedef XUINT32 uint32_t;        typedef XINT32 int32_t;
typedef XUINT64 uint64_t;        typedef XINT64 int64_t;

typedef XUINT8  UINT8;
typedef XUINT32 UINT32;

#if defined(_PREFAST_)
// SIZE_MAX is used in the SAL annotation of the itemization code
#ifndef SIZE_MAX
#define SIZE_MAX XUINT32_MAX
#endif
#endif


//------------------------------------------------------------------------
//
//  Unicode character classification methods
//
//------------------------------------------------------------------------

extern const XUINT8 g_ucdDataBytes[];

// Creates an OpenType tag as a 32bit integer such that
// the first character in the tag is the lowest byte,
// (least significant on little endian architectures)
// which can be used to compare with tags in the font file.
// This macro is compatible with DWRITE_FONT_FEATURE_TAG.
//
// Example: DWRITE_MAKE_OPENTYPE_TAG('c','c','m','p')
// Dword:   0x706D6363


#define DWRITE_MAKE_OPENTYPE_TAG(a,b,c,d) ( \
    (static_cast<UINT32>(static_cast<UINT8>(d)) << 24) | \
    (static_cast<UINT32>(static_cast<UINT8>(c)) << 16) | \
    (static_cast<UINT32>(static_cast<UINT8>(b)) << 8)  | \
     static_cast<UINT32>(static_cast<UINT8>(a)))
