// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using System.Threading;
using System.Threading.Tasks;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.UI;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Navigation;

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409

namespace Simple
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class MainPage : Page
    {
        public MainPage()
        {
            // Connect must actively clear default-constructed strings, not leave stale field values.
            this.NamedEmptyString = "Must be cleared by Connect";
            this.NamedEmptyInlineString = "Must be cleared by Connect";
            this.InitializeComponent();
        }

        protected override void OnNavigatedTo(NavigationEventArgs e)
        {
            base.OnNavigatedTo(e);
            this.RunTestScenario(e.Parameter as string);
        }

        private void ClickHandler(object sender, RoutedEventArgs e) { }

        private void TappedHandler(object sender, TappedRoutedEventArgs e) { }

        #region RuntimeTestCode
        async void RunTestScenario(string navigationParameter)
        {
            // First allow the UI to complete rendering the page and 
            // the test process to open this process and retrieves a handle.
            await this.ReleaseUIThreadForFewSeconds();

            // Execute the validation. Note that we may be running on a worker thread
            // here, which cannot access UI Elements, so use the dispatcher to execute the 
            // validation on the UI Thread.

            this.DispatcherQueue.TryEnqueue(() =>
            {

                // We are called normally, no explicit background value to check.
                if (string.IsNullOrEmpty(navigationParameter))
                {
                    // Grid color should be white normally.
                    HardAssert(((SolidColorBrush)(this.MainGrid.Background)).Color.Equals(Color.FromArgb(255, 255, 255, 255)),
                        "Grid color should be white normally.");
                    Console.WriteLine("Default background test passed.");
                }
                else if (navigationParameter == "IncrementalTest")
                {
                    // Grid color should be black now.
                    HardAssert(((SolidColorBrush)(this.MainGrid.Background)).Color.Equals(Color.FromArgb(255, 0, 0, 0)),
                        "Grid color should be black after the incremental change.");
                    Console.WriteLine("Incremental background test passed.");
                }

                // Check compiled fields and namescope lookup before the test process exits.
                this.ValidateNamedStrings();
                ValidateStringNameLookup();
                Console.WriteLine("Named string tests passed.");

                // Exit the app
                Application.Current.Exit();
            });

        }

        private async Task ReleaseUIThreadForFewSeconds()
        {
            await Task.Delay(TimeSpan.FromSeconds(5));
        }

        private void ValidateNamedStrings()
        {
            // Compare generated fields, resource values, and namescope lookups for the same strings.
            string resource = this.NamedResourceString;
            string empty = this.NamedEmptyString;
            string inline = this.NamedInlineString;
            // Unnamed strings provide the baseline for default-construction behavior.
            var unnamed = (Grid)Microsoft.UI.Xaml.Markup.XamlReader.Load(@"
<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
    <Grid.Resources><x:String x:Key='emptyString'/></Grid.Resources>
    <TextBlock><TextBlock.Text><x:String/></TextBlock.Text></TextBlock>
</Grid>");
            HardAssert(resource == "Resource value", "The named resource field must contain its initialized value.");
            HardAssert(empty == null, "A named default-constructed string must not gain an initialization value.");
            HardAssert(inline == "Inline value", "The named inline field must contain its initialized value.");
            HardAssert(this.NamedEmptyInlineString == null, "A named inline string must retain its default construction.");
            HardAssert((string)this.Resources["NamedResourceString"] == resource, "The implicit resource key must preserve the named value.");
            // Default resources can be absent or null; naming must preserve either representation.
            bool namedHasValue = this.Resources.TryGetValue("emptyString", out object namedValue);
            bool unnamedHasValue = unnamed.Resources.TryGetValue("emptyString", out object unnamedValue);
            HardAssert(namedHasValue == unnamedHasValue && Object.Equals(namedValue, unnamedValue), "Naming must not change default resource lookup behavior.");
            HardAssert(this.NamedStringTextBlock.Text == inline, "TextBlock.Text and its named string field must contain the same value.");
            HardAssert(this.EmptyStringTextBlock.Text == ((TextBlock)unnamed.Children[0]).Text, "Naming must not change a default string assigned to TextBlock.Text.");
            HardAssert((string)this.FindName("NamedResourceString") == resource, "The resource name must resolve to the initialized value.");
            HardAssert(Object.Equals(this.FindName("NamedEmptyString"), Microsoft.UI.Xaml.Markup.XamlReader.Load(
                "<x:String xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'/>")), "Named and unnamed default strings must have equivalent boxed values.");
            HardAssert((string)this.FindName("NamedInlineString") == inline, "The inline name must resolve to the initialized value.");
            HardAssert((string)this.FindName("NamedEmptyInlineString") == String.Empty, "The empty inline name must resolve to an empty string.");
            HardAssert(Object.ReferenceEquals(this.FindName("NamedStringTextBlock"), this.NamedStringTextBlock), "Control lookup must preserve object identity.");
            HardAssert(Object.ReferenceEquals(this.FindName("propertyBagObject"), this.propertyBagObject), "Custom object lookup must preserve object identity.");
            HardAssert(this.FindName("MissingName") == null, "An absent name must still return null.");
        }

        internal static void ValidateStringNameLookup()
        {
            // A classless compiled fixture exercises FindName without generated fields or callbacks.
            var compiledRoot = new Grid();
            Application.LoadComponent(compiledRoot, new Uri("ms-appx:///NamedStrings.xaml"), ComponentResourceLocation.Application);
            Console.WriteLine("Checking named string lookups.");
            HardAssert(compiledRoot.FindName("text") is string compiledText && compiledText == "Compiled string", "FindName must resolve a compiled resource string.");
            HardAssert(compiledRoot.FindName("empty") is string compiledEmpty && compiledEmpty == String.Empty, "FindName must resolve a compiled empty string.");
            HardAssert(Object.ReferenceEquals(compiledRoot.FindName("button"), compiledRoot.Children[0]), "Compiled control lookup must preserve object identity.");
            // Loose markup also checks that saved names preserve language handling and custom peers.
            var root = (Grid)Microsoft.UI.Xaml.Markup.XamlReader.Load(@"
<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
      xmlns:local='using:Simple'>
    <Grid.Resources>
        <x:String x:Name='text'>Loose string</x:String>
        <x:String x:Name='empty'></x:String>
        <local:PropertyBag x:Name='custom' StringProp='Custom value' />
    </Grid.Resources>
    <Button x:Name='button' xml:lang='fr-FR' />
    <Button xml:lang='fr-FR' />
</Grid>");
            HardAssert(root.FindName("text") is string text && text == "Loose string", "FindName must return a string.");
            HardAssert(root.FindName("empty") is string empty && empty == String.Empty, "FindName must return an empty string, not null.");
            HardAssert(Object.ReferenceEquals(root.FindName("button"), root.Children[0]), "Loose-markup control lookup must preserve object identity.");
            HardAssert(((Button)root.FindName("button")).Language == ((Button)root.Children[1]).Language,
                "A saved name must not change language handling.");
            HardAssert(root.FindName("custom") is PropertyBag custom && custom.StringProp == "Custom value" &&
                Object.ReferenceEquals(custom, root.Resources["custom"]), "Saved names must preserve custom CSharp object identity.");
            HardAssert(root.FindName("MissingName") == null, "An absent loose-markup name must return null.");
            // Reusing names in another root must not cross namescope boundaries.
            var otherRoot = (Grid)Microsoft.UI.Xaml.Markup.XamlReader.Load(@"
<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
    <Grid.Resources><x:String x:Name='text'>Other string</x:String></Grid.Resources>
    <Button x:Name='button' />
</Grid>");
            HardAssert(otherRoot.FindName("text") is string otherText && otherText == "Other string", "String names must resolve within their own namescope.");
            HardAssert((string)root.FindName("text") == "Loose string", "A second namescope must not replace the first scope's value.");
            HardAssert(Object.ReferenceEquals(otherRoot.FindName("button"), otherRoot.Children[0]), "Control names must remain isolated between roots.");
        }

        private static void HardAssert(bool expression, string message = null)
        {
            if (!expression)
            {
                Console.Error.WriteLine(message ?? "Runtime assertion failed.");
                throw new InvalidOperationException(message);
            }
        }

        #endregion
    }

    public class PropertyBag
    {
        public String StringProp { get; set; }
        public Int32 IntProp { get; set; }
        public Double DoubleProp { get; set; }
        public Boolean BooleanProp { get; set; }
    }
}
