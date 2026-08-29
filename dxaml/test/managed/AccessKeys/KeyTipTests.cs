// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;

using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;

using Private.Infrastructure;
using Microsoft.UI.Xaml.Tests.Common;
using XamlMedia = Microsoft.UI.Xaml.Media;
using XamlMarkup = Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Documents;
using Microsoft.UI.Xaml.Input;
using Windows.Foundation;
using System.Threading;
using Microsoft.UI.Xaml.Controls.Primitives;

using Colors = Microsoft.UI.Colors;

namespace Microsoft.UI.Xaml.Tests.Foundation.Graphics.AccessKeys
{
    [TestClass]
    public class KeyTipTests : XamlTestsBase
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

        [TestInitialize]
        public void TestSetup()
        {
            UIExecutor.Execute(() =>
            {
                // It helps test stability on phone to render a frame before the test starts, otherwise
                // we sometimes get an activation message during the test, which cases us to change the
                // FocusState.
                TestServices.WindowHelper.WindowContent = new Frame();
            });

            TestServices.WindowHelper.WaitForIdle();
        }

        [TestMethod]
        [TestProperty("VelocityTestPass:OneCoreStrict", "Desktop")]
        public void ApiSmokeTest()
        {
            UIExecutor.Execute(() =>
            {
                Verify.IsTrue(AccessKeyManager.AreKeyTipsEnabled);
                AccessKeyManager.AreKeyTipsEnabled = false;
                Verify.IsFalse(AccessKeyManager.AreKeyTipsEnabled);
                AccessKeyManager.AreKeyTipsEnabled = true;
                Verify.IsTrue(AccessKeyManager.AreKeyTipsEnabled);
            });
        }

