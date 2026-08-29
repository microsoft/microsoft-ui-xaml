// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml.Controls;

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name = "WebView2", Icon = "WebView2.png")]
    public sealed partial class WebView2Page : TestPage
    {
        public WebView2Page()
        {
            InitializeComponent();

            navigateToBasicWebView2.Click += delegate { Frame.NavigateWithoutAnimation(typeof(WebView2BasicPage), 0); };
            navigateToCoreObjectsWebView2.Click += delegate { Frame.NavigateWithoutAnimation(typeof(WebView2CoreObjectsPage), 0); };
        }
    }
}
