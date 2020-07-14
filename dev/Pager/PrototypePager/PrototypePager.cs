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
        public enum PagerDisplayModes { Auto, ComboBox, NumberBox, ButtonPanel, }
        public enum ButtonVisibilityMode { Auto, AlwaysVisible, HiddenOnLast, None, }

        private Button _FirstPageButton, _PreviousPageButton, _NextPageButton, _LastPageButton;
        private NumberBox _PageNumberBox;
        private ComboBox _PageComboBox;
        private Windows.UI.Xaml.Controls.ScrollViewer _ButtonPanelView;
        private ItemsRepeater _ButtonPanelItems;
        private double _ButtonPanel_ButtonWidth;

        public event TypedEventHandler<PrototypePager, PageChangedEventArgs> PageChanged;
        public PrototypePager()
        {
            DefaultStyleKey = typeof(Pager);
            Loaded += (s, args) => { PagerTemplateSettings = new PagerTemplateSettings(this); };
        }

        private void ApplyButtonPanelTemplate()
        {
            _ButtonPanelView = GetTemplateChild<Windows.UI.Xaml.Controls.ScrollViewer>("ButtonPanelViewer");
            _ButtonPanelItems = GetTemplateChild<ItemsRepeater>("ButtonPanelItemsRepeater");

            _ButtonPanelItems.ElementPrepared += ButtonPanelFirstButtonPrepared;
            _ButtonPanelItems.ElementPrepared += ButtonPanelSetButtonEvents;
        }

        private void ButtonPanelSetButtonEvents(ItemsRepeater sender, ItemsRepeaterElementPreparedEventArgs args)
        {
            ((Button)args.Element).Click += OnButtonPanelButtonClick;
            ((Button)args.Element).GotFocus += OnButtonPanelButtonGotFocus;
            ((Button)args.Element).LostFocus += OnButtonPanelButtonLostFocus;
            ((Button)args.Element).Click += (s, e) => { PageChanged?.Invoke(this, new PageChangedEventArgs(SelectedIndex + 1)); };
        }

        protected override void OnApplyTemplate()
        {
            base.OnApplyTemplate();

            RegisterPropertyChangedCallback(SelectedIndexProperty, OnSelectedIndexChanged_UpdateChevronButtons);
            RegisterPropertyChangedCallback(SelectedIndexProperty, (s, e) => { PageChanged?.Invoke(this, new PageChangedEventArgs(SelectedIndex)); });
            
            _FirstPageButton = GetTemplateChild<Button>("FirstPageButton");
            _PreviousPageButton = GetTemplateChild<Button>("PreviousPageButton");
            _NextPageButton = GetTemplateChild<Button>("NextPageButton");
            _LastPageButton = GetTemplateChild<Button>("LastPageButton");
            _PageNumberBox = GetTemplateChild<NumberBox>("PageNumberBox");
            _PageComboBox = GetTemplateChild<ComboBox>("PageComboBox");
            
            _FirstPageButton.Click += OnFirstPageButton_Click;
            _PreviousPageButton.Click += OnPreviousPageButton_Click;
            _NextPageButton.Click += OnNextPageButton_Click;
            _LastPageButton.Click += OnLastPageButton_Click;

            //ApplyButtonPanelTemplate();

            switch (PagerDisplayMode)
            {
                case PagerDisplayModes.NumberBox:
                    VisualStateManager.GoToState(this, "NumberBoxVisible", false);
                    break;
                case PagerDisplayModes.Auto:
                case PagerDisplayModes.ComboBox:
                    VisualStateManager.GoToState(this, "ComboBoxVisible", false);
                    break;
                case PagerDisplayModes.ButtonPanel:
                    GetTemplateChild<StackPanel>("ButtonPanelDisplay").Visibility = Visibility.Visible;
                    break;
            }
        }

        private void ButtonPanelFirstButtonPrepared(ItemsRepeater sender, ItemsRepeaterElementPreparedEventArgs args)
        {
            ((Button)args.Element).Loaded += SetButtonPanelView;
            sender.ElementPrepared -= ButtonPanelFirstButtonPrepared;
        }
    }

    public class PagerTemplateSettings : DependencyObject
    {
        public ObservableCollection<int> Pages { get; private set; }

        public PagerTemplateSettings(PrototypePager pager)
        {
            List<int> PageRange = new List<int>(Enumerable.Range(1, pager.NumberOfPages));
            Pages = new ObservableCollection<int>(PageRange);
        }
    }
}
