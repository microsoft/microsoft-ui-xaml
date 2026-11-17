// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Threading.Tasks;
using Microsoft.UI.Dispatching;
using Microsoft.UI.Windowing;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media;
using Windows.Graphics;
using Windows.Storage;

namespace WindowPlacementApp
{
    public sealed class PlacementWindow : Window
    {
        private static readonly List<PlacementWindow> s_openWindows = new();
        private static int s_windowCount;

        private readonly LaunchOptions m_options;
        private readonly bool m_isMain;
        private readonly TextBlock m_status;
        private readonly TextBlock m_details;
        private readonly TextBlock m_saved;
        private readonly DispatcherQueueTimer m_savedTimer;
        private readonly TextBox m_log;
        private bool m_closed;

        // Controls for opening new windows/processes. Main window only.
        private TextBox m_newId;
        private ComboBox m_newSetup;
        private CheckBox m_newKeepMinimized;
        private CheckBox m_newFixedSize;
        private CheckBox m_newLaunchFullScreen;
        private CheckBox m_newFakeMissingDisplay;

#if IXP_PROTOTYPE_PROJECTION
        private const PlacementRestorationBehavior AllowShowMinimized = PlacementRestorationBehavior.AllowShowMinimized;
#else
        // AllowShowMinimized is a proposed AppWindow flag. The NuGet IXP projection doesn't have it.
        // Build with IxpProjectionPath (see the csproj) to use the prototype projection instead.
        private const PlacementRestorationBehavior AllowShowMinimized = (PlacementRestorationBehavior)16;
#endif

        public static PlacementWindow Open(LaunchOptions options, bool isMain)
        {
            var window = new PlacementWindow(options, isMain);
            window.SetUpPlacementAndActivate();
            return window;
        }

        private PlacementWindow(LaunchOptions options, bool isMain)
        {
            m_options = options;
            m_isMain = isMain;
            int number = ++s_windowCount;
            Title = $"{(isMain ? "Main" : "Window")} #{number} [{options.Id}] pid {Environment.ProcessId}";

            var root = new StackPanel { Spacing = 8, Padding = new Thickness(12) };
            root.Children.Add(new TextBlock
            {
                Text = $"{Title}\nOptions: {options}",
                TextWrapping = TextWrapping.Wrap,
                FontWeight = Microsoft.UI.Text.FontWeights.SemiBold,
            });

            m_status = MonoText("StatusText");
            root.Children.Add(m_status);

            // Launcher first, so it is visible at the default window size.
            if (isMain)
            {
                root.Children.Add(CreateLauncherPanel());
            }

            var buttons = new VariableSizedWrapGrid { Orientation = Orientation.Horizontal, ItemWidth = 150, ItemHeight = 40 };
            AddButton(buttons, "Save", "SaveButton", Save);
            AddButton(buttons, "TryLoad", "TryLoadButton", TryLoad);
            AddButton(buttons, "GetCurrent", "GetCurrentButton", GetCurrent);
            AddButton(buttons, "Apply saved", "ApplySavedButton", ApplySaved);
            AddButton(buttons, "Copy to new window", "CopyWindowButton", CopyToNewWindow);
            AddButton(buttons, "Oversize", "OversizeButton", Oversize);
            AddButton(buttons, "Maximize", "MaximizeButton", () => Overlapped().Maximize());
            AddButton(buttons, "Minimize", "MinimizeButton", () => Overlapped().Minimize());
            AddButton(buttons, "Restore", "RestoreButton", () => Overlapped().Restore());
            AddButton(buttons, "Full screen", "FullScreenButton", () => AppWindow.SetPresenter(AppWindowPresenterKind.FullScreen));
            AddButton(buttons, "Compact overlay", "CompactOverlayButton", () => AppWindow.SetPresenter(AppWindowPresenterKind.CompactOverlay));
            AddButton(buttons, "Overlapped", "OverlappedButton", () => AppWindow.SetPresenter(AppWindowPresenterKind.Overlapped));
            AddButton(buttons, "Close", "CloseButton", Close);
            AddButton(buttons, "Close in 5s", "CloseIn5Button", () => _ = CloseAfterDelay(5000));
            AddButton(buttons, "Minimize + close", "MinimizeCloseButton", () => { Overlapped().Minimize(); _ = CloseAfterDelay(1000); });
            root.Children.Add(buttons);

            m_details = MonoText("DetailsText");
            root.Children.Add(m_details);

            // Raw record in WinUI's store for this PlacementId. Polled because other windows
            // and processes write to the same store, and WinUI saves after Closed fires.
            m_saved = MonoText("SavedText");
            root.Children.Add(m_saved);
            m_savedTimer = DispatcherQueue.CreateTimer();
            m_savedTimer.Interval = TimeSpan.FromSeconds(2);
            m_savedTimer.Tick += (sender, args) => RefreshSaved();
            m_savedTimer.Start();

            m_log = new TextBox
            {
                IsReadOnly = true,
                AcceptsReturn = true,
                TextWrapping = TextWrapping.Wrap,
                FontFamily = new FontFamily("Consolas"),
                FontSize = 11,
                Height = 180,
            };
            AutomationProperties.SetAutomationId(m_log, "LogText");
            root.Children.Add(m_log);
            Content = new ScrollViewer { Content = root, HorizontalScrollBarVisibility = ScrollBarVisibility.Disabled };

            Log.Changed += RefreshLog;
            RefreshLog();

            AppWindow.Changed += (sender, args) =>
            {
                if (args.DidPresenterChange)
                {
                    Log.Write($"{Title}: Presenter changed. {StatusLine()}");
                }
                if (args.DidPositionChange || args.DidSizeChange || args.DidPresenterChange)
                {
                    RefreshStatus();
                }
            };
            Closed += (sender, args) =>
            {
                bool winuiSaves = !string.IsNullOrEmpty(PlacementId) && PlacementMode != WindowPlacementMode.Manual;
                Log.Write(winuiSaves
                    ? $"{Title}: Closed. WinUI saves the placement during close."
                    : $"{Title}: Closed. WinUI does not save on close (Manual mode or no PlacementId).");
                m_closed = true;
                m_savedTimer.Stop();
                Log.Changed -= RefreshLog;
                s_openWindows.Remove(this);
            };
            s_openWindows.Add(this);
        }

