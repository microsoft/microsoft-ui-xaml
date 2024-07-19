// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

enum class KnownNamespaceIndex : UINT16;
enum class KnownTypeIndex : UINT16;
enum class KnownPropertyIndex : UINT16;
class CREATEPARAMETERS;
class CDependencyObject;
class CDependencyProperty;
class CClassInfo;
class CXMLObject;
struct IObject;
class CResourceDictionary;
struct IPALAsyncImageFactory;
class HWTextureManager;
enum DisplayRotation : uint8_t;
class DCompTreeHost;
class CEventArgs;
struct IPALDownloadRequest;
struct IPALTickableClock;
struct IPALSurface;
struct IPALGraphicsDeviceChangeListener;
struct IErrorService;
struct IPALResourceManager;
struct InputMessage;
struct DragMsg;
struct TouchInteractionMsg;
struct IPALWorkItemFactory;
struct IXcpDispatcher;
struct IPALExecuteOnUIThread;
class CMILMatrix;
struct IPlatformServices;

namespace Parser
{
    struct XamlBuffer;
}

#include "xstring_ptr.h"
#include "xstringmap.h"
#include "core_media.h"
#include "Events.h"
#include "CValue.h"
#include <xcpdebug.h>

enum DebugAllocationClass;

//------------------------------------------------------------------------
//
//  Interface:  IScriptObject
//
//  Synopsis:
//      Used to handle Objects handed to us from script
//
//------------------------------------------------------------------------
struct IScriptObject : public IObject
{
    virtual void *GetObject() = 0;
    virtual void Reset() = 0;
};


#include "CDependencyObject.h"
#include "IParserCoreServices.h"

//
// Instantiations of this type compile successfully if T is derived from B.
//
template<class T, class B>
struct DerivedFromConstraint
{
    static void Constraints(T *p) { B *pb = NULL; pb = p;}
    DerivedFromConstraint() { void(*p)(T*) = NULL; p = Constraints;}
};

//
// Generic dependency object factories. These functions validate that the output type is
// in fact a CDependencyObject-derived type.
//
// CreateDO uses type deduction to determine which type T to call T::Create on. Example usage:
//      CTimeline *pTimeline = NULL;
//      CTimeline *pTimelineWithCp = NULL;
//      CREATEPARAMETERS cp(...);
//
//      IFC(CreateDO(&pTimeline));
//      IFC(CreateDO(&pTimelineWithCp, &cp));
//
// CreateDO2 takes the factory type explicitly, and can deduce the output type or take
// it explicitly. Example usage:
//      CBrush *pBrush = NULL;
//      CREATEPARAMETERS cp(...);
//
//      IFC(CreateDO2<CSolidColorBrush>(&pBrush, &cp)); // CBrush abstract. Must explicitly specify factory.
//
template<class T> XCP_FORCEINLINE
_Check_return_ HRESULT CreateDO(_Outptr_ T** ppObject)
{
    DerivedFromConstraint<T, CDependencyObject>();
    return T::Create(reinterpret_cast<CDependencyObject**>(ppObject));
}

template<class T> XCP_FORCEINLINE
_Check_return_ HRESULT CreateDO(_Outptr_ T** ppObject, _In_ CREATEPARAMETERS *pCreate)
{
    DerivedFromConstraint<T, CDependencyObject>();
    return T::Create(reinterpret_cast<CDependencyObject**>(ppObject), pCreate);
}

template<class FactoryType, class T>
_Check_return_ HRESULT CreateDO2(_Outptr_ T** ppObject)
{
    DerivedFromConstraint<T, CDependencyObject>();
    DerivedFromConstraint<FactoryType, T>();
    return FactoryType::Create(reinterpret_cast<CDependencyObject**>(ppObject));
}

template<class FactoryType, class T>
_Check_return_ HRESULT CreateDO2(_Outptr_ T** ppObject, _In_ CREATEPARAMETERS *pCreate)
{
    DerivedFromConstraint<T, CDependencyObject>();
    DerivedFromConstraint<FactoryType, T>();
    return FactoryType::Create(reinterpret_cast<CDependencyObject**>(ppObject), pCreate);
}

