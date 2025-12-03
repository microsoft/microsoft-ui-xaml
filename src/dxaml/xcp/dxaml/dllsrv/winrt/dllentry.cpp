// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "DCompSurfaceMonitor.h"
#include "DCompSurfaceFactoryManager.h"
#include "FrameworkApplication.g.h"
#include "BackgroundTaskFrameworkContext.h"
#include <MetadataAPI.h>
#include <DependencyLocator.h>
#include <CStaticLock.h>
#include "ThreadPoolService.h"
#include <TraceLoggingInterop.h>
#include <RuntimeEnabledFeatures.h>
#include <XamlTraceLogging.h>
#include "XamlTelemetry.h"
#include "LoadLibraryAbs.h"

#include <WinUIrc.ver>           //  To define VER_FILEVERSION_STR

#pragma warning (suppress : 6387) // param 1 could be null warning from inside wil\Telemetry.
DECLARE_TRACELOGGING_CLASS(XamlTraceLogging, "Microsoft-Windows-XAML", (0x531A35AB, 0x63CE, 0x4BCF, 0xAA, 0x98, 0xF8, 0x8C, 0x7A, 0x89, 0xE4, 0x55));

class XamlFallbackLogging final : public TelemetryBase
{
    #pragma warning (suppress : 6387) // param 1 could be null warning from inside wil\Telemetry.
    IMPLEMENT_TELEMETRY_CLASS(XamlFallbackLogging, XamlTraceLogging);
};

//  from RuntimeProfiler.cpp
STDAPI_(void) SendTelemetryOnSuspend();

using namespace DirectUI;

#define DLL_PROCESS_ATTACH 1

#if !DEPENDENCY_VERIFICATION_BUILD
extern "C"
void __cdecl __security_init_cookie(void);

#endif

typedef HRESULT (WINAPI *COINTERNETSETFCKPROCESSNAMEFUNC)(
   _In_ PCWSTR pszProcessName);

const DWORD TLS_UNINITIALIZED = -1;

HINSTANCE g_hInstance = NULL;
HMODULE g_platformResourcesModuleHandle = nullptr;
DWORD g_dwTlsIndex = TLS_UNINITIALIZED;
DLL_DIRECTORY_COOKIE muxDllDirectoryCookie = nullptr;

// A flag to indicate if the CCoreServices are ready for action
// Used to prevent some destructor activities from occurring after the core has already been destroyed.
extern XUINT32 g_nTlsIsCoreServicesReady;

// version of binary, used in telemetry payload so that we may distinguish between versions.
const char *gFileVersion = (const char *)(VER_FILEVERSION_STR);

// Sets g_platformResourcesModuleHandle if needed
void
EnsurePlatformResourceModuleHandle()
{
    // Initial check to avoid taking the lock when we'e already clearly initialized the resources handle
    if (g_platformResourcesModuleHandle != nullptr)
    {
        return;
    }

    // Acquire the lock to safely initialize the resources handle
    static SRWLOCK s_resourceModuleInitLock {SRWLOCK_INIT };
    auto guard = wil::AcquireSRWLockExclusive(&s_resourceModuleInitLock);

    // After taking the lock, we need to recheck the resources handle isn't created yet in case the last lock holder
    // was the one who created it and the initial lockless check missed it
    if (g_platformResourcesModuleHandle != nullptr)
    {
        return;
    }

    g_platformResourcesModuleHandle =
        LoadLibraryExWAbs(
            L"Microsoft.UI.Xaml.Resources.19H1.dll",
            NULL,
            LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_IMAGE_RESOURCE);
    if (g_platformResourcesModuleHandle == nullptr)
    {
        // If you hit this crash, we weren't able to find the above resources DLL.  Make sure it's included
        // alongside Microsoft.UI.Xaml.dll, it's required for XAML to run.
        IFCFAILFAST(E_UNEXPECTED);
    }
}

