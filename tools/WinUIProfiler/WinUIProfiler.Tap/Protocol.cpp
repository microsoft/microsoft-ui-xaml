#include "Protocol.h"
#include <algorithm>
#include <initializer_list>
#include <new>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace WinUIProfiler::Tap
{
namespace
{
constexpr size_t HeaderBytes = 28, MaximumBody = 1048576, MaximumBootstrap = 1024;
constexpr size_t MaximumDepth = 32, MaximumNodes = 64, MaximumMembers = 16;
constexpr size_t CompactBootstrapBytes = 120, CompactBootstrapCharacters = 165;
constexpr std::wstring_view PipePrefix = L"WinUIProfiler-";
constexpr HRESULT RevisionMismatch = HRESULT_FROM_WIN32(ERROR_REVISION_MISMATCH);
struct Invalid { HRESULT Code; };
[[noreturn]] void Reject(HRESULT code = E_INVALIDARG) { throw Invalid{code}; }
void Need(bool condition) { if (!condition) Reject(); }

uint64_t ReadLE(const uint8_t* data, size_t count) noexcept
{
    uint64_t result{};
    for (size_t i = 0; i < count; ++i) result |= uint64_t(data[i]) << (8 * i);
    return result;
}
void WriteLE(uint8_t* data, uint64_t value, size_t count) noexcept
{
    for (size_t i = 0; i < count; ++i) data[i] = static_cast<uint8_t>(value >> (8 * i));
}
GUID ReadGuid(const uint8_t* bytes) noexcept
{
    GUID result{};
    result.Data1 = (uint32_t(bytes[0]) << 24) | (uint32_t(bytes[1]) << 16) |
        (uint32_t(bytes[2]) << 8) | bytes[3];
    result.Data2 = static_cast<uint16_t>((uint16_t(bytes[4]) << 8) | bytes[5]);
    result.Data3 = static_cast<uint16_t>((uint16_t(bytes[6]) << 8) | bytes[7]);
    std::copy_n(bytes + 8, 8, result.Data4);
    return result;
}
void WriteGuid(uint8_t* bytes, const GUID& value) noexcept
{
    for (size_t i = 0; i < 4; ++i) bytes[i] = static_cast<uint8_t>(value.Data1 >> ((3 - i) * 8));
    bytes[4] = static_cast<uint8_t>(value.Data2 >> 8); bytes[5] = static_cast<uint8_t>(value.Data2);
    bytes[6] = static_cast<uint8_t>(value.Data3 >> 8); bytes[7] = static_cast<uint8_t>(value.Data3);
    std::copy_n(value.Data4, 8, bytes + 8);
}
template<typename Char> int Hex(Char c) noexcept
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}
template<typename Char> GUID Guid(std::basic_string_view<Char> text)
{
    Need(text.size() == 36);
    uint8_t bytes[16]{};
    size_t source{};
    for (size_t i = 0; i < 16; ++i)
    {
        if (i == 4 || i == 6 || i == 8 || i == 10) Need(text[source++] == '-');
        const int high = Hex(text[source++]), low = Hex(text[source++]);
        Need(high >= 0 && low >= 0);
        bytes[i] = static_cast<uint8_t>((high << 4) | low);
    }
    const auto result = ReadGuid(bytes);
    Need(result != GUID{});
    return result;
}
std::string GuidText(const GUID& value)
{
    uint8_t bytes[16]{};
    WriteGuid(bytes, value);
    constexpr char digits[] = "0123456789abcdef";
    std::string result;
    result.reserve(36);
    for (size_t i = 0; i < 16; ++i)
    {
        if (i == 4 || i == 6 || i == 8 || i == 10) result += '-';
        result += digits[bytes[i] >> 4]; result += digits[bytes[i] & 15];
    }
    return result;
}
template<typename Char> uint64_t Decimal(std::basic_string_view<Char> text, uint64_t maximum)
{
    Need(!text.empty());
    uint64_t result{};
    for (Char c : text)
    {
        Need(c >= '0' && c <= '9');
        const auto digit = static_cast<uint64_t>(c - '0');
        Need(result <= (maximum - digit) / 10);
        result = result * 10 + digit;
    }
    return result;
}
uint32_t Scalar(std::string_view text, size_t& index)
{
    Need(index < text.size());
    const auto first = static_cast<uint8_t>(text[index++]);
    if (first < 0x80) return first;
    size_t remaining{};
    uint32_t value{}, minimum{};
    if (first >= 0xc2 && first <= 0xdf) { remaining = 1; value = first & 31; minimum = 0x80; }
    else if (first >= 0xe0 && first <= 0xef) { remaining = 2; value = first & 15; minimum = 0x800; }
    else if (first >= 0xf0 && first <= 0xf4) { remaining = 3; value = first & 7; minimum = 0x10000; }
    else Reject();
    Need(remaining <= text.size() - index);
    for (size_t i = 0; i < remaining; ++i)
    {
        const auto next = static_cast<uint8_t>(text[index++]);
        Need((next & 0xc0) == 0x80);
        value = (value << 6) | (next & 63);
    }
    Need(value >= minimum && value <= 0x10ffff && !(value >= 0xd800 && value <= 0xdfff));
    return value;
}
void Utf8(std::string_view text)
{
    size_t index{};
    while (index < text.size()) (void)Scalar(text, index);
}
void AppendScalar(std::string& text, uint32_t value)
{
    if (value < 0x80) text += static_cast<char>(value);
    else if (value < 0x800)
    {
        text += static_cast<char>(0xc0 | (value >> 6));
        text += static_cast<char>(0x80 | (value & 63));
    }
    else if (value < 0x10000)
    {
        text += static_cast<char>(0xe0 | (value >> 12));
        text += static_cast<char>(0x80 | ((value >> 6) & 63));
        text += static_cast<char>(0x80 | (value & 63));
    }
    else
    {
        text += static_cast<char>(0xf0 | (value >> 18));
        text += static_cast<char>(0x80 | ((value >> 12) & 63));
        text += static_cast<char>(0x80 | ((value >> 6) & 63));
        text += static_cast<char>(0x80 | (value & 63));
    }
}
bool HasText(std::string_view text)
{
    bool found{};
    size_t index{};
    while (index < text.size())
    {
        const auto c = Scalar(text, index);
        const bool whitespace = (c >= 9 && c <= 13) || c == 0x20 || c == 0x85 || c == 0xa0 ||
            c == 0x1680 || (c >= 0x2000 && c <= 0x200a) || c == 0x2028 || c == 0x2029 ||
            c == 0x202f || c == 0x205f || c == 0x3000;
        found |= !whitespace;
    }
    return found;
}

struct Value
{
    enum class Kind { Null, String, Number, Object } Type{Kind::Null};
    std::string Text;
    uint64_t Number{};
    std::vector<std::pair<std::string, Value>> Members;
};
// Only objects and scalar types appearing in request schemas are accepted.
// The body, total nodes, object width and nesting are independently bounded.
class Json
{
public:
    explicit Json(std::string_view text) : m_text(text) { Utf8(text); }
    Value Parse()
    {
        auto value = Read(1);
        Space();
        Need(m_at == m_text.size());
        return value;
    }
private:
    std::string_view m_text;
    size_t m_at{}, m_nodes{};
    void Space()
    {
        while (m_at < m_text.size() && (m_text[m_at] == ' ' || m_text[m_at] == '\r' ||
            m_text[m_at] == '\n' || m_text[m_at] == '\t')) ++m_at;
    }
    char Take() { Need(m_at < m_text.size()); return m_text[m_at++]; }
    uint32_t Hex4()
    {
        uint32_t value{};
        for (int i = 0; i < 4; ++i)
        {
            const int digit = Hex(Take());
            Need(digit >= 0);
            value = (value << 4) | static_cast<uint32_t>(digit);
        }
        return value;
    }
    std::string String()
    {
        Need(Take() == '"');
        std::string value;
        for (;;)
        {
            const char c = Take();
            if (c == '"') return value;
            Need(static_cast<uint8_t>(c) >= 0x20);
            if (c != '\\') { value += c; continue; }
            switch (Take())
            {
            case '"': value += '"'; break;
            case '\\': value += '\\'; break;
            case '/': value += '/'; break;
            case 'b': value += '\b'; break;
            case 'f': value += '\f'; break;
            case 'n': value += '\n'; break;
            case 'r': value += '\r'; break;
            case 't': value += '\t'; break;
            case 'u':
            {
                auto scalar = Hex4();
                if (scalar >= 0xd800 && scalar <= 0xdbff)
                {
                    Need(Take() == '\\' && Take() == 'u');
                    const auto low = Hex4();
                    Need(low >= 0xdc00 && low <= 0xdfff);
                    scalar = 0x10000 + ((scalar - 0xd800) << 10) + low - 0xdc00;
                }
                else Need(!(scalar >= 0xdc00 && scalar <= 0xdfff));
                AppendScalar(value, scalar);
                break;
            }
            default: Reject();
            }
        }
    }
    Value Read(size_t depth)
    {
        Need(depth <= MaximumDepth && ++m_nodes <= MaximumNodes);
        Space(); Need(m_at < m_text.size());
        Value value;
        const char first = m_text[m_at];
        if (first == '"') { value.Type = Value::Kind::String; value.Text = String(); }
        else if (first == '{')
        {
            value.Type = Value::Kind::Object; ++m_at; Space();
            if (m_at < m_text.size() && m_text[m_at] == '}') { ++m_at; return value; }
            for (;;)
            {
                Need(value.Members.size() < MaximumMembers);
                auto name = String();
                for (const auto& member : value.Members) Need(member.first != name);
                Space(); Need(Take() == ':');
                auto child = Read(depth + 1);
                value.Members.emplace_back(std::move(name), std::move(child));
                Space();
                const char delimiter = Take();
                if (delimiter == '}') break;
                Need(delimiter == ','); Space();
            }
        }
        else if (first == 'n')
        {
            Need(m_text.substr(m_at, 4) == "null"); m_at += 4;
        }
        else
        {
            Need(first >= '0' && first <= '9');
            const auto begin = m_at++;
            while (m_at < m_text.size() && m_text[m_at] >= '0' && m_text[m_at] <= '9') ++m_at;
            Need(m_at - begin == 1 || first != '0');
            value.Type = Value::Kind::Number;
            value.Number = Decimal(m_text.substr(begin, m_at - begin), UINT64_MAX);
        }
        return value;
    }
};
void Members(const Value& object, std::initializer_list<std::string_view> names)
{
    Need(object.Type == Value::Kind::Object && object.Members.size() == names.size());
    for (const auto& member : object.Members)
        Need(std::find(names.begin(), names.end(), member.first) != names.end());
}
const Value& Field(const Value& object, std::string_view name)
{
    for (const auto& member : object.Members) if (member.first == name) return member.second;
    Reject();
}
uint64_t Number(const Value& value, uint64_t minimum, uint64_t maximum)
{
    Need(value.Type == Value::Kind::Number && value.Number >= minimum && value.Number <= maximum);
    return value.Number;
}
GUID Guid(const Value& value)
{
    Need(value.Type == Value::Kind::String);
    return Guid(std::string_view(value.Text));
}
void Schema(const Value& value)
{
    if (Number(value, 0, INT32_MAX) != 1) Reject(RevisionMismatch);
}
ProcessKey Process(const Value& value)
{
    Members(value, {"processId", "creationTimeFileTime", "instanceId"});
    return {static_cast<uint32_t>(Number(Field(value, "processId"), 1, UINT32_MAX)),
        Number(Field(value, "creationTimeFileTime"), 1, INT64_MAX), Guid(Field(value, "instanceId"))};
}
int Base64Digit(char c) noexcept
{
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}
GUID PipeUuid(std::wstring_view pipe)
{
    Need(pipe.size() == PipePrefix.size() + 32 && pipe.substr(0, PipePrefix.size()) == PipePrefix);
    std::array<uint8_t, 16> bytes{};
    for (size_t i = 0; i < bytes.size(); ++i)
    {
        const int high = Hex(pipe[PipePrefix.size() + 2 * i]), low = Hex(pipe[PipePrefix.size() + 2 * i + 1]);
        Need(high >= 0 && low >= 0);
        bytes[i] = static_cast<uint8_t>((high << 4) | low);
    }
    const auto uuid = ReadGuid(bytes.data());
    Need(uuid != GUID{});
    return uuid;
}
void BootstrapIdentity(const Bootstrap& value)
{
    Need(value.Session != GUID{} && value.Target.Instance != GUID{} && value.Client.Instance != GUID{});
    Need(value.Target.Pid && value.Client.Pid);
    Need(value.Target.CreationTime && value.Target.CreationTime <= INT64_MAX &&
        value.Client.CreationTime && value.Client.CreationTime <= INT64_MAX);
}
Bootstrap CompactBootstrap(std::wstring_view text)
{
    Need(text.size() == CompactBootstrapCharacters && text.substr(0, 5) == L"WUP2|");
    std::array<uint8_t, CompactBootstrapBytes> bytes{};
    for (size_t i = 0; i < bytes.size() / 3; ++i)
    {
        uint32_t block{};
        for (size_t j = 0; j < 4; ++j)
        {
            const wchar_t c = text[5 + 4 * i + j];
            Need(c <= 0x7f);
            const int digit = Base64Digit(static_cast<char>(c));
            Need(digit >= 0);
            block = (block << 6) | static_cast<uint32_t>(digit);
        }
        bytes[3 * i] = static_cast<uint8_t>(block >> 16);
        bytes[3 * i + 1] = static_cast<uint8_t>(block >> 8);
        bytes[3 * i + 2] = static_cast<uint8_t>(block);
    }
    // 120 bytes contain no partial Base64 group: alphabet and exact length
    // validation alone imply a canonical unpadded representation.
    const auto pipe = ReadGuid(bytes.data());
    Need(pipe != GUID{});
    Bootstrap value{};
    value.Session = ReadGuid(bytes.data() + 16);
    value.Target = {static_cast<uint32_t>(ReadLE(bytes.data() + 32, 4)),
        ReadLE(bytes.data() + 36, 8), ReadGuid(bytes.data() + 44)};
    value.Client = {static_cast<uint32_t>(ReadLE(bytes.data() + 60, 4)),
        ReadLE(bytes.data() + 64, 8), ReadGuid(bytes.data() + 72)};
    std::copy_n(bytes.data() + 88, value.Nonce.size(), value.Nonce.begin());
    BootstrapIdentity(value);
    value.PipeName = PipePrefix;
    for (char c : GuidText(pipe)) if (c != '-') value.PipeName += static_cast<wchar_t>(c);
    return value;
}
std::array<uint8_t, 32> Nonce(const Value& value)
{
    Need(value.Type == Value::Kind::String);
    std::array<char, 44> text{};
    size_t length{};
    for (char c : value.Text)
    {
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') continue;
        Need(length < text.size());
        text[length++] = c;
    }
    Need(length == text.size() && text[43] == '=');
    std::array<uint8_t, 32> bytes{};
    uint32_t accumulator{};
    size_t bits{}, written{};
    for (size_t i = 0; i < 43; ++i)
    {
        const int digit = Base64Digit(text[i]);
        Need(digit >= 0);
        accumulator = (accumulator << 6) | static_cast<uint32_t>(digit);
        bits += 6;
        if (bits >= 8)
        {
            bits -= 8;
            bytes[written++] = static_cast<uint8_t>(accumulator >> bits);
        }
    }
    Need(written == bytes.size());
    return bytes;
}
bool EqualNonce(const std::array<uint8_t, 32>& left, const std::array<uint8_t, 32>& right) noexcept
{
    volatile unsigned difference{};
    for (size_t i = 0; i < left.size(); ++i) difference |= left[i] ^ right[i];
    return difference == 0;
}
void Scope(const Value& value, Command& result)
{
    Members(value, {"captureId", "rootId", "dispatcherId"});
    result.CaptureId = Guid(Field(value, "captureId"));
    const auto& root = Field(value, "rootId");
    result.HasRoot = root.Type != Value::Kind::Null;
    if (result.HasRoot) result.RootId = Guid(root);
    const auto& dispatcher = Field(value, "dispatcherId");
    result.HasDispatcher = dispatcher.Type != Value::Kind::Null;
    if (result.HasDispatcher) result.DispatcherId = Number(dispatcher, 1, UINT64_MAX);
}
std::string Quote(std::string_view value)
{
    Need(value.size() <= MaximumBody);
    Utf8(value);
    constexpr char digits[] = "0123456789abcdef";
    std::string result = "\"";
    for (const unsigned char c : value)
    {
        switch (c)
        {
        case '"': result += "\\\""; break;
        case '\\': result += "\\\\"; break;
        case '\b': result += "\\b"; break;
        case '\f': result += "\\f"; break;
        case '\n': result += "\\n"; break;
        case '\r': result += "\\r"; break;
        case '\t': result += "\\t"; break;
        default:
            if (c < 0x20)
            {
                result += "\\u00"; result += digits[c >> 4]; result += digits[c & 15];
            }
            else result += static_cast<char>(c);
        }
        Need(result.size() < MaximumBody);
    }
    result += '"';
    return result;
}
std::vector<uint8_t> Encode(MessageType type, const GUID& requestId, const std::string& body)
{
    Need(requestId != GUID{} && !body.empty() && body.size() <= MaximumBody);
    std::vector<uint8_t> frame(HeaderBytes + body.size());
    WriteLE(frame.data(), body.size(), 4); WriteLE(frame.data() + 4, 1, 2);
    WriteLE(frame.data() + 6, static_cast<uint16_t>(type), 2);
    WriteGuid(frame.data() + 8, requestId); WriteLE(frame.data() + 24, 1, 4);
    std::copy(body.begin(), body.end(), frame.begin() + HeaderBytes);
    return frame;
}
}

