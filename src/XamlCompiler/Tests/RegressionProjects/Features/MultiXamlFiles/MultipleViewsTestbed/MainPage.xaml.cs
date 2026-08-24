using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Navigation;

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409

namespace MultipleViewsTestbed
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class MainPage : Page
    {
        string filename = null;

        public MainPage()
        {
            Window.Current.SizeChanged += Current_SizeChanged;
            this.LoadCorrectXamlFile();
        }

        private void Current_SizeChanged(object sender, Microsoft.UI.Xaml.WindowSizeChangedEventArgs e)
        {
            this.LoadCorrectXamlFile();
        }

        private void LoadCorrectXamlFile()
        {
            // Figure out which file we want, do nothing if we've already chosen that file
            string correctFilename = (Window.Current.Bounds.Width > Window.Current.Bounds.Height) ? "MainPage.xaml" : "MainPage.Portrait.xaml";
            if (correctFilename == this.filename)
            {
                return;
            }

            // Clear the results of the last file - markup compiler generated code will eventually do this part
            this.filename = correctFilename;
            this._contentLoaded = false;
            this.Resources = null;

            // Load the new file. TBD: Do you pass the whole filename or just the qualifierish part (e.g. "Portrait")
            this.InitializeComponent(new System.Uri("ms-appx:///" + this.filename));

            // Set the text. Note that MyControlText is a TextBlock in both files, and MyControl is in both files with different types
            this.DisplayText.Text = "MyControl is a " + this.MyControl.GetType().Name;
        }

        private void MyControl_Click(object sender, RoutedEventArgs e)
        {
            this.DisplayText.Text += " (Clicked)";
        }

        private void MyControl_Checked(object sender, RoutedEventArgs e)
        {
            this.DisplayText.Text += " (Checked)";
        }
    }
}
