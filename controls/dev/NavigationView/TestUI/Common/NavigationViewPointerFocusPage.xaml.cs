// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Microsoft.UI.Dispatching;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;

namespace MUXControlsTestApp
{
    public sealed partial class NavigationViewPointerFocusPage : TestPage
    {
        private int m_pointerPressedCount;
        private bool m_outerReleaseRunning;
        private bool m_innerReleaseHandled;

        public NavigationViewPointerFocusPage()
        {
            InitializeComponent();
            ToggleState.Text = FocusToggle.IsOn.ToString();
            FocusToggle.Toggled += delegate { ToggleState.Text = FocusToggle.IsOn.ToString(); };
            TopMode.Checked += delegate { NavView.PaneDisplayMode = NavigationViewPaneDisplayMode.Top; };
            TopMode.Unchecked += delegate { NavView.PaneDisplayMode = NavigationViewPaneDisplayMode.Left; };
            // An ordinary subscription must keep receiving navigation item presses.
            NavView.PointerPressed += delegate { PointerPressedCount.Text = (++m_pointerPressedCount).ToString(); };
            CaptureTarget.PointerPressed += (_, args) =>
                CaptureSucceeded.Text = CaptureTarget.CapturePointer(args.Pointer).ToString();
            CaptureTarget.PointerReleased += (_, args) => CaptureTarget.ReleasePointerCapture(args.Pointer);
            NestedReleaseMode.Checked += delegate
            {
                NavView.Visibility = NormalContent.Visibility = Visibility.Collapsed;
                NestedContent.Visibility = Visibility.Visible;
            };
            OuterRelease.PointerReleased += delegate
            {
                m_outerReleaseRunning = true;
                m_innerReleaseHandled = false;
                NestedStatus.Text = "Waiting";
                bool timedOut = false;
                var timer = DispatcherQueue.CreateTimer();
                timer.Interval = TimeSpan.FromSeconds(10);
                timer.IsRepeating = false;
                timer.Tick += delegate
                {
                    timedOut = true;
                    DispatcherQueue.EnqueueEventLoopExit();
                };
                timer.Start();
                try
                {
                    DispatcherQueue.RunEventLoop(DispatcherRunOptions.QuitOnlyLocalLoop, null);
                }
                finally
                {
                    timer.Stop();
                    m_outerReleaseRunning = false;
                }
                NestedStatus.Text = !timedOut && m_innerReleaseHandled ? "Completed" : "Timed out";
            };
            InnerRelease.PointerReleased += (_, args) =>
            {
                args.Handled = true;
                if (m_outerReleaseRunning)
                {
                    m_innerReleaseHandled = true;
                    DispatcherQueue.EnqueueEventLoopExit();
                }
            };
        }
    }
}
