using Microsoft.UI.Xaml.Controls;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Diagnostics;
using System.Runtime.InteropServices.ComTypes;
using System.Text;
using Windows.UI;
using Windows.UI.Notifications;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Automation.Peers;
using Windows.UI.Xaml.Automation.Provider;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Media;

namespace MUXControlsTestApp
{
    public sealed partial class PrototypePager : Control
    { 
        private static void OnSelectedIndexChanged(DependencyObject sender, DependencyPropertyChangedEventArgs args)
        {
            (sender as PrototypePager).PreviousPageIndex = (int)args.OldValue - 1;
        }

        private void OnElementPrepared(ItemsRepeater sender, ItemsRepeaterElementPreparedEventArgs args)
        {
            if ((args.Element as Button).Content != null && ((args.Element as Button).Content as SymbolIcon)?.Tag != Ellipse.Tag) 
            {
                (args.Element as Button).Click += OnNumberPanelButtonClicked;

                if ((int)(args.Element as Button).Content == SelectedIndex)
                {
                    (args.Element as Button).Style = (Style)App.Current.Resources["NumberPanelSelectedButtonStyle"];
                }
                else
                {
                    (args.Element as Button).Style = (Style)App.Current.Resources["NumberPanelNotSelectedButtonStyle"];
                }
            }
        }

        private void OnElementClearing(ItemsRepeater sender, ItemsRepeaterElementClearingEventArgs args)
        {
            (args.Element as Button).Click -= OnNumberPanelButtonClicked;
        }

        private void OnNumberPanelButtonClicked(object sender, RoutedEventArgs args)
        {
            SelectedIndex = (int)(sender as Button).Content;
        }

        private void UpdateNumberPanel()
        {
            if (SelectedIndex < 5)
            {
                NumberPanelCurrentItems[0] = 1;
                NumberPanelCurrentItems[1] = 2;
                NumberPanelCurrentItems[2] = 3;
                NumberPanelCurrentItems[3] = 4;
                NumberPanelCurrentItems[4] = 5;
                NumberPanelCurrentItems[5] = Ellipse;
                NumberPanelCurrentItems[6] = NumberOfPages;
            }
            else if (SelectedIndex >= NumberOfPages - 3)
            {
                NumberPanelCurrentItems[0] = 1;
                NumberPanelCurrentItems[1] = Ellipse;
                NumberPanelCurrentItems[2] = NumberOfPages - 4;
                NumberPanelCurrentItems[3] = NumberOfPages - 3;
                NumberPanelCurrentItems[4] = NumberOfPages - 2;
                NumberPanelCurrentItems[5] = NumberOfPages - 1;
                NumberPanelCurrentItems[6] = NumberOfPages;
            }
            else if (SelectedIndex >= 5 && SelectedIndex < NumberOfPages - 3)
            {
                NumberPanelCurrentItems[0] = 1;
                NumberPanelCurrentItems[1] = Ellipse;
                NumberPanelCurrentItems[2] = SelectedIndex - 1;
                NumberPanelCurrentItems[3] = SelectedIndex;
                NumberPanelCurrentItems[4] = SelectedIndex + 1;
                NumberPanelCurrentItems[5] = Ellipse;
                NumberPanelCurrentItems[6] = NumberOfPages;
            }
            
        }

        private void OnPagerDisplayModeChanged()
        {
            switch (this.PagerDisplayMode)
            {
                case PagerDisplayModes.NumberBox:
                    VisualStateManager.GoToState(this, NumberBoxVisibleVisualState, false);
                    break;
                case PagerDisplayModes.Auto:
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
                    VisualStateManager.GoToState(this, LastPageButtonStates[0], false);
                    break;
                case ButtonVisibilityMode.None:
                    VisualStateManager.GoToState(this, LastPageButtonStates[1], false);
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
                    VisualStateManager.GoToState(this, NextPageButtonStates[0], false);
                    break;
                case ButtonVisibilityMode.None:
                    VisualStateManager.GoToState(this, NextPageButtonStates[1], false);
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
                    VisualStateManager.GoToState(this, PreviousPageButtonStates[0], false);
                    break;
                case ButtonVisibilityMode.None:
                    VisualStateManager.GoToState(this, PreviousPageButtonStates[1], false);
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
                    VisualStateManager.GoToState(this, FirstPageButtonStates[0], false);
                    break;
                case ButtonVisibilityMode.None:
                    VisualStateManager.GoToState(this, FirstPageButtonStates[1], false);
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
                VisualStateManager.GoToState(this, FirstPageButtonStates[3], false);
                VisualStateManager.GoToState(this, PreviousPageButtonStates[3], false);
                VisualStateManager.GoToState(this, NextPageButtonStates[2], false);
                VisualStateManager.GoToState(this, LastPageButtonStates[2], false);
            } 
            else if (SelectedIndex == NumberOfPages)
            {
                VisualStateManager.GoToState(this, FirstPageButtonStates[2], false);
                VisualStateManager.GoToState(this, PreviousPageButtonStates[2], false);
                VisualStateManager.GoToState(this, NextPageButtonStates[3], false);
                VisualStateManager.GoToState(this, LastPageButtonStates[3], false);
            }
            else
            {
                VisualStateManager.GoToState(this, FirstPageButtonStates[2], false);
                VisualStateManager.GoToState(this, PreviousPageButtonStates[2], false);
                VisualStateManager.GoToState(this, NextPageButtonStates[2], false);
                VisualStateManager.GoToState(this, LastPageButtonStates[2], false);
            }
            UpdateHiddenOnEdgeButtons();
        }

        private void UpdateHiddenOnEdgeButtons()
        {
            if (this.FirstPageButtonVisibility == ButtonVisibilityMode.HiddenOnEdge)
            {
                if (this.SelectedIndex != 1)
                {
                    VisualStateManager.GoToState(this, FirstPageButtonStates[0], false);
                }
                else
                {
                    VisualStateManager.GoToState(this, FirstPageButtonStates[1], false);
                }
            }

            if (this.PreviousPageButtonVisibility == ButtonVisibilityMode.HiddenOnEdge)
            {
                if (this.SelectedIndex != 1)
                {
                    VisualStateManager.GoToState(this, PreviousPageButtonStates[0], false);
                }
                else
                {
                    VisualStateManager.GoToState(this, PreviousPageButtonStates[1], false);
                }
            }

            if (this.NextPageButtonVisibility == ButtonVisibilityMode.HiddenOnEdge)
            {
                if (this.SelectedIndex != this.NumberOfPages)
                {
                    VisualStateManager.GoToState(this, NextPageButtonStates[0], false);
                }
                else
                {
                    VisualStateManager.GoToState(this, NextPageButtonStates[1], false);
                }
            }

            if (this.LastPageButtonVisibility == ButtonVisibilityMode.HiddenOnEdge)
            {
                if (this.SelectedIndex != this.NumberOfPages)
                {
                    VisualStateManager.GoToState(this, LastPageButtonStates[0], false);
                }
                else
                {
                    VisualStateManager.GoToState(this, LastPageButtonStates[1], false);
                }
            }
        }

    }
}
