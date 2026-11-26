// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ActivationABITests.h"
#include <XamlTailored.h>
#include <TestCleanupWrapper.h>
#include <wrl/wrappers/corewrappers.h>
#include <wil/result_macros.h>

#undef GetCurrentTime

#include <asyncinfo.h>
#include <Microsoft.UI.Xaml.h>
#include <windows.applicationmodel.activation.h>
#include <windows.applicationmodel.core.h>

#include <array>
#include <mutex>
#include <roerrorapi.h>
#include <thread>
#include <tuple>

using namespace Platform;
using namespace ::Windows::Foundation;
using namespace ABI::Microsoft::UI::Xaml;
using namespace ABI::Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;
using namespace WEX::Logging;
using namespace WEX::Common;
using namespace Microsoft;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework { namespace Activation {

bool ActivationABITests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

bool ActivationABITests::TestSetup()
{
    TestServices::WindowHelper->InitializeXaml();
    return true;
}

bool ActivationABITests::TestCleanup()
{
    TestServices::WindowHelper->ShutdownXaml();
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

void ActivationABITests::PointerReturnedForStyleSelectorIsCorrect()
{
    RunOnUIThread([&]()
    {
        typedef HRESULT(WINAPI *FpnDllGetActivationFactory)(HSTRING, IActivationFactory**);

        HMODULE hModXAML;
        VERIFY_IS_TRUE(GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, L"Microsoft.UI.Xaml.dll", &hModXAML) == TRUE, L"Unable to get the handle for Microsoft.UI.Xaml.dll.");

        FpnDllGetActivationFactory pfnGetActivationFactory = (FpnDllGetActivationFactory)GetProcAddress(hModXAML, "DllGetActivationFactory");
        VERIFY_IS_NOT_NULL(pfnGetActivationFactory);

        Microsoft::WRL::ComPtr<IActivationFactory> spActivationFactory;
        VERIFY_SUCCEEDED(pfnGetActivationFactory(WRL::Wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Controls_StyleSelector).Get(), &spActivationFactory));

        Microsoft::WRL::ComPtr<IStyleSelectorFactory> spStyleSelectorFactory;
        VERIFY_SUCCEEDED(spActivationFactory->QueryInterface(IID_PPV_ARGS(&spStyleSelectorFactory)));

        Microsoft::WRL::ComPtr<IStyleSelector> spStyleSelector;
        VERIFY_SUCCEEDED(spStyleSelectorFactory->CreateInstance(nullptr /* pOuter */, nullptr /* ppInner */, &spStyleSelector));

        Microsoft::WRL::ComPtr<IStyleSelector> spStyleSelectorAsIStyleSelector;
        VERIFY_SUCCEEDED(spStyleSelector->QueryInterface(IID_PPV_ARGS(&spStyleSelectorAsIStyleSelector)));

        // We should have received back an IStyleSelector the first time, so the QI should not change the pointer.
        // If it did, it means we returned some other vtable from the CreateInstance call which is not expected.
        // Games, such as Supermarket Management 2 HD, have taken a dependency on us returning the right type (without
        // the need to QI us).
        VERIFY_ARE_EQUAL(spStyleSelector.Get(), spStyleSelectorAsIStyleSelector.Get());
    });
}

// Microsoft.UI.Xaml.FrameworkView implements Windows.ApplicationModel.Core.IFrameworkView. CoreApplication normally
// drives it with a CoreApplicationView; that only exists in a UWP process, so these tests play CoreApplication's role
// with test doubles for the view and the activation args, and drive the real FrameworkView from Microsoft.UI.Xaml.dll.
// Initialize and Activated run on the UI thread, which already has a DXamlCore and Application.Current. Uninitialize
// tears down the calling thread's XAML state, so it runs on a dedicated thread that never had any.
namespace
{
    namespace wamc = ::ABI::Windows::ApplicationModel::Core;
    namespace wama = ::ABI::Windows::ApplicationModel::Activation;
    namespace wfabi = ::ABI::Windows::Foundation;

    using ActivatedHandler = wfabi::ITypedEventHandler<wamc::CoreApplicationView*, wama::IActivatedEventArgs*>;

    // Distinct failure codes, so the tests can tell which collaborator call failed.
    const HRESULT c_isMainFailure = HRESULT_FROM_WIN32(ERROR_INVALID_STATE);
    const HRESULT c_addActivatedFailure = HRESULT_FROM_WIN32(ERROR_NOT_READY);
    const INT64 c_registrationToken = 0x0F7A3E5D12345678;

    template <typename T>
    ULONG GetReferenceCount(T* object)
    {
        object->AddRef();
        return object->Release();
    }

    // Plays a CoreApplicationView: reports IsMain (a secondary view unless SetIsMain is called) and owns the Activated event.
    class CoreApplicationViewDouble final
        : public ::Microsoft::WRL::RuntimeClass<wamc::ICoreApplicationView, ::Microsoft::WRL::FtmBase>
    {
        InspectableClass(nullptr /*internal*/, BaseTrust);

    public:
        CoreApplicationViewDouble(HRESULT isMainResult, HRESULT addActivatedResult, INT64 tokenValue)
            : m_isMainResult(isMainResult)
            , m_addActivatedResult(addActivatedResult)
            , m_tokenValue(tokenValue)
        {
        }

        IFACEMETHOD(get_CoreWindow)(ABI::Windows::UI::Core::ICoreWindow** value) override
        {
            *value = nullptr;
            return E_NOTIMPL;
        }

        IFACEMETHOD(add_Activated)(ActivatedHandler* handler, ::EventRegistrationToken* token) override
        {
            std::lock_guard<std::mutex> guard(m_lock);
            ++m_addActivatedCalls;
            if (FAILED(m_addActivatedResult))
            {
                // Like a failing event source, leave the token output untouched.
                return m_addActivatedResult;
            }

            m_handler = handler;
            token->value = m_tokenValue;
            return S_OK;
        }

        IFACEMETHOD(remove_Activated)(::EventRegistrationToken token) override
        {
            std::lock_guard<std::mutex> guard(m_lock);
            ++m_removeActivatedCalls;
            m_lastRemovedToken = token.value;
            if (token.value == m_tokenValue)
            {
                m_handler.Reset();
            }
            return S_OK;
        }

        IFACEMETHOD(get_IsMain)(boolean* value) override
        {
            std::lock_guard<std::mutex> guard(m_lock);
            ++m_isMainCalls;
            if (FAILED(m_isMainResult))
            {
                return m_isMainResult;
            }

            *value = m_isMain ? TRUE : FALSE;
            return S_OK;
        }

        IFACEMETHOD(get_IsHosted)(boolean* value) override
        {
            *value = FALSE;
            return S_OK;
        }

        int IsMainCalls() { std::lock_guard<std::mutex> guard(m_lock); return m_isMainCalls; }
        int AddActivatedCalls() { std::lock_guard<std::mutex> guard(m_lock); return m_addActivatedCalls; }
        int RemoveActivatedCalls() { std::lock_guard<std::mutex> guard(m_lock); return m_removeActivatedCalls; }
        INT64 LastRemovedToken() { std::lock_guard<std::mutex> guard(m_lock); return m_lastRemovedToken; }
        ::Microsoft::WRL::ComPtr<ActivatedHandler> GetHandler() { std::lock_guard<std::mutex> guard(m_lock); return m_handler; }

        void SetAddActivatedResult(HRESULT result) { std::lock_guard<std::mutex> guard(m_lock); m_addActivatedResult = result; }
        void SetIsMain(bool isMain) { std::lock_guard<std::mutex> guard(m_lock); m_isMain = isMain; }

        // The handler wraps a raw FrameworkView pointer, so drop it before the FrameworkView goes away.
        void DropHandler() { std::lock_guard<std::mutex> guard(m_lock); m_handler.Reset(); }

    private:
        std::mutex m_lock;
        HRESULT m_isMainResult;
        HRESULT m_addActivatedResult;
        INT64 m_tokenValue;
        bool m_isMain = false;
        int m_isMainCalls = 0;
        int m_addActivatedCalls = 0;
        int m_removeActivatedCalls = 0;
        INT64 m_lastRemovedToken = 0;
        ::Microsoft::WRL::ComPtr<ActivatedHandler> m_handler;
    };

    // The activation args interfaces FrameworkView can query for, in the order of its ActivationKind switch.
    enum class ArgsInterface
    {
        Generic,            // IActivatedEventArgs, queried by the generic dispatch
        Launch,
        Search,
        ShareTarget,
        File,
        FileOpenPicker,
        FileSavePicker,
        CachedFileUpdater,
        Count
    };

    const wchar_t* GetArgsInterfaceName(ArgsInterface value)
    {
        static const std::array<const wchar_t*, static_cast<size_t>(ArgsInterface::Count)> names =
        {
            L"IActivatedEventArgs",
            L"ILaunchActivatedEventArgs",
            L"ISearchActivatedEventArgs",
            L"IShareTargetActivatedEventArgs",
            L"IFileActivatedEventArgs",
            L"IFileOpenPickerActivatedEventArgs",
            L"IFileSavePickerActivatedEventArgs",
            L"ICachedFileUpdaterActivatedEventArgs",
        };
        return names[static_cast<size_t>(value)];
    }

    using ActivatedEventArgsDoubleBase = ::Microsoft::WRL::RuntimeClass<
        wama::IActivatedEventArgs,
        wama::ILaunchActivatedEventArgs,
        wama::ISearchActivatedEventArgs,
        wama::IShareTargetActivatedEventArgs,
        wama::IFileActivatedEventArgs,
        wama::IFileOpenPickerActivatedEventArgs,
        wama::IFileSavePickerActivatedEventArgs,
        wama::ICachedFileUpdaterActivatedEventArgs,
        ::Microsoft::WRL::FtmBase>;

    // How the args double answers a query for the typed args interface of its kind.
    enum class TypedArgsMode
    {
        Implemented,    // returns the interface
        Refused,        // E_NOINTERFACE, as args that don't support the interface their kind needs
        Null,           // S_OK with a null pointer, so the Application's own null-args check rejects the args
    };

    // Activation args that implement every typed args interface FrameworkView knows about, and record which of them
    // FrameworkView queries for, so a test can tell which branch handled a given ActivationKind.
    class ActivatedEventArgsDouble final : public ActivatedEventArgsDoubleBase
    {
        InspectableClass(nullptr /*internal*/, BaseTrust);

    public:
        explicit ActivatedEventArgsDouble(wama::ActivationKind kind, TypedArgsMode typedArgsMode = TypedArgsMode::Implemented)
            : m_kind(kind)
            , m_typedArgsMode(typedArgsMode)
        {
        }

        // WRL calls this from QueryInterface before its own interface lookup.
        HRESULT CustomQueryInterface(REFIID riid, void** ppvObject, bool* handled)
        {
            const ArgsInterface queried = RecordQuery(riid);
            *handled = false;
            if (m_typedArgsMode != TypedArgsMode::Implemented && queried != ArgsInterface::Count && queried != ArgsInterface::Generic)
            {
                *ppvObject = nullptr;
                *handled = true;
                return (m_typedArgsMode == TypedArgsMode::Refused) ? E_NOINTERFACE : S_OK;
            }
            return S_OK;
        }

        // IActivatedEventArgs
        IFACEMETHOD(get_Kind)(wama::ActivationKind* value) override
        {
            std::lock_guard<std::mutex> guard(m_lock);
            ++m_kindCalls;
            *value = m_kind;
            return S_OK;
        }

        IFACEMETHOD(get_PreviousExecutionState)(wama::ApplicationExecutionState* value) override
        {
            *value = wama::ApplicationExecutionState_NotRunning;
            return S_OK;
        }

        IFACEMETHOD(get_SplashScreen)(wama::ISplashScreen** value) override
        {
            *value = nullptr;
            return E_NOTIMPL;
        }

        // ILaunchActivatedEventArgs
        IFACEMETHOD(get_Arguments)(HSTRING* value) override
        {
            {
                std::lock_guard<std::mutex> guard(m_lock);
                ++m_argumentsCalls;
            }
            static const wchar_t c_arguments[] = L"/frameworkview-test";
            return ::WindowsCreateString(c_arguments, ARRAYSIZE(c_arguments) - 1, value);
        }

        IFACEMETHOD(get_TileId)(HSTRING* value) override
        {
            *value = nullptr;
            return E_NOTIMPL;
        }

        // ISearchActivatedEventArgs
        IFACEMETHOD(get_QueryText)(HSTRING* value) override
        {
            *value = nullptr;
            return E_NOTIMPL;
        }

        IFACEMETHOD(get_Language)(HSTRING* value) override
        {
            *value = nullptr;
            return E_NOTIMPL;
        }

        // IShareTargetActivatedEventArgs
        IFACEMETHOD(get_ShareOperation)(ABI::Windows::ApplicationModel::DataTransfer::ShareTarget::IShareOperation** value) override
        {
            *value = nullptr;
            return E_NOTIMPL;
        }

        // IFileActivatedEventArgs
        IFACEMETHOD(get_Files)(__FIVectorView_1_Windows__CStorage__CIStorageItem** value) override
        {
            *value = nullptr;
            return E_NOTIMPL;
        }

        IFACEMETHOD(get_Verb)(HSTRING* value) override
        {
            *value = nullptr;
            return E_NOTIMPL;
        }

        // IFileOpenPickerActivatedEventArgs
        IFACEMETHOD(get_FileOpenPickerUI)(ABI::Windows::Storage::Pickers::Provider::IFileOpenPickerUI** value) override
        {
            *value = nullptr;
            return E_NOTIMPL;
        }

        // IFileSavePickerActivatedEventArgs
        IFACEMETHOD(get_FileSavePickerUI)(ABI::Windows::Storage::Pickers::Provider::IFileSavePickerUI** value) override
        {
            *value = nullptr;
            return E_NOTIMPL;
        }

        // ICachedFileUpdaterActivatedEventArgs
        IFACEMETHOD(get_CachedFileUpdaterUI)(ABI::Windows::Storage::Provider::ICachedFileUpdaterUI** value) override
        {
            *value = nullptr;
            return E_NOTIMPL;
        }

        void ResetObservations()
        {
            std::lock_guard<std::mutex> guard(m_lock);
            m_queryCounts.fill(0);
            m_kindCalls = 0;
            m_argumentsCalls = 0;
        }

        int QueryCount(ArgsInterface value) { std::lock_guard<std::mutex> guard(m_lock); return m_queryCounts[static_cast<size_t>(value)]; }
        int KindCalls() { std::lock_guard<std::mutex> guard(m_lock); return m_kindCalls; }
        int ArgumentsCalls() { std::lock_guard<std::mutex> guard(m_lock); return m_argumentsCalls; }

    private:
        // Counts a query for one of the args interfaces and returns which one it was (Count for any other interface).
        ArgsInterface RecordQuery(REFIID riid)
        {
            static const std::array<IID, static_cast<size_t>(ArgsInterface::Count)> iids =
            {
                __uuidof(wama::IActivatedEventArgs),
                __uuidof(wama::ILaunchActivatedEventArgs),
                __uuidof(wama::ISearchActivatedEventArgs),
                __uuidof(wama::IShareTargetActivatedEventArgs),
                __uuidof(wama::IFileActivatedEventArgs),
                __uuidof(wama::IFileOpenPickerActivatedEventArgs),
                __uuidof(wama::IFileSavePickerActivatedEventArgs),
                __uuidof(wama::ICachedFileUpdaterActivatedEventArgs),
            };

            std::lock_guard<std::mutex> guard(m_lock);
            for (size_t i = 0; i < iids.size(); ++i)
            {
                if (InlineIsEqualGUID(riid, iids[i]))
                {
                    ++m_queryCounts[i];
                    return static_cast<ArgsInterface>(i);
                }
            }
            return ArgsInterface::Count;
        }

        std::mutex m_lock;
        wama::ActivationKind m_kind;
        TypedArgsMode m_typedArgsMode;
        std::array<int, static_cast<size_t>(ArgsInterface::Count)> m_queryCounts = {};
        int m_kindCalls = 0;
        int m_argumentsCalls = 0;
    };

    struct ActivationCase
    {
        wama::ActivationKind kind;
        const wchar_t* name;
        ArgsInterface expectedInterface;
    };

    ::Microsoft::WRL::ComPtr<wamc::IFrameworkView> CreateFrameworkView()
    {
        ::Microsoft::WRL::ComPtr<wamc::IFrameworkView> frameworkView;
        RunOnUIThread([&]()
        {
            ::Microsoft::WRL::ComPtr<IInspectable> instance;
            VERIFY_SUCCEEDED(::RoActivateInstance(
                ::Microsoft::WRL::Wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_FrameworkView).Get(),
                &instance));
            VERIFY_SUCCEEDED(instance.As(&frameworkView));
        });
        VERIFY_IS_NOT_NULL(frameworkView.Get());
        return frameworkView;
    }

    ::Microsoft::WRL::ComPtr<IUnknown> GetCurrentApplicationIdentityOnUIThread()
    {
        ::Microsoft::WRL::ComPtr<IUnknown> identity;
        RunOnUIThread([&]()
        {
            ::Microsoft::WRL::ComPtr<::ABI::Microsoft::UI::Xaml::IApplicationStatics> applicationStatics;
            VERIFY_SUCCEEDED(::RoGetActivationFactory(
                ::Microsoft::WRL::Wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Application).Get(),
                IID_PPV_ARGS(&applicationStatics)));

            ::Microsoft::WRL::ComPtr<::ABI::Microsoft::UI::Xaml::IApplication> application;
            VERIFY_SUCCEEDED(applicationStatics->get_Current(&application));
            if (application)
            {
                VERIFY_SUCCEEDED(application.As(&identity));
            }
        });
        return identity;
    }

    HRESULT InitializeOnUIThread(const ::Microsoft::WRL::ComPtr<wamc::IFrameworkView>& frameworkView, wamc::ICoreApplicationView* view)
    {
        HRESULT result = E_PENDING;
        RunOnUIThread([&]()
        {
            result = frameworkView->Initialize(view);
        });
        return result;
    }

    HRESULT LoadOnUIThread(const ::Microsoft::WRL::ComPtr<wamc::IFrameworkView>& frameworkView, HSTRING entryPoint)
    {
        HRESULT result = E_PENDING;
        RunOnUIThread([&]()
        {
            result = frameworkView->Load(entryPoint);
        });
        return result;
    }

    // Uninitialize deinitializes the calling thread's DXamlCore, so call it on a new thread that has none, as the
    // already-deinitialized case the product handles. The thread only shares owned state, so it is safe even if the
    // wait times out and the thread is detached. Returns false on a timeout, when the call may still be in flight.
    bool TryUninitializeOnDedicatedThread(const ::Microsoft::WRL::ComPtr<wamc::IFrameworkView>& frameworkView, HRESULT* result)
    {
        struct UninitializeResult
        {
            HRESULT hr = E_PENDING;
            Event completed{ L"FrameworkViewUninitializeCompleted" };
        };

        auto shared = std::make_shared<UninitializeResult>();
        std::thread worker([frameworkView, shared]()
        {
            shared->hr = frameworkView->Uninitialize();
            shared->completed.Set();
        });

        const bool completed = shared->completed.WaitForNoThrow(std::chrono::seconds(30));
        if (completed)
        {
            worker.join();
            *result = shared->hr;
        }
        else
        {
            worker.detach();
        }
        return completed;
    }

    // Owns a FrameworkView under test and the CoreApplicationView double it is initialized with. Tests tear down
    // explicitly so they can verify each step. If a verification throws first, the destructor finishes the teardown
    // without throwing: it calls Uninitialize on a dedicated thread if an Initialize has not been followed by a completed
    // Uninitialize (but not while a timed-out Uninitialize may still be running), drops the double's Activated handler,
    // which wraps a raw FrameworkView pointer, and releases the FrameworkView on the UI thread.
    class FrameworkViewUnderTest
    {
    public:
        explicit FrameworkViewUnderTest(CoreApplicationViewDouble* view)
            : m_view(view)
            , m_frameworkView(CreateFrameworkView())
        {
        }

        FrameworkViewUnderTest(const FrameworkViewUnderTest&) = delete;
        FrameworkViewUnderTest& operator=(const FrameworkViewUnderTest&) = delete;

        ~FrameworkViewUnderTest()
        {
            try
            {
                if (m_frameworkView && m_uninitializePending && !m_uninitializeInFlight)
                {
                    LOG_OUTPUT(L"The test ended before FrameworkView.Uninitialize; uninitializing during cleanup.");
                    HRESULT result = E_PENDING;
                    if (!TryUninitializeOnDedicatedThread(m_frameworkView, &result))
                    {
                        Log::Warning(L"FrameworkView.Uninitialize did not complete during cleanup.");
                    }
                }
                Release();
            }
            catch (...)
            {
                Log::Warning(L"FrameworkView cleanup failed.");
            }
        }

        HRESULT Initialize()
        {
            m_uninitializePending = true;
            return InitializeOnUIThread(m_frameworkView, m_view.Get());
        }

        HRESULT Load(HSTRING entryPoint)
        {
            return LoadOnUIThread(m_frameworkView, entryPoint);
        }

        HRESULT Uninitialize()
        {
            VERIFY_IS_FALSE(m_uninitializeInFlight, L"A previous FrameworkView.Uninitialize is still running.");

            HRESULT result = E_PENDING;
            const bool completed = TryUninitializeOnDedicatedThread(m_frameworkView, &result);
            if (completed)
            {
                m_uninitializePending = false;
            }
            else
            {
                m_uninitializeInFlight = true;
            }

            VERIFY_IS_TRUE(completed, L"FrameworkView.Uninitialize should complete on the dedicated thread.");
            return result;
        }

        void Release()
        {
            m_view->DropHandler();
            if (m_frameworkView)
            {
                RunOnUIThread([&]()
                {
                    m_frameworkView.Reset();
                });
            }
        }

    private:
        ::Microsoft::WRL::ComPtr<CoreApplicationViewDouble> m_view;
        ::Microsoft::WRL::ComPtr<wamc::IFrameworkView> m_frameworkView;
        bool m_uninitializePending = false;
        bool m_uninitializeInFlight = false;
    };

    struct ActivationObservation
    {
        HRESULT invokeResult = E_PENDING;
        int kindReads = 0;
        int argumentsReads = 0;
        std::array<int, static_cast<size_t>(ArgsInterface::Count)> queries = {};
        ULONG argsReferencesBefore = 0;
        ULONG argsReferencesAfter = 0;
    };

    // Raises, on the UI thread, the Activated event that the FrameworkView registered with the view, using args of the
    // given kind, and records what the handler did with the args.
    ActivationObservation RaiseActivated(
        CoreApplicationViewDouble* view,
        wama::ActivationKind kind,
        TypedArgsMode typedArgsMode = TypedArgsMode::Implemented)
    {
        auto args = ::Microsoft::WRL::Make<ActivatedEventArgsDouble>(kind, typedArgsMode);
        ::Microsoft::WRL::ComPtr<wama::IActivatedEventArgs> activatedArgs(args.Get());
        ::Microsoft::WRL::ComPtr<ActivatedHandler> handler = view->GetHandler();
        VERIFY_IS_NOT_NULL(handler.Get(), L"The FrameworkView should have registered an Activated handler.");

        ActivationObservation observation;
        RunOnUIThread([&]()
        {
            observation.argsReferencesBefore = GetReferenceCount(activatedArgs.Get());
            args->ResetObservations();
            observation.invokeResult = handler->Invoke(view, activatedArgs.Get());
            observation.argsReferencesAfter = GetReferenceCount(activatedArgs.Get());

            if (FAILED(observation.invokeResult))
            {
                // Take the failure's error info off the UI thread, as the event source would.
                ::Microsoft::WRL::ComPtr<IRestrictedErrorInfo> errorInfo;
                std::ignore = ::GetRestrictedErrorInfo(&errorInfo);
            }
        });

        observation.kindReads = args->KindCalls();
        observation.argumentsReads = args->ArgumentsCalls();
        for (size_t i = 0; i < observation.queries.size(); ++i)
        {
            observation.queries[i] = args->QueryCount(static_cast<ArgsInterface>(i));
        }
        return observation;
    }
}

