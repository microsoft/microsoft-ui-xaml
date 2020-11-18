using System;
using System.Collections.Generic;
using System.Numerics;
using Windows.ApplicationModel.VoiceCommands;
using Windows.UI.Composition.Interactions;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Media;

namespace SplitButtonCustomization
{
    public sealed class FlatBreadCrumbNode : ContentControl
    {
        private SplitButton m_splitButton;
        internal FlatListBreadCrumb m_parentContainer;
        Microsoft.UI.Xaml.Controls.ItemsRepeater itemsRepeater;

        private Button secondaryButton = null;
        private Grid secondaryButtonGrid = null;
        private bool isChevronVisible = true;

        public string SecondaryButtonText
        {
            get { return (string)GetValue(SecondaryButtonTextProperty); }
            set { SetValue(SecondaryButtonTextProperty, value); }
        }

        public static readonly DependencyProperty SecondaryButtonTextProperty =
            DependencyProperty.Register("SecondaryButtonText", typeof(string), typeof(FlatBreadCrumbNode), new PropertyMetadata(""));

        public string SecondaryButtonTextDefault
        {
            get { return (string)GetValue(SecondaryButtonTextDefaultProperty); }
            set { SetValue(SecondaryButtonTextDefaultProperty, value); }
        }

        public static readonly DependencyProperty SecondaryButtonTextDefaultProperty =
            DependencyProperty.Register("SecondaryButtonTextDefault", typeof(string), typeof(FlatBreadCrumbNode), new PropertyMetadata(""));

        public FlatBreadCrumbNode()
        {
            this.DefaultStyleKey = typeof(FlatBreadCrumbNode);
        }

        protected override void OnApplyTemplate()
        {
            base.OnApplyTemplate();

            m_splitButton = GetTemplateChild("PART_SplitButton") as SplitButton;
            if (m_splitButton != null)
            {
                m_splitButton.Loaded += SplitButton_Loaded;
            }

            m_splitButton.Flyout.Opening += Flyout_Opening;
            m_splitButton.Flyout.Closing += Flyout_Closing;

            itemsRepeater = ((Flyout)m_splitButton.Flyout).Content as Microsoft.UI.Xaml.Controls.ItemsRepeater;
            itemsRepeater.ElementPrepared += ItemsRepeater_ElementPrepared;
        }

        private void Flyout_Opening(object sender, object e)
        {
            this.SetSecondaryButtonText(false);
        }

        private void Flyout_Closing(Windows.UI.Xaml.Controls.Primitives.FlyoutBase sender, Windows.UI.Xaml.Controls.Primitives.FlyoutBaseClosingEventArgs args)
        {
            this.SetSecondaryButtonText(true);
        }

        private void ItemsRepeater_ElementPrepared(Microsoft.UI.Xaml.Controls.ItemsRepeater sender, Microsoft.UI.Xaml.Controls.ItemsRepeaterElementPreparedEventArgs args)
        {
            Button button = args.Element as Button;

            button.ContentTemplate = m_parentContainer.DropDownItemTemplate;
            button.Click += FlatBreadCrumbNode_Click;   
        }

        private void FlatBreadCrumbNode_Click(object sender, RoutedEventArgs e)
        {
            m_parentContainer.CurrentItem = Content;

            if (m_parentContainer != null)
            {
                Button senderButton = sender as Button;

                // m_parentContainer.CurrentItem = senderButton.Content;
                m_parentContainer.RaiseSelectedEvent(this,
                    new BreadcrumbItemClickedEventArgs() { Item = senderButton.Content });               

                if (m_splitButton.Flyout.IsOpen)
                {
                    m_splitButton.Flyout.Hide();
                }
            }
        }

        private async void M_splitButton_Click(SplitButton sender, SplitButtonClickEventArgs args)
        {
            m_parentContainer.CurrentItem = Content;

            if (m_parentContainer != null)
            {
                m_parentContainer.RaiseSelectedEvent(this, 
                    new BreadcrumbItemClickedEventArgs() { Item = Content });
            }
        }

        private async void SecondaryButton_Click(object sender, RoutedEventArgs e)
        {
            m_parentContainer.CurrentItem = Content;

            if (m_parentContainer != null)
            {
                BreadcrumbItemExpandingEventArgs args = new BreadcrumbItemExpandingEventArgs();
                m_parentContainer.RaiseExpandingEvent(this, args);
                
                // Raise flyout
                if (args.Children != null && args.Children.Count > 0)
                {
                    Microsoft.UI.Xaml.Controls.ItemsRepeater itemsRepeater = ((Flyout)m_splitButton.Flyout).Content as Microsoft.UI.Xaml.Controls.ItemsRepeater;
                    itemsRepeater.ItemsSource = args.Children;
                    m_splitButton.Flyout.ShowAt(this);
                }
            }
        }

        private void SplitButton_Loaded(object sender, RoutedEventArgs e)
        {
            // m_splitButton.Content = this.Content;

            // Click is just for the primary button
            m_splitButton.Click += M_splitButton_Click;

            Grid rootGrid = VisualTreeHelper.GetChild(m_splitButton, 0) as Grid;

            if (rootGrid != null)
            {
                secondaryButtonGrid = VisualTreeHelper.GetChild(rootGrid, 1) as Grid;
                secondaryButton = VisualTreeHelper.GetChild(rootGrid, 4) as Button;
                secondaryButton.Click += SecondaryButton_Click;

                if (secondaryButton != null)
                {
                    this.SetSecondaryButtonText(true);
                    SetSecondaryButtonVisibility(isChevronVisible);
                }
            }

            if (m_parentContainer != null)
            {
                Vector2 renderedSize = m_splitButton.ActualSize;
                m_parentContainer.widthAcum += renderedSize.X;
            }
        }

        public void SetSecondaryButtonVisibility(bool isVisible)
        {
            isChevronVisible = isVisible;
            if (secondaryButton != null)
            {
                secondaryButton.Visibility = isChevronVisible ? Visibility.Visible : Visibility.Collapsed;
                secondaryButtonGrid.Visibility = isChevronVisible ? Visibility.Visible : Visibility.Collapsed;
            }
        }

        public void SetSecondaryButtonText(bool isCollapsed)
        {
            TextBlock secondaryButtonContent = secondaryButton.Content as TextBlock;
            if (secondaryButtonContent != null)
            {
                secondaryButtonContent.Text = isCollapsed ? this.SecondaryButtonText : this.SecondaryButtonTextDefault;
            }
        }

        public void ResetVisualProperties()
        {
            if (secondaryButton != null)
            {
                SetSecondaryButtonVisibility(true);
                SetSecondaryButtonText(true);
            }
        }
    }
}
