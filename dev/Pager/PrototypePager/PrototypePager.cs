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

        public event TypedEventHandler<PrototypePager, PageChangedEventArgs> PageChanged;
        
        public PrototypePager()
        {
            this.DefaultStyleKey = typeof(PrototypePager);
            this.Loaded += (s, args) => { PagerTemplateSettings = new PagerTemplateSettings(this); };

        }
        protected override void OnApplyTemplate()
        {
            // Grab UIElements for later
            FirstPageButton = GetTemplateChild<Button>("FirstPageButton");
            PreviousPageButton = GetTemplateChild<Button>("PreviousPageButton");
            NextPageButton = GetTemplateChild<Button>("NextPageButton");
            LastPageButton = GetTemplateChild<Button>("LastPageButton");
            FirstPageButtonTestHook = FirstPageButton;
            PreviousPageButtonTestHook = PreviousPageButton;
            NextPageButtonTestHook = NextPageButton;
            LastPageButtonTestHook = LastPageButton;
            NumberBoxDisplayTestHook = GetTemplateChild<NumberBox>("NumberBoxDisplay");
            ComboBoxDisplayTestHook = GetTemplateChild<ComboBox>("ComboBoxDisplay");

            // Attach Callbacks for property changes
            RegisterPropertyChangedCallback(SelectedIndexProperty, DisablePageButtonsOnEdge);
            RegisterPropertyChangedCallback(SelectedIndexProperty, (s, e) => { PageChanged?.Invoke(this, new PageChangedEventArgs(SelectedIndex - 1)); });

            // Attach click events
            FirstPageButton.Click += (s, e) => { SelectedIndex = 1; };
            PreviousPageButton.Click += (s, e) => { SelectedIndex -= 1; };
            NextPageButton.Click += (s, e) => { SelectedIndex += 1; };
            LastPageButton.Click += (s, e) => { SelectedIndex = NumberOfPages; };

            InitializeDisplayMode();
        }

        T GetTemplateChild<T>(string name) where T : DependencyObject
        {
            T templateChild = GetTemplateChild(name) as T;
            if (templateChild == null)
            {
                throw new NullReferenceException(name);
            }
            return templateChild;
        }

        private void InitializeDisplayMode()
        {
            switch (PagerDisplayMode)
            {
                case PagerDisplayModes.NumberBox:
                    VisualStateManager.GoToState(this, "NumberBoxVisible", false);
                    break;
                case PagerDisplayModes.Auto:
                case PagerDisplayModes.ComboBox:
                    VisualStateManager.GoToState(this, "ComboBoxVisible", false);
                    break;
                case PagerDisplayModes.NumberPanel:
                    VisualStateManager.GoToState(this, "NumberPanelVisible", false);
                    break;
            }
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
