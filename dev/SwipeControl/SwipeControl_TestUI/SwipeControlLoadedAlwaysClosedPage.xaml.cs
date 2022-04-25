﻿using Windows.UI.Xaml;

using SwipeControlOpenState = Microsoft.UI.Xaml.Controls.SwipeControlOpenState;
using SwipeControl = Microsoft.UI.Xaml.Controls.SwipeControl;
using SwipeItem = Microsoft.UI.Xaml.Controls.SwipeItem;
using SwipeBehaviorOnInvoked = Microsoft.UI.Xaml.Controls.SwipeBehaviorOnInvoked;
using SwipeItemInvokedEventArgs = Microsoft.UI.Xaml.Controls.SwipeItemInvokedEventArgs;

namespace MUXControlsTestApp
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class SwipeControlLoadedAlwaysClosedPage : TestPage
    {
        public static bool IsOpen(SwipeControlOpenState state)
        {
            return state == SwipeControlOpenState.Opened;
        }

        public SwipeControlLoadedAlwaysClosedPage()
        {
            this.InitializeComponent();
            sc.RegisterPropertyChangedCallback(SwipeControl.OpenStateProperty, SwipeControl_PropertyChanged);
        }

        void SwipeControl_PropertyChanged(DependencyObject obj, DependencyProperty dp)
        {
            if (backOnOpen.IsChecked != true) return;
            var isOpen = sc.OpenState == SwipeControlOpenState.Opened;
            if (isOpen)
            {
                Frame.GoBack();
            }
        }

        void SwipeItem_Invoked(SwipeItem sender, SwipeItemInvokedEventArgs e)
        {
            if (backOnInvoked.IsChecked != true) return;
            Frame.GoBack();
        }

        private void ComboBox_SelectionChanged(object sender, Windows.UI.Xaml.Controls.SelectionChangedEventArgs e)
        {
            si.BehaviorOnInvoked = (SwipeBehaviorOnInvoked) behaviorOnInvoked.SelectedIndex;
        }
    }
}
