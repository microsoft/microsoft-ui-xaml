// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using System;
using System.Collections.Generic;
using Windows.UI.Composition.Interactions;
using Windows.UI.Xaml;
using Common;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

#if BUILD_WINDOWS
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
#else
using Scroller = Microsoft.UI.Xaml.Controls.Primitives.Scroller;
using ScrollerTestHooks = Microsoft.UI.Private.Controls.ScrollerTestHooks;
using ScrollerTestHooksAnchorEvaluatedEventArgs = Microsoft.UI.Private.Controls.ScrollerTestHooksAnchorEvaluatedEventArgs;
using ScrollerTestHooksInteractionSourcesChangedEventArgs = Microsoft.UI.Private.Controls.ScrollerTestHooksInteractionSourcesChangedEventArgs;
using ScrollerTestHooksExpressionAnimationStatusChangedEventArgs = Microsoft.UI.Private.Controls.ScrollerTestHooksExpressionAnimationStatusChangedEventArgs;
#endif

namespace MUXControlsTestApp.Utilities
{
    public class ExpressionAnimationStatusChange
    {
        public ExpressionAnimationStatusChange(bool isExpressionAnimationStarted, string propertyName)
        {
            IsExpressionAnimationStarted = isExpressionAnimationStarted;
            PropertyName = propertyName;
        }

        public bool IsExpressionAnimationStarted { get; set; }

        public string PropertyName { get; set; }
    }

    // Utility class used to turn on Scroller test hooks and automatically turn them off when the instance gets disposed.
    public class ScrollerTestHooksHelper : IDisposable
    {
        Dictionary<Scroller, CompositionInteractionSourceCollection> m_interactionSources = null;
        Dictionary<Scroller, List<ExpressionAnimationStatusChange>> m_expressionAnimationStatusChanges = null;

        public ScrollerTestHooksHelper(
            bool enableAnchorNotifications = true,
            bool enableInteractionSourcesNotifications = true,
            bool enableExpressionAnimationStatusNotifications = true)
        {
            RunOnUIThread.Execute(() =>
            {
                if (enableAnchorNotifications)
                {
                    TurnOnAnchorNotifications();
                }

                if (enableInteractionSourcesNotifications)
                {
                    TurnOnInteractionSourcesNotifications();
                }

                if (enableExpressionAnimationStatusNotifications)
                {
                    TurnOnExpressionAnimationStatusNotifications();
                }

                m_interactionSources = new Dictionary<Scroller, CompositionInteractionSourceCollection>();
                m_expressionAnimationStatusChanges = new Dictionary<Scroller, List<ExpressionAnimationStatusChange>>();
            });
        }

        private bool AreAnchorNotificationsRaised
        {
            get;
            set;
        }

        private bool AreInteractionSourcesNotificationsRaised
        {
            get;
            set;
        }

        private bool AreExpressionAnimationStatusNotificationsRaised
        {
            get;
            set;
        }

        public void TurnOnAnchorNotifications()
        {
            if (!AreAnchorNotificationsRaised)
            {
                Log.Comment("ScrollerTestHooksHelper: Turning on anchor notifications.");
                ScrollerTestHooks.AreAnchorNotificationsRaised = AreAnchorNotificationsRaised = true;
                ScrollerTestHooks.AnchorEvaluated += ScrollerTestHooks_AnchorEvaluated;
            }
        }

        public void TurnOffAnchorNotifications()
        {
            if (AreAnchorNotificationsRaised)
            {
                Log.Comment("ScrollerTestHooksHelper: Turning off anchor notifications.");
                ScrollerTestHooks.AreAnchorNotificationsRaised = AreAnchorNotificationsRaised = false;
                ScrollerTestHooks.AnchorEvaluated -= ScrollerTestHooks_AnchorEvaluated;
            }
        }

