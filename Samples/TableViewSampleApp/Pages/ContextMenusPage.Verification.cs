// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#if TABLEVIEW_SAMPLE_VERIFY

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using Microsoft.UI;
using Microsoft.UI.Windowing;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation.Peers;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Controls.Tabular;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using TableViewSampleApp.Models;
using Windows.Foundation;

namespace TableViewSampleApp.Pages;

// Context menu self-check, run with --verify-context-menus --page=ContextMenus.
//
// Each case raises a real context request and records which of this page's flyouts opened and
// which row item its items inherited. Requests come from three sources:
//   mouse - SendInput right-click at the element's center (the window is pinned always-on-top so
//           another window cannot take the click);
//   key   - SendInput Shift+F10 or the Menu key on the focused row;
//   uia   - AutomationPeer.ShowContextMenu on the cell peer (a request with no position).
// Results go to verification-context-menus.txt next to the executable; the app then exits.
// This moves the real mouse pointer: do not use the machine while it runs.
public sealed partial class ContextMenusPage
{
    private const int OpenTimeoutMs = 1500;

    private readonly List<string> _verificationLines = new();
    private readonly Dictionary<FlyoutBase, string> _menuLabels = new();
    private string? _openedLabel;
    private object? _openedItem;
    private int _passed;
    private int _failed;
    private nint _hwnd;

    private string _logPath = App.GetContextMenuVerificationLogPath();
    private string _verificationTitle = "CONTEXT MENU VERIFICATION";

    partial void AttachVerification()
    {
        if (AttachRowStyleExperiment())
        {
            Loaded += OnVerificationLoaded;
            return;
        }

        if (!App.IsContextMenuVerificationEnabled())
        {
            return;
        }

        Loaded += OnVerificationLoaded;
    }

    private async void OnVerificationLoaded(object sender, RoutedEventArgs e)
    {
        Loaded -= OnVerificationLoaded;
        OverlappedPresenter? presenter = null;
        try
        {
            var windowId = XamlRoot.ContentIslandEnvironment.AppWindowId;
            _hwnd = (nint)Win32Interop.GetWindowFromWindowId(windowId);
            presenter = AppWindow.GetFromWindowId(windowId)?.Presenter as OverlappedPresenter;
            if (presenter is not null)
            {
                presenter.IsAlwaysOnTop = true;
            }

            SetForegroundWindow(_hwnd);
            await Task.Delay(1500);
            if (_rowStyleMode is not null)
            {
                await RunRowStyleExperimentAsync();
            }
            else
            {
                TrackMenus();
                await RunVerificationCasesAsync();
            }
        }
        catch (Exception ex)
        {
            ++_failed;
            _verificationLines.Add($"FAIL harness threw {ex.GetType().Name}: {ex.Message}");
        }
        finally
        {
            if (presenter is not null)
            {
                presenter.IsAlwaysOnTop = false;
            }
        }

        _verificationLines.Insert(0, $"{_verificationTitle} passed={_passed} failed={_failed}");
        File.WriteAllLines(_logPath, _verificationLines, new UTF8Encoding(false));
        Application.Current.Exit();
    }

    private void TrackMenus()
    {
        Track(_nameCellMenu, "NAME CELL");
        Track(_rowMenu, "ROW");
        Track(PeopleTable.ContextFlyout, "TABLE");
        foreach (var (column, menu) in _headerMenus)
        {
            Track(menu, $"HEADER {column.Header}");
        }

        void Track(FlyoutBase? menu, string label)
        {
            if (menu is null)
            {
                _verificationLines.Add($"FAIL setup: no {label} menu");
                ++_failed;
                return;
            }

            _menuLabels[menu] = label;
            menu.Opened += (sender, _) =>
            {
                var flyout = (FlyoutBase)sender!;
                _openedLabel = _menuLabels[flyout];
                _openedItem = (flyout as MenuFlyout)?.Items.FirstOrDefault()?.DataContext;
            };
        }
    }

