// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Unicode Shaping Library
//  Content:     Shaping library public type definitons

#pragma once

//-----------------------------------------
// Standard type definitions
//

#ifndef NULL
#ifdef __cplusplus
#define NULL    0
#else
#define NULL    ((void *)0)
#endif
#endif

#ifndef FALSE
#define FALSE (0L)
#endif

#ifndef TRUE
#define TRUE (1L)
#endif

typedef int             BOOL;
#ifdef __LP64__
typedef signed int      LONG;
typedef unsigned int   DWORD;
#else
typedef long            LONG;
typedef unsigned long   DWORD;
#endif
typedef short           SHORT;
typedef unsigned short  WORD;
typedef unsigned short  USHORT;
typedef unsigned int    UINT;
typedef unsigned char   BYTE;
typedef BYTE*           PBYTE;
typedef int             INT;
typedef INT*            PINT;
typedef wchar_t         WCHAR;
typedef WCHAR*          PWCHAR;
typedef const WCHAR*    PCWCHAR;
typedef WCHAR*          LPWSTR;
typedef const WCHAR*    LPCWSTR;
typedef unsigned long long ULONGLONG;

typedef unsigned long   LCHAR;
typedef WORD            GLYPHID;

typedef DWORD           OPENTYPE_TAG;

typedef unsigned short  SHAPING_CLUSTERMAP;
typedef void*           SHAPING_HANDLE;

#ifndef MAX_INT
#define MAX_INT (0x7FFFFFFF)
#endif

#ifndef MAX_LONG
#define MAX_LONG (0x7FFFFFFF)
#endif

#ifndef MAX_USHORT
#define MAX_USHORT 0xFFFF
#endif

#ifndef MAX_DWORD
#define MAX_DWORD (0xFFFFFFFF)
#endif

#define MAX_CHARS 0xFFFF
#define MAX_GLYPHS 0xFFFF

//-----------------------------------------
// Error code and standard errors
//

#define SHERR_NONE                  (0)
typedef _Success_(return == SHERR_NONE) LONG SHERR;

#define SHERR_INVALIDPARAMETER          (-1)
#define SHERR_OUTOFMEMORY               (-2)
#define SHERR_INSUFFICIENTBUFFERSIZE    (-3)

#define SHERR_INTERNAL                  (-100)
#define SHERR_NOTIMPLEMENTED            (-101)
#define SHERR_CLIENT                    (-102)

#define SHERR_SCRIPTNOTINFONT           (-200)

#define SHERR_FONTTABLENOTFOUND         (-300)
#define SHERR_CACHESLOTNOTFOUND         (-301)

#define SHERR_UNKNOWN                   (-1000)

//-----------------------------------------
// Script definitions
//

typedef LONG SHAPING_SCRIPT;

// This enumeration has to be shared with clients
// because they need to do itemization and script-specific
// font mapping.
enum SHAPING_SCRIPTS
{
    SHAPING_SCRIPT_UNKNOWN      = 0,
    SHAPING_SCRIPT_NEUTRAL      = 1,
    SHAPING_SCRIPT_LATIN        = 2,
    SHAPING_SCRIPT_CYRILLIC     = 3,
    SHAPING_SCRIPT_GREEK        = 4,
    SHAPING_SCRIPT_ARMENIAN     = 5,
    SHAPING_SCRIPT_GEORGIAN     = 6,
    SHAPING_SCRIPT_SURROGATE    = 7,
    SHAPING_SCRIPT_PRIVATE      = 8,
    SHAPING_SCRIPT_KANA         = 9,
    SHAPING_SCRIPT_HAN          = 10,
    SHAPING_SCRIPT_BOPOMOFO     = 11,
    SHAPING_SCRIPT_YI           = 12,
    SHAPING_SCRIPT_FE_COMMON    = 13,
    SHAPING_SCRIPT_ETHIOPIC     = 14,
    SHAPING_SCRIPT_CANADIAN     = 15,
    SHAPING_SCRIPT_CHEROKEE     = 16,
    SHAPING_SCRIPT_BRAILLE      = 17,
    SHAPING_SCRIPT_RUNIC        = 18,
    SHAPING_SCRIPT_OGHAM        = 19,
    SHAPING_SCRIPT_MONGOLIAN    = 20,
    SHAPING_SCRIPT_PHAGS_PA     = 21,

