// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using MUXControlsTestApp.Utilities;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Runtime.InteropServices;
using Windows.Foundation;
using Microsoft.UI.Xaml.Automation.Peers;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Media;
using Common;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.RepeaterTests
{
    // Contract-level checks over the public surface of the Repeater types: every dependency property is read and written
    // through both its CLR property and its DependencyProperty identifier, and every Repeater runtime class reports its
    // WinRT runtime class name and refuses (or allows) plain activation the way its metadata declares.
    [TestClass]
    public class RepeaterApiContractTests : ApiTestBase
    {
        private const int E_NOTIMPL = unchecked((int)0x80004001);
        private static readonly Guid IID_IInspectable = new Guid("AF86E2E0-B12D-4C6A-9C5A-D7AA65101E90");

        // Product concern PC-LFL-TIMER: a LinedFlowLayout whose asynchronous measure timer was created crashes the process
        // when it is destroyed off the UI thread (for example by the .NET finalizer). LinedFlowLayouts that run a layout
        // pass in this class are kept alive for the lifetime of the test process, like in LinedFlowLayoutTests.
        private static readonly List<LinedFlowLayout> s_linedFlowLayoutsKeptAlive = new List<LinedFlowLayout>();

        // Scenario: read, set, SetValue and ClearValue every dependency property of ItemsRepeater.
        // Expected: each property starts at its documented default, reads back what was set through either API, and returns
        //           to the default when cleared.
        // A failure means: XAML markup, bindings or styles that use these properties would see wrong or stale values.
        [TestMethod]
        [TestProperty("Description", "Round-trips every ItemsRepeater dependency property through its CLR property, GetValue/SetValue and ClearValue.")]
        public void VerifyItemsRepeaterDependencyProperties()
        {
            RunOnUIThread.Execute(() =>
            {
                var repeater = new ItemsRepeater();
                var source1 = new List<string>() { "A" };
                var source2 = new List<string>() { "B" };
                VerifyDependencyProperty(repeater, "ItemsSource", ItemsRepeater.ItemsSourceProperty,
                    () => repeater.ItemsSource, v => repeater.ItemsSource = v, null, source1, source2);

                var layout1 = new StackLayout();
                var layout2 = new UniformGridLayout();
                VerifyDependencyProperty(repeater, "Layout", ItemsRepeater.LayoutProperty,
                    () => repeater.Layout, v => repeater.Layout = v, null, layout1, layout2);

                var provider1 = new ItemCollectionTransitionProvider();
                var provider2 = new LinedFlowLayoutItemCollectionTransitionProvider();
                VerifyDependencyProperty(repeater, "ItemTransitionProvider", ItemsRepeater.ItemTransitionProviderProperty,
                    () => repeater.ItemTransitionProvider, v => repeater.ItemTransitionProvider = v, null, provider1, provider2);

                VerifyDependencyProperty(repeater, "HorizontalCacheLength", ItemsRepeater.HorizontalCacheLengthProperty,
                    () => repeater.HorizontalCacheLength, v => repeater.HorizontalCacheLength = v, 2.0, 4.0, 0.0);
                VerifyDependencyProperty(repeater, "VerticalCacheLength", ItemsRepeater.VerticalCacheLengthProperty,
                    () => repeater.VerticalCacheLength, v => repeater.VerticalCacheLength = v, 2.0, 0.5, 6.0);

                var brush1 = new SolidColorBrush(Microsoft.UI.Colors.Red);
                var brush2 = new SolidColorBrush(Microsoft.UI.Colors.Blue);
                VerifyDependencyProperty(repeater, "Background", ItemsRepeater.BackgroundProperty,
                    () => repeater.Background, v => repeater.Background = v, null, brush1, brush2);

                // ItemTemplate cannot be cleared back to null (product concern PC-ITEMTEMPLATE-NULL, covered by the ignored
                // RepeaterTests.VerifyClearingItemTemplateFallsBackToDefaultTemplate), so only default/set/SetValue are checked.
                var template1 = (DataTemplate)XamlReader.Load("<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'><TextBlock/></DataTemplate>");
                var template2 = new RecyclingElementFactory();
                Verify.IsNull(repeater.ItemTemplate);
                Verify.IsNull(repeater.GetValue(ItemsRepeater.ItemTemplateProperty));
                repeater.ItemTemplate = template1;
                Verify.AreSame(template1, repeater.ItemTemplate);
                Verify.AreSame(template1, repeater.GetValue(ItemsRepeater.ItemTemplateProperty));
                repeater.SetValue(ItemsRepeater.ItemTemplateProperty, template2);
                Verify.AreSame(template2, repeater.ItemTemplate);
            });
        }

        // Scenario: read, set, SetValue and ClearValue every dependency property of StackLayout.
        // Expected: Orientation, Spacing and IsVirtualizationEnabled start at their defaults, read back what was set and
        //           return to the defaults when cleared.
        // A failure means: StackLayout settings from markup, bindings or styles would not be applied or read correctly.
        [TestMethod]
        [TestProperty("Description", "Round-trips every StackLayout dependency property through its CLR property, GetValue/SetValue and ClearValue.")]
        public void VerifyStackLayoutDependencyProperties()
        {
            RunOnUIThread.Execute(() =>
            {
                var layout = new StackLayout();
                VerifyDependencyProperty(layout, "Orientation", StackLayout.OrientationProperty,
                    () => layout.Orientation, v => layout.Orientation = v, Orientation.Vertical, Orientation.Horizontal, Orientation.Vertical);
                VerifyDependencyProperty(layout, "Spacing", StackLayout.SpacingProperty,
                    () => layout.Spacing, v => layout.Spacing = v, 0.0, 12.0, 3.0);
                VerifyDependencyProperty(layout, "IsVirtualizationEnabled", StackLayout.IsVirtualizationEnabledProperty,
                    () => layout.IsVirtualizationEnabled, v => layout.IsVirtualizationEnabled = v, true, false, true);
            });
        }

        // Scenario: read, set, SetValue and ClearValue every dependency property of UniformGridLayout.
        // Expected: all eight properties start at their documented defaults, read back what was set and return to the
        //           defaults when cleared.
        // A failure means: grid settings such as ItemsJustification or ItemsStretch could not be read back by apps.
        [TestMethod]
        [TestProperty("Description", "Round-trips every UniformGridLayout dependency property through its CLR property, GetValue/SetValue and ClearValue.")]
        public void VerifyUniformGridLayoutDependencyProperties()
        {
            RunOnUIThread.Execute(() =>
            {
                var layout = new UniformGridLayout();
                VerifyDependencyProperty(layout, "Orientation", UniformGridLayout.OrientationProperty,
                    () => layout.Orientation, v => layout.Orientation = v, Orientation.Horizontal, Orientation.Vertical, Orientation.Horizontal);
                VerifyDependencyProperty(layout, "MinItemWidth", UniformGridLayout.MinItemWidthProperty,
                    () => layout.MinItemWidth, v => layout.MinItemWidth = v, 0.0, 40.0, 25.0);
                VerifyDependencyProperty(layout, "MinItemHeight", UniformGridLayout.MinItemHeightProperty,
                    () => layout.MinItemHeight, v => layout.MinItemHeight = v, 0.0, 30.0, 15.0);
                VerifyDependencyProperty(layout, "MinRowSpacing", UniformGridLayout.MinRowSpacingProperty,
                    () => layout.MinRowSpacing, v => layout.MinRowSpacing = v, 0.0, 5.0, 7.0);
                VerifyDependencyProperty(layout, "MinColumnSpacing", UniformGridLayout.MinColumnSpacingProperty,
                    () => layout.MinColumnSpacing, v => layout.MinColumnSpacing = v, 0.0, 6.0, 8.0);
                VerifyDependencyProperty(layout, "ItemsJustification", UniformGridLayout.ItemsJustificationProperty,
                    () => layout.ItemsJustification, v => layout.ItemsJustification = v,
                    UniformGridLayoutItemsJustification.Start, UniformGridLayoutItemsJustification.SpaceEvenly, UniformGridLayoutItemsJustification.End);
                VerifyDependencyProperty(layout, "ItemsStretch", UniformGridLayout.ItemsStretchProperty,
                    () => layout.ItemsStretch, v => layout.ItemsStretch = v,
                    UniformGridLayoutItemsStretch.None, UniformGridLayoutItemsStretch.Uniform, UniformGridLayoutItemsStretch.Fill);
                VerifyDependencyProperty(layout, "MaximumRowsOrColumns", UniformGridLayout.MaximumRowsOrColumnsProperty,
                    () => layout.MaximumRowsOrColumns, v => layout.MaximumRowsOrColumns = v, -1, 3, 5);
            });
        }

        // Scenario: read, set, SetValue and ClearValue every dependency property of FlowLayout.
        // Expected: Orientation, LineSpacing, MinItemSpacing and LineAlignment start at their defaults, read back what was
        //           set and return to the defaults when cleared.
        // A failure means: FlowLayout settings could not be read back by apps, bindings or styles.
        [TestMethod]
        [TestProperty("Description", "Round-trips every FlowLayout dependency property through its CLR property, GetValue/SetValue and ClearValue.")]
        public void VerifyFlowLayoutDependencyProperties()
        {
            RunOnUIThread.Execute(() =>
            {
                var layout = new FlowLayout();
                VerifyDependencyProperty(layout, "Orientation", FlowLayout.OrientationProperty,
                    () => layout.Orientation, v => layout.Orientation = v, Orientation.Horizontal, Orientation.Vertical, Orientation.Horizontal);
                VerifyDependencyProperty(layout, "LineSpacing", FlowLayout.LineSpacingProperty,
                    () => layout.LineSpacing, v => layout.LineSpacing = v, 0.0, 10.0, 4.0);
                VerifyDependencyProperty(layout, "MinItemSpacing", FlowLayout.MinItemSpacingProperty,
                    () => layout.MinItemSpacing, v => layout.MinItemSpacing = v, 0.0, 9.0, 2.0);
                VerifyDependencyProperty(layout, "LineAlignment", FlowLayout.LineAlignmentProperty,
                    () => layout.LineAlignment, v => layout.LineAlignment = v,
                    FlowLayoutLineAlignment.Start, FlowLayoutLineAlignment.SpaceAround, FlowLayoutLineAlignment.End);
            });
        }

        // Scenario: read, set, SetValue and ClearValue every dependency property of LinedFlowLayout.
        // Expected: the read-write properties start at their documented defaults (LineHeight is NaN), read back what was set
        //           and return to the defaults when cleared; ActualLineHeight reads 0 before any layout.
        // A failure means: LinedFlowLayout settings could not be read back by apps, bindings or styles.
        [TestMethod]
        [TestProperty("Description", "Round-trips every LinedFlowLayout dependency property through its CLR property, GetValue/SetValue and ClearValue.")]
        public void VerifyLinedFlowLayoutDependencyProperties()
        {
            RunOnUIThread.Execute(() =>
            {
                var layout = new LinedFlowLayout();
                VerifyDependencyProperty(layout, "ItemsJustification", LinedFlowLayout.ItemsJustificationProperty,
                    () => layout.ItemsJustification, v => layout.ItemsJustification = v,
                    LinedFlowLayoutItemsJustification.Start, LinedFlowLayoutItemsJustification.SpaceBetween, LinedFlowLayoutItemsJustification.Center);
                VerifyDependencyProperty(layout, "ItemsStretch", LinedFlowLayout.ItemsStretchProperty,
                    () => layout.ItemsStretch, v => layout.ItemsStretch = v,
                    LinedFlowLayoutItemsStretch.None, LinedFlowLayoutItemsStretch.Fill, LinedFlowLayoutItemsStretch.None);
                VerifyDependencyProperty(layout, "MinItemSpacing", LinedFlowLayout.MinItemSpacingProperty,
                    () => layout.MinItemSpacing, v => layout.MinItemSpacing = v, 0.0, 6.0, 3.0);
                VerifyDependencyProperty(layout, "LineSpacing", LinedFlowLayout.LineSpacingProperty,
                    () => layout.LineSpacing, v => layout.LineSpacing = v, 0.0, 8.0, 2.0);
                VerifyDependencyProperty(layout, "LineHeight", LinedFlowLayout.LineHeightProperty,
                    () => layout.LineHeight, v => layout.LineHeight = v, double.NaN, 90.0, 45.0);

                Verify.AreEqual(0.0, layout.ActualLineHeight, "ActualLineHeight before any layout (CLR getter).");
                Verify.AreEqual(0.0, (double)layout.GetValue(LinedFlowLayout.ActualLineHeightProperty), "ActualLineHeight before any layout (GetValue).");
            });
        }

        // Scenario: get the WinRT runtime class name of an instance of every Repeater runtime class, including event args
        //           raised by ItemsRepeater, SelectionModel, RecyclingElementFactory and LinedFlowLayout.
        // Expected: each object reports its exact Microsoft.UI.Xaml runtime class name, and an ElementIndexChanged handler
        //           removed with -= is no longer called.
        // A failure means: XAML, data binding or other languages could not identify these objects by their runtime type.
        [TestMethod]
        [TestProperty("Description", "Verifies IInspectable.GetRuntimeClassName for instances of every Repeater runtime class and event args.")]
        public void VerifyRuntimeClassNamesOfRepeaterObjects()
        {
            RunOnUIThread.Execute(() =>
            {
                const string ns = "Microsoft.UI.Xaml.Controls.";
                var instances = new List<Tuple<string, object>>()
                {
                    Tuple.Create<string, object>(ns + "ItemsRepeater", new ItemsRepeater()),
                    Tuple.Create<string, object>(ns + "ItemsRepeaterScrollHost", new ItemsRepeaterScrollHost()),
                    Tuple.Create<string, object>(ns + "ElementFactory", new ElementFactory()),
                    Tuple.Create<string, object>(ns + "RecyclingElementFactory", new RecyclingElementFactory()),
                    Tuple.Create<string, object>(ns + "RecyclePool", new RecyclePool()),
                    Tuple.Create<string, object>(ns + "VirtualizingLayout", new VirtualizingLayout()),
                    Tuple.Create<string, object>(ns + "NonVirtualizingLayout", new NonVirtualizingLayout()),
                    Tuple.Create<string, object>(ns + "VirtualizingLayoutContext", new VirtualizingLayoutContext()),
                    Tuple.Create<string, object>(ns + "NonVirtualizingLayoutContext", new NonVirtualizingLayoutContext()),
                    Tuple.Create<string, object>(ns + "StackLayout", new StackLayout()),
                    Tuple.Create<string, object>(ns + "StackLayoutState", new StackLayoutState()),
                    Tuple.Create<string, object>(ns + "UniformGridLayout", new UniformGridLayout()),
                    Tuple.Create<string, object>(ns + "UniformGridLayoutState", new UniformGridLayoutState()),
                    Tuple.Create<string, object>(ns + "FlowLayout", new FlowLayout()),
                    Tuple.Create<string, object>(ns + "FlowLayoutState", new FlowLayoutState()),
                    Tuple.Create<string, object>(ns + "LinedFlowLayout", new LinedFlowLayout()),
                    Tuple.Create<string, object>(ns + "ItemCollectionTransitionProvider", new ItemCollectionTransitionProvider()),
                    Tuple.Create<string, object>(ns + "LinedFlowLayoutItemCollectionTransitionProvider", new LinedFlowLayoutItemCollectionTransitionProvider()),
                    Tuple.Create<string, object>(ns + "IndexPath", IndexPath.CreateFrom(1, 2)),
                    Tuple.Create<string, object>(ns + "SelectionModel", new SelectionModel()),
                };

                // Event args are only created by the product, so raise each event once and keep the args.
                var data = new ObservableCollection<string>(Enumerable.Range(0, 3).Select(i => "Item #" + i));
                var repeater = new ItemsRepeater() {
                    ItemsSource = data,
                    ItemTemplate = (DataTemplate)XamlReader.Load("<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'><TextBlock Text='{Binding}' Height='20'/></DataTemplate>"),
                };
                object preparedArgs = null, clearingArgs = null, indexChangedArgs = null;
                repeater.ElementPrepared += (s, e) => preparedArgs = preparedArgs ?? e;
                repeater.ElementClearing += (s, e) => clearingArgs = clearingArgs ?? e;
                int indexChangedCount = 0;
                global::Windows.Foundation.TypedEventHandler<ItemsRepeater, ItemsRepeaterElementIndexChangedEventArgs> onIndexChanged = (s, e) =>
                {
                    indexChangedCount++;
                    indexChangedArgs = indexChangedArgs ?? e;
                };
                repeater.ElementIndexChanged += onIndexChanged;
                Content = repeater;
                Content.UpdateLayout();
                data.Insert(0, "New");
                data.RemoveAt(data.Count - 1);
                Content.UpdateLayout();
                Verify.IsGreaterThan(indexChangedCount, 0);
                repeater.ElementIndexChanged -= onIndexChanged;
                int countAfterRemoval = indexChangedCount;
                data.Insert(0, "Another");
                Content.UpdateLayout();
                Verify.AreEqual(countAfterRemoval, indexChangedCount, "A removed ElementIndexChanged handler is not called.");
                instances.Add(Tuple.Create(ns + "ItemsRepeaterElementPreparedEventArgs", preparedArgs));
                instances.Add(Tuple.Create(ns + "ItemsRepeaterElementClearingEventArgs", clearingArgs));
                instances.Add(Tuple.Create(ns + "ItemsRepeaterElementIndexChangedEventArgs", indexChangedArgs));

                object selectionChangedArgs = null, childrenRequestedArgs = null;
                var selectionModel = new SelectionModel() { Source = new List<object>() { new List<int>() { 1, 2 } } };
                selectionModel.SelectionChanged += (s, e) => selectionChangedArgs = e;
                selectionModel.ChildrenRequested += (s, e) =>
                {
                    childrenRequestedArgs = e;
                    e.Children = e.Source;
                };
                selectionModel.Select(0, 1);
                instances.Add(Tuple.Create(ns + "SelectionModelSelectionChangedEventArgs", selectionChangedArgs));
                instances.Add(Tuple.Create(ns + "SelectionModelChildrenRequestedEventArgs", childrenRequestedArgs));

                object selectTemplateArgs = null;
                var factory = new RecyclingElementFactory() { RecyclePool = new RecyclePool() };
                factory.Templates["T"] = (DataTemplate)XamlReader.Load("<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'><TextBlock/></DataTemplate>");
                // With a single template the factory does not ask for a key, so register a second one.
                factory.Templates["U"] = (DataTemplate)XamlReader.Load("<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'><Button/></DataTemplate>");
                factory.SelectTemplateKey += (s, e) =>
                {
                    selectTemplateArgs = e;
                    e.TemplateKey = "T";
                };
                Verify.IsNotNull(factory.GetElement(new ElementFactoryGetArgs() { Data = "Item", Parent = repeater }));
                instances.Add(Tuple.Create(ns + "SelectTemplateEventArgs", selectTemplateArgs));

                object itemsInfoArgs = null;
                // This layout runs a layout pass, which can start its measure timer, so keep it alive and detach it even if
                // a check fails (product concern PC-LFL-TIMER; see s_linedFlowLayoutsKeptAlive).
                var linedFlowLayout = new LinedFlowLayout() { LineHeight = 20 };
                s_linedFlowLayoutsKeptAlive.Add(linedFlowLayout);
                linedFlowLayout.ItemsInfoRequested += (s, e) => itemsInfoArgs = itemsInfoArgs ?? e;
                var linedRepeater = new ItemsRepeater() {
                    ItemsSource = Enumerable.Range(0, 5).Select(i => new Border() { Width = 30, Height = 20 }).ToList(),
                    Layout = linedFlowLayout,
                    Width = 200
                };
                try
                {
                    Content = linedRepeater;
                    Content.UpdateLayout();
                    instances.Add(Tuple.Create(ns + "LinedFlowLayoutItemsInfoRequestedEventArgs", itemsInfoArgs));
                }
                finally
                {
                    // Detaching stops the layout's measure timer on the UI thread.
                    linedRepeater.Layout = null;
                    Content = null;
                }

                foreach (var entry in instances)
                {
                    Verify.IsNotNull(entry.Item2, entry.Item1 + " instance was obtained.");
                    string actual = GetNativeRuntimeClassName(entry.Item2);
                    Log.Comment(entry.Item1 + " -> " + actual);
                    Verify.AreEqual(entry.Item1, actual);
                }
            });
        }

        // Scenario: create an ItemsSourceView over a list and get its WinRT runtime class name.
        // Expected: it reports Microsoft.UI.Xaml.Controls.ItemsSourceView, like its activation factory does.
        // Ignored: reproduces ItemsSourceView instances reporting an interface name as runtime class
        //          (PC-ITEMSSOURCEVIEW-CLASSNAME); currently fails because it reports Microsoft.UI.Xaml.Controls.IItemsSourceView.
        [TestMethod]
        [TestProperty("Ignore", "True")] // Product concern PC-ITEMSSOURCEVIEW-CLASSNAME: expected runtime class Microsoft.UI.Xaml.Controls.ItemsSourceView, actual Microsoft.UI.Xaml.Controls.IItemsSourceView (the InspectingDataSource implementation does not override GetRuntimeClassName).
        [TestProperty("Description", "Verifies an ItemsSourceView instance reports the ItemsSourceView runtime class name.")]
        public void VerifyItemsSourceViewRuntimeClassName()
        {
            RunOnUIThread.Execute(() =>
            {
                var view = new ItemsSourceView(new List<string>() { "A" });
                Verify.AreEqual("Microsoft.UI.Xaml.Controls.ItemsSourceView", GetNativeRuntimeClassName(view));
            });
        }

        // Scenario: get the activation factory of every activatable Repeater runtime class and of the static-only IndexPath,
        //           read the factory's runtime class name and call IActivationFactory.ActivateInstance on it.
        // Expected: each factory reports the class name; composable classes and IndexPath refuse ActivateInstance with
        //           E_NOTIMPL, while the sealed ItemsRepeaterScrollHost is created by it.
        // A failure means: native or non-.NET callers could create Repeater objects in an unsupported way, or fail to find them.
        [TestMethod]
        [TestProperty("Description", "Verifies the runtime class name and IActivationFactory.ActivateInstance behavior of every Repeater activation factory.")]
        public void VerifyActivationFactoriesOfRepeaterClasses()
        {
            RunOnUIThread.Execute(() =>
            {
                var notActivatable = new string[]
                {
                    "Microsoft.UI.Xaml.Controls.ItemsRepeater",
                    "Microsoft.UI.Xaml.Controls.ItemsSourceView",
                    "Microsoft.UI.Xaml.Controls.ElementFactory",
                    "Microsoft.UI.Xaml.Controls.RecyclingElementFactory",
                    "Microsoft.UI.Xaml.Controls.RecyclePool",
                    "Microsoft.UI.Xaml.Controls.VirtualizingLayout",
                    "Microsoft.UI.Xaml.Controls.NonVirtualizingLayout",
                    "Microsoft.UI.Xaml.Controls.VirtualizingLayoutContext",
                    "Microsoft.UI.Xaml.Controls.NonVirtualizingLayoutContext",
                    "Microsoft.UI.Xaml.Controls.StackLayout",
                    "Microsoft.UI.Xaml.Controls.StackLayoutState",
                    "Microsoft.UI.Xaml.Controls.UniformGridLayout",
                    "Microsoft.UI.Xaml.Controls.UniformGridLayoutState",
                    "Microsoft.UI.Xaml.Controls.FlowLayout",
                    "Microsoft.UI.Xaml.Controls.FlowLayoutState",
                    "Microsoft.UI.Xaml.Controls.LinedFlowLayout",
                    "Microsoft.UI.Xaml.Controls.ItemCollectionTransitionProvider",
                    "Microsoft.UI.Xaml.Controls.LinedFlowLayoutItemCollectionTransitionProvider",
                    "Microsoft.UI.Xaml.Controls.SelectionModel",
                    "Microsoft.UI.Xaml.Controls.IndexPath",
                    "Microsoft.UI.Xaml.Automation.Peers.RepeaterAutomationPeer",
                };

                foreach (var className in notActivatable)
                {
                    var factory = global::WinRT.ActivationFactory.Get(className);
                    Verify.AreEqual(className, global::WinRT.IInspectable.FromAbi(factory.ThisPtr).GetRuntimeClassName(false), className + " factory name");
                    int hr = 0;
                    try
                    {
                        global::ABI.WinRT.Interop.IActivationFactoryMethods.ActivateInstanceUnsafe(factory, IID_IInspectable);
                    }
                    catch (Exception e)
                    {
                        hr = e.HResult;
                    }
                    Log.Comment($"{className}: ActivateInstance HResult=0x{hr:X8}");
                    Verify.AreEqual(E_NOTIMPL, hr, className + " refuses plain activation.");
                }

                const string scrollHostName = "Microsoft.UI.Xaml.Controls.ItemsRepeaterScrollHost";
                var scrollHostFactory = global::WinRT.ActivationFactory.Get(scrollHostName);
                Verify.AreEqual(scrollHostName, global::WinRT.IInspectable.FromAbi(scrollHostFactory.ThisPtr).GetRuntimeClassName(false));
                var created = global::ABI.WinRT.Interop.IActivationFactoryMethods.ActivateInstanceUnsafe(scrollHostFactory, IID_IInspectable);
                Verify.IsNotNull(created);
                Verify.AreEqual(scrollHostName, global::WinRT.IInspectable.FromAbi(created.ThisPtr).GetRuntimeClassName(false), "The sealed scroll host is created by ActivateInstance.");
            });
        }

        // Scenario: subscribe a single handler to SelectionModel.ChildrenRequested and to RecyclingElementFactory.SelectTemplateKey,
        //           raise each event once, remove the handler (the last one, so the projection also unregisters from the
        //           native event) and raise the event again.
        // Expected: each handler runs once while subscribed and never after removal; with no handler left, the factory
        //           rejects a two-template GetElement with E_FAIL and nested selection still works.
        // A failure means: removed event handlers would keep running, leaking the subscriber or acting on stale state.
        [TestMethod]
        [TestProperty("Description", "Verifies removing the last ChildrenRequested and SelectTemplateKey handler detaches it.")]
        public void VerifyRemovingLastEventHandlerDetachesIt()
        {
            RunOnUIThread.Execute(() =>
            {
                var selectionModel = new SelectionModel() {
                    Source = new List<object>() { new List<object>() { 1, 2, 3 }, new List<object>() { 4, 5, 6 } }
                };
                int childrenRequestedCalls = 0;
                global::Windows.Foundation.TypedEventHandler<SelectionModel, SelectionModelChildrenRequestedEventArgs> onChildrenRequested = (s, e) =>
                {
                    childrenRequestedCalls++;
                    e.Children = e.Source is global::System.Collections.IList ? e.Source : null;
                };
                selectionModel.ChildrenRequested += onChildrenRequested;
                selectionModel.Select(0, 1);
                Verify.IsGreaterThan(childrenRequestedCalls, 0, "ChildrenRequested runs while subscribed.");
                selectionModel.ChildrenRequested -= onChildrenRequested;
                int childrenCallsAfterRemoval = childrenRequestedCalls;
                selectionModel.Source = new List<object>() { new List<object>() { 1, 2, 3 }, new List<object>() { 4, 5, 6 } };
                selectionModel.Select(1, 2);
                Verify.AreEqual(childrenCallsAfterRemoval, childrenRequestedCalls, "The removed (last) ChildrenRequested handler is not called.");
                Verify.IsTrue(selectionModel.IsSelected(1, 2).Value, "Nested selection still works without a handler.");

                var factory = new RecyclingElementFactory() { RecyclePool = new RecyclePool() };
                factory.Templates["T"] = (DataTemplate)XamlReader.Load("<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'><TextBlock/></DataTemplate>");
                factory.Templates["U"] = (DataTemplate)XamlReader.Load("<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'><Button/></DataTemplate>");
                int selectTemplateKeyCalls = 0;
                global::Windows.Foundation.TypedEventHandler<RecyclingElementFactory, SelectTemplateEventArgs> onSelectTemplateKey = (s, e) =>
                {
                    selectTemplateKeyCalls++;
                    e.TemplateKey = "U";
                };
                var owner = new StackPanel();
                factory.SelectTemplateKey += onSelectTemplateKey;
                Verify.IsTrue(factory.GetElement(new ElementFactoryGetArgs() { Data = 1, Parent = owner }) is Button);
                Verify.AreEqual(1, selectTemplateKeyCalls);
                factory.SelectTemplateKey -= onSelectTemplateKey;
                int hr = 0;
                try
                {
                    factory.GetElement(new ElementFactoryGetArgs() { Data = 2, Parent = owner });
                }
                catch (Exception e)
                {
                    hr = e.HResult;
                }
                Verify.AreEqual(1, selectTemplateKeyCalls, "The removed (last) SelectTemplateKey handler is not called.");
                Verify.AreEqual(unchecked((int)0x80004005), hr, "Without a SelectTemplateKey handler, a factory with two templates cannot pick one (E_FAIL).");
            });
        }

        private static string GetNativeRuntimeClassName(object instance)
        {
            var objRef = ((global::WinRT.IWinRTObject)instance).NativeObject;
            return global::WinRT.IInspectable.FromAbi(objRef.ThisPtr).GetRuntimeClassName(false);
        }

        private static bool AreSameValue(object expected, object actual)
        {
            if (expected is double e && actual is double a && double.IsNaN(e) && double.IsNaN(a))
            {
                return true;
            }
            return Equals(expected, actual);
        }

        private static void VerifyDependencyProperty<T>(DependencyObject owner, string name, DependencyProperty property,
            Func<T> getter, Action<T> setter, T defaultValue, T firstValue, T secondValue)
        {
            Log.Comment("Property " + name);
            Verify.IsNotNull(property, name + "Property identifier");
            Verify.IsTrue(AreSameValue(defaultValue, getter()), $"{name} default (CLR getter): expected {defaultValue}, actual {getter()}");
            Verify.IsTrue(AreSameValue(defaultValue, owner.GetValue(property)), $"{name} default (GetValue): expected {defaultValue}, actual {owner.GetValue(property)}");

            setter(firstValue);
            Verify.IsTrue(AreSameValue(firstValue, getter()), $"{name} after CLR set (CLR getter): expected {firstValue}, actual {getter()}");
            Verify.IsTrue(AreSameValue(firstValue, owner.GetValue(property)), $"{name} after CLR set (GetValue): expected {firstValue}, actual {owner.GetValue(property)}");

            owner.SetValue(property, secondValue);
            Verify.IsTrue(AreSameValue(secondValue, getter()), $"{name} after SetValue (CLR getter): expected {secondValue}, actual {getter()}");

            if (AreSameValue(secondValue, defaultValue))
            {
                // Two-valued properties use the default as the SetValue value; move away from it again so that ClearValue
                // has an observable effect.
                owner.SetValue(property, firstValue);
                Verify.IsTrue(AreSameValue(firstValue, getter()), $"{name} after second SetValue (CLR getter): expected {firstValue}, actual {getter()}");
            }

            owner.ClearValue(property);
            Verify.IsTrue(AreSameValue(defaultValue, getter()), $"{name} after ClearValue (CLR getter): expected {defaultValue}, actual {getter()}");
        }
    }
}