void ActivationABITests::FrameworkViewInitializeAndUninitializeManageActivatedRegistration()
{
    TestCleanupWrapper cleanup;

    auto view = Microsoft::WRL::Make<CoreApplicationViewDouble>(S_OK, S_OK, c_registrationToken);
    FrameworkViewUnderTest frameworkView(view.Get());
    const ULONG viewReferencesBefore = GetReferenceCount(view.Get());
    const auto applicationBefore = GetCurrentApplicationIdentityOnUIThread();
    VERIFY_IS_NOT_NULL(applicationBefore.Get());

    VERIFY_ARE_EQUAL(S_OK, frameworkView.Initialize());

    VERIFY_ARE_EQUAL(1, view->IsMainCalls());
    VERIFY_ARE_EQUAL(1, view->AddActivatedCalls());
    VERIFY_ARE_EQUAL(0, view->RemoveActivatedCalls());
    VERIFY_IS_NOT_NULL(view->GetHandler().Get(), L"Initialize should register an Activated handler.");
    VERIFY_ARE_EQUAL(viewReferencesBefore + 1, GetReferenceCount(view.Get()), L"FrameworkView should keep its CoreApplicationView.");
    VERIFY_ARE_EQUAL(applicationBefore.Get(), GetCurrentApplicationIdentityOnUIThread().Get(), L"Initializing a secondary view should keep Application.Current.");

    VERIFY_ARE_EQUAL(S_OK, frameworkView.Uninitialize());

    VERIFY_ARE_EQUAL(2, view->IsMainCalls());
    VERIFY_ARE_EQUAL(1, view->AddActivatedCalls());
    VERIFY_ARE_EQUAL(1, view->RemoveActivatedCalls());
    VERIFY_ARE_EQUAL(c_registrationToken, view->LastRemovedToken(), L"Uninitialize should remove the token that add_Activated returned.");
    VERIFY_IS_NULL(view->GetHandler().Get(), L"The Activated handler should be unregistered.");
    VERIFY_ARE_EQUAL(viewReferencesBefore, GetReferenceCount(view.Get()), L"Uninitialize should release the CoreApplicationView.");
    VERIFY_ARE_EQUAL(applicationBefore.Get(), GetCurrentApplicationIdentityOnUIThread().Get(), L"Uninitializing a secondary view should keep Application.Current.");

    frameworkView.Release();
}

