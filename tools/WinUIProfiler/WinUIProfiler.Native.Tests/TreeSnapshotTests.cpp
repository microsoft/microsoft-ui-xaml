#include <windows.h>
#include <xamlom.h>
#include <wrl.h>
#include <cstdio>
#include <thread>
#include <atomic>
#include "..\WinUIProfiler.Tap\TreeSnapshot.h"
#include "..\WinUIProfiler.Tap\TreeSnapshotJson.h"
#include "..\WinUIProfiler.Tap\Tap.h"

using namespace WinUIProfiler::Tap;
using Microsoft::WRL::RuntimeClass;
using Microsoft::WRL::RuntimeClassFlags;
using Microsoft::WRL::ClassicCom;

namespace
{
    template<typename Base>
    class DiagnosticsMethods : public Base
    {
    public:
        HRESULT STDMETHODCALLTYPE GetDispatcher(IInspectable**) override { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE GetUiLayer(IInspectable**) override { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE GetApplication(IInspectable**) override { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE GetIInspectableFromHandle(InstanceHandle, IInspectable**) override { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE GetHandleFromIInspectable(IInspectable*, InstanceHandle*) override { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE HitTest(RECT, unsigned int*, InstanceHandle**) override { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE RegisterInstance(IInspectable*, InstanceHandle*) override { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE GetInitializationData(BSTR*) override { return E_NOTIMPL; }
    };
    using Diagnostics = DiagnosticsMethods<RuntimeClass<RuntimeClassFlags<ClassicCom>, IXamlDiagnostics>>;

    class Service final : public DiagnosticsMethods<RuntimeClass<RuntimeClassFlags<ClassicCom>,
        IXamlDiagnostics, IVisualTreeService, Microsoft::WRL::FtmBase>>
    {
    public:
        Handle Entered{ CreateEventW(nullptr, TRUE, FALSE, nullptr) };
        Handle ReleaseAdvise{ CreateEventW(nullptr, TRUE, TRUE, nullptr) };
        std::atomic<unsigned> Unadvised{};
        DWORD AdvisingThread{};
        HRESULT AdviseResult = S_OK;
        HRESULT UnadviseResult = S_OK;
        DWORD WaitMilliseconds = 5000;
        Microsoft::WRL::ComPtr<IVisualTreeServiceCallback> Retained;
        HRESULT STDMETHODCALLTYPE AdviseVisualTreeChange(IVisualTreeServiceCallback* callback) override
        {
            AdvisingThread = GetCurrentThreadId();
            Retained = callback;
            SetEvent(Entered.Get());
            WaitForSingleObject(ReleaseAdvise.Get(), WaitMilliseconds);
            return AdviseResult;
        }
        HRESULT STDMETHODCALLTYPE UnadviseVisualTreeChange(IVisualTreeServiceCallback*) override
        {
            ++Unadvised;
            if (SUCCEEDED(UnadviseResult)) Retained.Reset();
            return UnadviseResult;
        }
        HRESULT STDMETHODCALLTYPE GetEnums(unsigned*, EnumType**) override { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE CreateInstance(BSTR, BSTR, InstanceHandle*) override { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE GetPropertyValuesChain(InstanceHandle, unsigned*, PropertyChainSource**,
            unsigned*, PropertyChainValue**) override { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE SetProperty(InstanceHandle, InstanceHandle, unsigned) override { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE ClearProperty(InstanceHandle, unsigned) override { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE GetCollectionCount(InstanceHandle, unsigned*) override { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE GetCollectionElements(InstanceHandle, unsigned, unsigned*,
            CollectionElementValue**) override { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE AddChild(InstanceHandle, InstanceHandle, unsigned) override { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE RemoveChild(InstanceHandle, unsigned) override { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE ClearChildren(InstanceHandle) override { return E_NOTIMPL; }
    };
    int checks{}, failures{};
    void Check(bool condition, const char* message)
    {
        ++checks;
        if (!condition) { ++failures; std::printf("FAIL: %s\n", message); }
    }
    void AwaitWorkers()
    {
        const auto deadline = GetTickCount64() + 5000;
        while (ActiveServers && GetTickCount64() < deadline) Sleep(1);
        Check(ActiveServers == 0, "subscription worker released all server/module ownership");
    }
}

int main()
{
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    const GUID serializationId{ 0x00112233, 0x4455, 0x6677,
        {0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff} };
    Check(SnapshotJson::CaptureIdText(serializationId) ==
        L"00112233-4455-6677-8899-AABBCCDDEEFF",
        "captureId uses unbraced 36-character D-format required by System.Text.Json Guid");
    Check(SnapshotJson::CaptureIdText(GUID{}) == L"00000000-0000-0000-0000-000000000000",
        "D-format preserves all zero-padded GUID fields");
    if (failures) { CoUninitialize(); return 1; }
    auto diagnostics = Microsoft::WRL::Make<Diagnostics>();
    TreeSnapshotResult result{ "stale" };
    std::string failure;
    const GUID capture{ 1, 2, 3, {4, 5, 6, 7, 8, 9, 10, 11} };
    FILETIME creation{}, exit{}, kernel{}, user{};
    GetProcessTimes(GetCurrentProcess(), &creation, &exit, &kernel, &user);
    const uint64_t creationTime = (static_cast<uint64_t>(creation.dwHighDateTime) << 32) | creation.dwLowDateTime;
    Check(CaptureTreeSnapshot(nullptr, nullptr, capture, 1, result, failure) == E_POINTER,
        "null diagnostics rejected");
    Check(result.Json.empty() && !failure.empty(), "failure clears stale output and explains cause");
    result.Json = "stale";
    Check(CaptureTreeSnapshot(diagnostics.Get(), nullptr, capture, creationTime, result, failure) == E_NOINTERFACE,
        "standard tree service is required (no private bridge fallback)");
    Check(result.Json.empty() && !failure.empty(), "unavailable service never fabricates success");
    AwaitWorkers();
    Check(CaptureTreeSnapshot(diagnostics.Get(), nullptr, capture, creationTime + 1, result, failure) ==
        E_INVALIDARG, "creation time must match injected process");
    HANDLE stop = CreateEventW(nullptr, TRUE, TRUE, nullptr);
    Check(CaptureTreeSnapshot(diagnostics.Get(), stop, capture, 1, result, failure) ==
        HRESULT_FROM_WIN32(ERROR_CANCELLED), "pre-cancelled command is rejected");
    CloseHandle(stop);
    {
        auto service = Microsoft::WRL::Make<Service>();
        Check(CaptureTreeSnapshot(service.Get(), nullptr, capture, creationTime, result, failure) ==
            HRESULT_FROM_WIN32(ERROR_NOT_FOUND), "empty standard tree is failure, not fabricated snapshot");
        Check(result.Json.empty() && !failure.empty(), "empty-tree error explains missing realized roots");
        Check(service->AdvisingThread != GetCurrentThreadId() && service->Unadvised == 1,
            "Advise and matching Unadvise are owned by off-caller MTA worker");
        AwaitWorkers();
    }
    {
        auto service = Microsoft::WRL::Make<Service>();
        service->AdviseResult = E_ACCESSDENIED;
        Check(CaptureTreeSnapshot(service.Get(), nullptr, capture, creationTime, result, failure) ==
            E_ACCESSDENIED, "Advise failure preserved");
        Check(service->Unadvised == 1, "partial failed Advise still attempts Unadvise");
        AwaitWorkers();
    }
    {
        auto service = Microsoft::WRL::Make<Service>();
        ResetEvent(service->ReleaseAdvise.Get());
        Handle cancelled{ CreateEventW(nullptr, TRUE, FALSE, nullptr) };
        HRESULT captured = E_UNEXPECTED;
        std::thread caller([&]
        {
            CoInitializeEx(nullptr, COINIT_MULTITHREADED);
            captured = CaptureTreeSnapshot(service.Get(), cancelled.Get(), capture, creationTime, result, failure);
            CoUninitialize();
        });
        Check(WaitForSingleObject(service->Entered.Get(), 5000) == WAIT_OBJECT_0, "blocking Advise entered");
        SetEvent(cancelled.Get());
        caller.join();
        Check(captured == HRESULT_FROM_WIN32(ERROR_CANCELLED) && result.Json.empty(),
            "cancellation returns while UI-bound Advise remains blocked");
        Check(ActiveServers > 0, "abandoned worker keeps DLL/server alive");
        cancelled.Reset(); // Worker owns a duplicated event, never the caller's borrowed handle.
        Check(CaptureTreeSnapshot(service.Get(), nullptr, capture, creationTime, result, failure) ==
            HRESULT_FROM_WIN32(ERROR_BUSY), "blocked subscription cannot accumulate background jobs");
        SetEvent(service->ReleaseAdvise.Get());
        AwaitWorkers();
        Check(service->Unadvised == 1, "cancelled blocked subscription eventually unadvises");
    }
    {
        auto service = Microsoft::WRL::Make<Service>();
        ResetEvent(service->ReleaseAdvise.Get());
        service->WaitMilliseconds = 30000;
        const auto start = GetTickCount64();
        Check(CaptureTreeSnapshot(service.Get(), nullptr, capture, creationTime, result, failure) ==
            HRESULT_FROM_WIN32(ERROR_TIMEOUT), "20 second deadline bounds a blocked UI subscription");
        const auto elapsed = GetTickCount64() - start;
        Check(elapsed >= 19000 && elapsed < 25000 && result.Json.empty(),
            "deadline returns bounded failure without truncated or fabricated JSON");
        Check(ActiveServers > 0, "deadline keeps executing DLL pinned until subscription cleanup");
        SetEvent(service->ReleaseAdvise.Get());
        AwaitWorkers();
        Check(service->Unadvised == 1, "deadline-abandoned subscription eventually unadvises");
    }
    {
        auto service = Microsoft::WRL::Make<Service>();
        service->UnadviseResult = E_FAIL;
        Check(FAILED(CaptureTreeSnapshot(service.Get(), nullptr, capture, creationTime, result, failure)),
            "failed cleanup is never successful snapshot");
        AwaitWorkers();
        // A faulty runtime can retain and call the sink after the command/job died.
        // It must be inert, not dereference freed state or diagnostics.
        VisualElement element{};
        element.Handle = 0xffffffffffffffff;
        Check(service->Retained && service->Retained->OnVisualTreeChange({}, element, Add) == S_OK,
            "late callback after failed Unadvise is inert and safe");
        service->Retained.Reset();
    }
    CoUninitialize();
    std::printf("TreeSnapshot: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
