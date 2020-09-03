using Microsoft.UI.Xaml.Controls;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Diagnostics;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices.ComTypes;
using System.Text;
using Windows.Foundation;
using Windows.UI;
using Windows.UI.Notifications;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Automation;
using Windows.UI.Xaml.Automation.Peers;
using Windows.UI.Xaml.Automation.Provider;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Media.Animation;

namespace MUXControlsTestApp
{
    public sealed partial class PrototypePager : Control
    {
        private static void OnPropertyChanged(DependencyObject obj, DependencyPropertyChangedEventArgs args)
        {
            var sender = obj as PrototypePager;
            DependencyProperty prop = args.Property;

            if (sender.templateApplied == false)
            {
                return;
            }            
            else if (prop == FirstPageButtonVisibilityProperty)
            {
                sender.OnFirstPageButtonVisibilityChanged();
            }
            else if (prop == PreviousPageButtonVisibilityProperty)
            {
                sender.OnPreviousPageButtonVisibilityChanged();
            }
            else if (prop == NextPageButtonVisibilityProperty)
            {
                sender.OnNextPageButtonVisibilityChanged();
            }
            else if (prop == LastPageButtonVisibilityProperty)
            {
                sender.OnLastPageButtonVisibilityChanged();
            }
            else if (prop == PagerDisplayModeProperty)
            {
                sender.OnPagerDisplayModeChanged();
            }
            else if (prop == NumberOfPagesProperty)
            {
                sender.OnNumberOfPagesChanged();
            }
            else if (prop == SelectedIndexProperty)
            {
                sender.OnSelectedIndexChanged((int)args.OldValue - 1);
            }
        }

        private void OnSelectedIndexChanged(int previousIndex)
        {
            PreviousPageIndex = previousIndex;
            if (PagerComboBox != null)
            {
                PagerComboBox.SelectedIndex = SelectedIndex - 1;
            }

            DisablePageButtonsOnEdge();

            if (PagerNumberPanel != null)
            {
                UpdateNumberPanel();
            }
            PageChanged?.Invoke(this, new PageChangedEventArgs(PreviousPageIndex, SelectedIndex - 1));
        }

        private void OnNumberOfPagesChanged()
        {
            NumberPanelEndStateStartIndex = NumberOfPages - 3;
            TemplateSettings.Pages?.Clear();
            foreach (var item in Enumerable.Range(1, NumberOfPages).Cast<object>())
            {
                TemplateSettings.Pages.Add(item);
            }

            if (PagerNumberPanel != null)
            {
                UpdateNumberPanel();
            }
            
            DisablePageButtonsOnEdge();
        }

        private void OnComboBoxSelectionChanged()
        {
            if (PagerComboBox.SelectedIndex == -1)
            {
                PagerComboBox.SelectedIndex = SelectedIndex - 1;
            } 
            else
            {
                SelectedIndex = PagerComboBox.SelectedIndex + 1;
            }
        }
        private void OnPagerDisplayModeChanged()
        {
            switch (this.PagerDisplayMode)
            {
                case PagerDisplayModes.Auto:
                    VisualStateManager.GoToState(this, NumberOfPages < 11 ? ComboBoxVisibleVisualState : NumberBoxVisibleVisualState, false);
                    break;
                case PagerDisplayModes.NumberBox:
                    VisualStateManager.GoToState(this, NumberBoxVisibleVisualState, false);
                    break;
                case PagerDisplayModes.ComboBox:
                    VisualStateManager.GoToState(this, ComboBoxVisibleVisualState, false);
                    break;
                case PagerDisplayModes.NumberPanel:
                    VisualStateManager.GoToState(this, NumberPanelVisibleVisualState, false);
                    break;
            }
        }

