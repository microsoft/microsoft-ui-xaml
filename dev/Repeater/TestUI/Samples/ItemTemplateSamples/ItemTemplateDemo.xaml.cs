// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.Generic;
using System.Linq;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;

using RecyclingElementFactory = Microsoft.UI.Xaml.Controls.RecyclingElementFactory;
using SelectTemplateEventArgs = Microsoft.UI.Xaml.Controls.SelectTemplateEventArgs;

namespace MUXControlsTestApp.Samples
{
    public sealed partial class ItemTemplateDemo : Page
    {
        public List<int> Data { get; set; }
        public List<MyData> Numbers = new List<MyData>();

        public ItemTemplateDemo()
        {
            Data = Enumerable.Range(0, 1000).ToList();
            
            for(int i=0;i<10;i++)
            {
                Numbers.Add(new MyData(i));
            }

            this.InitializeComponent();
        }

        private void OnSelectTemplateKey(RecyclingElementFactory sender, SelectTemplateEventArgs args)
        {
            args.TemplateKey = (((int)args.DataContext) % 2 == 0) ? "even" : "odd";
        }
    }

    public class MyData
    {
        public int number;

        public MyData(int number)
        {
            this.number = number;
        }
    }

    public class MySelector: DataTemplateSelector
    {
        public DataTemplate TemplateOdd { get; set; }

        public DataTemplate TemplateEven { get; set; }

        protected override DataTemplate SelectTemplateCore(object item)
        {
            return (((int)item) % 2 == 0) ? TemplateEven : TemplateOdd;
        }
    }
}
