#include "PipeServer.h"
#include "ReplayGuard.h"
#include "TreeSnapshot.h"
#include <objbase.h>
#include <algorithm>
#include <sddl.h>
#include <array>
#include <cstdio>
#include <cstring>
#include <vector>

namespace WinUIProfiler::Tap
{
    namespace
    {
        constexpr ULONGLONG OperationMilliseconds = 30'000;
        struct LocalMemory
        {
            void* Value{};
            ~LocalMemory() { if (Value) LocalFree(Value); }
        };
        struct ThreadState
        {
            std::shared_ptr<PipeServer> Server;
            HMODULE Module;
        };

        HRESULT UserSid(HANDLE process, std::vector<uint8_t>& result)
        {
            Handle token;
            HANDLE raw{};
            if (!OpenProcessToken(process, TOKEN_QUERY, &raw)) return HRESULT_FROM_WIN32(GetLastError());
            token.Reset(raw);
            DWORD length{};
            if (GetTokenInformation(token.Get(), TokenUser, nullptr, 0, &length) ||
                GetLastError() != ERROR_INSUFFICIENT_BUFFER) return E_UNEXPECTED;
            if (length > 64 * 1024) return HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
            result.resize(length);
            if (!GetTokenInformation(token.Get(), TokenUser, result.data(), length, &length))
                return HRESULT_FROM_WIN32(GetLastError());
            return S_OK;
        }

        HRESULT CreationTime(HANDLE process, uint64_t& result) noexcept
        {
            FILETIME created{}, exited{}, kernel{}, user{};
            if (!GetProcessTimes(process, &created, &exited, &kernel, &user))
                return HRESULT_FROM_WIN32(GetLastError());
            result = (static_cast<uint64_t>(created.dwHighDateTime) << 32) | created.dwLowDateTime;
            return S_OK;
        }

        std::string FailureReason(const char* operation, HRESULT result)
        {
            char value[160]{};
            sprintf_s(value, "%s failed (HRESULT 0x%08lX).", operation, static_cast<unsigned long>(result));
            return value;
        }

        void TraceFailure(HRESULT result) noexcept
        {
            wchar_t message[100]{};
            swprintf_s(message, L"WinUIProfiler TAP session ended: HRESULT 0x%08lX.\n",
                static_cast<unsigned long>(result));
            OutputDebugStringW(message);
        }
    }

    PipeServer::PipeServer(Bootstrap input, Microsoft::WRL::ComPtr<IXamlDiagnostics> xaml,
        std::function<void(PipeServer*)> callback)
        : bootstrap(std::move(input)), diagnostics(std::move(xaml)), finished(std::move(callback)),
          stop(CreateEventW(nullptr, TRUE, FALSE, nullptr)),
          initialized(stop.Get() ? S_OK : HRESULT_FROM_WIN32(GetLastError()))
    {
    }

