// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Contains the types and methods provided by the platform abstraction layer
//  Code in the core rasterizer can only access the operating system thru these
//  helping to make the core truly portable.

#pragma once

#include "Indexes.g.h"
#include "InternalEventHandler.h"
#include "palcore.h"

class CCoreServices;
class WindowsGraphicsDeviceManager;

// expiration date defines...
#ifndef STATIC_STRING_CCH
#define STATIC_STRING_CCH(arg)          (ARRAY_SIZE(arg) - 1)   // do not include NULL
#endif

#define CORE_PRINTING_MAX_DPI        600
#define CORE_PRINTING_SCREEN_DPI     96
#define CORE_PRINTING_POSTSCRIPT_DPI 72
#define CORE_PRINTING_MAX_PAGES      2000
#define CORE_PRINTING_MAX_PAGE_D1    7016    // A3 1st Dimension (Width or Height)
#define CORE_PRINTING_MAX_PAGE_D2    9921    // A3 2nd Dimension (Width or Height)

// REVIEW: Reuse CORECLRAPI definition?
#define CLR_API __stdcall

#ifdef __cplusplus

// these are defined in host.h
// TODO - This needs to move into hosting
struct IXcpHostSite;
struct IXcpBrowserHost;
struct IXcpDispatcher;

// TODO - This needs to move into hosting
struct MsgPacket;

//Forward declare for mediacore services
struct IMediaServices;

// TODO - This needs to move into Core!
//------------------------------------------------------------------------
//
//  Interface:  IValueStore
//
//  Synopsis:
//      Allows you to associate a value with a name.  So for instance you can
//  say the name "LightGoldenrodYellow" has a value of XHANDLE(0xfffafad2)
//
//------------------------------------------------------------------------

// TODO - This needs to move into Core!
typedef  void (*ValueStoreCleanupCallback)(XHANDLE hValue, XHANDLE hOwner);

// TODO - This needs to move into Core!
struct IValueStore
{
// Life time management

    virtual XUINT32 Release() = 0;

// Methods

    virtual _Check_return_ HRESULT PutValue(_In_ const xstring_ptr_view& strName, _In_ XHANDLE hValue) = 0;
    virtual _Check_return_ HRESULT GetValue(_In_ const xstring_ptr_view& strName, _Out_ XHANDLE *phValue) = 0;
    virtual _Check_return_ HRESULT SetCleanupNotifier(_In_ ValueStoreCleanupCallback pCallBack) = 0;
};

//------------------------------------------------------------------------
//
//  Interface:  IPlatformServices
//
//  Synopsis:
//      Provides an abstract way for the render core to access system services
//  such as files, memory, drawing surfaces, etc.  This also provides services
//  for the plug-ins so they can be written in a platform independent way.  See
//  the documentation on the project team web site for a description of each
//  service.
//
//  IPlatformServices provides allocation support, but this is not used for the
//  allocation of IPlatformServices itself.
//
//------------------------------------------------------------------------

struct IPlatformServices :
    // Core PAL
    public IPALDebuggingServices,
    public IPALMemoryServices,
    public IPALURIServices,
    public IPALMathServices,
    public IPALPrintIOServices,
    public IPALThreadingServices,
    public IPALTextServices,
    public IPALCoreServices,
    public IPALTouchInteractionServices,
    public IPALAutomationServices,
// Non-Core PAL
    public IPALThemingServices,
    public IPALPrintingServices,
    public IPALProcessCharacteristics
{
public:
// OS System Settings

    virtual _Check_return_ HRESULT CreateGraphicsDeviceManager(_Outptr_ WindowsGraphicsDeviceManager **ppIGraphicsDeviceManager) = 0;

// BrowserHost services
// TODO - Move into Hosting layer!!!
    virtual _Check_return_ HRESULT BrowserHostCreate(_In_ IXcpHostSite *pSite, _In_ IXcpDispatcher *pDispatcher, _Out_ IXcpBrowserHost ** ppHost) = 0;

    virtual _Check_return_ HRESULT CreateResourceProvider(_In_ CCoreServices *pCore, _Outptr_ IPALResourceProvider **ppResourceProvider) = 0;

    virtual _Check_return_ HRESULT CreateApplicationDataProvider(_Outptr_ IPALApplicationDataProvider **ppAppDataProvider) = 0;

    virtual _Check_return_ HRESULT CreateWorkItemFactory(
        _Outptr_ IPALWorkItemFactory **ppWork) = 0;

};

#include "encodedptr.h"
// Global extern for platform services
//

extern EncodedPtr<IPlatformServices> gps;


#if DBG
_Check_return_
    HRESULT
    ObtainPlatformServices(_Outptr_ IPlatformServices **ppInterface, XUINT8 testMode = FALSE);

#else
_Check_return_
    HRESULT
    ObtainPlatformServices(_Outptr_ IPlatformServices **ppInterface);

#endif // #if DBG
typedef _Check_return_ HRESULT (*ObtainPlatformServicesFunc)(IPlatformServices **ppInterface);

#define OBTAINPLATFORMSERVICES_STRING "ObtainPlatformServices"
#endif //__cplusplus
#include "xcp_error.h"

//------------------------------------------------------------------------
//
//  Interface: IPALWindowLessHost
//
//  Synopsis:
//      Interface for hosting a WindowLess Control. This is to provide the abstraction layer between calls with Windows related APIs
//      and UICore which doesn't understands win types. For bridging of UIA and RichEdit the communication is needed at UI Core
//      level with UIAutomation types and APIs extensively. this abstraction layer allows us to achieve that.
//
//------------------------------------------------------------------------
struct IPALWindowlessHost
{
    virtual _Check_return_ HRESULT Initialize(_In_ IXcpHostSite* pHostSite, _In_ CDependencyObject* pParentEditBox, _In_ XUINT32 uRuntimeId) = 0;
    virtual void Destroy(_In_ void* pRichEditWindowlessAcc) = 0;
    virtual _Check_return_ HRESULT GetChildRawElementProviderSimple(
        _In_ void* pRichEditWindowLessAcc,
        _Outptr_ void** pProvider) = 0;
    virtual _Check_return_ HRESULT GetUnwrappedPattern(
        _In_ void* pRichEditWindowLessAcc,
        _In_ XINT32 patternID,
        _In_ bool isRichEdit,
        _Outptr_ void** ppPattern) = 0;
};
