// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.RepeaterTests.Common;
using Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.RepeaterTests.Common.Mocks;
using MUXControlsTestApp.Utilities;
using System.Collections.ObjectModel;
using System.Linq;
using System.Runtime.InteropServices;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Media;
using Common;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.RepeaterTests
{
    [TestClass]
    public partial class RecyclePoolTests : ApiTestBase
    {
        // This test was missing its [TestMethod] attribute and therefore never ran.
        // Scenario: store Buttons, TextBlocks and StackPanels under different keys (and one under the empty key), then
        //           retrieve them.
        // Expected: each key returns only its own elements until empty, the empty key is a regular key, and non-Panel
        //           owners fail.
        // A failure means: recycled elements could come back for the wrong template (previously this test never ran).
        [TestMethod]
        [TestProperty("Description", "Verifies RecyclePool returns elements only for the key they were stored under and rejects non-Panel owners.")]
        public void ValidateElementsHaveCorrectKeys()
        {
            RunOnUIThread.Execute(() =>
            {
                var element = new Button();
                const string buttonKey = "ButtonKey";
                const string textBlockKey = "TextBlockKey";
                const string stackPanelKey = "StackPanelKey";

                RecyclePool pool = new RecyclePool();
                pool.PutElement(new Button(), buttonKey);
                pool.PutElement(new TextBlock(), textBlockKey);
                pool.PutElement(new StackPanel(), stackPanelKey);
                pool.PutElement(new Button(), buttonKey);
                pool.PutElement(new TextBlock(), textBlockKey);
                pool.PutElement(new StackPanel(), stackPanelKey);
                pool.PutElement(new Button(), buttonKey);
                pool.PutElement(new TextBlock(), textBlockKey);
                pool.PutElement(new StackPanel(), stackPanelKey);

                Verify.IsNotNull((Button)pool.TryGetElement(buttonKey));
                Verify.IsNotNull((Button)pool.TryGetElement(buttonKey));
                Verify.IsNotNull((Button)pool.TryGetElement(buttonKey));
                Verify.IsNull(pool.TryGetElement(buttonKey));

                Verify.IsNotNull((TextBlock)pool.TryGetElement(textBlockKey));
                Verify.IsNotNull((TextBlock)pool.TryGetElement(textBlockKey));
                Verify.IsNotNull((TextBlock)pool.TryGetElement(textBlockKey));
                Verify.IsNull(pool.TryGetElement(textBlockKey));

                Verify.IsNotNull((StackPanel)pool.TryGetElement(stackPanelKey));
                Verify.IsNotNull((StackPanel)pool.TryGetElement(stackPanelKey));
                Verify.IsNotNull((StackPanel)pool.TryGetElement(stackPanelKey));
                Verify.IsNull(pool.TryGetElement(stackPanelKey));


                // A null key cannot be distinguished from an empty key through the C# projection (both marshal
                // as an empty HSTRING), and RecyclePool treats the empty key as a regular key.
                var emptyKeyElement = new Button();
                pool.PutElement(emptyKeyElement, null, null);
                Verify.IsNull(pool.TryGetElement(buttonKey), "Elements stored under the empty key are not returned for another key.");
                Verify.AreSame(emptyKeyElement, pool.TryGetElement(string.Empty, null));
                Verify.IsNull(pool.TryGetElement(null, null), "The empty-key element was already returned.");

                Verify.Throws<COMException>(delegate
                {
                    pool.PutElement(new Button(), buttonKey, new Button() /* not a panel */);
                });
            });
        }

        [TestMethod]
        public void ValidateOwnershipWithStackPanel()
        {
            RunOnUIThread.Execute(() =>
            {
                RecyclePool pool = new RecyclePool();
                var owner = new StackPanel();
                var child = new Button();
                owner.Children.Add(child);
                pool.PutElement(child, "Key", owner);
                var recycled = pool.TryGetElement("Key", owner);
                Verify.AreSame(child, recycled);
                Verify.AreEqual(0, owner.Children.IndexOf(child));
            });
        }

        // Validate that if the pool has an element for the requested owner,
        // then that is given preference over other elements.
        [TestMethod]
        public void ValidateRecycledElementOwnerAffinity()
        {
            RunOnUIThread.Execute(() =>
            {
                ItemsRepeater repeater1 = null;
                ItemsRepeater repeater2 = null;
                const int numItems = 10;
                var dataCollection = new ObservableCollection<int>(Enumerable.Range(0, numItems));
                const string recycleKey = "key";

                var dataSource = MockItemsSource.CreateDataSource<int>(dataCollection, true);
                var layout = new StackLayout();
                var recyclePool = new RecyclePool();
                var itemTemplate = (DataTemplate)XamlReader.Load(
                    @"<DataTemplate  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                         <TextBlock Text='{Binding}' />
                    </DataTemplate>");

                repeater1 = new ItemsRepeater()
                {
                    ItemsSource = dataSource,
                    Layout = layout,
                    ItemTemplate = new RecyclingElementFactoryDerived()
                    {
                        Templates = { { "key", itemTemplate } },
                        RecyclePool = recyclePool,
                        SelectTemplateIdFunc = (object data, UIElement owner) => recycleKey
                    }
                };

                repeater2 = new ItemsRepeater()
                {
                    ItemsSource = dataSource,
                    Layout = layout,
                    ItemTemplate = new RecyclingElementFactoryDerived()
                    {
                        Templates = { { "key", itemTemplate } },
                        RecyclePool = recyclePool,
                        SelectTemplateIdFunc = (object data, UIElement owner) => recycleKey
                    }
                };

                var root = new StackPanel();
                root.Children.Add(repeater1);
                root.Children.Add(repeater2);

                Content = new ItemsRepeaterScrollHost()
                {
                    Width = 400,
                    Height = 400,
                    ScrollViewer = new ScrollViewer()
                    {
                        Content = root
                    }
                };

                Content.UpdateLayout();
                Verify.AreEqual(numItems, VisualTreeHelper.GetChildrenCount(repeater1));
                Verify.AreEqual(numItems, VisualTreeHelper.GetChildrenCount(repeater2));

                // Throw all the elements into the recycle pool
                dataCollection.Clear();
                Content.UpdateLayout();

                for (int i = 0; i < numItems; i++)
                {
                    var element1 = (FrameworkElement)recyclePool.TryGetElement(recycleKey, repeater1);
                    Verify.AreSame(repeater1, element1.Parent);

                    var element2 = (FrameworkElement)recyclePool.TryGetElement(recycleKey, repeater2);
                    Verify.AreSame(repeater2, element2.Parent);
                }
            });
        }

        // When the owner is not the same, the element we get out of the recycle
        // pool should be disconnected from its previous parent.
        [TestMethod]
        public void ValidateChildRemovedFromParentWhenOwnerIsDifferent()
        {
            RunOnUIThread.Execute(() =>
            {
                var element = new Button();
                const string key1 = "Key1";
                const string key2 = "Key2";

                RecyclePool pool = new RecyclePool();
                var parent1 = new StackPanel();
                var child1 = new Button();
                var child2 = new Button();
                parent1.Children.Add(child1);
                parent1.Children.Add(child2);
                
                pool.PutElement(child1, key1);
                pool.PutElement(child2, key2);

                var parent2 = new StackPanel();
                // Recycle the second child for a different parent. It should be disconnected
                var recycled1 = (FrameworkElement)pool.TryGetElement(key2, parent2);
                Verify.IsNull(recycled1.Parent);
                var recycled2 = (FrameworkElement)pool.TryGetElement(key1, parent2);
                Verify.IsNull(recycled2.Parent);
            });
        }

        // Scenario: attach a custom RecyclePool to a DataTemplate with SetPoolInstance, use the template as an
        //           ItemsRepeater's ItemTemplate, remove an item and then add one.
        // Expected: GetPoolInstance and GetValue(PoolInstanceProperty) return the pool, the removed item's element is found
        //           in that pool, and clearing or setting null detaches the pool.
        // A failure means: apps that share or customize element pools per template would not get their elements recycled.
        [TestMethod]
        [TestProperty("Description", "Verifies RecyclePool.SetPoolInstance/GetPoolInstance/PoolInstanceProperty and that ItemsRepeater recycles DataTemplate elements through the attached pool.")]
        public void VerifyPoolInstanceIsUsedForDataTemplateRecycling()
        {
            RunOnUIThread.Execute(() =>
            {
                var template = (DataTemplate)XamlReader.Load(
                    "<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'><TextBlock Text='{Binding}' Height='20'/></DataTemplate>");
                Verify.IsNull(RecyclePool.GetPoolInstance(template), "A template has no pool until one is attached.");

                var pool = new RecyclePool();
                RecyclePool.SetPoolInstance(template, pool);
                Verify.AreSame(pool, RecyclePool.GetPoolInstance(template));
                Verify.AreSame(pool, template.GetValue(RecyclePool.PoolInstanceProperty));

                var data = new ObservableCollection<string>(Enumerable.Range(0, 3).Select(i => "Item #" + i));
                var repeater = new ItemsRepeater() { ItemsSource = data, ItemTemplate = template };
                Content = repeater;
                Content.UpdateLayout();
                Verify.IsNull(pool.TryGetElement(string.Empty, repeater), "Nothing is recycled while all items are shown.");

                var removedElement = repeater.TryGetElement(2);
                Verify.IsNotNull(removedElement);
                data.RemoveAt(2);
                Content.UpdateLayout();
                Verify.AreSame(removedElement, pool.TryGetElement(string.Empty, repeater), "The removed item's element was recycled into the attached pool.");
                Verify.IsNull(pool.TryGetElement(string.Empty, repeater), "Only that element was recycled.");

                data.Add("Item #3");
                Content.UpdateLayout();
                var newElement = (TextBlock)repeater.TryGetElement(2);
                Verify.AreNotSame(removedElement, newElement, "The element taken out of the pool is not reused.");
                Verify.AreEqual("Item #3", newElement.Text);

                Content = null;
                template.ClearValue(RecyclePool.PoolInstanceProperty);
                Verify.IsNull(RecyclePool.GetPoolInstance(template));
                RecyclePool.SetPoolInstance(template, pool);
                RecyclePool.SetPoolInstance(template, null);
                Verify.IsNull(RecyclePool.GetPoolInstance(template), "SetPoolInstance(null) detaches the pool.");
            });
        }

        // Scenario: attach a RecyclePool subclass that overrides PutElementCore and TryGetElementCore to a DataTemplate used
        //           by an ItemsRepeater, remove an item and add one.
        // Expected: the repeater's put and get calls reach the overrides (the removed element is put with the empty key).
        // Ignored: reproduces RecyclePool overrides being bypassed (PC-RECYCLEPOOL-OVERRIDES); currently fails because
        //          RecyclePool.PutElement/TryGetElement call the base implementation directly.
        [TestMethod]
        [TestProperty("Ignore", "True")] // Product concern PC-RECYCLEPOOL-OVERRIDES: expected PutElementCore/TryGetElementCore overrides to be called; actual: 0 calls, RecyclePool::PutElement/TryGetElement call the base *Core directly (RecyclePool.cpp).
        [TestProperty("Description", "Verifies RecyclePool PutElementCore/TryGetElementCore overrides are used when ItemsRepeater recycles through the pool.")]
        public void VerifyRecyclePoolOverridesAreCalled()
        {
            RunOnUIThread.Execute(() =>
            {
                var template = (DataTemplate)XamlReader.Load(
                    "<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'><TextBlock Text='{Binding}' Height='20'/></DataTemplate>");
                var pool = new CountingRecyclePool();
                RecyclePool.SetPoolInstance(template, pool);
                var data = new ObservableCollection<string>(Enumerable.Range(0, 3).Select(i => "Item #" + i));
                var repeater = new ItemsRepeater() { ItemsSource = data, ItemTemplate = template };
                Content = repeater;
                Content.UpdateLayout();

                var removedElement = repeater.TryGetElement(2);
                data.RemoveAt(2);
                Content.UpdateLayout();
                Verify.AreEqual(1, pool.PutCount, "The removed item's element is put through the override.");
                Verify.AreSame(removedElement, pool.LastPutElement);
                Verify.AreEqual(string.Empty, pool.LastPutKey);

                data.Add("Item #3");
                Content.UpdateLayout();
                Verify.IsGreaterThanOrEqual(pool.TryGetCount, 1, "The repeater asks the override for an element.");
                Content = null;
            });
        }

        private partial class CountingRecyclePool : RecyclePool
        {
            public int PutCount { get; private set; }
            public int TryGetCount { get; private set; }
            public UIElement LastPutElement { get; private set; }
            public string LastPutKey { get; private set; }

            protected override void PutElementCore(UIElement element, string key, UIElement owner)
            {
                PutCount++;
                LastPutElement = element;
                LastPutKey = key;
                base.PutElementCore(element, key, owner);
            }

            protected override UIElement TryGetElementCore(string key, UIElement owner)
            {
                TryGetCount++;
                return base.TryGetElementCore(key, owner);
            }
        }
    }
}