    HRESULT PipeServer::Start() noexcept
    {
        if (FAILED(initialized)) return initialized;
        if (pipe.Get()) return E_UNEXPECTED;
        auto self = weak_from_this().lock();
        if (!self) return E_UNEXPECTED;
        try
        {
            uint64_t created{};
            HRESULT result = CreationTime(GetCurrentProcess(), created);
            if (FAILED(result)) return result;
            if (bootstrap.Target.Pid != GetCurrentProcessId() || bootstrap.Target.CreationTime != created)
                return E_ACCESSDENIED;
            std::vector<uint8_t> identity;
            result = UserSid(GetCurrentProcess(), identity);
            if (FAILED(result)) return result;
            wchar_t* sid{};
            if (!ConvertSidToStringSidW(reinterpret_cast<TOKEN_USER*>(identity.data())->User.Sid, &sid))
                return HRESULT_FROM_WIN32(GetLastError());
            const LocalMemory sidMemory{sid};
            std::wstring security = L"D:P(A;;GA;;;";
            security += sid;
            security += L")";
            PSECURITY_DESCRIPTOR descriptor{};
            if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(security.c_str(), SDDL_REVISION_1,
                &descriptor, nullptr)) return HRESULT_FROM_WIN32(GetLastError());
            const LocalMemory securityMemory{descriptor};
            SECURITY_ATTRIBUTES attributes{sizeof(attributes), descriptor, FALSE};
            const auto name = L"\\\\.\\pipe\\" + bootstrap.PipeName;
            const HANDLE raw = CreateNamedPipeW(name.c_str(),
                PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED | FILE_FLAG_FIRST_PIPE_INSTANCE,
                PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS, 1,
                64 * 1024, 64 * 1024, static_cast<DWORD>(OperationMilliseconds), &attributes);
            const DWORD error = GetLastError();
            if (raw == INVALID_HANDLE_VALUE) return HRESULT_FROM_WIN32(error);
            pipe.Reset(raw);
            auto state = std::make_unique<ThreadState>();
            state->Server = self;
            if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                reinterpret_cast<LPCWSTR>(&PipeServer::ThreadMain), &state->Module))
                return HRESULT_FROM_WIN32(GetLastError());
            ++ActiveServers;
            const HANDLE thread = CreateThread(nullptr, 0, &PipeServer::ThreadMain, state.get(), 0, nullptr);
            if (!thread)
            {
                const DWORD threadError = GetLastError();
                --ActiveServers;
                FreeLibrary(state->Module);
                pipe.Reset();
                return HRESULT_FROM_WIN32(threadError);
            }
            state.release();
            CloseHandle(thread);
            return S_OK;
        }
        catch (const std::bad_alloc&) { return E_OUTOFMEMORY; }
    }

    void PipeServer::Stop() noexcept
    {
        SetEvent(stop.Get());
    }

    HRESULT PipeServer::Wait(OVERLAPPED& operation, ULONGLONG deadline, DWORD& transferred) noexcept
    {
        const auto now = GetTickCount64();
        const DWORD remaining = now >= deadline ? 0 : static_cast<DWORD>(deadline - now);
        const HANDLE events[]{stop.Get(), operation.hEvent};
        const DWORD outcome = WaitForMultipleObjects(2, events, FALSE, remaining);
        if (outcome == WAIT_OBJECT_0 + 1)
            return GetOverlappedResult(pipe.Get(), &operation, &transferred, FALSE) ?
                S_OK : HRESULT_FROM_WIN32(GetLastError());
        const DWORD error = outcome == WAIT_OBJECT_0 ? ERROR_CANCELLED :
            outcome == WAIT_TIMEOUT ? ERROR_TIMEOUT : GetLastError();
        CancelIoEx(pipe.Get(), &operation);
        GetOverlappedResult(pipe.Get(), &operation, &transferred, TRUE);
        return HRESULT_FROM_WIN32(error);
    }

    HRESULT PipeServer::Transfer(void* buffer, DWORD length, bool write, ULONGLONG deadline) noexcept
    {
        auto bytes = static_cast<uint8_t*>(buffer);
        while (length)
        {
            if (WaitForSingleObject(stop.Get(), 0) == WAIT_OBJECT_0)
                return HRESULT_FROM_WIN32(ERROR_CANCELLED);
            if (GetTickCount64() >= deadline) return HRESULT_FROM_WIN32(ERROR_TIMEOUT);
            Handle event(CreateEventW(nullptr, TRUE, FALSE, nullptr));
            if (!event.Get()) return HRESULT_FROM_WIN32(GetLastError());
            OVERLAPPED operation{};
            operation.hEvent = event.Get();
            DWORD transferred{};
            const BOOL immediate = write ? WriteFile(pipe.Get(), bytes, length, &transferred, &operation) :
                ReadFile(pipe.Get(), bytes, length, &transferred, &operation);
            if (!immediate)
            {
                const DWORD error = GetLastError();
                if (error != ERROR_IO_PENDING) return HRESULT_FROM_WIN32(error);
                const HRESULT result = Wait(operation, deadline, transferred);
                if (FAILED(result)) return result;
            }
            if (!transferred) return HRESULT_FROM_WIN32(ERROR_BROKEN_PIPE);
            bytes += transferred;
            length -= transferred;
        }
        return S_OK;
    }

    HRESULT PipeServer::VerifyClient() noexcept
    {
        ULONG pid{};
        if (!GetNamedPipeClientProcessId(pipe.Get(), &pid)) return HRESULT_FROM_WIN32(GetLastError());
        if (pid != bootstrap.Client.Pid) return E_ACCESSDENIED;
        Handle client(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid));
        if (!client.Get()) return HRESULT_FROM_WIN32(GetLastError());
        uint64_t created{};
        HRESULT result = CreationTime(client.Get(), created);
        if (FAILED(result)) return result;
        if (created != bootstrap.Client.CreationTime) return E_ACCESSDENIED;
        try
        {
            std::vector<uint8_t> current, other;
            result = UserSid(GetCurrentProcess(), current);
            if (FAILED(result)) return result;
            result = UserSid(client.Get(), other);
            if (FAILED(result)) return result;
            return EqualSid(reinterpret_cast<TOKEN_USER*>(current.data())->User.Sid,
                reinterpret_cast<TOKEN_USER*>(other.data())->User.Sid) ? S_OK : E_ACCESSDENIED;
        }
        catch (const std::bad_alloc&) { return E_OUTOFMEMORY; }
    }

    HRESULT PipeServer::Serve()
    {
        Handle connected(CreateEventW(nullptr, TRUE, FALSE, nullptr));
        if (!connected.Get()) return HRESULT_FROM_WIN32(GetLastError());
        OVERLAPPED connection{};
        connection.hEvent = connected.Get();
        if (!ConnectNamedPipe(pipe.Get(), &connection))
        {
            const DWORD error = GetLastError();
            if (error == ERROR_IO_PENDING)
            {
                DWORD count{};
                const HRESULT result = Wait(connection, GetTickCount64() + OperationMilliseconds, count);
                if (FAILED(result)) return result;
            }
            else if (error != ERROR_PIPE_CONNECTED) return HRESULT_FROM_WIN32(error);
        }
        HRESULT result = VerifyClient();
        if (FAILED(result)) return result;
        bool authenticated = false;
        ReplayGuard requests;
        for (;;)
        {
            const auto deadline = GetTickCount64() + OperationMilliseconds;
            std::vector<uint8_t> frame(28);
            result = Transfer(frame.data(), 28, false, deadline);
            if (FAILED(result)) return result;
            Header header{};
            result = DecodeHeader(frame.data(), frame.size(), header);
            if (FAILED(result)) return result;
            frame.resize(28 + header.BodyLength);
            result = Transfer(frame.data() + 28, header.BodyLength, false, deadline);
            if (FAILED(result)) return result;
            Command command{};
            result = DecodeCommand(frame.data(), frame.size(), bootstrap, authenticated, command);
            if (SUCCEEDED(result) && !requests.Insert(header.RequestId))
                result = HRESULT_FROM_WIN32(ERROR_INVALID_STATE);
            std::vector<uint8_t> response;
            bool disconnect = false;
            if (FAILED(result))
            {
                response = EncodeAcknowledgement(header.RequestId, CommandStatus::Rejected, GUID_NULL,
                    FailureReason("TAP authentication/command validation", result));
                disconnect = true;
            }
            else if (!authenticated)
            {
                authenticated = true;
                response = EncodeAcknowledgement(header.RequestId, CommandStatus::Completed, GUID_NULL, "");
            }
            else if (header.Type == MessageType::Hello)
            {
                response = EncodeHello(header.RequestId);
            }
            else if (header.Type == MessageType::Capabilities)
            {
                response = EncodeCapabilities(header.RequestId);
            }
            else if (header.Type == MessageType::TakeSnapshot)
            {
                TreeSnapshotResult snapshot;
                std::string failure;
                result = CaptureTreeSnapshot(diagnostics.Get(), stop.Get(), command.CaptureId,
                    bootstrap.Target.CreationTime, snapshot, failure);
                if (FAILED(result))
                {
                    response = EncodeAcknowledgement(header.RequestId,
                        result == E_NOINTERFACE ? CommandStatus::Unsupported :
                        result == HRESULT_FROM_WIN32(ERROR_CANCELLED) ? CommandStatus::Cancelled : CommandStatus::Failed,
                        command.CaptureId, failure.empty() ? FailureReason("TAP tree snapshot", result) : failure);
                }
                else
                {
                    if (snapshot.Json.empty() || snapshot.Json.size() > MaximumSnapshotBytes)
                        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                    uint32_t index{};
                    for (size_t offset = 0; offset < snapshot.Json.size(); offset += SnapshotChunkBytes)
                    {
                        auto chunk = EncodeSnapshotChunk(header.RequestId, command.CaptureId, index++,
                            snapshot.Json.data() + offset, std::min(SnapshotChunkBytes, snapshot.Json.size() - offset));
                        result = Transfer(chunk.data(), static_cast<DWORD>(chunk.size()), true,
                            GetTickCount64() + OperationMilliseconds);
                        if (FAILED(result)) return result;
                    }
                    response = EncodeSnapshotResult(header.RequestId, command.CaptureId, index, snapshot.Json.size());
                }
            }
            else if (header.Type == MessageType::Disconnect)
            {
                response = EncodeAcknowledgement(header.RequestId, CommandStatus::Completed, GUID_NULL, "");
                disconnect = true;
            }
            else
            {
                response = EncodeAcknowledgement(header.RequestId, CommandStatus::Unsupported, command.CaptureId,
                    "This TAP supports transport, hello and tree snapshots only; this command is unsupported.");
            }
            result = Transfer(response.data(), static_cast<DWORD>(response.size()), true,
                GetTickCount64() + OperationMilliseconds);
            if (FAILED(result)) return result;
            if (disconnect)
            {
                // DisconnectNamedPipe discards unread responses. Bound the wait for
                // the client to consume its terminal response and close instead.
                uint8_t unexpected{};
                result = Transfer(&unexpected, 1, false, GetTickCount64() + OperationMilliseconds);
                return result == HRESULT_FROM_WIN32(ERROR_BROKEN_PIPE) ||
                    result == HRESULT_FROM_WIN32(ERROR_NO_DATA) ? S_OK : FAILED(result) ? result : E_INVALIDARG;
            }
        }
    }

    void PipeServer::Run() noexcept
    {
        const HRESULT apartment = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        HRESULT result = apartment;
        if (SUCCEEDED(apartment))
        {
            try { result = Serve(); }
            catch (const std::bad_alloc&) { result = E_OUTOFMEMORY; }
        }
        if (FAILED(result)) TraceFailure(result);
        DisconnectNamedPipe(pipe.Get());
        pipe.Reset();
        diagnostics.Reset();
        if (finished) finished(this);
        finished = {};
        if (SUCCEEDED(apartment)) CoUninitialize();
    }

    DWORD WINAPI PipeServer::ThreadMain(void* raw) noexcept
    {
        std::unique_ptr<ThreadState> state(static_cast<ThreadState*>(raw));
        const HMODULE module = state->Module;
        auto self = std::move(state->Server);
        state.reset();
        self->Run();
        self.reset();
        --ActiveServers;
        FreeLibraryAndExitThread(module, 0);
    }
}