        public void TurnOnInteractionSourcesNotifications()
        {
            if (!AreInteractionSourcesNotificationsRaised)
            {
                Log.Comment("ScrollerTestHooksHelper: Turning on InteractionSources notifications.");
                ScrollerTestHooks.AreInteractionSourcesNotificationsRaised = AreInteractionSourcesNotificationsRaised = true;
                ScrollerTestHooks.InteractionSourcesChanged += ScrollerTestHooks_InteractionSourcesChanged;
            }
        }

        public void TurnOffInteractionSourcesNotifications()
        {
            if (AreInteractionSourcesNotificationsRaised)
            {
                Log.Comment("ScrollerTestHooksHelper: Turning off InteractionSources notifications.");
                ScrollerTestHooks.AreInteractionSourcesNotificationsRaised = AreInteractionSourcesNotificationsRaised = false;
                ScrollerTestHooks.InteractionSourcesChanged -= ScrollerTestHooks_InteractionSourcesChanged;
            }
        }

        public void TurnOnExpressionAnimationStatusNotifications()
        {
            if (!AreExpressionAnimationStatusNotificationsRaised)
            {
                Log.Comment("ScrollerTestHooksHelper: Turning on ExpressionAnimation status notifications.");
                ScrollerTestHooks.AreExpressionAnimationStatusNotificationsRaised = AreExpressionAnimationStatusNotificationsRaised = true;
                ScrollerTestHooks.ExpressionAnimationStatusChanged += ScrollerTestHooks_ExpressionAnimationStatusChanged;
            }
        }

        public void TurnOffExpressionAnimationStatusNotifications()
        {
            if (AreExpressionAnimationStatusNotificationsRaised)
            {
                Log.Comment("ScrollerTestHooksHelper: Turning off ExpressionAnimation status notifications.");
                ScrollerTestHooks.AreExpressionAnimationStatusNotificationsRaised = AreExpressionAnimationStatusNotificationsRaised = false;
                ScrollerTestHooks.ExpressionAnimationStatusChanged -= ScrollerTestHooks_ExpressionAnimationStatusChanged;
            }
        }

        public void Dispose()
        {
            RunOnUIThread.Execute(() =>
            {
                TurnOffAnchorNotifications();
                TurnOffExpressionAnimationStatusNotifications();

                m_interactionSources.Clear();
                m_expressionAnimationStatusChanges.Clear();
            });
        }

        public CompositionInteractionSourceCollection GetInteractionSources(Scroller scroller)
        {
            if (m_interactionSources.ContainsKey(scroller))
            {
                return m_interactionSources[scroller];
            }
            else
            {
                return null;
            }
        }

        public List<ExpressionAnimationStatusChange> GetExpressionAnimationStatusChanges(Scroller scroller)
        {
            if (m_expressionAnimationStatusChanges.ContainsKey(scroller))
            {
                return m_expressionAnimationStatusChanges[scroller];
            }
            else
            {
                return null;
            }
        }

        public void ResetExpressionAnimationStatusChanges(Scroller scroller)
        {
            if (m_expressionAnimationStatusChanges.ContainsKey(scroller))
            {
                m_expressionAnimationStatusChanges.Remove(scroller);
            }
        }

