using Microsoft.UI.Xaml.Controls;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.UI;
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
        private ItemsRepeater NumberPanelItems;
        private ObservableCollection<object> NumberPanelCurrentItems = new ObservableCollection<object>();

        //private bool LeftEllipseEnabled = false;
        //private bool RightEllipseEnabled = false;
        
        private SymbolIcon Ellipse = new SymbolIcon(Symbol.More) {
            Tag = Symbol.More.ToString()
        };
        private static string NumberBoxVisibleVisualState = "NumberBoxVisible";
        private static string ComboBoxVisibleVisualState = "ComboBoxVisible";
        private static string NumberPanelVisibleVisualState = "NumberPanelVisible";
        private static string[] FirstPageButtonStates = new string[] { "FirstPageButtonVisible", "FirstPageButtonCollapsed",
                                                                       "FirstPageButtonEnabled", "FirstPageButtonDisabled" };
        private static string[] PreviousPageButtonStates = new string[] { "PreviousPageButtonVisible", "PreviousPageButtonCollapsed",
                                                                          "PreviousPageButtonEnabled", "PreviousPageButtonDisabled" };
        private static string[] NextPageButtonStates = new string[] { "NextPageButtonVisible", "NextPageButtonCollapsed",
                                                                      "NextPageButtonEnabled", "NextPageButtonDisabled" };
        private static string[] LastPageButtonStates = new string[] { "LastPageButtonVisible", "LastPageButtonCollapsed",
                                                                      "LastPageButtonEnabled", "LastPageButtonDisabled" };

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
            NumberPanelItems = GetTemplateChild("NumberPanelItemsRepeater") as ItemsRepeater;
            
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

            NumberPanelItems.Loaded += (s, e) => { InitializeNumberPanel(); };
            NumberPanelItems.ElementPrepared += OnElementPrepared;
            NumberPanelItems.ElementClearing += OnElementClearing;

            // This is for the initial page being loaded whatever page that might be.
            PageChanged?.Invoke(this, new PageChangedEventArgs(PreviousPageIndex, SelectedIndex - 1));
        }

        private void InitializeNumberPanel()
        {
            NumberPanelItems.ItemsSource = NumberPanelCurrentItems;

            if (NumberOfPages <= 7)
            {
                foreach(var num in TemplateSettings.Pages.GetRange(0, NumberOfPages))
                {
                    NumberPanelCurrentItems.Add(num);
                }
            } else
            {
                RegisterPropertyChangedCallback(SelectedIndexProperty, (s, e) => { 
                    UpdateNumberPanel();
                    Debug.WriteLine((NumberPanelItems.TryGetElement(SelectedIndex - 1) as Button)?.Content);
                    Debug.WriteLine((NumberPanelItems.TryGetElement(PreviousPageIndex) as Button)?.Content);
                });
                //RightEllipseEnabled = true;
                foreach (var num in TemplateSettings.Pages.GetRange(0, 5))
                {
                    NumberPanelCurrentItems.Add(num);
                }
                NumberPanelCurrentItems.Add(Ellipse);
                NumberPanelCurrentItems.Add(NumberOfPages);

            }
        }
    }

    public sealed class PagerTemplateSettings : DependencyObject
    {
        public List<object> Pages { get; set; }

        public PagerTemplateSettings(PrototypePager pager)
        {
            Pages = new List<object>(Enumerable.Range(1, pager.NumberOfPages).Cast<object>());
        }
    }
}
