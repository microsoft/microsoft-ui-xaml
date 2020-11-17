// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Windows.UI.Xaml.Controls;
using System.Collections.Generic;
using System.Linq;
using PipsPagerButtonVisibility = Microsoft.UI.Xaml.Controls.PipsPagerButtonVisibility;
using PipsPagerSelectedIndexChangedEventArgs = Microsoft.UI.Xaml.Controls.PipsPagerSelectedIndexChangedEventArgs;

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name = "PipsPager")]
    public sealed partial class PipsPagerPage : TestPage
    {
        public List<string> Pictures = new List<string>()
        {

            "Assets/ingredient1.png",
            "Assets/ingredient2.png",
            "Assets/ingredient3.png",
            "Assets/ingredient4.png",
            "Assets/ingredient5.png",
            "Assets/ingredient6.png",
            "Assets/ingredient7.png",
            "Assets/ingredient8.png",
        };

        public PipsPagerPage()
        {
            this.InitializeComponent();
            
        }

        public void OnPreviousPageButtonVisibilityChanged(object sender, SelectionChangedEventArgs e)
        {
           testPipsPager.PreviousButtonVisibility = ConvertComboBoxItemToVisibilityEnum((sender as ComboBox).SelectedItem as ComboBoxItem, testPipsPager.PreviousButtonVisibility);
        }
        public void OnNextPageButtonVisibilityChanged(object sender, SelectionChangedEventArgs e)
        {
           testPipsPager.NextButtonVisibility = ConvertComboBoxItemToVisibilityEnum((sender as ComboBox).SelectedItem as ComboBoxItem, testPipsPager.NextButtonVisibility);
        }
        public void OnNumberOfPagesChanged(object sender, SelectionChangedEventArgs e)
        {
            testPipsPager.NumberOfPages = ConvertComboBoxItemToNumberOfPages((sender as ComboBox).SelectedItem as ComboBoxItem);   
        }
        public void OnMaxDisplayedPagesChanged(object sender, SelectionChangedEventArgs e)
        {
            testPipsPager.MaxDisplayedPages = ConvertComboBoxItemToNumberOfPages((sender as ComboBox).SelectedItem as ComboBoxItem);
        }

        public void OnSelectedIndexChanged(object sender, PipsPagerSelectedIndexChangedEventArgs args)
        {
            PreviousIndexTextBlock.Text = $"Current index is: {args.NewPageIndex}";
            CurrentIndexTextBlock.Text = $"Previous index is: {args.PreviousPageIndex}";
        }

        private PipsPagerButtonVisibility ConvertComboBoxItemToVisibilityEnum(ComboBoxItem item, PipsPagerButtonVisibility defaultValue)
        {
            PipsPagerButtonVisibility newVisibility = defaultValue;
            string selectedItem = item.Content as string;
            if (!Enum.TryParse<PipsPagerButtonVisibility>(selectedItem, out newVisibility))
            {
                System.Diagnostics.Debug.WriteLine("Unable to convert " + selectedItem + " to PipsPagerButtonVisibility Enum");
            }

            return newVisibility;
        }
        private int ConvertComboBoxItemToNumberOfPages(ComboBoxItem item)
        {
            int numberOfPages = -1;
            string digitsOnlyString = new String((item.Content as string).Where(Char.IsDigit).ToArray());
            Int32.TryParse(digitsOnlyString, out numberOfPages);
            return numberOfPages;
        }
    }
}
