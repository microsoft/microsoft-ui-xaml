// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <windows.h>
#include <unknwn.h>
#include <ocidl.h>
#include <xamlom.h>
#include <wil/com.h>
#include <wil/resource.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Data.Json.h>
#include <winrt/Microsoft.UI.Dispatching.h>
#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include "OnlineXamlSnapshot.h"
#include "Protocol.h"
#include "TapIdentity.h"

namespace
{
    using namespace winrt::Windows::Data::Json;
    using winrt::Microsoft::UI::Dispatching::DispatcherQueue;
    std::atomic<ULONG> g_objects{};
    std::atomic<ULONG> g_serverLocks{};

    struct Counted
    {
        Counted() { ++g_objects; }
        ~Counted() { --g_objects; }
    };

    std::wstring Hex(UINT64 value)
    {
        if (!value)
        {
            return {};
        }
        wchar_t text[17]{};
        swprintf_s(text, L"%016llX", value);
        return text;
    }

    void String(JsonObject& object, PCWSTR name, std::wstring_view value)
    {
        object.Insert(name, JsonValue::CreateStringValue(value));
    }

    std::vector<BYTE> Encode(const JsonObject& object)
    {
        auto text = winrt::to_string(object.Stringify());
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW), text.size() > Protocol::MaximumPayloadSize);
        return {text.begin(), text.end()};
    }

    void Log(HRESULT status, PCWSTR operation)
    {
        wchar_t text[256]{};
        swprintf_s(text, L"OnlineXamlProfilerTap: %s failed (0x%08X)\n", operation, static_cast<UINT32>(status));
        OutputDebugStringW(text);
    }

    struct PropertyRecord
    {
        std::wstring Name;
        std::wstring Value;
        HRESULT Status{};
    };

    struct NodeRecord
    {
        OnlineXaml::Node Values;
        std::wstring Type;
        std::wstring Name;
        std::vector<PropertyRecord> Properties;
    };

    struct CaptureResult
    {
        OnlineXaml::CaptureInfo Info;
        std::vector<NodeRecord> Nodes;
        std::map<std::pair<OnlineXaml::TreeKind, UINT64>, size_t> NodeIndex;
        wil::unique_event Done{wil::EventOptions::ManualReset};
        std::atomic<bool> Cancelled{};
        HRESULT Status = E_PENDING;
    };

    HRESULT WINAPI AddNode(void* context, const OnlineXaml::Node* node) noexcept
    try
    {
        auto& result = *static_cast<CaptureResult*>(context);
        if (result.Cancelled)
        {
            return HRESULT_FROM_WIN32(ERROR_CANCELLED);
        }
        if (!node || node->Size != sizeof(*node) || !node->Id || !node->Type)
        {
            return E_INVALIDARG;
        }
        NodeRecord record;
        record.Values = *node;
        record.Values.Type = nullptr;
        record.Values.Name = nullptr;
        record.Type = node->Type;
        record.Name = node->Name ? node->Name : L"";
        if (!result.NodeIndex.emplace(std::make_pair(node->Tree, node->Id), result.Nodes.size()).second)
        {
            return E_INVALIDARG;
        }
        result.Nodes.push_back(std::move(record));
        return S_OK;
    }
    catch (...)
    {
        return winrt::to_hresult();
    }

    HRESULT WINAPI AddProperty(
        void* context, OnlineXaml::TreeKind tree, UINT64 id, PCWSTR name, PCWSTR value, HRESULT status) noexcept
    try
    {
        auto& result = *static_cast<CaptureResult*>(context);
        if (result.Cancelled)
        {
            return HRESULT_FROM_WIN32(ERROR_CANCELLED);
        }
        const auto node = result.NodeIndex.find({tree, id});
        if (node == result.NodeIndex.end() || !name || (SUCCEEDED(status) && !value))
        {
            return E_INVALIDARG;
        }
        result.Nodes[node->second].Properties.push_back({name, value ? value : L"", status});
        return S_OK;
    }
    catch (...)
    {
        return winrt::to_hresult();
    }

    JsonObject SerializeThread(DWORD threadId, HRESULT status, const CaptureResult* result)
    {
        JsonObject thread;
        thread.Insert(L"ThreadId", JsonValue::CreateNumberValue(threadId));
        thread.Insert(L"Status", JsonValue::CreateNumberValue(status));
        String(thread, L"Error", FAILED(status) ? winrt::hresult_error(status).message() : winrt::hstring{});
        String(thread, L"StartQpc", result ? std::to_wstring(result->Info.StartQpc) : L"0");
        String(thread, L"EndQpc", result ? std::to_wstring(result->Info.EndQpc) : L"0");
        String(thread, L"QpcFrequency", result ? std::to_wstring(result->Info.QpcFrequency) : L"0");
        JsonArray nodes;
        if (SUCCEEDED(status) && result)
        {
            for (const auto& record : result->Nodes)
            {
                const auto& values = record.Values;
                JsonObject node;
                node.Insert(L"Tree", JsonValue::CreateNumberValue(static_cast<UINT32>(values.Tree)));
                String(node, L"Id", Hex(values.Id));
                String(node, L"ParentId", Hex(values.ParentId));
                node.Insert(L"ChildIndex", JsonValue::CreateNumberValue(values.ChildIndex));
                String(node, L"Type", record.Type);
                String(node, L"Name", record.Name);
                String(node, L"XamlId", Hex(values.XamlId));
                String(node, L"CompId", Hex(values.CompId));
                String(node, L"VisualId", Hex(values.VisualId));
                JsonArray properties;
                for (const auto& captured : record.Properties)
                {
                    JsonObject property;
                    String(property, L"Name", captured.Name);
                    property.Insert(L"Status", JsonValue::CreateNumberValue(captured.Status));
                    property.Insert(L"Value", SUCCEEDED(captured.Status)
                        ? JsonValue::CreateStringValue(captured.Value) : JsonValue::CreateNullValue());
                    properties.Append(property);
                }
                node.Insert(L"Properties", properties);
                nodes.Append(node);
            }
        }
        thread.Insert(L"Nodes", nodes);
        return thread;
    }

    struct Tap : winrt::implements<Tap, IObjectWithSite, IVisualTreeServiceCallback2>, Counted
    {
        HRESULT STDMETHODCALLTYPE SetSite(IUnknown* site) noexcept override
        try
        {
            if (!site)
            {
                m_stop.SetEvent();
                return S_OK;
            }
            if (m_initialized.exchange(true))
            {
                return HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED);
            }
            m_site.copy_from(site);
            m_diagnostics = m_site.as<IXamlDiagnostics>();
            m_service = m_site.as<IVisualTreeService>();
            RegisterCurrentThread();
            wil::unique_bstr initialization;
            THROW_IF_FAILED(m_diagnostics->GetInitializationData(&initialization));
            std::wstring name = initialization ? initialization.get() : L"";
            THROW_HR_IF(E_INVALIDARG, name.rfind(L"OnlineXamlProfiler.", 0) != 0 ||
                name.size() > 200 || name.find_first_not_of(L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-") != std::wstring::npos);
            std::thread([self = get_strong(), name = std::move(name)]
            {
                self->Run(name);
            }).detach();
            return S_OK;
        }
        catch (...)
        {
            return winrt::to_hresult();
        }

        HRESULT STDMETHODCALLTYPE GetSite(REFIID iid, void** site) noexcept override
        {
            if (!site)
            {
                return E_POINTER;
            }
            *site = nullptr;
            return m_site ? m_site->QueryInterface(iid, site) : E_FAIL;
        }

        HRESULT STDMETHODCALLTYPE OnVisualTreeChange(
            ParentChildRelation, VisualElement, VisualMutationType) noexcept override
        try
        {
            RegisterCurrentThread();
            return S_OK;
        }
        catch (...)
        {
            return winrt::to_hresult();
        }

        HRESULT STDMETHODCALLTYPE OnElementStateChanged(
            InstanceHandle, VisualElementState, LPCWSTR) noexcept override
        {
            return S_OK;
        }

    private:
        void RegisterCurrentThread()
        {
            if (auto queue = DispatcherQueue::GetForCurrentThread())
            {
                std::scoped_lock guard(m_queueLock);
                m_queues.insert_or_assign(GetCurrentThreadId(), queue);
            }
        }

        void Send(Protocol::Kind kind, UINT64 requestId, const JsonObject& object)
        {
            Protocol::Write(m_pipe.get(), m_stop.get(), kind, requestId, Encode(object));
        }

        void SendError(UINT64 requestId, HRESULT status, PCWSTR message)
        {
            JsonObject error;
            error.Insert(L"Status", JsonValue::CreateNumberValue(status));
            String(error, L"Message", message);
            Send(Protocol::Kind::Error, requestId, error);
        }

        JsonObject Capture(UINT64 requestId, UINT32 maximumNodes)
        {
            THROW_HR_IF(E_INVALIDARG, maximumNodes == 0 || maximumNodes > OnlineXaml::MaximumNodeCount);
            std::map<DWORD, DispatcherQueue> queues;
            {
                std::scoped_lock guard(m_queueLock);
                queues = m_queues;
            }
            THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_NOT_READY), queues.empty());
            JsonArray threads;
            UINT32 remaining = maximumNodes;
            for (const auto& [threadId, queue] : queues)
            {
                auto result = std::make_shared<CaptureResult>();
                auto callback = [self = get_strong(), result, remaining]
                {
                    auto completed = wil::scope_exit([&] { result->Done.SetEvent(); });
                    if (result->Cancelled)
                    {
                        result->Status = HRESULT_FROM_WIN32(ERROR_CANCELLED);
                        return;
                    }
                    OnlineXaml::Sink sink;
                    sink.Context = result.get();
                    sink.AddNode = AddNode;
                    sink.AddProperty = AddProperty;
                    result->Status = self->m_capture(OnlineXaml::SnapshotAbiVersion, remaining, &sink, &result->Info);
                };
                if (!remaining)
                {
                    threads.Append(SerializeThread(threadId, HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW), nullptr));
                    continue;
                }
                if (!queue.TryEnqueue(callback))
                {
                    threads.Append(SerializeThread(threadId, RO_E_CLOSED, nullptr));
                    continue;
                }
                HANDLE waits[] = {m_stop.get(), result->Done.get()};
                const DWORD wait = WaitForMultipleObjects(ARRAYSIZE(waits), waits, FALSE, 15000);
                if (wait != WAIT_OBJECT_0 + 1)
                {
                    result->Cancelled = true;
                    THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_CANCELLED), wait == WAIT_OBJECT_0);
                    const auto error = HRESULT_FROM_WIN32(wait == WAIT_TIMEOUT ? ERROR_TIMEOUT : GetLastError());
                    threads.Append(SerializeThread(threadId, error, nullptr));
                    continue;
                }
                if (SUCCEEDED(result->Status))
                {
                    THROW_HR_IF(E_UNEXPECTED, result->Nodes.size() > remaining);
                    remaining -= static_cast<UINT32>(result->Nodes.size());
                }
                threads.Append(SerializeThread(threadId, result->Status, result.get()));
            }
            JsonObject snapshot;
            snapshot.Insert(L"SchemaVersion", JsonValue::CreateNumberValue(1));
            snapshot.Insert(L"ProcessId", JsonValue::CreateNumberValue(GetCurrentProcessId()));
            String(snapshot, L"ProcessStartTime", m_processStart);
            String(snapshot, L"RequestId", std::to_wstring(requestId));
            snapshot.Insert(L"Threads", threads);
            return snapshot;
        }

        void Run(const std::wstring& name) noexcept
        {
            bool adviseAttempted = false;
            bool apartmentInitialized = false;
            try
            {
                winrt::init_apartment(winrt::apartment_type::multi_threaded);
                apartmentInitialized = true;
                const std::wstring path = L"\\\\.\\pipe\\" + name;
                m_pipe.reset(CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                    OPEN_EXISTING, FILE_FLAG_OVERLAPPED | SECURITY_SQOS_PRESENT | SECURITY_IDENTIFICATION, nullptr));
                THROW_LAST_ERROR_IF(!m_pipe);
                auto module = GetModuleHandleW(L"Microsoft.UI.Xaml.dll");
                m_capture = reinterpret_cast<OnlineXaml::CaptureSnapshot>(
                    module ? GetProcAddress(module, "OnlineXamlProfilerCaptureSnapshot") : nullptr);
                if (!m_capture)
                {
                    SendError(0, HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND),
                        L"The target must load a WinUI runtime built with OnlineXamlProfilerCaptureSnapshot. Deploy the modified runtime and restart the target.");
                }
                else
                {
                    FILETIME created{}, exited{}, kernel{}, user{};
                    THROW_IF_WIN32_BOOL_FALSE(GetProcessTimes(GetCurrentProcess(), &created, &exited, &kernel, &user));
                    m_processStart = Hex((static_cast<UINT64>(created.dwHighDateTime) << 32) | created.dwLowDateTime);
                    std::wstring runtimePath(32768, L'\0');
                    const DWORD pathLength = GetModuleFileNameW(module, runtimePath.data(), static_cast<DWORD>(runtimePath.size()));
                    THROW_LAST_ERROR_IF(pathLength == 0);
                    THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER), pathLength >= runtimePath.size());
                    runtimePath.resize(pathLength);
                    adviseAttempted = true;
                    THROW_IF_FAILED(m_service->AdviseVisualTreeChange(this));
                    JsonObject hello;
                    hello.Insert(L"ProcessId", JsonValue::CreateNumberValue(GetCurrentProcessId()));
                    hello.Insert(L"SnapshotAbiVersion", JsonValue::CreateNumberValue(OnlineXaml::SnapshotAbiVersion));
                    String(hello, L"ProcessStartTime", m_processStart);
                    String(hello, L"RuntimePath", runtimePath);
                    Send(Protocol::Kind::Hello, 0, hello);
                    while (!m_stop.is_signaled())
                    {
                        const auto frame = Protocol::Read(m_pipe.get(), m_stop.get());
                        if (frame.Type == Protocol::Kind::Disconnect)
                        {
                            break;
                        }
                        try
                        {
                            THROW_HR_IF(E_INVALIDARG, frame.Type != Protocol::Kind::Capture ||
                                frame.RequestId == 0 || frame.Payload.size() != sizeof(UINT32));
                            Send(Protocol::Kind::Snapshot, frame.RequestId,
                                Capture(frame.RequestId, Protocol::ReadNumber<UINT32>(frame.Payload.data())));
                        }
                        catch (...)
                        {
                            const HRESULT status = winrt::to_hresult();
                            Log(status, L"Capture");
                            SendError(frame.RequestId, status, winrt::hresult_error(status).message().c_str());
                        }
                    }
                }
            }
            catch (...)
            {
                const HRESULT status = winrt::to_hresult();
                Log(status, L"Session");
                if (m_pipe && !m_stop.is_signaled())
                {
                    try
                    {
                        SendError(0, status, winrt::hresult_error(status).message().c_str());
                    }
                    catch (...)
                    {
                        Log(winrt::to_hresult(), L"Sending session failure");
                    }
                }
            }
            m_stop.SetEvent();
            if (adviseAttempted)
            {
                const auto status = m_service->UnadviseVisualTreeChange(this);
                if (FAILED(status))
                {
                    Log(status, L"UnadviseVisualTreeChange");
                }
            }
            m_pipe.reset();
            if (apartmentInitialized)
            {
                winrt::uninit_apartment();
            }
        }

        std::atomic<bool> m_initialized{};
        winrt::com_ptr<IUnknown> m_site;
        winrt::com_ptr<IXamlDiagnostics> m_diagnostics;
        winrt::com_ptr<IVisualTreeService> m_service;
        wil::unique_event m_stop{wil::EventOptions::ManualReset};
        wil::unique_handle m_pipe;
        std::mutex m_queueLock;
        std::map<DWORD, DispatcherQueue> m_queues;
        OnlineXaml::CaptureSnapshot m_capture{};
        std::wstring m_processStart;
    };

    struct Factory : winrt::implements<Factory, IClassFactory>, Counted
    {
        HRESULT STDMETHODCALLTYPE CreateInstance(IUnknown* outer, REFIID iid, void** object) noexcept override
        try
        {
            if (!object)
            {
                return E_POINTER;
            }
            *object = nullptr;
            if (outer)
            {
                return CLASS_E_NOAGGREGATION;
            }
            return winrt::make_self<Tap>()->QueryInterface(iid, object);
        }
        catch (...)
        {
            return winrt::to_hresult();
        }

        HRESULT STDMETHODCALLTYPE LockServer(BOOL lock) noexcept override
        {
            if (lock)
            {
                ++g_serverLocks;
            }
            else
            {
                --g_serverLocks;
            }
            return S_OK;
        }
    };
}

extern "C" HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID iid, void** object)
try
{
    if (!object)
    {
        return E_POINTER;
    }
    *object = nullptr;
    if (clsid != OnlineXamlProfiler::TapClsid)
    {
        return CLASS_E_CLASSNOTAVAILABLE;
    }
    return winrt::make_self<Factory>()->QueryInterface(iid, object);
}
catch (...)
{
    return winrt::to_hresult();
}

extern "C" HRESULT WINAPI DllCanUnloadNow()
{
    return g_objects == 0 && g_serverLocks == 0 ? S_OK : S_FALSE;
}
