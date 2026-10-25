#include "..\WinUIProfiler.Native\Metrics\GpuPdhArray.h"
#include <d3d11.h>
#include <dxgi1_2.h>
#include <d3dcompiler.h>
#include <pdhmsg.h>
#include <wrl/client.h>
#include <atomic>
#include <thread>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <algorithm>

using Microsoft::WRL::ComPtr;
bool AcceptHardware(const DXGI_ADAPTER_DESC1& requested, const DXGI_ADAPTER_DESC1& actual)
{
    return !(actual.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) &&
        !(actual.VendorId == 0x1414 && actual.DeviceId == 0x008c) &&
        actual.AdapterLuid.HighPart == requested.AdapterLuid.HighPart &&
        actual.AdapterLuid.LowPart == requested.AdapterLuid.LowPart;
}
bool AdapterGuards()
{
    DXGI_ADAPTER_DESC1 requested{};
    requested.VendorId = 0x8086; requested.DeviceId = 1;
    auto actual = requested;
    if (!AcceptHardware(requested, actual)) return false;
    actual.Flags = DXGI_ADAPTER_FLAG_SOFTWARE;
    if (AcceptHardware(requested, actual)) return false;
    actual = requested; actual.VendorId = 0x1414; actual.DeviceId = 0x008c;
    if (AcceptHardware(requested, actual)) return false;
    actual = requested; actual.AdapterLuid.LowPart++;
    return !AcceptHardware(requested, actual);
}
class OwnedGpuWorkload
{
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    ComPtr<ID3D11ComputeShader> shader;
    ComPtr<ID3D11Buffer> buffer, staging;
    ComPtr<ID3D11UnorderedAccessView> view;
    ComPtr<ID3D11Query> completed;
    std::atomic<bool> stop{false};
    std::atomic<HRESULT> workerStatus{S_OK};
    std::atomic<uint32_t> dispatches{0};
    std::thread worker;
public:
    ~OwnedGpuWorkload() { Stop(); }
    void Stop() { stop = true; if (worker.joinable()) worker.join(); }
    HRESULT Start()
    {
        const D3D_FEATURE_LEVEL levels[] = {D3D_FEATURE_LEVEL_11_0};
        ComPtr<IDXGIFactory1> factory;
        HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&factory));
        if (FAILED(hr)) return hr;
        ComPtr<IDXGIAdapter1> adapter1;
        DXGI_ADAPTER_DESC1 description{};
        bool found = false;
        for (uint32_t index = 0; index < 16; ++index)
        {
            adapter1.Reset();
            hr = factory->EnumAdapters1(index, &adapter1);
            if (hr == DXGI_ERROR_NOT_FOUND) break;
            if (FAILED(hr)) return hr;
            if (FAILED(hr = adapter1->GetDesc1(&description))) return hr;
            std::printf("DXGI adapter=%u name=%ls vendor=%04X device=%04X LUID=%08X:%08X flags=%u\n",
                index, description.Description, description.VendorId, description.DeviceId,
                static_cast<uint32_t>(description.AdapterLuid.HighPart), description.AdapterLuid.LowPart, description.Flags);
            if (description.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
            hr = D3D11CreateDevice(adapter1.Get(), D3D_DRIVER_TYPE_UNKNOWN, nullptr, 0,
                levels, 1, D3D11_SDK_VERSION, &device, nullptr, &context);
            std::printf(" Explicit hardware D3D11 create HRESULT=0x%08X\n", static_cast<uint32_t>(hr));
            if (SUCCEEDED(hr))
            {
                ComPtr<IDXGIDevice> actualDevice;
                ComPtr<IDXGIAdapter> actualAdapter;
                ComPtr<IDXGIAdapter1> actualAdapter1;
                DXGI_ADAPTER_DESC1 actual{};
                if (FAILED(hr = device.As(&actualDevice)) || FAILED(hr = actualDevice->GetAdapter(&actualAdapter)) ||
                    FAILED(hr = actualAdapter.As(&actualAdapter1)) || FAILED(hr = actualAdapter1->GetDesc1(&actual))) return hr;
                std::printf(" Actual device adapter=%ls vendor=%04X device=%04X LUID=%08X:%08X flags=%u\n",
                    actual.Description, actual.VendorId, actual.DeviceId,
                    static_cast<uint32_t>(actual.AdapterLuid.HighPart), actual.AdapterLuid.LowPart, actual.Flags);
                // Basic Render Driver identity is conservatively excluded even if enumeration omitted SOFTWARE.
                if (!AcceptHardware(description, actual))
                {
                    std::printf(" REJECT: software/basic-render or requested/actual adapter identity mismatch; not hardware evidence\n");
                    context.Reset(); device.Reset();
                    continue;
                }
                found = true;
                break;
            }
        }
        if (!found) return DXGI_ERROR_UNSUPPORTED;
        const char source[] =
            "RWStructuredBuffer<uint> output : register(u0);"
            "[numthreads(256,1,1)] void main(uint3 id : SV_DispatchThreadID) {"
            "uint v=id.x+1; [loop] for(uint i=0;i<256;i++)"
            "v=(v*1664525u+1013904223u)^(v>>7); output[id.x]=v;}";
        ComPtr<ID3DBlob> code, errors;
        hr = D3DCompile(source, sizeof(source) - 1, "OwnedGpuWorkload", nullptr, nullptr,
            "main", "cs_5_0", D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &code, &errors);
        if (FAILED(hr))
        {
            if (errors) std::printf("Shader error: %.*s\n", static_cast<int>(errors->GetBufferSize()),
                static_cast<const char*>(errors->GetBufferPointer()));
            return hr;
        }
        if (FAILED(hr = device->CreateComputeShader(code->GetBufferPointer(), code->GetBufferSize(), nullptr, &shader))) return hr;
        D3D11_BUFFER_DESC desc{};
        desc.ByteWidth = 65536 * sizeof(uint32_t); desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS; desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        desc.StructureByteStride = sizeof(uint32_t);
        if (FAILED(hr = device->CreateBuffer(&desc, nullptr, &buffer))) return hr;
        D3D11_UNORDERED_ACCESS_VIEW_DESC uav{};
        uav.ViewDimension = D3D11_UAV_DIMENSION_BUFFER; uav.Buffer.NumElements = 65536;
        if (FAILED(hr = device->CreateUnorderedAccessView(buffer.Get(), &uav, &view))) return hr;
        desc.Usage = D3D11_USAGE_STAGING; desc.BindFlags = 0; desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        if (FAILED(hr = device->CreateBuffer(&desc, nullptr, &staging))) return hr;
        const D3D11_QUERY_DESC query{D3D11_QUERY_EVENT, 0};
        if (FAILED(hr = device->CreateQuery(&query, &completed))) return hr;
        worker = std::thread([this] {
            context->CSSetShader(shader.Get(), nullptr, 0);
            auto uav = view.Get();
            context->CSSetUnorderedAccessViews(0, 1, &uav, nullptr);
            const auto deadline = GetTickCount64() + 15000;
            while (!stop && GetTickCount64() < deadline && dispatches < 10000)
            {
                context->Dispatch(256, 1, 1);
                context->End(completed.Get()); context->Flush();
                const auto waitDeadline = GetTickCount64() + 2000;
                BOOL done = FALSE;
                HRESULT status = S_FALSE;
                while (status == S_FALSE && GetTickCount64() < waitDeadline)
                {
                    status = context->GetData(completed.Get(), &done, sizeof(done), 0);
                    if (status == S_FALSE) Sleep(1);
                }
                if (status != S_OK || !done)
                { workerStatus = FAILED(status) ? status : HRESULT_FROM_WIN32(WAIT_TIMEOUT); break; }
                ++dispatches;
            }
            context->ClearState();
        });
        return S_OK;
    }
    HRESULT Finish()
    {
        Stop();
        if (FAILED(workerStatus)) return workerStatus;
        if (!dispatches) return E_FAIL;
        context->CopyResource(staging.Get(), buffer.Get());
        D3D11_MAPPED_SUBRESOURCE mapped{};
        const auto hr = context->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped);
        if (FAILED(hr)) return hr;
        const auto actual = *static_cast<uint32_t*>(mapped.pData);
        context->Unmap(staging.Get(), 0);
        uint32_t expected = 1;
        for (uint32_t i = 0; i < 256; ++i) expected = (expected * 1664525u + 1013904223u) ^ (expected >> 7);
        std::printf("Completed GPU dispatches=%u readback=%08X expected=%08X\n", dispatches.load(), actual, expected);
        return actual == expected ? S_OK : E_FAIL;
    }
};

