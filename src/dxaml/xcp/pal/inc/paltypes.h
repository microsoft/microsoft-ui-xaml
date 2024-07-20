// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//  Abstract:
//      Contains the basic types provided by the core platform abstraction layer

#ifndef __PAL__TYPES__
#define __PAL__TYPES__

#include "specstrings.h"
#include <minpal.h>
#include <windows.system.h>
#include <microsoft.ui.input.experimental.h>
#include <NamespaceAliases.h>

// TODO -- Check This
// This originated in xcp_error.h
#define E_INVALID_CHARS_IN_URI                          0x80000012

// REVIEW: For now we will take a small runtime perf hit to
// get PIC free builds under Xcode 3.0. This means we need
// to make sure that static(global) variables referenced in inline
// assembler code are frame variables and not global.
#define PIC_STATIC  static

// Disable warnings we don't need

#if !defined(__LITTLE_ENDIAN__) && !defined(__BIG_ENDIAN__)
#define __LITTLE_ENDIAN__ 1
#define __BIG_ENDIAN__ 0
#endif

#ifndef COMPILE_ASSERT
#define COMPILE_ASSERT(x) STATIC_ASSERT((x), CompileAssert)
#endif

#define XINFINITE 0xFFFFFFFF  // Infinite timeout

#ifdef SECURE_DEBUG
#define CRITICAL(c)  Critical##c
#define TREATASSAFE(c) TreatAsSafe##c
#define EXTERNALCRITICAL(c) ExternalCritical##c
#define EXTERNALTREATASSAFE(c) ExternalTreatAsSafe##c
#endif

//------------------------------------------------------------------------
// ***********************************************************************
//
// These need to be the minimum size for all platforms.
//
//
// ***********************************************************************
//------------------------------------------------------------------------

#define XINTERNET_MAX_HOST_NAME_LENGTH 256
#define XINTERNET_MAX_SCHEME_LENGTH 32
#define XINTERNET_MAX_FRAGMENT_LENGTH 20
#define XINTERNET_MAX_PATH_LENGTH 2048
#define XINTERNET_MAX_USER_NAME_LENGTH 128
#define XINTERNET_MAX_PASSWORD_LENGTH 128

// Struct for low/high word access

union XINT64_LARGE_INTEGER
{
    struct {
#if __BIG_ENDIAN__      // PowerPC
        XINT32  HighPart;
        XUINT32 LowPart;
#elif __LITTLE_ENDIAN__ // Intel
        XUINT32 LowPart;
        XINT32  HighPart;
#endif

    };
    XINT64 QuadPart;
};

// Boolean values

#ifndef FALSE
#define FALSE   0
#endif

#ifndef TRUE
#define TRUE    1
#endif

// Null

#ifndef NULL_VALUE
#define NULL_VALUE
#define NULL    0
#endif

// Thread priority levels.  Note, the internal OS values for these are NOT
// the same, this just establish an intent of the core for these values.

#define PAL_THREAD_PRIORITY_REAL_TIME   3
#define PAL_THREAD_PRIORITY_HIGH        2
#define PAL_THREAD_PRIORITY_ELEVATED    1
#define PAL_THREAD_PRIORITY_NORMAL      0
#define PAL_THREAD_PRIORITY_LOWERED    -1
#define PAL_THREAD_PRIORITY_LOW        -2
#define PAL_THREAD_PRIORITY_IDLE       -3

// Process priority classes.  A class defines the overall priority for the
// application. Threads can then be set relative to that class so you can
// have a high priority application with some low priority threads. As with
// thread priorities, these are not OS values.

#define PAL_PROCESS_PRIORITY_REAL_TIME  5
#define PAL_PROCESS_PRIORITY_HIGH       4
#define PAL_PROCESS_PRIORITY_ELEVATED   3
#define PAL_PROCESS_PRIORITY_NORMAL     2
#define PAL_PROCESS_PRIORITY_LOWERED    1
#define PAL_PROCESS_PRIORITY_IDLE       0

// Virtual page marks.  Allows pages to be readable, writable, executable as needed.

#define PAL_PAGE_READ                   1
#define PAL_PAGE_WRITE                  2
#define PAL_PAGE_EXECUTE                4

// Some commonly used structures

struct XVERTEX25D
{
    //
    // Fields
    //
    XFLOAT x;
    XFLOAT y;
    XFLOAT uOverW;
    XFLOAT vOverW;
    XFLOAT oneOverW;

#ifdef __cplusplus
    XVERTEX25D()
    {
        SetToIdentity();
    }

