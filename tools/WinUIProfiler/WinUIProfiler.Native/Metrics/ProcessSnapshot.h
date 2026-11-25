#pragma once
#include "..\Exports.h"
#include <Windows.h>
#include <winternl.h>
#include <cstddef>
#include <cstring>
#include <new>
#include <vector>

namespace wup::detail
{
using ProcessQuery = LONG(NTAPI*)(ULONG, void*, ULONG, ULONG*);
struct ProcessSnapshot
{
    WupMetricStatus status = WupUnavailable;
    uint32_t error = ERROR_INVALID_DATA;
    uint32_t domain = 1;
    uint32_t count = 0;
    uint64_t kernel = 0;
    uint64_t user = 0;
    uint64_t private_working_set = 0;
    uint64_t working_set = 0;
    uint64_t commit = 0;
};

// Isolate the Windows 11 x64/ARM64 SystemProcessInformation compatibility prefix.
static_assert(sizeof(void*) == 8 && sizeof(SYSTEM_PROCESS_INFORMATION) == 256);
static_assert(offsetof(SYSTEM_PROCESS_INFORMATION, Reserved1) == 8 &&
    offsetof(SYSTEM_PROCESS_INFORMATION, UniqueProcessId) == 80 &&
    offsetof(SYSTEM_PROCESS_INFORMATION, WorkingSetSize) == 144 &&
    offsetof(SYSTEM_PROCESS_INFORMATION, PrivatePageCount) == 200);

inline ProcessSnapshot ParseProcessSnapshot(const void* bytes, size_t length, WupProcessIdentity target)
{
    ProcessSnapshot result;
    if (!bytes || target.reserved || !target.pid || !target.creation_filetime_100ns)
    {
        return result;
    }
    const auto data = static_cast<const unsigned char*>(bytes);
    size_t offset = 0;
    bool found = false;
    while (true)
    {
        if (offset > length || length - offset < sizeof(SYSTEM_PROCESS_INFORMATION) || result.count == 16384)
        {
            return {};
        }
        SYSTEM_PROCESS_INFORMATION row{};
        std::memcpy(&row, data + offset, sizeof(row));
        ++result.count;
        if (reinterpret_cast<uintptr_t>(row.UniqueProcessId) == target.pid)
        {
            if (found)
            {
                return {};
            }
            found = true;
            uint64_t creation = 0;
            std::memcpy(&creation, row.Reserved1 + 24, sizeof(creation));
            std::memcpy(&result.user, row.Reserved1 + 32, sizeof(result.user));
            std::memcpy(&result.kernel, row.Reserved1 + 40, sizeof(result.kernel));
            std::memcpy(&result.private_working_set, row.Reserved1, sizeof(result.private_working_set));
            result.working_set = row.WorkingSetSize;
            result.commit = row.PrivatePageCount;
            if (creation != target.creation_filetime_100ns)
            {
                result.status = WupIdentityChanged;
            }
            else if (result.user > INT64_MAX || result.kernel > INT64_MAX ||
                result.private_working_set > result.working_set || result.commit > INT64_MAX ||
                result.working_set > INT64_MAX)
            {
                return {};
            }
            else
            {
                result.status = WupOk;
            }
        }
        if (!row.NextEntryOffset)
        {
            break;
        }
        if (row.NextEntryOffset < sizeof(row) || row.NextEntryOffset % alignof(SYSTEM_PROCESS_INFORMATION) ||
            row.NextEntryOffset > length - offset)
        {
            return {};
        }
        offset += row.NextEntryOffset;
    }
    if (!found)
    {
        result.status = WupTargetMissing;
    }
    result.error = result.status == WupOk ? ERROR_SUCCESS :
        result.status == WupTargetMissing ? ERROR_NOT_FOUND :
                                            ERROR_INVALID_DATA;
    result.domain = result.status == WupOk ? 0 : 1;
    return result;
}

inline ProcessQuery ResolveProcessQuery()
{
    const auto address = GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "NtQuerySystemInformation");
    ProcessQuery query{};
    static_assert(sizeof(query) == sizeof(address));
    std::memcpy(&query, &address, sizeof(query));
    return query;
}

inline ProcessSnapshot CaptureProcessSnapshot(ProcessQuery query, WupProcessIdentity target) noexcept
{
    if (!query)
    {
        return {WupUnavailable, ERROR_PROC_NOT_FOUND};
    }
    try
    {
        constexpr ULONG maximum = 16 * 1024 * 1024;
        std::vector<unsigned char> buffer(64 * 1024);
        for (unsigned attempt = 0; attempt < 4; ++attempt)
        {
            ULONG returned = 0;
            const auto status = query(5, buffer.data(), static_cast<ULONG>(buffer.size()), &returned);
            if (status == 0)
            {
                if (returned > buffer.size())
                {
                    return {};
                }
                return ParseProcessSnapshot(buffer.data(), returned, target);
            }
            if (static_cast<uint32_t>(status) != 0xC0000004)
            {
                return {WupUnavailable, static_cast<uint32_t>(status), 2};
            }
            const auto next = returned > buffer.size() ? returned : buffer.size() * 2;
            if (next > maximum || next <= buffer.size())
            {
                return {WupUnavailable, ERROR_INSUFFICIENT_BUFFER};
            }
            buffer.resize(next);
        }
        return {WupUnavailable, ERROR_RETRY};
    }
    catch (const std::bad_alloc&)
    {
        return {WupUnavailable, ERROR_NOT_ENOUGH_MEMORY};
    }
}
}