void ActivationABITests::FrameworkViewInitializeMainViewKeepsApplication()
{
    TestCleanupWrapper cleanup;

    auto view = Microsoft::WRL::Make<CoreApplicationViewDouble>(S_OK, S_OK, c_registrationToken);
    FrameworkViewUnderTest frameworkView(view.Get());
    const auto applicationBefore = GetCurrentApplicationIdentityOnUIThread();
    VERIFY_IS_NOT_NULL(applicationBefore.Get());

    view->SetIsMain(true);
    const HRESULT initializeResult = frameworkView.Initialize();
    // Uninitializing a main view releases Application.Current (FrameworkApplication::ReleaseCurrent), which this test
    // process can't get back, so from here on, including any cleanup, the view reports itself as a secondary view.
    view->SetIsMain(false);

    // The thread already has a XAML core and an Application, so the main-view initialization keeps that Application
    // instead of creating one, and the view is registered like any other. That existing state also makes the main-view
    // and secondary-view initialization indistinguishable here, so this checks only that a main view initializes
    // without replacing Application.Current or failing.
    VERIFY_ARE_EQUAL(S_OK, initializeResult);
    VERIFY_ARE_EQUAL(1, view->IsMainCalls());
    VERIFY_ARE_EQUAL(1, view->AddActivatedCalls());
    VERIFY_IS_NOT_NULL(view->GetHandler().Get(), L"Initialize should register an Activated handler for the main view.");
    VERIFY_ARE_EQUAL(applicationBefore.Get(), GetCurrentApplicationIdentityOnUIThread().Get(), L"Initializing the main view should keep the existing Application.Current.");

    const auto observation = RaiseActivated(view.Get(), wama::ActivationKind_Launch);
    VERIFY_ARE_EQUAL(S_OK, observation.invokeResult);
    VERIFY_ARE_EQUAL(1, observation.argumentsReads, L"A Launch activation of the main view should reach the Application.");

    VERIFY_ARE_EQUAL(S_OK, frameworkView.Uninitialize());
    VERIFY_ARE_EQUAL(1, view->RemoveActivatedCalls());
    VERIFY_IS_NULL(view->GetHandler().Get(), L"The Activated handler should be unregistered after Uninitialize.");

    frameworkView.Release();
}

