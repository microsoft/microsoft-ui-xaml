// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using Private.Infrastructure;
using System;
using System.Threading;
using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Tests.Common;
using Microsoft.UI.Xaml.Markup;

namespace Microsoft.UI.Xaml.Tests
{
    [TestClass]
    public class ElementFactoryTests : XamlTestsBase
    {
        static string TestDeploymentDir { get; set; }

        [ClassInitialize]
        [TestProperty("BinaryUnderTest", "Microsoft.UI.Xaml.dll")]
        [TestProperty("RunAs", "UAP")]
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
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")] // Catgates - ApplyManifestPhone.exe doesn't run on OneCore
        [TestProperty("Hosting:Mode", "UAP")]
        public void TestDataTemplateGeneration()
        {
            UIExecutor.Execute(() =>
            {
                var evenTemplate = (DataTemplate)XamlReader.Load(@"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                                                                    <Button Content='Even'/>
                                                                   </DataTemplate>");
                var oddTemplate = (DataTemplate)XamlReader.Load(@"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                                                                    <Button Content='Odd'/>
                                                                   </DataTemplate>");

                var tree0 = GetElement(evenTemplate, null, 0);
                Verify.IsNotNull(tree0);
                var button0 = tree0 as Button;
                Verify.AreEqual(button0.Content, "Even");
                Verify.IsNull(button0.DataContext);

                RecycleElement(evenTemplate, tree0, null);
            });
        }

        [TestMethod]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")] // Catgates - ApplyManifestPhone.exe doesn't run on OneCore
        public void TestDataTemplateSelectorGeneration()
        {
            UIExecutor.Execute(() =>
            {
                var evenTemplate = (DataTemplate)XamlReader.Load(@"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                                                                    <Button Content='Even'/>
                                                                   </DataTemplate>");
                var oddTemplate = (DataTemplate)XamlReader.Load(@"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                                                                    <Button Content='Odd'/>
                                                                   </DataTemplate>");

                // Tests for Selector with SelectTemplateCore(obj item)
                {
                    var selector = new MyDataTemplateSelector()
                    {
                        EvenTemplate = evenTemplate,
                        OddTemplate = oddTemplate
                    };

                    var tree0 = GetElement(selector, null, 0);
                    Verify.IsNotNull(tree0);
                    var button0 = tree0 as Button;
                    Verify.AreEqual(button0.Content, "Even");
                    Verify.IsNull(button0.DataContext);

                    var tree1 = GetElement(selector, null, 1);
                    Verify.IsNotNull(tree1);
                    var button1 = tree1 as Button;
                    Verify.AreEqual(button1.Content, "Odd");
                    Verify.IsNull(button1.DataContext);

                    RecycleElement(selector, tree0, null);
                    RecycleElement(selector, tree1, null);

                    Verify.AreSame(tree0, GetElement(selector, null, 0));
                    Verify.AreSame(tree1, GetElement(selector, null, 1));
                }

                // Tests for Selector with SelectTemplateCore(obj item, DependencyObject container)
                // We fallback to this overload if the one with just object is not impl or returns null.
                {
                    var selector = new MyDataTemplateSelectorWithContainer()
                    {
                        EvenTemplate = evenTemplate,
                        OddTemplate = oddTemplate
                    };

                    var tree0 = GetElement(selector, null, 0);
                    Verify.IsNotNull(tree0);
                    var button0 = tree0 as Button;
                    Verify.AreEqual(button0.Content, "Even");
                    Verify.IsNull(button0.DataContext);

                    var tree1 = GetElement(selector, null, 1);
                    Verify.IsNotNull(tree1);
                    var button1 = tree1 as Button;
                    Verify.AreEqual(button1.Content, "Odd");
                    Verify.IsNull(button1.DataContext);

                    RecycleElement(selector, tree0, null);
                    RecycleElement(selector, tree1, null);

                    Verify.AreSame(tree0, GetElement(selector, null, 0));
                    Verify.AreSame(tree1, GetElement(selector, null, 1));
                }
            });
        }


        [TestMethod]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")] // Catgates - ApplyManifestPhone.exe doesn't run on OneCore
        [TestProperty("Hosting:Mode", "UAP")]   // DCPP: Test fails on WPF from failure to activate DMManager
        public void VerifyElementFromDataTemplateSelectorGetsArranged()
        {
            // Regression coverage for: 
            // Repeater doesn't render content when using DataTemplateSelector
            // We want to verify that when a ContentControl (such as Button) is used as the root of a DataTemplate from a DataTemplateSelector
            // that the content of the ContentControl gets Arranged.
            
            StackPanel rootPanel = null;
            var rootPanelLoaded = new AutoResetEvent(false);
            
            UIExecutor.Execute(() =>
            {
                rootPanel = new StackPanel();
                rootPanel.Loaded += (s,e) => { rootPanelLoaded.Set(); };
                Window.Current.Content = rootPanel;
            });
            Verify.IsTrue(rootPanelLoaded.WaitOne(TimeSpan.FromSeconds(5)), "Wait for root panel to load");
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                var dataTemplate = (DataTemplate)XamlReader.Load(
                                        @"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                                             <Button Width='100' Height='100' HorizontalContentAlignment='Stretch' VerticalContentAlignment='Stretch' 
                                                     BorderThickness='0' Padding='0'>
                                                <Grid HorizontalAlignment='Stretch' VerticalAlignment='Stretch' Background='Red' />
                                             </Button>
                                          </DataTemplate>");

                var selector = new MyDataTemplateSelector()
                {
                    EvenTemplate = dataTemplate,
                    OddTemplate = dataTemplate
                };

                var item = GetElement(selector, null, 0) as ContentControl;
                rootPanel.Children.Add(item);
                rootPanel.UpdateLayout();
            
                var content = item.Content as Grid;
                Verify.IsNotNull(content);
                Verify.AreEqual(100, content.ActualWidth, "content.ActualWidth");
                Verify.AreEqual(100, content.ActualHeight, "content.ActualHeight");
            });
        }

        [TestMethod]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")] // Catgates - ApplyManifestPhone.exe doesn't run on OneCore
        [TestProperty("Hosting:Mode", "UAP")]   // DCPP: Test fails on WPF from failure to activate DMManager
        public void VerifyRecyclingBetweenTemplateAndSelector()
        {
            // Getting an element directly from the template and then recycling it through a selector should work.
            StackPanel rootPanel = null;
            var rootPanelLoaded = new AutoResetEvent(false);

            UIExecutor.Execute(() =>
            {
                rootPanel = new StackPanel();
                rootPanel.Loaded += (s, e) => { rootPanelLoaded.Set(); };
                Window.Current.Content = rootPanel;
            });
            Verify.IsTrue(rootPanelLoaded.WaitOne(TimeSpan.FromSeconds(5)), "Wait for root panel to load");
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                var dataTemplate = (DataTemplate)XamlReader.Load(
                                        @"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                                             <Button Width='100' Height='100' HorizontalContentAlignment='Stretch' VerticalContentAlignment='Stretch' 
                                                     BorderThickness='0' Padding='0'>
                                                <Grid HorizontalAlignment='Stretch' VerticalAlignment='Stretch' Background='Red' />
                                             </Button>
                                          </DataTemplate>");

                var selector = new MyDataTemplateSelector()
                {
                    EvenTemplate = dataTemplate,
                    OddTemplate = dataTemplate
                };

                // Get element through the template directly
                var item = GetElement(dataTemplate, null, 0) as ContentControl;
                // Recycle it through the selector
                RecycleElement(selector, item, null);
                // Get it back throught the template, it should have isRealized set to true.
                item = GetElement(dataTemplate, null, 0) as ContentControl;

                rootPanel.Children.Add(item);
                rootPanel.UpdateLayout();
                
                // Verify that layout was run on the item as a side effect of isRealized being true since 
                // isRealized/VirtualizationInfo are not public.
                var content = item.Content as Grid;
                Verify.IsNotNull(content);
                Verify.AreEqual(100, content.ActualWidth, "content.ActualWidth");
                Verify.AreEqual(100, content.ActualHeight, "content.ActualHeight");
            });
        }

        [TestMethod]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")] // Catgates - ApplyManifestPhone.exe doesn't run on OneCore
        public void TestDataTemplateRecycling()
        {
            UIExecutor.Execute(() =>
            {
                var template = (DataTemplate)XamlReader.Load(@"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                                                                 <Button Content='{Binding}'/>
                                                               </DataTemplate>");
                
                var tree0 = GetElement(template, null);
                Verify.IsNotNull(tree0);
                var button0 = tree0 as Button;

                RecycleElement(template, tree0, null);

                var recycledTree = GetElement(template, null);
                Verify.AreSame(tree0, recycledTree);
            });
        }

        [TestMethod]
        public void ValidateOwnershipWithStackPanel()
        {
            UIExecutor.Execute(() =>
            {
                var template = (DataTemplate)XamlReader.Load(@"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                                                                 <Button Content='{Binding}'/>
                                                               </DataTemplate>");

                var parent = new StackPanel();
                var child = new Button() { Content = "child" };
                parent.Children.Add(child);

                // add one element to the cache [parent, child]
                RecycleElement(template, child, parent);

                // ask for element with [parent]
                var recycledTree = GetElement(template, parent);
                Verify.AreSame(child, recycledTree);
                Verify.AreEqual(0, parent.Children.IndexOf(child));

                var parent2 = new StackPanel();
                var child2 = new Button() { Content = "child 2" };
                parent2.Children.Add(child2);

                // add one element to the cache [parent, child]
                RecycleElement(template, child, parent);

                // ask for element with [parent2], should get the existing one
                // and it will be removed from previous parent.
                var recycledTree2 = GetElement(template, parent2);
                Verify.AreSame(child, recycledTree2);
                Verify.AreEqual(0, parent.Children.Count);
            });
        }

        [TestMethod]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")] // This test only runs on Desktop
        public void TestDataTemplateRecyclingOwnerAffinity()
        {
            UIExecutor.Execute(() =>
            {
                var template = (DataTemplate)XamlReader.Load(@"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                                                                 <Button Content='{Binding}'/>
                                                               </DataTemplate>");

                var parent0 = new StackPanel();
                var parent0_element0 = GetElement(template, parent0);
                var parent0_element1 = GetElement(template, parent0);
                var parent0_element2 = GetElement(template, parent0);
                parent0.Children.Add(parent0_element0);
                parent0.Children.Add(parent0_element1);
                parent0.Children.Add(parent0_element2);

                var parent1 = new StackPanel();
                var parent1_element0 = GetElement(template, parent1);
                var parent1_element1 = GetElement(template, parent1);
                var parent1_element2 = GetElement(template, parent1);
                parent1.Children.Add(parent1_element0);
                parent1.Children.Add(parent1_element1);
                parent1.Children.Add(parent1_element2);

                // recycle in intermixed order of parents
                RecycleElement(template, parent0_element0, parent0);
                RecycleElement(template, parent1_element0, parent1);
                RecycleElement(template, parent0_element1, parent0);
                RecycleElement(template, parent1_element1, parent1);
                RecycleElement(template, parent0_element2, parent0);
                RecycleElement(template, parent1_element2, parent1);

                var recycled0_parent0 = GetElement(template, parent0);
                var recycled1_parent0 = GetElement(template, parent0);
                var recycled2_parent0 = GetElement(template, parent0);

                var recycled0_parent1 = GetElement(template, parent1);
                var recycled1_parent1 = GetElement(template, parent1);
                var recycled2_parent1 = GetElement(template, parent1);

                // no element should have been removed from their parent.
                Verify.AreEqual(parent0.Children.Count, 3);
                Verify.AreEqual(parent1.Children.Count, 3);

                Verify.IsTrue(parent0.Children.Contains(recycled0_parent0));
                Verify.IsTrue(parent0.Children.Contains(recycled1_parent0));
                Verify.IsTrue(parent0.Children.Contains(recycled2_parent0));
                Verify.IsTrue(parent1.Children.Contains(recycled0_parent1));
                Verify.IsTrue(parent1.Children.Contains(recycled1_parent1));
                Verify.IsTrue(parent1.Children.Contains(recycled2_parent1));
            });
        }

        private void RecycleElement(IElementFactory factory, UIElement element, UIElement parent)
        {
            var args = new ElementFactoryRecycleArgs();
            args.Element = element;
            args.Parent = parent;
            factory.RecycleElement(args);
        }

        private UIElement GetElement(IElementFactory factory, UIElement parent, object data =null)
        {
            var args = new ElementFactoryGetArgs();
            args.Parent = parent;
            args.Data = data;
            return factory.GetElement(args);
        }
    }

    public partial class MyDataTemplateSelector : DataTemplateSelector
    {
        public DataTemplate OddTemplate { get; set; }

        public DataTemplate EvenTemplate { get; set; }

        protected override DataTemplate SelectTemplateCore(object item)
        {
            return (int.Parse(item.ToString()) % 2 == 0) ? EvenTemplate : OddTemplate;
        }
    }

    public partial class MyDataTemplateSelectorWithContainer : DataTemplateSelector
    {
        public DataTemplate OddTemplate { get; set; }

        public DataTemplate EvenTemplate { get; set; }

        protected override DataTemplate SelectTemplateCore(object item, DependencyObject container)
        {
            return (int.Parse(item.ToString()) % 2 == 0) ? EvenTemplate : OddTemplate;
        }
    }
}
