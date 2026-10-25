#include <windows.h>
#include <ocidl.h>
#include <wrl.h>
#include <cstdio>
#include <algorithm>
#include <cstring>
#include "..\WinUIProfiler.Tap\Tap.h"
#include "..\WinUIProfiler.Tap\PipeServer.h"

extern "C" HRESULT __stdcall DllGetClassObject(REFCLSID, REFIID, void**);
extern "C" HRESULT __stdcall DllCanUnloadNow();

namespace
{
    int checks{}, failures{};
    void Check(bool value, const char* name)
    {
        ++checks;
        if (!value) { ++failures; std::printf("FAIL: %s\n", name); }
    }

    using namespace WinUIProfiler;
    using namespace WinUIProfiler::Tap;
    using Microsoft::WRL::ComPtr;
    const GUID RequestId{0x00112233, 0x4455, 0x6677, {0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff}};

    class Runtime final : public Microsoft::WRL::RuntimeClass<
        Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
        IWinUIProfilerRuntimeDiagnostics, Microsoft::WRL::FtmBase>
    {
    public:
        ProcessKey Target{};
        HRESULT STDMETHODCALLTYPE GetProfilerProtocolInfo(uint32_t version, uint32_t size,
            ProtocolInfo* info) override
        {
            if (!info || version != ProtocolVersion || size != sizeof(*info)) return E_INVALIDARG;
            *info = {};
            info->Size = sizeof(*info);
            info->Version = ProtocolVersion;
            info->ProducerEpoch = RequestId;
            info->Process = {Target.Pid, 1, static_cast<int64_t>(Target.CreationTime), Target.Instance};
            LARGE_INTEGER frequency{};
            QueryPerformanceFrequency(&frequency);
            info->QpcFrequency = frequency.QuadPart;
#ifdef _M_ARM64
            info->Architecture = 1;
#endif
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE GetProfilerSequenceCut(uint32_t, uint16_t, uint32_t,
            SequenceCut*) override { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE GetProfilerDispatcher(uint32_t, uint32_t, IInspectable**) override { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE RequestProfilerCapture(uint32_t, uint32_t, GUID) override { return E_NOTIMPL; }
    };

    bool Transfer(HANDLE pipe, uint8_t* bytes, DWORD length, bool write)
    {
        const auto deadline = GetTickCount64() + 5000;
        while (length)
        {
            Handle event(CreateEventW(nullptr, TRUE, FALSE, nullptr));
            OVERLAPPED operation{};
            operation.hEvent = event.Get();
            DWORD transferred{};
            const BOOL immediate = write ? WriteFile(pipe, bytes, length, &transferred, &operation) :
                ReadFile(pipe, bytes, length, &transferred, &operation);
            if (!immediate)
            {
                if (GetLastError() != ERROR_IO_PENDING) return false;
                const auto now = GetTickCount64();
                if (WaitForSingleObject(event.Get(), now >= deadline ? 0 : static_cast<DWORD>(deadline - now)) != WAIT_OBJECT_0)
                {
                    CancelIoEx(pipe, &operation);
                    GetOverlappedResult(pipe, &operation, &transferred, TRUE);
                    return false;
                }
                if (!GetOverlappedResult(pipe, &operation, &transferred, FALSE)) return false;
            }
            if (!transferred) return false;
            bytes += transferred;
            length -= transferred;
        }
        return true;
    }

    std::vector<uint8_t> Frame(uint16_t type, const std::string& body, GUID id = RequestId)
    {
        auto frame = EncodeAcknowledgement(id, CommandStatus::Completed, GUID_NULL, "");
        frame.resize(28 + body.size());
        for (size_t i = 0; i < 4; ++i) frame[i] = static_cast<uint8_t>(body.size() >> (8 * i));
        frame[6] = static_cast<uint8_t>(type);
        frame[7] = static_cast<uint8_t>(type >> 8);
        std::copy(body.begin(), body.end(), frame.begin() + 28);
        return frame;
    }

    bool Exchange(HANDLE pipe, std::vector<uint8_t> frame, Header& header, std::string& body)
    {
        if (!Transfer(pipe, frame.data(), static_cast<DWORD>(frame.size()), true)) return false;
        frame.resize(28);
        if (!Transfer(pipe, frame.data(), 28, false) || FAILED(DecodeHeader(frame.data(), 28, header))) return false;
        frame.resize(28 + header.BodyLength);
        if (!Transfer(pipe, frame.data() + 28, header.BodyLength, false)) return false;
        body.assign(frame.begin() + 28, frame.end());
        return true;
    }

    void HelloServerTest(bool authenticate, bool withRuntime = true)
    {
        Bootstrap bootstrap{};
        GUID unique{};
        CoCreateGuid(&unique);
        wchar_t text[39]{};
        StringFromGUID2(unique, text, ARRAYSIZE(text));
        bootstrap.PipeName = L"WinUIProfiler-";
        for (const wchar_t ch : text)
            if (ch && ch != L'{' && ch != L'}' && ch != L'-') bootstrap.PipeName += ch;
        bootstrap.Session = RequestId;
        FILETIME created{}, exited{}, kernel{}, user{};
        GetProcessTimes(GetCurrentProcess(), &created, &exited, &kernel, &user);
        bootstrap.Target = {GetCurrentProcessId(),
            (uint64_t(created.dwHighDateTime) << 32) | created.dwLowDateTime, RequestId};
        bootstrap.Client = bootstrap.Target;
        auto runtime = Microsoft::WRL::Make<Runtime>();
        runtime->Target = bootstrap.Target;
        ComPtr<IWinUIProfilerRuntimeDiagnostics> diagnostics;
        if (withRuntime) runtime.As(&diagnostics);
        Handle finished(CreateEventW(nullptr, TRUE, FALSE, nullptr));
        auto server = std::make_shared<PipeServer>(bootstrap, diagnostics,
            [event = finished.Get()](PipeServer*) { SetEvent(event); });
        const auto started = server->Start();
        Check(started == S_OK, "hello test starts actual native PipeServer with or without private runtime");
        if (FAILED(started)) return;
        const auto path = L"\\\\.\\pipe\\" + bootstrap.PipeName;
        Handle pipe(CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING,
            FILE_FLAG_OVERLAPPED | SECURITY_SQOS_PRESENT | SECURITY_IDENTIFICATION, nullptr));
        Check(pipe.Get() != INVALID_HANDLE_VALUE, "hello test connects current-process client");
        if (pipe.Get() != INVALID_HANDLE_VALUE)
        {
            Header header{};
            std::string body;
            if (authenticate)
            {
                const auto handshake = std::string("{\"sessionId\":\"00112233-4455-6677-8899-aabbccddeeff\",") +
                    "\"process\":{\"processId\":" + std::to_string(bootstrap.Target.Pid) +
                    ",\"creationTimeFileTime\":" + std::to_string(bootstrap.Target.CreationTime) +
                    ",\"instanceId\":\"00112233-4455-6677-8899-aabbccddeeff\"},\"schemaVersion\":1," +
                    "\"nonceBase64\":\"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=\"}";
                Check(Exchange(pipe.Get(), Frame(1, handshake), header, body) &&
                    header.Type == MessageType::Acknowledgement &&
                    body == R"({"status":"Completed","captureId":null,"reason":null})",
                    "hello fixture authenticates through real runtime COM interface");
            }
            const auto frame = Frame(10, R"({"message":"hello"})", unique);
            const bool replied = Exchange(pipe.Get(), frame, header, body);
            if (authenticate)
            {
                Check(replied && static_cast<uint16_t>(header.Type) == 10 && header.RequestId == unique &&
                    body == R"({"message":"hello"})", "native PipeServer returns exact correlated hello");
                if (replied)
                {
                    if (!withRuntime)
                    {
                        GUID query{};
                        CoCreateGuid(&query);
                        Check(Exchange(pipe.Get(), Frame(2, R"({"schemaVersion":1})", query), header, body) &&
                            header.Type == MessageType::CapabilitiesResult && body.find("\"epoch\":null") != std::string::npos,
                            "ordinary runtime capabilities do not fabricate a producer epoch");
                        CoCreateGuid(&query);
                        Check(Exchange(pipe.Get(), Frame(3,
                            R"({"captureId":"00112233-4455-6677-8899-aabbccddeeff","rootId":null,"dispatcherId":null})",
                            query), header, body) && body.find("\"Unsupported\"") != std::string::npos,
                            "ordinary runtime reports private capture unsupported");
                    }
                    Check(Exchange(pipe.Get(), frame, header, body) && header.Type == MessageType::Acknowledgement &&
                        body.find("\"Rejected\"") != std::string::npos, "hello does not bypass duplicate-request rejection");
                }
            }
            else
                Check(replied && header.Type == MessageType::Acknowledgement && header.RequestId == unique &&
                    body.find("\"Rejected\"") != std::string::npos, "native PipeServer rejects hello before authentication");
        }
        pipe.Reset();
        server->Stop();
        Check(WaitForSingleObject(finished.Get(), 5000) == WAIT_OBJECT_0, "hello server releases runtime and pipe");
        const auto deadline = GetTickCount64() + 5000;
        while (ActiveServers && GetTickCount64() < deadline) Sleep(1);
        Check(ActiveServers == 0, "hello worker module pin released");
    }
}

int main()
{
    using Microsoft::WRL::ComPtr;
    using namespace WinUIProfiler::Tap;
    const HRESULT apartment = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(apartment)) return 2;
    Check(DllCanUnloadNow() == S_OK, "initial module has no active objects");
    Check(DllGetClassObject(ClassId, IID_IClassFactory, nullptr) == E_POINTER, "null factory output");
    void* unknown = reinterpret_cast<void*>(1);
    Check(DllGetClassObject(GUID_NULL, IID_IClassFactory, &unknown) == CLASS_E_CLASSNOTAVAILABLE &&
        unknown == nullptr, "unknown class clears output");
    ComPtr<IClassFactory> factory;
    const HRESULT created = DllGetClassObject(ClassId, IID_PPV_ARGS(&factory));
    Check(created == S_OK && factory, "TAP class factory is available");
    if (factory)
    {
        Check(DllCanUnloadNow() == S_FALSE, "factory prevents unload");
        Check(factory->CreateInstance(nullptr, IID_IObjectWithSite, nullptr) == E_POINTER,
            "null object output");
        Check(factory->LockServer(TRUE) == S_OK, "server lock acquired");
        factory.Reset();
        HelloServerTest(false);
        HelloServerTest(true);
        HelloServerTest(false, false);
        HelloServerTest(true, false);
        Check(DllCanUnloadNow() == S_FALSE, "server lock prevents unload without factory");
        Check(DllGetClassObject(ClassId, IID_PPV_ARGS(&factory)) == S_OK, "factory reacquired");
        Check(factory->LockServer(FALSE) == S_OK, "server lock released");
        Check(FAILED(factory->LockServer(FALSE)), "unbalanced unlock is rejected");
        ComPtr<IObjectWithSite> tap;
        Check(factory->CreateInstance(nullptr, IID_PPV_ARGS(&tap)) == S_OK && tap,
            "TAP supports IObjectWithSite");
        if (tap)
        {
            void* output = reinterpret_cast<void*>(1);
            Check(tap->GetSite(IID_IUnknown, &output) == E_FAIL && output == nullptr,
                "unattached site has no fabricated interface");
            Check(tap->GetSite(IID_IUnknown, nullptr) == E_POINTER, "null site output");
            Check(tap->SetSite(factory.Get()) == E_NOINTERFACE, "non-diagnostics site rejected");
            Check(tap->SetSite(nullptr) == S_OK, "unattached disconnect is idempotent");
            ComPtr<IUnknown> identity;
            Check(tap.As(&identity) == S_OK && identity, "canonical COM identity");
            void* aggregated{};
            Check(factory->CreateInstance(identity.Get(), IID_IObjectWithSite, &aggregated) ==
                CLASS_E_NOAGGREGATION && !aggregated, "aggregation rejected");
            factory.Reset();
            Check(DllCanUnloadNow() == S_FALSE, "TAP object alone prevents unload");
            identity.Reset();
            tap.Reset();
        }
    }
    factory.Reset();
    Check(DllCanUnloadNow() == S_OK, "all COM references released");
    CoUninitialize();
    std::printf("TAP COM: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
