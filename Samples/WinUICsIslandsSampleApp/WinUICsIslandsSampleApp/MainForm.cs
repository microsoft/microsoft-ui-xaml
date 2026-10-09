// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

using System;
using System.Diagnostics;
using System.Runtime.InteropServices;
using Microsoft.UI;
using Microsoft.UI.Dispatching;
using Microsoft.UI.Xaml.Hosting;
using Forms = System.Windows.Forms;

namespace WinUICsIslandsSampleApp;

internal sealed class MainForm : Forms.Form
{
    private enum WinUIState
    {
        Running,
        ShuttingDown,
        ShutdownComplete,
    }

    private readonly Forms.Button _startButton;
    private readonly Forms.Button _shutdownButton;
    private readonly Forms.Button _restartButton;
    private readonly Forms.Label _statusLabel;
    private readonly Forms.Label _objectCountsLabel;
    private readonly Forms.Panel _xamlHostPanel;
    private readonly Forms.TextBox _eventLog;
    private readonly Forms.SplitContainer _contentSplitContainer;

    private DispatcherQueueController? _dispatcherQueueController;
    private DesktopWindowXamlSource? _xamlSource;
    private App? _xamlApplication;
    private nint _islandWindow;
    private int _generation;
    private WinUIState _state = WinUIState.ShutdownComplete;
    private bool _processEventHandlersRegistered;

    public MainForm()
    {
        Text = "WinUI C# Islands shutdown and restart sample";
        Width = 1000;
        Height = 700;
        MinimumSize = new System.Drawing.Size(700, 500);
        StartPosition = Forms.FormStartPosition.CenterScreen;

        _startButton = new Forms.Button
        {
            AutoSize = true,
            Text = "Start WinUI",
        };
        _startButton.Click += (_, _) => StartWinUI();

        _shutdownButton = new Forms.Button
        {
            AutoSize = true,
            Text = "Shut down WinUI",
        };
        _shutdownButton.Click += (_, _) => ShutdownWinUI();

        _restartButton = new Forms.Button
        {
            AutoSize = true,
            Text = "Restart WinUI",
        };
        _restartButton.Click += (_, _) =>
        {
            if (ShutdownWinUI())
            {
                StartWinUI();
            }
        };

        _statusLabel = new Forms.Label
        {
            AutoSize = true,
            Padding = new Forms.Padding(12, 7, 0, 0),
        };

        _objectCountsLabel = new Forms.Label
        {
            AutoSize = true,
            Padding = new Forms.Padding(4, 4, 0, 0),
        };

        Forms.FlowLayoutPanel commandPanel = new()
        {
            AutoSize = true,
            Dock = Forms.DockStyle.Top,
            FlowDirection = Forms.FlowDirection.LeftToRight,
            Padding = new Forms.Padding(8),
        };
        commandPanel.Controls.AddRange(
        [
            _startButton,
            _shutdownButton,
            _restartButton,
            _statusLabel,
            _objectCountsLabel,
        ]);
        commandPanel.SetFlowBreak(_statusLabel, true);

        _xamlHostPanel = new Forms.Panel
        {
            BackColor = System.Drawing.Color.DimGray,
            Dock = Forms.DockStyle.Fill,
            Margin = new Forms.Padding(0),
            TabStop = true,
        };
        _xamlHostPanel.Resize += (_, _) => ResizeIsland();

        _eventLog = new Forms.TextBox
        {
            Dock = Forms.DockStyle.Fill,
            Font = new System.Drawing.Font("Consolas", 9),
            Multiline = true,
            ReadOnly = true,
            ScrollBars = Forms.ScrollBars.Vertical,
        };

        _contentSplitContainer = new Forms.SplitContainer
        {
            Dock = Forms.DockStyle.Fill,
            FixedPanel = Forms.FixedPanel.Panel2,
            SplitterWidth = 6,
        };
        _contentSplitContainer.Panel1.Controls.Add(_xamlHostPanel);
        _contentSplitContainer.Panel2.Controls.Add(_eventLog);

        Controls.Add(_contentSplitContainer);
        Controls.Add(commandPanel);

        UpdateState(WinUIState.ShutdownComplete);
    }

    protected override void OnLoad(EventArgs args)
    {
        base.OnLoad(args);

        WindowsXamlManager.XamlShutdownStartingForProcess += OnXamlShutdownStartingForProcess;
        WindowsXamlManager.XamlShutdownCompletedForProcess += OnXamlShutdownCompletedForProcess;
        _processEventHandlersRegistered = true;

        AppendLog("Native WinForms host started. WinUI is not initialized.");
    }

