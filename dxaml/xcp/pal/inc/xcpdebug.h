// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Common constants for debugging services used from both
//      C and C++ code.

#ifndef __XCPDEBUG__
#define __XCPDEBUG__

#include "mindebug.h"
#include "xcperror.h"
#include "errorcontext.h"

// Monitor macro - controls whether ifc and debug macros use extended
// monitor functionality.
//
// Monitor functionality is off in prefast builds.
//
// Monitor functionality may be set from the compiler command line by
// predefining the symbol XCP_MONITOR as 0 or 1.
//
// If it not set on the command line then it defaults on.

#pragma once

#define USE_VIRTUAL_ALLOC 0

// The following macros which enable the definition of __WFILE__ are based on
// those described in MSDN library under __FILE__.
#if !defined(__WFILE__)
    #define __t1__(x)    L##x
    #define __t2__(x)    __t1__(x)
    #define __t3__(x,y)  __t1__(#y)
    #define __WFILE__    __t2__(__FILE__)
#endif

#if XCP_MONITOR

    // Monitor call types. Must be non-zero.

    enum MonitorCallType
    {
        MonitorAssert                  =  1,
        MonitorAssertSuccess           =  2,
        MonitorTrace                   =  3,
        MonitorIfc                     =  4,
        MonitorIfcOom                  =  5,
        MonitorIfcPtr                  =  6,
        MonitorIfcHndl                 =  7,
        MonitorIfcExpect               =  8,
        MonitorIfcW32                  =  9,
        MonitorIfcOsx                  = 10,
        MonitorIfcLS                   = 11,
        MonitorEnterSection            = 12,
        MonitorLeaveSection            = 13,
        MonitorEnterMethod             = 14,
        MonitorLeaveMethod             = 15,
        MonitorAllocation              = 16,
        MonitorDeallocation            = 17,
        MonitorMemoryHeaderCorruption  = 18,
        MonitorMemoryTrailerCorruption = 19,
        MonitorAllocationPerf          = 20,
        MonitorDeallocationPerf        = 21,
        MonitorRaw                     = 22,
        MonitorMaxType                 = 23,
        MonitorAnsiArgs                = 0x80000000u  // Reserved flag indicating varargs are char rather than WCHAR by default
    };

    // Trace flags control which messages are sent to xcpmon / OutputDebugString.
    //
    // Each flag is a bit in global XUINT32 '*(gps->m_pTraceFlags)'.
    //
    // When connected to xcpmon, gps->m_pTraceFlags addresses a trace flags doubleword
    // in the shared section, allowing xcpmon to turn individual tracing flags on
    // and off dynamically.
    //
    // If XcpMonitor is not attached, gps->m_pTraceFlags references a local doubleword that
    // is initialized to TraceAlways, so that only TRACE(Always, ...) messages are displayed.
    //
    // Trace flags are powers of 2.
    //
    // IMPORTANT: When adding to the trace flags, there are values in xcpmon that need to be changed in:
    // xcpmon/resource.h (add a new IDC_CHECK)
    // xcpmon/settings.cpp (add the new IDC_CHECK to Command)
    // xcpmon/xcpmon.rc (modify sizes and add Control to DIALOGEX)
    enum TraceFlags
    {
        TraceAlways            = 0x00000001u,  // (normally on)
        TraceIfc               = 0x00000002u,
        AssertIfc              = 0x00000004u,  // ASSERT on IFC macro
        TraceAlloc             = 0x00000008u,  // Trace every memory allocation / deallocation
        TraceSection           = 0x00000010u,  // Trace every ENTERSECTION/LEAVESECTION macro
        TraceGlyphs            = 0x00000020u,  // Glyph element parsing, path generation and rendering
        TraceZlibV0            = 0x00000040u,  // ZLIB trace call class V0
        TraceZlibV1            = 0x00000080u,  // ZLIB trace call class V1
        TraceZlibV2            = 0x00000100u,  // ZLIB trace call class V2
        TraceAllocationStack   = 0x00000200u,  // Indicates checked allocation should record the call stack
        TraceSubRuns           = 0x00000400u,  // Trace subruns created during line formatting
        TraceFontLoad          = 0x00000800u,  // Trace font uri loading
        TraceMedia             = 0x00001000u,  // Trace Media/DRM events
        TraceLeakAddresses     = 0x00002000u,  // Prints full allocation callstacks even when no symbols are found
        TraceLeakFullUpFront   = 0x00004000u,  // Prints all leaks upfront
        TraceLeakHexDump       = 0x00008000u,  // Prints a HEX dump of the inner memory of the object
        TraceLeakOwned         = 0x00010000u,  // Prints a separate list of the OWNED leak objects
        TraceLeakTopLevel      = 0x00020000u,  // Prints a separate list of the TOP LEVEL leak objects
        TraceLeakFullRefs      = 0x00040000u,  // Prints AddRef/Release stacks for non leaked objects on delete
        TraceLeakCoreChildren  = 0x00080000u,  // Includes children (sub-objects) of the core when printing leaks
        TraceSeadragonDraw     = 0x00100000u,  // Traces draw bounds and blend factors used for Seadragon tiles
        TraceManagedPeerStress = 0x00200000u,  // Enable managed peer stress testing
        TraceFontLoadDetail    = 0x00400000u,  // Internal details of OpenType table parsing
        TraceUnused2           = 0x00800000u,
        TraceShowGlyphMode     = 0x01000000u,  // Underline runs formatted using LS glyph mode
        TraceDrm               = 0x02000000u,  // Add DRM specific tracing
        TraceMemoryPerf        = 0x04000000u,  // Track and report memory usage by section
        TraceBidiLevels        = 0x08000000u,  // Dump resolved bidi levels
        TraceFetchedRuns       = 0x10000000u,  // Dump runs in the fetchrun cache every time fetchrun is called (slow)
    };

    #define MONITOR_TRACE_NAMES_HERE                      \
    const WCHAR *TraceNames[] =                           \
    {                                                     \
        L"0x00000001 Always",                             \
        L"0x00000002 IFC*",                               \
        L"0x00000004 Break on IFC*",                      \
        L"0x00000008 Allocations",                        \
        L"0x00000010 Trace ENTERSECTION/LEAVESECTION",    \
        L"0x00000020 Glyphs",                             \
        L"0x00000040 ZLib V0",                            \
        L"0x00000080 ZLib V1",                            \
        L"0x00000100 ZLib V2",                            \
        L"0x00000200 Record allocation stack",            \
        L"0x00000400 Subruns from LS",                    \
        L"0x00000800 Font loading",                       \
        L"0x00001000 Media event",                        \
        L"0x00002000 Allocation without symbols",         \
        L"0x00004000 Full upfront leak dump",             \
        L"0x00008000 HEX leaked memory dump",             \
        L"0x00010000 Owned leaked objects",               \
        L"0x00020000 Top-level leaked objects",           \
        L"0x00040000 Refcount for non-leaked",            \
        L"0x00080000 Include leaked core children",       \
        L"0x00100000 Seadragon tile drawings",            \
        L"0x00200000 Enable managed peer stress testing", \
        L"0x00400000 Internal details of OpenType parse", \
        L"0x00800000 ",                                   \
        L"0x01000000 Underline glyph mode runs",          \
        L"0x02000000 DRM Traces",                         \
        L"0x04000000 Memory usage by section",            \
        L"0x08000000 Resolved bidi levels",               \
        L"0x10000000 Cached runs each FetchRun",          \
        L"0x80000000 "                                    \
    };


    // Enter/Leave section macros control performance analysis breakdown in xcpmon:
    // time and memory usage is accounted separately by section.
    // Trace, assertion and ifc messages are also annotated with current section name.

    // Defined processing sections

    enum SectionIndices
    {
        SectionInitial                                 =  0,
        SectionMain                                    =  1,
        SectionFontStartup                             =  2,
        SectionGetFontResource                         =  3,
        SectionDetermineFontNames                      =  4,
        SectionRasterizeFrame                          =  5,
        SectionParseIndices                            =  6,
        SectionGlyphsRender                            =  7,
        SectionGlyphsFinalize                          =  8,
        SectionTextLayout                              =  9,
        SectionBreakLine                               = 10,
        SectionSubRunCreation                          = 11,
        SectionGlyphRunCreation                        = 12,
        SectionTextEdgeStoreBuild                      = 13, // Includes glyph typeface lookup
        SectionGetGlyphEdgeStore                       = 14, // Includes Cmap and Advance width lookup
        SectionEnsureGlyphPaths                        = 15,
        SectionCacheGlyphPaths                         = 16,
        SectionTrueTypeRasterize                       = 17,
        SectionLookupGlyphTypeface                     = 18,
        SectionLookupGlyphTypefaceUncached             = 19,
        SectionLsTextFormatterFormatLine               = 20,
        SectionLineServicesFetchRun                    = 21,
        SectionXcpTextSourceGetTextRun                 = 22,
        SectionXcpTextSourceCacheRunData               = 23,
        SectionCacheRawParagraph                       = 24,
        SectionAnalyseParagraph                        = 25,
        SectionGetTextRunFromData                      = 26,
        SectionGetTextRunFromDataLoop                  = 27,
        SectionScriptAnalysis                          = 28,
      //SectionTextIteratorAdvance                     = 29,
      //SectionTextIteratorAdvanceSlow                 = 30,
      //SectionTextIteratorLoadCharacter               = 31,
        SectionResolveTextProperties                   = 32,
        SectionResolveTextPropertiesLoop               = 33,
        SectionFontFamilyGetFontTypeface               = 34,
        SectionInlineCollectionGetRun                  = 35,
        SectionInlineCollectionGetRunDummyEOP          = 36,
        SectionInlineCollectionGetRunStartEnd          = 37,
        SectionInlineCollectionGetRunNested            = 38,
        SectionInlineCollectionGetRunGetCollection     = 39,
        SectionInlineCollectionCachePositionCounts     = 40,
        SectionGetFormattingProperties                 = 41,
        SectionLineServicesGetRunCharacterWidths       = 42,
        SectionXcpFontServicesGetCharacterWidths       = 43,
        SectionCharacterWidthLoop                      = 44,
        SectionLineServicesGetGlyphs                   = 45,
        SectionShapingFontGetGlyphs                    = 46,
        SectionLineServicesGetGlyphPositions           = 47,
        SectionLoadScriptEngine                        = 48,
        SectionShapingFontGetDefaultGlyphsW            = 49,
        SectionShapingFontGetDefaultGlyphsL            = 50,
        SectionShapingFontGetGlyphDefaultAdvanceWidths = 51,
        SectionShapingFontGetFontTable                 = 52,
        SectionShapingCacheWriterAllocateCacheSlot     = 53,
        SectionLineServicesGetBreakingClasses          = 54,
        SectionInitializeTypes                         = 55,
        SectionRegisterKnownClass                      = 56,
        SectionRegisterKnownProperty                   = 57,
        SectionCreatePropertyArray                     = 58,
        SectionRegisterPropertyHelper                  = 59,
        SectionCreateRenderPropertyArray               = 60,
        SectionCreateKnownIndexMap                     = 61,
        SectionInitializeSchemaContext                 = 62,
        SectionDOGetValue                              = 63,
        SectionDOSetValue                              = 64,
        SectionIsPropertyDefaultByIndex                = 65,
        SectionPullIsRightToLeft                       = 66,
        SectionPullIsRightToLeftGetParent              = 67,
        SectionMax                                     = 68
    };

    // Section names to be used in xcpmon.exe.
    // xcpmon.exe instatiates this macro.

    #define MONITOR_SECTION_NAMES_HERE                \
    const WCHAR *SectionNames[] =                     \
    {                                                 \
        L"(unspecified)",                             \
        L"Main",                                      \
        L" FontStartup",                              \
        L"  GetFontResource",                         \
        L"    DetermineFontNames",                    \
        L" RasterizeFrame",                           \
        L" ParseIndices",                             \
        L" GlyphsRender",                             \
        L"  GlyphsFinalize",                          \
        L" TextLayout",                               \
        L" BreakLine",                                \
        L"  SubRunCreation",                          \
        L"   GlyphRunCreation",                       \
        L"  TextEdgeStoreBuild",                      \
        L"   GetGlyphEdgeStore",                      \
        L"    EnsureGlyphPaths",                      \
        L"     CacheGlyphPaths",                      \
        L"      TrueTypeRasterize",                   \
        L"      LookupGlyphTypeface",                 \
        L"      LookupGlyphTypefaceUncached",         \
        L"LsTextFormatterFormatLine",                 \
        L" LineServicesFetchRun",                     \
        L"  XcpTextSourceGetTextRun",                 \
        L"   XcpTextSourceCacheRunData",              \
        L"    CacheRawParagraph",                     \
        L"    AnalyseParagraph",                      \
        L"     GetTextRunFromData",                   \
        L"     GetTextRunFromDataLoop",               \
        L"     ScriptAnalysis",                       \
        L"     TextIteratorAdvance",                  \
        L"     TextIteratorAdvanceSlow",              \
        L"     TextIteratorLoadCharacter",            \
        L"    ResolveTextProperties",                 \
        L"     ResolveTextPropertiesLoop",            \
        L"     FontFamilyGetFontTypeface",            \
        L"     InlineCollectionGetRun",               \
        L"     InlineCollGetRunDummyEOP",             \
        L"     InlineCollGetRunStartEnd",             \
        L"     InlineCollGetRunNested",               \
        L"     InlineCollGetRunGetCollection",        \
        L"      InlColCachePosCnts",                  \
        L"       GetFormattingProperties",            \
        L" LineServicesGetRunCharacterWidth",         \
        L"   XcpFontServicesGetCharacterWidths",      \
        L"    CharacterWidthLoop",                    \
        L" LineServicesGetGlyphs",                    \
        L"  ShapingFontGetGlyphs",                    \
        L" LineServicesGetGlyphPositions",            \
        L"  LoadScriptEngine",                        \
        L"  ShapingFontGetDefaultGlyphsW",            \
        L"  ShapingFontGetDefaultGlyphsL",            \
        L"  ShapingFontGetGlyphDefaultAdvanceWidths", \
        L"  ShapingFontGetFontTable",                 \
        L"  ShapingCacheWriterAllocateCacheSlot",     \
        L" LineServicesGetBreakingClasses",           \
        L" InitializeTypes",                          \
        L"  RegisterKnownClass",                      \
        L"  RegisterKnownProperty",                   \
        L"  CreatePropertyArray",                     \
        L"    RegisterPropertyHelper",                \
        L"  CreateRenderPropertyArray",               \
        L"  CreateKnownIndexMap",                     \
        L"  InitializeSchemaContext",                 \
        L"  DOGetValue",                              \
        L"  DOSetValue",                              \
        L"  IsPropertyDefaultByIndex",                \
        L"  PullIsRightToLeft",                       \
        L"    PullIsRightToLeftGetParent",            \
    };

