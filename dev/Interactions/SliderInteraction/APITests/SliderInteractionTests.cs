// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;

using MUXControlsTestApp.Utilities;

using Microsoft.UI.Private.Controls;

using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Media;
using Windows.UI.Text;
using Windows.Foundation.Metadata;
using Common;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

namespace Windows.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public class SliderInteractionTests
    {
        [TestMethod]
        public void ValidateDefaultValues()
        {
            if (!ApiInformation.IsPropertyPresent("Windows.UI.Xaml.UIElement", "Interactions"))
            {
                Log.Warning("UIElement.Interactions not supported on this build.");
                return;
            }

            RunOnUIThread.Execute(() =>
            {
                var interaction = new SliderInteraction();

                Verify.AreEqual(Orientation.Horizontal, interaction.Orientation);
                Verify.AreEqual(0.0, interaction.Minimum);
                Verify.AreEqual(100.0, interaction.Maximum);
                Verify.AreEqual(0.0, interaction.Position);
                Verify.AreEqual(1.0, interaction.SmallChange);
                Verify.AreEqual(10.0, interaction.LargeChange);
            });
        }

        [TestMethod]
        public void DoesAdvertiseSupportedEvents()
        {
            if (!ApiInformation.IsPropertyPresent("Windows.UI.Xaml.UIElement", "Interactions"))
            {
                Log.Warning("UIElement.Interactions not supported on this build.");
                return;
            }

            RunOnUIThread.Execute(() =>
            {
#if USE_INTERNAL_SDK
                var interaction = new SliderInteraction();
                var supportedEvents = interaction.GetSupportedEvents();

                Verify.IsGreaterThan(supportedEvents.Count, 0);
#endif
            });
        }
    }
}
