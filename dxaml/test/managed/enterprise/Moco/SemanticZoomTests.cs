// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using Private.Infrastructure;
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Tests.Common;

namespace Microsoft.UI.Xaml.Tests.Enterprise.Moco
{
    [TestClass]
    public class SemanticZoomTests : XamlTestsBase
    {
        static string TestDeploymentDir { get; set; }

        [ClassInitialize]
        [TestProperty("BinaryUnderTest", "Microsoft.UI.Xaml.dll")]
        [TestProperty("RunAs", "UAP")]
        [TestProperty("Hosting:Mode", "UAP")] // DCPP NoCoreWindow mode - SemanticZoom tests fail get stuck "Waiting for BuildTreeService to finish..."
        [TestProperty("Classification", "Integration")]
        [TestProperty("UAP:Praid", "XamlManagedTAEFTests")]
        [TestProperty("HelixWorkItemCreation", "CreateWorkItemPerTestClass")]
        public static void Setup(TestContext context)
        {
            AssemblySetup.CommonTestClassSetup();
            TestDeploymentDir = context.TestDeploymentDir;
        }

        [ClassCleanup]
        public void ClassCleanup()
        {
            base.CommonClassCleanup();
        }

        [TestMethod]
        public void ZoomWithKeyboard()
        {
            using (TestServices.KeyboardHelper.CreateKeyboardWaitKindGuard(KeyboardWaitKind.None))
            {
                SemanticZoom sezo = null;

                UIExecutor.Execute(() =>
                {
                    sezo = (SemanticZoom)XamlReader.Load(
                                @"<SemanticZoom xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                                    <SemanticZoom.ZoomedInView>
                                        <ListView>
                                            <TextBlock>Zoomed</TextBlock>
                                            <TextBlock>In</TextBlock>
                                        </ListView>
                                    </SemanticZoom.ZoomedInView>
                                    <SemanticZoom.ZoomedOutView>
                                        <ListView>
                                            <TextBlock>Zoomed</TextBlock>
                                            <TextBlock>Out</TextBlock>
                                        </ListView>
                                    </SemanticZoom.ZoomedOutView>
                                </SemanticZoom>");
                    TestServices.WindowHelper.WindowContent = sezo;
                });

                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    (sezo.ZoomedInView as ListViewBase).Focus(FocusState.Programmatic);
                    Verify.IsTrue(sezo.IsZoomedInViewActive, "ZoomedInView is active");
                });

                TestServices.WindowHelper.WaitForIdle();

                // ctrl -
                TestServices.KeyboardHelper.PressKeySequence("$d$_lctrl#$d$_-#$u$_-#$u$_lctrl");
                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    Verify.IsFalse(sezo.IsZoomedInViewActive, "ZoomedInView is false");
                });

                // ctrl +
                TestServices.KeyboardHelper.PressKeySequence("$d$_lctrl#$d$_+#$u$_+#$u$_lctrl");
                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    Verify.IsTrue(sezo.IsZoomedInViewActive, "ZoomedInView is active");
                });
            }
        }

        [TestMethod]
        public void TabNavigationInDifferentViews()
        {
            using (TestServices.KeyboardHelper.CreateKeyboardWaitKindGuard(KeyboardWaitKind.None))
            {
                SemanticZoom sezo = null;
                Button beforeSezoButton = null;
                UIExecutor.Execute(() =>
                {
                    sezo = (SemanticZoom)XamlReader.Load(
                                @"<SemanticZoom xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                                    <SemanticZoom.ZoomedInView>
                                        <ListView>
                                            <ListViewItem>ZoomedIn</ListViewItem>
                                        </ListView>
                                    </SemanticZoom.ZoomedInView>
                                    <SemanticZoom.ZoomedOutView>
                                        <ListView>
                                            <ListViewItem>ZoomedOut</ListViewItem>
                                        </ListView>
                                    </SemanticZoom.ZoomedOutView>
                                </SemanticZoom>");
                    StackPanel root = new StackPanel();
                    beforeSezoButton = new Button() { Content = "Before Sezo" };
                    root.Children.Add(beforeSezoButton);
                    root.Children.Add(sezo);
                    TestServices.WindowHelper.WindowContent = root;
                });

                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    beforeSezoButton.Focus(FocusState.Keyboard);
                    Verify.IsTrue(sezo.IsZoomedInViewActive, "verify in ZoomedInView");
                });

                TestServices.WindowHelper.WaitForIdle();
                TestServices.KeyboardHelper.Tab();
                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    var focused = (ListViewItem)FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot);
                    var focusedIndex = ((ListView)sezo.ZoomedInView).IndexFromContainer(focused);
                    Verify.AreEqual(0, focusedIndex);

                    sezo.IsZoomedInViewActive = false;
                    beforeSezoButton.Focus(FocusState.Keyboard);
                });

                TestServices.WindowHelper.WaitForIdle();
                TestServices.KeyboardHelper.Tab();
                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    var focused = (ListViewItem)FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot);
                    var focusedIndex = ((ListView)sezo.ZoomedOutView).IndexFromContainer(focused);
                    Verify.AreEqual(0, focusedIndex);

                    sezo.IsZoomedInViewActive = false;
                    beforeSezoButton.Focus(FocusState.Keyboard);
                });
            }
        }
    }
}
