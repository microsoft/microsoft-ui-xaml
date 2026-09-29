#include "pch.h"

using namespace std::chrono_literals;
using Microsoft::WRL::ComPtr;

namespace
{
    constexpr wchar_t WindowClassName[] = L"CoreMessagingReentrancyWindow";
    constexpr int RunButtonId = 1001;
    constexpr UINT RunTestMessage = WM_APP + 1;
    constexpr UINT TestCompleteMessage = WM_APP + 2;

#if USE_SYSTEM_COREMESSAGING
    constexpr wchar_t ProviderName[] = L"System CoreMessaging";
    constexpr wchar_t ProviderDll[] = L"CoreMessaging.dll";
    constexpr char CreateExport[] = "CoreUICreate";
    constexpr wchar_t CreateExportDisplay[] = L"CoreUICreate";
    constexpr wchar_t ExpectedResult[] = L"non-reentrant";
    constexpr bool ExpectedReentrant = false;
#else
    constexpr wchar_t ProviderName[] = L"Lifted CoreMessagingXP";
    constexpr wchar_t ProviderDll[] = L"CoreMessagingXP.dll";
    constexpr char CreateExport[] = "CoreMsgCreateSession";
    constexpr wchar_t CreateExportDisplay[] = L"CoreMsgCreateSession";
    constexpr wchar_t ExpectedResult[] = L"reentrant";
    constexpr bool ExpectedReentrant = true;
#endif

    void ThrowIfFailed(HRESULT result)
    {
        if (FAILED(result))
        {
            throw result;
        }
    }

    void ThrowLastErrorIfFalse(BOOL result)
    {
        if (!result)
        {
            throw HRESULT_FROM_WIN32(GetLastError());
        }
    }

    std::filesystem::path ExecutablePath()
    {
        std::wstring path(MAX_PATH, L'\0');
        for (;;)
        {
            const DWORD length = GetModuleFileNameW(
                nullptr,
                path.data(),
                static_cast<DWORD>(path.size()));
            ThrowLastErrorIfFalse(length != 0);
            if (length < path.size())
            {
                path.resize(length);
                return path;
            }
            path.resize(path.size() * 2);
        }
    }

    std::wstring ModulePath(const wchar_t* moduleName)
    {
        const HMODULE module = GetModuleHandleW(moduleName);
        if (!module)
        {
            return L"not loaded";
        }

        std::wstring path(MAX_PATH, L'\0');
        for (;;)
        {
            const DWORD length = GetModuleFileNameW(
                module,
                path.data(),
                static_cast<DWORD>(path.size()));
            if (length == 0)
            {
                return L"loaded; path unavailable";
            }
            if (length < path.size())
            {
                path.resize(length);
                return path;
            }
            path.resize(path.size() * 2);
        }
    }

    class App
    {
    public:
        int Run(HINSTANCE instance, int showCommand, bool automatic)
        {
            m_instance = instance;
            m_automatic = automatic;
            InitializeCoreMessaging();
            RegisterWindowClass();
            CreateMainWindow();

            ShowWindow(m_window, automatic ? SW_SHOWNOACTIVATE : showCommand);
            UpdateWindow(m_window);

            AppendEnvironment();
            if (automatic)
            {
                PostMessageW(m_window, RunTestMessage, 0, 0);
            }

            MSG message{};
            while (GetMessageW(&message, nullptr, 0, 0))
            {
                TranslateMessage(&message);
                DispatchMessageW(&message);
            }

            m_stableSession.Reset();
            m_session.Reset();
            if (m_providerModule)
            {
                FreeLibrary(m_providerModule);
                m_providerModule = nullptr;
            }
            return m_exitCode;
        }

    private:
#if USE_SYSTEM_COREMESSAGING
        using CreateSession = HRESULT(WINAPI*)(IMessageSession** session);
#else
        using CreateSession = HRESULT(WINAPI*)(MsgCreateFlags flags, IMessageSession** session);
#endif

