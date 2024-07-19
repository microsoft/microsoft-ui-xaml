﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Text;
using System.Threading;
using Windows.Foundation;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Hosting;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Private.Controls;
using MUXControlsTestApp.Utilities;
using Common;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name = "ScrollViewerAdapter", Icon = "ScrollViewer.png")]
    public sealed partial class ScrollViewerAdapterPage : TestPage
    {
        Dictionary<string, WeakReference> objects = new Dictionary<string, WeakReference>();
        
        public ScrollViewerAdapterPage()
        {
            this.InitializeComponent();
            this.AdaptOnSVWithoutWaitingForLoadedButton.Click += AdaptOnSVWithoutWaitingForLoaded_Click;
            this.RemoveButton.Click += Remove_Click;
            this.GCButton.Click += GC_Click;
            this.CheckLeaksButton.Click += CheckLeaks_Click;
            LogController.InitializeLogging();
        }

        private void AdaptOnSVWithoutWaitingForLoaded_Click(object sender, RoutedEventArgs e)
        {
            ScrollViewer sv = new ScrollViewer();
            objects["sv"] = new WeakReference(sv);
            sv.Content = new Button();
            ScrollViewerIRefreshInfoProviderAdapter adapter = new ScrollViewerIRefreshInfoProviderAdapter(RefreshPullDirection.TopToBottom, null);
            adapter.Adapt(sv, new Size(1.0, 1.0));
            this.TestRoot.Children.Add(sv);
        }

        private void Remove_Click(object sender, RoutedEventArgs e)
        {
            this.TestRoot.Children.Clear();
        }

        private void GC_Click(object sender, RoutedEventArgs e)
        {
            GC.Collect();
            GC.WaitForPendingFinalizers();
            GC.Collect();
        }

        private void CheckLeaks_Click(object sender, RoutedEventArgs e)
        {
            CheckLeaks(objects);
        }

        void CheckLeaks(Dictionary<string, WeakReference> objects)
        {
            foreach (var pair in objects)
            {
                Log.Comment("Checking if object {0} has been collected", pair.Key);
                object target = pair.Value.Target;
                if (target != null)
                {
                    Verify.DisableVerifyFailureExceptions = true; // throwing exceptions makes running this in TDP harder
                    Verify.Fail(String.Format("Object {0} is still alive when it should not be.", target));
                    Verify.DisableVerifyFailureExceptions = false;
                }
            }
            objects.Clear();
        }
    }
}