    SHAPING_SCRIPT_ARABIC       = 22,
    SHAPING_SCRIPT_SYRIAC       = 23,

    SHAPING_SCRIPT_HEBREW       = 24,
    SHAPING_SCRIPT_HEBCURRENCY  = 25,

    SHAPING_SCRIPT_CONTROLCHAR  = 26,

    SHAPING_SCRIPT_KHMER        = 27,
    SHAPING_SCRIPT_SINHALA      = 28,
    SHAPING_SCRIPT_TIBETAN      = 29,
    SHAPING_SCRIPT_TAI_LE       = 30,
    SHAPING_SCRIPT_THAANA       = 31,
    SHAPING_SCRIPT_NEW_TAI_LUE  = 32,
    SHAPING_SCRIPT_THAI         = 33,
    SHAPING_SCRIPT_LAO          = 34,

    SHAPING_SCRIPT_DIGITS       = 35,
    SHAPING_SCRIPT_MYANMAR      = 36,

    SHAPING_SCRIPT_HANGUL       = 37,
    SHAPING_SCRIPT_HINDI        = 38,
    SHAPING_SCRIPT_TAMIL        = 39,
    SHAPING_SCRIPT_BENGALI      = 40,
    SHAPING_SCRIPT_BENCURRENCY  = 41,
    SHAPING_SCRIPT_GURMUKHI     = 42,
    SHAPING_SCRIPT_GUJARATI     = 43,
    SHAPING_SCRIPT_GUJCURRENCY  = 44,
    SHAPING_SCRIPT_ORIYA        = 45,
    SHAPING_SCRIPT_TELUGU       = 46,
    SHAPING_SCRIPT_KANNADA      = 47,
    SHAPING_SCRIPT_MALAYALAM    = 48,

    SHAPING_SCRIPT_FARSI_NUM    = 49,
    SHAPING_SCRIPT_URDU_NUM     = 50,

// The SHAPING_SCRIPT_COUNT _must_ be #define, not the enum value because it is
// used in later in the preprocessor-based compile-time validations.

#define SHAPING_SCRIPT_COUNT 51

//@@ As engines for these scripts are completed, their ordinal value
//@@ should be moved ahead of SHAPING_SCRIPT_COUNT
/*    SHAPING_SCRIPT_TAGALOG,
    SHAPING_SCRIPT_HANUNOO,
    SHAPING_SCRIPT_BUHID,
    SHAPING_SCRIPT_TAGBANWA,
    SHAPING_SCRIPT_LIMBU,
    SHAPING_SCRIPT_BUGINESE,
    SHAPING_SCRIPT_BALINESE,
    SHAPING_SCRIPT_SUNDANESE,
    SHAPING_SCRIPT_LEPCHA,
    SHAPING_SCRIPT_OL_CHIKI,
    SHAPING_SCRIPT_SYMBOLS,
    SHAPING_SCRIPT_GLAGOLITIC,
    SHAPING_SCRIPT_TIFINAGH,
    SHAPING_SCRIPT_VAI,
    SHAPING_SCRIPT_SYLOTI_NAGRI,
    SHAPING_SCRIPT_SAURASHTRA,
    SHAPING_SCRIPT_KAYAH_LI,
    SHAPING_SCRIPT_REJANG,
    SHAPING_SCRIPT_CHAM,
    SHAPING_SCRIPT_LINEAR_B,
    SHAPING_SCRIPT_LYCIAN,
    SHAPING_SCRIPT_CARIAN,
    SHAPING_SCRIPT_OLD_ITALIC,
    SHAPING_SCRIPT_GOTHIC,
    SHAPING_SCRIPT_UGARITIC,
    SHAPING_SCRIPT_OLD_PERSIAN,
    SHAPING_SCRIPT_DESERET,
    SHAPING_SCRIPT_SHAVIAN,
    SHAPING_SCRIPT_OSMANYA,
    SHAPING_SCRIPT_CYPRIOT,
    SHAPING_SCRIPT_PHOENICIAN,
    SHAPING_SCRIPT_LYDIAN,
    SHAPING_SCRIPT_KHAROSHTHI,
    SHAPING_SCRIPT_CUNEIFORM,
    SHAPING_SCRIPT_TAMCURRENCY,
    SHAPING_SCRIPT_MATH_ALPHANUM,
*/
};