HRESULT DecodeHeader(const uint8_t* data, size_t length, Header& result) noexcept
{
    result = {};
    if (!data || length != HeaderBytes) return E_INVALIDARG;
    Header candidate{};
    candidate.BodyLength = static_cast<uint32_t>(ReadLE(data, 4));
    candidate.Version = static_cast<uint16_t>(ReadLE(data + 4, 2));
    candidate.Type = static_cast<MessageType>(ReadLE(data + 6, 2));
    candidate.RequestId = ReadGuid(data + 8);
    candidate.SchemaVersion = static_cast<uint32_t>(ReadLE(data + 24, 4));
    if (candidate.Version != 1 || candidate.SchemaVersion != 1) return RevisionMismatch;
    if (candidate.BodyLength == 0 || candidate.BodyLength > MaximumBody || candidate.RequestId == GUID{} ||
        candidate.Type < MessageType::Handshake || candidate.Type > MessageType::SnapshotResult) return E_INVALIDARG;
    result = candidate;
    return S_OK;
}

HRESULT DecodeCommand(const uint8_t* frame, size_t length, const Bootstrap& expected,
    bool authenticated, Command& result) noexcept
{
    result = {};
    if (!frame || length < HeaderBytes) return E_INVALIDARG;
    Command candidate{};
    const auto status = DecodeHeader(frame, HeaderBytes, candidate.Frame);
    if (FAILED(status)) return status;
    if (length != HeaderBytes + candidate.Frame.BodyLength) return E_INVALIDARG;
    const auto type = candidate.Frame.Type;
    if ((!authenticated && type != MessageType::Handshake) ||
        (authenticated && type == MessageType::Handshake)) return E_ACCESSDENIED;
    if (type == MessageType::Acknowledgement || type == MessageType::CapabilitiesResult ||
        type == MessageType::SnapshotChunk || type == MessageType::SnapshotResult) return E_INVALIDARG;
    try
    {
        const auto body = Json(std::string_view(reinterpret_cast<const char*>(frame + HeaderBytes),
            candidate.Frame.BodyLength)).Parse();
        switch (type)
        {
        case MessageType::Handshake:
        {
            Members(body, {"sessionId", "process", "schemaVersion", "nonceBase64"});
            Schema(Field(body, "schemaVersion"));
            const auto session = Guid(Field(body, "sessionId"));
            const auto process = Process(Field(body, "process"));
            const auto nonce = Nonce(Field(body, "nonceBase64"));
            const bool nonceMatches = EqualNonce(nonce, expected.Nonce);
            if (!nonceMatches || session != expected.Session || process.Pid != expected.Target.Pid ||
                process.CreationTime != expected.Target.CreationTime || process.Instance != expected.Target.Instance)
                Reject(E_ACCESSDENIED);
            break;
        }
        case MessageType::TakeSnapshot:
            Members(body, {"captureId"}); candidate.CaptureId = Guid(Field(body, "captureId")); break;
        case MessageType::Capabilities:
            Members(body, {"schemaVersion"}); Schema(Field(body, "schemaVersion")); break;
        case MessageType::Hello:
        {
            Members(body, {"message"});
            const auto& message = Field(body, "message");
            Need(message.Type == Value::Kind::String && message.Text == "hello");
            break;
        }
        case MessageType::Rundown:
        case MessageType::LightweightSnapshot:
            Scope(body, candidate); break;
        case MessageType::OwnershipGraph:
            Members(body, {"captureId", "maximumObjects", "maximumEdges"});
            candidate.CaptureId = Guid(Field(body, "captureId"));
            candidate.MaximumObjects = static_cast<uint32_t>(Number(Field(body, "maximumObjects"), 1, 1000000));
            candidate.MaximumEdges = static_cast<uint32_t>(Number(Field(body, "maximumEdges"), 1, 4000000));
            break;
        case MessageType::Cancel:
            Members(body, {"targetRequestId"}); candidate.TargetRequestId = Guid(Field(body, "targetRequestId")); break;
        case MessageType::Disconnect:
        {
            Members(body, {"reason"});
            const auto& reason = Field(body, "reason");
            Need(reason.Type == Value::Kind::String && HasText(reason.Text));
            break;
        }
        default: Reject();
        }
        result = candidate;
        return S_OK;
    }
    catch (const Invalid& error) { return error.Code; }
    catch (const std::bad_alloc&) { return E_OUTOFMEMORY; }
    catch (...) { return E_INVALIDARG; }
}

