// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <windows.h>
#include <cstdint>
#include <array>
#include <vector>
#include <wil/result.h>
#include <wil/resource.h>

namespace Protocol
{
    inline constexpr UINT32 Magic = 0x504D584F;
    inline constexpr UINT16 Version = 1;
    inline constexpr UINT32 HeaderSize = 20;
    inline constexpr UINT32 MaximumPayloadSize = 64 * 1024 * 1024;

    enum class Kind : UINT16
    {
        Hello = 1,
        Capture = 2,
        Snapshot = 3,
        Error = 4,
        Disconnect = 5
    };

    struct Frame
    {
        Kind Type{};
        UINT64 RequestId{};
        std::vector<BYTE> Payload;
    };

    template<typename T>
    T ReadNumber(const BYTE* data)
    {
        T value{};
        for (size_t index = 0; index < sizeof(T); ++index)
        {
            value |= static_cast<T>(data[index]) << (index * 8);
        }
        return value;
    }

    template<typename T>
    void WriteNumber(BYTE* data, T value)
    {
        for (size_t index = 0; index < sizeof(T); ++index)
        {
            data[index] = static_cast<BYTE>(value >> (index * 8));
        }
    }

    inline DWORD Transfer(HANDLE pipe, HANDLE stop, BYTE* buffer, DWORD length, bool writing)
    {
        wil::unique_event completed(wil::EventOptions::ManualReset);
        OVERLAPPED operation{};
        operation.hEvent = completed.get();
        DWORD transferred{};
        BOOL succeeded = writing
            ? WriteFile(pipe, buffer, length, &transferred, &operation)
            : ReadFile(pipe, buffer, length, &transferred, &operation);
        if (!succeeded)
        {
            const DWORD error = GetLastError();
            THROW_WIN32_IF(error, error != ERROR_IO_PENDING);
            HANDLE waits[] = {stop, completed.get()};
            const DWORD wait = WaitForMultipleObjects(ARRAYSIZE(waits), waits, FALSE, INFINITE);
            if (wait != WAIT_OBJECT_0 + 1)
            {
                const DWORD waitError = wait == WAIT_FAILED ? GetLastError() : ERROR_CANCELLED;
                LOG_IF_WIN32_BOOL_FALSE(CancelIoEx(pipe, &operation));
                WaitForSingleObject(completed.get(), INFINITE);
                THROW_WIN32(waitError);
            }
            THROW_IF_WIN32_BOOL_FALSE(GetOverlappedResult(pipe, &operation, &transferred, FALSE));
        }
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_BROKEN_PIPE), transferred == 0);
        return transferred;
    }

    inline void ReadExactly(HANDLE pipe, HANDLE stop, BYTE* buffer, DWORD length)
    {
        while (length)
        {
            const DWORD received = Transfer(pipe, stop, buffer, length, false);
            buffer += received;
            length -= received;
        }
    }

    inline void WriteAll(HANDLE pipe, HANDLE stop, const BYTE* buffer, DWORD length)
    {
        while (length)
        {
            const DWORD sent = Transfer(pipe, stop, const_cast<BYTE*>(buffer), length, true);
            buffer += sent;
            length -= sent;
        }
    }

    inline Frame Read(HANDLE pipe, HANDLE stop)
    {
        std::array<BYTE, HeaderSize> header{};
        ReadExactly(pipe, stop, header.data(), HeaderSize);
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_REVISION_MISMATCH),
            ReadNumber<UINT32>(header.data()) != Magic ||
            ReadNumber<UINT16>(header.data() + 4) != Version);
        Frame result;
        result.Type = static_cast<Kind>(ReadNumber<UINT16>(header.data() + 6));
        result.RequestId = ReadNumber<UINT64>(header.data() + 8);
        const UINT32 length = ReadNumber<UINT32>(header.data() + 16);
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW), length > MaximumPayloadSize);
        THROW_HR_IF(E_INVALIDARG, result.Type < Kind::Hello || result.Type > Kind::Disconnect);
        result.Payload.resize(length);
        ReadExactly(pipe, stop, result.Payload.data(), length);
        return result;
    }

    inline void Write(HANDLE pipe, HANDLE stop, Kind kind, UINT64 requestId, const std::vector<BYTE>& payload)
    {
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW), payload.size() > MaximumPayloadSize);
        std::array<BYTE, HeaderSize> header{};
        WriteNumber(header.data(), Magic);
        WriteNumber(header.data() + 4, Version);
        WriteNumber(header.data() + 6, static_cast<UINT16>(kind));
        WriteNumber(header.data() + 8, requestId);
        WriteNumber(header.data() + 16, static_cast<UINT32>(payload.size()));
        WriteAll(pipe, stop, header.data(), HeaderSize);
        WriteAll(pipe, stop, payload.data(), static_cast<DWORD>(payload.size()));
    }
}