        private void SetUpPlacementAndActivate()
        {
            if (m_options.Setup is not (PlacementSetup.None or PlacementSetup.AppStorage))
            {
                PlacementId = m_options.Id;
            }

            if (m_options.Restart)
            {
                // Spec example 2 (session restore). Allow minimized restoration too.
                // WinUI still does not restore full screen.
                AppWindow.PlacementRestorationBehavior = PlacementRestorationBehavior.All;
            }
            else if (m_options.KeepMinimized)
            {
                // A saved minimized window stays minimized. Works in every mode.
                AppWindow.PlacementRestorationBehavior |= AllowShowMinimized;
            }

            if (m_options.FixedSize)
            {
                Overlapped().IsResizable = false;
            }

            if (m_options.LaunchFullScreen)
            {
                ApplyFullScreenIfItWasLastTime();
            }
            else
            {
                ApplySetup();
            }

            IntPtr hwnd = WinRT.Interop.WindowNative.GetWindowHandle(this);
            bool visibleBeforeActivate = IsWindowVisible(hwnd);
            AppWindowPresenterKind presenterBeforeActivate = AppWindow.Presenter.Kind;
            Activate();
            Log.Write($"{Title}: Activate() done. Before Activate: visible={visibleBeforeActivate} presenter={presenterBeforeActivate}. {StatusLine()}");
            if (m_options.LaunchFullScreen && visibleBeforeActivate)
            {
                Log.Write($"{Title}: FAIL: the window was visible before Activate, so it can flash at its normal size.");
            }
            RefreshStatus();
        }

