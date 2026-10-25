#include "../Exports.h"
#include <limits>

namespace
{
bool Add(uint64_t& sum, uint64_t value)
{
    if (value > UINT64_MAX - sum) return false;
    sum += value;
    return true;
}

bool Valid(const WupCpuSnapshot* s, const WupCpuGroup* groups)
{
    if (!s || s->size != sizeof(*s) || s->version != WUP_METRICS_VERSION ||
        (s->source != WupSyntheticCpuSnapshot && s->source != WupNtGroupProcessTimesCompatibility &&
            s->source != WupNtProcessSnapshotCpu) ||
        s->status > WupIdentityChanged ||
        s->reserved || s->process.reserved || !s->clock_id ||
        !s->process.pid || !s->process.creation_filetime_100ns ||
        s->target_present > 1 || (s->status == WupOk && !s->expected_group_count) ||
        s->expected_group_count > WUP_MAX_CPU_GROUPS ||
        s->group_count > WUP_MAX_CPU_GROUPS ||
        s->group_count > s->expected_group_count || (s->group_count && !groups)) return false;
    for (uint32_t i = 0; i < s->group_count; ++i)
    {
        const auto& g = groups[i];
        if (g.size != sizeof(g) || g.version != WUP_METRICS_VERSION ||
            !g.logical_processor_count || g.logical_processor_count > 64) return false;
        for (uint32_t j = 0; j < i; ++j)
            if (groups[j].group_id == g.group_id) return false;
    }
    return true;
}

WupCpuMetric Missing(WupMetricStatus status)
{
    return {status, 0, std::numeric_limits<double>::quiet_NaN()};
}

void SetMissing(WupCpuResult& r, WupMetricStatus status)
{
    r.status = status;
    r.system_utilization = r.process_utilization = r.interrupt_utilization =
        r.other_or_unattributed_utilization = Missing(status);
}
}

uint32_t WupNormalizeCpu(const WupCpuSnapshot* previous, const WupCpuGroup* previous_groups,
    const WupCpuSnapshot* current, const WupCpuGroup* current_groups,
    WupCpuResult* output, uint32_t output_size) noexcept
{
    if (!output || output_size < sizeof(*output) || !Valid(current, current_groups) ||
        (previous && !Valid(previous, previous_groups))) return WUP_E_INVALIDARG;
    WupCpuResult r{};
    r.size = sizeof(r);
    r.version = WUP_METRICS_VERSION;
    r.source = current->source;
    r.process = current->process;
    r.clock_id = current->clock_id;
    r.begin_100ns = previous ? previous->timestamp_100ns : current->timestamp_100ns;
    r.end_100ns = current->timestamp_100ns;
    r.process_utility = Missing(WupUnavailable);
    SetMissing(r, WupWarmup);
    auto finish = [&](WupMetricStatus status) {
        SetMissing(r, status);
        *output = r;
        return WUP_S_OK;
    };
    if (current->status != WupOk) return finish(current->status);
    if (!current->target_present) return finish(WupTargetMissing);
    if (current->group_count != current->expected_group_count) return finish(WupPartial);
    if (!previous) return finish(WupWarmup);
    if (previous->source != current->source || previous->clock_id != current->clock_id ||
        current->timestamp_100ns <= previous->timestamp_100ns) return WUP_E_INVALIDARG;
    if (previous->process.pid != current->process.pid ||
        previous->process.creation_filetime_100ns != current->process.creation_filetime_100ns)
        return finish(WupIdentityChanged);
    if (!previous->target_present || previous->status != WupOk) return finish(WupWarmup);
    if (previous->group_count != previous->expected_group_count ||
        previous->group_count != current->group_count) return finish(WupPartial);

    if (current->process_kernel < previous->process_kernel ||
        current->process_user < previous->process_user) return finish(WupCounterReset);
    uint64_t process = current->process_kernel - previous->process_kernel;
    if (!Add(process, current->process_user - previous->process_user)) return WUP_E_INVALIDARG;
    uint64_t total = 0, busy = 0, interrupt = 0;
    for (uint32_t i = 0; i < current->group_count; ++i)
    {
        const auto& c = current_groups[i];
        const WupCpuGroup* p = nullptr;
        for (uint32_t j = 0; j < previous->group_count; ++j)
            if (previous_groups[j].group_id == c.group_id) p = &previous_groups[j];
        if (!p || p->logical_processor_count != c.logical_processor_count)
            return finish(WupPartial);
        if (c.kernel < p->kernel || c.user < p->user || c.idle < p->idle ||
            c.dpc < p->dpc || c.isr < p->isr)
            return finish(WupCounterReset);
        const uint64_t kernel = c.kernel - p->kernel, user = c.user - p->user;
        const uint64_t idle = c.idle - p->idle;
        uint64_t group_total = kernel;
        uint64_t group_interrupt = c.dpc - p->dpc;
        if (!Add(group_total, user) || !Add(group_interrupt, c.isr - p->isr) ||
            idle > kernel || group_interrupt > kernel - idle ||
            !Add(total, group_total) || !Add(busy, group_total - idle) ||
            !Add(interrupt, group_interrupt))
            return WUP_E_INVALIDARG;
    }
    if (!total) return finish(WupUnavailable);
    auto measured = [total](uint64_t value) -> WupCpuMetric {
        return {WupOk, 0, (static_cast<double>(value) / static_cast<double>(total)) * 100.0};
    };
    r.status = WupOk;
    r.system_utilization = measured(busy);
    r.interrupt_utilization = measured(interrupt);
    if (process > busy - interrupt)
    {
        r.status = WupPartial;
        r.process_utilization = r.other_or_unattributed_utilization = Missing(WupPartial);
    }
    else
    {
        r.process_utilization = measured(process);
        r.other_or_unattributed_utilization = measured(busy - process);
    }
    *output = r;
    return WUP_S_OK;
}