HRESULT ParseBootstrap(const wchar_t* text, Bootstrap& result) noexcept
{
    result = {};
    if (!text) return E_INVALIDARG;
    size_t length{};
    while (length <= MaximumBootstrap && text[length]) ++length;
    if (length > MaximumBootstrap) return E_INVALIDARG;
    try
    {
        const std::wstring_view input(text, length);
        if (input.substr(0, 5) == L"WUP2|")
        {
            auto candidate = CompactBootstrap(input);
            result = std::move(candidate);
            return S_OK;
        }
        std::array<std::wstring_view, 10> fields{};
        size_t start{}, count{};
        for (size_t i = 0; i <= length; ++i)
        {
            if (i != length && text[i] != '|') continue;
            Need(count < fields.size());
            fields[count++] = {text + start, i - start}; start = i + 1;
        }
        Need(count == fields.size() && fields[0] == L"WUP1");
        const auto pipe = fields[1];
        constexpr std::wstring_view prefix = L"WinUIProfiler-";
        Need(pipe.size() == prefix.size() + 32 && pipe.substr(0, prefix.size()) == prefix);
        for (size_t i = prefix.size(); i < pipe.size(); ++i) Need(Hex(pipe[i]) >= 0);
        Bootstrap candidate{};
        candidate.PipeName = pipe;
        candidate.Session = Guid(fields[2]);
        candidate.Target = {static_cast<uint32_t>(Decimal(fields[3], UINT32_MAX)),
            Decimal(fields[4], INT64_MAX), Guid(fields[5])};
        candidate.Client = {static_cast<uint32_t>(Decimal(fields[6], UINT32_MAX)),
            Decimal(fields[7], INT64_MAX), Guid(fields[8])};
        Need(candidate.Target.Pid && candidate.Target.CreationTime && candidate.Client.Pid && candidate.Client.CreationTime);
        Need(fields[9].size() == 64);
        for (size_t i = 0; i < candidate.Nonce.size(); ++i)
        {
            const int high = Hex(fields[9][2 * i]), low = Hex(fields[9][2 * i + 1]);
            Need(high >= 0 && low >= 0);
            candidate.Nonce[i] = static_cast<uint8_t>((high << 4) | low);
        }
        result = std::move(candidate);
        return S_OK;
    }
    catch (const Invalid& error) { return error.Code; }
    catch (const std::bad_alloc&) { return E_OUTOFMEMORY; }
    catch (...) { return E_INVALIDARG; }
}

