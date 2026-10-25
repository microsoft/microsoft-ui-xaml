#include "..\WinUIProfiler.Tap\Protocol.h"
#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <new>
#include <stdexcept>
#include <string_view>

bool FailNextAllocation{};
void* operator new(size_t size)
{
    if (FailNextAllocation) { FailNextAllocation = false; throw std::bad_alloc(); }
    if (auto memory = std::malloc(size ? size : 1)) return memory;
    throw std::bad_alloc();
}
void operator delete(void* memory) noexcept { std::free(memory); }
void operator delete(void* memory, size_t) noexcept { std::free(memory); }

using namespace WinUIProfiler::Tap;
namespace
{
int Checks{}, Failures{};
void Check(bool condition, const char* description)
{
    ++Checks;
    if (!condition) { ++Failures; std::printf("FAIL: %s\n", description); }
}
const GUID Id{0x00112233, 0x4455, 0x6677, {0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff}};
const char* IdText = "00112233-4455-6677-8899-aabbccddeeff";
const wchar_t* Init = L"WUP1|WinUIProfiler-00112233445566778899aabbccddeeff|00112233-4455-6677-8899-aabbccddeeff|42|9007199254740993|00112233-4455-6677-8899-aabbccddeeff|43|9223372036854775807|00112233-4455-6677-8899-aabbccddeeff|000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f";
const char* Handshake = R"({"sessionId":"00112233-4455-6677-8899-aabbccddeeff","process":{"processId":42,"creationTimeFileTime":9007199254740993,"instanceId":"00112233-4455-6677-8899-aabbccddeeff"},"schemaVersion":1,"nonceBase64":"AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8="})";
std::string Replace(std::string text, const std::string& from, const std::string& to)
{
    const auto at = text.find(from);
    if (at == std::string::npos) throw std::runtime_error("Bad test replacement");
    text.replace(at, from.size(), to);
    return text;
}
void Put(std::vector<uint8_t>& frame, size_t at, uint64_t value, size_t count)
{
    for (size_t i = 0; i < count; ++i) frame[at + i] = static_cast<uint8_t>(value >> (i * 8));
}
std::vector<uint8_t> Frame(MessageType type, const std::string& body)
{
    std::vector<uint8_t> frame(28 + body.size());
    Put(frame, 0, body.size(), 4); Put(frame, 4, 1, 2);
    Put(frame, 6, static_cast<uint16_t>(type), 2); Put(frame, 24, 1, 4);
    const uint8_t guid[]{0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff};
    std::copy(std::begin(guid), std::end(guid), frame.begin() + 8);
    std::copy(body.begin(), body.end(), frame.begin() + 28);
    return frame;
}
HRESULT Decode(MessageType type, const std::string& body, const Bootstrap& expected,
    bool auth = true, Command* result = nullptr)
{
    const auto frame = Frame(type, body);
    Command local{};
    return DecodeCommand(frame.data(), frame.size(), expected, auth, result ? *result : local);
}
std::string Body(const std::vector<uint8_t>& frame)
{
    if (frame.size() < 28) return {};
    return {frame.begin() + 28, frame.end()};
}
template<typename F> void Throws(F action, const char* description)
{
    bool threw{};
    try { action(); } catch (const std::invalid_argument&) { threw = true; }
    Check(threw, description);
}
void BootstrapTests(Bootstrap& expected)
{
    Check(ParseBootstrap(Init, expected) == S_OK, "valid bounded bootstrap");
    Check(expected.Target.Pid == 42 && expected.Target.CreationTime == 9007199254740993ull &&
        expected.Client.Pid == 43 && expected.Client.CreationTime == INT64_MAX &&
        expected.Nonce[31] == 31 && expected.Target.Instance == Id, "bootstrap preserves precise process identities");
    Bootstrap output{};
    Check(ParseBootstrap(nullptr, output) == E_INVALIDARG, "null bootstrap");
    Check(ParseBootstrap(std::wstring(1025, L'x').c_str(), output) == E_INVALIDARG, "bootstrap length bound");
    for (const auto& pair : std::vector<std::pair<std::wstring, std::wstring>>{
        {L"WUP1",L"WUP2"}, {L"WinUIProfiler-",L"\\\\.\\pipe\\WinUIProfiler-"},
        {L"WinUIProfiler-",L"//host/WinUIProfiler-"}, {L"WinUIProfiler-",L"winuiProfiler-"},
        {L"|42|",L"|0|"}, {L"|42|",L"|-1|"}, {L"|42|",L"|4294967296|"},
        {L"|42|",L"|+42|"}, {L"|42|",L"| 42|"},
        {L"|9007199254740993|",L"|0|"}, {L"|9007199254740993|",L"|9223372036854775808|"},
        {L"|43|",L"|0|"}, {L"|9223372036854775807|",L"|0|"},
        {L"00112233-4455-6677-8899-aabbccddeeff",L"00000000-0000-0000-0000-000000000000"},
        {L"00112233-4455-6677-8899-aabbccddeeff",L"{00112233-4455-6677-8899-aabbccddeeff}"},
        {L"00112233445566778899aabbccddeeff",L"00112233445566778899aabbccddeefg"},
        {L"1c1d1e1f",L"1c1d1e1"}, {L"1c1d1e1f",L"1c1d1e1g"}})
    {
        std::wstring text(Init);
        text.replace(text.find(pair.first), pair.first.size(), pair.second);
        Check(ParseBootstrap(text.c_str(), output) == E_INVALIDARG, "invalid bootstrap field");
    }
    Check(ParseBootstrap((std::wstring(Init) + L"|extra").c_str(), output) == E_INVALIDARG, "extra bootstrap field");
    Check(ParseBootstrap((std::wstring(Init) + L"\n").c_str(), output) == E_INVALIDARG, "bootstrap is one line");
    std::wstring missing(Init); missing.resize(missing.rfind(L'|'));
    Check(ParseBootstrap(missing.c_str(), output) == E_INVALIDARG, "missing bootstrap field");
}
bool SameBootstrap(const Bootstrap& left, const Bootstrap& right)
{
    return left.PipeName == right.PipeName && left.Session == right.Session &&
        left.Target.Pid == right.Target.Pid && left.Target.CreationTime == right.Target.CreationTime &&
        left.Target.Instance == right.Target.Instance && left.Client.Pid == right.Client.Pid &&
        left.Client.CreationTime == right.Client.CreationTime && left.Client.Instance == right.Client.Instance &&
        left.Nonce == right.Nonce;
}
std::wstring CompactText(const std::vector<uint8_t>& bytes)
{
    constexpr wchar_t alphabet[] = L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::wstring result = L"WUP2|";
    for (size_t i = 0; i < bytes.size(); i += 3)
    {
        const uint32_t block = (uint32_t(bytes[i]) << 16) | (uint32_t(bytes[i + 1]) << 8) | bytes[i + 2];
        for (size_t shift : {size_t(18), size_t(12), size_t(6), size_t(0)}) result += alphabet[(block >> shift) & 63];
    }
    return result;
}
void CompactBootstrapTests()
{
    Bootstrap expected{};
    Check(ParseBootstrap(Init, expected) == S_OK, "legacy WUP1 remains available");
    expected.Session = {0x10213243,0x5465,0x7687,{0x98,0xa9,0xba,0xcb,0xdc,0xed,0xfe,0x0f}};
    expected.Target.Pid = 0x12345678;
    expected.Target.Instance = {0xffeeddcc,0xbbaa,0x9988,{0x77,0x66,0x55,0x44,0x33,0x22,0x11,0x00}};
    expected.Client.Pid = 0x9abcdef0;
    expected.Client.Instance = {0x89abcdef,0x0123,0x4567,{0x89,0xab,0xcd,0xef,0x01,0x23,0x45,0x67}};
    const std::wstring golden = L"WUP2|ABEiM0RVZneImaq7zN3u/xAhMkNUZXaHmKm6y9zt/g94VjQSAQAAAAAAIAD/7t3Mu6qZiHdmVUQzIhEA8N68mv////////9/iavN7wEjRWeJq83vASNFZwABAgMEBQYHCAkKCwwNDg8QERITFBUWFxgZGhscHR4f";
    const auto encoded = EncodeBootstrap(expected);
    Check(encoded == golden, "WUP2 matches independent .NET binary/base64 golden");
    Check(encoded.size() == 165 && encoded.size() + 1 <= MAX_PATH, "WUP2 fits fixed MAX_PATH initialization buffer");
    Bootstrap parsed{};
    Check(ParseBootstrap(golden.c_str(), parsed) == S_OK && SameBootstrap(parsed, expected), "WUP2 all fields decode at exact offsets");
    Check(ParseBootstrap(encoded.c_str(), parsed) == S_OK && SameBootstrap(parsed, expected), "WUP2 encode/decode roundtrip");
    std::vector<uint8_t> bytes(120);
    const uint8_t ids[][16]{
        {0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff},
        {0x10,0x21,0x32,0x43,0x54,0x65,0x76,0x87,0x98,0xa9,0xba,0xcb,0xdc,0xed,0xfe,0x0f},
        {0xff,0xee,0xdd,0xcc,0xbb,0xaa,0x99,0x88,0x77,0x66,0x55,0x44,0x33,0x22,0x11,0x00},
        {0x89,0xab,0xcd,0xef,0x01,0x23,0x45,0x67,0x89,0xab,0xcd,0xef,0x01,0x23,0x45,0x67}};
    const size_t offsets[]{0,16,44,72};
    for (size_t i = 0; i < 4; ++i) std::copy_n(ids[i], 16, bytes.begin() + offsets[i]);
    Put(bytes,32,0x12345678,4); Put(bytes,36,9007199254740993ull,8);
    Put(bytes,60,0x9abcdef0,4); Put(bytes,64,INT64_MAX,8);
    for (size_t i = 0; i < 32; ++i) bytes[88 + i] = static_cast<uint8_t>(i);
    Check(CompactText(bytes) == golden, "test mutations start from independent binary fixture");
    for (size_t i = 0; i < golden.size(); ++i)
        Check(ParseBootstrap(golden.substr(0,i).c_str(), parsed) == E_INVALIDARG, "every WUP2 truncation rejected");
    for (const auto* extra : {L"=",L"==",L"A",L"AAAA",L" ",L"\r\n",L"|extra"})
        Check(ParseBootstrap((golden + extra).c_str(), parsed) == E_INVALIDARG, "WUP2 exact length and no padding");
    for (size_t i = 5; i < golden.size(); ++i)
    {
        for (const wchar_t invalid : {L'=',L' ',L'\t',L'\r',L'\n',L'-',L'_',L'|',L'?',L'\x80',L'\xff21',L'\0'})
        {
            auto bad = golden; bad[i] = invalid;
            Check(ParseBootstrap(bad.c_str(), parsed) == E_INVALIDARG, "WUP2 only canonical ASCII base64");
        }
    }
    for (const auto& field : std::vector<std::pair<size_t,size_t>>{{0,16},{16,16},{32,4},{36,8},{44,16},{60,4},{64,8},{72,16}})
    {
        auto bad = bytes;
        std::fill_n(bad.begin() + field.first, field.second, uint8_t{});
        parsed = expected;
        Check(ParseBootstrap(CompactText(bad).c_str(), parsed) == E_INVALIDARG &&
            SameBootstrap(parsed, Bootstrap{}), "WUP2 invalid binary field rejected and all output cleared");
    }
    for (size_t offset : {size_t(36),size_t(64)})
    {
        auto bad = bytes;
        Put(bad,offset,uint64_t(INT64_MAX) + 1,8);
        Check(ParseBootstrap(CompactText(bad).c_str(), parsed) == E_INVALIDARG, "WUP2 signed FILETIME upper bound");
        Put(bad,offset,UINT64_MAX,8);
        Check(ParseBootstrap(CompactText(bad).c_str(), parsed) == E_INVALIDARG, "WUP2 rejects UINT64_MAX FILETIME");
    }
    auto extremes = expected;
    extremes.Target.Pid = extremes.Client.Pid = UINT32_MAX;
    extremes.Target.CreationTime = extremes.Client.CreationTime = INT64_MAX;
    Check(ParseBootstrap(EncodeBootstrap(extremes).c_str(), parsed) == S_OK && SameBootstrap(parsed, extremes),
        "WUP2 maximum process identities");
    extremes.Target.Pid = extremes.Client.Pid = 1;
    extremes.Target.CreationTime = extremes.Client.CreationTime = 1;
    extremes.Nonce.fill(0xff);
    Check(ParseBootstrap(EncodeBootstrap(extremes).c_str(), parsed) == S_OK && SameBootstrap(parsed, extremes),
        "WUP2 minimum process identities and arbitrary nonce bytes");
    extremes.Nonce.fill(0xfb);
    Check(ParseBootstrap(EncodeBootstrap(extremes).c_str(), parsed) == S_OK && SameBootstrap(parsed, extremes),
        "WUP2 standard base64 plus character");
    for (const wchar_t* pipe : {L"",L"WinUIProfiler-00000000000000000000000000000000",
        L"WinUIProfiler-00112233445566778899aabbccddeefg",L"winuiProfiler-00112233445566778899aabbccddeeff",
        L"WinUIProfiler-00112233445566778899aabbccddeeffx",L"\\\\.\\pipe\\WinUIProfiler-00112233445566778899aabbccddeeff",
        L"\\\\remote\\pipe\\WinUIProfiler-00112233445566778899aabbccddeeff"})
    {
        auto bad = expected; bad.PipeName = pipe;
        Throws([&] { EncodeBootstrap(bad); }, "WUP2 encoder rejects invalid local pipe UUID/name");
    }
    for (size_t i = 0; i < 9; ++i)
    {
        auto bad = expected;
        switch (i)
        {
        case 0: bad.Session = {}; break;
        case 1: bad.Target.Instance = {}; break;
        case 2: bad.Client.Instance = {}; break;
        case 3: bad.Target.Pid = 0; break;
        case 4: bad.Client.Pid = 0; break;
        case 5: bad.Target.CreationTime = 0; break;
        case 6: bad.Client.CreationTime = 0; break;
        case 7: bad.Target.CreationTime = uint64_t(INT64_MAX) + 1; break;
        case 8: bad.Client.CreationTime = UINT64_MAX; break;
        }
        Throws([&] { EncodeBootstrap(bad); }, "WUP2 encoder validates every identity field");
    }
    auto upper = expected;
    upper.PipeName = L"WinUIProfiler-00112233445566778899AABBCCDDEEFF";
    Check(EncodeBootstrap(upper) == golden && ParseBootstrap(EncodeBootstrap(upper).c_str(), parsed) == S_OK &&
        parsed.PipeName == expected.PipeName, "WUP2 canonicalizes pipe N UUID to lowercase");
    bool allocationFailed{};
    FailNextAllocation = true;
    try { (void)EncodeBootstrap(expected); } catch (const std::bad_alloc&) { allocationFailed = true; }
    FailNextAllocation = false;
    Check(allocationFailed, "WUP2 encoder propagates bad_alloc");
    parsed = expected;
    FailNextAllocation = true;
    const auto allocationStatus = ParseBootstrap(golden.c_str(), parsed);
    FailNextAllocation = false;
    Check(allocationStatus == E_OUTOFMEMORY && SameBootstrap(parsed,Bootstrap{}), "WUP2 parser allocation failure clears output");
}
void HeaderTests(const Bootstrap& expected)
{
    Header header{};
    auto frame = Frame(MessageType::Capabilities, R"({"schemaVersion":1})");
    Check(DecodeHeader(frame.data(), 28, header) == S_OK && header.RequestId == Id &&
        header.BodyLength == 19 && header.Type == MessageType::Capabilities, "header RFC4122 and little endian fields");
    Check(DecodeHeader(nullptr, 28, header) == E_INVALIDARG, "null header");
    for (size_t n = 0; n < 28; ++n) Check(DecodeHeader(frame.data(), n, header) == E_INVALIDARG, "truncated header");
    Check(DecodeHeader(frame.data(), 29, header) == E_INVALIDARG, "header requires exact length");
    for (uint32_t length : {0u, 1048577u, UINT32_MAX})
    {
        auto bad = frame; Put(bad, 0, length, 4);
        Check(DecodeHeader(bad.data(), 28, header) == E_INVALIDARG, "bounded header body size");
    }
    auto maximum = frame; Put(maximum, 0, 1048576, 4);
    Check(DecodeHeader(maximum.data(), 28, header) == S_OK, "maximum body length header");
    for (size_t at : {size_t(4), size_t(24)})
    {
        auto bad = frame; Put(bad, at, 2, at == 4 ? 2 : 4);
        Check(DecodeHeader(bad.data(), 28, header) == HRESULT_FROM_WIN32(ERROR_REVISION_MISMATCH), "unsupported header versions");
    }
    auto bad = frame; std::fill(bad.begin() + 8, bad.begin() + 24, uint8_t{});
    Check(DecodeHeader(bad.data(), 28, header) == E_INVALIDARG, "empty request identity");
    bad = frame; Put(bad, 6, UINT16_MAX, 2);
    Check(DecodeHeader(bad.data(), 28, header) == E_INVALIDARG, "unknown message type");
    Command command{};
    Check(DecodeCommand(nullptr, frame.size(), expected, true, command) == E_INVALIDARG, "null command");
    for (size_t n = 0; n < frame.size(); ++n)
        Check(DecodeCommand(frame.data(), n, expected, true, command) == E_INVALIDARG, "truncated command");
    frame.push_back(0);
    Check(DecodeCommand(frame.data(), frame.size(), expected, true, command) == E_INVALIDARG, "extra frame bytes");
}
void AuthenticationTests(const Bootstrap& expected)
{
    Check(Decode(MessageType::Handshake, Handshake, expected, false) == S_OK, "valid unauthenticated handshake");
    Check(Decode(MessageType::Handshake, Handshake, expected) == E_ACCESSDENIED, "second handshake");
    for (auto type : {MessageType::Capabilities, MessageType::Rundown, MessageType::LightweightSnapshot,
        MessageType::OwnershipGraph, MessageType::Cancel, MessageType::Disconnect, MessageType::Acknowledgement,
        MessageType::CapabilitiesResult, MessageType::Hello, MessageType::TakeSnapshot,
        MessageType::SnapshotChunk, MessageType::SnapshotResult})
        Check(Decode(type, "{}", expected, false) == E_ACCESSDENIED, "pre-authentication rejects non-handshake");
    for (const auto& pair : std::vector<std::pair<std::string, std::string>>{
        {"9007199254740993","9007199254740992"}, {"\"processId\":42","\"processId\":43"},
        {"\"sessionId\":\"0011","\"sessionId\":\"1011"}, {"\"instanceId\":\"0011","\"instanceId\":\"1011"},
        {"AAECAwQF","BAECAwQF"}})
        Check(Decode(MessageType::Handshake, Replace(Handshake, pair.first, pair.second), expected, false) == E_ACCESSDENIED,
            "mismatched nonce/session/canonical target");
    for (const auto& pair : std::vector<std::pair<std::string, std::string>>{
        {"9007199254740993","null"}, {"9007199254740993","9223372036854775808"},
        {"9007199254740993","0"}, {"\"processId\":42","\"processId\":0"},
        {"AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8=","AA=="},
        {"AAECAwQF","!AECAwQF"}, {"\"schemaVersion\":1","\"schemaVersion\":\"1\""}})
        Check(Decode(MessageType::Handshake, Replace(Handshake, pair.first, pair.second), expected, false) == E_INVALIDARG,
            "invalid handshake typed value");
    Check(Decode(MessageType::Handshake, Replace(Handshake, "\"schemaVersion\":1", "\"schemaVersion\":2"), expected, false) ==
        HRESULT_FROM_WIN32(ERROR_REVISION_MISMATCH), "handshake body schema");
    for (size_t i = 0; i < expected.Nonce.size(); ++i)
    {
        auto other = expected; other.Nonce[i] ^= 0x80;
        Check(Decode(MessageType::Handshake, Handshake, other, false) == E_ACCESSDENIED, "every nonce byte authenticated");
    }
    Check(Decode(MessageType::Handshake, Replace(Handshake, "AAECAwQF", "AAEC\\nAwQF"), expected, false) == S_OK,
        "managed base64 whitespace");
    for (size_t i = 0; i < std::string_view(Handshake).size(); ++i)
        Check(Decode(MessageType::Handshake, std::string(Handshake, i), expected, false) == E_INVALIDARG,
            "truncated JSON with exact frame length");
}
void RequestTests(const Bootstrap& expected)
{
    const std::string snapshot = std::string("{\"captureId\":\"") + IdText + "\"}";
    Command snapshotCommand{};
    Check(Decode(MessageType::TakeSnapshot, snapshot, expected, true, &snapshotCommand) == S_OK &&
        snapshotCommand.CaptureId == Id, "TAP-owned snapshot retains capture correlation");
    for (const char* json : {"{}", R"({"captureId":null})",
        R"({"captureId":"00000000-0000-0000-0000-000000000000"})"})
        Check(Decode(MessageType::TakeSnapshot, json, expected) == E_INVALIDARG, "snapshot requires nonempty capture identity");
    Check(Decode(MessageType::TakeSnapshot, Replace(snapshot, "{", "{\"extra\":null,"), expected) == E_INVALIDARG,
        "snapshot rejects extra fields");
    Check(Decode(MessageType::SnapshotChunk, snapshot, expected) == E_INVALIDARG &&
        Decode(MessageType::SnapshotResult, snapshot, expected) == E_INVALIDARG, "snapshot responses cannot be requests");
    Check(Body(EncodeSnapshotChunk(Id, Id, 0, "Hello", 5)) ==
        std::string("{\"captureId\":\"") + IdText + "\",\"index\":0,\"dataBase64\":\"SGVsbG8=\"}",
        "snapshot chunk uses exact bounded base64 encoding");
    Check(Body(EncodeSnapshotResult(Id, Id, 1, 5)) ==
        std::string("{\"captureId\":\"") + IdText + "\",\"chunkCount\":1,\"byteCount\":5}",
        "snapshot terminal declares exact counts");
    const auto hello = MessageType::Hello;
    Command helloCommand{};
    Check(Decode(hello, R"({"message":"hello"})", expected, true, &helloCommand) == S_OK &&
        helloCommand.Frame.Type == hello && helloCommand.Frame.RequestId == Id, "authenticated typed hello preserves request");
    Check(Decode(hello, R"({"message":"hello"})", expected, false) == E_ACCESSDENIED,
        "hello requires authenticated handshake");
    for (const char* json : {"{}", "null", R"({"message":null})", R"({"message":"Hello"})",
        R"({"message":"hello "})", R"({"message":1})", R"({"message":"hello","message":"hello"})",
        R"({"message":"hello","\u006dessage":"hello"})", R"({"message":"hello","extra":null})"})
        Check(Decode(hello, json, expected) == E_INVALIDARG, "strict literal hello body");
    Check(Decode(MessageType::Capabilities, R"({"schemaVersion":1})", expected) == S_OK, "capabilities request");
    Check(Decode(MessageType::Capabilities, R"({"schemaVersion":2})", expected) == HRESULT_FROM_WIN32(ERROR_REVISION_MISMATCH),
        "capabilities body schema");
    const std::string scope = std::string(R"({"captureId":")") + IdText + R"(","rootId":null,"dispatcherId":null})";
    for (auto type : {MessageType::Rundown, MessageType::LightweightSnapshot})
    {
        Command command{};
        Check(Decode(type, scope, expected, true, &command) == S_OK && command.CaptureId == Id &&
            !command.HasRoot && !command.HasDispatcher, "nullable snapshot scope");
        auto full = Replace(Replace(scope, "\"rootId\":null", std::string("\"rootId\":\"") + IdText + "\""),
            "\"dispatcherId\":null", "\"dispatcherId\":18446744073709551615");
        Check(Decode(type, full, expected, true, &command) == S_OK && command.HasRoot && command.RootId == Id &&
            command.HasDispatcher && command.DispatcherId == UINT64_MAX, "precise unsigned 64-bit dispatcher");
        for (const char* number : {"0","-1","18446744073709551616","1.0","1e3","\"3\"","true","01"})
            Check(Decode(type, Replace(scope, "\"dispatcherId\":null", std::string("\"dispatcherId\":") + number), expected) ==
                E_INVALIDARG, "invalid dispatcher number");
        Check(Decode(type, Replace(scope, "\"rootId\":null", "\"rootId\":\"00000000-0000-0000-0000-000000000000\""), expected) ==
            E_INVALIDARG, "non-null root cannot be empty");
        Check(Decode(type, Replace(scope, ",\"rootId\":null", ""), expected) == E_INVALIDARG, "missing nullable member");
    }
    const std::string graph = std::string(R"({"captureId":")") + IdText + R"(","maximumObjects":1000000,"maximumEdges":4000000})";
    Command command{};
    Check(Decode(MessageType::OwnershipGraph, graph, expected, true, &command) == S_OK &&
        command.MaximumObjects == 1000000 && command.MaximumEdges == 4000000, "ownership graph maximum limits");
    Check(Decode(MessageType::OwnershipGraph, Replace(Replace(graph, "1000000", "1"), "4000000", "1"), expected) == S_OK,
        "ownership graph minimum limits");
    for (const auto& pair : std::vector<std::pair<std::string, std::string>>{
        {"1000000","1000001"},{"1000000","0"},{"4000000","4000001"},{"4000000","0"},{"4000000","-1"}})
        Check(Decode(MessageType::OwnershipGraph, Replace(graph, pair.first, pair.second), expected) == E_INVALIDARG,
            "ownership graph out of bounds");
    const auto cancel = std::string(R"({"targetRequestId":")") + IdText + "\"}";
    Check(Decode(MessageType::Cancel, cancel, expected, true, &command) == S_OK && command.TargetRequestId == Id, "cancel target");
    Check(Decode(MessageType::Disconnect, R"({"reason":"done"})", expected) == S_OK, "disconnect");
    for (const char* reason : {R"({"reason":""})", R"({"reason":" \t\r\n"})", R"({"reason":null})", R"({"reason":12})",
        R"({"reason":"\u2003\u0085"})"})
        Check(Decode(MessageType::Disconnect, reason, expected) == E_INVALIDARG, "disconnect requires non-whitespace text");
    Check(Decode(MessageType::Acknowledgement, "{}", expected) == E_INVALIDARG, "response-only acknowledgement rejected");
    Check(Decode(MessageType::CapabilitiesResult, "{}", expected) == E_INVALIDARG, "response-only capabilities rejected");
}
void JsonTests(const Bootstrap& expected)
{
    for (const char* json : {"{}", "null", "[]", R"({"schemaVersion":1,"extra":1})",
        R"({"schemaVersion":1,"schemaVersion":1})", R"({"schemaVersion":1,"\u0073chemaVersion":1})",
        R"({"SchemaVersion":1})", R"({"schemaVersion":null})", R"({"schemaVersion":true})",
        R"({"schemaVersion":01})", R"({"schemaVersion":1,})", R"({"schemaVersion":1}x)",
        R"({"schemaVersion":1/*comment*/})", R"({"schemaVersion":1.0})"})
        Check(Decode(MessageType::Capabilities, json, expected) == E_INVALIDARG, "strict object JSON schema");
    Check(Decode(MessageType::Capabilities, " \r\n{\"schemaVersion\":1}\t ", expected) == S_OK, "JSON whitespace");
    Check(Decode(MessageType::Capabilities, R"({"\u0073chemaVersion":1})", expected) == S_OK, "escaped member name");
    for (const char* json : {R"({"reason":"\uD800"})", R"({"reason":"\uDC00"})", R"({"reason":"\uD800\u0041"})",
        R"({"reason":"\x41"})", "{\"reason\":\"a\nb\"}"})
        Check(Decode(MessageType::Disconnect, json, expected) == E_INVALIDARG, "invalid string escapes");
    Check(Decode(MessageType::Disconnect, R"({"reason":"\uD83D\uDE00"})", expected) == S_OK, "paired escaped surrogates");
    for (const std::string& bytes : {std::string("\xc0\xaf",2),std::string("\xed\xa0\x80",3),
        std::string("\xf4\x90\x80\x80",4),std::string("\x80",1),std::string("\xe2\x82",2)})
        Check(Decode(MessageType::Disconnect, "{\"reason\":\"" + bytes + "\"}", expected) == E_INVALIDARG, "malformed UTF8");
    Check(Decode(MessageType::Disconnect, "{\"reason\":\"\xf0\x9f\x98\x80\"}", expected) == S_OK, "valid raw UTF8");
    std::string deep = "1"; for (int i = 0; i < 40; ++i) deep = "{\"a\":" + deep + "}";
    Check(Decode(MessageType::Capabilities, deep, expected) == E_INVALIDARG, "bounded JSON depth");
    std::string wide = "{"; for (int i = 0; i < 100; ++i) wide += "\"a" + std::to_string(i) + "\":1,";
    wide += "\"schemaVersion\":1}";
    Check(Decode(MessageType::Capabilities, wide, expected) == E_INVALIDARG, "bounded JSON node count");
    auto maximum = "{\"reason\":\"" + std::string(1048576 - 13, 'x') + "\"}";
    Check(maximum.size() == 1048576 && Decode(MessageType::Disconnect, maximum, expected) == S_OK, "maximum sized body");
    maximum.insert(maximum.size() - 2, 1, 'x');
    Check(Decode(MessageType::Disconnect, maximum, expected) == E_INVALIDARG, "oversized JSON body");
    Command output{}; output.MaximumObjects = 77;
    Check(Decode(MessageType::Capabilities, "{}", expected, true, &output) == E_INVALIDARG &&
        output.MaximumObjects == 0 && output.Frame.Version == 0, "failure clears output");
}
void SchemaTests(const Bootstrap& expected)
{
    const std::string capture = std::string("\"captureId\":\"") + IdText + "\"";
    const std::string scope = "{" + capture + ",\"rootId\":null,\"dispatcherId\":null}";
    const std::string graph = "{" + capture + ",\"maximumObjects\":1,\"maximumEdges\":1}";
    const std::string cancel = std::string("{\"targetRequestId\":\"") + IdText + "\"}";
    for (const auto& request : std::vector<std::pair<MessageType, std::string>>{
        {MessageType::Handshake, Handshake}, {MessageType::Capabilities,R"({"schemaVersion":1})"},
        {MessageType::Rundown,scope}, {MessageType::LightweightSnapshot,scope}, {MessageType::OwnershipGraph,graph},
        {MessageType::Cancel,cancel}, {MessageType::Disconnect,R"({"reason":"done"})"},
        {MessageType::Hello,R"({"message":"hello"})"}})
    {
        const bool auth = request.first != MessageType::Handshake;
        Check(Decode(request.first, Replace(request.second, "{", "{\"unknown\":null,"), expected, auth) == E_INVALIDARG,
            "every request rejects unknown fields");
        const auto colon = request.second.find(':');
        const auto comma = request.second.find(',', colon);
        const auto firstMember = request.second.substr(1, (comma == std::string::npos ? request.second.size() - 1 : comma) - 1);
        Check(Decode(request.first, Replace(request.second, "{", "{" + firstMember + ","), expected, auth) == E_INVALIDARG,
            "every request rejects duplicate fields");
        Check(Decode(request.first, "{}", expected, auth) == E_INVALIDARG, "every request requires fields");
    }
    for (const char* member : {"\"rootId\":null,", ",\"dispatcherId\":null"})
        Check(Decode(MessageType::Rundown, Replace(scope, member, ""), expected) == E_INVALIDARG, "all scope fields required");
    for (const char* member : {",\"maximumObjects\":1", ",\"maximumEdges\":1"})
        Check(Decode(MessageType::OwnershipGraph, Replace(graph, member, ""), expected) == E_INVALIDARG, "all graph fields required");
    for (const char* guid : {"", "00112233445566778899aabbccddeeff", "{00112233-4455-6677-8899-aabbccddeeff}",
        "00000000-0000-0000-0000-000000000000", "00112233-4455-6677-8899-aabbccddeefg"})
    {
        Check(Decode(MessageType::Cancel, Replace(cancel, IdText, guid), expected) == E_INVALIDARG, "invalid GUID text");
        Check(Decode(MessageType::Rundown, Replace(scope, IdText, guid), expected) == E_INVALIDARG, "invalid capture GUID");
    }
    for (const char* value : {"null", "true", "1", "{}", "[]"})
        Check(Decode(MessageType::Cancel, Replace(cancel, std::string("\"") + IdText + "\"", value), expected) == E_INVALIDARG,
            "GUID must be a string");
    Check(Decode(MessageType::Handshake, Replace(Handshake, "\"processId\":42", "\"processId\":42,\"extra\":1"),
        expected, false) == E_INVALIDARG, "nested process rejects unknown members");
    Check(Decode(MessageType::Handshake, Replace(Handshake, "\"processId\":42", "\"processId\":42,\"processId\":42"),
        expected, false) == E_INVALIDARG, "nested process rejects duplicate members");
    Check(Decode(MessageType::Handshake, Replace(Handshake, "\"processId\":42,", ""),
        expected, false) == E_INVALIDARG, "nested process requires PID");
    Check(Decode(MessageType::Handshake, Replace(Handshake, "\"processId\":42", "\"processId\":4294967296"),
        expected, false) == E_INVALIDARG, "PID is bounded uint32");
    Check(Decode(MessageType::Handshake, Replace(Handshake, "9007199254740993", "9007199254740993.0"),
        expected, false) == E_INVALIDARG, "FILETIME is an integer not floating point");
    const auto frame = Frame(MessageType::Disconnect, R"({"reason":"a reason long enough to allocate"})");
    Command result{};
    FailNextAllocation = true;
    const auto allocationStatus = DecodeCommand(frame.data(), frame.size(), expected, true, result);
    FailNextAllocation = false;
    Check(allocationStatus == E_OUTOFMEMORY && result.Frame.Version == 0, "decode allocation failure is explicit");
    Bootstrap bootstrap{};
    FailNextAllocation = true;
    const auto bootstrapStatus = ParseBootstrap(Init, bootstrap);
    FailNextAllocation = false;
    Check(bootstrapStatus == E_OUTOFMEMORY && bootstrap.PipeName.empty(), "bootstrap allocation failure is explicit");
}
void ResponseTests()
{
    const auto hello = EncodeHello(Id);
    Header helloHeader{};
    Check(Body(hello) == R"({"message":"hello"})" && hello[6] == 10 && hello[7] == 0 &&
        DecodeHeader(hello.data(), 28, helloHeader) == S_OK && helloHeader.Type == MessageType::Hello &&
        helloHeader.RequestId == Id, "exact correlated hello reply");
    Throws([] { EncodeHello(GUID{}); }, "hello requires nonempty request identity");
    auto ack = EncodeAcknowledgement(Id, CommandStatus::Completed, GUID{}, "");
    Check(Body(ack) == R"({"status":"Completed","captureId":null,"reason":null})", "exact completed acknowledgement");
    Header header{};
    Check(DecodeHeader(ack.data(), 28, header) == S_OK && header.RequestId == Id &&
        header.Type == MessageType::Acknowledgement && header.BodyLength == ack.size() - 28, "acknowledgement header");
    const char* names[]{"Accepted","Completed","Unsupported","Rejected","Cancelled","Failed"};
    for (int i = 0; i < 6; ++i)
    {
        auto encoded = EncodeAcknowledgement(Id, static_cast<CommandStatus>(i), Id, "why\n\"\\");
        const auto expected = std::string("{\"status\":\"") + names[i] + "\",\"captureId\":\"" + IdText +
            "\",\"reason\":\"why\\n\\\"\\\\\"}";
        Check(Body(encoded) == expected, "status/guid/reason encoding");
    }
    for (auto status : {CommandStatus::Rejected,CommandStatus::Failed,CommandStatus::Unsupported})
        Throws([=] { EncodeAcknowledgement(Id, status, GUID{}, " \t"); }, "negative status requires reason");
    Throws([] { EncodeAcknowledgement(Id, static_cast<CommandStatus>(6), GUID{}, "why"); }, "invalid output enum");
    Throws([] { EncodeAcknowledgement(GUID{}, CommandStatus::Completed, GUID{}, ""); }, "empty output request");
    Throws([] { EncodeCapabilities(Id, GUID{}); }, "empty output epoch");
    Throws([] { EncodeAcknowledgement(Id, CommandStatus::Completed, GUID{}, std::string(1048576, 'x')); }, "bounded output");
    Throws([] { EncodeAcknowledgement(Id, CommandStatus::Completed, GUID{}, "\xc0\xaf"); }, "invalid output UTF8");
    Check(Body(EncodeCapabilities(Id, Id)) == std::string("{\"epoch\":\"") + IdText +
        "\",\"schemaVersion\":1,\"capabilities\":[]}", "honest empty capabilities");
    const auto caps = EncodeCapabilities(Id, Id);
    Check(DecodeHeader(caps.data(), 28, header) == S_OK && header.Type == MessageType::CapabilitiesResult &&
        header.RequestId == Id, "capabilities response header");
}
}
int main()
{
    Bootstrap expected{};
    BootstrapTests(expected);
    CompactBootstrapTests();
    HeaderTests(expected);
    AuthenticationTests(expected);
    RequestTests(expected);
    JsonTests(expected);
    SchemaTests(expected);
    ResponseTests();
    std::printf("TAP protocol: %d checks, %d failures\n", Checks, Failures);
    return Failures ? 1 : 0;
}
