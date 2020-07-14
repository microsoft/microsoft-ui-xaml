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
        T GetTemplateChild<T>(string name) where T : DependencyObject
        {
            T templateChild = GetTemplateChild(name) as T;
            if (templateChild == null)
            {
                throw new NullReferenceException(name);
            }
            return templateChild;
        }

        private void OnFirstPageButton_Click(object sender, RoutedEventArgs args)
        {
            SelectedIndex = 1;
        }

        private void OnLastPageButton_Click(object sender, RoutedEventArgs args)
        {
            SelectedIndex = NumberOfPages;
        }

        private void OnNextPageButton_Click(object sender, RoutedEventArgs args)
        {

            if (PagerDisplayMode == PagerDisplayModes.ButtonPanel)
            {
                Button panelBtn = (Button)_ButtonPanelItems.TryGetElement(SelectedIndex -1);
                ButtonAutomationPeer peer = new ButtonAutomationPeer(panelBtn);
                IInvokeProvider invokeProv = peer.GetPattern(PatternInterface.Invoke) as IInvokeProvider;
                invokeProv.Invoke();
                panelBtn.Focus(FocusState.Programmatic);
            }
            else
            {
                SelectedIndex += 1;
            }
        }

        private void OnPreviousPageButton_Click(object sender, RoutedEventArgs args)
        {

            if (PagerDisplayMode == PagerDisplayModes.ButtonPanel)
            {
                Button panelBtn = (Button)_ButtonPanelItems.TryGetElement(SelectedIndex - 1);
                ButtonAutomationPeer peer = new ButtonAutomationPeer(panelBtn);
                IInvokeProvider invokeProv = peer.GetPattern(PatternInterface.Invoke) as IInvokeProvider;
                invokeProv.Invoke();
                panelBtn.Focus(FocusState.Programmatic);
            }
            else
            {
                SelectedIndex -= 1;
            }
        }

        private void OnButtonPanelButtonClick(object sender, RoutedEventArgs e)
        {
            int direction = -1;
            int prevIndex = SelectedIndex;

            SelectedIndex = _ButtonPanelItems.GetElementIndex((UIElement)sender);

            if (SelectedIndex > prevIndex)
            {
                direction = 1;
            }

            double middleOffset = SelectedIndex * _ButtonPanel_ButtonWidth * direction;
            _ButtonPanelView.ChangeView(middleOffset, 0, 1);
        }

        private void OnSelectedIndexChanged_UpdateChevronButtons(DependencyObject sender, DependencyProperty dp)
        {
            if (dp == PrototypePager.SelectedIndexProperty)
            {
                _FirstPageButton.IsEnabled = SelectedIndex != 1;
                _LastPageButton.IsEnabled = SelectedIndex != NumberOfPages;
            }

            if (FirstPageButtonVisibility == ButtonVisibilityMode.HiddenOnLast)
            {
                if (_FirstPageButton.IsEnabled)
                {
                    VisualStateManager.GoToState(this, "FirstPageButtonVisible", false);
                }
                else
                {
                    VisualStateManager.GoToState(this, "FirstPageButtonCollapsed", false);
                }
            }

            if (PreviousPageButtonVisibility == ButtonVisibilityMode.HiddenOnLast)
            {
                if (_PreviousPageButton.IsEnabled)
                {
                    VisualStateManager.GoToState(this, "PreviousPageButtonVisible", false);
                }
                else
                {
                    VisualStateManager.GoToState(this, "PreviousPageButtonCollapsed", false);
                }
            }

            if (NextPageButtonVisibility == ButtonVisibilityMode.HiddenOnLast)
            {
                if (_NextPageButton.IsEnabled)
                {
                    VisualStateManager.GoToState(this, "NextPageButtonVisible", false);
                }
                else
                {
                    VisualStateManager.GoToState(this, "NextPageButtonCollapsed", false);
                }
            }

            if (LastPageButtonVisibility == ButtonVisibilityMode.HiddenOnLast)
            {
                if (_LastPageButton.IsEnabled)
                {
                    VisualStateManager.GoToState(this, "LastPageButtonVisible", false);
                }
                else
                {
                    VisualStateManager.GoToState(this, "LastPageButtonCollapsed", false);
                }
            }
        }

        private void SetButtonPanelView(object sender, RoutedEventArgs args)
        {
            _ButtonPanel_ButtonWidth = ((Button)sender).ActualWidth + ((StackLayout)_ButtonPanelItems.Layout).Spacing;
            _ButtonPanelView.MaxWidth = EllipsisMaxBefore * _ButtonPanel_ButtonWidth;
        }

        private void OnButtonPanelButtonGotFocus(object sender, RoutedEventArgs args)
        {
            ((Button)sender).Background = new SolidColorBrush(Colors.Green);
        }

        private void OnButtonPanelButtonLostFocus(object sender, RoutedEventArgs args)
        {
            ((Button)sender).Background = new SolidColorBrush(Colors.White);
        }
    }
}
