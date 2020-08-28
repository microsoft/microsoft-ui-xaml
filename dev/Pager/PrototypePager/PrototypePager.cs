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
        private ItemsRepeater PagerNumberPanel;
        private Rectangle NumberPanelCurrentPageIdentifier;

        private static int MaxNumberOfElementsInRepeater = 7;
        private static int NumberPanelMiddleStateStartIndex = 5;
        private int NumberPanelEndStateStartIndex;

        private static IconElement LeftEllipse = new SymbolIcon(Symbol.More);
        private static IconElement RightEllipse = new SymbolIcon(Symbol.More);

        private ObservableCollection<object> NumberPanelLeftMostState
        {
            get { return new ObservableCollection<object>() { 1, 2, 3, 4, 5, RightEllipse, NumberOfPages }; }
        }
        private ObservableCollection<object> NumberPanelRightMostState
        {
            get { return new ObservableCollection<object>() { 1, LeftEllipse, NumberOfPages - 4, NumberOfPages - 3, NumberOfPages - 2, NumberOfPages - 1, NumberOfPages }; }
        }
        private ObservableCollection<object> NumberPanelMiddleState 
        { 
            get { return new ObservableCollection<object>() { 1, LeftEllipse, SelectedIndex - 1, SelectedIndex, SelectedIndex + 1, RightEllipse, NumberOfPages, }; }
        }


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
        }

        protected override void OnApplyTemplate()
        {
            SetValue(TemplateSettingsProperty, new PagerTemplateSettings(this));
            NumberPanelEndStateStartIndex = NumberOfPages - 3;

            // Grab UIElements for later
            FirstPageButton = GetTemplateChild("FirstPageButton") as Button;
            PreviousPageButton = GetTemplateChild("PreviousPageButton") as Button;
            NextPageButton = GetTemplateChild("NextPageButton") as Button;
            LastPageButton = GetTemplateChild("LastPageButton") as Button;
            PagerComboBox = GetTemplateChild("ComboBoxDisplay") as ComboBox;
            PagerNumberBox = GetTemplateChild("NumberBoxDisplay") as NumberBox;
            PagerNumberPanel = GetTemplateChild("NumberPanelItemsRepeater") as ItemsRepeater;
            NumberPanelCurrentPageIdentifier = GetTemplateChild("NumberPanelCurrentPageIdentifier") as Rectangle;

            // Attach TestHooks
            FirstPageButtonTestHook = FirstPageButton;
            PreviousPageButtonTestHook = PreviousPageButton;
            NextPageButtonTestHook = NextPageButton;
            LastPageButtonTestHook = LastPageButton;
            NumberBoxDisplayTestHook = PagerNumberBox;
            ComboBoxDisplayTestHook = PagerComboBox;
            NumberPanelDisplayTestHook = PagerNumberPanel;
            NumberPanelCurrentPageIdentifierTestHook = NumberPanelCurrentPageIdentifier;

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
                PagerComboBox.SelectionChanged += (s, e) => { OnComboBoxSelectionChanged(); };
            }
            if (PagerNumberPanel != null)
            {
                InitializeNumberPanel();
                PagerNumberPanel.ElementPrepared += OnElementPrepared;
                PagerNumberPanel.ElementClearing += OnElementClearing;
            }

            OnPagerDisplayModeChanged();

            // This is for the initial page being loaded whatever page that might be.
            PageChanged?.Invoke(this, new PageChangedEventArgs(PreviousPageIndex, SelectedIndex - 1));
        }

        private void InitializeNumberPanel()
        {
            if (NumberOfPages < MaxNumberOfElementsInRepeater)
            {
                PagerNumberPanel.ItemsSource = TemplateSettings.Pages;
            }
            else
            {
                PagerNumberPanel.ItemsSource = NumberPanelLeftMostState;
            }
        }
    }

    public sealed class PagerTemplateSettings : DependencyObject
    {
        public ObservableCollection<object> Pages { get; set; }

        public PagerTemplateSettings(PrototypePager pager)
        {
            Pages = new ObservableCollection<object>(Enumerable.Range(1, pager.NumberOfPages).Cast<object>());
        }
    }
}