    XVERTEX25D(
        XFLOAT xScreen,
        XFLOAT yScreen,
        XFLOAT uTexture,
        XFLOAT vTexture,
        XFLOAT w )
    {
        // 2D Screen Space
        x = xScreen;
        y = yScreen;

        // Interpolators
        oneOverW = 1.0f / w;
        uOverW = uTexture / w;
        vOverW = vTexture / w;
    }

    XVERTEX25D(_In_ const XVERTEX25D &other)
    {
        x        = other.x;
        y        = other.y;
        uOverW   = other.uOverW;
        vOverW   = other.vOverW;
        oneOverW = other.oneOverW;
    }

    void SetToIdentity()
    {
        x = 0.0f;
        y = 0.0f;
        oneOverW = 1.0f;
        uOverW = 0.0f;
        vOverW = 0.0f;
    }

    bool IsIdentity() const
    {
        return
            (x == 0.0f &&
            y == 0.0f &&
            oneOverW == 1.0f &&
            uOverW == 0.0f &&
            vOverW == 0.0f );
    }

    //
    // Computed field accessors
    //
    XFLOAT u() const { return uOverW / oneOverW; };
    XFLOAT v() const { return vOverW / oneOverW; };
    XFLOAT w() const { return 1.0f   / oneOverW; };

    void ScaleToFixPointRange( XFLOAT rScale )
    {
        // Scale only the inverse components, since there's no need to change the
        // screen space stuff as it is not immune to scale changing...
        uOverW /= rScale;
        vOverW /= rScale;
        oneOverW /= rScale;
    }

    //
    // Operator overloads
    //
    void operator*=(_In_ const XDOUBLE rScale)
    {
        x        *= (XFLOAT)rScale;
        y        *= (XFLOAT)rScale;
        uOverW   *= (XFLOAT)rScale;
        vOverW   *= (XFLOAT)rScale;
        oneOverW *= (XFLOAT)rScale;
    }

    void operator/=(_In_ const XDOUBLE rScale)
    {
        XDOUBLE rInvScale = 1.0 / rScale;

        x        *= (XFLOAT)rInvScale;
        y        *= (XFLOAT)rInvScale;
        uOverW   *= (XFLOAT)rInvScale;
        vOverW   *= (XFLOAT)rInvScale;
        oneOverW *= (XFLOAT)rInvScale;
    }

    XVERTEX25D operator-(_In_ const XVERTEX25D &other) const
    {
        XVERTEX25D result;

        result.x        = x - other.x;
        result.y        = y - other.y;
        result.uOverW   = uOverW - other.uOverW;
        result.vOverW   = vOverW - other.vOverW;
        result.oneOverW = oneOverW - other.oneOverW;

        return result;
    }

    XVERTEX25D operator+(_In_ const XVERTEX25D &other) const
    {
        XVERTEX25D result;

        result.x        = x + other.x;
        result.y        = y + other.y;
        result.uOverW   = uOverW + other.uOverW;
        result.vOverW   = vOverW + other.vOverW;
        result.oneOverW = oneOverW + other.oneOverW;

        return result;
    }

    XVERTEX25D operator*(_In_ const XFLOAT rScale) const
    {
        XVERTEX25D result;

        result.x        = x * rScale;
        result.y        = y * rScale;
        result.uOverW   = uOverW * rScale;
        result.vOverW   = vOverW * rScale;
        result.oneOverW = oneOverW * rScale;

        return result;
    }

#endif //__cplusplus
};

// Intermediate structure used for interpolation of clipped 3D coordinates
struct SDPoint4UV
{
    XFLOAT x;
    XFLOAT y;
    XFLOAT z;
    XFLOAT w;
    XFLOAT u, v;
};

// scRGB color struct

struct XCOLORF {
    XFLOAT r;
    XFLOAT g;
    XFLOAT b;
    XFLOAT a;
};

// This flag must be synced with Win7 touch flag definition(win\inc\touchwin7.h)
#define TOUCH_MESSAGE_MOVE      0x0001
#define TOUCH_MESSAGE_DOWN      0x0002
#define TOUCH_MESSAGE_UP        0x0004
#define TOUCH_MESSAGE_PRIMARY   0x0010

