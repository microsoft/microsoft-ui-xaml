// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;

namespace RecyclePoolSampleApp.Common
{
    /// <summary>
    /// Scroll helpers for the screenshot scenarios.
    /// </summary>
    /// <remarks>
    /// A scenario is applied as soon as the page is navigated to, which is before the
    /// <see cref="ScrollViewer"/> has been loaded and measured. ChangeView is a no-op at that
    /// point, and an ItemsRepeater's extent only grows as items are realized, so a single call
    /// cannot reach a deep offset either. This retries until the requested offset is reached or
    /// the extent stops growing.
    /// </remarks>
    public static class ScrollHelper
    {
        public static void ScrollToWhenReady(ScrollViewer scroller, double verticalOffset, Action completed = null)
        {
            if (scroller.IsLoaded)
            {
                Step(scroller, verticalOffset, 0, completed);
            }
            else
            {
                void OnLoaded(object s, RoutedEventArgs e)
                {
                    scroller.Loaded -= OnLoaded;
                    Step(scroller, verticalOffset, 0, completed);
                }

                scroller.Loaded += OnLoaded;
            }
        }

        private static void Step(ScrollViewer scroller, double target, int attempt, Action completed)
        {
            scroller.UpdateLayout();
            scroller.ChangeView(null, target, null, true);

            const int MaxAttempts = 12;
            if (attempt >= MaxAttempts || Math.Abs(scroller.VerticalOffset - target) < 1.0)
            {
                completed?.Invoke();
                return;
            }

            scroller.DispatcherQueue.TryEnqueue(
                Microsoft.UI.Dispatching.DispatcherQueuePriority.Low,
                () => Step(scroller, target, attempt + 1, completed));
        }
    }
}
