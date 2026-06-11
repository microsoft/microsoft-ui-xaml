// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;

using MUXControlsTestApp.Utilities;

using Microsoft.UI.Private.Controls;

using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media;
using Windows.UI.Text;
using Windows.Foundation.Metadata;
using Common;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    //[TestClass] This feature requires the Xaml2018 feature, which is currently disabled by Velocity
    public class ButtonInteractionTests
    {
        [TestMethod]
        public void ValidateDefaultValues()
        {
            if (!ApiInformation.IsPropertyPresent("Microsoft.UI.Xaml.UIElement", "Interactions"))
            {
                Log.Warning("UIElement.Interactions not supported on this build.");
                return;
            }

            RunOnUIThread.Execute(() =>
            {
                var interaction = new ButtonInteraction();

                Verify.AreEqual(ButtonInteractionInvokeMode.Release, interaction.InvokeMode);
                Verify.AreEqual(false, interaction.IsHovering);
                Verify.AreEqual(false, interaction.IsPressing);
            });
        }

        [TestMethod]
        public void DoesAdvertiseSupportedEvents()
        {
            if (!ApiInformation.IsPropertyPresent("Microsoft.UI.Xaml.UIElement", "Interactions"))
            {
                Log.Warning("UIElement.Interactions not supported on this build.");
                return;
            }

            RunOnUIThread.Execute(() =>
            {
                var interaction = new ButtonInteraction();
                var supportedEvents = interaction.GetSupportedEvents();

                Verify.IsGreaterThan(supportedEvents.Count, 0);
            });
        }
    }
}