enum XPointerInputType
{
    XcpPointerInputTypePointer  = 1,
    XcpPointerInputTypeTouch    = 2,
    XcpPointerInputTypePen      = 3,
    XcpPointerInputTypeMouse    = 4
};

enum XcpHoldingState
{
    XcpHoldingStateStarted   = 0,
    XcpHoldingStateCompleted = 1,
    XcpHoldingStateCanceled  = 2
};

struct PhysicalKeyStatus
{
    XUINT32 m_uiRepeatCount;
    XUINT32 m_uiScanCode;
    bool m_bIsExtendedKey;
    bool m_bIsMenuKeyDown;
    bool m_bWasKeyDown;
    bool m_bIsKeyReleased;

#ifdef __cplusplus
    //Ctor
    PhysicalKeyStatus()
    {
        m_uiRepeatCount = 0;
        m_uiScanCode = 0;
        m_bIsExtendedKey = FALSE;
        m_bIsMenuKeyDown = FALSE;
        m_bWasKeyDown = FALSE;
        m_bIsKeyReleased = FALSE;
    }

    // Destructor
    ~PhysicalKeyStatus()
    {
        m_uiRepeatCount = 0;
        m_uiScanCode = 0;
        m_bIsExtendedKey = FALSE;
        m_bIsMenuKeyDown = FALSE;
        m_bWasKeyDown = FALSE;
        m_bIsKeyReleased = FALSE;
    }
#endif
};

// TODO: Task 23284769: Remove unneeded use of win32 input functions and structures
struct PointerInfo
{
    XUINT64 m_qpcInput                      = 0;
    XHANDLE m_pCoreWindow                   = nullptr;
    XPointerInputType m_pointerInputType    = XcpPointerInputTypePointer;
    XUINT32 m_pointerId                     = 0;
    XUINT32 m_frameId                       = 0;
    XPOINTF m_pointerLocation               = {};
    XUINT32 m_timeStamp                     = 0;
    bool    m_bInContact                    = false;
    bool    m_bInRange                      = false;
    bool    m_bInPrimary                    = false;
    bool    m_bCanceled                     = false;
    bool    m_bLeftButtonPressed            = false;
    bool    m_bRightButtonPressed           = false;
    bool    m_bMiddleButtonPressed          = false;
    bool    m_bBarrelButtonPressed          = false;
};

#ifdef __cplusplus
struct ManipulationTransform
{
    XPOINTF     m_pointTranslation;
    XFLOAT      m_floatScale;
    XFLOAT      m_floatExpansion;
    XFLOAT      m_floatRotation;

    //Ctor
    ManipulationTransform()
    {
        m_pointTranslation.x = 0;
        m_pointTranslation.y = 0;
        m_floatScale = 0;
        m_floatExpansion = 0;
        m_floatRotation = 0;
    }

    // Destructor
    ~ManipulationTransform()
    {
    }
};
#endif

#ifdef __cplusplus
struct ManipulationVelocity
{
    XPOINTF     m_pointLinear;
    XFLOAT      m_floatExpansion;
    XFLOAT      m_floatAngular;

    //Ctor
    ManipulationVelocity()
    {
        m_pointLinear.x = 0;
        m_pointLinear.y = 0;
        m_floatExpansion = 0;
        m_floatAngular = 0;
    }

    // Destructor
    ~ManipulationVelocity()
    {
    }
};
#endif


// DirectManipulation PAL structures and enums

enum XDMViewportInteractionType
{
    XcpDMViewportInteractionBegin = 0,
    XcpDMViewportInteractionManipulation = 1,
    XcpDMViewportInteractionGestureTap = 2,
    XcpDMViewportInteractionGestureHold = 3,
    XcpDMViewportInteractionGestureCrossSlide = 4,
    XcpDMViewportInteractionGesturePinchZoom = 5,
    XcpDMViewportInteractionEnd = 100
};

enum XDMViewportStatus
{
    XcpDMViewportBuilding = 0,
    XcpDMViewportEnabled = 1,
    XcpDMViewportDisabled = 2,
    XcpDMViewportRunning = 3,
    XcpDMViewportInertia = 4,
    XcpDMViewportReady = 5,
    XcpDMViewportSuspended = 6,
    XcpDMViewportAutoRunning = 7
};

