// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using System;
using System.Collections.Generic;
using Windows.UI.Composition.Interactions;
using Windows.UI.Xaml;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

using Scroller = Microsoft.UI.Xaml.Controls.Primitives.Scroller;
using ScrollerTestHooks = Microsoft.UI.Private.Controls.ScrollerTestHooks;
using ScrollerTestHooksAnchorEvaluatedEventArgs = Microsoft.UI.Private.Controls.ScrollerTestHooksAnchorEvaluatedEventArgs;
using ScrollerTestHooksInteractionSourcesChangedEventArgs = Microsoft.UI.Private.Controls.ScrollerTestHooksInteractionSourcesChangedEventArgs;
using ScrollerTestHooksExpressionAnimationStatusChangedEventArgs = Microsoft.UI.Private.Controls.ScrollerTestHooksExpressionAnimationStatusChangedEventArgs;

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
            bool enableExpressionAnimationStatusNotifications = true,
            bool? isAnimationsEnabledOverride = null)
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

                if (isAnimationsEnabledOverride.HasValue)
                {
                    SetIsAnimationsEnabledOverride(isAnimationsEnabledOverride.Value);
                }

                m_interactionSources = new Dictionary<Scroller, CompositionInteractionSourceCollection>();
                m_expressionAnimationStatusChanges = new Dictionary<Scroller, List<ExpressionAnimationStatusChange>>();
            });
        }

        public void TurnOnAnchorNotifications()
        {
            if (!ScrollerTestHooks.AreAnchorNotificationsRaised)
            {
                Log.Comment("ScrollerTestHooksHelper: Turning on anchor notifications.");
                ScrollerTestHooks.AreAnchorNotificationsRaised = true;
                ScrollerTestHooks.AnchorEvaluated += ScrollerTestHooks_AnchorEvaluated;
            }
        }

        public void TurnOffAnchorNotifications()
        {
            if (ScrollerTestHooks.AreAnchorNotificationsRaised)
            {
                Log.Comment("ScrollerTestHooksHelper: Turning off anchor notifications.");
                ScrollerTestHooks.AreAnchorNotificationsRaised = false;
                ScrollerTestHooks.AnchorEvaluated -= ScrollerTestHooks_AnchorEvaluated;
            }
        }

        public void TurnOnInteractionSourcesNotifications()
        {
            if (!ScrollerTestHooks.AreInteractionSourcesNotificationsRaised)
            {
                Log.Comment("ScrollerTestHooksHelper: Turning on InteractionSources notifications.");
                ScrollerTestHooks.AreInteractionSourcesNotificationsRaised = true;
                ScrollerTestHooks.InteractionSourcesChanged += ScrollerTestHooks_InteractionSourcesChanged;
            }
        }

        public void TurnOffInteractionSourcesNotifications()
        {
            if (ScrollerTestHooks.AreInteractionSourcesNotificationsRaised)
            {
                Log.Comment("ScrollerTestHooksHelper: Turning off InteractionSources notifications.");
                ScrollerTestHooks.AreInteractionSourcesNotificationsRaised = false;
                ScrollerTestHooks.InteractionSourcesChanged -= ScrollerTestHooks_InteractionSourcesChanged;
            }
        }

        public void TurnOnExpressionAnimationStatusNotifications()
        {
            if (!ScrollerTestHooks.AreExpressionAnimationStatusNotificationsRaised)
            {
                Log.Comment("ScrollerTestHooksHelper: Turning on ExpressionAnimation status notifications.");
                ScrollerTestHooks.AreExpressionAnimationStatusNotificationsRaised = true;
                ScrollerTestHooks.ExpressionAnimationStatusChanged += ScrollerTestHooks_ExpressionAnimationStatusChanged;
            }
        }

        public void TurnOffExpressionAnimationStatusNotifications()
        {
            if (ScrollerTestHooks.AreExpressionAnimationStatusNotificationsRaised)
            {
                Log.Comment("ScrollerTestHooksHelper: Turning off ExpressionAnimation status notifications.");
                ScrollerTestHooks.AreExpressionAnimationStatusNotificationsRaised = false;
                ScrollerTestHooks.ExpressionAnimationStatusChanged -= ScrollerTestHooks_ExpressionAnimationStatusChanged;
            }
        }

        public void SetIsAnimationsEnabledOverride(bool isAnimationsEnabledOverride)
        {
            if (!ScrollerTestHooks.IsAnimationsEnabledOverride.HasValue || ScrollerTestHooks.IsAnimationsEnabledOverride.Value != isAnimationsEnabledOverride)
            {
                Log.Comment($"ScrollerTestHooksHelper: Setting IsAnimationsEnabledOverride to {isAnimationsEnabledOverride}.");
                ScrollerTestHooks.IsAnimationsEnabledOverride = isAnimationsEnabledOverride;
            }
        }

        public void ResetIsAnimationsEnabledOverride()
        {
            if (ScrollerTestHooks.IsAnimationsEnabledOverride.HasValue)
            {
                Log.Comment($"ScrollerTestHooksHelper: Resetting IsAnimationsEnabledOverride from {ScrollerTestHooks.IsAnimationsEnabledOverride.Value}.");
                ScrollerTestHooks.IsAnimationsEnabledOverride = null;
            }
        }

        public void Dispose()
        {
            RunOnUIThread.Execute(() =>
            {
                TurnOffAnchorNotifications();
                TurnOffInteractionSourcesNotifications();
                TurnOffExpressionAnimationStatusNotifications();
                ResetIsAnimationsEnabledOverride();

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
