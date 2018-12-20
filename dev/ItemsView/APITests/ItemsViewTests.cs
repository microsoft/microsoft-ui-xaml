using DEPControlsTestApp.ItemsViewPrototype;
using DEPControlsTestApp.Utilities;
using Microsoft.UI.Xaml.Controls;
using System.Linq;
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Markup;

namespace DEPControls.ApiTests.ItemsViewTests
{
    [TestClass]
    class ItemsViewTests : TestsBase
    {
        [TestMethod]
        public void ValidateItemsView()
        {
            RunOnUIThread.Execute(() =>
            {
                var viewGenerator = new RecyclingViewGenerator();
                viewGenerator.RecyclePool = new RecyclePool();
                viewGenerator.Templates["Item"] = (DataTemplate)XamlReader.Load(
                    @"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'> 
                          <TextBlock Text='{Binding}' Height='50' />
                      </DataTemplate>");

                var itemsView = new ItemsView()
                {
                    ItemsSource = Enumerable.Range(0, 10).Select(i => string.Format("Item #{0}", i)),
                    ViewDefinition = new ViewDefinition()
                    {
                        ViewGenerator = viewGenerator,
                        Header = new TextBlock() { Text = "Header" },
                        Layout = new StackLayout()
                    }
                };
                
                Content = itemsView;

                Content.UpdateLayout();

                for (int i = 0; i < 10; i++)
                {
                    var element = itemsView.GetElement(i);
                    Verify.IsNotNull(element);
                    Verify.AreEqual(string.Format("Item #{0}", i), ((TextBlock)element).Text);
                    Verify.AreEqual(i, itemsView.GetElementIndex(element));
                }

                Verify.IsNull(itemsView.GetElement(20));
            });
        }
    }
}
