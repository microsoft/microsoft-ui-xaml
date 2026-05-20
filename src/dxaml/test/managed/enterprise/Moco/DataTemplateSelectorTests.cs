// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using Private.Infrastructure;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Threading;
using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using Windows.UI;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Tests.Common;

namespace Microsoft.UI.Xaml.Tests.Controls.ListViewBase
{
    [TestClass]
    public class DataTemplateSelectorTests : XamlTestsBase
    {
        static string TestDeploymentDir { get; set; }

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
            TestDeploymentDir = context.TestDeploymentDir;
        }

        [ClassCleanup]
        public void ClassCleanup()
        {
            base.CommonClassCleanup();
        }

        public FrameworkElement LoadXamlIntoWindow(string xamlText)
        {
            FrameworkElement target = null;
            UIExecutor.Execute(() =>
            {
                Log.Comment("Loading xaml: ");
                target = (FrameworkElement)XamlReader.Load(xamlText);
                Verify.IsNotNull(target, "Unable to load xaml.");

                TestServices.WindowHelper.WindowContent = target;
            });

            return target;
        }

        /// <summary>
        /// Tests the DataTemplateSelector using ListView with and without ItemTemplate applied
        /// 
        /// Verification:
        /// When ItemTemplate is applied, it takes the precedence over the ItemTemplateSelector
        /// 
        /// Steps:
        /// 1. Load the XAML containing Listview
        /// 2. Populate the ListView
        /// 3. Set ItemTemplate to null (turn off ItemTemplate)
        /// 4. Set the DataTemplateSelector to ItemTemplateSelector property
        /// 5. Verify the Items visual
        /// 6. Set a DataTemplate to ItemsTemplate property (turn on ItemTemplate)
        /// 7. Verify the Items visual
        /// 8. Repeat steps 3 to 7 multiple times
        /// </summary>
        /// 
        [TestMethod]
        [Priority(1)]
        [Description("Tests the DataTemplateSelector using ListView with and without ItemTemplate applied")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void TestWithItemTemplate()
        {
            ListView lv = null;
            const int numberOfIterations = 2;
            const int numberOfItems = 10;
            MoCoDataTemplateSelector1 dataTemplateSelector = null;

            // Load Xaml
            //
            var TargetPanel = LoadXamlIntoWindow(xamlText);

            TestServices.WindowHelper.WaitForIdle();

            // Find the Lists and ItemsPanel
            //
            UIExecutor.Execute(() =>
            {
                Verify.IsNotNull(TargetPanel, "TargetPanel not found");

                // Find the list view
                lv = TargetPanel.FindNameInSubtree("lv") as ListView;
                Verify.IsNotNull(lv, "ListView not found");

                // Populate the list
                //
                Log.Comment("Populating list...");
                lv.ItemsSource = GetFlatDataCollection(numberOfItems);

                // Set the data template selector
                //
                Log.Comment("Setting datatemplate selector...");
                dataTemplateSelector = new MoCoDataTemplateSelector1();
                lv.ItemTemplateSelector = dataTemplateSelector;
            });

            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Starting test with ItemTemplate turned off...");

            // Run for multiple times switching the DataTemplate on and off
            //
            for (int c = 0; c < numberOfIterations; c++)
            {
                UIExecutor.Execute(() =>
                {
                    // Verify if the datatemplate selector is still applied
                    //
                    Verify.AreEqual(dataTemplateSelector, lv.ItemTemplateSelector, "Assigned datatemplate selector is not valid");

                    // Verify if the assigne datatemplate selector has applied correct template on items
                    //
                    Log.Comment("Verifying the current template applied on items...");
                    for (int i = 0; i < numberOfItems; i++)
                    {
                        // Get the ListViewItem
                        //
                        SelectorItem selectorItem = lv.ContainerFromIndex(i) as SelectorItem;
                        Verify.IsNotNull(selectorItem, "Unable to access Item#" + i);

                        // Find the parent border in its visual tree
                        //
                        Border itemBorder = selectorItem.FindNameInSubtree("border", true) as Border;
                        Verify.IsNotNull(itemBorder, "Parent Border is not found in item#" + i);

                        // Verify its background color if it is correctly applied from the DataTemplateSelector
                        //
                        SolidColorBrush borderBrush = itemBorder.Background as SolidColorBrush;
                        Verify.IsNotNull(borderBrush, "Border's brush is not a SolidColorBrush");

                        if (lv.ItemTemplate == null)
                        {
                            DataObject data = lv.Items[i] as DataObject;
                            Verify.IsNotNull(data, "Unable to find the data for item#" + i);

                            Verify.AreEqual((data.Flag ? Colors.Green : Colors.Red), borderBrush.Color, "DataTemplateSelector has not applied expected template");
                        }
                        else
                        {
                            Verify.AreEqual(Colors.Blue, borderBrush.Color, "DataTemplate from ItemTemplate is not applied");
                        }
                    }
                });

                TestServices.WindowHelper.WaitForIdle();

                // Swith the datatemplate on/off
                //
                UIExecutor.Execute(() =>
                {
                    if (lv.ItemTemplate == null)
                    {
                        Log.Comment("Turning on ItemTemplate...");

                        DataTemplate itemTemplate = TargetPanel.Resources["MoCoDataTemplate"] as DataTemplate;
                        Verify.IsNotNull(itemTemplate, "Unable to find the ItemTemplate from the resource");

                        lv.ItemTemplate = itemTemplate;
                    }
                    else
                    {
                        Log.Comment("Turning off ItemTemplate...");
                        lv.ItemTemplate = null;
                    }
                });

                TestServices.WindowHelper.WaitForIdle();
            }
        }

        /// <summary>
        /// Tests DataTemplateSelector API by updating an item in the ListView
        /// 
        /// Verification:
        /// When an item is updated, the ItemTemplateSelector selects the right template for that item
        /// i.,e the SelectTemplate2Core() method in DataTemplateSelector gets called
        /// 
        /// Steps:
        /// 1. Load the XAML containing Listview
        /// 2. Populate the ListView
        /// 3. Set the DataTemplateSelector to ItemTemplateSelector property
        /// 4. Update an item in the data collection
        /// 5. Verify if SelectTemplate2Core has been called
        /// </summary>
        /// 
        [TestMethod]
        [Priority(1)]
        [Description("Tests DataTemplateSelector API by updating an item in the ListView")]
        public void TestUpdatingItem()
        {
            ListView lv = null;
            const int numberOfItems = 10;
            const int updateItemIndex = 3;
            MoCoDataTemplateSelector1 dataTemplateSelector = null;
            ObservableCollection<DataObject> dataCollection = GetFlatDataCollection(numberOfItems);

            // Load Xaml
            //
            var TargetPanel = LoadXamlIntoWindow(xamlText);

            // Find the Lists and ItemsPanel
            //
            UIExecutor.Execute(() =>
            {
                Verify.IsNotNull(TargetPanel, "TargetPanel not found");

                // Find the list view
                lv = TargetPanel.FindNameInSubtree("lv") as ListView;
                Verify.IsNotNull(lv, "ListView not found");

                // Populate the list
                //
                Log.Comment("Populating list...");
                lv.ItemsSource = dataCollection;

                // Set the data template selector
                //
                Log.Comment("Setting datatemplate selector...");
                dataTemplateSelector = new MoCoDataTemplateSelector1();
                lv.ItemTemplateSelector = dataTemplateSelector;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                // Verify if the datatemplate selector is still applied
                //
                Verify.AreEqual(dataTemplateSelector, lv.ItemTemplateSelector, "Assigned datatemplate selector is not valid");

                // Clear the flag indicating the datatemplate selector has not called
                //
                Log.Comment("Clearing flag...");
                dataTemplateSelector.IsSelectTemplateCoreCalled = false;

                // Update an item with new data
                //
                Log.Comment("Updating an item...");
                //lv.Items[updateItemIndex] = new DataObject("New Item", true);
                dataCollection[updateItemIndex] = new DataObject("New Item", true);
            });

            TestServices.WindowHelper.WaitForIdle(false);

            UIExecutor.Execute(() =>
            {
                // Verify if the DataTemplateSelector api has been called
                //
                Log.Comment("Verifying if DataTemplateSelector api has been called...");
                Verify.IsTrue(dataTemplateSelector.IsSelectTemplateCoreCalled, "SelectTemplate2Core() has not been called after updating an item");
            });
        }

        /// <summary>
        /// Tests ListView items template by changing the DataTemplateSelector instance
        /// 
        /// Verification:
        /// When changing the ItemTemplateSelector, the items are updated according to the current DataTemplateSelector
        /// 
        /// Steps:
        /// 1. Load the XAML containing Listview
        /// 2. Populate the ListView
        /// 3. Create two different DataTemplateSelectors
        /// 4. Assign the first DataTemplateSelector to ItemTemplateSelector
        /// 5. Verify the Items visual
        /// 6. Assign the second DataTemplateSelector to ItemTemplateSelector
        /// 7. Verify the Items if their visual has been updated with the new DataTemplateSelector
        /// </summary>
        /// 
        [TestMethod]
        [Priority(1)]
        [Description("Tests ListView items template by changing the DataTemplateSelector instance")]
        public void TestUpdatingItemTemplateSelector()
        {
            ListView lv = null;
            const int numberOfIterations = 4;
            const int numberOfItems = 10;
            MoCoDataTemplateSelector1 dts1 = null;
            MoCoDataTemplateSelector2 dts2 = null;

            // Load Xaml
            //
            var TargetPanel = LoadXamlIntoWindow(xamlText);

            // Find the Lists and ItemsPanel
            //
            UIExecutor.Execute(() =>
            {
                Verify.IsNotNull(TargetPanel, "TargetPanel not found");

                // Find the list view
                lv = TargetPanel.FindNameInSubtree("lv") as ListView;
                Verify.IsNotNull(lv, "ListView not found");

                // Populate the list
                //
                Log.Comment("Populating list...");
                lv.ItemsSource = GetFlatDataCollection(numberOfItems);

                // Create DataTemplateSelectors
                //
                dts1 = new MoCoDataTemplateSelector1();
                dts2 = new MoCoDataTemplateSelector2();
            });

            TestServices.WindowHelper.WaitForIdle();

            // Run for multiple times switching the DataTemplate on and off
            //
            for (int c = 0; c < numberOfIterations; c++)
            {
                UIExecutor.Execute(() =>
                {
                    // Set the data template selector
                    //
                    Log.Comment("Setting datatemplate selector# " + ((c % 2) + 1) + "...");
                    lv.ItemTemplateSelector = (c % 2 == 0) ? (DataTemplateSelector)dts1 : (DataTemplateSelector)dts2;
                });

                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    // Verify if the datatemplate selector is still applied
                    //
                    Verify.AreEqual((c % 2 == 0) ? (DataTemplateSelector)dts1 : (DataTemplateSelector)dts2, lv.ItemTemplateSelector, "Assigned datatemplate selector is not valid");

                    // Verify if the assigne datatemplate selector has applied correct template on items
                    //
                    Log.Comment("Verifying the current template applied on items...");
                    for (int i = 0; i < numberOfItems; i++)
                    {
                        // Get the ListViewItem
                        //
                        SelectorItem selectorItem = lv.ContainerFromIndex(i) as SelectorItem;
                        Verify.IsNotNull(selectorItem, "Unable to access Item#" + i);

                        // Find the parent border in its visual tree
                        //
                        Border itemBorder = selectorItem.FindNameInSubtree("border") as Border;
                        Verify.IsNotNull(itemBorder, "Parent Border is not found in item#" + i);

                        // Verify its background color if it is correctly applied from the DataTemplateSelector
                        //
                        SolidColorBrush borderBrush = itemBorder.Background as SolidColorBrush;
                        Verify.IsNotNull(borderBrush, "Border's brush is not a SolidColorBrush");

                        DataObject data = lv.Items[i] as DataObject;
                        Verify.IsNotNull(data, "Unable to find the data for item#" + i);

                        if ((c % 2 == 0))
                        {
                            Verify.AreEqual((data.Flag ? Colors.Green : Colors.Red), borderBrush.Color, "DataTemplateSelector1 has not applied expected template");
                        }
                        else
                        {
                            Verify.AreEqual((data.Flag ? Colors.Brown : Colors.Orange), borderBrush.Color, "DataTemplateSelector2 has not applied expected template");
                        }
                    }
                });
            }

        }

        [TestMethod]
        public void CanHandleSentinelsInRealizationRangeWhenRecycling()
        {
            ListView list = null;
            var listLoaded = new AutoResetEvent(false);
            var data = new ObservableCollection<DataObject>(Enumerable.Range(0, 15).Select(i => new DataObject($"Item #{i}", false)));

            //
            // We will prepare a ListView with 15 items, 8 of which are realized.
            // The realization window is going to be equal to the visible window
            // because we set ItemsStackPanel.CacheLength to zero.
            // All items will initially have the same template.
            //
            // Step 1:
            // We will resize the viewport to a smaller size such as the
            // containers at the end become candidate for "lazy" recycling.
            // Step 2:
            // We will then insert a "special item" at index 1 and a regular item
            // at index 8 (i.e. right before the last realized container).
            // The special item has a different template. During the next layout pass,
            // ItemsStackPanel will have to generate a container for our special container.
            // It will try to pick one with a compatible template which we don't have.
            // In doing so, it will iterate over our sentinel. A sentinel is a recently
            // added container that hasn't been realized yet. Sentinel may appear
            // *inside* a realized range but never at its beginning or end.
            // We need to make sure we don't crash when we iterate over them.
            //

            UIExecutor.Execute(() =>
            {
                Log.Comment("Preparing ListView.");
                list = new ListView
                {
                    ItemsSource = data,
                    ItemTemplateSelector = new MoCoDataTemplateSelector1(),
                    Height = 390
                };

                list.Loaded += (s, e) =>
                {
                    Log.Comment("ListView loaded. Setting ISP.CacheLength to zero.");
                    ((ItemsStackPanel)list.ItemsPanelRoot).CacheLength = 0;
                    listLoaded.Set();
                };
                TestServices.WindowHelper.WindowContent = list;
            });

            Verify.IsTrue(listLoaded.WaitOne(TimeSpan.FromSeconds(10)), "Didn't receive loaded event from list");
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Log.Comment("Resizing the viewport to 300 so that containers from the realized range become candidate for lazy recycling.");
                list.Height = 300;
                list.UpdateLayout();

                Log.Comment("Inserting a special item (with a different template than the others) at index 1.");
                data.Insert(1, new DataObject("Special Item", true));

                Log.Comment("Inserting a sentinel in the realized range.");
                data.Insert(8, new DataObject("Sentinel", false));
                // We used to crash during this call.
                list.UpdateLayout();

                Log.Comment("Validating the realized range.");
                {
                    for (int i = 0; i < 8; ++i)
                    {
                        Verify.AreEqual(data[i], list.ItemFromContainer(list.ContainerFromIndex(i)));
                    }

                    // All containers from the sentinel to the end of the realized range should have been recycled now.
                    for (int i = 8; i < data.Count; ++i)
                    {
                        Verify.AreEqual(null, list.ContainerFromIndex(i));
                    }
                }
            });
        }

        /// <summary>
        /// Tests ListView items template using DataTemplateSelector when they are virtualized
        /// 
        /// Verification:
        /// When the items are virtualized they pick the right data template from the DataTemplateSelector's new API
        /// 
        /// Steps:
        /// 1. Load the XAML containing Listview
        /// 2. Populate the ListView with large number of items
        /// 3. Scroll to a disconnected view
        /// 4. Verify if SelectTemplate2Core() is called
        /// 5. Verify the item's template is correct
        /// </summary>
        /// 
        [TestMethod]
        [Priority(1)]
        [Description("Tests ListView items template using DataTemplateSelector when they are virtualized")]
        public void TestWithVirtualization()
        {
            ListView lv = null;
            const int numberOfItems = 20;
            MoCoDataTemplateSelector1 dts = null;
            List<int> scrollingIndices = new List<int>() { numberOfItems - 1, 0 };

            // Load Xaml
            //
            var TargetPanel = LoadXamlIntoWindow(xamlText);

            // Find the Lists and ItemsPanel
            //
            UIExecutor.Execute(() =>
            {
                Verify.IsNotNull(TargetPanel, "TargetPanel not found");

                // Find the list view
                lv = (ListView)TargetPanel.FindNameInSubtree("lv");
                Verify.IsNotNull(lv, "ListView not found");

                // Populate the list
                //
                Log.Comment("Populating list...");
                lv.ItemsSource = GetFlatDataCollection(numberOfItems);

                // Create and apply DataTemplateSelector
                //
                dts = new MoCoDataTemplateSelector1();
                lv.ItemTemplateSelector = dts;
            });

            TestServices.WindowHelper.WaitForIdle();

            foreach (int index in scrollingIndices)
            {
                UIExecutor.Execute(() =>
                {
                    // Clear the flag indicating the SelectTemplate2Core has not called
                    //
                    dts.IsSelectTemplateCoreCalled = false;

                    // Scroll to a non-virtualized item
                    //
                    Log.Comment("Scrolling to the item#" + index + "...");
                    lv.ScrollIntoView(lv.Items[index]);
                });

                TestServices.WindowHelper.WaitForIdle(false);

                UIExecutor.Execute(() =>
                {
                    // Verify if SelectTemplate2Core has been called
                    //
                    Verify.IsTrue(dts.IsSelectTemplateCoreCalled, "SelectTemplate2Core() has not been called when scrolling to item#" + index);

                    // Get the ListViewItem
                    //
                    SelectorItem selectorItem = lv.ContainerFromIndex(index) as SelectorItem;
                    Verify.IsNotNull(selectorItem, "Unable to access Item#" + index);

                    // Find the parent border in its visual tree
                    //
                    Border itemBorder = selectorItem.FindNameInSubtree("border") as Border;
                    Verify.IsNotNull(itemBorder, "Parent Border is not found in item#" + index);

                    // Verify its background color if it is correctly applied from the DataTemplateSelector
                    //
                    SolidColorBrush borderBrush = itemBorder.Background as SolidColorBrush;
                    Verify.IsNotNull(borderBrush, "Border's brush is not a SolidColorBrush");

                    DataObject data = lv.Items[index] as DataObject;
                    Verify.IsNotNull(data, "Unable to find the data for item#" + index);
                    Verify.AreEqual((data.Flag ? Colors.Green : Colors.Red), borderBrush.Color, "DataTemplateSelector has not applied expected template");
                });
            }
        }

        /// <summary>
        /// Creates and returns a flat data collection to use at the ItemsSource in ListView/GridView
        /// </summary>
        /// <param name="count">Number of items</param>
        /// <returns>Collection of flat data</returns>
        /// 
        protected ObservableCollection<DataObject> GetFlatDataCollection(int count)
        {
            ObservableCollection<DataObject> data = new ObservableCollection<DataObject>();
            for (int i = 0; i < count; i++)
            {
                data.Add(new DataObject("Item " + i, i % 2 == 0));
            }

            return data;
        }

        string xamlText = @"<StackPanel  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                                <StackPanel.Resources>
                                 <DataTemplate x:Key='MoCoDataTemplate'>
                                     <Border x:Name='border' Width='100' Height='50' Background='Blue'>
                                         <TextBlock Text='Text From DataTemplate' HorizontalAlignment='Center' VerticalAlignment='Center'/>
                                     </Border>
                                 </DataTemplate>
                                </StackPanel.Resources>

                                <Grid x:Name='Root' Width='400' Height='600'>
                                 <ListView x:Name='lv' Background='Gray'>
                                    <ListView.ItemsPanel>
                                        <ItemsPanelTemplate>
                                            <ItemsStackPanel Background='Yellow' CacheLength='0'/>
                                        </ItemsPanelTemplate>
                                    </ListView.ItemsPanel>
                                 </ListView>
                                </Grid>
                            </StackPanel>";
    }

    public partial class MoCoDataTemplateSelector1 : DataTemplateSelector
    {
        public MoCoDataTemplateSelector1()
        {
        }

        protected override DataTemplate SelectTemplateCore(object item)
        {
            IsSelectTemplateCoreCalled = true;

            if (item != null && item is DataObject)
            {
                DataObject _object = item as DataObject;

                if (_object.Flag)
                    return TrueTemplate;
                else
                    return FalseTemplate;
            }

            return null;
        }

        public bool IsSelectTemplateCoreCalled { get; set; }

        public DataTemplate TrueTemplate
        {
            get
            {
                if (_trueTemplate == null)
                {
                    _trueTemplate = (DataTemplate)XamlReader.Load(
                                       @"<DataTemplate x:Key='trueTemplate'
                                                       xmlns='http://schemas.microsoft.com/client/2007' 
                                                       xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                                              <Border x:Name='border' Background='Green' Width='100' Height='50'>
                                                  <TextBlock Text='{Binding Path=Label}' />
                                              </Border>
                                         </DataTemplate>");
                }

                return _trueTemplate;
            }
        }

        public DataTemplate FalseTemplate
        {
            get
            {
                if (_falseTemplate == null)
                {
                    _falseTemplate = (DataTemplate)XamlReader.Load(
                                       @"<DataTemplate x:Key='falseTemplate'
                                                       xmlns='http://schemas.microsoft.com/client/2007' 
                                                       xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                                             <Border x:Name='border' Background='Red' Width='100' Height='50'>
                                                 <TextBlock Text='{Binding Path=Label}' />
                                             </Border>
                                         </DataTemplate>");
                }

                return _falseTemplate;
            }
        }

        private DataTemplate _trueTemplate = null;
        private DataTemplate _falseTemplate = null;
    }

    /// <summary>
    /// DataTemplateSelector used in ListView tests
    /// </summary>
    /// 
    public partial class MoCoDataTemplateSelector2 : DataTemplateSelector
    {
        public MoCoDataTemplateSelector2()
        {
        }

        protected override DataTemplate SelectTemplateCore(object item)
        {
            IsSelectTemplateCoreCalled = true;

            if (item != null && item is DataObject)
            {
                DataObject _object = item as DataObject;

                if (_object.Flag)
                    return TrueTemplate;
                else
                    return FalseTemplate;
            }

            return null;
        }

        public bool IsSelectTemplateCoreCalled { get; set; }

        public DataTemplate TrueTemplate
        {
            get
            {
                if (_trueTemplate == null)
                {
                    _trueTemplate = (DataTemplate)XamlReader.Load(
                                       @"<DataTemplate x:Key='trueTemplate'
                                                       xmlns='http://schemas.microsoft.com/client/2007' 
                                                       xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                                              <Border x:Name='border' Background='Brown' Width='100' Height='50'>
                                                  <TextBlock Text='{Binding Path=Label}' />
                                              </Border>
                                         </DataTemplate>");
                }

                return _trueTemplate;
            }
        }

        public DataTemplate FalseTemplate
        {
            get
            {
                if (_falseTemplate == null)
                {
                    _falseTemplate = (DataTemplate)XamlReader.Load(
                                       @"<DataTemplate x:Key='falseTemplate'
                                                       xmlns='http://schemas.microsoft.com/client/2007' 
                                                       xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                                             <Border x:Name='border' Background='Orange' Width='100' Height='50'>
                                                 <TextBlock Text='{Binding Path=Label}' />
                                             </Border>
                                         </DataTemplate>");
                }

                return _falseTemplate;
            }
        }

        private DataTemplate _trueTemplate = null;
        private DataTemplate _falseTemplate = null;
    }

    public class DataObject
    {
        public DataObject(string label, bool flag)
        {
            this.Label = label;
            this.Flag = flag;
        }

        public bool Flag { get; set; }
        public string Label { get; set; }

        public override string ToString()
        {
            return string.Format("{0}:{1}", Flag, Label);
        }
    }

}
