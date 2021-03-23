using Microsoft.UI.Xaml.Controls;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using System.Text;
using Windows.ApplicationModel.DataTransfer;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Documents;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;

// The Blank Page item template is documented at https://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409

namespace BaselineResourcesGenerator
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class MainPage : Page
    {
        public MainPage()
        {
            this.InitializeComponent();
        }

        private void RetrieveResources_Click(object sender, RoutedEventArgs e)
        {
            PopulateBaselineTextBlock();
        }

        private void Copy_Click(object sender, RoutedEventArgs e)
        {
            DataPackage dataPackage = new DataPackage();
            dataPackage.SetText(BaselineTextBox.Text);

            Clipboard.SetContent(dataPackage);
        }

        private void PopulateBaselineTextBlock()
        {
            string baselineText = GenerateHeader() + FormatResourcesKeyList(RetrieveResourcesKeyList()) + GenerateFooter();
            BaselineTextBox.Text = baselineText;
        }

        private string GenerateHeader()
        {
            return @"using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Text;

namespace Windows.UI.Xaml.Tests.MUXControls.ApiTests
{
    public static class BaselineResources
    {
";
        }

        private string FormatResourcesKeyList(List<string> resourceKeyList)
        {
            StringBuilder declaration = new StringBuilder(@"public static readonly IList<String> BaselineResourcesList = new ReadOnlyCollection<string>(new List<string>{");
            
            foreach (var resourceKey in resourceKeyList)
            {
                if (!string.IsNullOrWhiteSpace(resourceKey))
                {
                    string formattedResourceKey = Environment.NewLine + "\"" + resourceKey + "\",";
                    declaration.Append(formattedResourceKey);
                }
            }
            
            declaration.Append(@"});");

            return declaration.ToString();
        }

        private string GenerateFooter()
        {
            return @"
    }
}";
        }

        private List<string> RetrieveResourcesKeyList()
        {
            var resourcesKeys = new HashSet<string>();

            var resourceDictionaries = new XamlControlsResources() { };

            foreach (var dictionaryName in resourceDictionaries.ThemeDictionaries.Keys)
            {
                var themeDictionary = resourceDictionaries.ThemeDictionaries[dictionaryName] as ResourceDictionary;

                foreach (var entry in themeDictionary)
                {
                    string entryKey = entry.Key as string;
                    if (!resourcesKeys.Contains(entryKey))
                    {
                        resourcesKeys.Add(entryKey);
                    }
                }
            }

            foreach (var entry in resourceDictionaries)
            {
                string entryKey = entry.Key as string;
                if (!resourcesKeys.Contains(entryKey))
                {
                    resourcesKeys.Add(entryKey);
                }
            }

            return resourcesKeys.ToList<string>();
        }
    }
}