        // Spec example 4: start full screen if the window was full screen last time.
        // Same calls as the example, with the key based on this window's id. The app applies
        // placement itself, so this path always uses SaveOnClose, whatever Setup says.
        private void ApplyFullScreenIfItWasLastTime()
        {
            var settings = ApplicationData.Current.LocalSettings.Values;
            string wasFullScreenKey = $"{m_options.Id}.WasFullScreen";

            PlacementId = m_options.Id;
            PlacementMode = WindowPlacementMode.SaveOnClose;
            Log.Write($"{Title}: Example 4. PlacementMode = SaveOnClose (Setup {m_options.Setup} is ignored on this path).");

            // Not in spec example 4 yet. The default behavior (A7) doesn't include
            // AllowShowFullScreen, and SetCurrentPlacement honors it (A4). Without this line,
            // SetCurrentPlacement drops PlacementInfo.FullScreen and the window opens normal.
            AppWindow.PlacementRestorationBehavior |= PlacementRestorationBehavior.AllowShowFullScreen;
            Log.Write($"{Title}: PlacementRestorationBehavior |= AllowShowFullScreen -> {AppWindow.PlacementRestorationBehavior}");

            Closed += (s, e) =>
            {
                settings[wasFullScreenKey] =
                    AppWindow.Presenter.Kind == AppWindowPresenterKind.FullScreen;
                Log.Write($"{Title}: Closed. settings[\"{wasFullScreenKey}\"] = {settings[wasFullScreenKey]}");
            };

            try
            {
                AppWindowPlacementDetails saved = TryLoadPlacementDetails();
                Log.Write($"{Title}: TryLoad -> {Describe(saved)}");
                if (saved != null)
                {
                    if (settings[wasFullScreenKey] is true)
                    {
                        saved = AppWindowPlacementDetails.Create(
                            saved.NormalRect, saved.WorkArea, saved.Dpi, saved.ShowCmd,
                            saved.ArrangeRect, saved.Flags | PlacementInfo.FullScreen, saved.DeviceName);
                        Log.Write($"{Title}: settings[\"{wasFullScreenKey}\"] is true. Added PlacementInfo.FullScreen -> {Describe(saved)}");
                    }
                    else
                    {
                        Log.Write($"{Title}: settings[\"{wasFullScreenKey}\"] = {settings[wasFullScreenKey] ?? "null"}. Not adding full screen. " +
                            "It wasn't full screen when it last closed. To test example 4: Full screen, close, then open it again.");
                    }
                    bool applied = AppWindow.SetCurrentPlacement(saved, isFirstWindow: true);
                    Log.Write($"{Title}: SetCurrentPlacement(saved, isFirstWindow:true) -> {applied}. presenter={AppWindow.Presenter.Kind}");
                }
                else
                {
                    Log.Write($"{Title}: Nothing saved yet. Opening normal. To test example 4: Full screen, close, then open it again.");
                }
            }
            catch (Exception ex)
            {
                Log.Write($"{Title}: FAIL: example 4 threw 0x{ex.HResult:X8} {ex.GetType().Name}: {ex.Message}");
            }
        }

        private void ApplySetup()
        {
            // When the app applies placement itself, it passes isFirstWindow. The main window is
            // this process's first window, so it honors STARTUPINFO. RestoreAndSave always
            // passes false, because WinUI can't know which window is the app's first.
            bool isFirstWindow = m_isMain;

            switch (m_options.Setup)
            {
                case PlacementSetup.SaveOnClose:
                case PlacementSetup.Manual:
                {
                    // Same calls that RestoreAndSave makes, written by the app.
                    // SaveOnClose: WinUI saves on close. Manual (spec example 7): only the Save
                    // button saves.
                    PlacementMode = m_options.Setup == PlacementSetup.SaveOnClose ? WindowPlacementMode.SaveOnClose : WindowPlacementMode.Manual;
                    AppWindowPlacementDetails saved = TryLoadPlacementDetails();
                    Log.Write($"{Title}: {PlacementMode}. TryLoad -> {Describe(saved)}");
                    if (saved != null)
                    {
                        bool applied = AppWindow.SetCurrentPlacement(saved, isFirstWindow);
                        Log.Write($"{Title}: SetCurrentPlacement(saved, isFirstWindow:{isFirstWindow}) -> {applied}");
                    }
                    break;
                }
                case PlacementSetup.ManualSaveInClosed:
                {
                    // Overview "Manual placement" sample. Same calls as the doc, plus logging.
                    PlacementMode = WindowPlacementMode.Manual;
                    AppWindowPlacementDetails saved = TryLoadPlacementDetails();
                    Log.Write($"{Title}: Manual. TryLoad -> {Describe(saved)}");
                    if (saved != null)
                    {
                        if (m_options.FakeMissingDisplay)
                        {
                            saved = FakeMissingDisplay(saved);
                            Log.Write($"{Title}: Fake missing display -> {Describe(saved)}");
                        }
#if IXP_PROTOTYPE_PROJECTION
                        DisplayArea savedDisplay = saved.TryGetDisplayArea();
                        Log.Write($"{Title}: TryGetDisplayArea() -> {Describe(savedDisplay)}");
                        if (savedDisplay == null)
                        {
                            saved = saved.MoveToDisplayArea(DisplayArea.Primary);
                            Log.Write($"{Title}: MoveToDisplayArea(DisplayArea.Primary) -> {Describe(saved)}");
                        }
#else
                        Log.Write($"{Title}: Skipped the display check. TryGetDisplayArea and MoveToDisplayArea need a build with IxpProjectionPath.");
#endif
                        bool applied = AppWindow.SetCurrentPlacement(saved, isFirstWindow: true);
                        Log.Write($"{Title}: SetCurrentPlacement(saved, isFirstWindow:true) -> {applied}");
                    }
                    Closed += (s, e) =>
                    {
                        SavePlacementDetails();
                        Log.Write($"{Title}: Closed. SavePlacementDetails() done.");
                    };
                    break;
                }
                case PlacementSetup.SaveOnCloseCentered:
                {
                    // Spec example 3.
                    PlacementMode = WindowPlacementMode.SaveOnClose;
                    AppWindowPlacementDetails saved = TryLoadPlacementDetails();
                    Log.Write($"{Title}: SaveOnClose. TryLoad -> {Describe(saved)}");
                    if (saved != null)
                    {
                        RectInt32 work = saved.WorkArea;
                        RectInt32 r = saved.NormalRect;
                        r.X = work.X + (work.Width - r.Width) / 2;
                        r.Y = work.Y + (work.Height - r.Height) / 2;
                        var centered = AppWindowPlacementDetails.Create(
                            r, saved.WorkArea, saved.Dpi, saved.ShowCmd,
                            saved.ArrangeRect, saved.Flags, saved.DeviceName);
                        bool applied = AppWindow.SetCurrentPlacement(centered, isFirstWindow);
                        Log.Write($"{Title}: SetCurrentPlacement(centered, isFirstWindow:{isFirstWindow}) -> {applied}");
                    }
                    else
                    {
                        AppWindow.Resize(new SizeInt32(1200, 800));
                        Log.Write($"{Title}: Nothing saved. Resize(1200x800).");
                    }
                    break;
                }
                case PlacementSetup.AppStorage:
                {
                    // Spec example 5. No PlacementId, so WinUI does nothing extra.
                    AppWindowPlacementDetails saved = MyStore.Load(m_options.Id);
                    Log.Write($"{Title}: AppStorage. MyStore.Load -> {Describe(saved)}");
                    if (saved != null)
                    {
                        bool applied = AppWindow.SetCurrentPlacement(saved, isFirstWindow);
                        Log.Write($"{Title}: SetCurrentPlacement(saved, isFirstWindow:{isFirstWindow}) -> {applied}");
                    }
                    Closed += (s, e) => SaveToMyStore();
                    break;
                }
                case PlacementSetup.SaveOnCloseFixedRect:
                {
                    PlacementMode = WindowPlacementMode.SaveOnClose;
                    Log.Write($"{Title}: SaveOnClose. Ignoring saved data. MoveAndResize to a fixed rect.");
                    AppWindow.MoveAndResize(new RectInt32(200, 200, 800, 600));
                    break;
                }
                default:
                    // RestoreAndSave (or None): Activate() does the work.
                    break;
            }
        }

