// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.VisualStudio.TestTools.UnitTesting;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.System.Threading;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;
using Windows.UI.Xaml.Tests.MUXControls.ApiTests;
using Common;

namespace MUXControlsTestApp
{
    public sealed partial class ApiTestPage : TestPage
    {
        private TestFrame testFrame = null;

        public ApiTestPage()
        {
            this.InitializeComponent();
            Loaded += OnLoaded;
        }

        private void OnLoaded(object sender, RoutedEventArgs e)
        {
            testFrame = Frame as TestFrame;
            Log.LogMessageAction = (message) => testFrame.ReportApiTestLog(message);
        }

        protected override void OnNavigatingFrom(NavigatingCancelEventArgs e)
        {
            base.OnNavigatingFrom(e);
            Log.LogMessageAction = null;
        }

        public async void ColorPickerTests_ValidateHueRange_Click(object sender, object args)
        {
            await testFrame.RunApiTest(() =>
            {
                ColorPickerTests tests = new ColorPickerTests();
                tests.ValidateHueRange();
            });
        }
    }
}
