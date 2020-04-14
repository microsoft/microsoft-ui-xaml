using ItemsRepeaterExperiments.Common;
using Microsoft.UI.Xaml.Controls;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Documents;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;

// The Templated Control item template is documented at https://go.microsoft.com/fwlink/?LinkId=234235

namespace ItemsRepeaterExperiments.Container
{
    public sealed class HandleContainer : ContentControl
    {

        public bool IsSelected
        {
            get { return (bool)GetValue(IsSelectedProperty); }
            set { 
                SetValue(IsSelectedProperty, value);
                IsSelectedChanged(value);
            }
        }
        // Using a DependencyProperty as the backing store for IsSelected.  This enables animation, styling, binding, etc...
        public static readonly DependencyProperty IsSelectedProperty =
            DependencyProperty.Register("IsSelected", typeof(bool), typeof(HandleContainer), new PropertyMetadata(false));

        public event RoutedEventHandler Click;
        // Using a DependencyProperty as the backing store for Click.  This enables animation, styling, binding, etc...
        public static readonly DependencyProperty ClickProperty =
            DependencyProperty.Register("Click", typeof(RoutedEventHandler), typeof(HandleContainer), new PropertyMetadata(null));

        private bool IsPointerOver = false;


        public HandleContainer()
        {
            DefaultStyleKey = typeof(HandleContainer);

            GotFocus += HandleContainer_GotFocus;
            LostFocus += HandleContainer_LostFocus;

            DragStarting += HandleContainer_DragStarting;
            DragOver += HandleContainer_DragOver;
            DragLeave += HandleContainer_DragLeave;

            PointerEntered += HandleContainer_PointerEntered;
            PointerExited += HandleContainer_PointerExited;
            PointerPressed += HandleContainer_PointerPressed;
            PointerReleased += HandleContainer_PointerReleased;
        }

        private void IsSelectedChanged(bool isSelected)
        {
            if (isSelected)
            {
                VisualStateManager.GoToState(this, "Selected", true);
            }
            else
            {
                VisualStateManager.GoToState(this, "Deselected", true);
            }
        }


        private void HandleContainer_LostFocus(object sender, RoutedEventArgs e)
        {
            VisualStateManager.GoToState(this,"Unfocused",true);
        }

        private void HandleContainer_GotFocus(object sender, RoutedEventArgs e)
        {
            VisualStateManager.GoToState(this,"Focused",true);
        }


        private void HandleContainer_DragStarting(UIElement sender, DragStartingEventArgs args)
        {
            // @WINUI We need to add the container to the datapackage somewhere, otherwise we can't find out what is being dropped
            args.Data.Properties.Add("Container", this);
            args.Data.Properties.Add("Content", Content);
            VisualStateManager.GoToState(this, "Dragging", true);
        }
        
        private void HandleContainer_DragLeave(object sender, DragEventArgs e)
        {
            VisualStateManager.GoToState(this, "NoDrag", true);
        }
        private void HandleContainer_DragOver(object sender, DragEventArgs e)
        {
            VisualStateManager.GoToState(this, "NoDrag", true);
        }

        private void HandleContainer_PointerReleased(object sender, PointerRoutedEventArgs e)
        {
            if(IsPointerOver)
            {
                VisualStateManager.GoToState(this, "PointerOver", true);
            }
            else
            {
                VisualStateManager.GoToState(this, "Normal", true);
            }
            Click?.Invoke(this,new RoutedEventArgs());
        }

        private void HandleContainer_PointerEntered(object sender, PointerRoutedEventArgs e)
        {
            IsPointerOver = true;
            VisualStateManager.GoToState(this, "PointerOver", false);
        }

        private void HandleContainer_PointerPressed(object sender, PointerRoutedEventArgs e)
        {
            VisualStateManager.GoToState(this, "Pressed", true);
        }

        private void HandleContainer_PointerExited(object sender, PointerRoutedEventArgs e)
        {
            IsPointerOver = false; 
            VisualStateManager.GoToState(this, "Normal", true);
        }
    }
}
