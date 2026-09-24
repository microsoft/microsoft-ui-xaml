// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System.Collections.ObjectModel;
using System.ComponentModel;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using System.Threading;
using Windows.UI;


namespace BindTestbed
{
    internal sealed partial class PhasingTests : UserControl
    {
        private int ItemsCount = 10;
        private ObservableCollection<MyItem> myItems;
        private bool Initialized = false;
        public PhasingTests()
        {
            myItems = new ObservableCollection<MyItem>();
            CreateTestItems();
            this.InitializeComponent();
            myGridView.ItemsSource = myItems;
            //          myGridView.ItemTemplate = PhasedTemplate;
            Initialized = true;
            DetectLeaksPage.TrackObject(this);
        }

        void CreateTestItems()
        {
            for (int i = 0; i < ItemsCount; i++)
            {
                MyItem myItem = new MyItem(i,
                     "Title:" + i.ToString(), // Title.
                     "Sub:" + i.ToString(), // Subtitle.
                     "Desc:" + i.ToString(), // Description.
                      new MyInfo(
                         "ImageUrl" + i.ToString(), // ImageUrl of MyInfo
                         "Caption" + i.ToString()), // Caption of MyInfo
                      new ExtraInfo(
                         "OtherCaption" + i.ToString()
                         ),
                     "DP" + i.ToString()
                     );
                myItems.Add(myItem);
            }
        }

        private void Reset_Click(object sender, RoutedEventArgs e)
        {
            myItems.Clear();
            CreateTestItems();
        }


        private void Reload_Click(object sender, RoutedEventArgs e)
        {
            myItems = new ObservableCollection<MyItem>();
            CreateTestItems();
            myGridView.ItemsSource = myItems;
        }


        private void myGridView_ContainerContentChanging(ListViewBase sender, ContainerContentChangingEventArgs args)
        {
            wait(1);
            if (args.Phase < 20) args.RegisterUpdateCallback(myGridView_ContainerContentChanging);
        }

        private void wait(int msTime)
        {
            AutoResetEvent h = new AutoResetEvent(false);
            h.WaitOne(msTime);
        }

        private void SlowPhasing_UnChecked(object sender, RoutedEventArgs e)
        {
            SlowPhasing_Checked(sender, e);
        }

        private void SlowPhasing_Checked(object sender, RoutedEventArgs e)
        {
            if (Initialized)
            {
                if (SlowPhasing.IsChecked.Value)
                {
                    myGridView.ContainerContentChanging += myGridView_ContainerContentChanging;
                }
                else
                {
                    myGridView.ContainerContentChanging -= myGridView_ContainerContentChanging;
                }
            }
        }

        private void PhasedTemplate_UnChecked(object sender, RoutedEventArgs e)
        {
            PhasedTemplate_Checked(sender, e);
        }

        private void PhasedTemplate_Checked(object sender, RoutedEventArgs e)
        {
            if (Initialized)
            {
                if (PhasedTemplateCbx.IsChecked.Value)
                {
                    myGridView.ItemTemplate = (DataTemplate)Resources["PhasedTemplate"];
                }
                else
                {
                    myGridView.ItemTemplate = (DataTemplate)Resources["NonPhasedTemplate"];
                }
            }
        }

        private void StackPanel_PointerReleased(object sender, PointerRoutedEventArgs e)
        {
            StackPanel root = sender as StackPanel;
            root.Background = new SolidColorBrush(Microsoft.UI.Colors.White);
            root.FindName("deferedTextBlock");
            root.FindName("deferedAndPhasedTextBlock");
        }
    }


    internal class ExtraInfo
    {
        public string Caption { get; set; }

        public ExtraInfo(string caption)
        {
            Caption = caption;
        }
    }


    internal class MyInfo : INotifyPropertyChanged
    {
        public string ImageUrl { get; set; }
        public string Caption { get; set; }

        public string Prop1
        {
            get { return this.Caption; }
            set
            {
                if (this.PropertyChanged != null)
                {
                    this.PropertyChanged(this, new PropertyChangedEventArgs("Prop1"));
                }
            }
        }
        public string Prop2
        {
            get { return this.Caption; }
            set
            {
                if (this.PropertyChanged != null)
                {
                    this.PropertyChanged(this, new PropertyChangedEventArgs("Prop2"));
                }
            }
        }
        public string Prop3
        {
            get { return this.Caption; }
            set
            {
                if (this.PropertyChanged != null)
                {
                    this.PropertyChanged(this, new PropertyChangedEventArgs("Prop3"));
                }
            }
        }
        public string Prop4
        {
            get { return this.Caption; }
            set
            {
                if (this.PropertyChanged != null)
                {
                    this.PropertyChanged(this, new PropertyChangedEventArgs("Prop4"));
                }
            }
        }
        public string Prop5
        {
            get { return this.Caption; }
            set
            {
                if (this.PropertyChanged != null)
                {
                    this.PropertyChanged(this, new PropertyChangedEventArgs("Prop5"));
                }
            }
        }

        public event PropertyChangedEventHandler PropertyChanged;

        public MyInfo(string imageUrl, string caption)
        {
            ImageUrl = imageUrl;
            Caption = caption;
        }
    }
    internal class MyItem : DependencyObject, INotifyPropertyChanged
    {
        private static Microsoft.UI.Xaml.DependencyProperty _DPOnMyItemProperty = DependencyProperty.Register("DPOnMyItem", typeof(string), typeof(MyItem), null);
        private string _description;

        public int Index { get; set; }
        public string Title;
        public string Subtitle;
        public string Description
        {
            get { return _description; }
            set
            {
                if (value != _description)
                {
                    _description = value;
                    NotifyPropertyChanged("Description");
                }
            }
        }

        public MyInfo Info;
        public ExtraInfo OtherInfo;

        public string DPOnMyItem
        {
            get { return (string)GetValue(DPOnMyItemProperty); }
            set
            {
                SetValue(DPOnMyItemProperty, value);
            }
        }

        public static Microsoft.UI.Xaml.DependencyProperty DPOnMyItemProperty
        {
            get { return _DPOnMyItemProperty; }
            set { _DPOnMyItemProperty = value; }
        }
        public MyItem(int index, string title, string subtitle, string description, MyInfo info, ExtraInfo otherInfo, string dp)
        {
            Index = index;
            Title = title;
            Subtitle = subtitle;
            Description = description;
            Info = info;
            OtherInfo = otherInfo;
            DPOnMyItem = dp;
        }

        // Fired when properties change
        public event PropertyChangedEventHandler PropertyChanged;


        private void NotifyPropertyChanged(string propertyName)
        {
            if (PropertyChanged != null)
            {
                PropertyChangedEventArgs args = new PropertyChangedEventArgs(propertyName);
                PropertyChanged(this, args);
            }
        }
    }
}