#endif // #if XCP_MONITOR

// Only issue ETW render events in CHK builds, but this can be changed for instrumented builds
#define RENDER_EVENTS DBG

//
// BEGIN NON-PLATFORM-INDEPENDENT
//

//
// If this non-platform-independent section of xcpdebug.h is considered harmful,
// then one possible fix is to have a platform-specific header into which this stuff
// can be moved.  The platform-specific header would have the same filename, but be
// in a different directory, for each platform.
//

#ifndef VOID
typedef void VOID;
#endif

typedef int                 BOOL;

#if (defined(_M_IX86) || defined(_M_IA64) || defined(_M_AMD64) || defined(_M_ARM) || defined(_M_ARM64)) && !defined(MIDL_PASS)
#define DECLSPEC_IMPORT __declspec(dllimport)
#else
#define DECLSPEC_IMPORT
#endif

#if !defined(WINBASEAPI)
#if !defined(_KERNEL32_)
#define WINBASEAPI DECLSPEC_IMPORT
#else
#define WINBASEAPI
#endif
#endif

#ifdef _MAC
#define WINAPI      CDECL
#elif (_MSC_VER >= 800) || defined(_STDCALL_SUPPORTED)
#define WINAPI      __stdcall
#else
#define WINAPI
#endif

#ifdef __cplusplus
extern "C"
#endif
WINBASEAPI
BOOL
WINAPI
IsDebuggerPresent(
    VOID
    );

#ifdef __cplusplus
extern "C"
#endif
WINBASEAPI
VOID
WINAPI
DebugBreak(
    VOID
    );

//
// END NON-PLATFORM-INDEPENDENT
//

#endif

