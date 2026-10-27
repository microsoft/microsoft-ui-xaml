#pragma once

#include <windows.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace WinUIProfiler::Tap
{
inline constexpr size_t MaximumSnapshotBytes = 64 * 1024 * 1024;
inline constexpr size_t SnapshotChunkBytes = 48 * 1024;
enum class MessageType : uint16_t
{
    Handshake = 1, Capabilities = 2, Rundown = 3, LightweightSnapshot = 4,
    OwnershipGraph = 5, Cancel = 6, Disconnect = 7, Acknowledgement = 8,
    CapabilitiesResult = 9, Hello = 10, TakeSnapshot = 11, SnapshotChunk = 12, SnapshotResult = 13
};
enum class CommandStatus { Accepted, Completed, Unsupported, Rejected, Cancelled, Failed };
struct ProcessKey { uint32_t Pid{}; uint64_t CreationTime{}; GUID Instance{}; };
struct Bootstrap
{
    std::wstring PipeName;
    GUID Session{};
    ProcessKey Target{}, Client{};
    std::array<uint8_t, 32> Nonce{};
};
struct Header
{
    uint32_t BodyLength{};
    uint16_t Version{};
    MessageType Type{};
    GUID RequestId{};
    uint32_t SchemaVersion{};
};
struct Command
{
    Header Frame{};
    GUID CaptureId{}, RootId{}, TargetRequestId{};
    uint64_t DispatcherId{};
    bool HasRoot{}, HasDispatcher{};
    uint32_t MaximumObjects{}, MaximumEdges{};
};
HRESULT DecodeHeader(const uint8_t* data, size_t length, Header& result) noexcept;
HRESULT DecodeCommand(const uint8_t* frame, size_t length, const Bootstrap& expected,
    bool authenticated, Command& result) noexcept;
HRESULT ParseBootstrap(const wchar_t* text, Bootstrap& result) noexcept;
// Invalid output arguments throw std::invalid_argument; allocation failure propagates.
// Emits 165 ASCII characters of WUP2; ParseBootstrap also accepts legacy WUP1.
std::wstring EncodeBootstrap(const Bootstrap& bootstrap);
std::vector<uint8_t> EncodeAcknowledgement(GUID requestId, CommandStatus status,
    GUID captureId, const std::string& reason);
std::vector<uint8_t> EncodeCapabilities(GUID requestId, GUID epoch = {}, bool hasRuntime = false);
std::vector<uint8_t> EncodeHello(GUID requestId);
std::vector<uint8_t> EncodeSnapshotChunk(GUID requestId, GUID captureId, uint32_t index,
    const char* data, size_t length);
std::vector<uint8_t> EncodeSnapshotResult(GUID requestId, GUID captureId, uint32_t chunks, size_t bytes);
}