        private Panel CreateLauncherPanel()
        {
            var panel = new StackPanel { Spacing = 6, BorderThickness = new Thickness(0, 1, 0, 0), Padding = new Thickness(0, 8, 0, 0) };
            panel.BorderBrush = (Brush)Application.Current.Resources["ControlStrokeColorDefaultBrush"];
            panel.Children.Add(new TextBlock { Text = "Open a window or process with these options:", FontWeight = Microsoft.UI.Text.FontWeights.SemiBold });

            var row = new StackPanel { Orientation = Orientation.Horizontal, Spacing = 8 };
            m_newId = new TextBox { Header = "PlacementId", Text = "Doc", Width = 120 };
            AutomationProperties.SetAutomationId(m_newId, "NewIdBox");
            m_newSetup = EnumCombo<PlacementSetup>("Setup", "NewSetupCombo", PlacementSetup.RestoreAndSave);
            m_newKeepMinimized = new CheckBox { Content = "Keep minimized", IsChecked = false, VerticalAlignment = VerticalAlignment.Bottom };
            AutomationProperties.SetAutomationId(m_newKeepMinimized, "NewKeepMinimizedCheck");
            m_newFixedSize = new CheckBox { Content = "Not resizable", IsChecked = false, VerticalAlignment = VerticalAlignment.Bottom };
            AutomationProperties.SetAutomationId(m_newFixedSize, "NewFixedSizeCheck");
            m_newLaunchFullScreen = new CheckBox { Content = "Full screen if it was last time (ex. 4)", IsChecked = false, VerticalAlignment = VerticalAlignment.Bottom };
            ToolTipService.SetToolTip(m_newLaunchFullScreen,
                "Spec example 4. Uses SaveOnClose, whatever Setup says. The window opens full screen only if it was full screen when it last closed. " +
                "To test: open it, click Full screen, close it, then open it again with this box checked.");
            AutomationProperties.SetAutomationId(m_newLaunchFullScreen, "NewLaunchFullScreenCheck");
            m_newFakeMissingDisplay = new CheckBox { Content = "Fake missing display", IsChecked = false, VerticalAlignment = VerticalAlignment.Bottom };
            ToolTipService.SetToolTip(m_newFakeMissingDisplay, "ManualSaveInClosed only: the loaded details name a display that doesn't exist, so TryGetDisplayArea returns null.");
            AutomationProperties.SetAutomationId(m_newFakeMissingDisplay, "NewFakeMissingDisplayCheck");
            row.Children.Add(m_newId);
            row.Children.Add(m_newSetup);
            panel.Children.Add(row);
            var checks = new StackPanel { Orientation = Orientation.Horizontal, Spacing = 8 };
            checks.Children.Add(m_newKeepMinimized);
            checks.Children.Add(m_newFixedSize);
            checks.Children.Add(m_newLaunchFullScreen);
            checks.Children.Add(m_newFakeMissingDisplay);
            panel.Children.Add(checks);

            var buttons = new VariableSizedWrapGrid { Orientation = Orientation.Horizontal, ItemWidth = 150, ItemHeight = 40 };
            AddButton(buttons, "Open window", "OpenWindowButton", () => Open(NewOptions(), isMain: false));
            AddButton(buttons, "Open process", "OpenProcessButton", () => StartProcess(NewOptions().ToCommandLine(restart: false), minimized: false));
            AddButton(buttons, "Open process min", "OpenProcessMinButton", () => StartProcess(NewOptions().ToCommandLine(restart: false), minimized: true));
            AddButton(buttons, "Save as startup", "SaveStartupButton", () => { NewOptions().SaveAsStartupDefaults(); Log.Write($"Startup options saved: {NewOptions()}"); });
            AddButton(buttons, "Reset startup", "ResetStartupButton", () => { LaunchOptions.ClearStartupDefaults(); Log.Write("Startup options reset."); });
            AddButton(buttons, "Relaunch", "RelaunchButton", () => _ = Relaunch(minimizeFirst: false, restart: false));
            AddButton(buttons, "Minimize + relaunch", "MinimizeRelaunchButton", () => _ = Relaunch(minimizeFirst: true, restart: false));
            AddButton(buttons, "Restart app", "RestartButton", () => _ = Relaunch(minimizeFirst: false, restart: true));
            AddButton(buttons, "Minimize + restart", "MinimizeRestartButton", () => _ = Relaunch(minimizeFirst: true, restart: true));
            AddButton(buttons, "Kill + relaunch", "KillRelaunchButton", KillAndRelaunch);
            AddButton(buttons, "Clear all data", "ClearDataButton", ClearAllData);
            panel.Children.Add(buttons);
            panel.Children.Add(new Expander
            {
                Header = "Scenarios",
                HorizontalAlignment = HorizontalAlignment.Stretch,
                HorizontalContentAlignment = HorizontalAlignment.Left,
                Content = new TextBlock { Text = ScenarioText, TextWrapping = TextWrapping.Wrap, IsTextSelectionEnabled = true },
            });
            return panel;
        }

