#include "GpuFoundation.h"
#include <string_view>
#include <cmath>
#include <limits>
namespace wup
{
namespace
{
bool ConsumeLiteral(std::wstring_view& rest, std::wstring_view value)
{
    if (rest.substr(0, value.size()) != value)
    {
        return false;
    }
    rest.remove_prefix(value.size());
    return true;
}

bool ConsumeNumber(std::wstring_view& rest, uint32_t& value, bool hex)
{
    uint64_t result = 0;
    uint32_t digits = 0;
    while (!rest.empty())
    {
        const auto c = rest.front();
        int d = c >= L'0' && c <= L'9'    ? c - L'0' :
            hex && c >= L'a' && c <= L'f' ? c - L'a' + 10 :
            hex && c >= L'A' && c <= L'F' ? c - L'A' + 10 :
                                            -1;
        if (d < 0 || (!hex && d > 9))
        {
            break;
        }
        if (result > (UINT32_MAX - static_cast<uint32_t>(d)) / (hex ? 16u : 10u))
        {
            return false;
        }
        result = result * (hex ? 16u : 10u) + d;
        rest.remove_prefix(1);
        ++digits;
    }
    if (!digits || (hex && digits != 8))
    {
        return false;
    }
    value = static_cast<uint32_t>(result);
    return true;
}

bool SameEngine(const GpuInstance& a, const GpuInstance& b)
{
    return a.luid_high == b.luid_high && a.luid_low == b.luid_low && a.physical == b.physical && a.engine == b.engine;
}
}

bool ParseGpuInstance(const wchar_t* text, uint32_t length, GpuInstance& output)
{
    if (!text || !length || length > 255)
    {
        return false;
    }
    std::wstring_view rest(text, length);
    GpuInstance result{};
    if (!ConsumeLiteral(rest, L"pid_") || !ConsumeNumber(rest, result.pid, false) || !result.pid ||
        !ConsumeLiteral(rest, L"_luid_0x") || !ConsumeNumber(rest, result.luid_high, true) ||
        !ConsumeLiteral(rest, L"_0x") || !ConsumeNumber(rest, result.luid_low, true) ||
        !ConsumeLiteral(rest, L"_phys_") || !ConsumeNumber(rest, result.physical, false))
    {
        return false;
    }
    if (rest.substr(0, 5) == L"_eng_")
    {
        if (!ConsumeLiteral(rest, L"_eng_") || !ConsumeNumber(rest, result.engine, false) ||
            !ConsumeLiteral(rest, L"_engtype_"))
        {
            return false;
        }
        uint32_t labelLength = 0;
        while (!rest.empty() && rest.front() != L'#')
        {
            const auto c = rest.front();
            if (!((c >= L'A' && c <= L'Z') || (c >= L'a' && c <= L'z') || (c >= L'0' && c <= L'9') || c == L'_') ||
                ++labelLength > 63)
            {
                return false;
            }
            rest.remove_prefix(1);
        }
        if (!labelLength)
        {
            return false;
        }
        result.has_engine = true;
    }
    if (!rest.empty() &&
        (!ConsumeLiteral(rest, L"#") || !ConsumeNumber(rest, result.duplicate_index, false) || !result.duplicate_index))
    {
        return false;
    }
    if (!rest.empty())
    {
        return false;
    }
    output = result;
    return true;
}

GpuSummary AggregateGpu(const GpuContext* rows, uint32_t count, uint32_t selected_pid)
{
    GpuSummary result{};
    result.status = WupUnavailable;
    result.percent = std::numeric_limits<double>::quiet_NaN();
    result.dedicated_committed = {WupUnavailable, WupBytes, UINT64_MAX};
    result.shared_committed = {WupUnavailable, WupBytes, UINT64_MAX};
    result.dedicated_resident = {WupUnavailable, WupBytes, UINT64_MAX};
    result.shared_resident = {WupUnavailable, WupBytes, UINT64_MAX};
    if (!count)
    {
        return result;
    }
    result.status = WupPartial;
    if (!rows || count > MaxGpuRows || !selected_pid)
    {
        return result;
    }
    result.clock_id = rows[0].clock_id;
    result.begin_100ns = rows[0].begin_100ns;
    result.end_100ns = rows[0].end_100ns;
    double totals[MaxGpuRows]{};
    uint32_t representatives[MaxGpuRows]{};
    for (uint32_t i = 0; i < count; ++i)
    {
        const auto& row = rows[i];
        if (row.instance.pid != selected_pid || !row.instance.has_engine || row.instance.duplicate_index ||
            row.end_100ns <= row.begin_100ns || row.status > WupIdentityChanged || !row.clock_id ||
            row.clock_id != result.clock_id || row.begin_100ns != rows[0].begin_100ns ||
            row.end_100ns != rows[0].end_100ns)
        {
            return result;
        }
        if (row.status != WupOk)
        {
            result.status = row.status;
            return result;
        }
        if (!std::isfinite(row.percent) || row.percent < 0 || row.percent > 100)
        {
            return result;
        }
        uint32_t engine = 0;
        for (; engine < result.engine_count; ++engine)
        {
            if (SameEngine(row.instance, rows[representatives[engine]].instance))
            {
                break;
            }
        }
        for (uint32_t j = 0; j < i; ++j)
        {
            if (SameEngine(row.instance, rows[j].instance) && row.context_id == rows[j].context_id)
            {
                return result;
            }
        }
        if (engine == result.engine_count)
        {
            representatives[result.engine_count++] = i;
        }
        totals[engine] += row.percent;
        if (totals[engine] > 100)
        {
            return result;
        }
    }
    result.percent = 0;
    for (uint32_t i = 0; i < result.engine_count; ++i)
    {
        if (totals[i] > result.percent)
        {
            result.percent = totals[i];
        }
    }
    result.status = WupOk;
    return result;
}
}
