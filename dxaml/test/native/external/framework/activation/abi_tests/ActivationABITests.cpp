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
#include <thread>

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

    // Plays the CoreApplicationView of a secondary (non-main) view: reports IsMain and owns the Activated event.
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

            *value = FALSE;
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

        // The handler wraps a raw FrameworkView pointer, so drop it before the FrameworkView goes away.
        void DropHandler() { std::lock_guard<std::mutex> guard(m_lock); m_handler.Reset(); }

    private:
        std::mutex m_lock;
        HRESULT m_isMainResult;
        HRESULT m_addActivatedResult;
        INT64 m_tokenValue;
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

    // Activation args that implement every typed args interface FrameworkView knows about, and record which of them
    // FrameworkView queries for, so a test can tell which branch handled a given ActivationKind.
    class ActivatedEventArgsDouble final : public ActivatedEventArgsDoubleBase
    {
        InspectableClass(nullptr /*internal*/, BaseTrust);

    public:
        explicit ActivatedEventArgsDouble(wama::ActivationKind kind)
            : m_kind(kind)
        {
        }

        // WRL calls this from QueryInterface before its own interface lookup.
        HRESULT CustomQueryInterface(REFIID riid, void** /*ppvObject*/, bool* handled)
        {
            RecordQuery(riid);
            *handled = false;
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
        void RecordQuery(REFIID riid)
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
                }
            }
        }

        std::mutex m_lock;
        wama::ActivationKind m_kind;
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
    // wait times out and the thread is detached.
    HRESULT UninitializeOnDedicatedThread(const ::Microsoft::WRL::ComPtr<wamc::IFrameworkView>& frameworkView)
    {
        struct UninitializeResult
        {
            HRESULT hr = E_PENDING;
            Event completed{ L"FrameworkViewUninitializeCompleted" };
        };

        auto result = std::make_shared<UninitializeResult>();
        std::thread worker([frameworkView, result]()
        {
            result->hr = frameworkView->Uninitialize();
            result->completed.Set();
        });

        const bool completed = result->completed.WaitForNoThrow(std::chrono::seconds(30));
        if (completed)
        {
            worker.join();
        }
        else
        {
            worker.detach();
        }

        VERIFY_IS_TRUE(completed, L"FrameworkView.Uninitialize should complete on the dedicated thread.");
        return result->hr;
    }

    void ReleaseOnUIThread(::Microsoft::WRL::ComPtr<wamc::IFrameworkView>& frameworkView)
    {
        RunOnUIThread([&]()
        {
            frameworkView.Reset();
        });
    }

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
    ActivationObservation RaiseActivated(CoreApplicationViewDouble* view, wama::ActivationKind kind)
    {
        auto args = ::Microsoft::WRL::Make<ActivatedEventArgsDouble>(kind);
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
    auto frameworkView = CreateFrameworkView();
    const ULONG viewReferencesBefore = GetReferenceCount(view.Get());
    const auto applicationBefore = GetCurrentApplicationIdentityOnUIThread();
    VERIFY_IS_NOT_NULL(applicationBefore.Get());

    VERIFY_ARE_EQUAL(S_OK, InitializeOnUIThread(frameworkView, view.Get()));

    VERIFY_ARE_EQUAL(1, view->IsMainCalls());
    VERIFY_ARE_EQUAL(1, view->AddActivatedCalls());
    VERIFY_ARE_EQUAL(0, view->RemoveActivatedCalls());
    VERIFY_IS_NOT_NULL(view->GetHandler().Get(), L"Initialize should register an Activated handler.");
    VERIFY_ARE_EQUAL(viewReferencesBefore + 1, GetReferenceCount(view.Get()), L"FrameworkView should keep its CoreApplicationView.");
    VERIFY_ARE_EQUAL(applicationBefore.Get(), GetCurrentApplicationIdentityOnUIThread().Get(), L"Initializing a secondary view should keep Application.Current.");

    VERIFY_ARE_EQUAL(S_OK, UninitializeOnDedicatedThread(frameworkView));

    VERIFY_ARE_EQUAL(2, view->IsMainCalls());
    VERIFY_ARE_EQUAL(1, view->AddActivatedCalls());
    VERIFY_ARE_EQUAL(1, view->RemoveActivatedCalls());
    VERIFY_ARE_EQUAL(c_registrationToken, view->LastRemovedToken(), L"Uninitialize should remove the token that add_Activated returned.");
    VERIFY_IS_NULL(view->GetHandler().Get(), L"The Activated handler should be unregistered.");
    VERIFY_ARE_EQUAL(viewReferencesBefore, GetReferenceCount(view.Get()), L"Uninitialize should release the CoreApplicationView.");
    VERIFY_ARE_EQUAL(applicationBefore.Get(), GetCurrentApplicationIdentityOnUIThread().Get(), L"Uninitializing a secondary view should keep Application.Current.");

    ReleaseOnUIThread(frameworkView);
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
    auto frameworkView = CreateFrameworkView();
    VERIFY_ARE_EQUAL(S_OK, InitializeOnUIThread(frameworkView, view.Get()));

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

    VERIFY_ARE_EQUAL(S_OK, UninitializeOnDedicatedThread(frameworkView));
    VERIFY_ARE_EQUAL(1, view->RemoveActivatedCalls());
    VERIFY_IS_NULL(view->GetHandler().Get(), L"The Activated handler should be unregistered after Uninitialize.");

    ReleaseOnUIThread(frameworkView);
}