void ActivationABITests::FrameworkViewActivatedDispatchesEachActivationKindToItsArgs()
{
    TestCleanupWrapper cleanup;

    // LockScreen and LockScreenComponent also make the XAML core's background transparent. There is no getter for
    // that state, so only their dispatch is checked here.
    const ActivationCase cases[] =
    {
        { wama::ActivationKind_Launch, L"Launch", ArgsInterface::Launch },
        { wama::ActivationKind_Search, L"Search", ArgsInterface::Search },
        { wama::ActivationKind_ShareTarget, L"ShareTarget", ArgsInterface::ShareTarget },
        { wama::ActivationKind_File, L"File", ArgsInterface::File },
        { wama::ActivationKind_FileOpenPicker, L"FileOpenPicker", ArgsInterface::FileOpenPicker },
        { wama::ActivationKind_FileSavePicker, L"FileSavePicker", ArgsInterface::FileSavePicker },
        { wama::ActivationKind_CachedFileUpdater, L"CachedFileUpdater", ArgsInterface::CachedFileUpdater },
        { wama::ActivationKind_LockScreen, L"LockScreen", ArgsInterface::Generic },
        { wama::ActivationKind_LockScreenComponent, L"LockScreenComponent", ArgsInterface::Generic },
        { wama::ActivationKind_Protocol, L"Protocol", ArgsInterface::Generic },
    };

    auto view = Microsoft::WRL::Make<CoreApplicationViewDouble>(S_OK, S_OK, c_registrationToken);
    FrameworkViewUnderTest frameworkView(view.Get());
    VERIFY_ARE_EQUAL(S_OK, frameworkView.Initialize());

    for (const auto& activationCase : cases)
    {
        LOG_OUTPUT(L"Raising Activated with ActivationKind %s", activationCase.name);
        const auto observation = RaiseActivated(view.Get(), activationCase.kind);

        VERIFY_ARE_EQUAL(S_OK, observation.invokeResult);
        VERIFY_ARE_EQUAL(1, observation.kindReads, L"The handler should read the ActivationKind once.");

        // Exactly the args interface that matches the kind is queried, once; the other kinds' interfaces are not.
        for (size_t i = 0; i < observation.queries.size(); ++i)
        {
            const auto argsInterface = static_cast<ArgsInterface>(i);
            WEX::Common::String message;
            message.Format(L"Queries for %s", GetArgsInterfaceName(argsInterface));
            VERIFY_ARE_EQUAL((argsInterface == activationCase.expectedInterface) ? 1 : 0, observation.queries[i], message);
        }

        VERIFY_ARE_EQUAL((activationCase.expectedInterface == ArgsInterface::Launch) ? 1 : 0, observation.argumentsReads,
            L"Only a Launch activation passes the launch arguments on.");
        VERIFY_ARE_EQUAL(observation.argsReferencesBefore, observation.argsReferencesAfter,
            L"The handler should release every args interface it queried.");
    }

    VERIFY_ARE_EQUAL(S_OK, frameworkView.Uninitialize());
    VERIFY_ARE_EQUAL(1, view->RemoveActivatedCalls());
    VERIFY_IS_NULL(view->GetHandler().Get(), L"The Activated handler should be unregistered after Uninitialize.");

    frameworkView.Release();
}