//------------------------------------------------------------------------
//
//  Defines for AutomationPeer
//
//  Synopsis:
//      The set of enums and interfaces defined for UIAutomation compat
//
//------------------------------------------------------------------------
#include "UIAEnums.h"
#include "UIAStructs.h"

//------------------------------------------------------------------------
//
//  Interface:  ICoreServicesSite
//
//  Synopsis:
//      Site interface for ICoreServicesSite
//
//------------------------------------------------------------------------

struct ICoreServicesSite
{
    // This member is used for cases where there are legitimate exceptions
    // to site-of-origin download restriction.  If you make a call to this
    // member, comment the call as to why the exception is valid and secure.
    virtual _Check_return_ HRESULT UnsecureDownload(
        _In_ IPALDownloadRequest *pDownloadRequest,
        _Outptr_opt_ IPALAbortableOperation **ppIAbortableDownload,
        _In_opt_ IPALUri *pPreferredBaseUri = NULL) = 0 ;


    virtual HRESULT _Check_return_ CheckUri(
        _In_ const xstring_ptr& strRelativeUri,
        _In_ XUINT32 eUnsecureDownloadAction,
        _Inout_ XINT32 *pfShouldSuppressCookies = NULL) = 0;
};

enum RequestFrameReason : UINT32;

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Interface to a specific compositor instance in the compositor scheduler that allows for each
//      to be scheduled independently.
//
//------------------------------------------------------------------------------
struct IFrameScheduler : IObject
{
    virtual _Check_return_ HRESULT RequestAdditionalFrame(
        XUINT32 nextTickIntervalInMilliseconds,
        RequestFrameReason reason) = 0;

    virtual const bool HasRequestedAdditionalFrame() = 0;
};

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Interface to an off-thread compositor instance in the compositor scheduler that allows for
//      ticking to occur on a separate thread.
//
//------------------------------------------------------------------------------
struct ITickableFrameScheduler : IFrameScheduler
{
    virtual _Check_return_ HRESULT BeginTick() = 0;
    virtual _Check_return_ HRESULT EndTick() = 0;
    virtual bool IsInTick() = 0;
    virtual bool IsHighPriority() = 0;
    virtual IPALTickableClock* GetClock() = 0;  // TODO: HWPC: Can we factor TimeManager's clock dependency into the UIThreadFrameScheduler completely?
};

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Interface to an instance of a core services object.
//
//------------------------------------------------------------------------------

// Flag to use when posting events to keep firing event even for the handled event
#define EVENT_HANDLEDEVENTSTOO              0x00000008

#define EVENT_SENDER_PEGGED                 0x00000020

#define EVENT_SYNC_INPUT                    0x00000080
#define EVENT_SYNC_LOADED                   0x00000100

#define EVENT_ASYNC_STATIC                  0x00000400

// These APIs are exposed by the core
_Check_return_ HRESULT __stdcall ObtainCoreServices(_In_ IPlatformServices *pPlatform, _Outptr_ CCoreServices **ppCore);

//-----------------------------------------------------------------------------
//
// Event listener types
//
// Synopsis:
//
// Not all event listeners are treated the same, treatment is differentiated
//  according to how they are attached.  These values identify the attachment
//  method.
//
// Implementation note: It would make sense to have this as an enum, but we
//  have compiler problems that makes it nasty.  See what had to be done for
//  MouseCursor in uielement.h, and imagine that spread through every *.h file
//  for every type derived from DependencyObject.
//
//-----------------------------------------------------------------------------

// C# (representing all CLR languages):
//
// myCanvas.MouseEnter += new EventHandler(canvasEnter);
//
// Special treatment:
//  * Do not validate names, they have been deliberately mangled to avoid name collision with valid names.
//  * Assign an identifier token of -1.
#define EVENTLISTENER_CLR    3

