using Microsoft.UI.Xaml.Controls;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Documents;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;

// The Templated Control item template is documented at https://go.microsoft.com/fwlink/?LinkId=234235

namespace MUXControlsTestApp
{
    public sealed partial class PrototypePager : Control
    {
        public enum PagerDisplayModes { Auto, ComboBox, NumberBox, NumberPanel, }
        public enum ButtonVisibilityMode { Auto, AlwaysVisible, HiddenOnEdge, None, }

        private Button FirstPageButton, PreviousPageButton, NextPageButton, LastPageButton;
        private ComboBox PagerComboBox;

        private static string NumberBoxVisibleVisualState = "NumberBoxVisible";
        private static string ComboBoxVisibleVisualState = "ComboBoxVisible";
        private static string NumberPanelVisibleVisualState = "NumberPanelVisible";

        private int PreviousPageIndex = -1;

        public event TypedEventHandler<PrototypePager, PageChangedEventArgs> PageChanged;
        
        public PrototypePager()
        {
            this.DefaultStyleKey = typeof(PrototypePager);
            this.Loaded += (s, args) => { SetValue(TemplateSettingsProperty, new PagerTemplateSettings(this)); };

        }
        protected override void OnApplyTemplate()
        {
            // Grab UIElements for later
            FirstPageButton = GetTemplateChild("FirstPageButton") as Button;
            PreviousPageButton = GetTemplateChild("PreviousPageButton") as Button;
            NextPageButton = GetTemplateChild("NextPageButton") as Button;
            LastPageButton = GetTemplateChild("LastPageButton") as Button;
            PagerComboBox = GetTemplateChild("ComboBoxDisplay") as ComboBox;

            // Attach TestHooks
            FirstPageButtonTestHook = FirstPageButton;
            PreviousPageButtonTestHook = PreviousPageButton;
            NextPageButtonTestHook = NextPageButton;
            LastPageButtonTestHook = LastPageButton;
            NumberBoxDisplayTestHook = GetTemplateChild("NumberBoxDisplay") as NumberBox;
            ComboBoxDisplayTestHook = PagerComboBox;

            // Attach Callbacks for property changes
            RegisterPropertyChangedCallback(SelectedIndexProperty, (s,e) => {
                if (PagerComboBox != null)
                {
                    PagerComboBox.SelectedIndex = SelectedIndex - 1;
                }
                DisablePageButtonsOnEdge();
                PageChanged?.Invoke(this, new PageChangedEventArgs(PreviousPageIndex, SelectedIndex - 1));
            });
            RegisterPropertyChangedCallback(PagerDisplayModeProperty, (s,e) => { OnPagerDisplayModeChanged(); });
            RegisterPropertyChangedCallback(FirstPageButtonVisibilityProperty, (s, e) => { OnFirstPageButtonVisibilityChanged(); });
            RegisterPropertyChangedCallback(PreviousPageButtonVisibilityProperty, (s, e) => { OnPreviousPageButtonVisibilityChanged(); });
            RegisterPropertyChangedCallback(NextPageButtonVisibilityProperty, (s, e) => { OnNextPageButtonVisibilityChanged(); });
            RegisterPropertyChangedCallback(LastPageButtonVisibilityProperty, (s, e) => { OnLastPageButtonVisibilityChanged(); });

            // Attach click events
            if (FirstPageButton != null)
            {
                FirstPageButton.Click += (s, e) => { SelectedIndex = 1; };
            }
            if (PreviousPageButton != null)
            {
                PreviousPageButton.Click += (s, e) => { SelectedIndex -= 1; };
            }
            if (NextPageButton != null)
            {
                NextPageButton.Click += (s, e) => { SelectedIndex += 1; };
            }
            if (LastPageButton != null)
            {
                LastPageButton.Click += (s, e) => { SelectedIndex = NumberOfPages; };
            }
            if (PagerComboBox != null)
            {
                PagerComboBox.SelectedIndex = SelectedIndex - 1;
                PagerComboBox.SelectionChanged += (s, e) => {
                    SelectedIndex = PagerComboBox.SelectedIndex + 1; };
            }

            OnPagerDisplayModeChanged();
            // This is for the initial page being loaded whatever page that might be.
            PageChanged?.Invoke(this, new PageChangedEventArgs(PreviousPageIndex, SelectedIndex - 1));
        }
    }

    public sealed class PagerTemplateSettings : DependencyObject
    {
        public ObservableCollection<int> Pages { get; set; }
        public ObservableCollection<int> PagesZeroIndexed { get; set; }

        public PagerTemplateSettings(PrototypePager pager)
        {
            Pages = new ObservableCollection<int>(Enumerable.Range(1, pager.NumberOfPages));
            PagesZeroIndexed = new ObservableCollection<int>(Enumerable.Range(0, pager.NumberOfPages - 1));
        }
    }
}
