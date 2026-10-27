#pragma once
#include "Protocol.h"
#include "Tap.h"
#include <functional>
#include <memory>
#include <wrl.h>
#include <xamlom.h>

namespace WinUIProfiler::Tap
{
    class PipeServer final : public std::enable_shared_from_this<PipeServer>
    {
    public:
        PipeServer(Bootstrap bootstrap, Microsoft::WRL::ComPtr<IXamlDiagnostics> diagnostics,
            std::function<void(PipeServer*)> finished);
        HRESULT Start() noexcept;
        void Stop() noexcept;
    private:
        void Run() noexcept;
        static DWORD WINAPI ThreadMain(void* state) noexcept;
        HRESULT Serve();
        HRESULT Transfer(void* bytes, DWORD length, bool write, ULONGLONG deadline) noexcept;
        HRESULT Wait(OVERLAPPED& operation, ULONGLONG deadline, DWORD& transferred) noexcept;
        HRESULT VerifyClient() noexcept;
        Bootstrap bootstrap;
        Microsoft::WRL::ComPtr<IXamlDiagnostics> diagnostics;
        std::function<void(PipeServer*)> finished;
        Handle stop, pipe;
        HRESULT initialized;
    };
}