        private const string ScenarioText =
            "Relaunch and Restart close every window here (WinUI saves each one), then start a new process with the main window's options.\n" +
            "1. Move or resize, then Relaunch. Same rect.\n" +
            "2. Maximize, then Relaunch. Maximized.\n" +
            "3. Minimize + relaunch. Restored and active (default behavior drops minimized).\n" +
            "4. Minimize + restart. Minimized and not active (--restart sets PlacementRestorationBehavior = All).\n" +
            "5. Maximize, Full screen, then Relaunch. Maximized, not full screen (TryLoad strips FullScreen).\n" +
            "6. Full screen, then Copy to new window. The copy uses GetCurrentPlacement (keeps FullScreen) and behavior All. " +
            "The log shows it stayed hidden and inactive during SetCurrentPlacement. It opens full screen. Try it from maximized too.\n" +
            "7. Setup SaveOnCloseCentered / SaveOnCloseFixedRect: Open window. Centered saved size / (200,200) 800x600. WinUI still saves on close.\n" +
            "8. Not resizable: Oversize, Save, then Open window with Not resizable. Kept size, moved on screen. Without Not resizable: shrunk to fit.\n" +
            "9. Open process min with Setup SaveOnClose: main window honors STARTUPINFO (minimized). With RestoreAndSave: not minimized.\n" +
            "9b. Setup Manual: Open window, move it, Close, Open window again. Old rect (no save on close). Save button saves.\n" +
            "10. Setup None: no PlacementId, nothing saved or restored.\n" +
            "11. Clear all data, then Relaunch. OS default position.\n" +
            "\n" +
            "Spec examples (window-placement-spec.md, \"Examples\"). Each runs the same calls as the doc:\n" +
            "Ex1 RestoreAndSave: Setup RestoreAndSave. Move, then Relaunch. Same rect.\n" +
            "Ex2 Session restore: Minimize + restart. --restart sets PlacementRestorationBehavior = All. Comes back minimized. " +
            "Full screen + Restart app: comes back Overlapped (WinUI doesn't restore full screen).\n" +
            "Ex3 SaveOnClose, adjust, apply: Setup SaveOnCloseCentered. Open window. Centered saved size. " +
            "After Clear all data: Resize(1200x800).\n" +
            "Ex4 Full screen if it was: check Full screen if it was last time, Open process. Uses SaveOnClose. It opens normal the first time. " +
            "In it: Full screen, then Relaunch. It first appears already full screen, on the same display, with no normal-size flash. Overlapped goes back to the normal or maximized state from before full screen. " +
            "Leave full screen, Relaunch again: opens Overlapped. Open window works the same way inside one process.\n" +
            "Ex5 App's own storage: Setup AppStorage. Open window, move, Close, Open window. Same rect. " +
            "No PlacementId. Saved record panel shows MyStore.\n" +
            "Ex6 Save before close: Save, move the window, then Kill + relaunch. Opens at the saved rect, not the moved one.\n" +
            "Ex7 Manual: Setup Manual. Open window, move, Close, Open window. Old rect. Save button saves.\n" +
            "Overview \"Automatic placement\": same as Ex1.\n" +
            "Overview \"Manual placement\": Setup ManualSaveInClosed. Open window, move, Close, Open window. Same rect. " +
            "The log shows TryGetDisplayArea returned the display. " +
            "Check Fake missing display too: TryGetDisplayArea returns null and MoveToDisplayArea(DisplayArea.Primary) " +
            "puts the window back on the primary display. Needs a build with IxpProjectionPath.";