    protected override void OnShown(EventArgs args)
    {
        base.OnShown(args);
        SetInitialEventLogWidth();
    }

    protected override void OnFormClosing(Forms.FormClosingEventArgs args)
    {
        if (_dispatcherQueueController is not null && !ShutdownWinUI())
        {
            args.Cancel = true;
        }

        base.OnFormClosing(args);
    }

    protected override void OnFormClosed(Forms.FormClosedEventArgs args)
    {
        if (_processEventHandlersRegistered)
        {
            WindowsXamlManager.XamlShutdownStartingForProcess -= OnXamlShutdownStartingForProcess;
            WindowsXamlManager.XamlShutdownCompletedForProcess -= OnXamlShutdownCompletedForProcess;
        }

        base.OnFormClosed(args);
    }

    private void StartWinUI()
    {
        if (_dispatcherQueueController is not null)
        {
            return;
        }

        if (_state == WinUIState.ShuttingDown)
        {
            ShowError("Wait for XamlShutdownCompletedForProcess before restarting WinUI.");
            return;
        }

        try
        {
            AppendLog("Creating DispatcherQueueController.");
            _dispatcherQueueController = DispatcherQueueController.CreateOnCurrentThread();
            _dispatcherQueueController.DispatcherQueue.ShutdownStarting +=
                (_, _) => AppendLog("DispatcherQueue.ShutdownStarting");
            _dispatcherQueueController.DispatcherQueue.ShutdownCompleted +=
                (_, _) => AppendLog("DispatcherQueue.ShutdownCompleted");

            AppendLog("Creating managed WinUI Application and WindowsXamlManager.");
            _xamlApplication = new App();
            _xamlApplication.InitialWindowsXamlManager!.XamlShutdownCompletedOnThread +=
                (_, _) => AppendLog("XamlShutdownCompletedOnThread");

            _xamlHostPanel.CreateControl();
            _xamlSource = new DesktopWindowXamlSource();
            _xamlSource.Initialize(Win32Interop.GetWindowIdFromWindow(_xamlHostPanel.Handle));
            _islandWindow = Win32Interop.GetWindowFromWindowId(_xamlSource.SiteBridge.WindowId);
            AppendLog("DesktopWindowXamlSource initialized.");

            _generation++;
            WinUIContent content = new();
            content.SetGeneration(_generation);
            _xamlSource.Content = content;
            ResizeIsland();

            UpdateState(WinUIState.Running);
            AppendLog($"WinUI generation {_generation} started.");
        }
        catch (Exception exception)
        {
            AppendLog($"WinUI startup failed: {exception}");
            CleanupAfterFailedStartup();
            ShowError($"Unable to start WinUI: 0x{exception.HResult:X8}\n{exception.Message}");
        }
    }

    private bool ShutdownWinUI()
    {
        if (_dispatcherQueueController is null)
        {
            return true;
        }

        try
        {
            AppendLog("Releasing hosted XAML content.");
            if (_xamlSource is not null)
            {
                _xamlSource.Content = null;
                _xamlSource.Dispose();
                _xamlSource = null;
                _islandWindow = 0;
            }

            _xamlApplication?.ReleaseWindowsXamlManager();
            _xamlApplication = null;

            UpdateState(WinUIState.ShuttingDown);
            AppendLog("Shutting down DispatcherQueue.");
            _dispatcherQueueController.ShutdownQueue();
            _dispatcherQueueController = null;
            CollectManagedXamlObjects();
            UpdateObjectCounts();
            return true;
        }
        catch (Exception exception)
        {
            AppendLog($"WinUI shutdown failed: {exception}");
            ShowError($"Unable to shut down WinUI: 0x{exception.HResult:X8}\n{exception.Message}");
            return false;
        }
    }

    private void CleanupAfterFailedStartup()
    {
        if (_xamlSource is not null)
        {
            _xamlSource.Content = null;
            _xamlSource.Dispose();
            _xamlSource = null;
            _islandWindow = 0;
        }

        _xamlApplication?.ReleaseWindowsXamlManager();
        _xamlApplication = null;

        if (_dispatcherQueueController is not null)
        {
            _dispatcherQueueController.ShutdownQueue();
            _dispatcherQueueController = null;
        }

        CollectManagedXamlObjects();
        UpdateState(WinUIState.ShutdownComplete);
    }