// Keep in sync with DMConfigurations in DirectManipulationTypes.h
enum XDMConfigurations
{
    XcpDMConfigurationNone        = 0x00,
    XcpDMConfigurationInteraction = 0x01,
    XcpDMConfigurationPanX        = 0x02,
    XcpDMConfigurationPanY        = 0x04,
    XcpDMConfigurationZoom        = 0x10,
    XcpDMConfigurationPanInertia  = 0x20,
    XcpDMConfigurationZoomInertia = 0x80,
    XcpDMConfigurationRailsX      = 0x100,
    XcpDMConfigurationRailsY      = 0x200
};

// Keep in sync with DMMotionTypes in DirectManipulationTypes.h
enum XDMMotionTypes
{
    XcpDMMotionTypeNone    = 0x00,
    XcpDMMotionTypePanX    = 0x01,
    XcpDMMotionTypePanY    = 0x02,
    XcpDMMotionTypeZoom    = 0x04,
    XcpDMMotionTypeCenterX = 0x10,
    XcpDMMotionTypeCenterY = 0x20
};

// Keep in sync with DMAlignment in DirectManipulationTypes.h
enum XDMAlignment
{
    XcpDMAlignmentNone          = 0x00,
    XcpDMAlignmentNear          = 0x01,
    XcpDMAlignmentCenter        = 0x02,
    XcpDMAlignmentFar           = 0x04,
    XcpDMAlignmentUnlockCenter  = 0x08
};

// Keep in sync with DMSnapCoordinate in DirectManipulationTypes.h
enum XDMSnapCoordinate
{
    XcpDMSnapCoordinateBoundary = 0x00,
    XcpDMSnapCoordinateOrigin   = 0x01,
    XcpDMSnapCoordinateMirrored = 0x10
};

// Keep in sync with DMContentType in DirectManipulationTypes.h
enum XDMContentType
{
    XcpDMContentTypePrimary = 0,
    XcpDMContentTypeTopLeftHeader = 1,
    XcpDMContentTypeTopHeader = 2,
    XcpDMContentTypeLeftHeader = 3,
    XcpDMContentTypeCustom = 4,
    XcpDMContentTypeDescendant = 5
};

// Keep in sync with DMOverpanMode in DirectManipulationTypes.h
enum XDMOverpanMode
{
    XcpDMOverpanModeDefault         = 0x00,
    XcpDMOverpanModeNone            = 0x04,
};

// Keep in sync with DirectManipulationProperty in enumdefs.g.h
enum XDMProperty
{
    XcpDMPropertyNone = 0,
    XcpDMPropertyTranslationX = 1,
    XcpDMPropertyTranslationY = 2,
    XcpDMPropertyZoom = 3,
};

#ifdef __cplusplus
struct CParametricCurveSegmentDefinition
{
    XFLOAT m_beginOffset;
    XFLOAT m_constantCoefficient;
    XFLOAT m_linearCoefficient;
    XFLOAT m_quadraticCoefficient;
    XFLOAT m_cubicCoefficient;
};

struct CParametricCurveDefinition
{
    XUINT32 m_segments;
    CParametricCurveSegmentDefinition *m_pSegments;
    XDMProperty m_primaryDMProperty;
    XDMProperty m_secondaryDMProperty;

    CParametricCurveDefinition()
    {
        m_segments = 0;
        m_pSegments = NULL;
        m_primaryDMProperty = XcpDMPropertyNone;
        m_secondaryDMProperty = XcpDMPropertyNone;
    }
};
#endif

#define KEY_MODIFIER_ALT            0x0001
#define KEY_MODIFIER_CTRL           0x0002
#define KEY_MODIFIER_SHIFT          0x0004
#define KEY_MODIFIER_WINDOWS        0x0008
#define KEY_MODIFIER_MOUSEDOWN      0x0010

//--------------------------------------------------------
//
// PAL Text Types
//
//--------------------------------------------------------