    private async Task RunVerificationCasesAsync()
    {
        var row0 = People[0];
        var row1 = People[1];
        var row2 = People[2];
        var row3 = People[3];

        // Pointer requests: the cell under the pointer decides.
        await ExpectAsync("C1 mouse Name cell row 0", () => RightClick(Cell(0, NameColumn)), "NAME CELL", row0);
        await ExpectAsync("C2 mouse Name cell row 3", () => RightClick(Cell(3, NameColumn)), "NAME CELL", row3);
        await ExpectAsync("C3 mouse Office cell row 1", () => RightClick(Cell(1, OfficeColumn)), "ROW", row1);
        await ExpectAsync("C4 mouse Department (template) cell row 2", () => RightClick(Cell(2, DepartmentColumn)), "ROW", row2);
        await ExpectAsync("C5 mouse Photo (template) cell row 0", () => RightClick(Cell(0, PhotoColumn)), "ROW", row0);
        await ExpectAsync("C6 mouse Name cell row 1, near left edge", () => RightClick(Cell(1, NameColumn), 0.1), "NAME CELL", row1);
        await ExpectAsync("C7 mouse Name cell row 2, near right edge", () => RightClick(Cell(2, NameColumn), 0.9), "NAME CELL", row2);

        // The current cell must not override the cell under the pointer.
        LeftClick(Cell(1, OfficeColumn));
        await Task.Delay(300);
        await ExpectAsync("C8 mouse Name cell row 1 after left-click Office row 1", () => RightClick(Cell(1, NameColumn)), "NAME CELL", row1);
        LeftClick(Cell(2, NameColumn));
        await Task.Delay(300);
        await ExpectAsync("C9 mouse Office cell row 2 after left-click Name row 2", () => RightClick(Cell(2, OfficeColumn)), "ROW", row2);

        // Keyboard requests use the focused row's current cell.
        LeftClick(Cell(3, NameColumn));
        await Task.Delay(300);
        await ExpectAsync("K1 Shift+F10, current cell Name row 3", () => SendKeys(VkShift, VkF10), "NAME CELL", row3);
        await ExpectAsync("K2 Menu key, current cell Name row 3", () => SendKeys(VkApps), "NAME CELL", row3);
        await ExpectAsync("K2b Shift+F10 again, current cell Name row 3", () => SendKeys(VkShift, VkF10), "NAME CELL", row3);
        LeftClick(Cell(0, NameColumn));
        await Task.Delay(300);
        await ExpectAsync("K2c Menu key first, current cell Name row 0", () => SendKeys(VkApps), "NAME CELL", row0);
        LeftClick(Cell(2, OfficeColumn));
        await Task.Delay(300);
        await ExpectAsync("K3 Shift+F10, current cell Office row 2", () => SendKeys(VkShift, VkF10), "ROW", row2);

        // UIA ShowContextMenu targets one specific cell, with no position.
        await ExpectAsync("U1 uia Name cell row 0 (current cell Office row 2)", () => ShowContextMenu(0, NameColumn), "NAME CELL", row0);
        await ExpectAsync("U2 uia Office cell row 1", () => ShowContextMenu(1, OfficeColumn), "ROW", row1);
        LeftClick(Cell(3, NameColumn));
        await Task.Delay(300);
        await ExpectAsync("U3 uia Name cell row 3 (current cell Name row 3)", () => ShowContextMenu(3, NameColumn), "NAME CELL", row3);

        // Header requests.
        await ExpectAsync("H1 mouse Name header", () => RightClick(Header(NameColumn)), "HEADER Name", null);
        await ExpectAsync("H2 mouse Salary header", () => RightClick(Header(SalaryColumn)), "HEADER Salary", null);
        await ExpectAsync("H3 mouse Photo header (no header menu)", () => RightClick(Header(PhotoColumn)), "TABLE", null);

        // Toggling menus off and on again.
        NameMenuToggle.IsOn = false;
        await Task.Delay(200);
        await ExpectAsync("T1 Name cell menu off: mouse Name cell row 1", () => RightClick(Cell(1, NameColumn)), "ROW", row1);
        NameMenuToggle.IsOn = true;
        await Task.Delay(200);
        await ExpectAsync("T2 Name cell menu back on: mouse Name cell row 1", () => RightClick(Cell(1, NameColumn)), "NAME CELL", row1);
        RowMenuToggle.IsOn = false;
        await Task.Delay(200);
        await ExpectAsync("T3 row menu off: mouse Office cell row 1", () => RightClick(Cell(1, OfficeColumn)), "TABLE", null);
        await ExpectAsync("T4 row menu off: mouse Name cell row 1", () => RightClick(Cell(1, NameColumn)), "NAME CELL", row1);
        RowMenuToggle.IsOn = true;
        await Task.Delay(200);
        HeaderMenuToggle.IsOn = false;
        await Task.Delay(200);
        await ExpectAsync("T5 header menus off: mouse Name header", () => RightClick(Header(NameColumn)), "TABLE", null);
        HeaderMenuToggle.IsOn = true;
        await Task.Delay(200);
        await ExpectAsync("T6 header menus back on: mouse Name header", () => RightClick(Header(NameColumn)), "HEADER Name", null);
    }