std::wstring EncodeBootstrap(const Bootstrap& bootstrap)
{
    try
    {
        const auto pipe = PipeUuid(bootstrap.PipeName);
        BootstrapIdentity(bootstrap);
        std::array<uint8_t, CompactBootstrapBytes> bytes{};
        WriteGuid(bytes.data(), pipe);
        WriteGuid(bytes.data() + 16, bootstrap.Session);
        WriteLE(bytes.data() + 32, bootstrap.Target.Pid, 4);
        WriteLE(bytes.data() + 36, bootstrap.Target.CreationTime, 8);
        WriteGuid(bytes.data() + 44, bootstrap.Target.Instance);
        WriteLE(bytes.data() + 60, bootstrap.Client.Pid, 4);
        WriteLE(bytes.data() + 64, bootstrap.Client.CreationTime, 8);
        WriteGuid(bytes.data() + 72, bootstrap.Client.Instance);
        std::copy(bootstrap.Nonce.begin(), bootstrap.Nonce.end(), bytes.begin() + 88);
        constexpr wchar_t alphabet[] = L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::wstring result = L"WUP2|";
        result.reserve(CompactBootstrapCharacters);
        for (size_t i = 0; i < bytes.size(); i += 3)
        {
            const uint32_t block = (uint32_t(bytes[i]) << 16) | (uint32_t(bytes[i + 1]) << 8) | bytes[i + 2];
            for (size_t shift : {size_t(18), size_t(12), size_t(6), size_t(0)}) result += alphabet[(block >> shift) & 63];
        }
        return result;
    }
    catch (const Invalid&) { throw std::invalid_argument("Invalid TAP bootstrap."); }
}

