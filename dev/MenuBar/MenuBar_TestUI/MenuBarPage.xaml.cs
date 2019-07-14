// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Windows.Foundation.Metadata;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Input;

#if !BUILD_WINDOWS
using MenuBarItem = Microsoft.UI.Xaml.Controls.MenuBarItem;
#endif

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name = "MenuBar", Icon = "MenuBar.png")]
    public sealed partial class MenuBarPage : TestPage
    {
        public MenuBarPage()
        {
            this.InitializeComponent();

            FileItem.AccessKey = "A";
            NewItem.AccessKey = "B";

            if (ApiInformation.IsTypePresent("Windows.UI.Xaml.Input.StandardUICommand"))
            {
                var cutCommand = new StandardUICommand(StandardUICommandKind.Cut);
                cutCommand.ExecuteRequested += CutCommand_ExecuteRequested;
                CutItem.Command = cutCommand;

            }

            if (ApiInformation.IsTypePresent("Windows.UI.Xaml.Input.KeyboardAccelerator"))
            {
                var accelerator = new KeyboardAccelerator();
                accelerator.Modifiers = Windows.System.VirtualKeyModifiers.Control;
                accelerator.Key = Windows.System.VirtualKey.Z;
                UndoItem.KeyboardAccelerators.Add(accelerator);
            }
            UndoItem.Click += UndoItem_Click;
            
        }

        private void UndoItem_Click(object sender, RoutedEventArgs e)
        {
            TextOutput.Text = "Undo Clicked";
        }

        private void CutCommand_ExecuteRequested(XamlUICommand sender, ExecuteRequestedEventArgs args)
        {
            TextOutput.Text = "Cut Clicked";
        }

        private void AddMenuBarItem_Click(object sender, RoutedEventArgs e)
        {
            var item = new MenuBarItem();
            item.Title = "New Menu Bar Item";
            menuBar.Items.Add(item);
        }

        private void RemoveMenuBarItem_Click(object sender, RoutedEventArgs e)
        {
            var size = menuBar.Items.Count;
            menuBar.Items.RemoveAt(size - 1);
        }

        private void AddFlyoutItem_Click(object sender, RoutedEventArgs e)
        {
            var item = new MenuFlyoutItem();
            item.Text = "New Flyout Item";
            FileItem.Items.Add(item);
        }

        private void RemoveFlyoutItem_Click(object sender, RoutedEventArgs e)
        {
            var size = FileItem.Items.Count;
            FileItem.Items.RemoveAt(size - 1);
        }

        private void NewItem_Click(object sender, RoutedEventArgs e)
        {
            TextOutput.Text = "New Clicked";
        }

        private void TestFrameCheckbox_Checked(object sender, RoutedEventArgs e)
        {
            var testFrame = Window.Current.Content as TestFrame;
            testFrame.ChangeBarVisibility(Visibility.Visible);
        }

        private void TestFrameCheckbox_Unchecked(object sender, RoutedEventArgs e)
        {
            var testFrame = Window.Current.Content as TestFrame;
            testFrame.ChangeBarVisibility(Visibility.Collapsed);
        }
    }
}
