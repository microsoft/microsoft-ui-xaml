// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;

using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;

using Private.Infrastructure;
using Microsoft.UI.Xaml.Tests.Common;
using XamlMedia = Microsoft.UI.Xaml.Media;

using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Documents;
using Microsoft.UI.Xaml.Input;
using Windows.Foundation;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI;
using Microsoft.UI.Xaml.Media;

namespace Microsoft.UI.Xaml.Tests.Controls
{
    [TestClass]
    public class RichTextBlockTests : XamlTestsBase
    {
        [ClassInitialize]
        [TestProperty("BinaryUnderTest", "Microsoft.UI.Xaml.dll")]
        [TestProperty("RunAs", "UAP")]
        [TestProperty("UAP:Praid", "XamlManagedTAEFTests")]
        [TestProperty("Classification", "Integration")]
        public static void Setup(TestContext context)
        {
            AssemblySetup.CommonTestClassSetup();
        }

        [ClassCleanup]
        public void ClassCleanup()
        {
            base.CommonClassCleanup();
        }

        [TestMethod]
        [TestProperty("Description", "Validates textblock selection highlight on touch input")]
        [TestProperty("Hosting:Mode", "UAP")]
        [TestProperty("HasAssociatedMasterFile", "True")]
        public void RichTextBlockHighlightGripperSelectionFlyoutOnTouch()
        {

            using (TestServices.Utilities.CreateRenderingScopeGuard(
                DCompRendering.WUCCompleteSynchronousCompTree,
                resizeWindow: false,
                injectMockDComp: true,
                resetDevice: true))
            {
                const string rootPageXaml =
                    @"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <RichTextBlock x:Name = 'richTextBlock'>
                                <Paragraph>
                                    <Bold>
                                        <Span Foreground = 'DarkSlateBlue' FontSize = '16'> Mission Mars:</Span>
                                    </Bold>
                                    While working on the Mars
                                    <Hyperlink TabIndex='0' x:Name='hyperlink1'> Pathfinder </Hyperlink>
                                    mission at NASA\92s Jet Propulsion Laboratory in Pasadena, California, I came to a fork in the road.It was a moment when I had to decide whether to move out of my comfort zone and take a risk.
                                    NASA was putting together a 25 - person launch team that would temporarily move from California to Florida near the Kennedy Space Center.All of my best buddies were going to be on this team.
                                    I knew it was going to be a \93work hard, play hard\94 scenario, and I really wanted to go.
                                    Up until that point I had been working as the lead for the simulation software that was used to prepare for the mission. At one point, a colleague and I were tasked with fixing the flight simulation code,
                                    including the star scanner\97the eyeball of the spacecraft.The flight simulation code fakes out the spacecraft so it thinks it\92s flying to Mars when it\92s really sitting in the lab in Pasadena.
                                    The work had fallen nine months behind schedule, and my teammate Miguel and I had been given eight weeks to complete the code.Neither of us had ever written this kind of code before, but
                                    <Hyperlink TabIndex='1' x:Name='hyperlink2'> failure was not an option.</Hyperlink>
                                </Paragraph>
                            </RichTextBlock>
                        </Page>";

                Page  root = null;
                RichTextBlock richTextBlock = null;
                Point screenPoint = new Point(0,0);

                UIExecutor.Execute(() =>
                {
                    root = (Page)XamlReader.Load(rootPageXaml);
                    richTextBlock = (RichTextBlock)root.FindName("richTextBlock");

                    TestServices.WindowHelper.WindowContent = root;

                });
                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    GeneralTransform gt = richTextBlock.TransformToVisual(root);
                    screenPoint = gt.TransformPoint(screenPoint);                
                });
                TestServices.WindowHelper.WaitForIdle();

                // Touch at some text in RichTextBlock
                screenPoint.Y+=50;
                screenPoint.X+=100;
                TestServices.InputHelper.Tap(screenPoint);
                TestServices.WindowHelper.WaitForIdle();
                TestServices.Utilities.VerifyMockDCompOutput(MockDComp.SurfaceComparison.NoComparison);
                
                // Dismiss the selection
                screenPoint.X+=100;
                TestServices.InputHelper.Tap(screenPoint);
                TestServices.WindowHelper.WaitForIdle();
                
                // Touch at some text in RichTextBlock
                screenPoint.X+=100;
                TestServices.InputHelper.Tap(screenPoint);
                TestServices.WindowHelper.WaitForIdle();
                TestServices.Utilities.VerifyMockDCompOutput(MockDComp.SurfaceComparison.NoComparison, "touch_again");
            }

        }

    }
}