void ActivationABITests::FrameworkViewActivatedReturnsTypedArgsFailures()
{
    TestCleanupWrapper cleanup;

    const ActivationCase cases[] =
    {
        { wama::ActivationKind_Launch, L"Launch", ArgsInterface::Launch },
        { wama::ActivationKind_Search, L"Search", ArgsInterface::Search },
        { wama::ActivationKind_ShareTarget, L"ShareTarget", ArgsInterface::ShareTarget },
        { wama::ActivationKind_File, L"File", ArgsInterface::File },
        { wama::ActivationKind_FileOpenPicker, L"FileOpenPicker", ArgsInterface::FileOpenPicker },
        { wama::ActivationKind_FileSavePicker, L"FileSavePicker", ArgsInterface::FileSavePicker },
        { wama::ActivationKind_CachedFileUpdater, L"CachedFileUpdater", ArgsInterface::CachedFileUpdater },
    };

    auto view = Microsoft::WRL::Make<CoreApplicationViewDouble>(S_OK, S_OK, c_registrationToken);
    FrameworkViewUnderTest frameworkView(view.Get());
    VERIFY_ARE_EQUAL(S_OK, frameworkView.Initialize());

    for (const auto& activationCase : cases)
    {
        const size_t typedArgs = static_cast<size_t>(activationCase.expectedInterface);

        LOG_OUTPUT(L"Raising Activated with ActivationKind %s and args without %s", activationCase.name, GetArgsInterfaceName(activationCase.expectedInterface));
        const auto refused = RaiseActivated(view.Get(), activationCase.kind, TypedArgsMode::Refused);
        VERIFY_ARE_EQUAL(E_NOINTERFACE, refused.invokeResult, L"The handler should return the failure to get the typed args.");
        VERIFY_ARE_EQUAL(1, refused.queries[typedArgs]);
        if (activationCase.kind == wama::ActivationKind_Launch)
        {
            VERIFY_ARE_EQUAL(0, refused.argumentsReads, L"The launch arguments should not be read without the typed args.");
        }
        VERIFY_ARE_EQUAL(refused.argsReferencesBefore, refused.argsReferencesAfter, L"The handler should release the args on failure.");

        // The Application's On<Kind>Activated methods reject null args with E_INVALIDARG, so the handler only returns
        // that if it passed the typed args on to the Application. Launch is not checked this way: the launch arguments
        // are read from the typed args before the Application is called.
        if (activationCase.kind != wama::ActivationKind_Launch)
        {
            LOG_OUTPUT(L"Raising Activated with ActivationKind %s and null %s", activationCase.name, GetArgsInterfaceName(activationCase.expectedInterface));
            const auto rejected = RaiseActivated(view.Get(), activationCase.kind, TypedArgsMode::Null);
            VERIFY_ARE_EQUAL(E_INVALIDARG, rejected.invokeResult, L"The handler should pass the typed args to the Application and return its failure.");
            VERIFY_ARE_EQUAL(1, rejected.queries[typedArgs]);
            VERIFY_ARE_EQUAL(rejected.argsReferencesBefore, rejected.argsReferencesAfter, L"The handler should release the args on failure.");
        }
    }

    // A failed activation leaves the registration working.
    const auto launch = RaiseActivated(view.Get(), wama::ActivationKind_Launch);
    VERIFY_ARE_EQUAL(S_OK, launch.invokeResult);
    VERIFY_ARE_EQUAL(1, launch.argumentsReads);

    VERIFY_ARE_EQUAL(S_OK, frameworkView.Uninitialize());
    VERIFY_ARE_EQUAL(1, view->RemoveActivatedCalls());

    frameworkView.Release();
}

