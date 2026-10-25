#include <windows.h>
#include <ocidl.h>
#include <xamlOM.h>
#include <wrl.h>
#include <WinUIProfilerProtocol.h>
#include "..\WinUIProfiler.Tap\Protocol.h"
#include "..\WinUIProfiler.Tap\Tap.h"
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>

namespace
{
    using namespace Microsoft::WRL;
    class DiagnosticsSite final : public RuntimeClass<RuntimeClassFlags<ClassicCom>,
        IXamlDiagnostics, WinUIProfiler::IWinUIProfilerRuntimeDiagnostics, FtmBase>
    {
    public:
        std::wstring Initialization;
        WinUIProfiler::Tap::Bootstrap Bootstrap;
        HRESULT STDMETHODCALLTYPE GetDispatcher(IInspectable**) override { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE GetUiLayer(IInspectable**) override { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE GetApplication(IInspectable**) override { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE GetIInspectableFromHandle(InstanceHandle, IInspectable**) override { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE GetHandleFromIInspectable(IInspectable*, InstanceHandle*) override { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE HitTest(RECT, unsigned int*, InstanceHandle**) override { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE RegisterInstance(IInspectable*, InstanceHandle*) override { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE GetInitializationData(BSTR* output) override
        {
            if (!output) return E_POINTER;
            *output = SysAllocStringLen(Initialization.data(), static_cast<UINT>(Initialization.size()));
            return *output ? S_OK : E_OUTOFMEMORY;
        }
        HRESULT STDMETHODCALLTYPE GetProfilerProtocolInfo(uint32_t version, uint32_t size,
            WinUIProfiler::ProtocolInfo* info) override
        {
            if (!info || version != 1 || size != sizeof(*info)) return E_INVALIDARG;
            *info = {};
            info->Size = sizeof(*info);
            info->Version = 1;
            info->ProducerEpoch = Bootstrap.Session;
            info->Process = {Bootstrap.Target.Pid, 1,
                static_cast<int64_t>(Bootstrap.Target.CreationTime), Bootstrap.Target.Instance};
            LARGE_INTEGER frequency{};
            if (!QueryPerformanceFrequency(&frequency)) return HRESULT_FROM_WIN32(GetLastError());
            info->QpcFrequency = frequency.QuadPart;
#ifdef _M_ARM64
            info->Architecture = 1;
#endif
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE GetProfilerSequenceCut(uint32_t, uint16_t, uint32_t,
            WinUIProfiler::SequenceCut*) override { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE GetProfilerDispatcher(uint32_t, uint32_t, IInspectable**) override { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE RequestProfilerCapture(uint32_t, uint32_t, GUID) override { return E_NOTIMPL; }
    };

    template<typename T> T Export(HMODULE module, const char* name)
    {
        const auto address = GetProcAddress(module, name);
        static_assert(sizeof(T) == sizeof(address));
        T function{};
        std::memcpy(&function, &address, sizeof(function));
        return function;
    }

    int Run()
    {
        std::wstring input;
        std::getline(std::wcin, input);
        auto site = Make<DiagnosticsSite>();
        if (!site) return 2;
        site->Initialization = input;
        if (FAILED(WinUIProfiler::Tap::ParseBootstrap(input.c_str(), site->Bootstrap))) return 3;
        wchar_t executable[32768]{};
        const DWORD length = GetModuleFileNameW(nullptr, executable, ARRAYSIZE(executable));
        if (!length || length == ARRAYSIZE(executable)) return 4;
        const auto path = std::filesystem::path(executable).parent_path() / L"WinUIProfiler.Tap.dll";
        HMODULE module = LoadLibraryExW(path.c_str(), nullptr,
            LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (!module) return 5;
        const auto factoryMethod = Export<HRESULT(__stdcall*)(REFCLSID, REFIID, void**)>(module, "DllGetClassObject");
        const auto canUnload = Export<HRESULT(__stdcall*)()>(module, "DllCanUnloadNow");
        if (!factoryMethod || !canUnload) { FreeLibrary(module); return 6; }
        ComPtr<IClassFactory> factory;
        HRESULT result = factoryMethod(WinUIProfiler::Tap::ClassId, IID_PPV_ARGS(&factory));
        ComPtr<IObjectWithSite> tap;
        if (SUCCEEDED(result)) result = factory->CreateInstance(nullptr, IID_PPV_ARGS(&tap));
        if (SUCCEEDED(result)) result = tap->SetSite(static_cast<IXamlDiagnostics*>(site.Get()));
        if (FAILED(result))
        {
            std::fprintf(stderr, "SetSite HRESULT 0x%08lX\n", static_cast<unsigned long>(result));
            tap.Reset();
            factory.Reset();
            FreeLibrary(module);
            return 7;
        }
        // Framework diagnostics releases its temporary TAP interface after SetSite.
        tap.Reset();
        factory.Reset();
        std::puts("READY");
        std::fflush(stdout);
        const auto deadline = GetTickCount64() + 15000;
        while (canUnload() != S_OK && GetTickCount64() < deadline) Sleep(5);
        const bool unloaded = canUnload() == S_OK;
        site->AddRef();
        const ULONG references = site->Release();
        FreeLibrary(module);
        while (GetModuleHandleW(L"WinUIProfiler.Tap.dll") && GetTickCount64() < deadline) Sleep(5);
        if (!unloaded || references != 1 || GetModuleHandleW(L"WinUIProfiler.Tap.dll"))
        {
            std::fprintf(stderr, "Cleanup: idle=%d siteRefs=%lu\n", unloaded, references);
            return 8;
        }
        std::puts("UNLOADED");
        std::fflush(stdout);
        return 0;
    }
}

int main()
{
    const HRESULT apartment = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(apartment)) return 1;
    const int result = Run();
    CoUninitialize();
    return result;
}
