using Microsoft.UI.Xaml.Controls;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Text;
using Windows.UI;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Automation.Peers;
using Windows.UI.Xaml.Automation.Provider;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Media;

namespace MUXControlsTestApp
{
    public sealed partial class PrototypePager : Control
    { 

        private static void OnPagerDisplayModeChanged(DependencyObject sender, DependencyPropertyChangedEventArgs args)
        {
            PagerDisplayModes mode = (PagerDisplayModes)args.NewValue;
            PrototypePager pager = sender as PrototypePager;
            switch (mode)
            {
                case PagerDisplayModes.NumberBox:
                    VisualStateManager.GoToState(pager, NumberBoxVisibleVisualState, false);
                    break;
                case PagerDisplayModes.Auto:
                case PagerDisplayModes.ComboBox:
                    VisualStateManager.GoToState(pager, ComboBoxVisibleVisualState, false);
                    break;
                case PagerDisplayModes.NumberPanel:
                    VisualStateManager.GoToState(pager, NumberPanelVisibleVisualState, false);
                    break;
            }
        }

        private static void OnLastPageButtonVisibilityChanged(DependencyObject sender, DependencyPropertyChangedEventArgs args)
        {
            ButtonVisibilityMode lastPageButtonVisibilityMode = (ButtonVisibilityMode)args.NewValue;
            PrototypePager pager = sender as PrototypePager;

            switch (lastPageButtonVisibilityMode)
            {
                case ButtonVisibilityMode.Auto:
                case ButtonVisibilityMode.AlwaysVisible:
                    VisualStateManager.GoToState(pager, "LastPageButtonVisible", false);
                    break;
                case ButtonVisibilityMode.None:
                    VisualStateManager.GoToState(pager, "LastPageButtonCollapsed", false);
                    break;
                case ButtonVisibilityMode.HiddenOnEdge:
                    UpdateHiddenOnEdgeButtons(pager);
                    break;
                default:
                    break;
            }
        }

        private static void OnNextPageButtonVisibilityChanged(DependencyObject sender, DependencyPropertyChangedEventArgs args)
        {
            ButtonVisibilityMode nextPageButtonVisibilityMode = (ButtonVisibilityMode)args.NewValue;
            PrototypePager pager = sender as PrototypePager;

            switch (nextPageButtonVisibilityMode)
            {
                case ButtonVisibilityMode.Auto:
                case ButtonVisibilityMode.AlwaysVisible:
                    VisualStateManager.GoToState(pager, "NextPageButtonVisible", false);
                    break;
                case ButtonVisibilityMode.None:
                    VisualStateManager.GoToState(pager, "NextPageButtonCollapsed", false);
                    break;
                case ButtonVisibilityMode.HiddenOnEdge:
                    UpdateHiddenOnEdgeButtons(pager);
                    break;
                default:
                    break;
            }
        }

        private static void OnPreviousPageButtonVisibilityChanged(DependencyObject sender, DependencyPropertyChangedEventArgs args)
        {
            ButtonVisibilityMode prevPageButtonVisibilityMode = (ButtonVisibilityMode)args.NewValue;
            PrototypePager pager = sender as PrototypePager;
            switch (prevPageButtonVisibilityMode)
            {
                case ButtonVisibilityMode.Auto:
                case ButtonVisibilityMode.AlwaysVisible:
                    VisualStateManager.GoToState(pager, "PreviousPageButtonVisible", false);
                    break;
                case ButtonVisibilityMode.None:
                    VisualStateManager.GoToState(pager, "PreviousPageButtonCollapsed", false);
                    break;
                case ButtonVisibilityMode.HiddenOnEdge:
                    UpdateHiddenOnEdgeButtons(pager);
                    break;
                default:
                    break;
            }
        }

        private static void OnFirstPageButtonVisibilityChanged(DependencyObject sender, DependencyPropertyChangedEventArgs args)
        {
            ButtonVisibilityMode firstPageButtonVisibilityMode = (ButtonVisibilityMode)args.NewValue;
            PrototypePager pager = sender as PrototypePager;

            switch (firstPageButtonVisibilityMode)
            {
                case ButtonVisibilityMode.Auto:
                case ButtonVisibilityMode.AlwaysVisible:
                    VisualStateManager.GoToState(pager, "FirstPageButtonVisible", false);
                    break;
                case ButtonVisibilityMode.None:
                    VisualStateManager.GoToState(pager, "FirstPageButtonCollapsed", false);
                    break;
                case ButtonVisibilityMode.HiddenOnEdge:
                    UpdateHiddenOnEdgeButtons(pager);
                    break;
                default:
                    break;
            }
        }

        private void DisablePageButtonsOnEdge(DependencyObject sender, DependencyProperty dp)
        {
            FirstPageButton.IsEnabled = SelectedIndex != 1;
            PreviousPageButton.IsEnabled = SelectedIndex != 1;
            NextPageButton.IsEnabled = SelectedIndex != NumberOfPages;
            LastPageButton.IsEnabled = SelectedIndex != NumberOfPages;
            UpdateHiddenOnEdgeButtons(this);
        }

        private static void UpdateHiddenOnEdgeButtons(PrototypePager pager)
        {
            if (pager.FirstPageButtonVisibility == ButtonVisibilityMode.HiddenOnEdge)
            {
                if (pager.SelectedIndex != 1)
                {
                    VisualStateManager.GoToState(pager, "FirstPageButtonVisible", false);
                }
                else
                {
                    VisualStateManager.GoToState(pager, "FirstPageButtonCollapsed", false);
                }
            }

            if (pager.PreviousPageButtonVisibility == ButtonVisibilityMode.HiddenOnEdge)
            {
                if (pager.SelectedIndex != 1)
                {
                    VisualStateManager.GoToState(pager, "PreviousPageButtonVisible", false);
                }
                else
                {
                    VisualStateManager.GoToState(pager, "PreviousPageButtonCollapsed", false);
                }
            }

            if (pager.NextPageButtonVisibility == ButtonVisibilityMode.HiddenOnEdge)
            {
                if (pager.SelectedIndex != pager.NumberOfPages)
                {
                    VisualStateManager.GoToState(pager, "NextPageButtonVisible", false);
                }
                else
                {
                    VisualStateManager.GoToState(pager, "NextPageButtonCollapsed", false);
                }
            }

            if (pager.LastPageButtonVisibility == ButtonVisibilityMode.HiddenOnEdge)
            {
                if (pager.SelectedIndex != pager.NumberOfPages)
                {
                    VisualStateManager.GoToState(pager, "LastPageButtonVisible", false);
                }
                else
                {
                    VisualStateManager.GoToState(pager, "LastPageButtonCollapsed", false);
                }
            }
        }

    }
}