void ActivationABITests::FrameworkViewInitializeReturnsIsMainFailure()
{
    TestCleanupWrapper cleanup;

    auto view = Microsoft::WRL::Make<CoreApplicationViewDouble>(c_isMainFailure, S_OK, c_registrationToken);
    FrameworkViewUnderTest frameworkView(view.Get());
    const ULONG viewReferencesBefore = GetReferenceCount(view.Get());

    VERIFY_ARE_EQUAL(c_isMainFailure, frameworkView.Initialize());

    VERIFY_ARE_EQUAL(1, view->IsMainCalls());
    VERIFY_ARE_EQUAL(0, view->AddActivatedCalls(), L"Initialize should not register after IsMain failed.");
    VERIFY_ARE_EQUAL(viewReferencesBefore, GetReferenceCount(view.Get()), L"Initialize should not keep the view after IsMain failed.");

    // The view was never stored, so Uninitialize has nothing to query or unregister.
    VERIFY_ARE_EQUAL(S_OK, frameworkView.Uninitialize());
    VERIFY_ARE_EQUAL(1, view->IsMainCalls());
    VERIFY_ARE_EQUAL(0, view->RemoveActivatedCalls());

    frameworkView.Release();
}

void ActivationABITests::FrameworkViewInitializeReturnsActivatedRegistrationFailure()
{
    TestCleanupWrapper cleanup;

    auto view = Microsoft::WRL::Make<CoreApplicationViewDouble>(S_OK, c_addActivatedFailure, c_registrationToken);
    FrameworkViewUnderTest frameworkView(view.Get());
    const ULONG viewReferencesBefore = GetReferenceCount(view.Get());

    VERIFY_ARE_EQUAL(c_addActivatedFailure, frameworkView.Initialize());

    VERIFY_ARE_EQUAL(1, view->IsMainCalls());
    VERIFY_ARE_EQUAL(1, view->AddActivatedCalls());
    VERIFY_IS_NULL(view->GetHandler().Get());
    VERIFY_ARE_EQUAL(viewReferencesBefore + 1, GetReferenceCount(view.Get()), L"The view is stored before the registration is attempted.");

    // Uninitialize still queries and releases the stored view, but there is no registration to remove.
    VERIFY_ARE_EQUAL(S_OK, frameworkView.Uninitialize());
    VERIFY_ARE_EQUAL(2, view->IsMainCalls());
    VERIFY_ARE_EQUAL(0, view->RemoveActivatedCalls());
    VERIFY_ARE_EQUAL(viewReferencesBefore, GetReferenceCount(view.Get()), L"Uninitialize should release the view.");

    frameworkView.Release();
}