//-----------------------------------------
// Justification types
//

enum SHAPING_SCRIPT_JUSTIFICATION
{
    SHAPING_SCRIPT_JUSTIFY_NONE             = 0,    // Justification can't be applied at this glyph
    SHAPING_SCRIPT_JUSTIFY_ARABIC_BLANK     = 1,    // This glyph represents a blank in an Arabic run
    SHAPING_SCRIPT_JUSTIFY_CHARACTER        = 2,    // Inter-character justification point follows this glyph
    SHAPING_SCRIPT_JUSTIFY_RESERVED1        = 3,    // Reserved #1
    SHAPING_SCRIPT_JUSTIFY_BLANK            = 4,    // This glyph represents a blank outside an Arabic run
    SHAPING_SCRIPT_JUSTIFY_RESERVED2        = 5,    // Reserved #2
    SHAPING_SCRIPT_JUSTIFY_RESERVED3        = 6,    // Reserved #3
    SHAPING_SCRIPT_JUSTIFY_ARABIC_NORMAL    = 7,    // Normal Middle-Of-Word glyph that connects to the right (begin)
    SHAPING_SCRIPT_JUSTIFY_ARABIC_KASHIDA   = 8,    // Kashida(U+640) in middle of word
    SHAPING_SCRIPT_JUSTIFY_ARABIC_ALEF      = 9,    // Final form of Alef-like (U+627, U+625, U+623, U+632)
    SHAPING_SCRIPT_JUSTIFY_ARABIC_HA        = 10,   // Final form of Ha (U+647)
    SHAPING_SCRIPT_JUSTIFY_ARABIC_RA        = 11,   // Final form of Ra (U+631)
    SHAPING_SCRIPT_JUSTIFY_ARABIC_BA        = 12,   // Middle-Of-Word form of Ba (U+628)
    SHAPING_SCRIPT_JUSTIFY_ARABIC_BARA      = 13,   // Ligature of alike (U+628,U+631)
    SHAPING_SCRIPT_JUSTIFY_ARABIC_SEEN      = 14,   // Highest priority: Initial shape of Seen(U+633) (end)
    SHAPING_SCRIPT_JUSTIFY_ARABIC_SEEN_M    = 15    // Reserved #4
};


//-----------------------------------------
// Shaping parameters
//

enum SHAPING_ORIENTATION {
    SHAPING_HORIZONTAL,
    SHAPING_VERTICAL,
    SHAPING_STACKED
};

struct SHAPING_PROPERTIES {
    SHAPING_SCRIPT      scriptId;
    OPENTYPE_TAG        tagScript;
    OPENTYPE_TAG        tagLangSys;

    SHAPING_ORIENTATION textOrientation;

                                            // Allocating 4 bytes for flags because the structure
                                            // has only int- and other 4-byte-based members, so will be
                                            // padded for 4 bytes boundary anyway.
    DWORD   fRtl                    :1;
    DWORD   fLinkBefore             :1;
    DWORD   fLinkAfter              :1;
    DWORD   fDisplayControlChars    :1;
    DWORD   fInhibitSymSwap         :1;     // Inhibit CMAP lookup for alternate glyphs for RTL mode
    DWORD   fInhibitCombPrecomp     :1;     // Inhibit the processing of combining marks into precomposed chars
    DWORD   fVisualLeftOffsets      :1;     // Treat the positioning offsets as the visual-left based
                                            // as opposed to the logical-left
    DWORD   fReserved               :25;    // Ramainder: the unused bits
};

