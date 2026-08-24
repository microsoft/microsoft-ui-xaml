// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;

namespace ManagedDesktopSample
{
    /// <summary>
    /// Interaction logic for MainPage.xaml
    /// </summary>
    public partial class MainPage : Page, System.ComponentModel.INotifyPropertyChanged
    {
        private string _text;

        public MainPage()
        {
            _text = "initial x:Bind text value";
            this.InitializeComponent();
        }

       public async void UpdateTextBlockAsync(object sender, RoutedEventArgs e)
       {
            string updateText = await Task.Run(() =>
            {
                Thread.Sleep(1000);
                return "Updated text from async routine!!";
            });

            text.Text = updateText;
       }

       public event PropertyChangedEventHandler PropertyChanged;

       public string Text
       {
            get { return this._text; }
            set
            {
                this._text = value;
                this.OnPropertyChanged();
            }
        }

        public void OnPropertyChanged([CallerMemberName] string propertyName = null)
        {
            // Raise the PropertyChanged event, passing the name of the property whose value has changed.
            this.PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }
}