struct Query
{
    PDH_HQUERY handle = nullptr;
    ~Query() { if (handle) PdhCloseQuery(handle); }
};
struct Target
{
    HANDLE handle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE, FALSE, GetCurrentProcessId());
    ~Target() { if (handle) CloseHandle(handle); }
};
bool Same(const WupGpuRawEngine& a, const WupGpuRawEngine& b)
{
    return a.luid_high == b.luid_high && a.luid_low == b.luid_low && a.physical == b.physical && a.engine == b.engine;
}
PDH_RAW_COUNTER Raw(const WupGpuRawEngine& row)
{
    PDH_RAW_COUNTER result{};
    result.CStatus = row.pdh_status;
    result.TimeStamp = {static_cast<DWORD>(row.timestamp_filetime_100ns), static_cast<DWORD>(row.timestamp_filetime_100ns >> 32)};
    result.FirstValue = static_cast<LONGLONG>(row.running_time); result.SecondValue = static_cast<LONGLONG>(row.timer);
    return result;
}
bool Calculate(PDH_HCOUNTER counter, const WupGpuSnapshot& a, const WupGpuSnapshot& b, double& maximum)
{
    maximum = 0;
    if (!a.row_count || a.row_count != b.row_count) return false;
    for (uint32_t i = 0; i < b.row_count; ++i)
    {
        const WupGpuRawEngine* old = nullptr;
        for (uint32_t j = 0; j < a.row_count; ++j) if (Same(a.rows[j], b.rows[i])) old = &a.rows[j];
        if (!old) return false;
        auto current = Raw(b.rows[i]), previous = Raw(*old);
        PDH_FMT_COUNTERVALUE formatted{};
        const auto error = PdhCalculateCounterFromRawValue(counter, PDH_FMT_DOUBLE | PDH_FMT_NOCAP100,
            &current, &previous, &formatted);
        if (error || formatted.CStatus > PDH_CSTATUS_NEW_DATA || !std::isfinite(formatted.doubleValue)) return false;
        maximum = (std::max)(maximum, formatted.doubleValue);
    }
    return true;
}
bool FormattedMaximum(PDH_HCOUNTER counter, uint32_t pid, double& maximum, uint32_t& selected)
{
    maximum = 0; selected = 0;
    for (uint32_t attempt = 0; attempt < 3; ++attempt)
    {
        DWORD bytes = 0, count = 0;
        auto error = PdhGetFormattedCounterArrayW(counter, PDH_FMT_DOUBLE | PDH_FMT_NOCAP100, &bytes, &count, nullptr);
        if (error != PDH_MORE_DATA || !bytes || bytes > 1024 * 1024) return false;
        const DWORD capacity = bytes;
        auto rows = static_cast<PDH_FMT_COUNTERVALUE_ITEM_W*>(HeapAlloc(GetProcessHeap(), 0, capacity));
        if (!rows) return false;
        error = PdhGetFormattedCounterArrayW(counter, PDH_FMT_DOUBLE | PDH_FMT_NOCAP100, &bytes, &count, rows);
        bool valid = !error && bytes <= capacity && count <= 4096 && count <= bytes / sizeof(*rows);
        if (valid)
        {
            const auto begin = reinterpret_cast<uintptr_t>(rows), end = begin + bytes;
            for (uint32_t i = 0; i < count; ++i)
            {
                const auto name = reinterpret_cast<uintptr_t>(rows[i].szName);
                if (name < begin + count * sizeof(*rows) || name >= end || name % alignof(wchar_t)) { valid = false; break; }
                const auto available = (end - name) / sizeof(wchar_t);
                uint32_t length = 0;
                while (length < available && length <= 255 && rows[i].szName[length]) ++length;
                wup::GpuInstance instance{};
                if (length >= available || length > 255 || !wup::ParseGpuInstance(rows[i].szName, length, instance))
                { valid = false; break; }
                if (instance.pid != pid) continue;
                if (!instance.has_engine || instance.duplicate_index || rows[i].FmtValue.CStatus > PDH_CSTATUS_NEW_DATA ||
                    !std::isfinite(rows[i].FmtValue.doubleValue)) { valid = false; break; }
                ++selected;
                maximum = (std::max)(maximum, rows[i].FmtValue.doubleValue);
            }
        }
        HeapFree(GetProcessHeap(), 0, rows);
        if (error != PDH_MORE_DATA) return valid && selected != 0;
    }
    return false;
}

