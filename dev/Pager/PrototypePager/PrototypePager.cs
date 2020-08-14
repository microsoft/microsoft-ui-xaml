using Microsoft.UI.Xaml.Controls;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.UI;
using Windows.UI.Core;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Documents;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Shapes;

// The Templated Control item template is documented at https://go.microsoft.com/fwlink/?LinkId=234235

namespace MUXControlsTestApp
{
    public sealed partial class PrototypePager : Control
    {
        public enum PagerDisplayModes { Auto, ComboBox, NumberBox, NumberPanel, }
        public enum ButtonVisibilityMode { Auto, AlwaysVisible, HiddenOnEdge, None, }

        private Button FirstPageButton, PreviousPageButton, NextPageButton, LastPageButton;
        private ComboBox PagerComboBox;
        private NumberBox PagerNumberBox;
        private ItemsRepeater NumberPanelItems;
        private Rectangle NumberPanelCurrentPageIdentifier;
        private ObservableCollection<object> NumberPanelCurrentItems = new ObservableCollection<object>();

        private IconElement LeftEllipse = new SymbolIcon(Symbol.More);
        private IconElement RightEllipse = new SymbolIcon(Symbol.More);

        private static string NumberBoxVisibleVisualState = "NumberBoxVisible";
        private static string ComboBoxVisibleVisualState = "ComboBoxVisible";
        private static string NumberPanelVisibleVisualState = "NumberPanelVisible";

        private static string FirstPageButtonVisibleVisualState = "FirstPageButtonVisible";
        private static string FirstPageButtonNotVisibleVisualState = "FirstPageButtonCollapsed";
        private static string FirstPageButtonEnabledVisualState = "FirstPageButtonEnabled";
        private static string FirstPageButtonDisabledVisualState = "FirstPageButtonDisabled";

        private static string PreviousPageButtonVisibleVisualState = "PreviousPageButtonVisible";
        private static string PreviousPageButtonNotVisibleVisualState = "PreviousPageButtonCollapsed";
        private static string PreviousPageButtonEnabledVisualState = "PreviousPageButtonEnabled";
        private static string PreviousPageButtonDisabledVisualState = "PreviousPageButtonDisabled";

        private static string NextPageButtonVisibleVisualState = "NextPageButtonVisible";
        private static string NextPageButtonNotVisibleVisualState = "NextPageButtonCollapsed";
        private static string NextPageButtonEnabledVisualState = "NextPageButtonEnabled";
        private static string NextPageButtonDisabledVisualState = "NextPageButtonDisabled";

        private static string LastPageButtonVisibleVisualState = "LastPageButtonVisible";
        private static string LastPageButtonNotVisibleVisualState = "LastPageButtonCollapsed";
        private static string LastPageButtonEnabledVisualState = "LastPageButtonEnabled";
        private static string LastPageButtonDisabledVisualState = "LastPageButtonDisabled";

        private int PreviousPageIndex = -1;

        public event TypedEventHandler<PrototypePager, PageChangedEventArgs> PageChanged;
        
        public PrototypePager()
        {
            this.DefaultStyleKey = typeof(PrototypePager);
            this.Loaded += OnLoad;
        }

        private void OnLoad(object sender, RoutedEventArgs args)
        {
            SetValue(TemplateSettingsProperty, new PagerTemplateSettings(this));
            // Attach Callbacks for property changes
            RegisterPropertyChangedCallback(NumberOfPagesProperty, (s, e) => { OnNumberOfPagesChanged(); });
            RegisterPropertyChangedCallback(SelectedIndexProperty, (s, e) => { OnSelectedIndexChanged(); });
            RegisterPropertyChangedCallback(PagerDisplayModeProperty, (s, e) => { OnPagerDisplayModeChanged(); });
            RegisterPropertyChangedCallback(FirstPageButtonVisibilityProperty, (s, e) => { OnFirstPageButtonVisibilityChanged(); });
            RegisterPropertyChangedCallback(PreviousPageButtonVisibilityProperty, (s, e) => { OnPreviousPageButtonVisibilityChanged(); });
            RegisterPropertyChangedCallback(NextPageButtonVisibilityProperty, (s, e) => { OnNextPageButtonVisibilityChanged(); });
            RegisterPropertyChangedCallback(LastPageButtonVisibilityProperty, (s, e) => { OnLastPageButtonVisibilityChanged(); });
            RegisterPropertyChangedCallback(PagerNumberBox.Maximum)
        }
        protected override void OnApplyTemplate()
        {
            // Grab UIElements for later
            FirstPageButton = GetTemplateChild("FirstPageButton") as Button;
            PreviousPageButton = GetTemplateChild("PreviousPageButton") as Button;
            NextPageButton = GetTemplateChild("NextPageButton") as Button;
            LastPageButton = GetTemplateChild("LastPageButton") as Button;
            PagerComboBox = GetTemplateChild("ComboBoxDisplay") as ComboBox;
            PagerNumberBox = GetTemplateChild("NumberBoxDisplay") as NumberBox;
            NumberPanelItems = GetTemplateChild("NumberPanelItemsRepeater") as ItemsRepeater;
            NumberPanelCurrentPageIdentifier = GetTemplateChild("NumberPanelCurrentPageIdentifier") as Rectangle;

            // Attach TestHooks
            FirstPageButtonTestHook = FirstPageButton;
            PreviousPageButtonTestHook = PreviousPageButton;
            NextPageButtonTestHook = NextPageButton;
            LastPageButtonTestHook = LastPageButton;
            NumberBoxDisplayTestHook = PagerNumberBox;
            ComboBoxDisplayTestHook = PagerComboBox;

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
                PagerComboBox.SelectionChanged += (s, e) => { SelectedIndex = PagerComboBox.SelectedIndex + 1; };
            }
            if (NumberPanelItems != null)
            {
                NumberPanelItems.Loaded += (s, e) => { InitializeNumberPanel(); };
                NumberPanelItems.ElementPrepared += OnElementPrepared;
                NumberPanelItems.ElementClearing += OnElementClearing;
            }

            OnPagerDisplayModeChanged();

            // This is for the initial page being loaded whatever page that might be.
            PageChanged?.Invoke(this, new PageChangedEventArgs(PreviousPageIndex, SelectedIndex - 1));
        }

        private void InitializeNumberPanel()
        {
            NumberPanelItems.ItemsSource = NumberPanelCurrentItems;

            if (NumberOfPages <= 7)
            {
                RegisterPropertyChangedCallback(SelectedIndexProperty, (s, e) => { MoveIdentifierToCurrentPage(); });
                foreach(var num in TemplateSettings.Pages.GetRange(0, NumberOfPages))
                {
                    NumberPanelCurrentItems.Add(num);
                }
            } else
            {
                RegisterPropertyChangedCallback(SelectedIndexProperty, (s, e) => { UpdateNumberPanel(); });

                foreach (var num in TemplateSettings.Pages.GetRange(0, 5))
                {
                    NumberPanelCurrentItems.Add(num);
                }
                NumberPanelCurrentItems.Add(RightEllipse);
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
