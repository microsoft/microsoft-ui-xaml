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
#endif

namespace MUXControlsTestApp.Utilities
{
    // Utility class used to turn on Scroller test hooks and automatically turn them off when the instance gets disposed.
    public class ScrollerTestHooksHelper : IDisposable
    {
        Dictionary<Scroller, CompositionInteractionSourceCollection> m_interactionSources = null;

        public ScrollerTestHooksHelper(bool enableAnchorNotifications = true, bool enableInteractionSourcesNotifications = true)
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

                m_interactionSources = new Dictionary<Scroller, CompositionInteractionSourceCollection>();
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

        public void Dispose()
        {
            RunOnUIThread.Execute(() =>
            {
                TurnOffAnchorNotifications();

                m_interactionSources.Clear();
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
    }
}
