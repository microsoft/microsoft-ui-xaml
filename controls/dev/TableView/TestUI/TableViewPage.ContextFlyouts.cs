// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using Microsoft.UI.Dispatching;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Controls.Tabular;

namespace MUXControlsTestApp
{
    public sealed partial class TableViewPage
    {
        private readonly List<TableViewTestPerson> _contextItems = new List<TableViewTestPerson>
        {
            new TableViewTestPerson(0, "Context row 0", 20, "A"),
            new TableViewTestPerson(1, "Context row 1", 21, "B"),
            new TableViewTestPerson(2, "Context row 2", 22, "C"),
        };
        private FlyoutBase _contextTextMenu;
        private FlyoutBase _contextTextHeaderMenu;
        private MenuFlyout _openContextMenu;
        private bool _contextRowsHooked;

        private void InitializeContextFlyouts()
        {
            ContextFlyoutsTableView.ItemsSource = _contextItems;
            _contextTextMenu = ContextTextColumn.CellContextFlyout;
            _contextTextHeaderMenu = ContextTextColumn.HeaderContextFlyout;
            HookContextMenu((MenuFlyout)_contextTextMenu, "Text");
            HookContextMenu((MenuFlyout)_contextTextHeaderMenu, "TextHeader");
            HookContextMenu((MenuFlyout)ContextTemplateColumn.CellContextFlyout, "Template");
            HookContextMenu((MenuFlyout)ContextTemplateColumn.HeaderContextFlyout, "TemplateHeader");
            ContextFlyoutsTableView.ContextFlyout = MakeContextMenu("Table");
            ContextFlyoutsTableView.BeginningEdit += (sender, args) =>
                DispatcherQueue.TryEnqueue(DispatcherQueuePriority.Low, () =>
                {
                    var editor = FindDescendant<TextBox>(ContextFlyoutsTableView);
                    if (editor != null)
                    {
                        AutomationProperties.SetAutomationId(editor, "ContextNativeEditor");
                    }
                });
        }

        private void OnGoToContextFlyoutsClick(object sender, RoutedEventArgs e)
        {
            ShowTableHost(ContextFlyoutsTableHost);
            ContextFlyoutsTableView.UpdateLayout();
            if (_contextRowsHooked)
            {
                return;
            }

            ContextFlyoutsTableView.Select(0);
            var repeater = FindDescendantByName<ItemsRepeater>(ContextFlyoutsTableView, "PART_RowsRepeater");
            for (int i = 0; i < _contextItems.Count; i++)
            {
                var row = (TableViewRow)repeater.TryGetElement(i);
                row.ContextFlyout = MakeContextMenu("Row");
                var child = FindDescendantByName<Border>(row, "ContextChild");
                HookContextMenu((MenuFlyout)child.ContextFlyout, "Child");
            }
            _contextRowsHooked = true;
        }

        private MenuFlyout MakeContextMenu(string kind)
        {
            var menu = new MenuFlyout();
            menu.Items.Add(new MenuFlyoutItem { Text = kind + " action" });
            HookContextMenu(menu, kind);
            return menu;
        }

        private void HookContextMenu(MenuFlyout menu, string kind)
        {
            menu.Opened += (sender, args) =>
            {
                _openContextMenu = menu;
                ContextFlyoutReport.Text += DescribeContextMenu(menu, kind) + ";";
            };
            menu.Closed += (sender, args) =>
            {
                if (ReferenceEquals(_openContextMenu, menu))
                {
                    _openContextMenu = null;
                }
            };
            ((MenuFlyoutItem)menu.Items[0]).Click += (sender, args) =>
                ContextFlyoutActionReport.Text += DescribeContextMenu(menu, kind) + ";";
        }

        // Observe the actual presenter's inherited context, never inject the row into the menu.
        private string DescribeContextMenu(MenuFlyout menu, string kind) =>
            kind + "|" + ((menu.Items[0].DataContext as TableViewTestPerson)?.Name ?? "<none>") +
            "|selected=" + ((ContextFlyoutsTableView.SelectedItem as TableViewTestPerson)?.Name ?? "<none>");

        private void OnClearContextFlyoutsClick(object sender, RoutedEventArgs e)
        {
            ContextTextColumn.CellContextFlyout = null;
            ContextTextColumn.HeaderContextFlyout = null;
        }

        private void OnRestoreContextFlyoutsClick(object sender, RoutedEventArgs e)
        {
            ContextTextColumn.CellContextFlyout = _contextTextMenu;
            ContextTextColumn.HeaderContextFlyout = _contextTextHeaderMenu;
        }

        private void OnRetargetContextRowClick(object sender, RoutedEventArgs e)
        {
            var menu = _openContextMenu;
            if (menu == null)
            {
                ContextFlyoutLifetimeReport.Text = "No open menu";
                return;
            }

            var invokingItem = menu.Items[0].DataContext;
            var repeater = FindDescendantByName<ItemsRepeater>(ContextFlyoutsTableView, "PART_RowsRepeater");
            var row = (TableViewRow)repeater.TryGetElement(1);
            row.DataContext = new TableViewTestPerson(100, "Replacement row", 30, "Replacement");
            DispatcherQueue.TryEnqueue(DispatcherQueuePriority.Low, () =>
            {
                string result = !menu.IsOpen ? "Closed" :
                    ReferenceEquals(invokingItem, menu.Items[0].DataContext) ? "Snapshot" : "Retargeted";
                ContextFlyoutLifetimeReport.Text = result + "|" + ((TableViewTestPerson)row.DataContext).Name;
            });
        }
    }
}