enum MessageMap
{
    XCP_NULL = 0, // NO Message
    XCP_MOUSEMOVE = 1,
    XCP_LBUTTONDOWN = 2,
    XCP_LBUTTONUP = 3,
    XCP_RBUTTONDOWN = 4,
    XCP_RBUTTONUP = 5,
    XCP_MOUSEWHEEL = 6,
    XCP_MOUSELEAVE = 7,
    XCP_KEYUP = 8,
    XCP_KEYDOWN = 9,
    XCP_GOTFOCUS = 10,
    XCP_LOSTFOCUS = 11,
    XCP_TEXTCOMPOSITION_START = 12,
    XCP_TEXTCOMPOSITION_COMPOSE = 13,
    XCP_TEXTCOMPOSITION_END = 14,
    XCP_TEXTCOMPOSITION_COMPLETEDCHAR = 15,
    XCP_INPUTMETHOD_NOTIFY = 16,
    XCP_INPUTLANGCHANGE = 17,
    XCP_CHAR = 18,
    XCP_TOUCH = 19,
    XCP_DRAGENTER = 20,
    XCP_DRAGLEAVE = 21,
    XCP_DRAGOVER = 22,
    XCP_DROP = 23,
    XCP_ACTIVATE = 24,
    XCP_DEACTIVATE = 25,
    XCP_POINTERDOWN = 26,
    XCP_POINTERUPDATE = 27,
    XCP_POINTERUP = 28,
    XCP_POINTERENTER = 29,
    XCP_POINTERLEAVE = 30,
    XCP_POINTERCAPTURECHANGED = 31,
    XCP_POINTERWHEELCHANGED = 32,
    XCP_GESTURETAP = 33,
    XCP_GESTUREDOUBLETAP = 34,
    XCP_GESTUREHOLD = 35,
    XCP_GESTURERIGHTTAP = 36,
    XCP_MANIPULATIONSTARTED = 37,
    XCP_MANIPULATIONDELTA = 38,
    XCP_MANIPULATIONCOMPLETED = 39,
    XCP_MANIPULATIONINERTIASTARTING = 40,
    XCP_DEADCHAR = 41,
    XCP_DMPOINTERHITTEST = 42,
    XCP_CONTEXTMENU = 43,
    XCP_POINTERSUSPENDED = 44,
    XCP_WINDOWMOVE = 45,
};

// Enumeration representing editing concepts that are usually (but not always)
//  invoked with multi-key combinations that differ across platforms.
enum XEDITKEY
{
    XEDITKEY_NONE,
    XEDITKEY_CUT,
    XEDITKEY_COPY,
    XEDITKEY_PASTE,
    XEDITKEY_CHAR_LEFT,
    XEDITKEY_CHAR_RIGHT,
    XEDITKEY_CHAR_DELETE_PREV,
    XEDITKEY_CHAR_DELETE_NEXT,
    XEDITKEY_WORD_LEFT,
    XEDITKEY_WORD_RIGHT,
    XEDITKEY_WORD_DELETE_PREV,
    XEDITKEY_WORD_DELETE_NEXT,
    XEDITKEY_PAGE_PREV,
    XEDITKEY_PAGE_NEXT,
    XEDITKEY_LINE_START,
    XEDITKEY_LINE_END,
    XEDITKEY_DOCUMENT_START,
    XEDITKEY_DOCUMENT_END,
    XEDITKEY_UNDO,
    XEDITKEY_REDO,
    XEDITKEY_LINE_UP,
    XEDITKEY_LINE_DOWN,
    XEDITKEY_HANJA,
    XEDITKEY_SELECT_ALL
};

// C++ new and delete memory allocation
// forward declaration to ensure that proper destructor get used in ~InputMessage

#if defined(__cplusplus) && !defined(NO_XCP_NEW_AND_DELETE) // for example, xcpmon and drtapp set this.

XCP_FORCEINLINE __declspec(allocator) void * __cdecl operator new(size_t cSize);

__bcount(cSize) XCP_FORCEINLINE __declspec(allocator) void * __cdecl operator new[](size_t cSize);

#pragma warning(push)
#pragma warning(disable:26021 28301)
XCP_FORCEINLINE void __cdecl operator delete(_Frees_ptr_opt_ void *pAddress);

XCP_FORCEINLINE void __cdecl operator delete[](_Frees_ptr_opt_ void *pAddress);
#pragma warning(pop)

#endif

struct InputMessage
{
    XHANDLE         m_hWindow {nullptr};
    XHANDLE         m_hCoreWindow {nullptr};
    XHANDLE         m_hPlatformPacket {nullptr};
    XDWORD          m_langID {0};
    XINT32          m_xPos {0};
    XINT32          m_yPos {0};
    wsy::VirtualKey m_platformKeyCode {wsy::VirtualKey::VirtualKey_None};

    // Layout of lowest byte [ 0, 0, 0, 0, 0, SHIFT, CTRL, ALT ]
    XUINT32         m_modifierKeys {0}; // SHIFT is bit 2, CTRL is bit 1, ALT is bit 0