    private async Task ExpectAsync(string name, Action request, string expectedMenu, Person? expectedItem)
    {
        _openedLabel = null;
        _openedItem = null;
        int selectedBefore = PeopleTable.SelectedIndex;
        var focused = FocusManager.GetFocusedElement(XamlRoot);
        string focus = focused switch
        {
            TableViewRow focusedRow => $"row {RowIndex(focusedRow)}",
            FrameworkElement { Tag: TableViewColumn column } cell =>
                $"cell {column.Header} row {RowIndex(Ancestors(cell).OfType<TableViewRow>().FirstOrDefault())}",
            FrameworkElement element => $"{element.GetType().Name} '{element.Name}'",
            _ => focused?.GetType().Name ?? "null",
        };
        string observed;
        try
        {
            request();
            for (int waited = 0; _openedLabel is null && waited < OpenTimeoutMs; waited += 50)
            {
                await Task.Delay(50);
            }

            observed = _openedLabel ?? "(no menu)";
        }
        catch (Exception ex)
        {
            observed = $"threw {ex.GetType().Name}: {ex.Message}";
        }

        var openedItem = _openedItem as Person;
        bool itemMatches = expectedItem is null || ReferenceEquals(openedItem, expectedItem);
        bool pass = observed == expectedMenu && itemMatches;
        string item = openedItem?.FullName ?? "(none)";
        string selection = PeopleTable.SelectedIndex == selectedBefore ? "" : $"; selection {selectedBefore}->{PeopleTable.SelectedIndex}";
        _verificationLines.Add(
            $"{(pass ? "PASS" : "FAIL")} {name}: expected {expectedMenu}" +
            (expectedItem is null ? "" : $" [{expectedItem.FullName}]") +
            $", got {observed} [{item}]; focus {focus}{selection}");
        if (pass)
        {
            ++_passed;
        }
        else
        {
            ++_failed;
        }

        File.WriteAllLines(_logPath, _verificationLines, new UTF8Encoding(false));
        await CloseOpenMenusAsync();
    }

    private async Task CloseOpenMenusAsync()
    {
        foreach (var popup in VisualTreeHelper.GetOpenPopupsForXamlRoot(XamlRoot))
        {
            if (popup.Child is MenuFlyoutPresenter)
            {
                popup.IsOpen = false;
            }
        }

        foreach (var menu in _menuLabels.Keys)
        {
            menu.Hide();
        }

        await Task.Delay(250);
    }

    // ---- Element lookup ---------------------------------------------------------------------

    private TableViewRow Row(int index)
    {
        var repeater = Descendants(PeopleTable).OfType<ItemsRepeater>()
            .First(candidate => candidate.Name == "PART_RowsRepeater");
        return repeater.TryGetElement(index) as TableViewRow
            ?? throw new InvalidOperationException($"row {index} is not realized");
    }

    private FrameworkElement Cell(int rowIndex, TableViewColumn column) =>
        Descendants(Row(rowIndex)).OfType<Grid>().FirstOrDefault(grid => ReferenceEquals(grid.Tag, column))
            ?? throw new InvalidOperationException($"no {column.Header} cell in row {rowIndex}");

    private FrameworkElement Header(TableViewColumn column) =>
        Descendants(PeopleTable).OfType<Grid>().FirstOrDefault(grid =>
            ReferenceEquals(grid.Tag, column) &&
            VisualTreeHelper.GetParent(grid) is FrameworkElement { Name: "PART_HeaderHost" })
            ?? throw new InvalidOperationException($"no {column.Header} header");

    private static IEnumerable<DependencyObject> Ancestors(DependencyObject node)
    {
        for (var parent = VisualTreeHelper.GetParent(node); parent is not null; parent = VisualTreeHelper.GetParent(parent))
        {
            yield return parent;
        }
    }

    private int RowIndex(TableViewRow? row) => row is null
        ? -1
        : Descendants(PeopleTable).OfType<ItemsRepeater>().First(r => r.Name == "PART_RowsRepeater").GetElementIndex(row);

    private static IEnumerable<DependencyObject> Descendants(DependencyObject root)
    {
        for (int i = 0; i < VisualTreeHelper.GetChildrenCount(root); ++i)
        {
            var child = VisualTreeHelper.GetChild(root, i);
            yield return child;
            foreach (var descendant in Descendants(child))
            {
                yield return descendant;
            }
        }
    }

