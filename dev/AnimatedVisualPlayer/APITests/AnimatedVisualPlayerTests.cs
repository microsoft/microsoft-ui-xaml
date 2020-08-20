// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using MUXControlsTestApp.Utilities;

using Microsoft.UI.Xaml.Controls;
using Common;
using AnimatedVisuals;
using Windows.UI.Xaml.Markup;
using System.Threading;

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
    public class AnimatedVisualPlayerTests : ApiTestBase
    {
        [TestMethod]
        public void VerifyAutoPlayWhenSetInStyle()
        {
            AnimatedVisualPlayer avp = null;
            EventWaitHandle loadedEWH = null;
            RunOnUIThread.Execute(() =>
            {
                loadedEWH = new AutoResetEvent(false);

                avp = new AnimatedVisualPlayer();
                Content = avp;

                avp.Source = new LottieLogo();

                // Verify AnimatedVisualPlayer.AutoPlay defaults to true.
                Verify.IsTrue(avp.AutoPlay, "Default value of [AutoPlay] was expected to be [true].");

                // Set a custom style for our AnimatedVisualPlayer with [AutoPlay] set to false.
                var avpStyle = (Style)XamlReader.Load(
                        @"<Style xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:muxc='using:Microsoft.UI.Xaml.Controls' TargetType='muxc:AnimatedVisualPlayer'>
                            <Setter Property='AutoPlay' Value='False' />
                        </Style>");
                avp.Style = avpStyle;

                // We want to ensure that we only check if the player is playing the animation when the player has been loaded.
                avp.Loaded += (s, e) =>
                {
                    // Signal to our waiting thread that it can now proceed.
                    loadedEWH.Set();
                };

                Content.UpdateLayout();
            });

            // Wait until our AnimatedVisualPlayer instance has been loaded.
            loadedEWH.WaitOne(5000);

            // AnimatedVisualPlayer has been loaded. Now check if it is playing the animation or not.
            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(avp.IsAnimatedVisualLoaded, "The animated visual should have been loaded");
                Verify.IsFalse(avp.IsPlaying, "[AutoPlay] was set to [false] by a style, so the animation should have not been playing");
            });
        }
    }
}
