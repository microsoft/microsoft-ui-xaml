# WinRT API changes for Desktop apps

Some Windows APIs are designed for and only completely supported in a UWP app, especially [CoreDispatcher](https://docs.microsoft.com/uwp/api/Windows.UI.Core.CoreDispatcher), [CoreWindow](https://docs.microsoft.com/uwp/api/Windows.UI.Core.CoreWindow), [ApplicationView](https://docs.microsoft.com/uwp/api/windows.ui.viewmanagement.applicationview), and some related classes. These APIs are partially supported in Desktop apps when _hosting_ XAML ([XAML Islands](https://docs.microsoft.com/windows/apps/desktop/modernize/xaml-islands)), and in WinUI 3 previews before Preview 4. As of WinUI 3 Preview 4 they are fully unsupported.

Note that when an API is unsupported, other APIs that depend on the former become unsupported too. For example, several classes in the Windows Runtime API have a static `GetForCurrentView()` method, such as [UIViewSettings.GetForCurrentView](https://docs.microsoft.com/uwp/api/Windows.UI.ViewManagement.UIViewSettings.GetForCurrentView), which is a pattern that indicates a dependence on `ApplicationView`, and therefore these are not supported in Desktop apps. 

This document lists out these APIs - these are no longer supported in Desktop apps, and neither are any of its members or other APIs that depend on it. There are suggested replacements below, where available, to achieve the same functionality. There are also COM and C# interops provided where available. For more information on C# and COM interop, see the [COM Interop Guide](https://github.com/microsoft/CsWinRT/blob/master/docs/interop.md#COM-Interop). 

This doc is a live document and will continue to be updated as we identify more workarounds/replacements. If you are encountering issues with an API not listed here, please go ahead and [file a bug](https://github.com/microsoft/microsoft-ui-xaml/issues/new?assignees=&labels=&template=bug_report.md&title=) on this repo with the API in question and what you are trying to achieve by using it. 

## ApplicationView

- [`ApplicationViewTitleBar.ExtendViewIntoTitleBar`](https://docs.microsoft.com/uwp/api/windows.applicationmodel.core.coreapplicationviewtitlebar.extendviewintotitlebar) 
    
    Use `Window.ExtendsContentIntoTitleBar` instead. This API was added in Preview 4 and is intended as a permanent replacement in WinUI 3. API reference documentation for this is coming soon.
    
As mentioned above, `GetForCurrentView()` methods have a dependence on `ApplicationView` and are not supported in Desktop apps. Note that some `GetForCurrentView` methods will not only return null, but will also throw exceptions. We advise checking if `Window.Current` is null before calling these APIs - although `Window.Current` returning null may be expected in some scenarios, such as during background activation. 

## CoreWindow

- [`CoreWindow.GetKeyState()`](https://docs.microsoft.com/uwp/api/windows.ui.core.corewindow.getkeystate) 

    Use [`KeyboardInput.GetKeyStateForCurrentThread`](https://docs.microsoft.com/windows/winui/api/microsoft.ui.input.keyboardinput.getkeystateforcurrentthread?view=winui-3.0-preview) instead.

- [`CoreWindow.PointerCursor`](https://docs.microsoft.com/uwp/api/windows.ui.core.corewindow.pointercursor) 

    Use [`UIElement.ProtectedCursor`](https://docs.microsoft.com/windows/winui/api/microsoft.ui.xaml.uielement.protectedcursor?view=winui-3.0-preview) instead. Note that you'll need to have a subclass of `UIElement` to access this API. 

## CoreDispatcher
Note that the [Window.Dispatcher](https://docs.microsoft.com/uwp/api/Windows.UI.Xaml.Window.Dispatcher) and [DependencyObject.Dispatcher](https://docs.microsoft.com/uwp/api/Windows.UI.Xaml.DependencyObject.Dispatcher) properties return `null` in a Desktop app.

- [`Window.Dispatcher`](https://docs.microsoft.com/windows/winui/api/microsoft.ui.xaml.window.dispatcher?view=winui-3.0-preview) or [`CoreDispatcher`](https://docs.microsoft.com/uwp/api/windows.ui.core.coredispatcher)

    Use [`DispatcherQueue`](https://docs.microsoft.com/windows/winui/api/microsoft.ui.xaml.window.dispatcherqueue?view=winui-3.0-preview) instead. `DispatcherQueue` is the permanent WinUI 3 replacement for `Dispatcher` and `CoreDispatcher`.

## CoreApplicationView
This API is no longer supported in Desktop apps, and neither are any of its members or other APIs that depend on it.

## DisplayInformation

- [`DisplayInformation.LogicalDpi`](https://docs.microsoft.com/uwp/api/windows.graphics.display.displayinformation.logicaldpi)

    Use [`XamlRoot.RasterizationScale`](https://docs.microsoft.com/windows/winui/api/microsoft.ui.xaml.xamlroot.rasterizationscale?view=winui-3.0-preview) instead, and listen for changes on the [`XamlRoot.Changed`](https://docs.microsoft.com/uwp/api/windows.ui.xaml.xamlroot.changed) event. 

- [`DisplayInformation.RawPixelsPerViewPixel`](https://docs.microsoft.com/uwp/api/windows.graphics.display.displayinformation.rawpixelsperviewpixel)

    Use [`XamlRoot.RasterizationScale`](https://docs.microsoft.com/windows/winui/api/microsoft.ui.xaml.xamlroot.rasterizationscale?view=winui-3.0-preview) instead. 

## SystemNavigationManager


- [`SystemNavigationManager.GetForCurrentView()`](https://docs.microsoft.com/uwp/api/windows.ui.core.systemnavigationmanager.getforcurrentview)

    No alternative - this functionality is not intended to be supported in Desktop apps. 

## CoreTextServicesManager 
- [`CoreTextServicesManager.GetForCurrentView()`](https://docs.microsoft.com/uwp/api/windows.ui.text.core.coretextservicesmanager?view=winrt-19041)
    
    This method will be supported for Desktop apps in future Windows versions, and is currently only supported in Insider builds. For non-Insider builds, this API is unsupported and there is no current workaround. 

## CoreInputView
- [`CoreInputView.GetForCurrentView()`](https://docs.microsoft.com/uwp/api/windows.ui.viewmanagement.core.coreinputview.getforcurrentview?view=winrt-19041)

    This method is **still working and supported for Desktop apps**, even without [CoreWindow](https://docs.microsoft.com/uwp/api/windows.ui.core.corewindow?view=winrt-19041). It can be created on any thread, and if that thread has a foreground window, it will produce events.

## AccountSettingsPane
- [`AccountSettingsPane.GetForCurrentView()`](https://docs.microsoft.com/uwp/api/windows.ui.applicationsettings.accountssettingspane.getforcurrentview?view=winrt-19041)

    This method is no longer supported for Desktop apps. There is a C++ COM interop for this class, as well as a C# version of the interop.

    ### COM Interop

    ```c++
    winrt::IAsyncAction DoAccountsSettingsPaneAsync(int option)
    {
        using namespace winrt::Windows::UI::ApplicationSettings;
        using namespace winrt::Windows::Security::Authentication::Web::Core;

        winrt::apartment_context ui_thread; // Capture calling context of UI thread.

        auto factory =
            winrt::get_activation_factory<winrt::Windows::UI::ApplicationSettings::AccountsSettingsPane, IAccountsSettingsPaneInterop>();

        auto accountsSettingsPane = winrt::capture<winrt::Windows::UI::ApplicationSettings::AccountsSettingsPane>(
            factory, &IAccountsSettingsPaneInterop::GetForWindow, m_dialogWindow);

        auto strongThis = shared_from_this();

        auto msaProvider =
            co_await WebAuthenticationCoreManager::FindAccountProviderAsync(L"https://login.microsoft.com", L"consumers");
        auto aadProvider =
            co_await WebAuthenticationCoreManager::FindAccountProviderAsync(L"https://login.microsoft.com", L"organizations");

        co_await ui_thread; // Switch back to calling context since AccountsControl must be shown on UI thread

        auto providerInvokedHandler = [this](winrt::WebAccountProviderCommand const& cmd) {
            // TODO: log DisplayName won't compile: cmd.WebAccountProvider().DisplayName().c_str() complains about DisplayName.
            Log(L"Provider handler called\r\n");
        };

        auto msaProviderCmd = WebAccountProviderCommand(msaProvider, providerInvokedHandler);
        auto aadProviderCmd = WebAccountProviderCommand(aadProvider, providerInvokedHandler);

        auto eventToken = accountsSettingsPane.AccountCommandsRequested(
            [this, msaProviderCmd, aadProviderCmd, option](
                winrt::Windows::UI::ApplicationSettings::AccountsSettingsPane sender, AccountsSettingsPaneCommandsRequestedEventArgs args) {
                Log(L"AccountCommandsRequested called\r\n");

                // TODO: add other command types

                if (option == 0)
                {
                    args.HeaderText(L"ShowAddAccountForWindowAsync");
                    args.WebAccountProviderCommands().Append(msaProviderCmd);
                    args.WebAccountProviderCommands().Append(aadProviderCmd);
                }
                else
                {
                    args.HeaderText(L"ShowManageAccountsForWindowAsync");
                }
            });

        auto op = (option == 0) ?
            // Has saved accounts view
            winrt::capture<winrt::IAsyncAction>(factory,
                &IAccountsSettingsPaneInterop::ShowAddAccountForWindowAsync,
                m_dialogWindow) :
            // does not have saved accounts view
            winrt::capture<winrt::IAsyncAction>(factory,
                &IAccountsSettingsPaneInterop::ShowManageAccountsForWindowAsync,
                m_dialogWindow);

        m_cancelFunctions.emplace_back(std::bind(&winrt::IAsyncAction::Cancel, op));
    }
    ```

    ### C# version of Interop

    ```csharp
    namespace UWPInterop
    {

        [System.Runtime.InteropServices.Guid("D3EE12AD-3865-4362-9746-B75A682DF0E6")]
        [System.Runtime.InteropServices.InterfaceType(System.Runtime.InteropServices.ComInterfaceType.InterfaceIsIInspectable)]
        public interface IAccountsSettingsPaneInterop
        {
            AccountsSettingsPane GetForWindow(IntPtr appWindow, [System.Runtime.InteropServices.In] ref Guid riid);
            IAsyncAction ShowManagedAccountsForWindowAsync(IntPtr appWindow, [System.Runtime.InteropServices.In] ref Guid riid);
            IAsyncAction ShowAddAccountForWindowAsync(IntPtr appWindow, [System.Runtime.InteropServices.In] ref Guid riid);
        }

        //Helper to initialize AccountsSettingsPane
        public static class AccountsSettingsPaneInterop 
        {
            public static AccountsSettingsPane GetForWindow(IntPtr hWnd)
            {
                IAccountsSettingsPaneInterop accountsSettingsPaneInterop = (IAccountsSettingsPaneInterop)WindowsRuntimeMarshal.GetActivationFactory(typeof(AccountsSettingsPane));
                Guid guid = typeof(AccountsSettingsPane).GUID;

                return accountsSettingsPaneInterop.GetForWindow(hWnd, ref guid);
            }
            public static IAsyncAction ShowManagedAccountsForWindowAsync(IntPtr hWnd)
            {
                IAccountsSettingsPaneInterop accountsSettingsPaneInterop = (IAccountsSettingsPaneInterop)WindowsRuntimeMarshal.GetActivationFactory(typeof(AccountsSettingsPane));
                Guid guid = typeof(IAsyncAction).GUID;

                return accountsSettingsPaneInterop.ShowManagedAccountsForWindowAsync(hWnd, ref guid);
            }
            public static IAsyncAction ShowAddAccountForWindowAsync(IntPtr hWnd)
            {
                IAccountsSettingsPaneInterop accountsSettingsPaneInterop = (IAccountsSettingsPaneInterop)WindowsRuntimeMarshal.GetActivationFactory(typeof(AccountsSettingsPane));
                Guid guid = typeof(IAsyncAction).GUID;

                return accountsSettingsPaneInterop.ShowAddAccountForWindowAsync(hWnd, ref guid);
            }
        }
    }
    ```

## CoreDragDropManager
- [`CoreDragDropManager.GetForCurrentView()`](https://docs.microsoft.com/uwp/api/windows.applicationmodel.datatransfer.dragdrop.core.coredragdropmanager.getforcurrentview?view=winrt-19041)

    This method is no longer supported for Desktop apps. There is a C++ COM interop for this class, as well as a C# version of the interop.

    ### COM Interop
    The corresponding COM interop interface is `dragdropinterop`. There's currently no official documentation or samples on this - please follow up with the team if you have issues using this. 

    ### C# version of interop
    ```csharp
    namespace UWPInterop
    {
        [System.Runtime.InteropServices.Guid("5AD8CBA7-4C01-4DAC-9074-827894292D63")]
        [System.Runtime.InteropServices.InterfaceType(System.Runtime.InteropServices.ComInterfaceType.InterfaceIsIInspectable)]
        public interface IDragDropManagerInterop
        {
            CoreDragDropManager GetForWindow(IntPtr hWnd, [System.Runtime.InteropServices.In] ref Guid riid);
        }

        //Helper to initalize DragDropManager
        public static class DragDropManagerInterop
        {
            public static CoreDragDropManager GetForWindow(IntPtr hWnd)
            {
                IDragDropManagerInterop dragDropManagerInterop = (IDragDropManagerInterop)WindowsRuntimeMarshal.GetActivationFactory(typeof(CoreDragDropManager));
                Guid guid = typeof(CoreDragDropManager).GUID;

                return dragDropManagerInterop.GetForWindow(hWnd, ref guid);
            }
        }
    }

    ```


## DataTransferManager
- [`DataTransferManager.GetForCurrentView()`](https://docs.microsoft.com/uwp/api/windows.applicationmodel.datatransfer.datatransfermanager.getforcurrentview?view=winrt-19041)

    This method is no longer supported for Desktop apps. There's a C++ COM interop that can be used as a workaround. Currently, there's no C# version of the interop for this method. 

    ### COM interop

    ```c++
    void DataTransferManager(bool delayRender = false)
    {
        wchar_t textBuffer[255];
        GetDlgItemTextW(m_hwndCombo, IDC_EDIT, textBuffer, ARRAYSIZE(textBuffer));
        winrt::hstring text{textBuffer};

        auto dataTransferMangerFactory =
            winrt::get_activation_factory<winrt::DataTransferManager, IDataTransferManagerInterop>();

        auto dataTransferManager = winrt::capture<winrt::DataTransferManager>(dataTransferMangerFactory, &IDataTransferManagerInterop::GetForWindow,
            m_dialogWindow);

        dataTransferManager.DataRequested([this, text, delayRender](auto&& sender, auto&& args) {
            auto dataPackage = args.Request().Data();
            dataPackage.Properties().Title(L"Win32 Share Example");

            if (delayRender)
            {
                dataPackage.SetDataProvider(winrt::StandardDataFormats::Text(), [this, text](auto&& request) -> winrt::fire_and_forget {
                    Log(L"delay render invoked\r\n");
                    auto deferral = request.GetDeferral();
                    winrt::InMemoryRandomAccessStream stream;
                    winrt::DataWriter dataWriter(stream);
                    dataWriter.UnicodeEncoding(winrt::Windows::Storage::Streams::UnicodeEncoding::Utf16LE);
                    dataWriter.WriteString(text);
                    dataWriter.WriteString(L"Delayed content from Win32 app.");
                    co_await dataWriter.StoreAsync();
                    request.SetData(dataWriter.DetachStream());
                    deferral.Complete();
                });
            }
            else
            {
                dataPackage.SetText(text);
            }
        });

        // Win32 clients must use the interop interface instead of dataTransferManager.ShowShareUI()
        winrt::check_hresult(dataTransferMangerFactory->ShowShareUIForWindow(m_dialogWindow));
    }
    ```

## EdgeGesture
- [`EdgeGesture.GetForCurrentView()`](https://docs.microsoft.com/uwp/api/windows.ui.input.edgegesture.getforcurrentview?view=winrt-19041)

    This method is no longer supported for Desktop apps. There's a C++ COM interop that can be used as a workaround. Currently, there's no C# version of the interop for this method. 

    ### COM interop

    ```c++
    void EdgeGesture()
    {
        SetFullscreen(m_dialogWindow);

        auto edgeGestureFactory  =
            winrt::get_activation_factory<winrt::Windows::UI::Input::EdgeGesture, IEdgeGestureStaticsInternal>();

        auto edgeGesture = winrt::capture<winrt::Windows::UI::Input::EdgeGesture>(edgeGestureFactory,
            &IEdgeGestureStaticsInternal::GetForWindow, m_dialogWindow);

        edgeGesture.Completed([this](auto&&, auto&&) { Log(__FUNCTIONW__ L"\r\n"); });
        edgeGesture.Starting([this](auto&&, auto&&) { Log(__FUNCTIONW__ L"\r\n"); });
        edgeGesture.Canceled([this](auto&&, auto&&) { Log(__FUNCTIONW__ L"\r\n"); });
    }
    ```

## InputPane
- [`InputPane.GetForCurrentView()`](https://docs.microsoft.com/uwp/api/windows.ui.viewmanagement.inputpane.getforcurrentview?view=winrt-19041)

    This method is no longer supported for Desktop apps. There is a C++ COM interop interface for this class, as well as a C# version of the interop.

    ### COM interop

    See the [IInteropPaneInterop interface](https://docs.microsoft.com/windows/win32/api/inputpaneinterop/nn-inputpaneinterop-iinputpaneinterop) documentation, and the sample code below.

    ```c++
    void InputPaneTryShow()
    {
        auto inputPaneInterop = winrt::get_activation_factory<winrt::InputPane, IInputPaneInterop>();
        auto inputPane = winrt::capture<winrt::InputPane>(inputPaneInterop, &IInputPaneInterop::GetForWindow, m_dialogWindow);

        inputPane.Visible(true);
        inputPane.TryShow();
    }
    ```

    ### C# version of interop
    ```csharp
    namespace UWPInterop
    {
        [System.Runtime.InteropServices.Guid("75CF2C57-9195-4931-8332-F0B409E916AF")]
        [System.Runtime.InteropServices.InterfaceType(System.Runtime.InteropServices.ComInterfaceType.InterfaceIsIInspectable)]
        public interface IInputPaneInterop
        {
            InputPane GetForWindow(IntPtr appWindow, [System.Runtime.InteropServices.In] ref Guid riid);
        }

        //Helper to initialize InputPane
        public static class InputPaneInterop
        {
            public static InputPane GetForWindow(IntPtr hWnd)
            {
                IInputPaneInterop inputPaneInterop = (IInputPaneInterop)WindowsRuntimeMarshal.GetActivationFactory(typeof(InputPane));
                Guid guid = typeof(InputPane).GUID;

                return inputPaneInterop.GetForWindow(hWnd, ref guid);
            }
        }
    }
    ```

## PlayToManager
- [`PlayToManager.GetForCurrentView()`](https://docs.microsoft.com/uwp/api/windows.media.playto.playtomanager.getforcurrentview?view=winrt-190411)

    This method is no longer supported for Desktop apps. There is a C++ COM interop interface for this class, as well as a C# version of the interop.

    ### COM interop

    See the [IPlayToManagerInterop interface](https://docs.microsoft.com/en-us/windows/win32/api/playtomanagerinterop/nn-playtomanagerinterop-iplaytomanagerinterop) documentation.

    ### C# version of interop
    ```csharp
    namespace UWPInterop
    {
        [System.Runtime.InteropServices.Guid("24394699-1F2C-4EB3-8CD7-0EC1DA42A540")]
        [System.Runtime.InteropServices.InterfaceType(System.Runtime.InteropServices.ComInterfaceType.InterfaceIsIInspectable)]
        public interface IPlayToManagerInterop
        {
            [Obsolete]
            PlayToManager GetForWindow(IntPtr appWindow, [System.Runtime.InteropServices.In] ref Guid riid);
            void ShowPlayToUIForWindow(IntPtr appWindow);
        }

        //Helper to initialize PlayToManager
        public static class PlayToManagerInterop
        {
            [Obsolete]
            public static PlayToManager GetForWindow(IntPtr hWnd)
            {
                IPlayToManagerInterop playToManagerInterop = (IPlayToManagerInterop)WindowsRuntimeMarshal.GetActivationFactory(typeof(PlayToManager));
                Guid guid = typeof(PlayToManager).GUID;

                return playToManagerInterop.GetForWindow(hWnd, ref guid);
            }
            public static void ShowPlayToUIForWindow(IntPtr hWnd)
            {
                IPlayToManagerInterop playToManagerInterop = (IPlayToManagerInterop)WindowsRuntimeMarshal.GetActivationFactory(typeof(PlayToManager));
                playToManagerInterop.ShowPlayToUIForWindow(hWnd);
            }
        }
    }
    ```

## Print3DManager
- [`Print3DManager.GetForCurrentView()`](https://docs.microsoft.com/uwp/api/windows.graphics.printing3d.print3dmanager.getforcurrentview?view=winrt-19041)

    This method is no longer supported for Desktop apps. There is a C++ COM interop interface for this class, as well as a C# version of the interop.

    ### COM interop

    The corresponding COM interop interface is `Print3DManagerInterop`. There's currently no official documentation or samples on this - please follow up with the team if you have issues using this. 

    ### C# version of interop
    ```csharp
    namespace UWPInterop
    {
        [System.Runtime.InteropServices.Guid("9CA31010-1484-4587-B26B-DDDF9F9CAECD")]
        [System.Runtime.InteropServices.InterfaceType(System.Runtime.InteropServices.ComInterfaceType.InterfaceIsIInspectable)]
        public interface IPrinting3DManagerInterop
        {
            Print3DManager GetForWindow(IntPtr appWindow, [System.Runtime.InteropServices.In] ref Guid riid);
            IAsyncOperation<bool> ShowPrintUIForWindowAsync(IntPtr appWindow, [System.Runtime.InteropServices.In] ref Guid riid);
        }

        //Helper to initialize Print3DManager
        public static class Print3DManagerInterop
        {
            public static Print3DManager GetForWindow(IntPtr hWnd)
            {
                IPrinting3DManagerInterop printing3DManagerInterop = (IPrinting3DManagerInterop)WindowsRuntimeMarshal.GetActivationFactory(typeof(Print3DManager));
                Guid guid = typeof(Print3DManager).GUID;

                return printing3DManagerInterop.GetForWindow(hWnd, ref guid);
            }
            public static IAsyncOperation<bool> ShowPrintUIForWindowAsync(IntPtr hWnd)
            {
                IPrinting3DManagerInterop printing3DManagerInterop = (IPrinting3DManagerInterop)WindowsRuntimeMarshal.GetActivationFactory(typeof(Print3DManager));
                Guid guid = typeof(IAsyncOperation<bool>).GUID;

                return printing3DManagerInterop.ShowPrintUIForWindowAsync(hWnd, ref guid);
            }
        }
    }
    ```

## PrintManager
- [`PrintManager.GetForCurrentView()`](https://docs.microsoft.com/uwp/api/windows.graphics.printing.printmanager.getforcurrentview?view=winrt-19041)

    This method is no longer supported for Desktop apps. There is a C++ COM interop interface for this class, as well as a C# version of the interop.

    ### COM interop

    See the [IPrintManagerInterop interface](https://docs.microsoft.com/windows/win32/api/printmanagerinterop/nn-printmanagerinterop-iprintmanagerinterop) documentation.

    ### C# version of interop
    ```csharp
    namespace UWPInterop
    {
        [System.Runtime.InteropServices.Guid("c5435a42-8d43-4e7b-a68a-ef311e392087")]
        [System.Runtime.InteropServices.InterfaceType(System.Runtime.InteropServices.ComInterfaceType.InterfaceIsIInspectable)]
        public interface IPrintManagerInterop
        {
            PrintManager GetForWindow(IntPtr appWindow, [System.Runtime.InteropServices.In] ref Guid riid);
            IAsyncOperation<bool> ShowPrintUIForWindowAsync(IntPtr appWindow, [System.Runtime.InteropServices.In] ref Guid riid);
        }

        //Helper to initialize PrintManager
        public static class PrintManagerInterop
        {
            public static PrintManager GetForWindow(IntPtr hWnd)
            {
                IPrintManagerInterop printManagerInterop = (IPrintManagerInterop)WindowsRuntimeMarshal.GetActivationFactory(typeof(PrintManager));
                Guid guid = typeof(PrintManager).GetInterface("IPrintManager").GUID;

                return printManagerInterop.GetForWindow(hWnd, ref guid);
            }
            public static IAsyncOperation<bool> ShowPrintUIForWindowAsync(IntPtr hWnd)
            {
                IPrintManagerInterop printManagerInterop = (IPrintManagerInterop)WindowsRuntimeMarshal.GetActivationFactory(typeof(PrintManager));
                Guid guid = typeof(IAsyncOperation<bool>).GUID;

                return printManagerInterop.ShowPrintUIForWindowAsync(hWnd, ref guid);
            }
        }
    }
    ```
## RadialControllerConfiguration
- [`RadialControllerConfiguration.GetForCurrentView()`](https://docs.microsoft.com/uwp/api/windows.ui.input.radialcontrollerconfiguration.getforcurrentview?view=winrt-19041)

    This method is no longer supported for Desktop apps. There is a C++ COM interop interface for this class, as well as a C# version of the interop.

    ### COM interop

    See the [IRadialControllerInterop interface](https://docs.microsoft.com/windows/win32/api/radialcontrollerinterop/nn-radialcontrollerinterop-iradialcontrollerconfigurationinterop) documentation, and the sample code below.

    ```c++
    void RadialDial()
    {
        auto controllerFactory = winrt::get_activation_factory<winrt::RadialController, IRadialControllerInterop>();
        m_radialController = winrt::capture<winrt::RadialController>(controllerFactory,
            &IRadialControllerInterop::CreateForWindow, m_dialogWindow);

        LogPrintf(
            L"RadialController::IsSupported() returned %s\r\n", m_radialController.IsSupported() ? L"true" : L"false");

        // m_radialController.Menu().IsEnabled(true);

        m_radialController.ButtonClicked([this](auto&& sender, auto&& args) { Log(__FUNCTIONW__ L"\r\n"); });
        m_radialController.RotationChanged([this](auto&& sender, auto&& args) { Log(__FUNCTIONW__ L"\r\n"); });
        m_radialController.ControlAcquired([this](auto&& sender, auto&& args) { Log(__FUNCTIONW__ L"\r\n"); });
        m_radialController.ScreenContactStarted([this](auto&& sender, auto&& args) { Log(__FUNCTIONW__ L"\r\n"); });;
    }
    ```

    ### C# version of interop
    ```csharp
   namespace UWPInterop
    {
        [System.Runtime.InteropServices.Guid("c5435a42-8d43-4e7b-a68a-ef311e392087")]
        [System.Runtime.InteropServices.InterfaceType(System.Runtime.InteropServices.ComInterfaceType.InterfaceIsIInspectable)]
        public interface IPrintManagerInterop
        {
            PrintManager GetForWindow(IntPtr appWindow, [System.Runtime.InteropServices.In] ref Guid riid);
            IAsyncOperation<bool> ShowPrintUIForWindowAsync(IntPtr appWindow, [System.Runtime.InteropServices.In] ref Guid riid);
        }

        //Helper to initialize PrintManager
        public static class PrintManagerInterop
        {
            public static PrintManager GetForWindow(IntPtr hWnd)
            {
                IPrintManagerInterop printManagerInterop = (IPrintManagerInterop)WindowsRuntimeMarshal.GetActivationFactory(typeof(PrintManager));
                Guid guid = typeof(PrintManager).GetInterface("IPrintManager").GUID;

                return printManagerInterop.GetForWindow(hWnd, ref guid);
            }
            public static IAsyncOperation<bool> ShowPrintUIForWindowAsync(IntPtr hWnd)
            {
                IPrintManagerInterop printManagerInterop = (IPrintManagerInterop)WindowsRuntimeMarshal.GetActivationFactory(typeof(PrintManager));
                Guid guid = typeof(IAsyncOperation<bool>).GUID;

                return printManagerInterop.ShowPrintUIForWindowAsync(hWnd, ref guid);
            }
        }
    }
    ```

## SearchPane
- [`SearchPane.GetForCurrentView()`](https://docs.microsoft.com/uwp/api/windows.applicationmodel.search.searchpane.getforcurrentview?view=winrt-19041)

    This method is no longer supported for Desktop apps. There's a C++ COM interop that can be used as a workaround. Currently, there's no C# version of the interop for this method. 

    ### COM interop

    ```c++
    void SearchPane()
    {
        auto searchPaneFactory =
            winrt::get_activation_factory<winrt::Windows::ApplicationModel::Search::SearchPane, ISearchPaneStaticsInternal>();
        auto searchPane = winrt::capture<winrt::Windows::ApplicationModel::Search::SearchPane>(
            searchPaneFactory, &ISearchPaneStaticsInternal::CreateSearchPaneClient);
        searchPane.as<IInitializeWithWindow>()->Initialize(m_dialogWindow);
        // searchPane.VisibilityChanged([](auto&& s, auto&& e) {});
        // searchPane.PlaceholderText(L"Placeholder");
        // searchPane.QuerySubmitted([](auto&& s, auto&& r) {});
        // searchPane.QueryChanged([](auto&& s, auto&& r) {});
        // searchPane.ResultSuggestionChosen([](auto&& s, auto&& r) {});
        searchPane.Show(L"Query Text");
    }
    ```

## SpatialInteractionManager
- [`SpatialInteractionManager.GetForCurrentView()`](https://docs.microsoft.com/uwp/api/windows.ui.input.spatial.spatialinteractionmanager.getforcurrentview?view=winrt-19041)

    This method is no longer supported for Desktop apps. There is a C++ COM interop for this class, as well as a C# version of the interop.

    ### COM Interop
    The corresponding COM interop interface is `SpatialInteractionManagerInterop`. There's currently no official documentation or samples on this - please follow up with the team if you have issues using this. 

    ### C# version of interop
    ```csharp
    namespace UWPInterop
    {
        [System.Runtime.InteropServices.Guid("5C4EE536-6A98-4B86-A170-587013D6FD4B")]
        [System.Runtime.InteropServices.InterfaceType(System.Runtime.InteropServices.ComInterfaceType.InterfaceIsIInspectable)]
        interface ISpatialInteractionManagerInterop
        {
            SpatialInteractionManager GetForWindow(IntPtr Window, [System.Runtime.InteropServices.In] ref Guid riid);
        }

        //Helper to initialize SpatialInteractionManager
        public static class SpatialInteractionManagerInterop
        {
            public static SpatialInteractionManager GetForWindow(IntPtr hWnd)
            {
                ISpatialInteractionManagerInterop spatialInteractionManagerInterop = (ISpatialInteractionManagerInterop)WindowsRuntimeMarshal.GetActivationFactory(typeof(SpatialInteractionManager));
                Guid guid = typeof(SpatialInteractionManager).GUID;

                return spatialInteractionManagerInterop.GetForWindow(hWnd, ref guid);
            }
        }
    }
    ```

## SystemMediaTransportControls
- [`SpatialInteractionManager.GetForCurrentView()`](https://docs.microsoft.com/uwp/api/windows.media.systemmediatransportcontrols.getforcurrentview?view=winrt-19041)

    This method is no longer supported for Desktop apps. There is a C++ COM interop for this class, as well as a C# version of the interop.

    ### COM Interop
    The corresponding COM interop interface is `SystemMediaTransportControlsInterop`. There's currently no official documentation or samples on this - please follow up with the team if you have issues using this. 

    ### C# version of interop
    ```csharp
    namespace UWPInterop
    {
        [System.Runtime.InteropServices.Guid("ddb0472d-c911-4a1f-86d9-dc3d71a95f5a")]
        [System.Runtime.InteropServices.InterfaceType(System.Runtime.InteropServices.ComInterfaceType.InterfaceIsIInspectable)]
        interface ISystemMediaTransportControlsInterop
        {
            SystemMediaTransportControls GetForWindow(IntPtr appWindow, [System.Runtime.InteropServices.In] ref Guid riid);
        }

        //Helper to initialize SystemMediaTransportControls
        public static class SystemMediaTransportControlsInterop
        {
            public static SystemMediaTransportControls GetForWindow(IntPtr hWnd)
            {
                ISystemMediaTransportControlsInterop systemMediaTransportControlsInterop = (ISystemMediaTransportControlsInterop)WindowsRuntimeMarshal.GetActivationFactory(typeof(SystemMediaTransportControls));
                Guid guid = typeof(SystemMediaTransportControls).GUID;

                return systemMediaTransportControlsInterop.GetForWindow(hWnd, ref guid);
            }
        }
    }
    ```

## UIViewSettings
- [`UIViewSettings.GetForCurrentView()`](https://docs.microsoft.com/uwp/api/windows.ui.viewmanagement.uiviewsettings.getforcurrentview?view=winrt-19041)

    This method is no longer supported for Desktop apps. There is a C++ COM interop for this class, as well as a C# version of the interop.

    ### COM Interop
    The corresponding COM interop interface is `UIViewSettingsInterop`. There's currently no official documentation or samples on this - please follow up with the team if you have issues using this. 

    ### C# version of interop
    ```csharp
    namespace UWPInterop
    {
        [System.Runtime.InteropServices.Guid("3694dbf9-8f68-44be-8ff5-195c98ede8a6")]
        [System.Runtime.InteropServices.InterfaceType(System.Runtime.InteropServices.ComInterfaceType.InterfaceIsIInspectable)]
        interface IUIViewSettingsInterop
        {
            UIViewSettings GetForWindow(IntPtr appWindow, [System.Runtime.InteropServices.In] ref Guid riid);
        }

        [System.Runtime.InteropServices.Guid("ECC62F5D-14AA-4971-9F06-B2159B1FFD40")]
        [System.Runtime.InteropServices.InterfaceType(System.Runtime.InteropServices.ComInterfaceType.InterfaceIsIUnknown)]
        interface IClassicApplicationViewFactory
        {
            UIViewSettings GetForWindow(IntPtr appWindow, [System.Runtime.InteropServices.In] ref Guid riid);
        }

        [System.Runtime.InteropServices.Guid("7A05F995-6242-440E-A64E-34B7ED3413D3")]
        [System.Runtime.InteropServices.InterfaceType(System.Runtime.InteropServices.ComInterfaceType.InterfaceIsIUnknown)]
        interface IClassicApplicationView
        {
            IntPtr GetTitleBar([System.Runtime.InteropServices.In] ref Guid riid);
            IntPtr GetActiveIcon();
            void SetActiveIcon(IntPtr value);
        }

        //Helper to initialize UIViewSettings
        public static class UIViewSettingsInterop
        {
            public static UIViewSettings GetForWindow(IntPtr hWnd)
            {
                IUIViewSettingsInterop uIViewSettingsInterop = (IUIViewSettingsInterop)WindowsRuntimeMarshal.GetActivationFactory(typeof(UIViewSettings));
                Guid guid = typeof(UIViewSettings).GUID;

                return uIViewSettingsInterop.GetForWindow(hWnd, ref guid);
            }
        }

    }
    ```

## UserActivityRequestManager
- [`UIViewSettings.GetForCurrentView()`](https://docs.microsoft.com/en-us/uwp/api/windows.applicationmodel.useractivities.useractivityrequestmanager.getforcurrentview?view=winrt-19041)

    This method is no longer supported for Desktop apps. There is a C++ COM interop for this class, as well as a C# version of the interop.

    ### COM Interop
    The corresponding COM interop interface is `UIViewSettingsInterop`. There's currently no official documentation or samples on this - please follow up with the team if you have issues using this. 

    ### C# version of interop
    ```csharp
    namespace UWPInterop
    {
        [System.Runtime.InteropServices.Guid("1ADE314D-0E0A-40D9-824C-9A088A50059F")]
        [System.Runtime.InteropServices.InterfaceType(System.Runtime.InteropServices.ComInterfaceType.InterfaceIsIInspectable)]
        public interface IUserActivityInterop
        {
            UserActivitySession CreateSessionForWindow(IntPtr appWindow, [System.Runtime.InteropServices.In] ref Guid riid);
        }

        [System.Runtime.InteropServices.Guid("C15DF8BC-8844-487A-B85B-7578E0F61419")]
        [System.Runtime.InteropServices.InterfaceType(System.Runtime.InteropServices.ComInterfaceType.InterfaceIsIInspectable)]
        public interface IUserActivitySourceHostInterop
        {
            void SetActivitySourceHost([MarshalAs(UnmanagedType.HString)] string activitySourceHost);
        }

        [System.Runtime.InteropServices.Guid("DD69F876-9699-4715-9095-E37EA30DFA1B")]
        [System.Runtime.InteropServices.InterfaceType(System.Runtime.InteropServices.ComInterfaceType.InterfaceIsIInspectable)]
        public interface IUserActivityRequestManagerInterop
        {
            UserActivityRequestManager GetForWindow(IntPtr appWindow, [System.Runtime.InteropServices.In] ref Guid riid);
        }

        //Helper to initialize UserActivity
        public static class UserActivityInterop
        {
            public static UserActivitySession CreateSessionForWindow(IntPtr hWnd)
            {
                IUserActivityInterop userActivityInterop = (IUserActivityInterop)WindowsRuntimeMarshal.GetActivationFactory(typeof(UserActivitySession));
                Guid guid = typeof(UserActivitySession).GUID;

                return userActivityInterop.CreateSessionForWindow(hWnd, ref guid);
            }

            public static void SetActivitySourceHost(string activitySourceHost)
            {
                IUserActivitySourceHostInterop userActivitySourceHostInterop = (IUserActivitySourceHostInterop)WindowsRuntimeMarshal.GetActivationFactory(typeof(UserActivityChannel));
                userActivitySourceHostInterop.SetActivitySourceHost(activitySourceHost);
            }
            public static UserActivityRequestManager GetForWindow(IntPtr hWnd)
            {
                IUserActivityRequestManagerInterop userActivityRequestManagerInterop = (IUserActivityRequestManagerInterop)WindowsRuntimeMarshal.GetActivationFactory(typeof(UserActivityRequestManager));
                Guid guid = typeof(UserActivityRequestManager).GUID;

                return userActivityRequestManagerInterop.GetForWindow(hWnd, ref guid);
            }
        }
    }
    ```