void ActivationABITests::FrameworkViewDoesNotRemoveZeroActivatedToken()
{
    TestCleanupWrapper cleanup;

    auto view = Microsoft::WRL::Make<CoreApplicationViewDouble>(S_OK, S_OK, 0 /*tokenValue*/);
    FrameworkViewUnderTest frameworkView(view.Get());
    const ULONG viewReferencesBefore = GetReferenceCount(view.Get());

    VERIFY_ARE_EQUAL(S_OK, frameworkView.Initialize());
    VERIFY_ARE_EQUAL(1, view->AddActivatedCalls());

    VERIFY_ARE_EQUAL(S_OK, frameworkView.Uninitialize());
    VERIFY_ARE_EQUAL(2, view->IsMainCalls());
    VERIFY_ARE_EQUAL(0, view->RemoveActivatedCalls(), L"A zero token means there is no registration to remove.");
    VERIFY_ARE_EQUAL(viewReferencesBefore, GetReferenceCount(view.Get()), L"Uninitialize should release the view.");

    // Release also drops the handler the double still holds, since it was never removed.
    frameworkView.Release();
}

void ActivationABITests::FrameworkViewReinitializeDoesNotRemoveStaleActivatedToken()
{
    TestCleanupWrapper cleanup;

    auto view = Microsoft::WRL::Make<CoreApplicationViewDouble>(S_OK, S_OK, c_registrationToken);
    FrameworkViewUnderTest frameworkView(view.Get());
    const ULONG viewReferencesBefore = GetReferenceCount(view.Get());

    VERIFY_ARE_EQUAL(S_OK, frameworkView.Initialize());
    VERIFY_ARE_EQUAL(S_OK, frameworkView.Uninitialize());
    VERIFY_ARE_EQUAL(1, view->RemoveActivatedCalls());
    VERIFY_ARE_EQUAL(c_registrationToken, view->LastRemovedToken());

    // Initialize again, but this time the registration fails and leaves the token output untouched.
    view->SetAddActivatedResult(c_addActivatedFailure);
    VERIFY_ARE_EQUAL(c_addActivatedFailure, frameworkView.Initialize());
    VERIFY_ARE_EQUAL(2, view->AddActivatedCalls());

    VERIFY_ARE_EQUAL(S_OK, frameworkView.Uninitialize());
    VERIFY_ARE_EQUAL(4, view->IsMainCalls());
    VERIFY_ARE_EQUAL(1, view->RemoveActivatedCalls(), L"The token removed by the first Uninitialize should not be removed again.");
    VERIFY_ARE_EQUAL(viewReferencesBefore, GetReferenceCount(view.Get()), L"Uninitialize should release the view.");

    frameworkView.Release();
}

