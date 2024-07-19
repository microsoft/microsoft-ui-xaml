﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Numerics;
using Windows.UI;
using Microsoft.UI.Composition;
using Windows.UI.Core;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Hosting;
using Microsoft.UI.Xaml.Navigation;
using Microsoft.UI.Private.Controls;
using Microsoft.UI.Dispatching;

namespace MUXControlsTestApp
{
    public sealed partial class ScrollPresenterExpressionAnimationSourcesPage : TestPage
    {
        private bool isLoggingMessages = false;
        private int layoutCompletedCount = 0;
        private List<string> fullLogs = new List<string>();
        private Visual hScrollBarVisual;
        private Visual hScrollIndicatorVisual;
        private SpriteVisual hScrollIndicatorSpriteVisual;
        private ExpressionAnimation hIndicatorWidthExpression;
        private ExpressionAnimation hIndicatorOffsetExpression;

        private Visual vScrollBarVisual;
        private Visual vScrollIndicatorVisual;
        private SpriteVisual vScrollIndicatorSpriteVisual;
        private ExpressionAnimation vIndicatorHeightExpression;
        private ExpressionAnimation vIndicatorOffsetExpression;

        public ScrollPresenterExpressionAnimationSourcesPage()
        {
            InitializeComponent();
            Loaded += ScrollPresenterExpressionAnimationSourcesPage_Loaded;
            scrollPresenter.SizeChanged += ScrollPresenter_SizeChanged;

            ChkLogScrollPresenterMessages_Checked(null, null);
        }

        ~ScrollPresenterExpressionAnimationSourcesPage()
        {
        }

        protected override void OnNavigatedFrom(NavigationEventArgs e)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("ScrollPresenter", isLoggingInfoLevel: false, isLoggingVerboseLevel: false);
            MUXControlsTestHooks.LoggingMessage -= MUXControlsTestHooks_LoggingMessage;

            base.OnNavigatedFrom(e);
        }

        private void SetLayoutCompleted()
        {
            this.fullLogs.Add("SetLayoutCompleted layoutCompletedCount=" + layoutCompletedCount);
            layoutCompletedCount--;

            if (layoutCompletedCount == 0)
            {
                txtLayoutCompleted.Text = "Yes";
            }
        }

        private void ScrollPresenterExpressionAnimationSourcesPage_Loaded(object sender, RoutedEventArgs e)
        {
            this.fullLogs.Add("ScrollPresenterExpressionAnimationSourcesPage_Loaded");
            var ignored = DispatcherQueue.TryEnqueue(Microsoft.UI.Dispatching.DispatcherQueuePriority.Normal, SetupScrollbars);
        }

        private void ScrollPresenter_SizeChanged(object sender, SizeChangedEventArgs e)
        {       
            this.fullLogs.Add("ScrollPresenter_SizeChanged PreviousSize=" + e.PreviousSize + ", NewSize=" + e.NewSize);
            var ignored = DispatcherQueue.TryEnqueue(Microsoft.UI.Dispatching.DispatcherQueuePriority.Normal, UpdateScrollbars);

            // One additional SetLayoutCompleted call is required to declare the layout final
            txtLayoutCompleted.Text = "No";
            layoutCompletedCount++;
            ignored = this.DispatcherQueue.TryEnqueue(DispatcherQueuePriority.Low, SetLayoutCompleted);
        }

        private void SetupScrollbars()
        {
            this.fullLogs.Add("SetupScrollbars");

            hScrollIndicatorVisual = ElementCompositionPreview.GetElementVisual(hScrollIndicator);
            vScrollIndicatorVisual = ElementCompositionPreview.GetElementVisual(vScrollIndicator);
            hScrollBarVisual = ElementCompositionPreview.GetElementVisual(hScrollBar);
            vScrollBarVisual = ElementCompositionPreview.GetElementVisual(vScrollBar);

            hScrollIndicatorSpriteVisual = hScrollIndicatorVisual.Compositor.CreateSpriteVisual();
            vScrollIndicatorSpriteVisual = vScrollIndicatorVisual.Compositor.CreateSpriteVisual();

            CompositionColorBrush brush = hScrollIndicatorVisual.Compositor.CreateColorBrush(Microsoft.UI.Colors.Red);
            hScrollIndicatorSpriteVisual.Brush = brush;
            vScrollIndicatorSpriteVisual.Brush = brush;

            ElementCompositionPreview.SetElementChildVisual(hScrollIndicator, hScrollIndicatorSpriteVisual);
            ElementCompositionPreview.SetElementChildVisual(vScrollIndicator, vScrollIndicatorSpriteVisual);

            UpdateScrollbars();

            hIndicatorWidthExpression = hScrollIndicatorVisual.Compositor.CreateExpressionAnimation(
                "Vector2(Max(20.0f, Min(hScrollBarVisual.Size.X , seas.Viewport.X * hScrollBarVisual.Size.X / Max(0.0001f, seas.Extent.X * seas.ZoomFactor))), 30.0f)");
            vIndicatorHeightExpression = vScrollIndicatorVisual.Compositor.CreateExpressionAnimation(
                "Vector2(30.0f, Max(20.0f, Min(vScrollBarVisual.Size.Y, seas.Viewport.Y * vScrollBarVisual.Size.Y / Max(0.0001f, seas.Extent.Y * seas.ZoomFactor))))");
            hIndicatorWidthExpression.SetReferenceParameter("seas", scrollPresenter.ExpressionAnimationSources);
            vIndicatorHeightExpression.SetReferenceParameter("seas", scrollPresenter.ExpressionAnimationSources);
            hIndicatorWidthExpression.SetReferenceParameter("hScrollBarVisual", hScrollBarVisual);
            vIndicatorHeightExpression.SetReferenceParameter("vScrollBarVisual", vScrollBarVisual);
            hIndicatorOffsetExpression = hScrollIndicatorVisual.Compositor.CreateExpressionAnimation(
                "Vector3(" +
                    "Max(0.0f, Min(hScrollBarVisual.Size.X - hScrollIndicatorSpriteVisual.Size.X, " +
                        "seas.Offset.X * (hScrollBarVisual.Size.X - hScrollIndicatorSpriteVisual.Size.X) / Max(1.0f, (seas.Extent.X * seas.ZoomFactor - seas.Viewport.X))))," +
                    "0.0f," +
                    "0.0f)");
            vIndicatorOffsetExpression = vScrollIndicatorVisual.Compositor.CreateExpressionAnimation(
                "Vector3(" +
                    "0.0f," +
                    "Max(0.0f, Min(vScrollBarVisual.Size.Y - vScrollIndicatorSpriteVisual.Size.Y, " +
                        "seas.Offset.Y * (vScrollBarVisual.Size.Y - vScrollIndicatorSpriteVisual.Size.Y) / Max(1.0f, (seas.Extent.Y * seas.ZoomFactor - seas.Viewport.Y))))," +
                    "0.0f)");
            hIndicatorOffsetExpression.SetReferenceParameter("seas", scrollPresenter.ExpressionAnimationSources);
            vIndicatorOffsetExpression.SetReferenceParameter("seas", scrollPresenter.ExpressionAnimationSources);
            hIndicatorOffsetExpression.SetReferenceParameter("hScrollBarVisual", hScrollBarVisual);
            vIndicatorOffsetExpression.SetReferenceParameter("vScrollBarVisual", vScrollBarVisual);
            hIndicatorOffsetExpression.SetReferenceParameter("hScrollIndicatorSpriteVisual", hScrollIndicatorSpriteVisual);
            vIndicatorOffsetExpression.SetReferenceParameter("vScrollIndicatorSpriteVisual", vScrollIndicatorSpriteVisual);

            hScrollIndicatorSpriteVisual.StartAnimation("Size", hIndicatorWidthExpression);
            vScrollIndicatorSpriteVisual.StartAnimation("Size", vIndicatorHeightExpression);
            hScrollIndicatorVisual.StartAnimation("Offset", hIndicatorOffsetExpression);
            vScrollIndicatorVisual.StartAnimation("Offset", vIndicatorOffsetExpression);
        }