        private void OnLastPageButtonVisibilityChanged()
        {

            switch (this.LastPageButtonVisibility)
            {
                case ButtonVisibilityMode.Auto:
                case ButtonVisibilityMode.AlwaysVisible:
                    VisualStateManager.GoToState(this, LastPageButtonVisibleVisualState, false);
                    break;
                case ButtonVisibilityMode.None:
                    VisualStateManager.GoToState(this, LastPageButtonNotVisibleVisualState, false);
                    break;
                case ButtonVisibilityMode.HiddenOnEdge:
                    UpdateHiddenOnEdgeButtons();
                    break;
                default:
                    break;
            }
        }

        private void OnNextPageButtonVisibilityChanged()
        {
            switch (this.NextPageButtonVisibility)
            {
                case ButtonVisibilityMode.Auto:
                case ButtonVisibilityMode.AlwaysVisible:
                    VisualStateManager.GoToState(this, NextPageButtonVisibleVisualState, false);
                    break;
                case ButtonVisibilityMode.None:
                    VisualStateManager.GoToState(this, NextPageButtonNotVisibleVisualState, false);
                    break;
                case ButtonVisibilityMode.HiddenOnEdge:
                    UpdateHiddenOnEdgeButtons();
                    break;
                default:
                    break;
            }
        }

        private void OnPreviousPageButtonVisibilityChanged()
        {
            switch (this.PreviousPageButtonVisibility)
            {
                case ButtonVisibilityMode.Auto:
                case ButtonVisibilityMode.AlwaysVisible:
                    VisualStateManager.GoToState(this, PreviousPageButtonVisibleVisualState, false);
                    break;
                case ButtonVisibilityMode.None:
                    VisualStateManager.GoToState(this, PreviousPageButtonNotVisibleVisualState, false);
                    break;
                case ButtonVisibilityMode.HiddenOnEdge:
                    UpdateHiddenOnEdgeButtons();
                    break;
                default:
                    break;
            }
        }

        private void OnFirstPageButtonVisibilityChanged()
        {
            switch (this.FirstPageButtonVisibility)
            {
                case ButtonVisibilityMode.Auto:
                case ButtonVisibilityMode.AlwaysVisible:
                    VisualStateManager.GoToState(this, FirstPageButtonVisibleVisualState, false);
                    break;
                case ButtonVisibilityMode.None:
                    VisualStateManager.GoToState(this, FirstPageButtonNotVisibleVisualState, false);
                    break;
                case ButtonVisibilityMode.HiddenOnEdge:
                    UpdateHiddenOnEdgeButtons();
                    break;
                default:
                    break;
            }
        }

        private void DisablePageButtonsOnEdge()
        {
            if (SelectedIndex == 1)
            {
                VisualStateManager.GoToState(this, FirstPageButtonDisabledVisualState, false);
                VisualStateManager.GoToState(this, PreviousPageButtonDisabledVisualState, false);
                VisualStateManager.GoToState(this, NextPageButtonEnabledVisualState, false);
                VisualStateManager.GoToState(this, LastPageButtonEnabledVisualState, false);
            } 
            else if (SelectedIndex == NumberOfPages)
            {
                VisualStateManager.GoToState(this, FirstPageButtonEnabledVisualState, false);
                VisualStateManager.GoToState(this, PreviousPageButtonEnabledVisualState, false);
                VisualStateManager.GoToState(this, NextPageButtonDisabledVisualState, false);
                VisualStateManager.GoToState(this, LastPageButtonDisabledVisualState, false);
            }
            else
            {
                VisualStateManager.GoToState(this, FirstPageButtonEnabledVisualState, false);
                VisualStateManager.GoToState(this, PreviousPageButtonEnabledVisualState, false);
                VisualStateManager.GoToState(this, NextPageButtonEnabledVisualState, false);
                VisualStateManager.GoToState(this, LastPageButtonEnabledVisualState, false);
            }
            UpdateHiddenOnEdgeButtons();
        }

