#pragma once
#include "Protocol.h"
#include "Tap.h"
#include <WinUIProfilerProtocol.h>
#include <wrl.h>
#include <xamlom.h>
#include <functional>
#include <memory>

namespace WinUIProfiler::Tap
{
    class PipeServer final : public std::enable_shared_from_this<PipeServer>
    {
    public:
        PipeServer(Bootstrap bootstrap, Microsoft::WRL::ComPtr<IWinUIProfilerRuntimeDiagnostics> runtime,
            std::function<void(PipeServer*)> finished,
            Microsoft::WRL::ComPtr<IXamlDiagnostics> xamlDiagnostics = {});
        HRESULT Start() noexcept;
        void Stop() noexcept;
    private:
        void Run() noexcept;
        static DWORD WINAPI ThreadMain(void* state) noexcept;
        HRESULT Serve();
        HRESULT Transfer(void* bytes, DWORD length, bool write, ULONGLONG deadline) noexcept;
        HRESULT Wait(OVERLAPPED& operation, ULONGLONG deadline, DWORD& transferred) noexcept;
        HRESULT VerifyClient() noexcept;
        HRESULT RuntimeInfo(ProtocolInfo& info) noexcept;
        Bootstrap bootstrap;
        Microsoft::WRL::ComPtr<IWinUIProfilerRuntimeDiagnostics> runtime;
        Microsoft::WRL::ComPtr<IXamlDiagnostics> xamlDiagnostics;
        std::function<void(PipeServer*)> finished;
        Handle stop, pipe;
        HRESULT initialized;
    };
}