// Internal listener (upon event, call method on a native class in our codebase.)
//
// Special treatment:
//  * No name
//  * Every internal listener will have the same identifier token.
//  * Callback is a INTERNAL_EVENT_HANDLER pointer
#define EVENTLISTENER_INTERNAL 4 // Note this is also defined in VisualStateMachineActuator.cpp, don't change the value willy-nilly.


//-----------------------------------------------------------------------------
// The following APIs are exposed by core in debug builds and made available
// to all Jolt code through the PAL.

#if XCP_MONITOR

    //------------------------------------------------------------------------
    //
    //  Method:   XcpCheckLeaks
    //
    //  Synopsis:
    //
    //      Walks the allocated heap listing every heap node still
    //      allocated including where it was allocated from.
    //
    //------------------------------------------------------------------------

    HRESULT XcpCheckLeaks(unsigned int leakThreshold = UINT_MAX);




    //------------------------------------------------------------------------
    //
    //  Method:   XcpDebugAllocate
    //
    //  Synopsis:
    //
    //      Memory allocator for new / malloc, that adds check blocks before
    //  and after the allocated memory, and sends a message to the monitoring
    //  process recording the allocation and where in the source it came from.
    //
    //------------------------------------------------------------------------

    _Check_return_ __declspec(allocator) void *XcpDebugAllocate(
        size_t               cSize,
        DebugAllocationClass allocationClass
    );




    //------------------------------------------------------------------------
    //
    //  Method:   XcpDebugFree
    //
    //  Synopsis:
    //      Memory deallocator for delete/free, that tests check blocks before
    //  and after the allocated memory, and sends a message to the monitoring
    //  process recording the allocation and where in the source it came from.
    //
    //------------------------------------------------------------------------

    void XcpDebugFree(
        _In_ void            *pAddress,
        DebugAllocationClass  allocationClass
    );

    //------------------------------------------------------------------------
    //
    //  Method:   XcpDebugResize
    //
    //  Synopsis:
    //
    //      Memory allocator for new / malloc, that adds check blocks before
    //  and after the allocated memory, and sends a message to the monitoring
    //  process recording the allocation and where in the source it came from.
    //
    //------------------------------------------------------------------------

    _Check_return_ __declspec(allocator) void *XcpDebugResize(
        _In_ void            *pAddress,
        size_t                cSize,
        DebugAllocationClass  allocationClass
    );

    //------------------------------------------------------------------------
    //
    //  Method:   XcpTrackAddRef/Release
    //
    //  Synopsis:
    //      Keep a list of all the places where we addRef and release a
    //      specific object
    //
    //------------------------------------------------------------------------
    void XcpTrackAddRefRelease(
        _In_ void *pAddress,
        XUINT32  cRef
    );

    //------------------------------------------------------------------------
    //
    //  Method:   XcpIgnoreAllOutstandingAlloctions
    //
    //  Synopsis:
    //
    //      Should be called at beginning of a test run
    //
    //      Walks the allocated heap and sets the IgnoreLeakDetectionFlag
    //
    //------------------------------------------------------------------------
    void XcpIgnoreAllOutstandingAllocations();

    //------------------------------------------------------------------------
    //
    //  Method:   XcpVTrace
    //
    //  Synopsis:
    //      Trace message handler. All trace messages go through this API.
    //
    //------------------------------------------------------------------------

    bool XcpVTrace(
        XUINT32  uClass,          // Class of monitoring event, a.g. assertion, trace
        const WCHAR   *pFilename,
        XINT32   iLine,
        XINT32   iValue,          // E.g. assertion value, HRESULT
        const WCHAR    *pTestString,    // E.g. Assertion string
        const WCHAR    *pMessageString, // Message and args as a string - tells us whether to look for var args
        void     *pVArgs          // Var args
    );


    void XcpEnterSection(XUINT16 id);
    void XcpLeaveSection(XUINT16 id);
    void XcpPopToMark();

#if USE_VIRTUAL_ALLOC
    HRESULT XcpTraceMonitorInitialize(XUINT8 bIsUseVirtualOverrideSet);
#else
    HRESULT XcpTraceMonitorInitialize();
#endif
    void    XcpTraceMonitorShutdown();

#endif // #if XCP_MONITOR