        private LaunchOptions NewOptions() => new LaunchOptions
        {
            Id = m_newId.Text,
            Setup = Enum.Parse<PlacementSetup>((string)m_newSetup.SelectedItem),
            KeepMinimized = m_newKeepMinimized.IsChecked == true,
            FixedSize = m_newFixedSize.IsChecked == true,
            LaunchFullScreen = m_newLaunchFullScreen.IsChecked == true,
            FakeMissingDisplay = m_newFakeMissingDisplay.IsChecked == true,
        };

        private void Save()
        {
            if (m_options.Setup == PlacementSetup.AppStorage)
            {
                SaveToMyStore();
                return;
            }
            SavePlacementDetails();
            Log.Write($"{Title}: SavePlacementDetails(). Stored: {Describe(TryLoadPlacementDetails())}");
        }

        // Spec example 5: the app saves GetCurrentPlacement in its own store.
        private void SaveToMyStore()
        {
            AppWindowPlacementDetails current = AppWindow.GetCurrentPlacement();
            MyStore.Save(m_options.Id, current);
            Log.Write($"{Title}: MyStore.Save(GetCurrentPlacement) -> {Describe(current)}");
        }

        private void TryLoad()
        {
            AppWindowPlacementDetails details = TryLoadPlacementDetails();
            m_details.Text = "TryLoadPlacementDetails:\n" + Describe(details, multiline: true);
            Log.Write($"{Title}: TryLoad -> {Describe(details)}");
        }

        private void GetCurrent()
        {
            AppWindowPlacementDetails details = AppWindow.GetCurrentPlacement();
            m_details.Text = "AppWindow.GetCurrentPlacement:\n" + Describe(details, multiline: true);
            Log.Write($"{Title}: GetCurrentPlacement -> {Describe(details)}");
        }

        private void ApplySaved()
        {
            AppWindowPlacementDetails details = TryLoadPlacementDetails();
            if (details == null)
            {
                Log.Write($"{Title}: Apply saved: nothing saved.");
                return;
            }
            bool applied = AppWindow.SetCurrentPlacement(details, false);
            Log.Write($"{Title}: SetCurrentPlacement(saved, isFirstWindow:false) -> {applied}. {StatusLine()}");
        }