    // ---- Request sources --------------------------------------------------------------------

    private void ShowContextMenu(int rowIndex, TableViewColumn column)
    {
        var row = Row(rowIndex);
        var cell = Cell(rowIndex, column);
        var rowPeer = FrameworkElementAutomationPeer.FromElement(row) ?? FrameworkElementAutomationPeer.CreatePeerForElement(row);
        rowPeer.GetChildren();
        var cellPeer = FrameworkElementAutomationPeer.FromElement(cell) ?? FrameworkElementAutomationPeer.CreatePeerForElement(cell)
            ?? throw new InvalidOperationException($"no {column.Header} cell peer in row {rowIndex}");
        cellPeer.ShowContextMenu();
    }

    private void RightClick(FrameworkElement element, double xFraction = 0.5) =>
        Click(element, xFraction, MouseEventFRightDown, MouseEventFRightUp);

    private void LeftClick(FrameworkElement element) =>
        Click(element, 0.5, MouseEventFLeftDown, MouseEventFLeftUp);

    private void Click(FrameworkElement element, double xFraction, uint down, uint up)
    {
        var local = new Point(element.ActualWidth * xFraction, element.ActualHeight / 2);
        var client = element.TransformToVisual(null).TransformPoint(local);
        double scale = XamlRoot.RasterizationScale;
        var screen = new NativePoint { X = (int)Math.Round(client.X * scale), Y = (int)Math.Round(client.Y * scale) };
        ClientToScreen(_hwnd, ref screen);
        SetCursorPos(screen.X, screen.Y);
        SendMouse(down);
        SendMouse(up);
    }

    private static void SendKeys(params ushort[] keys)
    {
        var inputs = new List<Input>();
        foreach (var key in keys)
        {
            inputs.Add(KeyInput(key, 0));
        }

        foreach (var key in Enumerable.Reverse(keys))
        {
            inputs.Add(KeyInput(key, KeyEventFKeyUp));
        }

        SendInput((uint)inputs.Count, inputs.ToArray(), Marshal.SizeOf<Input>());
    }

    private static void SendMouse(uint flags)
    {
        var input = new Input { Type = InputMouse, Data = new InputUnion { Mouse = new MouseInput { Flags = flags } } };
        SendInput(1, new[] { input }, Marshal.SizeOf<Input>());
    }

    private static Input KeyInput(ushort key, uint flags) =>
        new() { Type = InputKeyboard, Data = new InputUnion { Keyboard = new KeyboardInput { VirtualKey = key, Flags = flags } } };

    // ---- Win32 ------------------------------------------------------------------------------

    private const uint InputMouse = 0;
    private const uint InputKeyboard = 1;
    private const uint MouseEventFLeftDown = 0x0002;
    private const uint MouseEventFLeftUp = 0x0004;
    private const uint MouseEventFRightDown = 0x0008;
    private const uint MouseEventFRightUp = 0x0010;
    private const uint KeyEventFKeyUp = 0x0002;
    private const ushort VkShift = 0x10;
    private const ushort VkF10 = 0x79;
    private const ushort VkApps = 0x5D;

    [StructLayout(LayoutKind.Sequential)]
    private struct NativePoint
    {
        public int X;
        public int Y;
    }

    [StructLayout(LayoutKind.Sequential)]
    private struct MouseInput
    {
        public int Dx;
        public int Dy;
        public uint MouseData;
        public uint Flags;
        public uint Time;
        public nint ExtraInfo;
    }

    [StructLayout(LayoutKind.Sequential)]
    private struct KeyboardInput
    {
        public ushort VirtualKey;
        public ushort ScanCode;
        public uint Flags;
        public uint Time;
        public nint ExtraInfo;
    }

    [StructLayout(LayoutKind.Explicit)]
    private struct InputUnion
    {
        [FieldOffset(0)] public MouseInput Mouse;
        [FieldOffset(0)] public KeyboardInput Keyboard;
    }

    [StructLayout(LayoutKind.Sequential)]
    private struct Input
    {
        public uint Type;
        public InputUnion Data;
    }

    [DllImport("user32.dll")]
    private static extern uint SendInput(uint count, Input[] inputs, int size);

    [DllImport("user32.dll")]
    private static extern bool SetCursorPos(int x, int y);

    [DllImport("user32.dll")]
    private static extern bool ClientToScreen(nint hwnd, ref NativePoint point);

    [DllImport("user32.dll")]
    private static extern bool SetForegroundWindow(nint hwnd);
}

#endif
