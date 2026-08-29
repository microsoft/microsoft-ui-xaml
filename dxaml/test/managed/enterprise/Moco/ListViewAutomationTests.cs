// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using Private.Infrastructure;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Threading;
using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using Windows.ApplicationModel.DataTransfer;
using Windows.Foundation;
using Microsoft.UI.Xaml.Automation.Peers;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Tests.Common;


namespace Microsoft.UI.Xaml.Tests.Controls.ListViewBase
{
    [TestClass]
    public class ListViewAutomationTests : XamlTestsBase
    {
        [ClassInitialize]
        [TestProperty("BinaryUnderTest", "Microsoft.UI.Xaml.dll")]
        [TestProperty("RunAs", "UAP")]
        [TestProperty("Hosting:Mode", "UAP")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("UAP:Praid", "XamlManagedTAEFTests")]
        [TestProperty("HelixWorkItemCreation", "CreateWorkItemPerTestClass")]
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
        public void ValidateGetAutomationControlType()
        {
            ValidateGetAutomationControlTypeImpl(AutomationControlType.Separator);
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")]
        public void ValidateSelectionPatternOnListView()
        {
            ListView list = null;

            UIExecutor.Execute(() =>
            {
                list = new ListView()
                {
                    SelectionMode = ListViewSelectionMode.None,
                    ShowsScrollingPlaceholders = false,
                    Height = 300,
                    Width = 300,
                    ItemsSource = Enumerable.Range(0, 10)
                };

                TestServices.WindowHelper.WindowContent = list;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                var peer = ListViewAutomationPeer.CreatePeerForElement(list);
                Verify.IsNull(peer.GetPattern(PatternInterface.Selection));
                list.SelectionMode = ListViewSelectionMode.Single;
                Verify.IsNotNull(peer.GetPattern(PatternInterface.Selection));
            });
        }

        private void ValidateGetAutomationControlTypeImpl(AutomationControlType expectedControlType)
        {
            ListView list = null;
            var listLoaded = new AutoResetEvent(false);

            UIExecutor.Execute(() =>
            {
                Log.Comment("Loading list.");
                list = new ListView();
                list.Items.Add(new MyListViewItem { Content = "Item #1" });
                list.Loaded += delegate { listLoaded.Set(); };
                TestServices.WindowHelper.WindowContent = list;
            });

            Verify.IsTrue(listLoaded.WaitOne(TimeSpan.FromSeconds(5)), "Could not load list.");
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                var peer = FrameworkElementAutomationPeer.CreatePeerForElement(list);
                Verify.AreEqual(expectedControlType, peer.GetChildren()[0].GetAutomationControlType());
            });
        }
    }

    public partial class MyListViewItem : ListViewItem
    {
        protected override AutomationPeer OnCreateAutomationPeer()
        {
            return new MyListViewItemAuotmationPeer(this);
        }
    }

    public partial class MyListViewItemAuotmationPeer : ListViewItemAutomationPeer
    {
        public MyListViewItemAuotmationPeer(ListViewItem owner) : base(owner) { }

        protected override AutomationControlType GetAutomationControlTypeCore()
        {
            return AutomationControlType.Separator;
        }
    }
}