BOOL InitializeDll()
{
    HRESULT hr = S_OK;
    IPlatformServices* pPal = NULL;

    // Static data initialization. Initialize first as the shutdown part (DeinitializeDll) relies on them.
    IFC(DirectUI::StaticLockGlobalInit());

    // Initialize this among the first as lots of other components can rely on this being initialized.
    DependencyLocator::InitializeProcess();

    // Do this second so we can capture any errors that occur during the rest of DLL initialization.
    IFC(ErrorContextGlobalInit(TLS_UNINITIALIZED));

    IFC(WarningContextGlobalInit(TLS_UNINITIALIZED));

    // Register ETW Tracing
    EventRegisterMicrosoft_Windows_XAML();
    EventRegisterMicrosoft_Windows_XAML_Diagnostics();

    // Register trace logging provider
    TraceLoggingRegister(g_hTraceProvider);

    // Override the debugger output string formatting method that WIL uses to log errors.  The default formatting
    // is more verbose than desired compared to the previous XAML debug output.  This bridges the difference.
    wil::SetResultMessageCallback(WILResultMessageCallback);

    // Initialize telemetry fallback provider as early as possible so all IFC-type macros will log errors
    wil::SetResultTelemetryFallback(&XamlFallbackLogging::FallbackTelemetryCallback);

    IFC(ObtainPlatformServices(&pPal));
    gps.Set(pPal);

    g_nTlsIsCoreServicesReady = TlsAlloc();
    IFCEXPECT(TLS_OUT_OF_INDEXES != g_nTlsIsCoreServicesReady);

    IFC(DXamlInstanceStorage::Initialize());

    IFC(FrameworkApplication::GlobalInit());

    IFC(DCompSurfaceMonitor::Initialize());

    DCompSurfaceFactoryManager::EnsureInitialized();

    IFC(BackgroundTaskFrameworkContext::GlobalInit());

    // We can sometimes be in the situation in which the executable that has MUX DLLs loaded
    // is not in the same folder as the MUX DLLs themselves - e.g., in the case of a XAML islands
    // application, the MUX DLLs can sometimes be binplaced to a subdirectory, while the executable
    // loading them is binplaced to a separate subdirectory.  To account for this circumstance,
    // we'll add the directory path for Microsoft.UI.Xaml.dll to our DLL search path.
    WCHAR muxPath[MAX_PATH];

    HMODULE muxModule;
    if (!GetModuleHandleEx(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            (LPWSTR) &InitializeDll, &muxModule))
    {
        IFC(HRESULT_FROM_WIN32(GetLastError()));
    }

    GetModuleFileName(GetModuleHandle(L"Microsoft.UI.Xaml.dll"), muxPath, MAX_PATH);
    FAIL_FAST_ASSERT(PathRemoveFileSpec(muxPath) != 0);
    muxDllDirectoryCookie = AddDllDirectory(muxPath);

Cleanup:
    return SUCCEEDED(hr) ? TRUE : FALSE;
}

void DeinitializeDll()
{
    if (muxDllDirectoryCookie != nullptr)
    {
        RemoveDllDirectory(muxDllDirectoryCookie);
        muxDllDirectoryCookie = nullptr;
    }

    if (TLS_OUT_OF_INDEXES != g_nTlsIsCoreServicesReady)
    {
        TlsFree(g_nTlsIsCoreServicesReady);
        g_nTlsIsCoreServicesReady = TLS_OUT_OF_INDEXES;
    }

    // Ideally the metadata has already been cleaned up by now. We called MetadataAPI::Destroy() when the
    // last (main) FrameworkView went away (and called FrameworkApplication::ReleaseCurrent()).
    // The only exception right now is our Standalone test runner (ttds.exe). Instead of updating that code,
    // we're leaving this call here to preserve the existing
    // cleanup behavior as a fallback option. If anyone ever runs into a crash because we're cleaning up
    // metadata here that ends up trying to release a reference to an object whose code pages are no longer
    // there, they will need to make sure there's a MetadataAPI::Reset() call that happened sooner (*before*
    // modules are being unloaded).
    MetadataAPI::Destroy();

    ThreadPoolService::DetachFactories();

    BackgroundTaskFrameworkContext::GlobalDeinit();

    IGNORERESULT(FrameworkApplication::GlobalDeinit());

    IGNORERESULT(DXamlInstanceStorage::Deinitialize());

    // Fire any outstanding telemetry before unregistering the provider
    SendTelemetryOnSuspend();

    // Clear out the tracelogging telemetry fallback after the final calls that might possibly log.
    wil::SetResultTelemetryFallback(nullptr);

    // Clear out the custom formatter for WIL error messages
    wil::SetResultMessageCallback(nullptr);

    // Unregister trace logging provider
    TraceLoggingUnregister(g_hTraceProvider);

    IGNORERESULT(DCompSurfaceMonitor::DeInitialize());

    ctl::__module.ClearFactoryCache();

    DCompSurfaceFactoryManager::Deinitialize();

    // Unregister ETW Tracing
    EventUnregisterMicrosoft_Windows_XAML();
    EventUnregisterMicrosoft_Windows_XAML_Diagnostics();

    // Do this after checking for leaks because we can log leaks that require our ErrorContext to be initialized.
    // Do it before DependencyLocator::UninitializeProcess because our global ErrorContext has a dependency on the
    // DependencyLocator being initialized
    IGNORERESULT(ErrorContextGlobalDeinit());

    IGNORERESULT(WarningContextGlobalDeinit());

    // Uninitializing the dependency locator needs to be the absolute last thing that we do*.
    // Any of the above calls could rely on a Dependency and since we never get this far in
    // a normal modern app process, uninitializing it any earlier is a bug farm waiting to happen
    // in the few processes that use other hosting APIs like the designer.
    DependencyLocator::UninitializeProcess();

    // *we'll deinit the lock last since this **should** never have a dependency on anything
    // DependencyLocator related. It's more likely a Depenency will try to take the lock, so we
    // don't want to deinit the lock beforehand.
    DirectUI::StaticLockGlobalDeinit();
}