struct SHAPING_TEXTMETRICS {
    LONG    lDesignUnitsPerEm;
    LONG    lPpemX;
    LONG    lPpemY;

// These two parameters are used for the Syriac Abbreviation Mark size calculation
    LONG    lHeight;
    LONG    lInternalLeading;
};

struct TEXTRANGE_FEATURE_RECORD {
    OPENTYPE_TAG    tagFeature;     // Feature tag
    LONG            lParameter;     // Feature parameter (0 - disabled)
};

struct SHAPING_TEXTRANGE_PROPERTIES {
    _Field_size_(cFeatureCount) TEXTRANGE_FEATURE_RECORD* pFeatures;
    LONG  cFeatureCount;
};

//-----------------------------------------
// Drawing surface type definitions
//

typedef void* SHAPING_DRAWING_PARAMETERS; // Drawing parameters provided by client

//-----------------------------------------
// Shaping and analysis properties
//

struct SHAPING_CHARPROP {
    WORD    fCanGlyphAlone      :1;
    WORD    fSkippedFromShaping :1;
    WORD    reserved            :14;    // Reserved
};

struct SHAPING_GLYPHPROP {
    WORD    uJustification  :4;     // Justification class /aka SHAPING_SCRIPT_JUSTIFICATION
    WORD    fClusterStart   :1;     // First glyph of representation of cluster
    WORD    fDiacritic      :1;     // Diacritic
    WORD    fZeroWidth      :1;     // Blank, ZWJ, ZWNJ etc, with no width
    WORD    fReserved       :1;     // General reserved
    WORD    fShapeReserved  :8;     // Reserved for use by shaping engines
};

struct SHAPING_BREAKINGPROP {
    BYTE    fSoftBreak      :1;     // Potential linebreak point
    BYTE    fWhiteSpace     :1;     // A unicode whitespace character, except NBSP, ZWNBSP
    BYTE    fCharStop       :1;     // Valid cursor position (for left/right arrow)
    BYTE    fWordStop       :1;     // Valid cursor position (for ctrl + left/right arrow)
    BYTE    fInvalid        :1;     // Invalid character sequence
    BYTE    fReserved       :3;
};

struct SHAPING_GLYPHOFFSET
{
    LONG    du;
    LONG    dv;
};

struct SHAPING_GLYPHMETRICS
{
    LONG    iLeftSidebearing;
    LONG    iAdvanceWidth;
    LONG    iRightSidebearing;
};

struct SHAPING_COMBINING_OPTIONS
{
    DWORD   fBreakTogether  :1;
    DWORD   fDrawTogether   :1;
};

// Character range structure for rotating glyphs information

struct SHAPING_CHARACTER_RANGE
{
    LCHAR   lchFirstChar;
    LCHAR   lchLastChar;
};

//-----------------------------------------
// Cache manager
//

typedef DWORD CACHE_SLOT_ID;
typedef DWORD CACHE_SLOT_VERSION;

//-------------------------------------------------------------------------------------
// Following definitions are taken from ShapingTypes, to allow reuse Uniscribe cache
// Move it back when Uniscribe will implement generic caching store
//

#define CACHE_SLOT_COMMON (0x00000000)
#define CACHE_SLOT_OTLS   (0x00000001)

//
// Engine-specific caches will go after this id.
//
// Script-specific cache will use CACHE_SLOT_ENGINE_BASE+script
// cache slot id.
//
#define CACHE_SLOT_ENGINE_BASE         (0x00000100)

//
// End of cache slot definitions
//-------------------------------------------------------------------------------------