void ActivationABITests::FrameworkViewUninitializeIsNoOpWithoutView()
{
    TestCleanupWrapper cleanup;

    auto view = Microsoft::WRL::Make<CoreApplicationViewDouble>(S_OK, S_OK, c_registrationToken);
    FrameworkViewUnderTest frameworkView(view.Get());
    const ULONG viewReferencesBefore = GetReferenceCount(view.Get());

    VERIFY_ARE_EQUAL(S_OK, frameworkView.Uninitialize(), L"Uninitialize before Initialize should succeed.");

    VERIFY_ARE_EQUAL(S_OK, frameworkView.Initialize());
    VERIFY_ARE_EQUAL(S_OK, frameworkView.Uninitialize());
    VERIFY_ARE_EQUAL(2, view->IsMainCalls());
    VERIFY_ARE_EQUAL(1, view->RemoveActivatedCalls());

    VERIFY_ARE_EQUAL(S_OK, frameworkView.Uninitialize(), L"A second Uninitialize should succeed.");
    VERIFY_ARE_EQUAL(2, view->IsMainCalls(), L"A second Uninitialize should not query the released view.");
    VERIFY_ARE_EQUAL(1, view->RemoveActivatedCalls(), L"A second Uninitialize should not unregister again.");
    VERIFY_ARE_EQUAL(viewReferencesBefore, GetReferenceCount(view.Get()));

    frameworkView.Release();
}

void ActivationABITests::FrameworkViewLoadSucceedsWithoutTouchingTheView()
{
    TestCleanupWrapper cleanup;

    auto view = Microsoft::WRL::Make<CoreApplicationViewDouble>(S_OK, S_OK, c_registrationToken);
    FrameworkViewUnderTest frameworkView(view.Get());

    VERIFY_ARE_EQUAL(S_OK, frameworkView.Initialize());

    VERIFY_ARE_EQUAL(S_OK, frameworkView.Load(Microsoft::WRL::Wrappers::HStringReference(L"App").Get()));
    VERIFY_ARE_EQUAL(S_OK, frameworkView.Load(nullptr));
    VERIFY_ARE_EQUAL(1, view->IsMainCalls(), L"Load should not query the view.");
    VERIFY_ARE_EQUAL(1, view->AddActivatedCalls(), L"Load should not register again.");
    VERIFY_ARE_EQUAL(0, view->RemoveActivatedCalls(), L"Load should not unregister.");

    VERIFY_ARE_EQUAL(S_OK, frameworkView.Uninitialize());
    frameworkView.Release();
}

} } } } } }
