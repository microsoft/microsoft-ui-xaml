#include "PipeServer.h"
#include <ocidl.h>
#include <xamlOM.h>
#include <mutex>
#include <cstdio>

namespace WinUIProfiler::Tap
{
    namespace
    {
        using namespace Microsoft::WRL;
        std::atomic<ULONG> ServerLocks{};

        struct SiteState
        {
            std::mutex Mutex;
            ComPtr<IUnknown> Site;
            std::weak_ptr<PipeServer> Server;
        };

        class TapObject final : public RuntimeClass<RuntimeClassFlags<ClassicCom>, IObjectWithSite, FtmBase>
        {
        public:
            TapObject() : state(std::make_shared<SiteState>()) { ++ActiveObjects; }
            ~TapObject() override
            {
                --ActiveObjects;
            }

            HRESULT STDMETHODCALLTYPE SetSite(IUnknown* site) override
            {
                const wchar_t* phase = L"disconnect";
                const HRESULT result = Attach(site, phase);
                if (FAILED(result))
                {
                    wchar_t message[256]{};
                    swprintf_s(message, L"WinUIProfiler TAP SetSite %s failed: HRESULT 0x%08lX.\n",
                        phase, static_cast<unsigned long>(result));
                    OutputDebugStringW(message);
                }
                return result;
            }

            HRESULT Attach(IUnknown* site, const wchar_t*& phase)
            {
                if (!site)
                {
                    ComPtr<IUnknown> released;
                    std::shared_ptr<PipeServer> server;
                    {
                        std::lock_guard<std::mutex> lock(state->Mutex);
                        released = std::move(state->Site);
                        server = state->Server.lock();
                        state->Server.reset();
                    }
                    if (server) server->Stop();
                    return S_OK;
                }
                try
                {
                    phase = L"IXamlDiagnostics query";
                    ComPtr<IXamlDiagnostics> diagnostics;
                    HRESULT result = site->QueryInterface(IID_PPV_ARGS(&diagnostics));
                    if (FAILED(result)) return result;
                    ComPtr<IWinUIProfilerRuntimeDiagnostics> runtime;
                    phase = L"private runtime interface query";
                    result = site->QueryInterface(IID_PPV_ARGS(&runtime));
                    if (FAILED(result) && result != E_NOINTERFACE) return result;
                    if (result == E_NOINTERFACE)
                        OutputDebugStringW(L"WinUIProfiler TAP: private runtime bridge unavailable; transport and hello remain supported.\n");
                    struct Initialization
                    {
                        BSTR Text{};
                        ~Initialization() { SysFreeString(Text); }
                    } initialization;
                    phase = L"initialization data";
                    result = diagnostics->GetInitializationData(&initialization.Text);
                    if (FAILED(result)) return result;
                    if (!initialization.Text || SysStringLen(initialization.Text) > 1024 ||
                        wcsnlen_s(initialization.Text, 1025) != SysStringLen(initialization.Text))
                        return E_INVALIDARG;
                    Bootstrap bootstrap{};
                    phase = L"bootstrap validation";
                    result = ParseBootstrap(initialization.Text, bootstrap);
                    if (FAILED(result))
                    {
                        unsigned separators{};
                        for (const wchar_t* character = initialization.Text; *character; ++character)
                            if (*character == L'|') ++separators;
                        wchar_t message[128]{};
                        swprintf_s(message, L"WinUIProfiler bootstrap shape: characters=%u separators=%u.\n",
                            SysStringLen(initialization.Text), separators);
                        OutputDebugStringW(message);
                        return result;
                    }
                    const auto active = state;
                    auto server = std::make_shared<PipeServer>(std::move(bootstrap), std::move(runtime),
                        [active](PipeServer* ended)
                        {
                            ComPtr<IUnknown> released;
                            {
                                std::lock_guard<std::mutex> lock(active->Mutex);
                                if (active->Server.lock().get() != ended) return;
                                released = std::move(active->Site);
                                active->Server.reset();
                            }
                        }, diagnostics);
                    {
                        std::lock_guard<std::mutex> lock(state->Mutex);
                        if (state->Site || !state->Server.expired()) return HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED);
                        phase = L"authenticated pipe creation";
                        result = server->Start();
                        if (SUCCEEDED(result))
                        {
                            state->Site = site;
                            state->Server = server;
                        }
                    }
                    return result;
                }
                catch (const std::bad_alloc&) { return E_OUTOFMEMORY; }
            }

            HRESULT STDMETHODCALLTYPE GetSite(REFIID iid, void** output) override
            {
                if (!output) return E_POINTER;
                *output = nullptr;
                ComPtr<IUnknown> site;
                {
                    std::lock_guard<std::mutex> lock(state->Mutex);
                    site = state->Site;
                }
                return site ? site->QueryInterface(iid, output) : E_FAIL;
            }
        private:
            std::shared_ptr<SiteState> state;
        };

        class Factory final : public RuntimeClass<RuntimeClassFlags<ClassicCom>, IClassFactory>
        {
        public:
            Factory() noexcept { ++ActiveObjects; }
            ~Factory() override { --ActiveObjects; }
            HRESULT STDMETHODCALLTYPE CreateInstance(IUnknown* outer, REFIID iid, void** output) override
            {
                if (!output) return E_POINTER;
                *output = nullptr;
                if (outer) return CLASS_E_NOAGGREGATION;
                try
                {
                    auto instance = Make<TapObject>();
                    return instance ? instance->QueryInterface(iid, output) : E_OUTOFMEMORY;
                }
                catch (const std::bad_alloc&) { return E_OUTOFMEMORY; }
            }
            HRESULT STDMETHODCALLTYPE LockServer(BOOL acquire) override
            {
                if (acquire) { ++ServerLocks; return S_OK; }
                ULONG count = ServerLocks.load();
                while (count)
                    if (ServerLocks.compare_exchange_weak(count, count - 1)) return S_OK;
                return E_UNEXPECTED;
            }
        };
    }
}

extern "C" HRESULT __stdcall DllGetClassObject(REFCLSID clsid, REFIID iid, void** output)
{
    if (!output) return E_POINTER;
    *output = nullptr;
    if (clsid != WinUIProfiler::Tap::ClassId) return CLASS_E_CLASSNOTAVAILABLE;
    auto factory = Microsoft::WRL::Make<WinUIProfiler::Tap::Factory>();
    return factory ? factory->QueryInterface(iid, output) : E_OUTOFMEMORY;
}

extern "C" HRESULT __stdcall DllCanUnloadNow()
{
    using namespace WinUIProfiler::Tap;
    return ActiveObjects.load() || ActiveServers.load() || ServerLocks.load() ? S_FALSE : S_OK;
}
