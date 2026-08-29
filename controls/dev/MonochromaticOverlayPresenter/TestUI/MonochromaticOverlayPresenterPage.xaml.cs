// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Markup;
using Windows.UI;
using System.Windows.Input;

using MonochromaticOverlayPresenter = Microsoft.UI.Xaml.Controls.Primitives.MonochromaticOverlayPresenter;

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name = "MonochromaticOverlayPresenter")]
    public sealed partial class MonochromaticOverlayPresenterPage : TestPage
    {
        public MonochromaticOverlayPresenterPage()
        {
            this.InitializeComponent();
        }
    }
}