void ActivationABITests::FrameworkViewInitializeReturnsIsMainFailure()
{
    TestCleanupWrapper cleanup;

    auto view = Microsoft::WRL::Make<CoreApplicationViewDouble>(c_isMainFailure, S_OK, c_registrationToken);
    auto frameworkView = CreateFrameworkView();
    const ULONG viewReferencesBefore = GetReferenceCount(view.Get());

    VERIFY_ARE_EQUAL(c_isMainFailure, InitializeOnUIThread(frameworkView, view.Get()));

    VERIFY_ARE_EQUAL(1, view->IsMainCalls());
    VERIFY_ARE_EQUAL(0, view->AddActivatedCalls(), L"Initialize should not register after IsMain failed.");
    VERIFY_ARE_EQUAL(viewReferencesBefore, GetReferenceCount(view.Get()), L"Initialize should not keep the view after IsMain failed.");

    // The view was never stored, so Uninitialize has nothing to query or unregister.
    VERIFY_ARE_EQUAL(S_OK, UninitializeOnDedicatedThread(frameworkView));
    VERIFY_ARE_EQUAL(1, view->IsMainCalls());
    VERIFY_ARE_EQUAL(0, view->RemoveActivatedCalls());

    ReleaseOnUIThread(frameworkView);
}

void ActivationABITests::FrameworkViewInitializeReturnsActivatedRegistrationFailure()
{
    TestCleanupWrapper cleanup;

    auto view = Microsoft::WRL::Make<CoreApplicationViewDouble>(S_OK, c_addActivatedFailure, c_registrationToken);
    auto frameworkView = CreateFrameworkView();
    const ULONG viewReferencesBefore = GetReferenceCount(view.Get());

    VERIFY_ARE_EQUAL(c_addActivatedFailure, InitializeOnUIThread(frameworkView, view.Get()));

    VERIFY_ARE_EQUAL(1, view->IsMainCalls());
    VERIFY_ARE_EQUAL(1, view->AddActivatedCalls());
    VERIFY_IS_NULL(view->GetHandler().Get());
    VERIFY_ARE_EQUAL(viewReferencesBefore + 1, GetReferenceCount(view.Get()), L"The view is stored before the registration is attempted.");

    // Uninitialize still queries and releases the stored view, but there is no registration to remove.
    VERIFY_ARE_EQUAL(S_OK, UninitializeOnDedicatedThread(frameworkView));
    VERIFY_ARE_EQUAL(2, view->IsMainCalls());
    VERIFY_ARE_EQUAL(0, view->RemoveActivatedCalls());
    VERIFY_ARE_EQUAL(viewReferencesBefore, GetReferenceCount(view.Get()), L"Uninitialize should release the view.");

    ReleaseOnUIThread(frameworkView);
}

void ActivationABITests::FrameworkViewDoesNotRemoveZeroActivatedToken()
{
    TestCleanupWrapper cleanup;

    auto view = Microsoft::WRL::Make<CoreApplicationViewDouble>(S_OK, S_OK, 0 /*tokenValue*/);
    auto frameworkView = CreateFrameworkView();
    const ULONG viewReferencesBefore = GetReferenceCount(view.Get());

    VERIFY_ARE_EQUAL(S_OK, InitializeOnUIThread(frameworkView, view.Get()));
    VERIFY_ARE_EQUAL(1, view->AddActivatedCalls());

    VERIFY_ARE_EQUAL(S_OK, UninitializeOnDedicatedThread(frameworkView));
    VERIFY_ARE_EQUAL(2, view->IsMainCalls());
    VERIFY_ARE_EQUAL(0, view->RemoveActivatedCalls(), L"A zero token means there is no registration to remove.");
    VERIFY_ARE_EQUAL(viewReferencesBefore, GetReferenceCount(view.Get()), L"Uninitialize should release the view.");

    view->DropHandler();
    ReleaseOnUIThread(frameworkView);
}