        private void UpdateHiddenOnEdgeButtons()
        {
            if (this.FirstPageButtonVisibility == ButtonVisibilityMode.HiddenOnEdge)
            {
                if (this.SelectedIndex != 1)
                {
                    VisualStateManager.GoToState(this, FirstPageButtonVisibleVisualState, false);
                }
                else
                {
                    VisualStateManager.GoToState(this, FirstPageButtonNotVisibleVisualState, false);
                }
            }

            if (this.PreviousPageButtonVisibility == ButtonVisibilityMode.HiddenOnEdge)
            {
                if (this.SelectedIndex != 1)
                {
                    VisualStateManager.GoToState(this, PreviousPageButtonVisibleVisualState, false);
                }
                else
                {
                    VisualStateManager.GoToState(this, PreviousPageButtonNotVisibleVisualState, false);
                }
            }

            if (this.NextPageButtonVisibility == ButtonVisibilityMode.HiddenOnEdge)
            {
                if (this.SelectedIndex != this.NumberOfPages)
                {
                    VisualStateManager.GoToState(this, NextPageButtonVisibleVisualState, false);
                }
                else
                {
                    VisualStateManager.GoToState(this, NextPageButtonNotVisibleVisualState, false);
                }
            }

            if (this.LastPageButtonVisibility == ButtonVisibilityMode.HiddenOnEdge)
            {
                if (this.SelectedIndex != this.NumberOfPages)
                {
                    VisualStateManager.GoToState(this, LastPageButtonVisibleVisualState, false);
                }
                else
                {
                    VisualStateManager.GoToState(this, LastPageButtonNotVisibleVisualState, false);
                }
            }
        }

        /* Events regarding the number panel display mode */

        private void OnElementPrepared(ItemsRepeater sender, ItemsRepeaterElementPreparedEventArgs args)
        {
            if (args.Element == null || args.Element.GetType() != typeof(Button))
            {
                return;
            }

            (args.Element as Button).Click += OnNumberPanelButtonClicked;
            (args.Element as FrameworkElement).Loaded += MoveIdentifierToCurrentPage;
        }

        private void OnElementClearing(ItemsRepeater sender, ItemsRepeaterElementClearingEventArgs args)
        {
            if (args.Element.GetType() == typeof(Button))
            {
                (args.Element as Button).Click -= OnNumberPanelButtonClicked;
                (args.Element as Button).Loaded -= MoveIdentifierToCurrentPage;
            }
        }

        private void OnNumberPanelButtonClicked(object sender, RoutedEventArgs args)
        {
            SelectedIndex = (int)(sender as Button).Content;
        }

        private void MoveIdentifierToCurrentPage(object sender, RoutedEventArgs args)
        {
            var element = sender as FrameworkElement;

            if ((int)element.Tag == SelectedIndex)
            {
                var boundingRect = LayoutInformation.GetLayoutSlot(element);
                var numberPanelRectMargins = NumberPanelCurrentPageIdentifier.Margin;
                NumberPanelCurrentPageIdentifier.Margin = new Thickness(boundingRect.Left, numberPanelRectMargins.Top, numberPanelRectMargins.Right, numberPanelRectMargins.Bottom);  
            }
        }

        private void MoveIdentifierToCurrentPage()
        {
            for (int i = 0; i < NumberOfPages; i++)
            {
                var element = PagerNumberPanel.TryGetElement(i);
                if (element != null)
                {
                    MoveIdentifierToCurrentPage(element, null);
                }
            }
        }

        private void UpdateNumberPanel()
        {

            if (NumberOfPages <= MaxNumberOfElementsInRepeater) // Show all pages
            {
                PagerNumberPanel.ItemsSource = TemplateSettings.Pages;
                MoveIdentifierToCurrentPage();
            }          
            else if (SelectedIndex < NumberPanelMiddleStateStartIndex) // Start State
            {
                PagerNumberPanel.ItemsSource = NumberPanelLeftMostState;
            }
            else if (SelectedIndex >= NumberPanelEndStateStartIndex) // End State 
            {
                PagerNumberPanel.ItemsSource = NumberPanelRightMostState;
            }
            else // Middle State
            {
                PagerNumberPanel.ItemsSource = NumberPanelMiddleState;
            }
        }


    }
}