        public static void LogInteractionSources(CompositionInteractionSourceCollection interactionSources)
        {
            if (interactionSources == null)
            {
                Log.Warning("LogInteractionSources: parameter interactionSources is null");
                throw new ArgumentNullException("interactionSources");
            }

            Log.Comment("    Interaction sources count: " + interactionSources.Count);

            foreach (ICompositionInteractionSource interactionSource in interactionSources)
            {
                VisualInteractionSource visualInteractionSource = interactionSource as VisualInteractionSource;
                if (visualInteractionSource != null)
                {
                    Log.Comment("    VisualInteractionSource: ManipulationRedirectionMode: " + visualInteractionSource.ManipulationRedirectionMode);
                    Log.Comment("    VisualInteractionSource: IsPositionXRailsEnabled: " + visualInteractionSource.IsPositionXRailsEnabled +
                        ", IsPositionYRailsEnabled:" + visualInteractionSource.IsPositionYRailsEnabled);
                    Log.Comment("    VisualInteractionSource: PositionXChainingMode: " + visualInteractionSource.PositionXChainingMode +
                        ", PositionYChainingMode:" + visualInteractionSource.PositionYChainingMode +
                        ", ScaleChainingMode:" + visualInteractionSource.ScaleChainingMode);
                    Log.Comment("    VisualInteractionSource: PositionXSourceMode: " + visualInteractionSource.PositionXSourceMode +
                        ", PositionYSourceMode:" + visualInteractionSource.PositionYSourceMode +
                        ", ScaleSourceMode:" + visualInteractionSource.ScaleSourceMode);
                    if (PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone5))
                    {
                        Log.Comment("    VisualInteractionSource: PointerWheelConfig: (" + visualInteractionSource.PointerWheelConfig.PositionXSourceMode +
                            ", " + visualInteractionSource.PointerWheelConfig.PositionYSourceMode +
                            ", " + visualInteractionSource.PointerWheelConfig.ScaleSourceMode + ")");
                    }
                }
            }
        }

        public static void LogExpressionAnimationStatusChanges(List<ExpressionAnimationStatusChange> expressionAnimationStatusChanges)
        {
            if (expressionAnimationStatusChanges == null)
            {
                Log.Comment("expressionAnimationStatusChanges is null");
                return;
            }

            Log.Comment("expressionAnimationStatusChanges:");
            foreach (ExpressionAnimationStatusChange expressionAnimationStatusChange in expressionAnimationStatusChanges)
            {
                Log.Comment($"  IsExpressionAnimationStarted: {expressionAnimationStatusChange.IsExpressionAnimationStarted}, PropertyName: {expressionAnimationStatusChange.PropertyName}");
            }
        }

        private void ScrollerTestHooks_AnchorEvaluated(Scroller sender, ScrollerTestHooksAnchorEvaluatedEventArgs args)
        {
            string anchorName = (args.AnchorElement is FrameworkElement) ? (args.AnchorElement as FrameworkElement).Name : string.Empty;

            Log.Comment("  AnchorEvaluated: s:" + sender.Name + ", a:" + anchorName + ", ap:(" + args.ViewportAnchorPointHorizontalOffset + "," + args.ViewportAnchorPointVerticalOffset + ")");
        }

        private void ScrollerTestHooks_InteractionSourcesChanged(Scroller sender, ScrollerTestHooksInteractionSourcesChangedEventArgs args)
        {
            Log.Comment("  InteractionSourcesChanged: s: " + sender.Name);
            if (!m_interactionSources.ContainsKey(sender))
            {
                m_interactionSources.Add(sender, args.InteractionSources);
            }
            else
            {
                m_interactionSources[sender] = args.InteractionSources;
            }
            LogInteractionSources(args.InteractionSources);
        }

        private void ScrollerTestHooks_ExpressionAnimationStatusChanged(Scroller sender, ScrollerTestHooksExpressionAnimationStatusChangedEventArgs args)
        {
            Log.Comment($"  ExpressionAnimationStatusChanged: s: {sender.Name}, IsExpressionAnimationStarted: {args.IsExpressionAnimationStarted}, PropertyName: {args.PropertyName}");
            List<ExpressionAnimationStatusChange> expressionAnimationStatusChanges = null;

            if (!m_expressionAnimationStatusChanges.ContainsKey(sender))
            {
                expressionAnimationStatusChanges = new List<ExpressionAnimationStatusChange>();
                m_expressionAnimationStatusChanges.Add(sender, expressionAnimationStatusChanges);
            }
            else
            {
                expressionAnimationStatusChanges = m_expressionAnimationStatusChanges[sender];
            }

            expressionAnimationStatusChanges.Add(new ExpressionAnimationStatusChange(args.IsExpressionAnimationStarted, args.PropertyName));
        }
    }
}