    private void OnXamlShutdownStartingForProcess(object? sender, object? args)
    {
        UpdateState(WinUIState.ShuttingDown);
        AppendLog(
            $"XamlShutdownStartingForProcess (sender null: {sender is null}, args null: {args is null})");
    }

    private void OnXamlShutdownCompletedForProcess(object? sender, object? args)
    {
        UpdateState(WinUIState.ShutdownComplete);
        AppendLog(
            $"XamlShutdownCompletedForProcess (sender null: {sender is null}, args null: {args is null})");
    }

    private void UpdateState(WinUIState state)
    {
        _state = state;
        _statusLabel.Text = $"WinUI: {GetStateText(state)} | Generation: {_generation}";
        _startButton.Enabled = state == WinUIState.ShutdownComplete;
        _shutdownButton.Enabled = state == WinUIState.Running;
        _restartButton.Enabled = state == WinUIState.Running;
        UpdateObjectCounts();
    }

    private void UpdateObjectCounts()
    {
        int dispatcherQueueCount = _dispatcherQueueController is null ? 0 : 1;
        int applicationCount = _xamlApplication is null ? 0 : 1;
        int windowsXamlManagerCount =
            _xamlApplication?.InitialWindowsXamlManager is null ? 0 : 1;
        int desktopWindowXamlSourceCount = _xamlSource is null ? 0 : 1;

        _objectCountsLabel.Text =
            $"Object counts | DispatcherQueue: {dispatcherQueueCount} | " +
            $"Application: {applicationCount} | " +
            $"WindowsXamlManager: {windowsXamlManagerCount} | " +
            $"DesktopWindowXamlSource: {desktopWindowXamlSourceCount}";
    }

    private static string GetStateText(WinUIState state)
    {
        return state switch
        {
            WinUIState.Running => "Running",
            WinUIState.ShuttingDown => "Shutting down",
            WinUIState.ShutdownComplete => "Shutdown complete",
            _ => throw new ArgumentOutOfRangeException(nameof(state)),
        };
    }

    private void AppendLog(string message)
    {
        string entry = $"{DateTime.Now:HH:mm:ss.fff} {message}";
        Debug.WriteLine(entry);
        _eventLog.AppendText(entry + Environment.NewLine);
    }

    private void CollectManagedXamlObjects()
    {
        AppendLog("Collecting managed XAML wrappers.");
        GC.Collect();
        GC.WaitForPendingFinalizers();
        GC.Collect();
    }

    private void SetInitialEventLogWidth()
    {
        const int xamlPanelMinimumWidth = 350;
        const int eventLogMinimumWidth = 300;
        const int eventLogWidth = 360;
        int availableWidth =
            _contentSplitContainer.ClientSize.Width - _contentSplitContainer.SplitterWidth;
        if (availableWidth < xamlPanelMinimumWidth + eventLogMinimumWidth)
        {
            return;
        }

        int splitterDistance =
            availableWidth - eventLogWidth;

        _contentSplitContainer.SplitterDistance =
            Math.Clamp(
                splitterDistance,
                xamlPanelMinimumWidth,
                availableWidth - eventLogMinimumWidth);
        _contentSplitContainer.Panel1MinSize = xamlPanelMinimumWidth;
        _contentSplitContainer.Panel2MinSize = eventLogMinimumWidth;
    }

    private void ResizeIsland()
    {
        if (_islandWindow != 0)
        {
            if (!SetWindowPos(
                    _islandWindow,
                    0,
                    0,
                    0,
                    _xamlHostPanel.ClientSize.Width,
                    _xamlHostPanel.ClientSize.Height,
                    SetWindowPosFlags.NoZOrder | SetWindowPosFlags.ShowWindow))
            {
                throw new System.ComponentModel.Win32Exception(Marshal.GetLastWin32Error());
            }
        }
    }

    private void ShowError(string message)
    {
        Forms.MessageBox.Show(
            this,
            message,
            Text,
            Forms.MessageBoxButtons.OK,
            Forms.MessageBoxIcon.Error);
    }

    [Flags]
    private enum SetWindowPosFlags : uint
    {
        NoZOrder = 0x0004,
        ShowWindow = 0x0040,
    }

    [DllImport("user32.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool SetWindowPos(
        nint window,
        nint insertAfter,
        int x,
        int y,
        int width,
        int height,
        SetWindowPosFlags flags);
}