        private void UpdateScrollbars()
        {
            this.fullLogs.Add("UpdateScrollbars");

            if (hScrollIndicatorSpriteVisual == null || hScrollIndicatorVisual == null || hScrollBarVisual == null ||
                vScrollIndicatorSpriteVisual == null || vScrollIndicatorVisual == null || vScrollBarVisual == null)
            {
                return;
            }

            Vector2 viewport;
            CompositionGetValueStatus cgvs = scrollPresenter.ExpressionAnimationSources.Properties.TryGetVector2("Viewport", out viewport);
            txtViewport.Text = cgvs.ToString() + ", " + viewport.ToString();

            Vector2 extent;
            cgvs = scrollPresenter.ExpressionAnimationSources.Properties.TryGetVector2("Extent", out extent);
            txtExtent.Text = cgvs.ToString() + ", " + extent.ToString();

            txtBarVisualWidth.Text = hScrollBarVisual.Size.X.ToString();
            txtIndicatorWidth.Text = (viewport.X * hScrollBarVisual.Size.X / Math.Max(1.0f, extent.X)).ToString();
            txtBarVisualHeight.Text = vScrollBarVisual.Size.Y.ToString();
            txtIndicatorHeight.Text = (viewport.Y * vScrollBarVisual.Size.Y / Math.Max(1.0f, extent.Y)).ToString();
        }

        private void ChkLogScrollPresenterMessages_Checked(object sender, RoutedEventArgs e)
        {
            if (!isLoggingMessages)
            {
                //Turn on info and verbose logging for the ScrollPresenter type:
                MUXControlsTestHooks.SetLoggingLevelForType("ScrollPresenter", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);

                MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;

                isLoggingMessages = true;
            }
        }

        private void ChkLogScrollPresenterMessages_Unchecked(object sender, RoutedEventArgs e)
        {
            if (isLoggingMessages)
            {
                //Turn off info and verbose logging for the ScrollPresenter type:
                MUXControlsTestHooks.SetLoggingLevelForType("ScrollPresenter", isLoggingInfoLevel: false, isLoggingVerboseLevel: false);

                MUXControlsTestHooks.LoggingMessage -= MUXControlsTestHooks_LoggingMessage;

                isLoggingMessages = false;
            }
        }

        private void MUXControlsTestHooks_LoggingMessage(object sender, MUXControlsTestHooksLoggingMessageEventArgs args)
        {
            // Cut off the terminating new line.
            string msg = args.Message.Substring(0, args.Message.Length - 1);
            string senderName = string.Empty;

            try
            {
                FrameworkElement fe = sender as FrameworkElement;

                if (fe != null)
                {
                    senderName = "s:" + fe.Name + ", ";
                }
            }
            catch
            {
            }

            if (args.IsVerboseLevel)
            {
                this.fullLogs.Add("Verbose: " + senderName + "m:" + msg);
            }
            else
            {
                this.fullLogs.Add("Info: " + senderName + "m:" + msg);
            }
        }

        private void btnGetFullLog_Click(object sender, RoutedEventArgs e)
        {
            foreach (string log in this.fullLogs)
            {
                this.cmbFullLog.Items.Add(log);
            }
        }

        private void btnClearFullLog_Click(object sender, RoutedEventArgs e)
        {
            this.fullLogs.Clear();
            this.cmbFullLog.Items.Clear();
        }
    }
}
