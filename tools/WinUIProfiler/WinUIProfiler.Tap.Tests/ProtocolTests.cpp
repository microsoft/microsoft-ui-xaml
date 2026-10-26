#include "..\WinUIProfiler.Tap\Protocol.h"
#include "..\WinUIProfiler.Tap\ReplayGuard.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <stdexcept>

using namespace WinUIProfiler::Tap;
unsigned RunPipeServerChecks();

namespace
{
    unsigned checks{};
    void Check(bool value, const char* name)
    {
        ++checks;
        if (!value) throw std::runtime_error(name);
    }
    const GUID Identity{0x01234567, 0x89ab, 0xcdef, {0x80, 1, 2, 3, 4, 5, 6, 7}};
    std::string Body(const std::vector<uint8_t>& frame)
    {
        return {frame.begin() + 28, frame.end()};
    }
    std::vector<uint8_t> Request(MessageType type, const std::string& body)
    {
        auto result = EncodeHello(Identity);
        result.resize(28 + body.size());
        const auto length = static_cast<uint32_t>(body.size());
        std::memcpy(result.data(), &length, sizeof(length));
        const auto code = static_cast<uint16_t>(type);
        std::memcpy(result.data() + 6, &code, sizeof(code));
        std::copy(body.begin(), body.end(), result.begin() + 28);
        return result;
    }
    Bootstrap Sample()
    {
        Bootstrap result;
        result.PipeName = L"WinUIProfiler-0123456789abcdef8001020304050607";
        result.Session = Identity;
        result.Target = {123, 987654321, Identity};
        result.Client = {456, 987654322, Identity};
        return result;
    }
    void ProtocolChecks()
    {
        const auto expected = Sample();
        const auto compact = EncodeBootstrap(expected);
        Check(compact.size() == 165 && compact.substr(0, 5) == L"WUP2|", "compact bootstrap");
        Bootstrap decoded;
        Check(SUCCEEDED(ParseBootstrap(compact.c_str(), decoded)), "parse bootstrap");
        Check(decoded.PipeName == expected.PipeName && decoded.Session == expected.Session &&
            decoded.Target.Pid == 123 && decoded.Target.CreationTime == 987654321 &&
            decoded.Client.Pid == 456 && decoded.Nonce == expected.Nonce, "bootstrap identity");
        Check(FAILED(ParseBootstrap((compact + L"x").c_str(), decoded)), "oversized bootstrap");
        auto hello = EncodeHello(Identity);
        Header header;
        Check(SUCCEEDED(DecodeHeader(hello.data(), 28, header)) && header.Type == MessageType::Hello &&
            header.Version == 1 && header.SchemaVersion == 1 && header.RequestId == Identity, "28-byte header");
        Check(Body(hello) == R"({"message":"hello"})", "hello JSON");
        Command command;
        Check(DecodeCommand(hello.data(), hello.size(), expected, false, command) == E_ACCESSDENIED,
            "hello requires authentication");
        Check(SUCCEEDED(DecodeCommand(hello.data(), hello.size(), expected, true, command)), "authenticated hello");
        hello[24] = 2;
        Check(FAILED(DecodeCommand(hello.data(), hello.size(), expected, true, command)), "schema mismatch");
        hello = Request(MessageType::Hello, R"({"message":"hello","message":"hello"})");
        Check(FAILED(DecodeCommand(hello.data(), hello.size(), expected, true, command)), "duplicate JSON key");
        const std::string handshake = R"({"sessionId":"01234567-89ab-cdef-8001-020304050607","process":{"processId":123,"creationTimeFileTime":987654321,"instanceId":"01234567-89ab-cdef-8001-020304050607"},"schemaVersion":1,"nonceBase64":"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA="})";
        auto frame = Request(MessageType::Handshake, handshake);
        Check(SUCCEEDED(DecodeCommand(frame.data(), frame.size(), expected, false, command)), "handshake");
        Check(DecodeCommand(frame.data(), frame.size(), expected, true, command) == E_ACCESSDENIED, "handshake replay");
        auto wrongNonce = expected;
        wrongNonce.Nonce[31] = 1;
        Check(DecodeCommand(frame.data(), frame.size(), wrongNonce, false, command) == E_ACCESSDENIED, "nonce mismatch");
        auto wrongProcess = expected;
        ++wrongProcess.Target.CreationTime;
        Check(DecodeCommand(frame.data(), frame.size(), wrongProcess, false, command) == E_ACCESSDENIED, "PID reuse");
        frame = Request(MessageType::Disconnect, R"({"reason":"done"})");
        Check(SUCCEEDED(DecodeCommand(frame.data(), frame.size(), expected, true, command)), "disconnect");
        const auto acknowledgement = EncodeAcknowledgement(Identity, CommandStatus::Unsupported, GUID{}, "Transport only.");
        Check(Body(acknowledgement).find("\"status\":\"Unsupported\"") != std::string::npos, "unsupported status");
        const auto capabilities = EncodeCapabilities(Identity, GUID{});
        Check(Body(capabilities) == R"({"epoch":null,"schemaVersion":1,"capabilities":[]})", "retail transport capabilities");
        bool rejectedRuntime = false;
        try { (void)EncodeCapabilities(Identity, Identity, true); }
        catch (const std::invalid_argument&) { rejectedRuntime = true; }
        Check(rejectedRuntime, "cannot advertise a private runtime");
        ReplayGuard requests;
        Check(requests.Insert(Identity), "first request");
        Check(!requests.Insert(Identity), "duplicate request");
        for (uint32_t index = 0; index < 100000; ++index)
        {
            auto request = Identity;
            request.Data1 = index;
            Check(requests.Insert(request), "no short request lifetime cap");
        }
        Check(!requests.Insert(Identity), "old replay remains rejected after 100000 requests");
    }
}

int main()
{
    try
    {
        ProtocolChecks();
        checks += RunPipeServerChecks();
        std::printf("PASS: %u native protocol checks.\n", checks);
        return 0;
    }
    catch (const std::exception& error)
    {
        std::fprintf(stderr, "FAIL after %u checks: %s\n", checks, error.what());
        return 1;
    }
}