        // App-owned store path. GetCurrentPlacement keeps FullScreen (TryLoad strips it). The copy
        // is hidden until Activate, so SetCurrentPlacement must keep it hidden and inactive.
        private void CopyToNewWindow()
        {
            AppWindowPlacementDetails details = AppWindow.GetCurrentPlacement();
            Log.Write($"{Title}: Copy. GetCurrentPlacement -> {Describe(details)}");
            var options = m_options.Clone();
            options.Setup = PlacementSetup.None;
            var copy = new PlacementWindow(options, isMain: false);
            copy.AppWindow.PlacementRestorationBehavior = PlacementRestorationBehavior.All;
            IntPtr hwnd = WinRT.Interop.WindowNative.GetWindowHandle(copy);
            bool visibleBefore = IsWindowVisible(hwnd);
            bool applied = copy.AppWindow.SetCurrentPlacement(details, false);
            bool visibleAfter = IsWindowVisible(hwnd);
            bool foregroundAfter = GetForegroundWindow() == hwnd;
            Log.Write($"{copy.Title}: SetCurrentPlacement(copy, All) -> {applied}. Visible before={visibleBefore} after={visibleAfter} foreground={foregroundAfter} " +
                $"presenter={copy.AppWindow.Presenter.Kind}. {(visibleAfter || foregroundAfter ? "FAIL: hidden contract broken" : "PASS: stayed hidden and inactive")}");
            copy.Activate();
            Log.Write($"{copy.Title}: Activate() done. {copy.StatusLine()}");
            copy.RefreshStatus();
        }

        // Makes the window bigger than its work area, for the Not resizable scenario.
        private void Oversize()
        {
            RectInt32 work = DisplayArea.GetFromWindowId(AppWindow.Id, DisplayAreaFallback.Nearest).WorkArea;
            Overlapped().Restore();
            AppWindow.MoveAndResize(new RectInt32(work.X + 20, work.Y + 20, work.Width + 300, work.Height + 200));
            Log.Write($"{Title}: Oversize. Work area {work.Width}x{work.Height}. {StatusLine()}");
        }

        [System.Runtime.InteropServices.DllImport("user32.dll")]
        private static extern bool IsWindowVisible(IntPtr hwnd);

        [System.Runtime.InteropServices.DllImport("user32.dll")]
        private static extern IntPtr GetForegroundWindow();

        private static void StartProcess(string args, bool minimized)
        {
            var info = new ProcessStartInfo(Environment.ProcessPath, args)
            {
                UseShellExecute = true,
                WindowStyle = minimized ? ProcessWindowStyle.Minimized : ProcessWindowStyle.Normal,
            };
            Process.Start(info);
            Log.Write($"Started process: {args}{(minimized ? " (SW_SHOWMINNOACTIVE)" : "")}");
        }

        // Closes every window here so WinUI saves each one's placement, then starts a new process
        // with the main window's options. restart adds --restart (keeps a minimized window minimized).
        private async Task Relaunch(bool minimizeFirst, bool restart)
        {
            if (minimizeFirst)
            {
                Overlapped().Minimize();
                await Task.Delay(1000);
            }
            string args = m_options.ToCommandLine(restart);
            foreach (PlacementWindow window in s_openWindows.ToList())
            {
                window.Close();
            }
            StartProcess(args, minimized: false);
        }

        // For spec example 6. Ends this process without closing its windows, so WinUI doesn't save
        // on close. Only an earlier SavePlacementDetails (the Save button) is kept.
        private void KillAndRelaunch()
        {
            Log.Write("Kill + relaunch: no window closes, so WinUI doesn't save on close.");
            StartProcess(m_options.ToCommandLine(restart: false), minimized: false);
            Process.GetCurrentProcess().Kill();
        }

        // WinUI saves each window's placement when it closes. So clearing the data alone isn't
        // enough: the next Relaunch closes the windows and saves them again. Setting PlacementId
        // to "" turns off that save, so the cleared data stays cleared.
        private void ClearAllData()
        {
            foreach (PlacementWindow window in s_openWindows)
            {
                window.PlacementId = "";
            }
            foreach (var container in ApplicationData.Current.LocalSettings.Containers.Keys.ToList())
            {
                ApplicationData.Current.LocalSettings.DeleteContainer(container);
            }
            ApplicationData.Current.LocalSettings.Values.Clear();
            Log.Write("Cleared all LocalSettings (saved placements and startup options). Set PlacementId=\"\" on open windows so closing them doesn't save again.");
        }

        private async Task CloseAfterDelay(int ms)
        {
            Log.Write($"{Title}: closing in {ms} ms");
            await Task.Delay(ms);
            if (!m_closed)
            {
                Close();
            }
        }

        private OverlappedPresenter Overlapped()
        {
            if (AppWindow.Presenter is not OverlappedPresenter)
            {
                AppWindow.SetPresenter(AppWindowPresenterKind.Overlapped);
            }
            return (OverlappedPresenter)AppWindow.Presenter;
        }

