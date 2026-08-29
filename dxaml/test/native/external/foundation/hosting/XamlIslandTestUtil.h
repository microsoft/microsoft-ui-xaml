// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

inline void WaitForSingleObjectWithTimeout(wil::unique_handle& handle, int timeout = 60000)
{
    const DWORD timeoutInMs = ::IsDebuggerPresent() ? INFINITE : timeout;
    auto result = ::WaitForSingleObject(handle.get(), timeoutInMs);
    if (result != WAIT_OBJECT_0)
    {
        VERIFY_FAIL(L"Timeout hit during WaitForSingleObject");
    }
}

inline wil::unique_handle RunOnNewThread(std::function<void(void)> threadFunc)
{
    struct ThreadFuncWrapper
    {
        std::function<void(void)> m_threadFunc;
    };

    HANDLE handle = CreateThread(nullptr, 0, [](LPVOID param) -> DWORD  {
        ThreadFuncWrapper* wrapper = static_cast<ThreadFuncWrapper*>(param);
        wrapper->m_threadFunc();
        delete wrapper;
        return 0;
     }, new ThreadFuncWrapper {threadFunc}, 0, nullptr);
     return wil::unique_handle{handle};
}

inline void DrainMessageQueue()
{
    MSG msg {};
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        DispatchMessage(&msg);
    }
}

template <typename TFunction>
inline void RunOnIslandUIThread(test_infra::IslandHelper^ islandHelper, const TFunction& function)
{
    msy::DispatcherQueue^ dq = islandHelper->DispatcherQueue;
    RunOnDispatcherThread(dq, true, function);
}

template <typename TFunction>
inline void RunOnIslandUIThreadAsync(test_infra::IslandHelper^ islandHelper, const TFunction& function)
{
    msy::DispatcherQueue^ dq = islandHelper->DispatcherQueue;
    RunOnDispatcherThread(dq, false, function);
}

// In the future let's make this better, more like WindowHelper->WaitForIdle().
inline void LowBudgetWaitForIdle(DispatcherQueue^ dq)
{
    if (dq->HasThreadAccess)
    {
        VERIFY_FAIL(L"This function must not be called on the DispatcherQueue's thread.");
    }

    Event waitCompleted;
    dq->TryEnqueue(
        Microsoft::UI::Dispatching::DispatcherQueuePriority::Low,
        ref new Microsoft::UI::Dispatching::DispatcherQueueHandler([&](){
            waitCompleted.Set();
        }));
    waitCompleted.WaitForDefault();

    WEX::Common::String value_ignored;
    if (SUCCEEDED(WEX::TestExecution::RuntimeParameters::TryGetValue(L"GoSlow", value_ignored)))
    {
        ::Sleep(1000);
    }
}

inline void LowBudgetWaitForIdle(test_infra::IslandHelper^ islandHelper)
{
    LowBudgetWaitForIdle(islandHelper->DispatcherQueue);
}

// TODO: Long-term we should fold these into the WinRT KeyboardHelper, but right now
// KeyboardHelper depends on WindowHelper which doesn't work in XamlIslandTests.
inline void SendTab()
{
    INPUT inputs[2] = {};
    ZeroMemory(inputs, sizeof(inputs));

    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = VK_TAB;

    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = VK_TAB;
    inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;

    LOG_OUTPUT(L"Send tab.");
    ::SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
}

// TODO: Long-term we should fold these into the WinRT KeyboardHelper, but right now
// KeyboardHelper depends on WindowHelper which doesn't work in XamlIslandTests.
inline void SendShiftTab()
{
    INPUT inputs[4] = {};
    ZeroMemory(inputs, sizeof(inputs));

    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = VK_SHIFT;

    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = VK_TAB;

    inputs[2].type = INPUT_KEYBOARD;
    inputs[2].ki.wVk = VK_TAB;
    inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;

    inputs[3].type = INPUT_KEYBOARD;
    inputs[3].ki.wVk = VK_SHIFT;
    inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;

    LOG_OUTPUT(L"Send shift+tab.");
    ::SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
}

inline void SendKeyPresses(std::initializer_list<WORD> keyPressList)
{
    std::vector<INPUT> keyPresses;

    for (WORD keyPress : keyPressList)
    {
        INPUT keyDownInput{};
        keyDownInput.type = INPUT_KEYBOARD;
        keyDownInput.ki.wVk = keyPress;

        keyPresses.push_back(keyDownInput);

        INPUT keyUpInput{};
        keyUpInput.type = INPUT_KEYBOARD;
        keyUpInput.ki.wVk = keyPress;
        keyUpInput.ki.dwFlags = KEYEVENTF_KEYUP;

        keyPresses.push_back(keyUpInput);
    }

    ::SendInput(static_cast<UINT>(keyPresses.size()), keyPresses.data(), sizeof(INPUT));
}

inline const wchar_t* GetString(XamlSourceFocusNavigationReason reason)
{
    switch (reason)
    {
        case XamlSourceFocusNavigationReason::Left: return L"Left";
        case XamlSourceFocusNavigationReason::Right: return L"Right";
        case XamlSourceFocusNavigationReason::Up: return L"Up";
        case XamlSourceFocusNavigationReason::Down: return L"Down";
        case XamlSourceFocusNavigationReason::First: return L"First";
        case XamlSourceFocusNavigationReason::Last: return L"Last";
        case XamlSourceFocusNavigationReason::Programmatic: return L"Programmatic";
        case XamlSourceFocusNavigationReason::Restore: return L"Restore";
    }
    return L"Unknown";
}

 // Helper to call Close on an object that supports IClosable
inline void CloseObject(Platform::Object^ obj)
{
    Microsoft::WRL::ComPtr<IUnknown> unk(reinterpret_cast<IUnknown*>(obj));
    Microsoft::WRL::ComPtr<ABI::Windows::Foundation::IClosable> closable;

    VERIFY_SUCCEEDED(unk.As(&closable));
    VERIFY_SUCCEEDED(closable->Close());
}

template <typename T>
void CopyObjectTo(Object^ obj, T** ptr)
{
    if (obj == nullptr)
    {
        *ptr = nullptr;
    }
    else
    {
        reinterpret_cast<IInspectable*>(obj)->QueryInterface(__uuidof(T), reinterpret_cast<void**>(ptr));
    }
}

template <typename T>
Object^ ToObject(T* ptr)
{
    wrl::ComPtr<IInspectable> insp;
    ptr->QueryInterface(__uuidof(IInspectable), reinterpret_cast<void**>(insp.GetAddressOf()));
    return reinterpret_cast<Object^>(insp.Get());
}

template <typename T>
wrl::ComPtr<T> CastHatTo(Object^ obj)
{
    wrl::ComPtr<T> t;
    reinterpret_cast<IInspectable*>(obj)->QueryInterface(IID_PPV_ARGS(&t));
    return t;
}

// Helper to check if the user set a parameter on the Taef command line
inline bool IsTestParameterSet(const wchar_t* name)
{
    WEX::Common::String value_ignored;
    if (SUCCEEDED(WEX::TestExecution::RuntimeParameters::TryGetValue(name, value_ignored)))
    {
        return true;
    }
    return false;
}
