#include "GpuPdhArray.h"
#include "GpuSourceApi.h"
#include <pdhmsg.h>

PDH_STATUS wup::ReadGpuPdhArray(PDH_HCOUNTER counter, uint32_t pid, WupGpuSnapshot& result,
    uint32_t& observed, uint32_t& malformed)
{
    return detail::ReadGpuPdhArrayWithApi(counter, pid, result, observed, malformed, PdhGetRawCounterArrayW, false);
}

PDH_STATUS wup::detail::ReadGpuPdhArrayWithApi(PDH_HCOUNTER counter, uint32_t pid, WupGpuSnapshot& result,
    uint32_t& observed, uint32_t& malformed, decltype(&PdhGetRawCounterArrayW) read, bool require_engine)
{
    observed = malformed = 0;
    result.row_count = 0;
    result.status = WupUnavailable;
    for (uint32_t attempt = 0; attempt < 3; ++attempt)
    {
        DWORD bytes = 0, count = 0;
        auto error = read(counter, &bytes, &count, nullptr);
        if (error != PDH_MORE_DATA)
            return error == ERROR_SUCCESS && (bytes || count) ? PDH_INVALID_BUFFER : error;
        if (!bytes || bytes > 1024 * 1024) return PDH_INVALID_BUFFER;
        const DWORD capacity = bytes;
        auto items = static_cast<PDH_RAW_COUNTER_ITEM_W*>(HeapAlloc(GetProcessHeap(), 0, capacity));
        if (!items) return ERROR_NOT_ENOUGH_MEMORY;
        error = read(counter, &bytes, &count, items);
        if (error == ERROR_SUCCESS && bytes <= capacity && count <= 4096 &&
            count <= bytes / sizeof(*items))
        {
            observed = count;
            const auto begin = reinterpret_cast<uintptr_t>(items);
            const auto end = begin + bytes;
            for (uint32_t i = 0; i < count; ++i)
            {
                const auto name = reinterpret_cast<uintptr_t>(items[i].szName);
                if (name < begin + count * sizeof(*items) || name >= end || name % alignof(wchar_t))
                { ++malformed; continue; }
                const auto available = (end - name) / sizeof(wchar_t);
                uint32_t length = 0;
                while (length < available && length <= 255 && items[i].szName[length]) ++length;
                GpuInstance instance{};
                if (length > 255 || length >= available || !ParseGpuInstance(items[i].szName, length, instance))
                { ++malformed; continue; }
                if (instance.pid != pid) continue;
                if (!instance.has_engine) { if (require_engine) ++malformed; continue; }
                if (result.row_count == WUP_MAX_GPU_ROWS) { error = PDH_INVALID_BUFFER; break; }
                auto& row = result.rows[result.row_count++];
                const auto& raw = items[i].RawValue;
                row = {instance.luid_high, instance.luid_low, instance.physical, instance.engine,
                    instance.duplicate_index, WupOk, raw.CStatus, 0,
                    static_cast<uint64_t>(raw.FirstValue), static_cast<uint64_t>(raw.SecondValue),
                    (static_cast<uint64_t>(raw.TimeStamp.dwHighDateTime) << 32) | raw.TimeStamp.dwLowDateTime};
                if (raw.CStatus != PDH_CSTATUS_VALID_DATA && raw.CStatus != PDH_CSTATUS_NEW_DATA)
                    row.status = WupUnavailable;
                if (raw.FirstValue < 0 || raw.SecondValue < 0 || instance.duplicate_index ||
                    !row.timestamp_filetime_100ns)
                    row.status = WupPartial;
                for (uint32_t j = 0; j + 1 < result.row_count; ++j)
                {
                    auto& previous = result.rows[j];
                    if (previous.luid_high == row.luid_high && previous.luid_low == row.luid_low &&
                        previous.physical == row.physical && previous.engine == row.engine)
                    {
                        previous.status = row.status = WupPartial;
                        previous.running_time = previous.timer = 0;
                    }
                }
                if (row.status != WupOk) row.running_time = row.timer = 0;
            }
            result.status = malformed ? WupPartial : result.row_count ? WupOk : WupUnavailable;
            for (uint32_t i = 0; i < result.row_count; ++i)
                if (result.rows[i].status != WupOk) result.status = WupPartial;
            if (malformed) error = PDH_INVALID_BUFFER;
        }
        else if (error == ERROR_SUCCESS) error = PDH_INVALID_BUFFER;
        HeapFree(GetProcessHeap(), 0, items);
        if (error == PDH_MORE_DATA) continue;
        if (error != ERROR_SUCCESS)
        { result.row_count = 0; result.status = WupUnavailable; for (auto& row : result.rows) row = {}; }
        return error;
    }
    return PDH_MORE_DATA;
}