extern "C"
BOOL WINAPI
DllMain(
    _In_ HINSTANCE hinstDLL,
    _In_ unsigned int fdwReason,
    _In_opt_ void *
)
{
    BOOL fRetVal = TRUE;
    // Perform actions based on the reason for calling.
    switch( fdwReason )
    {
        case DLL_PROCESS_ATTACH:
            // Initialize once for each new process.
            // Return FALSE to fail DLL load.

            g_hInstance = hinstDLL;
            fRetVal = InitializeDll();

#if !DEPENDENCY_VERIFICATION_BUILD
            //
            // The /GS security cookie must be initialized before any exception
            // handling targetting the current image is registered.  No function
            // using exception handling can be called in the current image until
            // after __security_init_cookie has been called.
            //

            __security_init_cookie();
#endif
            break;

        case DLL_PROCESS_DETACH:
            DeinitializeDll();
            break;

        case DLL_THREAD_DETACH:
            IGNOREHR(ErrorContextThreadDeinit());
            IGNOREHR(WarningContextThreadDeinit());
            break;
    }

    return fRetVal;
}

extern "C" HRESULT WINAPI DllGetActivationFactory(_In_ HSTRING hstrAcid, _Outptr_ IActivationFactory** factory)
{
    *factory = ctl::__module.GetActivationFactory(hstrAcid);
    if (*factory == NULL)
    {
        // To avoid printing superficial error stack traces in the test infrastructure we don't trace this common, expected failure
        IFC_NOTRACE_RETURN(CLASS_E_CLASSNOTAVAILABLE);
    }

    return S_OK;
}

__control_entrypoint(DllExport)
STDAPI
DllCanUnloadNow()
{
    return S_FALSE;
}

extern "C" void WINAPI OverrideXamlMetadataProvider(_In_opt_ xaml_markup::IXamlMetadataProvider* pMetadataProvider)
{
    MetadataAPI::OverrideMetadataProvider(pMetadataProvider);
}

extern "C" void WINAPI OverrideXamlResourcePropertyBag(_In_opt_ std::map<std::wstring, std::vector<std::pair<std::wstring, std::wstring>>>* propertyBag)
{
    CCoreServices* core = DXamlServices::GetHandle();
    core->OverrideResourcePropertyBag(propertyBag);
}

extern "C" DWORD WINAPI GetErrorContextIndex()
{
    DWORD errorContextIndex = GetErrorContextTlsIndex();

    // If we haven't yet initialized an error context index for this DLL, let's do so now.
    if (errorContextIndex == TLS_UNINITIALIZED)
    {
        VERIFYHR(ErrorContextGlobalInit(TLS_UNINITIALIZED));
        errorContextIndex = GetErrorContextTlsIndex();
    }

    return errorContextIndex;
}

extern "C" DWORD WINAPI GetWarningContextIndex()
{
    DWORD warningContextIndex = GetWarningContextTlsIndex();

    // If we haven't yet initialized a warning context index for this DLL, let's do so now.
    if (warningContextIndex == TLS_UNINITIALIZED)
    {
        VERIFYHR(WarningContextGlobalInit(TLS_UNINITIALIZED));
        warningContextIndex = GetWarningContextTlsIndex();
    }

    return warningContextIndex;
}

extern XHANDLE ghHeap;
extern "C" XHANDLE WINAPI GetHeapHandle()
{
    // This allows extension DLLs to use the same heap as Xaml itself.  Typically, this is just the
    // process heap, but in the cases where they aren't we want them to be using the same heap.
    return ghHeap;
}

extern "C" void WINAPI XamlCheckProcessRequirements()
{
    //
    // This function used to contain a check that would block processes if they were launched as elevated. It no longer
    // does anything, but we can't remove it because it was an exported method that some app could still be calling.
    // Just no-op.
    //
    // Also log an ETW event because apps might be taking a perf hit if the manually load/unload MUX.dll just to call
    // here. Loading MUX.dll is expensive because it depends on dxgi.dll, which has an expensive dllmain_dispatch
    // initialization function. Apps can look for this event and delete the call to this function.
    //

    TraceLoggingProviderWrite(
        XamlTelemetry, "XamlCheckProcessRequirements_Obsolete",
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

    return;
}
