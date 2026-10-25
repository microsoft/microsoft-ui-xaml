#include "../WinUIProfiler.Native/Metrics/GpuMemoryCore.h"
#include <cstdio>
#if __has_include("../WinUIProfiler.Native/Metrics/GpuMemorySourceApi.h")
#include "../WinUIProfiler.Native/Metrics/GpuMemorySourceApi.h"
#define HAVE_SOURCE 1
#endif
static int checks, failures;
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; std::printf("FAIL %d: %s\n", __LINE__, #x); } } while (0)
#ifdef HAVE_SOURCE
struct State {
    uint32_t open_hr{}, count_hr{}, count{2}, calls{}, closed{}, identities{};
    bool supported{true};
    uint32_t pre_status{WupOk}, post_status{WupOk};
    uint64_t begin{100}, end{110};
    wup::GpuMemoryFamily d{0, 1, 1234, 123}, s{0, 1, 0, 0};
};
static uint32_t Open(void* p, uint32_t high, uint32_t low, uint32_t* error)
{ auto& s = *static_cast<State*>(p); CHECK(high == 9 && low == 8); *error = 0; return s.open_hr; }
static uint32_t Count(void* p, uint32_t* count)
{ auto& s = *static_cast<State*>(p); *count = s.count; return s.count_hr; }
static bool Supported(void* p) { return static_cast<State*>(p)->supported; }
static wup::GpuMemoryFamily Query(void* p, uint32_t physical, uint32_t pid, bool shared)
{ auto& s = *static_cast<State*>(p); ++s.calls; CHECK(physical == 1 && pid == 12); return shared ? s.s : s.d; }
static void Close(void* p) { ++static_cast<State*>(p)->closed; }
static uint32_t Identity(void* p, void*, WupProcessIdentity, uint32_t* error)
{ auto& s = *static_cast<State*>(p); *error = 0; return ++s.identities == 1 ? s.pre_status : s.post_status; }
static uint64_t Tick(void* p)
{ auto& s = *static_cast<State*>(p); return s.identities ? s.end : s.begin; }
#endif
int main()
{
#ifndef HAVE_SOURCE
    CHECK(false && "real source boundary missing");
#else
    for (int scenario = 0; scenario < 19; ++scenario)
    {
        State state;
        if (scenario == 1) state.open_hr = 0x8007007Eu;
        if (scenario == 2) state.open_hr = 0x887A0002u;
        if (scenario == 3) state.count_hr = 0x80004005u;
        if (scenario == 4) state.count = 0;
        if (scenario == 5) state.count = 1;
        if (scenario == 6) state.supported = false;
        if (scenario == 7) state.d.hresult = 0x887A0005u;
        if (scenario == 8) state.d.process_query_succeeded = 0;
        if (scenario == 9) state.post_status = WupIdentityChanged;
        if (scenario == 10) state.post_status = WupTargetMissing;
        if (scenario == 11) state.end = 99;
        if (scenario == 12) state.end = UINT64_MAX;
        if (scenario == 13) state.pre_status = WupTargetMissing;
        if (scenario == 14) state.pre_status = WupIdentityChanged;
        if (scenario == 15) state.pre_status = WupUnavailable;
        if (scenario == 16) state.d.process_query_succeeded = state.s.process_query_succeeded = 0;
        if (scenario == 17) state.s.hresult = 0x80070005u;
        wup::detail::GpuMemorySourceApi api{
            &state, Open, Count, Supported, Query, Close, Identity, Tick};
        WupGpuMemoryRequest request{sizeof(request), 1, {12, 0, 34}, 9, 8, 1, 0};
        WupGpuMemoryResult result{};
        if (scenario == 18)
        {
            api.query = nullptr;
            result.size = 123;
            CHECK(wup::detail::CaptureGpuMemoryWithApi(nullptr, &request, &result, sizeof(result), api) == WUP_E_INVALIDARG);
            CHECK(result.size == 123 && !state.closed && !state.identities);
            continue;
        }
        CHECK(wup::detail::CaptureGpuMemoryWithApi(nullptr, &request, &result, sizeof(result), api) == WUP_S_OK);
        if (scenario >= 13 && scenario <= 15)
        {
            CHECK(!state.closed && !state.calls && state.identities == 1);
            CHECK(result.status == state.pre_status && result.dedicated_committed.value == UINT64_MAX);
            continue;
        }
        CHECK(state.closed == 1 && state.identities == 2);
        if (scenario == 0) CHECK(result.status == WupOk && result.shared_resident.value == 0);
        if (scenario >= 1 && scenario <= 6)
        { CHECK(result.status == WupUnavailable && !state.calls); CHECK(result.dedicated_committed.value == UINT64_MAX); }
        if (scenario == 1 || scenario == 2) CHECK(result.dedicated_hresult == state.open_hr);
        if (scenario == 3) CHECK(result.dedicated_hresult == state.count_hr);
        if (scenario == 7 || scenario == 8)
        { CHECK(result.status == WupPartial); CHECK(result.shared_committed.value == 0); }
        if (scenario == 7) CHECK(result.dedicated_hresult == 0x887A0005u);
        if (scenario == 9) CHECK(result.status == WupIdentityChanged);
        if (scenario == 10) CHECK(result.status == WupTargetMissing);
        if (scenario >= 9 && scenario <= 12) CHECK(result.dedicated_resident.value == UINT64_MAX);
        if (scenario == 11 || scenario == 12) CHECK(result.status == WupUnavailable && result.win32_error != 0);
        if (scenario == 16) CHECK(result.status == WupUnavailable && !result.process_query_flags);
        if (scenario == 17) CHECK(result.status == WupPartial && result.shared_hresult == 0x80070005u);
    }
#endif
    std::printf("GPU memory source: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