int wmain(int argc, wchar_t** argv)
{
    if (!AdapterGuards()) { std::printf("FAIL: adapter identity/software guard fixtures\n"); return 1; }
    if (argc == 2 && wcscmp(argv[1], L"--adapter-guards-only") == 0)
    { std::printf("PASSED: adapter guard fixtures only; NOT hardware validation\n"); return 0; }
    if (argc != 2 || wcslen(argv[1]) < 3 || argv[1][1] != L':' || argv[1][2] != L'\\') return 1;
    OwnedGpuWorkload workload;
    const auto hardware = workload.Start();
    std::printf("Owned hardware workload Start HRESULT=0x%08X\n", static_cast<uint32_t>(hardware));
    if (FAILED(hardware)) { std::printf("UNAVAILABLE: owned D3D11 hardware workload; no hardware validation/zero/parity result\n"); return 2; }
    HMODULE module = LoadLibraryExW(argv[1], nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!module) { std::printf("FAIL DLL Win32=%u\n", GetLastError()); return 1; }
    decltype(&WupCaptureGpu) capture{};
    decltype(&WupNormalizeGpu) normalize{};
    auto address = GetProcAddress(module, "WupCaptureGpu"); std::memcpy(&capture, &address, sizeof(capture));
    address = GetProcAddress(module, "WupNormalizeGpu"); std::memcpy(&normalize, &address, sizeof(normalize));
    int failures = 0;
#define CHECK(x) do { if (!(x)) { std::printf("FAIL GPU line %d: %s\n", __LINE__, #x); ++failures; } } while (0)
    if (!capture || !normalize) { FreeLibrary(module); return 1; }
    Target target;
    FILETIME created{}, exit{}, kernel{}, user{};
    CHECK(target.handle && GetProcessTimes(target.handle, &created, &exit, &kernel, &user));
    const WupGpuRequest request{sizeof(request), WUP_GPU_VERSION, {GetCurrentProcessId(), 0,
        (static_cast<uint64_t>(created.dwHighDateTime) << 32) | created.dwLowDateTime}};
    std::printf("Selected owned PID=%u creation=%llu\n", request.process.pid, request.process.creation_filetime_100ns);
    Query query;
    PDH_HCOUNTER counter = nullptr;
    CHECK(PdhOpenQueryW(nullptr, 0, &query.handle) == ERROR_SUCCESS);
    CHECK(PdhAddEnglishCounterW(query.handle, L"\\GPU Engine(*)\\Utilization Percentage", 0, &counter) == ERROR_SUCCESS);
    WupGpuSnapshot a{}, b{}, oracleA{}, oracleB{};
    WupGpuResult result{};
    bool measured = false;
    for (uint32_t attempt = 0; attempt < 3 && !measured; ++attempt)
    {
        Sleep(300);
        LONGLONG oracleBegin = 0, oracleEnd = 0;
        CHECK(PdhCollectQueryDataWithTime(query.handle, &oracleBegin) == ERROR_SUCCESS);
        uint32_t observed = 0, malformed = 0;
        CHECK(wup::ReadGpuPdhArray(counter, request.process.pid, oracleA, observed, malformed) == ERROR_SUCCESS);
        CHECK(capture(target.handle, &request, &a, sizeof(a)) == WUP_S_OK);
        std::printf("GPU first attempt=%u status=%u rows=%u error=0x%08X\n", attempt, a.status, a.row_count, a.pdh_error);
        if (a.status != WupOk || !a.row_count) continue;
        CHECK(normalize(nullptr, &a, &result, sizeof(result)) == WUP_S_OK);
        CHECK(result.status == WupWarmup && std::isnan(result.utilization.percent));
        Sleep(200);
        CHECK(capture(target.handle, &request, &b, sizeof(b)) == WUP_S_OK);
        CHECK(normalize(&a, &b, &result, sizeof(result)) == WUP_S_OK);
        std::printf("GPU native status=%u rows=%u/%u max=%.12f interval=%llu..%llu\n",
            result.status, a.row_count, b.row_count, result.utilization.percent, result.begin_100ns, result.end_100ns);
        for (uint32_t i = 0; i < b.row_count && i < 16; ++i)
            std::printf(" row luid=%08X:%08X phys=%u eng=%u duplicate=%u status=%u N=%llu D=%llu timestamp=%llu\n",
                b.rows[i].luid_high, b.rows[i].luid_low, b.rows[i].physical, b.rows[i].engine,
                b.rows[i].duplicate_index, b.rows[i].status, b.rows[i].running_time, b.rows[i].timer, b.rows[i].timestamp_filetime_100ns);
        if (result.status != WupOk) continue;
        CHECK(result.utilization.percent > 0);
        double nativeOracle = 0;
        CHECK(Calculate(counter, a, b, nativeOracle));
        CHECK(std::abs(nativeOracle - result.utilization.percent) < 1e-8);
        CHECK(PdhCollectQueryDataWithTime(query.handle, &oracleEnd) == ERROR_SUCCESS);
        CHECK(wup::ReadGpuPdhArray(counter, request.process.pid, oracleB, observed, malformed) == ERROR_SUCCESS);
        double oracleRaw = 0, oracleFormatted = 0;
        uint32_t selected = 0;
        CHECK(Calculate(counter, oracleA, oracleB, oracleRaw));
        CHECK(FormattedMaximum(counter, request.process.pid, oracleFormatted, selected));
        CHECK(oracleRaw > 0 && oracleFormatted > 0 && std::abs(oracleRaw - oracleFormatted) < 1e-8);
        std::printf("PDH native-raw calculator max=%.12f; independent query raw=%.12f formatted=%.12f rows=%u interval=%lld..%lld\n",
            nativeOracle, oracleRaw, oracleFormatted, selected, oracleBegin, oracleEnd);
        measured = true;
    }
    CHECK(measured);
    CHECK(workload.Finish() == S_OK);
    FreeLibrary(module);
    std::printf("%s: hardware selected-process GPU (%d failures); independent query windows differ, no timing/parity claim\n",
        failures ? "FAILED" : "PASSED", failures);
    return failures ? 1 : 0;
}
