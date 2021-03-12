// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Windows.UI.Core;
using Windows.UI.Input.Inking;
using Windows.UI.Xaml.Controls;

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
        }
    }
}