        void InitializeCoreMessaging()
        {
#if USE_SYSTEM_COREMESSAGING
            m_providerModule = LoadLibraryExW(
                ProviderDll,
                nullptr,
                LOAD_LIBRARY_SEARCH_SYSTEM32);
#else
            const auto dllPath = ExecutablePath().parent_path() / ProviderDll;
            m_providerModule = LoadLibraryExW(
                dllPath.c_str(),
                nullptr,
                LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
#endif
            if (!m_providerModule)
            {
                throw HRESULT_FROM_WIN32(GetLastError());
            }

            const auto createSession = reinterpret_cast<CreateSession>(
                GetProcAddress(m_providerModule, CreateExport));
            if (!createSession)
            {
                throw HRESULT_FROM_WIN32(GetLastError());
            }

#if USE_SYSTEM_COREMESSAGING
            ThrowIfFailed(createSession(&m_session));
#else
            ThrowIfFailed(createSession(MsgCreateFlags::Default, &m_session));
#endif
            ThrowIfFailed(m_session.As(&m_stableSession));
        }

        static LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
        {
            App* app{};
            if (message == WM_NCCREATE)
            {
                const auto create = reinterpret_cast<CREATESTRUCTW*>(lParam);
                app = static_cast<App*>(create->lpCreateParams);
                SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
                app->m_window = window;
            }
            else
            {
                app = reinterpret_cast<App*>(GetWindowLongPtrW(window, GWLP_USERDATA));
            }

            return app
                ? app->HandleMessage(message, wParam, lParam)
                : DefWindowProcW(window, message, wParam, lParam);
        }

        LRESULT HandleMessage(UINT message, WPARAM wParam, LPARAM lParam)
        {
            switch (message)
            {
            case WM_CREATE:
                CreateControls();
                return 0;

            case WM_SIZE:
                LayoutControls(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
                return 0;

            case WM_COMMAND:
                if (LOWORD(wParam) == RunButtonId)
                {
                    StartTest();
                    return 0;
                }
                break;

            case RunTestMessage:
                StartTest();
                return 0;

            case TestCompleteMessage:
                CompleteTest();
                return 0;

            case WM_DESTROY:
                PostQuitMessage(m_exitCode);
                return 0;
            }

            return DefWindowProcW(m_window, message, wParam, lParam);
        }

        void RegisterWindowClass()
        {
            WNDCLASSEXW windowClass{ sizeof(windowClass) };
            windowClass.lpfnWndProc = WindowProc;
            windowClass.hInstance = m_instance;
            windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
            windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
            windowClass.lpszClassName = WindowClassName;
            ThrowLastErrorIfFalse(RegisterClassExW(&windowClass) != 0);
        }

        void CreateMainWindow()
        {
            std::wstring title = L"CoreMessaging reentrancy - ";
            title += ProviderName;

            m_window = CreateWindowExW(
                0,
                WindowClassName,
                title.c_str(),
                WS_OVERLAPPEDWINDOW,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                900,
                620,
                nullptr,
                nullptr,
                m_instance,
                this);
            ThrowLastErrorIfFalse(m_window != nullptr);
        }

        void CreateControls()
        {
            std::wstring description =
                std::wstring(ProviderName) + L"\r\n" +
                L"IMessageSessionStable::DeferInvoke\r\nExpected: callback B is " +
                ExpectedResult + L" while callback A runs a nested Win32 message pump.";

            m_description = CreateWindowExW(
                0,
                L"STATIC",
                description.c_str(),
                WS_CHILD | WS_VISIBLE,
                12,
                12,
                850,
                62,
                m_window,
                nullptr,
                m_instance,
                nullptr);

            m_runButton = CreateWindowExW(
                0,
                L"BUTTON",
                L"Run nested-pump test",
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
                12,
                82,
                220,
                34,
                m_window,
                reinterpret_cast<HMENU>(static_cast<INT_PTR>(RunButtonId)),
                m_instance,
                nullptr);

            m_log = CreateWindowExW(
                WS_EX_CLIENTEDGE,
                L"EDIT",
                L"",
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE |
                    ES_AUTOVSCROLL | ES_READONLY,
                12,
                128,
                850,
                430,
                m_window,
                nullptr,
                m_instance,
                nullptr);

            const HFONT font = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
            SendMessageW(m_description, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
            SendMessageW(m_runButton, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
            SendMessageW(m_log, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
        }

        void LayoutControls(int width, int height)
        {
            constexpr int margin = 12;
            constexpr int descriptionHeight = 62;
            constexpr int buttonHeight = 34;
            constexpr int gap = 8;

            MoveWindow(m_description, margin, margin, width - 2 * margin, descriptionHeight, TRUE);
            MoveWindow(
                m_runButton,
                margin,
                margin + descriptionHeight + gap,
                220,
                buttonHeight,
                TRUE);
            MoveWindow(
                m_log,
                margin,
                margin + descriptionHeight + gap + buttonHeight + gap,
                width - 2 * margin,
                height - (margin + descriptionHeight + gap + buttonHeight + gap + margin),
                TRUE);
        }

        void AppendEnvironment()
        {
            AppendLog(std::wstring(L"Provider: ") + ProviderName);
            AppendLog(std::wstring(L"Creation export: ") + CreateExportDisplay);
            AppendLog(std::wstring(L"CoreMessaging.dll: ") + ModulePath(L"CoreMessaging.dll"));
            AppendLog(std::wstring(L"CoreMessagingXP.dll: ") + ModulePath(L"CoreMessagingXP.dll"));
            AppendLog(L"");
        }

        void AppendLog(std::wstring_view text)
        {
            std::wstringstream line;
            line << L'[' << (GetTickCount64() - m_startTick) << L" ms]"
                 << L"[thread " << GetCurrentThreadId() << L"] "
                 << text << L"\r\n";

            const std::wstring value = line.str();
            m_transcript += value;
            OutputDebugStringW(value.c_str());

            if (m_log)
            {
                const int length = GetWindowTextLengthW(m_log);
                SendMessageW(m_log, EM_SETSEL, length, length);
                SendMessageW(m_log, EM_REPLACESEL, FALSE, reinterpret_cast<LPARAM>(value.c_str()));
            }
        }

        static HRESULT CALLBACK CallbackAThunk(void* context)
        {
            static_cast<App*>(context)->CallbackA();
            return S_OK;
        }

        static HRESULT CALLBACK CallbackBThunk(void* context)
        {
            static_cast<App*>(context)->CallbackB();
            return S_OK;
        }

        void StartTest()
        {
            if (m_testRunning)
            {
                return;
            }

            m_testRunning = true;
            m_aActive = false;
            m_aCompleted = false;
            m_bRan = false;
            m_observedReentrancy = false;
            m_enqueueResult = E_PENDING;
            EnableWindow(m_runButton, FALSE);

            AppendLog(L"--- Test started ---");
            AppendLog(L"Deferring callback A on the selected message session.");

            const HRESULT result = m_stableSession->DeferInvoke(
                CallbackAThunk,
                this,
                MsgPriority::Normal);
            if (FAILED(result))
            {
                m_enqueueResult = result;
                AppendLog(L"ERROR: MessageSession rejected callback A.");
                PostMessageW(m_window, TestCompleteMessage, 0, 0);
            }
        }

        void CallbackA()
        {
            m_aActive = true;
            AppendLog(L"A ENTER: callback A is active.");
            AppendLog(L"A starts a worker that will defer callback B after 100 ms.");

            ComPtr<IMessageSessionStable> stableSession = m_stableSession;
            std::thread producer([this, stableSession]
            {
                std::this_thread::sleep_for(100ms);
                m_enqueueResult = stableSession->DeferInvoke(
                    CallbackBThunk,
                    this,
                    MsgPriority::Normal);
            });

            AppendLog(L"A ENTERS nested PeekMessage/DispatchMessage pump for up to 750 ms.");
            PumpNestedMessages(750ms);
            producer.join();

            AppendLog(L"A LEAVES nested message pump.");
            m_aActive = false;
            m_aCompleted = true;
            AppendLog(L"A EXIT.");

            if (FAILED(m_enqueueResult) || m_bRan)
            {
                PostMessageW(m_window, TestCompleteMessage, 0, 0);
            }
        }

        void CallbackB()
        {
            m_observedReentrancy = m_aActive;
            m_bRan = true;

            AppendLog(
                m_observedReentrancy
                    ? L"B RUNS while A is active: REENTRANT dispatch observed."
                    : L"B RUNS after A exited: NON-REENTRANT dispatch observed.");

            if (m_aCompleted)
            {
                PostMessageW(m_window, TestCompleteMessage, 0, 0);
            }
        }

        void PumpNestedMessages(std::chrono::milliseconds duration)
        {
            const auto deadline = std::chrono::steady_clock::now() + duration;
            while (!m_bRan && std::chrono::steady_clock::now() < deadline)
            {
                const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
                    deadline - std::chrono::steady_clock::now());
                const DWORD waitMilliseconds =
                    static_cast<DWORD>((remaining.count() > 0) ? remaining.count() : 0);

                MsgWaitForMultipleObjectsEx(
                    0,
                    nullptr,
                    waitMilliseconds,
                    QS_ALLINPUT,
                    MWMO_INPUTAVAILABLE);

                MSG message{};
                while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
                {
                    if (message.message == WM_QUIT)
                    {
                        PostQuitMessage(static_cast<int>(message.wParam));
                        return;
                    }

                    TranslateMessage(&message);
                    DispatchMessageW(&message);
                }
            }
        }

        void CompleteTest()
        {
            if (!m_testRunning)
            {
                return;
            }

            m_testRunning = false;
            const bool passed =
                SUCCEEDED(m_enqueueResult) &&
                m_bRan &&
                (m_observedReentrancy == ExpectedReentrant);

            std::wstring result = passed ? L"PASS: observed expected " : L"FAIL: observed unexpected ";
            result += m_observedReentrancy ? L"reentrant behavior." : L"non-reentrant behavior.";
            AppendLog(result);
            AppendLog(L"--- Test completed ---");
            EnableWindow(m_runButton, TRUE);

            m_exitCode = passed ? 0 : 2;
            if (m_automatic)
            {
                WriteResult();
                DestroyWindow(m_window);
            }
        }

        void WriteResult()
        {
            const auto executable = ExecutablePath();
            const auto resultPath =
                executable.parent_path() / (executable.stem().wstring() + L".result.txt");

            std::ofstream output(resultPath, std::ios::binary | std::ios::trunc);
            if (!output)
            {
                m_exitCode = 4;
                return;
            }

            const int required = WideCharToMultiByte(
                CP_UTF8,
                0,
                m_transcript.data(),
                static_cast<int>(m_transcript.size()),
                nullptr,
                0,
                nullptr,
                nullptr);
            std::string utf8(required, '\0');
            WideCharToMultiByte(
                CP_UTF8,
                0,
                m_transcript.data(),
                static_cast<int>(m_transcript.size()),
                utf8.data(),
                required,
                nullptr,
                nullptr);
            output.write(utf8.data(), static_cast<std::streamsize>(utf8.size()));
        }

        HINSTANCE m_instance{};
        HWND m_window{};
        HWND m_description{};
        HWND m_runButton{};
        HWND m_log{};
        HMODULE m_providerModule{};
        ComPtr<IMessageSession> m_session;
        ComPtr<IMessageSessionStable> m_stableSession;
        ULONGLONG m_startTick{ GetTickCount64() };
        std::wstring m_transcript;
        std::atomic<HRESULT> m_enqueueResult{ E_PENDING };
        bool m_automatic{};
        bool m_testRunning{};
        bool m_aActive{};
        bool m_aCompleted{};
        bool m_bRan{};
        bool m_observedReentrancy{};
        int m_exitCode{};
    };

    bool HasArgument(std::wstring_view expected)
    {
        int argumentCount{};
        PWSTR* arguments = CommandLineToArgvW(GetCommandLineW(), &argumentCount);
        if (!arguments)
        {
            throw HRESULT_FROM_WIN32(GetLastError());
        }

        bool found = false;
        for (int index = 1; index < argumentCount; ++index)
        {
            if (_wcsicmp(arguments[index], expected.data()) == 0)
            {
                found = true;
                break;
            }
        }
        LocalFree(arguments);
        return found;
    }
}

int WINAPI wWinMain(
    _In_ HINSTANCE instance,
    _In_opt_ HINSTANCE,
    _In_ PWSTR,
    _In_ int showCommand)
{
    const HRESULT initializeResult = RoInitialize(RO_INIT_SINGLETHREADED);
    if (FAILED(initializeResult))
    {
        return static_cast<int>(initializeResult);
    }

    int result{};
    try
    {
        App app;
        result = app.Run(instance, showCommand, HasArgument(L"--auto"));
    }
    catch (HRESULT error)
    {
        wchar_t message[128]{};
        swprintf_s(message, L"Failure HRESULT: 0x%08X", static_cast<unsigned int>(error));
        MessageBoxW(
            nullptr,
            message,
            L"CoreMessaging reentrancy test failed",
            MB_OK | MB_ICONERROR);
        result = static_cast<int>(error);
    }

    RoUninitialize();
    return result;
}
