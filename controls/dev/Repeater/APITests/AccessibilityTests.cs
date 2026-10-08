// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.RepeaterTests.Common;
using Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.RepeaterTests.Common.Mocks;
using MUXControlsTestApp.Utilities;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using Windows.Foundation;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Automation.Peers;
using Microsoft.UI.Xaml.Controls;
using Common;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.RepeaterTests
{
    [TestClass]
    public class AccessibilityTests : ApiTestBase
    {
        [TestMethod]
        public void ValidateChildrenPeers()
        {
            RunOnUIThread.Execute(() =>
            {
                var data = new ObservableCollection<string>(Enumerable.Range(0, 3).Select(i => string.Format("Item #{0}", i)));
                var dataSource = MockItemsSource.CreateDataSource(data, supportsUniqueIds: false);

                // StackPanel doesn't have an automation peer, but we still need
                // to report its children's peer.
                var group = new StackPanel();
                group.Children.Add(new ListViewItem());
                group.Children.Add(new ListViewItem());

                var mapping = new Dictionary<int, UIElement>
                {
                    { 0, new ListViewItem() },
                    { 1, new ListViewItem() },
                    { 2, group },
                };

                var elementFactory = MockElementFactory.CreateElementFactory(mapping);
                var layout = new MockVirtualizingLayout
                {
                    MeasureLayoutFunc = (availableSize, context) =>
                    {
                        var ctx = (VirtualizingLayoutContext)context;

                        // Realize elements 0, 1, 2 in a random order.
                        ctx.GetOrCreateElementAt(2);
                        ctx.GetOrCreateElementAt(0);
                        var element1 = ctx.GetOrCreateElementAt(1);

                        // Clear element 1 because we will be validating
                        // that we ignore unrealized elements.
                        ctx.RecycleElement(element1);

                        return default(Size);
                    }
                };

                var repeater = CreateRepeater(dataSource, elementFactory, layout);

                Content = repeater;
                repeater.UpdateLayout();

                var peer = FrameworkElementAutomationPeer.CreatePeerForElement(repeater);
                var children = peer.GetChildren().Select(p => ((FrameworkElementAutomationPeer)p).Owner).ToList();

                Verify.AreEqual(3, children.Count);
                Verify.AreEqual(mapping[0], children[0]);
                Verify.AreEqual(group.Children[0], children[1]);
                Verify.AreEqual(group.Children[1], children[2]);
            });
        }

        // Scenario: construct a RepeaterAutomationPeer with its public constructor for a repeater showing three Buttons.
        // Expected: the peer owns the repeater, reports control type Group, and returns one child peer per realized item in
        //           item order; the framework creates the same kind of peer for the repeater.
        // A failure means: screen readers would see the repeater's items missing, duplicated or out of order.
        [TestMethod]
        [TestProperty("Description", "Verifies a RepeaterAutomationPeer created with its public constructor reports Group and realized children in index order.")]
        public void VerifyRepeaterAutomationPeerCreatedDirectly()
        {
            RunOnUIThread.Execute(() =>
            {
                var repeater = new ItemsRepeater() {
                    ItemsSource = Enumerable.Range(0, 3).Select(i => "Item #" + i).ToList(),
                    ItemTemplate = (DataTemplate)Microsoft.UI.Xaml.Markup.XamlReader.Load(
                        "<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'><Button Content='{Binding}' Height='30'/></DataTemplate>"),
                };
                Content = repeater;
                Content.UpdateLayout();

                var peer = new RepeaterAutomationPeer(repeater);
                Verify.AreSame(repeater, peer.Owner);
                Verify.AreEqual(AutomationControlType.Group, peer.GetAutomationControlType());
                Log.Comment("ClassName: '" + peer.GetClassName() + "'");

                var children = peer.GetChildren().Select(p => ((FrameworkElementAutomationPeer)p).Owner).ToList();
                Verify.AreEqual(3, children.Count);
                for (int i = 0; i < 3; i++)
                {
                    Verify.AreSame(repeater.TryGetElement(i), children[i], $"Child peer {i} belongs to item {i}.");
                    Verify.AreEqual("Item #" + i, ((Button)children[i]).Content);
                }

                Verify.IsTrue(FrameworkElementAutomationPeer.CreatePeerForElement(repeater) is RepeaterAutomationPeer,
                    "The framework creates a RepeaterAutomationPeer for ItemsRepeater.");
            });
        }

        private ItemsRepeater CreateRepeater(object dataSource, object elementFactory, VirtualizingLayout layout = null)
        {
            var repeater = new ItemsRepeater
            {
                ItemsSource = dataSource,
                ItemTemplate = elementFactory,
            };
            repeater.Layout = layout ?? CreateLayout(repeater);
            return repeater;
        }

        private VirtualizingLayout CreateLayout(ItemsRepeater repeater)
        {
            var layout = new MockVirtualizingLayout();
            var children = new List<UIElement>();

            layout.MeasureLayoutFunc = (availableSize, context) =>
            {
                repeater.Tag = repeater.Tag ?? context;
                children.Clear();
                var itemCount = context.ItemCount;

                for (int i = 0; i < itemCount; ++i)
                {
                    var element = repeater.TryGetElement(i) ?? context.GetOrCreateElementAt(i);
                    element.Measure(availableSize);
                    children.Add(element);
                }

                return new Size(10, 10);
            };

            return layout;
        }
    }
}