    MessageMap      m_msgID {XCP_NULL};

    // Some XCP_messageIDs map to multiple WM_messageIDs.
    // This flag differentiates WM_POINTERWHEEL from WM_POINTERHWHEEL (both mapped to XCP_POINTERWHEELCHANGED)
    // and WM_KEYDOWN from WM_SYSKEYDOWN (both mapped to XCP_KEYDOWN).
    // Used in CDirectManipulationService::GetWindowsMessageFromMessageMap.
    bool            m_bIsSecondaryMessage {false};

    // Xaml replays pointer input in response to the UI changing underneath a pointer that's not moving. This
    // keeps the hover state of the UI updated. These replayed pointer inputs shouldn't trigger things like
    // scroll bars fading in, because that would trigger more replays and create a cycle. This field differentiates
    // replayed inputs from real ones.
    bool            m_isReplayedMessage  {false};

    PhysicalKeyStatus m_physicalKeyStatus;
    PointerInfo m_pointerInfo;

    ixp::IPointerPoint* m_pPointerPointNoRef {nullptr};

    ixp::IPointerEventArgs* m_pPointerEventArgsNoRef {nullptr};

    bool IsReplayedMessage() const { return m_isReplayedMessage; }

    // For re-entrancy scenarios, this flag is set when XAML has processed a message later than this one.
    // Currently this is just to help debug.
    bool m_supersededByLaterMessage {false};
};

struct DragMsg
{
    MessageMap m_msgID;
    XINT32 m_xPos;
    XINT32 m_yPos;
    XUINT32 m_fileCount;
    XUINT32 m_filePathSize;
    XCHAR* m_filePaths;

#ifdef __cplusplus
    //Ctor
    DragMsg()
    {
        m_msgID = XCP_NULL;
        m_xPos = 0;
        m_yPos = 0;
        m_fileCount = 0;
        m_filePathSize = 0;
        m_filePaths = NULL;
    }

    // Destructor
    ~DragMsg()
    {
        m_xPos = 0;
        m_yPos = 0;
        m_fileCount = 0;

        if (m_filePaths)
        {
            delete [] m_filePaths;
            m_filePaths = NULL;
            m_filePathSize = 0;
        }
    }
#endif
};

#ifdef __cplusplus
struct TouchInteractionMsg
{
    XHANDLE                 m_hWnd = nullptr;
    MessageMap              m_msgID = XCP_NULL;
    XPointerInputType       m_pointerInputType = XcpPointerInputTypePointer;
    XPOINTF                 m_pointInteraction = {};
    XPOINTF                 m_pointPivot = {};
    bool m_bInertial = false;
    bool m_bPivotEnabled = false;
    ManipulationTransform   m_delta;
    ManipulationTransform   m_cumulative;
    ManipulationVelocity    m_velocity;
    XUINT32                 m_holdingState = 0;
    XFLOAT                  m_fInertiaTranslationDeceleration = 0;
    XFLOAT                  m_fInertiaTranslationDisplacement = 0;
    XFLOAT                  m_fInertiaRotationDeceleration = 0;     // degrees
    XFLOAT                  m_fInertiaRotationAngle = 0;            // degrees
    XFLOAT                  m_fInertiaExpansionDeceleration = 0;
    XFLOAT                  m_fInertiaExpansionExpansion = 0;
};
#endif

enum PathPointType
{
    PathPointTypeStart           = 0,       // move
    PathPointTypeLine            = 1,       // line
    PathPointTypeQuadratic       = 2,       // quadratic curve
    PathPointTypeBezier          = 3,       // cubic Bezier
    PathPointTypeCurve           = 0x02,    // Both a mask and a count
    PathPointTypePathTypeMask    = 0x07,    // type mask (lowest 3 bits).
    PathPointTypeEmptyFill       = 0x10,   // only stroke the point, no fill
    PathPointTypeStroked         = 0x40,    // Is this segment stroked?
    PathPointTypeCloseSubpath    = 0x80,    // closed flag
};


//---------------------------------------------------------------------------
//
//  TimeRange struct for Media Service
//
//  It contains Start position and End position in second.
//
//---------------------------------------------------------------------------

struct XTIMERANGE
{
    // Start position of the time range.
    XFLOAT  Start;

    // End position of the time range.
    XFLOAT  End;
};
#endif // #ifndef __PAL__TYPES__