std::vector<uint8_t> EncodeAcknowledgement(GUID requestId, CommandStatus status,
    GUID captureId, const std::string& reason)
{
    try
    {
        constexpr const char* names[]{"Accepted", "Completed", "Unsupported", "Rejected", "Cancelled", "Failed"};
        const auto code = static_cast<unsigned>(status);
        Need(code < std::size(names));
        Need(reason.size() <= MaximumBody);
        if (status == CommandStatus::Unsupported || status == CommandStatus::Rejected || status == CommandStatus::Failed)
            Need(HasText(reason));
        const std::string body = std::string("{\"status\":\"") + names[code] + "\",\"captureId\":" +
            (captureId == GUID{} ? "null" : Quote(GuidText(captureId))) + ",\"reason\":" +
            (reason.empty() ? "null" : Quote(reason)) + "}";
        return Encode(MessageType::Acknowledgement, requestId, body);
    }
    catch (const Invalid&) { throw std::invalid_argument("Invalid TAP acknowledgement."); }
}

std::vector<uint8_t> EncodeHello(GUID requestId)
{
    try { return Encode(MessageType::Hello, requestId, R"({"message":"hello"})"); }
    catch (const Invalid&) { throw std::invalid_argument("Invalid TAP hello request identity."); }
}

std::vector<uint8_t> EncodeCapabilities(GUID requestId, GUID epoch, bool hasRuntime)
{
    try
    {
        Need(!hasRuntime && epoch == GUID{});
        return Encode(MessageType::CapabilitiesResult, requestId,
            "{\"epoch\":null,\"schemaVersion\":1,\"capabilities\":[]}");
    }
    catch (const Invalid&) { throw std::invalid_argument("Invalid TAP capabilities."); }
}
}
