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

using ScrollingPresenter = Microsoft.UI.Xaml.Controls.Primitives.ScrollingPresenter;
using ScrollingPresenterTestHooks = Microsoft.UI.Private.Controls.ScrollingPresenterTestHooks;
using ScrollingPresenterTestHooksAnchorEvaluatedEventArgs = Microsoft.UI.Private.Controls.ScrollingPresenterTestHooksAnchorEvaluatedEventArgs;
using ScrollingPresenterTestHooksInteractionSourcesChangedEventArgs = Microsoft.UI.Private.Controls.ScrollingPresenterTestHooksInteractionSourcesChangedEventArgs;
using ScrollingPresenterTestHooksExpressionAnimationStatusChangedEventArgs = Microsoft.UI.Private.Controls.ScrollingPresenterTestHooksExpressionAnimationStatusChangedEventArgs;

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

    // Utility class used to turn on ScrollingPresenter test hooks and automatically turn them off when the instance gets disposed.
    public class ScrollingPresenterTestHooksHelper : IDisposable
    {
        Dictionary<ScrollingPresenter, CompositionInteractionSourceCollection> m_interactionSources = null;
        Dictionary<ScrollingPresenter, List<ExpressionAnimationStatusChange>> m_expressionAnimationStatusChanges = null;

        public ScrollingPresenterTestHooksHelper(
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

                m_interactionSources = new Dictionary<ScrollingPresenter, CompositionInteractionSourceCollection>();
                m_expressionAnimationStatusChanges = new Dictionary<ScrollingPresenter, List<ExpressionAnimationStatusChange>>();
            });
        }

        public void TurnOnAnchorNotifications()
        {
            if (!ScrollingPresenterTestHooks.AreAnchorNotificationsRaised)
            {
                Log.Comment("ScrollingPresenterTestHooksHelper: Turning on anchor notifications.");
                ScrollingPresenterTestHooks.AreAnchorNotificationsRaised = true;
                ScrollingPresenterTestHooks.AnchorEvaluated += ScrollingPresenterTestHooks_AnchorEvaluated;
            }
        }

        public void TurnOffAnchorNotifications()
        {
            if (ScrollingPresenterTestHooks.AreAnchorNotificationsRaised)
            {
                Log.Comment("ScrollingPresenterTestHooksHelper: Turning off anchor notifications.");
                ScrollingPresenterTestHooks.AreAnchorNotificationsRaised = false;
                ScrollingPresenterTestHooks.AnchorEvaluated -= ScrollingPresenterTestHooks_AnchorEvaluated;
            }
        }

        public void TurnOnInteractionSourcesNotifications()
        {
            if (!ScrollingPresenterTestHooks.AreInteractionSourcesNotificationsRaised)
            {
                Log.Comment("ScrollingPresenterTestHooksHelper: Turning on InteractionSources notifications.");
                ScrollingPresenterTestHooks.AreInteractionSourcesNotificationsRaised = true;
                ScrollingPresenterTestHooks.InteractionSourcesChanged += ScrollingPresenterTestHooks_InteractionSourcesChanged;
            }
        }

        public void TurnOffInteractionSourcesNotifications()
        {
            if (ScrollingPresenterTestHooks.AreInteractionSourcesNotificationsRaised)
            {
                Log.Comment("ScrollingPresenterTestHooksHelper: Turning off InteractionSources notifications.");
                ScrollingPresenterTestHooks.AreInteractionSourcesNotificationsRaised = false;
                ScrollingPresenterTestHooks.InteractionSourcesChanged -= ScrollingPresenterTestHooks_InteractionSourcesChanged;
            }
        }

        public void TurnOnExpressionAnimationStatusNotifications()
        {
            if (!ScrollingPresenterTestHooks.AreExpressionAnimationStatusNotificationsRaised)
            {
                Log.Comment("ScrollingPresenterTestHooksHelper: Turning on ExpressionAnimation status notifications.");
                ScrollingPresenterTestHooks.AreExpressionAnimationStatusNotificationsRaised = true;
                ScrollingPresenterTestHooks.ExpressionAnimationStatusChanged += ScrollingPresenterTestHooks_ExpressionAnimationStatusChanged;
            }
        }

        public void TurnOffExpressionAnimationStatusNotifications()
        {
            if (ScrollingPresenterTestHooks.AreExpressionAnimationStatusNotificationsRaised)
            {
                Log.Comment("ScrollingPresenterTestHooksHelper: Turning off ExpressionAnimation status notifications.");
                ScrollingPresenterTestHooks.AreExpressionAnimationStatusNotificationsRaised = false;
                ScrollingPresenterTestHooks.ExpressionAnimationStatusChanged -= ScrollingPresenterTestHooks_ExpressionAnimationStatusChanged;
            }
        }

        public void SetIsAnimationsEnabledOverride(bool isAnimationsEnabledOverride)
        {
            if (!ScrollingPresenterTestHooks.IsAnimationsEnabledOverride.HasValue || ScrollingPresenterTestHooks.IsAnimationsEnabledOverride.Value != isAnimationsEnabledOverride)
            {
                Log.Comment($"ScrollingPresenterTestHooksHelper: Setting IsAnimationsEnabledOverride to {isAnimationsEnabledOverride}.");
                ScrollingPresenterTestHooks.IsAnimationsEnabledOverride = isAnimationsEnabledOverride;
            }
        }

        public void ResetIsAnimationsEnabledOverride()
        {
            if (ScrollingPresenterTestHooks.IsAnimationsEnabledOverride.HasValue)
            {
                Log.Comment($"ScrollingPresenterTestHooksHelper: Resetting IsAnimationsEnabledOverride from {ScrollingPresenterTestHooks.IsAnimationsEnabledOverride.Value}.");
                ScrollingPresenterTestHooks.IsAnimationsEnabledOverride = null;
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

        public CompositionInteractionSourceCollection GetInteractionSources(ScrollingPresenter scrollingPresenter)
        {
            if (m_interactionSources.ContainsKey(scrollingPresenter))
            {
                return m_interactionSources[scrollingPresenter];
            }
            else
            {
                return null;
            }
        }

        public List<ExpressionAnimationStatusChange> GetExpressionAnimationStatusChanges(ScrollingPresenter scrollingPresenter)
        {
            if (m_expressionAnimationStatusChanges.ContainsKey(scrollingPresenter))
            {
                return m_expressionAnimationStatusChanges[scrollingPresenter];
            }
            else
            {
                return null;
            }
        }

        public void ResetExpressionAnimationStatusChanges(ScrollingPresenter scrollingPresenter)
        {
            if (m_expressionAnimationStatusChanges.ContainsKey(scrollingPresenter))
            {
                m_expressionAnimationStatusChanges.Remove(scrollingPresenter);
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

        private void ScrollingPresenterTestHooks_AnchorEvaluated(ScrollingPresenter sender, ScrollingPresenterTestHooksAnchorEvaluatedEventArgs args)
        {
            string anchorName = (args.AnchorElement is FrameworkElement) ? (args.AnchorElement as FrameworkElement).Name : string.Empty;

            Log.Comment("  AnchorEvaluated: s:" + sender.Name + ", a:" + anchorName + ", ap:(" + args.ViewportAnchorPointHorizontalOffset + "," + args.ViewportAnchorPointVerticalOffset + ")");
        }

        private void ScrollingPresenterTestHooks_InteractionSourcesChanged(ScrollingPresenter sender, ScrollingPresenterTestHooksInteractionSourcesChangedEventArgs args)
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

        private void ScrollingPresenterTestHooks_ExpressionAnimationStatusChanged(ScrollingPresenter sender, ScrollingPresenterTestHooksExpressionAnimationStatusChangedEventArgs args)
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