        [TestMethod]
        [TestProperty("HasAssociatedMasterFile", "True")]
        public void KeyTipsOnlyShowWhenEnabled()
        {
            XamlRoot xamlRoot = null;
            UIExecutor.Execute(() =>
            {
                xamlRoot = TestServices.WindowHelper.WindowContent.XamlRoot;
            });

            using (new KeyTipHelper(xamlRoot))
            {
                Page root;

                UIExecutor.Execute(() =>
                {
                    root = (Page)XamlMarkup.XamlReader.Load(@"
                    <Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                        <StackPanel Margin='10'>
                            <Button AccessKey='1' KeyTipPlacementMode='Center'>button 1</Button>
                            <Button AccessKey='2' KeyTipPlacementMode='Center'>button 2</Button>
                            <Button AccessKey='3' KeyTipPlacementMode='Center'>button 3</Button>
                        </StackPanel>
                    </Page>");
                    TestServices.WindowHelper.WindowContent = root;
                    AccessKeyManager.AreKeyTipsEnabled = false;
                });
                TestServices.WindowHelper.WaitForIdle();
                AccessKeyHelper.TryMovingFocusToXamlForInit();

                TestServices.Utilities.VerifyUIElementTree("1");

                using (var eventTester = EventTester<object,object>.FromStaticEvent<AccessKeyManager>("IsDisplayModeEnabledChanged"))
                {
                    Log.Comment("Pressing ALT key to show KeyTips");
                    TestServices.KeyboardHelper.Alt();
                    eventTester.Wait();
                }

                // Wait a few ticks to make sure KeyTips aren't going to show up
                TestServices.WindowHelper.SynchronouslyTickUIThread(3);
                TestServices.Utilities.VerifyUIElementTree("2");
            }
        }


        [TestMethod]
        [TestProperty("VelocityTestPass:OneCoreStrict", "Desktop")]
        [TestProperty("HasAssociatedMasterFile", "True")]
        public void ClearKeyTipsWhenDisabled()
        {
            XamlRoot xamlRoot = null;
            UIExecutor.Execute(() =>
            {
                xamlRoot = TestServices.WindowHelper.WindowContent.XamlRoot;
            });

            using (new KeyTipHelper(xamlRoot))
            {
                Page root;
                Button button1 = null;

                UIExecutor.Execute(() =>
                {
                    root = (Page)XamlMarkup.XamlReader.Load(@"
                    <Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <StackPanel Margin='10'>
                            <Button x:Name='button1' AccessKey='123' KeyTipPlacementMode='Center'>button 1</Button>
                            <Button AccessKey='234' KeyTipPlacementMode='Center'>button 2</Button>
                            <Button AccessKey='345' KeyTipPlacementMode='Center'>button 3</Button>
                        </StackPanel>
                    </Page>");
                    TestServices.WindowHelper.WindowContent = root;
                    button1 = (Button)root.FindName("button1");
                });
                TestServices.WindowHelper.WaitForIdle();
                AccessKeyHelper.TryMovingFocusToXamlForInit();

                using (var eventTester1 = new EventTester<Button, AccessKeyDisplayRequestedEventArgs>(button1, "AccessKeyDisplayRequested"))
                {
                    Log.Comment("Pressing ALT key to show KeyTips");
                    TestServices.KeyboardHelper.Alt();
                    eventTester1.Wait();

                    KeyTipHelper.WaitUntilKeyTips("123 234 345");
                    TestServices.Utilities.VerifyUIElementTree("1");

                    UIExecutor.Execute(() =>
                    {
                        Log.Comment("Turn off KeyTips");
                        AccessKeyManager.AreKeyTipsEnabled = false;
                    });
                    KeyTipHelper.WaitUntilKeyTips("");

                    TestServices.Utilities.VerifyUIElementTree("2");

                    using (var eventTester = new EventTester<Button, AccessKeyDisplayRequestedEventArgs>(button1, "AccessKeyDisplayRequested"))
                    {
                        Log.Comment("Press 1 key, AccessKeyDisplayRequested event should still fire");
                        TestServices.KeyboardHelper.PressKeySequence("1");
                        eventTester.Wait();
                    }

                    Log.Comment("Scene should remain unchanged.");

                    // Wait a few ticks to make sure KeyTips aren't going to show up
                    TestServices.WindowHelper.SynchronouslyTickUIThread(3);
                    TestServices.Utilities.VerifyUIElementTree("2");
                }
            }
        }


        [TestMethod]
        [TestProperty("VelocityTestPass:OneCoreStrict", "Desktop")]
        [TestProperty("HasAssociatedMasterFile", "True")]
        public void SimpleScene()
        {
            XamlRoot xamlRoot = null;
            UIExecutor.Execute(() =>
            {
                xamlRoot = TestServices.WindowHelper.WindowContent.XamlRoot;
            });

            using (new KeyTipHelper(xamlRoot))
            {
                Page root;

                UIExecutor.Execute(() =>
                {
                    root = (Page)XamlMarkup.XamlReader.Load(@"
                    <Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                        <StackPanel>
                            <Button AccessKey='1' KeyTipPlacementMode='Center'>button 1</Button>
                            <Button AccessKey='ii' KeyTipPlacementMode='Center'>button 2</Button>
                            <Button AccessKey='WW' KeyTipPlacementMode='Center'>button 3</Button>
                            <TextBlock><Hyperlink AccessKey='h' KeyTipPlacementMode='Center'>Hyperlink</Hyperlink></TextBlock>
                        </StackPanel>
                    </Page>");

                    TestServices.WindowHelper.WindowContent = root;
                });
                TestServices.WindowHelper.WaitForIdle();
                AccessKeyHelper.TryMovingFocusToXamlForInit();

                using (var eventHelper = EventTester<object,object>.FromStaticEvent<AccessKeyManager>("IsDisplayModeEnabledChanged"))
                {
                    Log.Comment("Pressing ALT key to show KeyTips");
                    TestServices.KeyboardHelper.Alt();
                    eventHelper.Wait();
                }

                KeyTipHelper.WaitUntilKeyTips("1 ii WW h");
                TestServices.Utilities.VerifyUIElementTree();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Validates key tips on pivot control.")]
        [TestProperty("VelocityTestPass:OneCoreStrict", "Desktop")]
        [TestProperty("HasAssociatedMasterFile", "True")]
        [TestProperty("Hosting:Mode", "UAP")]  // crash reason: crashes at the end
        public void PivotSimpleScene()
        {
            XamlRoot xamlRoot = null;
            UIExecutor.Execute(() =>
            {
                xamlRoot = TestServices.WindowHelper.WindowContent.XamlRoot;
            });

            using (new KeyTipHelper(xamlRoot))
            {
                TestServices.WindowHelper.SetWindowSizeOverride(new Size(800, 400));

                const string rootPanelXaml =
                   @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <Pivot x:Name='rootPivot' Title='PIVOT WITH ACCESS KEYS' >
                            <Pivot.RightHeader>
                                <CommandBar ClosedDisplayMode='Compact'>
                                    <AppBarButton x:Name='Back' Icon='Back' Label='Previous'  />
                                    <AppBarButton x:Name='Forward' Icon='Forward' Label='Next'   />
                                </CommandBar>
                            </Pivot.RightHeader>
                            <PivotItem x:Name = 'pivotItem1' Header='Pivot Item P' AccessKey='p'>
                                <TextBlock Text='Content of pivot item 1.' />
                            </PivotItem>
                            <PivotItem x:Name = 'pivotItem2' Header='Pivot Item 123' AccessKey='123'>
                                <TextBlock Text='Content of pivot item 2.' />
                            </PivotItem>
                            <PivotItem x:Name = 'pivotItem4' Header='Pivot Item R' AccessKey='r'>
                                <TextBlock Text='Content of pivot item 4.' />
                            </PivotItem>
                        </Pivot>
                    </StackPanel>";

                StackPanel rootPanel = null;

                UIExecutor.Execute(() =>
                {
                    rootPanel = (StackPanel)XamlMarkup.XamlReader.Load(rootPanelXaml);
                    TestServices.WindowHelper.WindowContent = rootPanel;
                });
                TestServices.WindowHelper.WaitForIdle();

                using (var eventHelper = EventTester<object, object>.FromStaticEvent<AccessKeyManager>("IsDisplayModeEnabledChanged"))
                {
                    Log.Comment("Pressing ALT key to show KeyTips");
                    TestServices.KeyboardHelper.Alt();
                    eventHelper.Wait();
                }

                KeyTipHelper.WaitUntilKeyTips("p 123 r");
                TestServices.Utilities.VerifyUIElementTree();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Validates key tip displayed at specified key tip target.")]
        [TestProperty("VelocityTestPass:OneCoreStrict", "Desktop")]
        [TestProperty("HasAssociatedMasterFile", "True")]
        [TestProperty("Hosting:Mode", "UAP")] // crash reason: crash at test end

        public void ValidateKeyTipTarget()
        {
            XamlRoot xamlRoot = null;
            UIExecutor.Execute(() =>
            {
                xamlRoot = TestServices.WindowHelper.WindowContent.XamlRoot;
            });

            using (new KeyTipHelper(xamlRoot))
            {
                const string rootPanelXaml =
                   @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <Button x:Name = 'MyButton' AccessKey='N' KeyTipPlacementMode='Center'>Button</Button>
                        <TextBlock><Hyperlink x:Name = 'MyLink'>Hyperlink</Hyperlink></TextBlock>
                    </StackPanel>";

                StackPanel rootPanel = null;
                Button button = null;
                Hyperlink link = null;

                UIExecutor.Execute(() =>
                {
                    rootPanel = (StackPanel)XamlMarkup.XamlReader.Load(rootPanelXaml);
                    button = (Button)rootPanel.FindName("MyButton");
                    link = (Hyperlink)rootPanel.FindName("MyLink");
                    button.KeyTipTarget = link;
                    TestServices.WindowHelper.WindowContent = rootPanel;
                });
                TestServices.WindowHelper.WaitForIdle();

                using (var eventHelper = EventTester<object, object>.FromStaticEvent<AccessKeyManager>("IsDisplayModeEnabledChanged"))
                {
                    Log.Comment("Pressing ALT key to show KeyTips");
                    TestServices.KeyboardHelper.Alt();
                    eventHelper.Wait();
                }

                KeyTipHelper.WaitUntilKeyTips("N");
                TestServices.Utilities.VerifyUIElementTree();
            }
        }

        [TestMethod]
        [TestProperty("VelocityTestPass:OneCoreStrict", "Desktop")]
        [TestProperty("HasAssociatedMasterFile", "True")]
        public void KeyTipLightTheme()
        {
            XamlRoot xamlRoot = null;
            UIExecutor.Execute(() =>
            {
                xamlRoot = TestServices.WindowHelper.WindowContent.XamlRoot;
            });

            using (new KeyTipHelper(xamlRoot))
            {
                Page root;

                UIExecutor.Execute(() =>
                {
                    root = (Page)XamlMarkup.XamlReader.Load(@"
                    <Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                        <StackPanel>
                            <Button AccessKey='1' KeyTipPlacementMode='Center'>button 1</Button>
                        </StackPanel>
                    </Page>");

                    TestServices.WindowHelper.WindowContent = root;
                    root.RequestedTheme = ElementTheme.Light;
                });
                TestServices.WindowHelper.WaitForIdle();
                AccessKeyHelper.TryMovingFocusToXamlForInit();

                using (var eventHelper = EventTester<object, object>.FromStaticEvent<AccessKeyManager>("IsDisplayModeEnabledChanged"))
                {
                    Log.Comment("Pressing ALT key to show KeyTips");
                    TestServices.KeyboardHelper.Alt();
                    eventHelper.Wait();
                }

                KeyTipHelper.WaitUntilKeyTips("1");
                TestServices.Utilities.VerifyUIElementTree();
            }
        }

        [TestMethod]
        [TestProperty("VelocityTestPass:OneCoreStrict", "Desktop")]
        [TestProperty("HasAssociatedMasterFile", "True")]
        public void KeyTipDarkTheme()
        {
            XamlRoot xamlRoot = null;
            UIExecutor.Execute(() =>
            {
                xamlRoot = TestServices.WindowHelper.WindowContent.XamlRoot;
            });

            using (new KeyTipHelper(xamlRoot))
            {
                Page root;

                UIExecutor.Execute(() =>
                {
                    root = (Page)XamlMarkup.XamlReader.Load(@"
                    <Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                        <StackPanel>
                            <Button AccessKey='1' KeyTipPlacementMode='Center'>button 1</Button>
                        </StackPanel>
                    </Page>");

                    TestServices.WindowHelper.WindowContent = root;
                    root.RequestedTheme = ElementTheme.Dark;
                });
                TestServices.WindowHelper.WaitForIdle();
                AccessKeyHelper.TryMovingFocusToXamlForInit();

                using (var eventHelper = EventTester<object, object>.FromStaticEvent<AccessKeyManager>("IsDisplayModeEnabledChanged"))
                {
                    Log.Comment("Pressing ALT key to show KeyTips");
                    TestServices.KeyboardHelper.Alt();
                    eventHelper.Wait();
                }

                KeyTipHelper.WaitUntilKeyTips("1");
                TestServices.Utilities.VerifyUIElementTree();
            }
        }

        [TestMethod]
        [TestProperty("VelocityTestPass:OneCoreStrict", "Desktop")]
        [TestProperty("HasAssociatedMasterFile", "True")]
        public void KeyTipHCTheme()
        {
            HighContrastWorker(ApplicationHighContrastAdjustment.Auto);
        }

        [TestMethod]
        [TestProperty("VelocityTestPass:OneCoreStrict", "Desktop")]
        [TestProperty("HasAssociatedMasterFile", "True")]
        public void KeyTipHCThemeNoAdjustment()
        {
            HighContrastWorker(ApplicationHighContrastAdjustment.None);
        }

        public void HighContrastWorker(ApplicationHighContrastAdjustment adjustment)
        {
            XamlRoot xamlRoot = null;
            UIExecutor.Execute(() =>
            {
                xamlRoot = TestServices.WindowHelper.WindowContent.XamlRoot;
            });

            using (new KeyTipHelper(xamlRoot))
            {
                Page root;
                ApplicationHighContrastAdjustment previousHighContastAdjustment = ApplicationHighContrastAdjustment.None;
                HighContrastTheme previousHighContrastTheme = HighContrastTheme.None;

                UIExecutor.Execute(() =>
                {
                    previousHighContastAdjustment = Application.Current.HighContrastAdjustment;
                    Application.Current.HighContrastAdjustment = adjustment;

                    root = (Page)XamlMarkup.XamlReader.Load(@"
                    <Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                        <StackPanel>
                            <Button AccessKey='1' KeyTipPlacementMode='Center'>button 1</Button>
                        </StackPanel>
                    </Page>");

                    TestServices.WindowHelper.WindowContent = root;
                    previousHighContrastTheme = TestServices.ThemingHelper.HighContrastTheme;
                    TestServices.ThemingHelper.HighContrastTheme = HighContrastTheme.Test;
                    root.Background = new Microsoft.UI.Xaml.Media.SolidColorBrush(Colors.Black);
                });
                TestServices.WindowHelper.WaitForIdle();
                AccessKeyHelper.TryMovingFocusToXamlForInit();

                UIExecutor.Execute(() =>
                {
                    TestServices.ThemingHelper.HighContrastTheme = HighContrastTheme.Test;
                });
                TestServices.WindowHelper.WaitForIdle();

                using (var eventHelper = EventTester<object, object>.FromStaticEvent<AccessKeyManager>("IsDisplayModeEnabledChanged"))
                {
                    Log.Comment("Pressing ALT key to show KeyTips");
                    TestServices.KeyboardHelper.Alt();
                    eventHelper.Wait();
                }

                KeyTipHelper.WaitUntilKeyTips("1");
                TestServices.Utilities.VerifyUIElementTree();

                UIExecutor.Execute(() =>
                {
                    Log.Comment("Reset HighContrastTheme and HighContrastAdjustment.");
                    TestServices.ThemingHelper.HighContrastTheme = previousHighContrastTheme;
                    Application.Current.HighContrastAdjustment = previousHighContastAdjustment;
                });
                TestServices.WindowHelper.WaitForIdle();
            }
        }

        [TestMethod]
        [TestProperty("VelocityTestPass:OneCoreStrict", "Desktop")]
        [TestProperty("HasAssociatedMasterFile", "True")]
        [TestProperty("Hosting:Mode", "UAP")]  // crash reason: cannot move focus to hyperlink
        public void MultipleHyperlinks()
        {
            XamlRoot xamlRoot = null;
            UIExecutor.Execute(() =>
            {
                xamlRoot = TestServices.WindowHelper.WindowContent.XamlRoot;
            });

            using (new KeyTipHelper(xamlRoot))
            {
                Page root;

                UIExecutor.Execute(() =>
                {
                    root = (Page)XamlMarkup.XamlReader.Load(@"
                    <Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                        <StackPanel>
                            <TextBlock>
                                <Hyperlink AccessKey='h' KeyTipPlacementMode='Right'>Hyperlink</Hyperlink>
                                <LineBreak/>
                                <Hyperlink AccessKey='y' KeyTipPlacementMode='Right'>Hyperlink</Hyperlink>
                                <LineBreak/>
                                <Hyperlink AccessKey='p' KeyTipPlacementMode='Right'>Hyperlink</Hyperlink>
                            </TextBlock>
                        </StackPanel>
                    </Page>");

                    TestServices.WindowHelper.WindowContent = root;
                });
                TestServices.WindowHelper.WaitForIdle();
                AccessKeyHelper.TryMovingFocusToXamlForInit();

                using (var eventHelper = EventTester<object,object>.FromStaticEvent<AccessKeyManager>("IsDisplayModeEnabledChanged"))
                {
                    Log.Comment("Pressing ALT key to show KeyTips");
                    TestServices.KeyboardHelper.Alt();
                    eventHelper.Wait();
                }

                KeyTipHelper.WaitUntilKeyTips("h y p");
                TestServices.Utilities.VerifyUIElementTree();
            }
        }

        [TestMethod]
        [TestProperty("VelocityTestPass:OneCoreStrict", "Desktop")]
        [TestProperty("HasAssociatedMasterFile", "True")]
        [TestProperty("Hosting:Mode", "UAP")] // crash reason: focus doesn't move
        public void KeyTipProperlyPlacedOnHyperlinkWithScaledTextBlock()
        {
            XamlRoot xamlRoot = null;
            UIExecutor.Execute(() =>
            {
                xamlRoot = TestServices.WindowHelper.WindowContent.XamlRoot;
            });

            using (new KeyTipHelper(xamlRoot))
            {
                Page root;

                UIExecutor.Execute(() =>
                {
                    root = (Page)XamlMarkup.XamlReader.Load(@"
                    <Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                        <StackPanel>
                            <TextBlock>
                                <TextBlock.RenderTransform>
                                    <TransformGroup>
                                        <ScaleTransform ScaleX='5' ScaleY='5'/>
                                    </TransformGroup>
                                </TextBlock.RenderTransform>
                              <Hyperlink AccessKey='h' KeyTipPlacementMode='Right'>hyperlink</Hyperlink>
                            </TextBlock>
                        </StackPanel>
                    </Page>");

                    TestServices.WindowHelper.WindowContent = root;
                });
                TestServices.WindowHelper.WaitForIdle();
                AccessKeyHelper.TryMovingFocusToXamlForInit();

                using (var eventHelper = EventTester<object,object>.FromStaticEvent<AccessKeyManager>("IsDisplayModeEnabledChanged"))
                {
                    Log.Comment("Pressing ALT key to show KeyTips");
                    TestServices.KeyboardHelper.Alt();
                    eventHelper.Wait();
                }

                KeyTipHelper.WaitUntilKeyTips("h");
                TestServices.Utilities.VerifyUIElementTree();
            }
        }

        [TestMethod]
        [TestProperty("VelocityTestPass:OneCoreStrict", "Desktop")]
        [TestProperty("HasAssociatedMasterFile", "True")]
        [TestProperty("Hosting:Mode", "UAP")]  // crash reason: focus doesn't move
        public void HyperlinkOnMultipleLines()
        {
           XamlRoot xamlRoot = null;
            UIExecutor.Execute(() =>
            {
                xamlRoot = TestServices.WindowHelper.WindowContent.XamlRoot;
            });

            using (new KeyTipHelper(xamlRoot))
            {
                Page root;

                UIExecutor.Execute(() =>
                {
                    root = (Page)XamlMarkup.XamlReader.Load(@"
                    <Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                        <StackPanel>
                            <TextBlock>
                                <Hyperlink KeyTipPlacementMode='Right' AccessKey='h' xml:space='preserve'>This is a really long hyperlink
that spans multiple lines. BlahBlahBlahBlah</Hyperlink>
                            </TextBlock>
                        </StackPanel>
                    </Page>");

                    TestServices.WindowHelper.WindowContent = root;
                });
                TestServices.WindowHelper.WaitForIdle();
                AccessKeyHelper.TryMovingFocusToXamlForInit();

                Log.Comment("Pressing ALT key to show KeyTips");
                TestServices.KeyboardHelper.Alt();

                KeyTipHelper.WaitUntilKeyTips("h");
                TestServices.Utilities.VerifyUIElementTree();
            }
        }

        [TestMethod]
        [TestProperty("VelocityTestPass:OneCoreStrict", "Desktop")]
        [TestProperty("HasAssociatedMasterFile", "True")]
        [TestProperty("Hosting:Mode", "UAP")]  // crash reason: focus doesn't move
        public void MultipleCharacters()
        {
            XamlRoot xamlRoot = null;
            UIExecutor.Execute(() =>
            {
                xamlRoot = TestServices.WindowHelper.WindowContent.XamlRoot;
            });

            using (new KeyTipHelper(xamlRoot))
            {
                Page page;
                Button button = null;

                UIExecutor.Execute(() =>
                {
                    page = (Page)XamlMarkup.XamlReader.Load(@"
                    <Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <StackPanel>
                            <Button AccessKey='ABC' KeyTipPlacementMode='Center'>button 1</Button>
                            <Button AccessKey='ADE' KeyTipPlacementMode='Center' x:Name='button'>button 2</Button>
                            <Button AccessKey='ZXY' KeyTipPlacementMode='Center'>button 3</Button>
                        </StackPanel>
                    </Page>");
                    button = (Button)page.FindName("button");
                    TestServices.WindowHelper.WindowContent = page;
                });
                TestServices.WindowHelper.WaitForIdle();

                Log.Comment("Pressing ALT key to show KeyTips");
                TestServices.KeyboardHelper.Alt();
                KeyTipHelper.WaitUntilKeyTips("ABC ADE ZXY");
                TestServices.Utilities.VerifyUIElementTree("1");

                TestServices.KeyboardHelper.PressKeySequence("a");
                KeyTipHelper.WaitUntilKeyTips("BC DE");
                TestServices.Utilities.VerifyUIElementTree("2");

                TestServices.KeyboardHelper.PressKeySequence("d");
                KeyTipHelper.WaitUntilKeyTips("E");
                TestServices.Utilities.VerifyUIElementTree("3");

                TestServices.KeyboardHelper.PressKeySequence("e");
                KeyTipHelper.WaitUntilKeyTips("");
                TestServices.Utilities.VerifyUIElementTree("4");
            }
        }

        [TestMethod]
        [TestProperty("VelocityTestPass:OneCoreStrict", "Desktop")]
        [TestProperty("HasAssociatedMasterFile", "True")]
        [TestProperty("IsolationLevel", "Method")]
        public void OldPopupsAreRemoved()
        {
            XamlRoot xamlRoot = null;
            UIExecutor.Execute(() =>
            {
                xamlRoot = TestServices.WindowHelper.WindowContent.XamlRoot;
            });

            using (new KeyTipHelper(xamlRoot))
            using (new OverrideAppResources(@"
                <ResourceDictionary xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                    <Thickness x:Key='KeyTipBorderThemeThickness'>2</Thickness>
                    <SolidColorBrush x:Key='KeyTipBorderBrush' Color='#ffff0000' />
                </ResourceDictionary>"))
            {
                Page page;
                Button button = null;

                UIExecutor.Execute(() =>
                {
                    page = (Page)XamlMarkup.XamlReader.Load(@"
                    <Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <StackPanel>
                            <Button AccessKey='WWW' KeyTipPlacementMode='Right' x:Name='button'>button</Button>
                        </StackPanel>
                    </Page>");
                    button = (Button)page.FindName("button");
                    TestServices.WindowHelper.WindowContent = page;
                });
                TestServices.WindowHelper.WaitForIdle();
                AccessKeyHelper.TryMovingFocusToXamlForInit();

                using (var eventTester = new EventTester<Button, AccessKeyDisplayRequestedEventArgs>(button, "AccessKeyDisplayRequested"))
                {
                    Log.Comment("Pressing ALT key to show KeyTips");
                    TestServices.KeyboardHelper.Alt();
                    eventTester.Wait();
                }

                using (var eventTester = new EventTester<Button, AccessKeyDisplayRequestedEventArgs>(button, "AccessKeyDisplayRequested"))
                {
                    Log.Comment("Pressing W");
                    TestServices.KeyboardHelper.PressKeySequence("w");
                    eventTester.Wait();
                    KeyTipHelper.WaitUntilKeyTips("WW");
                    TestServices.Utilities.VerifyUIElementTree();
                }
            }
        }


        [TestMethod]
        [TestProperty("VelocityTestPass:OneCoreStrict", "Desktop")]
        [TestProperty("HasAssociatedMasterFile", "True")]
        public void MultipleScopes()
        {
            XamlRoot xamlRoot = null;
            UIExecutor.Execute(() =>
            {
                xamlRoot = TestServices.WindowHelper.WindowContent.XamlRoot;
            });

            using (new KeyTipHelper(xamlRoot))
            {
                Page page;
                StackPanel root;
                StackPanel stackPanel = null;
                Button button1 = null;

                UIExecutor.Execute(() =>
                {
                    page = (Page)XamlMarkup.XamlReader.Load(@"
                    <Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <StackPanel Margin='10'>
                            <Button AccessKey='ABC' KeyTipPlacementMode='Center'>button 1</Button>
                            <Button AccessKey='ADE' KeyTipPlacementMode='Center'>button 2</Button>
                            <Button AccessKey='XYZ' KeyTipPlacementMode='Center'>button 3</Button>
                            <StackPanel x:Name='stackPanel' AccessKey='S' IsAccessKeyScope='True' KeyTipPlacementMode='Center'>
                                <Button x:Name='button1' AccessKey='145' KeyTipPlacementMode='Center'>button 1</Button>
                                <Button AccessKey='999' KeyTipPlacementMode='Center'>button 2</Button>
                                <Button AccessKey='123' KeyTipPlacementMode='Center'>button 3</Button>
                            </StackPanel>
                        </StackPanel>
                    </Page>");
                    stackPanel = (StackPanel)page.FindName("stackPanel");
                    button1 = (Button)page.FindName("button1");
                    root = (StackPanel)page.Content;
                    TestServices.WindowHelper.WindowContent = page;
                });
                TestServices.WindowHelper.WaitForIdle();
                AccessKeyHelper.TryMovingFocusToXamlForInit();

                using (var displayRequestedTester = new EventTester<StackPanel, AccessKeyDisplayRequestedEventArgs>(stackPanel, "AccessKeyDisplayRequested"))
                {
                    Log.Comment("Pressing ALT key to show KeyTips");
                    TestServices.KeyboardHelper.Alt();
                    displayRequestedTester.Wait();
                    KeyTipHelper.WaitUntilKeyTips("ABC ADE XYZ S");
                    TestServices.Utilities.VerifyUIElementTree("1");
                }

                using (var displayRequestedTester = new EventTester<Button, AccessKeyDisplayRequestedEventArgs>(button1, "AccessKeyDisplayRequested"))
                {
                    TestServices.KeyboardHelper.PressKeySequence("s");
                    displayRequestedTester.Wait();
                    KeyTipHelper.WaitUntilKeyTips("145 999 123");
                    TestServices.Utilities.VerifyUIElementTree("2");
                }

                using (var displayRequestedTester = new EventTester<Button, AccessKeyDisplayRequestedEventArgs>(button1, "AccessKeyDisplayRequested"))
                {
                    TestServices.KeyboardHelper.PressKeySequence("1");
                    displayRequestedTester.Wait();
                    KeyTipHelper.WaitUntilKeyTips("45 23");
                    TestServices.Utilities.VerifyUIElementTree("3");
                }

                using (var displayRequestedTester = new EventTester<Button, AccessKeyDisplayRequestedEventArgs>(button1, "AccessKeyDisplayRequested"))
                {
                    TestServices.KeyboardHelper.Escape();
                    displayRequestedTester.Wait();
                    KeyTipHelper.WaitUntilKeyTips("145 999 123");
                    TestServices.Utilities.VerifyUIElementTree("4");
                }

                using (var displayRequestedTester = new EventTester<StackPanel, AccessKeyDisplayRequestedEventArgs>(stackPanel, "AccessKeyDisplayRequested"))
                {
                    TestServices.KeyboardHelper.Escape();
                    displayRequestedTester.Wait();
                    KeyTipHelper.WaitUntilKeyTips("ABC ADE XYZ S");
                    TestServices.Utilities.VerifyUIElementTree("5");
                }
            }
        }

        [TestMethod]
        [TestProperty("VelocityTestPass:OneCoreStrict", "Desktop")]
        [TestProperty("HasAssociatedMasterFile", "True")]
        [TestProperty("Hosting:Mode", "UAP")]  // crash reason: cannot move focus

        public void ElementRemoval()
        {
            XamlRoot xamlRoot = null;
            UIExecutor.Execute(() =>
            {
                xamlRoot = TestServices.WindowHelper.WindowContent.XamlRoot;
            });

            using (new KeyTipHelper(xamlRoot))
            {
                StackPanel root = null;

                UIExecutor.Execute(() =>
                {
                    root = (StackPanel)XamlMarkup.XamlReader.Load(@"
                    <StackPanel Margin='10' Padding='10' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                    </StackPanel>");

                    TestServices.WindowHelper.WindowContent = root;
                });
                TestServices.WindowHelper.WaitForIdle();


                TestServices.Utilities.VerifyUIElementTree("1");

                UIExecutor.Execute(() =>
                {
                    Log.Comment("Add some buttons");
                    root.Children.Add(new Button() { Content = "button 1", AccessKey = "1", KeyTipPlacementMode = KeyTipPlacementMode.Center });
                    root.Children.Add(new Button() { Content = "button 2", AccessKey = "2", KeyTipPlacementMode = KeyTipPlacementMode.Center  });
                    root.Children.Add(new Button() { Content = "button 3", AccessKey = "3", KeyTipPlacementMode = KeyTipPlacementMode.Center  });
                });
                TestServices.WindowHelper.WaitForIdle();

                using (var displayModeEventTester = EventTester<object,object>.FromStaticEvent<AccessKeyManager>("IsDisplayModeEnabledChanged"))
                {
                    Log.Comment("Pressing ALT key to show KeyTips");
                    TestServices.KeyboardHelper.Alt();
                    displayModeEventTester.Wait();
                }

                Log.Comment("There should be some popups now");
                KeyTipHelper.WaitUntilKeyTips("1 2 3");
                TestServices.Utilities.VerifyUIElementTree("2");

                UIExecutor.Execute(() =>
                {
                    Log.Comment("Clear out the elements that had KeyTips");
                    root.Children.Clear();
                });
                TestServices.WindowHelper.WaitForIdle();

                Log.Comment("Pressing ALT key to hide KeyTips");
                TestServices.KeyboardHelper.Alt();
                KeyTipHelper.WaitUntilKeyTips("");

                Log.Comment("Scene should be empty");
                TestServices.Utilities.VerifyUIElementTree("1");
            }
        }

        [TestMethod]
        [TestProperty("VelocityTestPass:OneCoreStrict", "Desktop")]
        [TestProperty("HasAssociatedMasterFile", "True")]
        public void KeyTipPlacement()
        {
            XamlRoot xamlRoot = null;
            UIExecutor.Execute(() =>
            {
                xamlRoot = TestServices.WindowHelper.WindowContent.XamlRoot;
            });

            using (new KeyTipHelper(xamlRoot))
            {
                Page root = null;
                Button button1 = null;
                Button button2 = null;
                Button button3 = null;

                UIExecutor.Execute(() =>
                {
                    root = (Page)XamlMarkup.XamlReader.Load(@"
                    <Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <StackPanel Margin='10'>
                            <Button x:Name='button1' KeyTipPlacementMode='Left' AccessKey='1'>button 1</Button>
                            <Button x:Name='button2' KeyTipHorizontalOffset='20' AccessKey='2'>button 2</Button>
                            <Button x:Name='button3' KeyTipPlacementMode='Bottom' KeyTipVerticalOffset='-20' AccessKey='3'>button 3</Button>
                        </StackPanel>
                    </Page>");

                    button1 = (Button)root.FindName("button1");
                    button2 = (Button)root.FindName("button2");
                    button3 = (Button)root.FindName("button3");
                    TestServices.WindowHelper.WindowContent = root;

                    Verify.IsTrue((KeyTipPlacementMode)button2.GetValue(UIElement.KeyTipPlacementModeProperty) == KeyTipPlacementMode.Auto);
                    button2.KeyTipPlacementMode = KeyTipPlacementMode.Center;
                });
                TestServices.WindowHelper.WaitForIdle();
                AccessKeyHelper.TryMovingFocusToXamlForInit();

                using (var displayRequestedTester = new EventTester<Button, AccessKeyDisplayRequestedEventArgs>(button1, "AccessKeyDisplayRequested"))
                {
                    Log.Comment("Pressing ALT key to show KeyTips");
                    TestServices.KeyboardHelper.Alt();
                    displayRequestedTester.Wait();
                }

                KeyTipHelper.WaitUntilKeyTips("1 2 3");
                TestServices.Utilities.VerifyUIElementTree("1");
            }
        }

        [TestMethod]
        [TestProperty("VelocityTestPass:OneCoreStrict", "Desktop")]
        [TestProperty("HasAssociatedMasterFile", "True")]
        public void KeyTipPlacementInheritance()
        {
            XamlRoot xamlRoot = null;
            UIExecutor.Execute(() =>
            {
                xamlRoot = TestServices.WindowHelper.WindowContent.XamlRoot;
            });

            using (new KeyTipHelper(xamlRoot))
            {
                Page root = null;
                Button button1 = null;
                Button button2 = null;
                Button button3 = null;

                UIExecutor.Execute(() =>
                {
                    root = (Page)XamlMarkup.XamlReader.Load(@"
                    <Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                        KeyTipVerticalOffset='7' KeyTipHorizontalOffset='7'>
                        <StackPanel Margin='20' KeyTipPlacementMode='Right'>
                            <Button x:Name='button1' KeyTipPlacementMode='Left' AccessKey='1'>button 1</Button>
                            <Button x:Name='button2' KeyTipHorizontalOffset='20' AccessKey='2'>button 2</Button>
                            <Button x:Name='button3' KeyTipPlacementMode='Bottom' KeyTipVerticalOffset='-20' AccessKey='3'>button 3</Button>
                        </StackPanel>
                    </Page>");

                    button1 = (Button)root.FindName("button1");
                    button2 = (Button)root.FindName("button2");
                    button3 = (Button)root.FindName("button3");
                    TestServices.WindowHelper.WindowContent = root;

                    Verify.AreEqual(KeyTipPlacementMode.Left, button1.KeyTipPlacementMode);
                    Verify.AreEqual(KeyTipPlacementMode.Right, button2.KeyTipPlacementMode);
                    Verify.AreEqual(KeyTipPlacementMode.Bottom, button3.KeyTipPlacementMode);
                });
                TestServices.WindowHelper.WaitForIdle();
                AccessKeyHelper.TryMovingFocusToXamlForInit();

                using (var displayRequestedTester = new EventTester<Button, AccessKeyDisplayRequestedEventArgs>(button1, "AccessKeyDisplayRequested"))
                {
                    Log.Comment("Pressing ALT key to show KeyTips");
                    TestServices.KeyboardHelper.Alt();
                    displayRequestedTester.Wait();
                }

                KeyTipHelper.WaitUntilKeyTips("1 2 3");
                TestServices.Utilities.VerifyUIElementTree();
            }
        }

        [TestMethod]
        [TestProperty("VelocityTestPass:OneCoreStrict", "Desktop")]
        [TestProperty("HasAssociatedMasterFile", "True")]
        public void ClippedOut()
        {
            XamlRoot xamlRoot = null;
            UIExecutor.Execute(() =>
            {
                xamlRoot = TestServices.WindowHelper.WindowContent.XamlRoot;
            });

            using (new KeyTipHelper(xamlRoot))
            {
                Page root = null;
                Button button1 = null;

                UIExecutor.Execute(() =>
                {
                    root = (Page)XamlMarkup.XamlReader.Load(@"
                    <Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <StackPanel Margin='10'>
                            <StackPanel.Clip>
                                <RectangleGeometry Rect='0,0, 500,40' />
                            </StackPanel.Clip>
                            <Button x:Name='button1' AccessKey='1' KeyTipPlacementMode='Center'>button 1</Button>
                            <Button x:Name='button2' AccessKey='2' KeyTipPlacementMode='Center'>button 2</Button>
                            <Button x:Name='button3' AccessKey='3' KeyTipPlacementMode='Center'>button 3</Button>
                        </StackPanel>
                    </Page>");
                    button1 = (Button)root.FindName("button1");
                    TestServices.WindowHelper.WindowContent = root;
                });
                TestServices.WindowHelper.WaitForIdle();
                AccessKeyHelper.TryMovingFocusToXamlForInit();

                using (var displayRequestedTester = new EventTester<Button, AccessKeyDisplayRequestedEventArgs>(button1, "AccessKeyDisplayRequested"))
                {
                    Log.Comment("Pressing ALT key to show KeyTips");
                    TestServices.KeyboardHelper.Alt();
                    displayRequestedTester.Wait();
                }

                KeyTipHelper.WaitUntilKeyTips("1 2");
                TestServices.Utilities.VerifyUIElementTree("1");
            }
        }

        [TestMethod]
        [TestProperty("VelocityTestPass:OneCoreStrict", "Desktop")]
        [TestProperty("HasAssociatedMasterFile", "True")]
        public void RightToLeft()
        {
            XamlRoot xamlRoot = null;
            UIExecutor.Execute(() =>
            {
                xamlRoot = TestServices.WindowHelper.WindowContent.XamlRoot;
            });

            using (new KeyTipHelper(xamlRoot))
            {
                Page root = null;
                Button button1 = null;

                UIExecutor.Execute(() =>
                {
                    root = (Page)XamlMarkup.XamlReader.Load(@"
                    <Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' FlowDirection='RightToLeft'>
                        <StackPanel Margin='30'>
                            <Button x:Name='button1' AccessKey='1' KeyTipPlacementMode='Left'>button 1</Button>
                            <Button x:Name='button2' AccessKey='2' KeyTipPlacementMode='Right' KeyTipHorizontalOffset='-5'>button 2</Button>
                            <Button x:Name='button3' AccessKey='3' KeyTipPlacementMode='Center'>button 3</Button>
                        </StackPanel>
                    </Page>");
                    button1 = (Button)root.FindName("button1");
                    TestServices.WindowHelper.WindowContent = root;
                });
                TestServices.WindowHelper.WaitForIdle();
                AccessKeyHelper.TryMovingFocusToXamlForInit();

                using (var displayRequestedTester = new EventTester<Button, AccessKeyDisplayRequestedEventArgs>(button1, "AccessKeyDisplayRequested"))
                {
                    Log.Comment("Pressing ALT key to show KeyTips");
                    TestServices.KeyboardHelper.Alt();
                    displayRequestedTester.Wait();
                }

                KeyTipHelper.WaitUntilAnyKeyTips();
                TestServices.Utilities.VerifyUIElementTree("1");
            }
        }

        [TestMethod]
        [TestProperty("VelocityTestPass:OneCoreStrict", "Desktop")]
        [TestProperty("HasAssociatedMasterFile", "True")]
        [TestProperty("Hosting:Mode", "UAP")] // Test Failure: KeyTipTests.OverrideResources in WPF-hosting mode fails sometimes with incorrect offset of KeyTip
        public void OverrideResources()
        {
            XamlRoot xamlRoot = null;
            UIExecutor.Execute(() =>
            {
                xamlRoot = TestServices.WindowHelper.WindowContent.XamlRoot;
            });

            using (new KeyTipHelper(xamlRoot))
            using (new OverrideAppResources(@"
                <ResourceDictionary xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                    <x:Double x:Key='KeyTipContentThemeFontSize'>18</x:Double>
                    <Thickness x:Key='KeyTipBorderThemeThickness'>1.2345</Thickness>
                    <Thickness x:Key='KeyTipThemePadding'>2.9876</Thickness>
                    <SolidColorBrush x:Key='KeyTipForeground' Color='#ff101010' />
                    <SolidColorBrush x:Key='KeyTipBackground' Color='#ddffff44' />
                    <SolidColorBrush x:Key='KeyTipBorderBrush' Color='#ffbbbb33' />
                    <FontFamily x:Name='KeyTipFontFamily'>Helvetica</FontFamily>
                </ResourceDictionary>"))
            {
                Page root;

                UIExecutor.Execute(() =>
                {
                    root = (Page)XamlMarkup.XamlReader.Load(@"
                    <Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <StackPanel>
                            <Button AccessKey='1' KeyTipPlacementMode='Center'>button 1</Button>
                            <Button AccessKey='ii' KeyTipPlacementMode='Center'>button 2</Button>
                            <Button AccessKey='WW' KeyTipPlacementMode='Right'>button 3</Button>
                        </StackPanel>
                    </Page>");

                    TestServices.WindowHelper.WindowContent = root;
                });
                TestServices.WindowHelper.WaitForIdle();
                AccessKeyHelper.TryMovingFocusToXamlForInit();

                using (var eventHelper = EventTester<object,object>.FromStaticEvent<AccessKeyManager>("IsDisplayModeEnabledChanged"))
                {
                    Log.Comment("Pressing ALT key to show KeyTips");
                    TestServices.KeyboardHelper.Alt();
                    eventHelper.Wait();
                }

                KeyTipHelper.WaitUntilAnyKeyTips();
                TestServices.Utilities.VerifyUIElementTree();
            }
        }

        [TestMethod]
        [TestProperty("VelocityTestPass:OneCoreStrict", "Desktop")]
        [TestProperty("HasAssociatedMasterFile", "True")]
        public void AutopositioningKeytipsForColumn()
        {
            XamlRoot xamlRoot = null;
            UIExecutor.Execute(() =>
            {
                xamlRoot = TestServices.WindowHelper.WindowContent.XamlRoot;
            });

            using (new KeyTipHelper(xamlRoot))
            {
                Page root;

                UIExecutor.Execute(() =>
                {
                    root = (Page)XamlMarkup.XamlReader.Load(@"
                    <Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                        <StackPanel Margin='100'>
                            <Button AccessKey='1'>button 1</Button>
                            <Button AccessKey='2'>button 2</Button>
                            <Button AccessKey='3'>button 3</Button>
                            <Button AccessKey='4'>button 4</Button>
                        </StackPanel>
                    </Page>");

                    TestServices.WindowHelper.WindowContent = root;
                });
                TestServices.WindowHelper.WaitForIdle();
                AccessKeyHelper.TryMovingFocusToXamlForInit();


                Log.Comment("Pressing ALT key to show KeyTips");
                TestServices.KeyboardHelper.Alt();

                KeyTipHelper.WaitUntilAnyKeyTips();
                TestServices.Utilities.VerifyUIElementTree();
            }
        }

        [TestMethod]
        [TestProperty("VelocityTestPass:OneCoreStrict", "Desktop")]
        [TestProperty("HasAssociatedMasterFile", "True")]
        [TestProperty("Hosting:Mode", "UAP")]  // crash reason: crash at the end of test

        public void AutopositioningKeytipsForColumnWithPlateauScaling()
        {
            XamlRoot xamlRoot = null;
            UIExecutor.Execute(() =>
            {
                xamlRoot = TestServices.WindowHelper.WindowContent.XamlRoot;
            });

            //Setting scale factor to 2
            using (new KeyTipHelper(xamlRoot, 2.0f))
            {
                Page root;

                UIExecutor.Execute(() =>
                {
                    root = (Page)XamlMarkup.XamlReader.Load(@"
                    <Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                        <StackPanel Margin='50'>
                            <Button AccessKey='1'>button 1</Button>
                            <Button AccessKey='2'>button 2</Button>
                            <Button AccessKey='3'>button 3</Button>
                            <Button AccessKey='4'>button 4</Button>
                        </StackPanel>
                    </Page>");

                    TestServices.WindowHelper.WindowContent = root;
                });
                TestServices.WindowHelper.WaitForIdle();
                AccessKeyHelper.TryMovingFocusToXamlForInit();

                Log.Comment("Pressing ALT key to show KeyTips");
                TestServices.KeyboardHelper.Alt();
                KeyTipHelper.WaitUntilAnyKeyTips();
                TestServices.Utilities.VerifyUIElementTree();
            }
        }

        [TestMethod]
        [TestProperty("VelocityTestPass:OneCoreStrict", "Desktop")]
        [TestProperty("HasAssociatedMasterFile", "True")]
        public void AutopositioningKeytipsHorizontal()
        {
            XamlRoot xamlRoot = null;
            UIExecutor.Execute(() =>
            {
                xamlRoot = TestServices.WindowHelper.WindowContent.XamlRoot;
            });

            using (new KeyTipHelper(xamlRoot))
            {
                Page root;

                UIExecutor.Execute(() =>
                {
                    root = (Page)XamlMarkup.XamlReader.Load(@"
                    <Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                        <StackPanel Orientation='Horizontal'>
                            <Button Name = 'b1' AccessKey = 'H1' Content = 'Button 1' Margin = '0,0,50,0'/>
                            <Button Name = 'b2' AccessKey = 'H2' Content = 'Button 2' Margin = '0,0,50,0'/>
                            <Button Name = 'b3' AccessKey = 'H3' Content = 'Button 3' Margin = '0,0,50,0' />
                        </StackPanel >
                    </Page>");

                    TestServices.WindowHelper.WindowContent = root;
                });
                TestServices.WindowHelper.WaitForIdle();
                AccessKeyHelper.TryMovingFocusToXamlForInit();


                Log.Comment("Pressing ALT key to show KeyTips");
                TestServices.KeyboardHelper.Alt();

                KeyTipHelper.WaitUntilAnyKeyTips();
                TestServices.Utilities.VerifyUIElementTree();
            }
        }

        [TestMethod]
        [TestProperty("VelocityTestPass:OneCoreStrict", "Desktop")]
        [TestProperty("HasAssociatedMasterFile", "True")]
        public void KeyTipsAutopositioningAvoidsFocusableElements()
        {
            XamlRoot xamlRoot = null;
            UIExecutor.Execute(() =>
            {
                xamlRoot = TestServices.WindowHelper.WindowContent.XamlRoot;
            });

            using (new KeyTipHelper(xamlRoot))
            {
                Page root;

                UIExecutor.Execute(() =>
                {
                    root = (Page)XamlMarkup.XamlReader.Load(@"
                    <Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                        <StackPanel Orientation='Vertical'>
                            <Button Name = 'b1' AccessKey = 'A' Content = 'Button 1' />
                            <Button Name = 'b2' Content = 'Button 2' />
                        </StackPanel >
                    </Page>");

                    TestServices.WindowHelper.WindowContent = root;
                });
                TestServices.WindowHelper.WaitForIdle();
                AccessKeyHelper.TryMovingFocusToXamlForInit();

                Log.Comment("Pressing ALT key to show KeyTips");
                TestServices.KeyboardHelper.Alt();

                KeyTipHelper.WaitUntilAnyKeyTips();
                TestServices.Utilities.VerifyUIElementTree();
            }
        }

        [TestMethod]
        [TestProperty("VelocityTestPass:OneCoreStrict", "Desktop")]
        [TestProperty("Ignore", "TRUE")] // Move windowed popups to lifted input
        [TestProperty("HasAssociatedMasterFile", "True")]
        public void KeyTipsAutopositioningAvoidsFocusableElementsWhenPlateauScaling()
        {
            XamlRoot xamlRoot = null;
            UIExecutor.Execute(() =>
            {
                xamlRoot = TestServices.WindowHelper.WindowContent.XamlRoot;
            });

            //Setting scale factor to 2
            using (new KeyTipHelper(xamlRoot, 2.0f))
            {
                Page root;

                UIExecutor.Execute(() =>
                {
                    root = (Page)XamlMarkup.XamlReader.Load(@"
                    <Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                        <StackPanel Orientation='Vertical'>
                            <Button Name = 'b1' AccessKey = 'A' Content = 'Button 1' />
                            <Button Name = 'b2' Content = 'Button 2' />
                        </StackPanel >
                    </Page>");

                    TestServices.WindowHelper.WindowContent = root;
                });
                TestServices.WindowHelper.WaitForIdle();
                AccessKeyHelper.TryMovingFocusToXamlForInit();

                Log.Comment("Pressing ALT key to show KeyTips");
                TestServices.KeyboardHelper.Alt();


                KeyTipHelper.WaitUntilAnyKeyTips();
                TestServices.Utilities.VerifyUIElementTree();
            }
        }

        [TestMethod]
        [TestProperty("TestPass:ExcludeOn", "WindowsCore")]
        [TestProperty("Ignore", "TRUE")] // Move windowed popups to lifted input
        [TestProperty("HasAssociatedMasterFile", "True")]
        public void KeyTipsAutopositioningAvoidsFocusableElementsInPopups()
        {
            ComboBox cb = null;

            XamlRoot xamlRoot = null;
            UIExecutor.Execute(() =>
            {
                xamlRoot = TestServices.WindowHelper.WindowContent.XamlRoot;
            });

            using (new KeyTipHelper(xamlRoot))
            {
                Page root;

                UIExecutor.Execute(() =>
                {
                    root = (Page)XamlMarkup.XamlReader.Load(@"
                     <Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                         <StackPanel Orientation='Vertical' Margin='50,50,50,50'>
                             <ComboBox Name = 'cb'>
                                 <ComboBoxItem>Item1</ComboBoxItem>
                                 <ComboBoxItem>Item2</ComboBoxItem>
                                 <ComboBoxItem>Item3</ComboBoxItem>
                                 <ComboBoxItem>Item4</ComboBoxItem>
                                 <ComboBoxItem>Item5</ComboBoxItem>
                             </ComboBox>
                             <Button Name = 'b1' AccessKey = 'A' Content = 'Button 1' />
                         </StackPanel >
                     </Page>");
                    cb = (ComboBox)root.FindName("cb");

                    TestServices.WindowHelper.WindowContent = root;
                });
                TestServices.WindowHelper.WaitForIdle();
                AccessKeyHelper.TryMovingFocusToXamlForInit();

                UIExecutor.Execute(() =>
                {
                    cb.IsDropDownOpen = true;
                });

                Log.Comment("Pressing ALT key to show KeyTips");
                TestServices.KeyboardHelper.Alt();

                KeyTipHelper.WaitUntilAnyKeyTips();

                if (TestServices.Utilities.IsDesktop)
                {
                    TestServices.Utilities.VerifyUIElementTree("Windowed");
                }
                else
                {
                    TestServices.Utilities.VerifyUIElementTree("Unwindowed");
                }
            }
        }

        [TestMethod]
        [TestProperty("TestPass:ExcludeOn", "WindowsCore")]
        [TestProperty("Ignore", "TRUE")]    // [DCPPTest] Xaml tests are failing because Xaml no longer applies the plateau scale
        [TestProperty("HasAssociatedMasterFile", "True")]
        public void KeyTipsAutopositioningAvoidsFocusableElementsInPopupsWhenPlateauScaling()
        {
            ComboBox cb = null;

            XamlRoot xamlRoot = null;
            UIExecutor.Execute(() =>
            {
                xamlRoot = TestServices.WindowHelper.WindowContent.XamlRoot;
            });

            //Setting scale factor to 1.5
            using (new KeyTipHelper(xamlRoot, 1.5f))
            {
                Page root;

                UIExecutor.Execute(() =>
                {
                    root = (Page)XamlMarkup.XamlReader.Load(@"
                     <Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                         <StackPanel Orientation='Vertical' Margin='50,50,50,50'>
                             <ComboBox Name = 'cb'>
                                 <ComboBoxItem>Item1</ComboBoxItem>
                                 <ComboBoxItem>Item2</ComboBoxItem>
                                 <ComboBoxItem>Item3</ComboBoxItem>
                                 <ComboBoxItem>Item4</ComboBoxItem>
                                 <ComboBoxItem>Item5</ComboBoxItem>
                             </ComboBox>
                             <Button Name = 'b1' AccessKey = 'A' Content = 'Button 1' />
                         </StackPanel >
                     </Page>");
                    cb = (ComboBox)root.FindName("cb");

                    TestServices.WindowHelper.WindowContent = root;
                });
                TestServices.WindowHelper.WaitForIdle();
                AccessKeyHelper.TryMovingFocusToXamlForInit();

                UIExecutor.Execute(() =>
                {
                    cb.IsDropDownOpen = true;
                });

                Log.Comment("Pressing ALT key to show KeyTips");
                TestServices.KeyboardHelper.Alt();

                KeyTipHelper.WaitUntilAnyKeyTips();

                if (TestServices.Utilities.IsDesktop)
                {
                    TestServices.Utilities.VerifyUIElementTree("Windowed");
                }
                else
                {
                    TestServices.Utilities.VerifyUIElementTree("Unwindowed");
                }
            }
        }

        [TestMethod]
        [TestProperty("VelocityTestPass:OneCoreStrict", "Desktop")]
        [TestProperty("HasAssociatedMasterFile", "True")]
        public void KeyTipsAutopositioningAvoidsFocusableHyperlinks()
        {
            XamlRoot xamlRoot = null;
            UIExecutor.Execute(() =>
            {
                xamlRoot = TestServices.WindowHelper.WindowContent.XamlRoot;
            });

            using (new KeyTipHelper(xamlRoot))
            {
                Page root;

                UIExecutor.Execute(() =>
                {
                    root = (Page)XamlMarkup.XamlReader.Load(@"
                    <Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                        <StackPanel VerticalAlignment='Center'>
                            <Button Name = 'b1' AccessKey = 'A' Content = 'Button 1'  Height='200' Width='200' />
                            <TextBlock Height='200' Width='200'>
                                <Hyperlink Name='Hyperlink1'>hyperlink</Hyperlink>
                            </TextBlock>
                        </StackPanel >
                    </Page>");
                    TestServices.WindowHelper.WindowContent = root;
                });
                TestServices.WindowHelper.WaitForIdle();
                AccessKeyHelper.TryMovingFocusToXamlForInit();

                Log.Comment("Pressing ALT key to show KeyTips");
                TestServices.KeyboardHelper.Alt();

                KeyTipHelper.WaitUntilAnyKeyTips();
                TestServices.Utilities.VerifyUIElementTree();
            }
        }

        [TestMethod]
        [TestProperty("VelocityTestPass:OneCoreStrict", "Desktop")]
        [TestProperty("HasAssociatedMasterFile", "True")]
        [TestProperty("Hosting:Mode", "UAP")]  // crash reason: crashes at the end
        public void KeyTipsAutopositioningAvoidsFocusableHyperlinksWhenPlateauScaling()
        {
            XamlRoot xamlRoot = null;
            UIExecutor.Execute(() =>
            {
                xamlRoot = TestServices.WindowHelper.WindowContent.XamlRoot;
            });

            //Setting scale factor to 1.25x
            using (new KeyTipHelper(xamlRoot, 1.25f))
            {
                Page root;

                UIExecutor.Execute(() =>
                {
                    root = (Page)XamlMarkup.XamlReader.Load(@"
                    <Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                        <StackPanel VerticalAlignment='Center'>
                            <Button Name = 'b1' AccessKey = 'A' Content = 'Button 1'  Height='200' Width='200' />
                            <TextBlock Height='200' Width='200'>
                                <Hyperlink Name='Hyperlink1'>hyperlink</Hyperlink>
                            </TextBlock>
                        </StackPanel >
                    </Page>");
                    TestServices.WindowHelper.WindowContent = root;
                });
                TestServices.WindowHelper.WaitForIdle();
                AccessKeyHelper.TryMovingFocusToXamlForInit();

                Log.Comment("Pressing ALT key to show KeyTips");
                TestServices.KeyboardHelper.Alt();

                KeyTipHelper.WaitUntilAnyKeyTips();
                TestServices.Utilities.VerifyUIElementTree();
            }
        }

        [TestMethod]
        [TestProperty("VelocityTestPass:OneCoreStrict", "Desktop")]
        [TestProperty("HasAssociatedMasterFile", "True")]
        public void KeyTipsAutopositioningDoesNotAvoidCollapsedElements()
        {
            XamlRoot xamlRoot = null;
            UIExecutor.Execute(() =>
            {
                xamlRoot = TestServices.WindowHelper.WindowContent.XamlRoot;
            });

            using (new KeyTipHelper(xamlRoot))
            {
                Page root;

                UIExecutor.Execute(() =>
                {
                    root = (Page)XamlMarkup.XamlReader.Load(@"
                    <Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                        <StackPanel Orientation='Vertical'>
                            <Button Name = 'b1' AccessKey = 'A' Content = 'Button 1' />
                            <Button Name = 'b2' Content = 'Button 2' Visibility='Collapsed'/>
                        </StackPanel >
                    </Page>");

                    TestServices.WindowHelper.WindowContent = root;
                });
                TestServices.WindowHelper.WaitForIdle();
                AccessKeyHelper.TryMovingFocusToXamlForInit();

                Log.Comment("Pressing ALT key to show KeyTips");
                TestServices.KeyboardHelper.Alt();

                KeyTipHelper.WaitUntilAnyKeyTips();
                TestServices.Utilities.VerifyUIElementTree();
            }
        }

        [TestMethod]
        [TestProperty("VelocityTestPass:OneCoreStrict", "Desktop")]
        [TestProperty("HasAssociatedMasterFile", "True")]
        public void FilteringDoesNotMoveKeyTips()
        {
            XamlRoot xamlRoot = null;
            UIExecutor.Execute(() =>
            {
                xamlRoot = TestServices.WindowHelper.WindowContent.XamlRoot;
            });

            using (new KeyTipHelper(xamlRoot))
            {
                Page page;
                Button button = null;

                UIExecutor.Execute(() =>
                {
                    page = (Page)XamlMarkup.XamlReader.Load(@"
                    <Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <StackPanel>
                            <Button AccessKey='ABC' KeyTipPlacementMode='Right'>Button 1</Button>
                            <Button AccessKey='ADE' x:Name='button'>Button 2</Button>
                        </StackPanel>
                    </Page>");
                    button = (Button)page.FindName("button");
                    TestServices.WindowHelper.WindowContent = page;
                });
                TestServices.WindowHelper.WaitForIdle();
                AccessKeyHelper.TryMovingFocusToXamlForInit();

                using (var requestedEventTester = new EventTester<Button, AccessKeyDisplayRequestedEventArgs>(button, "AccessKeyDisplayRequested"))
                {
                    Log.Comment("Pressing ALT key to show KeyTips");
                    TestServices.KeyboardHelper.Alt();
                    requestedEventTester.Wait();
                }

                using (var requestedEventTester = new EventTester<Button, AccessKeyDisplayRequestedEventArgs>(button, "AccessKeyDisplayRequested"))
                {
                    TestServices.KeyboardHelper.PressKeySequence("a");
                    requestedEventTester.Wait();
                    KeyTipHelper.WaitUntilKeyTips("BC DE");
                    TestServices.Utilities.VerifyUIElementTree("1");
                }

                using (var requestedEventTester = new EventTester<Button, AccessKeyDisplayRequestedEventArgs>(button, "AccessKeyDisplayRequested"))
                {
                    TestServices.KeyboardHelper.PressKeySequence("d");
                    requestedEventTester.Wait();
                    KeyTipHelper.WaitUntilKeyTips("E");
                    TestServices.Utilities.VerifyUIElementTree("2");
                }

                using (var dismissedEventTester = new EventTester<Button, AccessKeyDisplayDismissedEventArgs>(button, "AccessKeyDisplayDismissed"))
                {
                    TestServices.KeyboardHelper.PressKeySequence("e");
                    dismissedEventTester.Wait();
                }
            }
        }

        [TestMethod]
        [TestProperty("Description", "When a button has a template that contains a TextBlock, and that TextBlock sets Control.IsTemplateKeyTipTarget to true, the KeyTip placement should be relative to that TextBlock")]
        [TestProperty("VelocityTestPass:OneCoreStrict", "Desktop")]
        [TestProperty("HasAssociatedMasterFile", "True")]
        public void TextBlockKeyTipTarget()
        {
            XamlRoot xamlRoot = null;
            UIExecutor.Execute(() =>
            {
                xamlRoot = TestServices.WindowHelper.WindowContent.XamlRoot;
            });

            using (new KeyTipHelper(xamlRoot))
            {
                Page root;

                UIExecutor.Execute(() =>
                {
                    root = (Page)XamlMarkup.XamlReader.Load(@"
                    <Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                        <StackPanel Padding='20'>
                            <Button AccessKey='1'>button 1</Button>
                            <Button AccessKey='2' KeyTipPlacementMode='Left'>
                                <Button.Template>
                                    <ControlTemplate>
                                        <Border BorderThickness='2' BorderBrush='White'>
                                            <StackPanel>
                                                <TextBlock Text='Text inside button' />
                                                <TextBlock Control.IsTemplateKeyTipTarget='True' Text='KeyTipTarget text' />
                                            </StackPanel>
                                        </Border>
                                    </ControlTemplate>
                                </Button.Template>
                            </Button>
                            <Button AccessKey='3'>button 3</Button>
                        </StackPanel >
                    </Page>");

                    TestServices.WindowHelper.WindowContent = root;
                });
                TestServices.WindowHelper.WaitForIdle();
                AccessKeyHelper.TryMovingFocusToXamlForInit();

                Log.Comment("Pressing ALT key to show KeyTips");
                TestServices.KeyboardHelper.Alt();

                KeyTipHelper.WaitUntilAnyKeyTips();
                TestServices.Utilities.VerifyUIElementTree();
            }
        }

        [TestMethod]
        [TestProperty("Description", "When a button has a template that contains a text Run, and that Run sets Control.IsTemplateKeyTipTarget to true, KeyTip placement is unchanged (for now)")]
        [TestProperty("VelocityTestPass:OneCoreStrict", "Desktop")]
        [TestProperty("HasAssociatedMasterFile", "True")]
        public void TextRunKeyTipTarget()
        {
            XamlRoot xamlRoot = null;
            UIExecutor.Execute(() =>
            {
                xamlRoot = TestServices.WindowHelper.WindowContent.XamlRoot;
            });

            using (new KeyTipHelper(xamlRoot))
            {
                Page root;

                UIExecutor.Execute(() =>
                {
                    root = (Page)XamlMarkup.XamlReader.Load(@"
                    <Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                        <StackPanel Padding='20'>
                            <Button AccessKey='r' KeyTipPlacementMode='Center'>
                                <Button.Template>
                                    <ControlTemplate>
                                        <Border>
                                            <StackPanel>
                                                <TextBlock Text='Text inside button' />
                                                <TextBlock Text='Text inside button' />
                                                <TextBlock Text='Text inside button' />
                                                <TextBlock>
                                <Run Control.IsTemplateKeyTipTarget='True'  xml:space='preserve'>This is a really long run
that spans multiple lines. BlahBlahBlahBlah</Run>
                                                </TextBlock>
                                            </StackPanel>
                                        </Border>
                                    </ControlTemplate>
                                </Button.Template>
                            </Button>
                        </StackPanel >
                    </Page>");

                    TestServices.WindowHelper.WindowContent = root;
                });
                TestServices.WindowHelper.WaitForIdle();
                AccessKeyHelper.TryMovingFocusToXamlForInit();

                Log.Comment("Pressing ALT key to show KeyTips");
                TestServices.KeyboardHelper.Alt();
                KeyTipHelper.WaitUntilAnyKeyTips();

                TestServices.Utilities.VerifyUIElementTree();
            }
        }

        [TestMethod]
        [TestProperty("Description", "When Control.IsTemplateKeyTipTarget==true outside of a template it doesn't do anything")]
        [TestProperty("VelocityTestPass:OneCoreStrict", "Desktop")]
        [TestProperty("HasAssociatedMasterFile", "True")]
        public void KeyTipTargetOnlyHonoredInTemplate()
        {
            XamlRoot xamlRoot = null;
            UIExecutor.Execute(() =>
            {
                xamlRoot = TestServices.WindowHelper.WindowContent.XamlRoot;
            });

            using (new KeyTipHelper(xamlRoot))
            {
                Page root;
                UIExecutor.Execute(() =>
                {
                    root = (Page)XamlMarkup.XamlReader.Load(@"
                    <Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                        <StackPanel Padding='20' AccessKey='z' KeyTipPlacementMode='Center'>
                            <Button>button 1</Button>
                            <TextBlock>arbitrary text</TextBlock>
                            <TextBlock>arbitrary text</TextBlock>
                            <TextBlock Control.IsTemplateKeyTipTarget='True'>TextBlock with IsTemplateKeyTipTarget</TextBlock>
                        </StackPanel >
                    </Page>");

                    TestServices.WindowHelper.WindowContent = root;
                });
                TestServices.WindowHelper.WaitForIdle();
                AccessKeyHelper.TryMovingFocusToXamlForInit();

                Log.Comment("Pressing ALT key to show KeyTips");
                TestServices.KeyboardHelper.Alt();
                KeyTipHelper.WaitUntilAnyKeyTips();

                TestServices.Utilities.VerifyUIElementTree();
            }
        }

        [TestMethod]
        [TestProperty("Description", "When multiple objects set Control.IsTemplateKeyTipTarget==true, honor the last one")]
        [TestProperty("VelocityTestPass:OneCoreStrict", "Desktop")]
        [TestProperty("HasAssociatedMasterFile", "True")]
        public void ChooseLastOfMultipleKeyTipTargets()
        {
            XamlRoot xamlRoot = null;
            UIExecutor.Execute(() =>
            {
                xamlRoot = TestServices.WindowHelper.WindowContent.XamlRoot;
            });

            using (new KeyTipHelper(xamlRoot))
            {
                Page root;

                UIExecutor.Execute(() =>
                {
                    root = (Page)XamlMarkup.XamlReader.Load(@"
                    <Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                        <StackPanel Padding='20'>
                            <Button AccessKey='r' KeyTipPlacementMode='Center'>
                                <Button.Template>
                                    <ControlTemplate>
                                        <Border>
                                            <StackPanel>
                                                <TextBlock Text='arbitrary text' />
                                                <TextBlock Control.IsTemplateKeyTipTarget='True' Text='TextBlock that is a target' />
                                                <TextBlock Text='arbitrary text' />
                                                <TextBlock Control.IsTemplateKeyTipTarget='True' Text='TextBlock that is a target' />
                                                <TextBlock Text='arbitrary text' />
                                            </StackPanel>
                                        </Border>
                                    </ControlTemplate>
                                </Button.Template>
                            </Button>
                        </StackPanel >
                    </Page>");

                    TestServices.WindowHelper.WindowContent = root;
                });
                TestServices.WindowHelper.WaitForIdle();
                AccessKeyHelper.TryMovingFocusToXamlForInit();

                Log.Comment("Pressing ALT key to show KeyTips");
                TestServices.KeyboardHelper.Alt();
                KeyTipHelper.WaitUntilAnyKeyTips();

                TestServices.Utilities.VerifyUIElementTree();
            }
        }

        [TestMethod]
        [TestProperty("Description", "A KeyTipTarget can be removed by setting IsTemplateKeyTipTarget to false")]
        [TestProperty("VelocityTestPass:OneCoreStrict", "Desktop")]
        [TestProperty("HasAssociatedMasterFile", "True")]
        [TestProperty("Ignore", "True")] // Unreliable tests in WPF netcore 5: KeyTips tests
        public void KeyTipTargetElementSetsIsTemplateKeyTipTargetToFalse()
        {
            XamlRoot xamlRoot = null;
            UIExecutor.Execute(() =>
            {
                xamlRoot = TestServices.WindowHelper.WindowContent.XamlRoot;
            });

            using (new KeyTipHelper(xamlRoot))
            {
                Page root;
                StackPanel stackPanel;
                MyControl myControl = null;

                UIExecutor.Execute(() =>
                {
                    root = (Page)XamlMarkup.XamlReader.Load(@"
                    <Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <StackPanel x:Name='stackPanel' Padding='20'>
                            <Button AccessKey='1' KeyTipPlacementMode='Left'>button 1</Button>
                            <Button AccessKey='3' KeyTipPlacementMode='Left'>button 3</Button>
                        </StackPanel>
                    </Page>");

                    myControl = new MyControl();
                    myControl.Template = (ControlTemplate)XamlMarkup.XamlReader.Load(
                        @"<ControlTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <Border BorderThickness='2' BorderBrush='White'>
                                <StackPanel>
                                    <TextBlock Text='Text inside control' />
                                    <TextBlock Text='Text inside control' />
                                    <TextBlock Text='Text inside control' />
                                    <TextBlock x:Name='textBlockTarget' Control.IsTemplateKeyTipTarget='True' Text='KeyTipTarget text' />
                                </StackPanel>
                            </Border>
                        </ControlTemplate>");

                    stackPanel = (StackPanel)root.FindName("stackPanel");
                    stackPanel.Children.Insert(1, myControl);

                    myControl.AccessKey = "2";
                    myControl.KeyTipPlacementMode = KeyTipPlacementMode.Center;

                    TestServices.WindowHelper.WindowContent = root;
                });
                TestServices.WindowHelper.WaitForIdle();
                AccessKeyHelper.TryMovingFocusToXamlForInit();

                UIExecutor.Execute(() =>
                {
                    ((TextBlock)myControl.GetChildFromTemplate("textBlockTarget")).SetValue(Control.IsTemplateKeyTipTargetProperty, false);
                });
                TestServices.WindowHelper.WaitForIdle();

                Log.Comment("Pressing ALT key to show KeyTips");
                TestServices.KeyboardHelper.Alt();
                KeyTipHelper.WaitUntilAnyKeyTips();

                TestServices.Utilities.VerifyUIElementTree();
            }
        }

        [TestMethod]
        [TestProperty("Description", "A CommandBar's KeyTip is drawn for its 'more' button")]
        [TestProperty("TestPass:ExcludeOn", "WindowsCore")]
        [TestProperty("VelocityTestPass:OneCoreStrict", "Desktop")]
        [TestProperty("HasAssociatedMasterFile", "True")]
        [TestProperty("Hosting:Mode", "UAP")]  // crash reason: focus doesn't move
        public void CommandBarKeyTipIsDrawnForMoreButton()
        {
            Log.Comment("Press ALT for a CommandBar that does not have an overflow button");
            XamlRoot xamlRoot = null;
            UIExecutor.Execute(() =>
            {
                xamlRoot = TestServices.WindowHelper.WindowContent.XamlRoot;
            });

            using (new KeyTipHelper(xamlRoot))
            {
                UIExecutor.Execute(() =>
                {
                    TestServices.WindowHelper.WindowContent = (Page)XamlMarkup.XamlReader.Load(@"
                    <Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <Grid>
                            <CommandBar x:Name='commandBar' AccessKey='c' KeyTipPlacementMode='Center' OverflowButtonVisibility='Collapsed'>
                                <AppBarButton AccessKey='a'>A</AppBarButton>
                            </CommandBar>
                        </Grid>
                    </Page>");
                });
                TestServices.WindowHelper.WaitForIdle();
                AccessKeyHelper.TryMovingFocusToXamlForInit();

                Log.Comment("Pressing ALT key to show KeyTips");
                TestServices.KeyboardHelper.Alt();
                KeyTipHelper.WaitUntilAnyKeyTips();
                TestServices.Utilities.VerifyUIElementTree("1");
            }

            Log.Comment("Press ALT for a CommandBar that has an overflow button");

            using (new KeyTipHelper(xamlRoot))
            {
                UIExecutor.Execute(() =>
                {
                    TestServices.WindowHelper.WindowContent = (Page)XamlMarkup.XamlReader.Load(@"
                    <Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <Grid>
                            <CommandBar x:Name='commandBar' AccessKey='c' KeyTipPlacementMode='Center' OverflowButtonVisibility='Visible'>
                                <AppBarButton AccessKey='a'>A</AppBarButton>
                            </CommandBar>
                        </Grid>
                    </Page>");
                });
                TestServices.WindowHelper.WaitForIdle();
                AccessKeyHelper.TryMovingFocusToXamlForInit();

                Log.Comment("Pressing ALT key to show KeyTips");
                TestServices.KeyboardHelper.Alt();
                KeyTipHelper.WaitUntilAnyKeyTips();
                TestServices.Utilities.VerifyUIElementTree("2");
            }
        }

        [TestMethod]
        [TestProperty("Description", "A CommandBar's KeyTip is drawn for its secondary menu items")]
        [TestProperty("TestPass:ExcludeOn", "WindowsCore")]
        [TestProperty("Ignore", "TRUE")] // Move windowed popups to lifted input
        [TestProperty("HasAssociatedMasterFile", "True")]
        public void VerifyCommandBarKeyTipsDrawnOnSecondaryMenuItems()
        {
            XamlRoot xamlRoot = null;
            UIExecutor.Execute(() =>
            {
                xamlRoot = TestServices.WindowHelper.WindowContent.XamlRoot;
            });

            using (new KeyTipHelper(xamlRoot))
            {
                UIExecutor.Execute(() =>
                {
                    TestServices.WindowHelper.WindowContent = (Page)XamlMarkup.XamlReader.Load(@"
                    <Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <Grid>
                            <CommandBar x:Name='MainCommandBar' AccessKey='m'>
                                <AppBarButton AccessKey='Q' Icon='Globe' Label='Go'/>
                                <AppBarButton AccessKey='S' Icon='Stop' Label='Stop'/>
                                <AppBarSeparator/>
                                <AppBarButton AccessKey='R' Icon='Refresh' Label='Refresh' IsAccessKeyScope='True'>
                                    <AppBarButton.Flyout>
                                        <MenuFlyout>
                                            <MenuFlyoutItem AccessKey='A' Text='Refresh A'/>
                                            <MenuFlyoutItem AccessKey='B' Text='Refresh B'/>
                                            <MenuFlyoutItem AccessKey='C'  Text='Refresh C'/>
                                            <MenuFlyoutItem AccessKey='D' Text='Refresh D'/>
                                        </MenuFlyout>
                                    </AppBarButton.Flyout>
                                </AppBarButton>
                                <AppBarButton AccessKey='B' Icon='Back' Label='Back'/>
                                <AppBarButton AccessKey='F' Icon='Forward' Label='Forward'/>
                                <AppBarSeparator/>
                                <AppBarToggleButton AccessKey='V' Icon='Favorite' Label='Favorite'/>
                                <CommandBar.SecondaryCommands>
                                    <AppBarToggleButton Icon='Like' AccessKey='L' Label='Like'/>
                                    <AppBarButton Icon='Setting' AccessKey='W' Label='Settings'/>
                                </CommandBar.SecondaryCommands>
                            </CommandBar>
                        </Grid>
                    </Page>");
                });
                TestServices.WindowHelper.WaitForIdle();
                AccessKeyHelper.TryMovingFocusToXamlForInit();

                Log.Comment("Pressing ALT key to show KeyTips");
                TestServices.KeyboardHelper.Alt();
                KeyTipHelper.WaitUntilAnyKeyTips();

                if (TestServices.Utilities.IsDesktop)
                {
                    TestServices.Utilities.VerifyUIElementTree("1.Windowed");
                }
                else
                {
                    TestServices.Utilities.VerifyUIElementTree("1.Unwindowed");
                }

                Log.Comment("Pressing access key to show SecondaryCommand KeyTips");
                TestServices.KeyboardHelper.PressKeySequence("m");
                KeyTipHelper.WaitUntilAnyKeyTips();

                if (TestServices.Utilities.IsDesktop)
                {
                    TestServices.Utilities.VerifyUIElementTree("2.Windowed");
                }
                else
                {
                    TestServices.Utilities.VerifyUIElementTree("2.Unwindowed");
                }
            }
            TestServices.KeyboardHelper.Escape();
            TestServices.WindowHelper.WaitForIdle();
        }

        [TestMethod]
        [TestProperty("Description", "Verify the official commandbar secondary-command keytip workaround.")]
        [TestProperty("TestPass:ExcludeOn", "WindowsCore")]
        [TestProperty("Ignore", "TRUE")] // Move windowed popups to lifted input
        [TestProperty("HasAssociatedMasterFile", "True")]
        public void VerifyCommandBarKeyTipWorkaround()
        {
            XamlRoot xamlRoot = null;
            UIExecutor.Execute(() =>
            {
                xamlRoot = TestServices.WindowHelper.WindowContent.XamlRoot;
            });

            using (new KeyTipHelper(xamlRoot))
            {
                // <CommandBarHack x:Name='MainCommandBar' AccessKey='m'>
                //    <AppBarButton AccessKey='G' Label='Go'/>
                //    <AppBarButton AccessKey='S' Label='Stop'/>
                //    <AppBarToggleButton AccessKey='V' Label='Favorite'/>
                //    <CommandBar.SecondaryCommands>
                //        <AppBarToggleButton AccessKey='L' Label='Like'/>
                //        <AppBarButton AccessKey='W' Label='Settings'/>
                //    </CommandBar.SecondaryCommands>
                //</CommandBarHack>
                CommandBarHack commandBar = null;

                UIExecutor.Execute(() =>
                {
                    commandBar = new CommandBarHack() { Name = "MainCommandBar", AccessKey="m" };
                    commandBar.PrimaryCommands.Add(new AppBarButton() { AccessKey = "G", Label = "Go" });
                    commandBar.PrimaryCommands.Add(new AppBarButton() { AccessKey = "S", Label = "Stop" });
                    commandBar.PrimaryCommands.Add(new AppBarToggleButton() { AccessKey = "V", Label = "Favorite" });
                    commandBar.SecondaryCommands.Add(new AppBarToggleButton() { AccessKey = "L", Label = "Like" });
                    commandBar.SecondaryCommands.Add(new AppBarButton() { AccessKey = "W", Label = "Settings" });

                    TestServices.WindowHelper.WindowContent = commandBar;
                });
                TestServices.WindowHelper.WaitForIdle();
                AccessKeyHelper.TryMovingFocusToXamlForInit();

                Log.Comment("Pressing ALT key to show KeyTips");
                TestServices.KeyboardHelper.Alt();
                KeyTipHelper.WaitUntilAnyKeyTips();

                if (TestServices.Utilities.IsDesktop)
                {
                    TestServices.Utilities.VerifyUIElementTree("1.Windowed");
                }
                else
                {
                    TestServices.Utilities.VerifyUIElementTree("1.Unwindowed");
                }

                Log.Comment("Pressing access key to show SecondaryCommand KeyTips");
                TestServices.KeyboardHelper.PressKeySequence("m");
                KeyTipHelper.WaitUntilAnyKeyTips();

                if (TestServices.Utilities.IsDesktop)
                {
                    TestServices.Utilities.VerifyUIElementTree("2.Windowed");
                }
                else
                {
                    TestServices.Utilities.VerifyUIElementTree("2.Unwindowed");
                }
            }
            TestServices.KeyboardHelper.Escape();
            TestServices.WindowHelper.WaitForIdle();
        }

    }

    partial class MyControl : Control
    {
        public DependencyObject GetChildFromTemplate(string name)
        {
            return this.GetTemplateChild(name);
        }
    }

    // This is a sample we ship publicly to demonstrate how to support a CommandBar's SecondaryCommands
    // with access keys available once a parent command is invoked. As a result, we want to test
    // this sample exactly as is.
    public partial class CommandBarHack : CommandBar
    {
        CommandBarOverflowPresenter secondaryItemsControl;
        Popup overflowPopup;

        public CommandBarHack()
        {
            this.ExitDisplayModeOnAccessKeyInvoked = false;
            AccessKeyInvoked += OnAccessKeyInvoked;
        }

        protected override void OnApplyTemplate()
        {
            base.OnApplyTemplate();

            Button moreButton = GetTemplateChild("MoreButton") as Button;
            moreButton.SetValue(Control.IsTemplateKeyTipTargetProperty, true);
            moreButton.IsAccessKeyScope = true;

            // SecondaryItemsControl changes
            secondaryItemsControl = GetTemplateChild("SecondaryItemsControl") as CommandBarOverflowPresenter;
            secondaryItemsControl.AccessKeyScopeOwner = moreButton;

            overflowPopup = GetTemplateChild("OverflowPopup") as Popup;

        }
        private void OnAccessKeyInvoked(UIElement sender, AccessKeyInvokedEventArgs args)
        {
            if (overflowPopup != null)
            {
                overflowPopup.Opened += SecondaryMenuOpened;
            }
        }

        private void SecondaryMenuOpened(object sender, object e)
        {
            var item = secondaryItemsControl.Items[0];
            if (item != null && item is Control)
            {
                (item as Control).Focus(FocusState.Keyboard);
            }
            overflowPopup.Opened -= SecondaryMenuOpened;
        }
    }
}