void ActivationABITests::FrameworkViewReinitializeDoesNotRemoveStaleActivatedToken()
{
    TestCleanupWrapper cleanup;

    auto view = Microsoft::WRL::Make<CoreApplicationViewDouble>(S_OK, S_OK, c_registrationToken);
    auto frameworkView = CreateFrameworkView();
    const ULONG viewReferencesBefore = GetReferenceCount(view.Get());

    VERIFY_ARE_EQUAL(S_OK, InitializeOnUIThread(frameworkView, view.Get()));
    VERIFY_ARE_EQUAL(S_OK, UninitializeOnDedicatedThread(frameworkView));
    VERIFY_ARE_EQUAL(1, view->RemoveActivatedCalls());
    VERIFY_ARE_EQUAL(c_registrationToken, view->LastRemovedToken());

    // Initialize again, but this time the registration fails and leaves the token output untouched.
    view->SetAddActivatedResult(c_addActivatedFailure);
    VERIFY_ARE_EQUAL(c_addActivatedFailure, InitializeOnUIThread(frameworkView, view.Get()));
    VERIFY_ARE_EQUAL(2, view->AddActivatedCalls());

    VERIFY_ARE_EQUAL(S_OK, UninitializeOnDedicatedThread(frameworkView));
    VERIFY_ARE_EQUAL(4, view->IsMainCalls());
    VERIFY_ARE_EQUAL(1, view->RemoveActivatedCalls(), L"The token removed by the first Uninitialize should not be removed again.");
    VERIFY_ARE_EQUAL(viewReferencesBefore, GetReferenceCount(view.Get()), L"Uninitialize should release the view.");

    ReleaseOnUIThread(frameworkView);
}

void ActivationABITests::FrameworkViewUninitializeIsNoOpWithoutView()
{
    TestCleanupWrapper cleanup;

    auto view = Microsoft::WRL::Make<CoreApplicationViewDouble>(S_OK, S_OK, c_registrationToken);
    auto frameworkView = CreateFrameworkView();
    const ULONG viewReferencesBefore = GetReferenceCount(view.Get());

    VERIFY_ARE_EQUAL(S_OK, UninitializeOnDedicatedThread(frameworkView), L"Uninitialize before Initialize should succeed.");

    VERIFY_ARE_EQUAL(S_OK, InitializeOnUIThread(frameworkView, view.Get()));
    VERIFY_ARE_EQUAL(S_OK, UninitializeOnDedicatedThread(frameworkView));
    VERIFY_ARE_EQUAL(2, view->IsMainCalls());
    VERIFY_ARE_EQUAL(1, view->RemoveActivatedCalls());

    VERIFY_ARE_EQUAL(S_OK, UninitializeOnDedicatedThread(frameworkView), L"A second Uninitialize should succeed.");
    VERIFY_ARE_EQUAL(2, view->IsMainCalls(), L"A second Uninitialize should not query the released view.");
    VERIFY_ARE_EQUAL(1, view->RemoveActivatedCalls(), L"A second Uninitialize should not unregister again.");
    VERIFY_ARE_EQUAL(viewReferencesBefore, GetReferenceCount(view.Get()));

    ReleaseOnUIThread(frameworkView);
}

void ActivationABITests::FrameworkViewLoadSucceedsWithoutTouchingTheView()
{
    TestCleanupWrapper cleanup;

    auto view = Microsoft::WRL::Make<CoreApplicationViewDouble>(S_OK, S_OK, c_registrationToken);
    auto frameworkView = CreateFrameworkView();

    VERIFY_ARE_EQUAL(S_OK, InitializeOnUIThread(frameworkView, view.Get()));

    VERIFY_ARE_EQUAL(S_OK, LoadOnUIThread(frameworkView, Microsoft::WRL::Wrappers::HStringReference(L"App").Get()));
    VERIFY_ARE_EQUAL(S_OK, LoadOnUIThread(frameworkView, nullptr));
    VERIFY_ARE_EQUAL(1, view->IsMainCalls(), L"Load should not query the view.");
    VERIFY_ARE_EQUAL(1, view->AddActivatedCalls(), L"Load should not register again.");
    VERIFY_ARE_EQUAL(0, view->RemoveActivatedCalls(), L"Load should not unregister.");

    VERIFY_ARE_EQUAL(S_OK, UninitializeOnDedicatedThread(frameworkView));
    ReleaseOnUIThread(frameworkView);
}

} } } } } }
