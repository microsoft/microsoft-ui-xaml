// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Windows.UI.Core;
using Windows.UI.Input.Inking;
using Microsoft.UI.Xaml.Controls;

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name = "InkToolbar")]
    public sealed partial class InkToolbarPage : Page
    {
        private InkPresenter _inkPresenter;

        public InkToolbarPage()
        {
            this.InitializeComponent();
            _inkPresenter = inkCanvas.InkPresenter;
            _inkPresenter.InputDeviceTypes =
            CoreInputDeviceTypes.Mouse | CoreInputDeviceTypes.Pen | CoreInputDeviceTypes.Touch;

            // Surface the toolbar events so they can be verified by manual gallery testing.
            InkToolBar.ActiveToolChanged += (s, e) =>
            {
                var tool = InkToolBar.ActiveTool?.GetType().Name ?? "(none)";
                StatusText.Text = "Active tool: " + tool;
            };
            InkToolBar.EraseAllClicked += (s, e) =>
            {
                StatusText.Text = "EraseAllClicked";
            };
            InkToolBar.EraserFlyoutItemClicked += (s, e) =>
            {
                // This event's TypedEventHandler shape is <EventArgs, Object>, so the
                // strongly-typed args arrive as the sender.
                StatusText.Text = "EraserFlyoutItemClicked: " + s.EraserFlyoutItemKind;
            };
            InkToolBar.IsStencilButtonCheckedChanged += (s, e) =>
            {
                StatusText.Text = "Stencil checked changed";
            };
        }
    }
}
