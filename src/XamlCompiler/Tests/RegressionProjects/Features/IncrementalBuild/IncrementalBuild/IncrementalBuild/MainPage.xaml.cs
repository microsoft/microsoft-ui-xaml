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
using System.ComponentModel;
using IncrementalBuildRemoteAsm;

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409

namespace IncrementalBuild
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class MainPage : Page//, INotifyPropertyChanged
    {
        private string _testString;
        private RemoteShirt _shirt = new RemoteShirt();
        public MainPage()
        {
            this.InitializeComponent();
        }

        public string TestString
        {
            get { return _testString; }
            set
            {
                _testString = value;
                RaisePropertyChanged("TestString");
            }
        }

        public RemoteShirt TestShirt
        {
            get { return _shirt; }
            set
            {
                _shirt = value;
                RaisePropertyChanged("TestShirt");
            }
        }

        public event PropertyChangedEventHandler PropertyChanged;
        protected void RaisePropertyChanged(string name)
        {
            if (PropertyChanged != null)
            {
                PropertyChanged(this, new PropertyChangedEventArgs(name));
            }
        }
    }

    public class Person : INotifyPropertyChanged
    {
        private string _name;
        private RemoteShirt _shirt;
        public string FirstName
        {
            get { return _name; }
            set
            {
                _name = value;
                RaisePropertyChanged("FirstName");
            }
        }

        public RemoteShirt Shirt
        {
            get { return _shirt; }
            set
            {
                _shirt = value;
                RaisePropertyChanged("Shirt");
            }
        }

        public event PropertyChangedEventHandler PropertyChanged;
        protected void RaisePropertyChanged(string name)
        {
            if (PropertyChanged != null)
            {
                PropertyChanged(this, new PropertyChangedEventArgs(name));
            }
        }
    }
}