        private string StatusLine()
        {
            AppWindow appWindow = AppWindow;
            string state = appWindow.Presenter is OverlappedPresenter overlapped
                ? overlapped.State.ToString()
                : appWindow.Presenter.Kind.ToString();
            return $"state={state} pos=({appWindow.Position.X},{appWindow.Position.Y}) size={appWindow.Size.Width}x{appWindow.Size.Height}";
        }

        private void RefreshStatus()
        {
            if (!m_closed)
            {
                m_status.Text = $"PlacementId=\"{PlacementId}\" PlacementMode={PlacementMode} RestorationBehavior={AppWindow.PlacementRestorationBehavior}\n{StatusLine()}";
                RefreshSaved();
            }
        }

        private void RefreshSaved()
        {
            if (m_closed)
            {
                return;
            }
            string text;
            try
            {
                text = m_options.Setup == PlacementSetup.AppStorage
                    ? $"(AppStorage, from MyStore)\n{Describe(MyStore.Load(m_options.Id), multiline: true)}"
                    : SavedRecord.Describe(PlacementId);
            }
            catch (Exception ex)
            {
                text = $"(read failed: 0x{ex.HResult:X8} {ex.Message})";
            }
            m_saved.Text = $"Saved record for \"{(m_options.Setup == PlacementSetup.AppStorage ? m_options.Id : PlacementId)}\":\n{text}";
        }

        private void RefreshLog()
        {
            if (!m_closed)
            {
                m_log.Text = string.Join("\n", Log.Lines.Reverse().Take(200));
            }
        }

        private static string Describe(AppWindowPlacementDetails d, bool multiline = false)
        {
            if (d == null)
            {
                return "null";
            }
            string sep = multiline ? "\n" : "; ";
            return string.Join(sep,
                $"NormalRect=({d.NormalRect.X},{d.NormalRect.Y} {d.NormalRect.Width}x{d.NormalRect.Height})",
                $"WorkArea=({d.WorkArea.X},{d.WorkArea.Y} {d.WorkArea.Width}x{d.WorkArea.Height})",
                $"ArrangeRect=({d.ArrangeRect.X},{d.ArrangeRect.Y} {d.ArrangeRect.Width}x{d.ArrangeRect.Height})",
                $"Dpi={d.Dpi}",
                $"ShowCmd={d.ShowCmd}",
                $"Flags={d.Flags}",
                $"Device={d.DeviceName}");
        }

        private static string Describe(DisplayArea area)
        {
            if (area == null)
            {
                return "null";
            }
            RectInt32 w = area.WorkArea;
            return $"DisplayId={area.DisplayId.Value}; WorkArea=({w.X},{w.Y} {w.Width}x{w.Height}); IsPrimary={area.IsPrimary}";
        }

        // Pretend the saved display was a second monitor to the right that is gone now.
        private static AppWindowPlacementDetails FakeMissingDisplay(AppWindowPlacementDetails d)
        {
            int dx = d.WorkArea.Width;
            RectInt32 Shift(RectInt32 r) => (r.Width == 0 && r.Height == 0) ? r : new RectInt32(r.X + dx, r.Y, r.Width, r.Height);
            return AppWindowPlacementDetails.Create(
                Shift(d.NormalRect),
                Shift(d.WorkArea),
                d.Dpi,
                d.ShowCmd,
                Shift(d.ArrangeRect),
                d.Flags,
                @"\\.\FAKEDISPLAY9");
        }

        private static TextBlock MonoText(string automationId)
        {
            var text = new TextBlock
            {
                FontFamily = new FontFamily("Consolas"),
                IsTextSelectionEnabled = true,
                TextWrapping = TextWrapping.Wrap,
            };
            AutomationProperties.SetAutomationId(text, automationId);
            return text;
        }

        private static ComboBox EnumCombo<T>(string header, string automationId, T selected) where T : struct, Enum
        {
            var combo = new ComboBox { Header = header, MinWidth = 150 };
            foreach (T value in Enum.GetValues<T>())
            {
                combo.Items.Add(value.ToString());
            }
            combo.SelectedItem = selected.ToString();
            AutomationProperties.SetAutomationId(combo, automationId);
            return combo;
        }

        private void AddButton(Panel panel, string text, string automationId, Action action)
        {
            var button = new Button { Content = text, HorizontalAlignment = HorizontalAlignment.Stretch };
            AutomationProperties.SetAutomationId(button, automationId);
            button.Click += (sender, args) =>
            {
                try
                {
                    action();
                    RefreshStatus();
                }
                catch (Exception ex)
                {
                    Log.Write($"{Title}: {text} failed: 0x{ex.HResult:X8} {ex.Message}");
                }
            };
            panel.Children.Add(button);
        }
    }
}
