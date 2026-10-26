#include "..\WinUIProfiler.Tap\PipeServer.h"
#include <objbase.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>

using namespace WinUIProfiler::Tap;

namespace
{
    unsigned checks{};
    void Check(bool condition, const char* name)
    {
        ++checks;
        if (!condition) throw std::runtime_error(name);
    }
    GUID NewId()
    {
        GUID result{};
        Check(SUCCEEDED(CoCreateGuid(&result)), "create test identity");
        return result;
    }
    std::string Text(GUID id)
    {
        wchar_t text[40]{};
        Check(StringFromGUID2(id, text, ARRAYSIZE(text)) == 39, "format test identity");
        std::string result;
        for (size_t i = 1; i < 37; ++i) result += static_cast<char>(text[i]);
        return result;
    }
    Bootstrap Sample()
    {
        Bootstrap result;
        result.Session = NewId();
        result.Target.Pid = result.Client.Pid = GetCurrentProcessId();
        result.Target.Instance = NewId();
        result.Client.Instance = NewId();
        FILETIME creation{}, exit{}, kernel{}, user{};
        Check(GetProcessTimes(GetCurrentProcess(), &creation, &exit, &kernel, &user) != FALSE,
            "test process creation time");
        result.Target.CreationTime = result.Client.CreationTime =
            (static_cast<uint64_t>(creation.dwHighDateTime) << 32) | creation.dwLowDateTime;
        auto pipe = Text(NewId());
        pipe.erase(std::remove(pipe.begin(), pipe.end(), '-'), pipe.end());
        result.PipeName = L"WinUIProfiler-" + std::wstring(pipe.begin(), pipe.end());
        return result;
    }
    class Session final
    {
    public:
        explicit Session(Bootstrap bootstrap)
            : identity(std::move(bootstrap)), ended(std::make_shared<Handle>(CreateEventW(nullptr, TRUE, FALSE, nullptr))),
              server(std::make_shared<PipeServer>(identity, [event = ended](PipeServer*) { SetEvent(event->Get()); }))
        {
            Check(ended->Get() != nullptr, "session completion event");
        }
        ~Session()
        {
            server->Stop();
            if (started) WaitForSingleObject(ended->Get(), 5000);
        }
        void Start()
        {
            Check(SUCCEEDED(server->Start()), "start authenticated server");
            started = true;
        }
        void Connect()
        {
            const auto name = L"\\\\.\\pipe\\" + identity.PipeName;
            client.Reset(CreateFileW(name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr));
            Check(client.Get() != INVALID_HANDLE_VALUE, "connect local pipe");
            ULONG pid{};
            Check(GetNamedPipeServerProcessId(client.Get(), &pid) && pid == identity.Target.Pid,
                "client checks server PID ownership");
        }
        HRESULT Transfer(void* bytes, DWORD size, bool write)
        {
            auto cursor = static_cast<uint8_t*>(bytes);
            const auto deadline = GetTickCount64() + 5000;
            while (size)
            {
                Handle event(CreateEventW(nullptr, TRUE, FALSE, nullptr));
                if (!event.Get()) return HRESULT_FROM_WIN32(GetLastError());
                OVERLAPPED operation{};
                operation.hEvent = event.Get();
                DWORD transferred{};
                const BOOL immediate = write ?
                    WriteFile(client.Get(), cursor, size, &transferred, &operation) :
                    ReadFile(client.Get(), cursor, size, &transferred, &operation);
                if (!immediate)
                {
                    const auto error = GetLastError();
                    if (error != ERROR_IO_PENDING) return HRESULT_FROM_WIN32(error);
                    const auto now = GetTickCount64();
                    if (WaitForSingleObject(event.Get(), now >= deadline ? 0 :
                        static_cast<DWORD>(deadline - now)) != WAIT_OBJECT_0)
                    {
                        CancelIoEx(client.Get(), &operation);
                        GetOverlappedResult(client.Get(), &operation, &transferred, TRUE);
                        return HRESULT_FROM_WIN32(ERROR_TIMEOUT);
                    }
                    if (!GetOverlappedResult(client.Get(), &operation, &transferred, FALSE))
                        return HRESULT_FROM_WIN32(GetLastError());
                }
                if (!transferred) return HRESULT_FROM_WIN32(ERROR_BROKEN_PIPE);
                size -= transferred;
                cursor += transferred;
            }
            return S_OK;
        }
        void Send(MessageType type, GUID request, const std::string& body)
        {
            auto frame = EncodeHello(request);
            frame.resize(28 + body.size());
            const auto length = static_cast<uint32_t>(body.size());
            std::memcpy(frame.data(), &length, sizeof(length));
            const auto code = static_cast<uint16_t>(type);
            std::memcpy(frame.data() + 6, &code, sizeof(code));
            std::copy(body.begin(), body.end(), frame.begin() + 28);
            Check(SUCCEEDED(Transfer(frame.data(), static_cast<DWORD>(frame.size()), true)), "send request");
        }
        std::string Read(MessageType expected, GUID request)
        {
            std::array<uint8_t, 28> bytes{};
            Check(SUCCEEDED(Transfer(bytes.data(), static_cast<DWORD>(bytes.size()), false)), "read response header");
            Header header;
            Check(SUCCEEDED(DecodeHeader(bytes.data(), bytes.size(), header)) &&
                header.Type == expected && header.RequestId == request, "response framing and correlation");
            std::string body(header.BodyLength, '\0');
            Check(SUCCEEDED(Transfer(body.data(), header.BodyLength, false)), "read response body");
            return body;
        }
        void Handshake()
        {
            const auto request = NewId();
            Send(MessageType::Handshake, request, "{\"sessionId\":\"" + Text(identity.Session) +
                "\",\"process\":{\"processId\":" + std::to_string(identity.Target.Pid) +
                ",\"creationTimeFileTime\":" + std::to_string(identity.Target.CreationTime) +
                ",\"instanceId\":\"" + Text(identity.Target.Instance) +
                "\"},\"schemaVersion\":1,\"nonceBase64\":\"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=\"}");
            Check(Read(MessageType::Acknowledgement, request).find("\"status\":\"Completed\"") !=
                std::string::npos, "authenticated handshake completed");
        }
        void Closed()
        {
            client.Reset();
            Check(WaitForSingleObject(ended->Get(), 5000) == WAIT_OBJECT_0, "server shuts down promptly");
        }
        Bootstrap identity;
        std::shared_ptr<Handle> ended;
        std::shared_ptr<PipeServer> server;
        Handle client;
        bool started{};
    };
    void HappySession()
    {
        Session session(Sample());
        session.Start();
        session.Connect();
        session.Handshake();
        auto request = NewId();
        for (uint32_t i = 0; i < 5000; ++i)
        {
            request.Data1 = i;
            session.Send(MessageType::Hello, request, R"({"message":"hello"})");
            Check(session.Read(MessageType::Hello, request) == R"({"message":"hello"})", "live hello past 4096 requests");
        }
        request = NewId();
        session.Send(MessageType::Capabilities, request, R"({"schemaVersion":1})");
        Check(session.Read(MessageType::CapabilitiesResult, request) ==
            R"({"epoch":null,"schemaVersion":1,"capabilities":[]})", "no-runtime capabilities");
        const auto capture = Text(NewId());
        for (const auto type : {MessageType::Rundown, MessageType::LightweightSnapshot,
            MessageType::OwnershipGraph, MessageType::Cancel, MessageType::TakeSnapshot})
        {
            request = NewId();
            std::string body;
            if (type == MessageType::Cancel) body = "{\"targetRequestId\":\"" + capture + "\"}";
            else
            {
                body = "{\"captureId\":\"" + capture + "\"";
                if (type == MessageType::Rundown || type == MessageType::LightweightSnapshot)
                    body += ",\"rootId\":null,\"dispatcherId\":null";
                if (type == MessageType::OwnershipGraph) body += ",\"maximumObjects\":1,\"maximumEdges\":1";
                body += "}";
            }
            session.Send(type, request, body);
            Check(session.Read(MessageType::Acknowledgement, request).find("\"status\":\"Unsupported\"") !=
                std::string::npos, "unimplemented commands explicitly unsupported");
        }
        request = NewId();
        session.Send(MessageType::Disconnect, request, R"({"reason":"done"})");
        Check(session.Read(MessageType::Acknowledgement, request).find("\"status\":\"Completed\"") !=
            std::string::npos, "disconnect acknowledgement delivered before close");
        session.Closed();
    }
    void ReplaySession()
    {
        Session session(Sample());
        session.Start();
        session.Connect();
        session.Handshake();
        const auto request = NewId();
        session.Send(MessageType::Hello, request, R"({"message":"hello"})");
        (void)session.Read(MessageType::Hello, request);
        session.Send(MessageType::Hello, request, R"({"message":"hello"})");
        Check(session.Read(MessageType::Acknowledgement, request).find("\"status\":\"Rejected\"") !=
            std::string::npos, "duplicate request rejected");
        session.Closed();
    }
    void OwnershipSession(bool incorrectPid)
    {
        auto identity = Sample();
        if (incorrectPid) ++identity.Client.Pid;
        else ++identity.Client.CreationTime;
        Session session(identity);
        session.Start();
        session.Connect();
        uint8_t byte{};
        Check(FAILED(session.Transfer(&byte, 1, false)), "wrong client PID or creation time rejected");
        session.Closed();
    }
    void BadNonceSession()
    {
        auto identity = Sample();
        identity.Nonce[0] = 1;
        Session session(identity);
        session.Start();
        session.Connect();
        const auto request = NewId();
        session.Send(MessageType::Handshake, request, "{\"sessionId\":\"" + Text(identity.Session) +
            "\",\"process\":{\"processId\":" + std::to_string(identity.Target.Pid) +
            ",\"creationTimeFileTime\":" + std::to_string(identity.Target.CreationTime) +
            ",\"instanceId\":\"" + Text(identity.Target.Instance) +
            "\"},\"schemaVersion\":1,\"nonceBase64\":\"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=\"}");
        Check(session.Read(MessageType::Acknowledgement, request).find("\"status\":\"Rejected\"") !=
            std::string::npos, "bad nonce rejected by live server");
        session.Closed();
    }
    void CancellationSession(bool partialFrame)
    {
        Session session(Sample());
        session.Start();
        if (partialFrame)
        {
            session.Connect();
            auto header = EncodeHello(NewId());
            Check(SUCCEEDED(session.Transfer(header.data(), 28, true)), "send partial frame");
        }
        session.server->Stop();
        Check(WaitForSingleObject(session.ended->Get(), 5000) == WAIT_OBJECT_0,
            "cancel pending connection or partial read");
        session.Closed();
    }
}

unsigned RunPipeServerChecks()
{
    HappySession();
    ReplaySession();
    OwnershipSession(true);
    OwnershipSession(false);
    BadNonceSession();
    CancellationSession(false);
    CancellationSession(true);
    auto identity = Sample();
    ++identity.Target.CreationTime;
    Session wrongTarget(identity);
    Check(wrongTarget.server->Start() == E_ACCESSDENIED, "server validates target creation time");
    std::printf("PASS: %u live named-pipe checks (5000 hello requests, rejection, disconnect, cancellation).\n", checks);
    return checks;
}
