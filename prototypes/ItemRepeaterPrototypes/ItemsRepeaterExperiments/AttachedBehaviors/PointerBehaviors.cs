using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Input;

namespace ItemsRepeaterExperiments.AttachedBehaviors
{
    public class PointerBehaviors : DependencyObject
    {

        public static event RoutedEventHandler Click;

        public static void AttachProperty(FrameworkElement dp)
        {
            if (dp == null)
            {
                return;
            }

            dp.PointerPressed += Dp_PointerPressed;
            dp.PointerReleased += Dp_PointerReleased;
            dp.PointerEntered += Dp_PointerEntered;
            dp.PointerExited += Dp_PointerExited;

            dp.Tapped += Dp_Tapped;
        }

        private static void Dp_Tapped(object sender, TappedRoutedEventArgs e)
        {
            Click?.Invoke(sender, new RoutedEventArgs());
        }

        private static void Dp_PointerExited(object sender, PointerRoutedEventArgs e)
        {
            SetIsPointerOverNotPressed(sender as DependencyObject, false);
            SetIsPointerOver(sender as DependencyObject, false);
        }

        private static void Dp_PointerEntered(object sender, PointerRoutedEventArgs e)
        {
            SetIsPointerOver(sender as DependencyObject, true);
            SetIsPointerOverNotPressed(sender as DependencyObject, true);
        }

        private static void Dp_PointerReleased(object sender, PointerRoutedEventArgs e)
        {
            SetIsPressed(sender as DependencyObject, false);
            Click?.Invoke(sender, new RoutedEventArgs());
        }

        private static void Dp_PointerPressed(object sender, PointerRoutedEventArgs e)
        {
            SetIsPointerOverNotPressed(sender as DependencyObject, false);
            SetIsPressed(sender as DependencyObject, true);
        }


        public static bool GetIsPointerOver(DependencyObject obj)
        {
            return (bool)obj.GetValue(IsPointerOverProperty);
        }

        public static void SetIsPointerOver(DependencyObject obj, bool value)
        {
            obj.SetValue(IsPointerOverProperty, value);
        }

        // Using a DependencyProperty as the backing store for IsSelected.  This enables animation, styling, binding, etc...
        public static readonly DependencyProperty IsPointerOverProperty =
            DependencyProperty.RegisterAttached("IsPointerOver", typeof(bool), typeof(PointerBehaviors), new PropertyMetadata(false));


        public static bool GetIsPressed(DependencyObject obj)
        {
            return (bool)obj.GetValue(IsPressedProperty);
        }

        public static void SetIsPressed(DependencyObject obj, bool value)
        {
            obj.SetValue(IsPressedProperty, value);
        }

        // Using a DependencyProperty as the backing store for IsSelected.  This enables animation, styling, binding, etc...
        public static readonly DependencyProperty IsPressedProperty =
            DependencyProperty.RegisterAttached("IsPressed", typeof(bool), typeof(PointerBehaviors), new PropertyMetadata(false));
        
        public static bool GetIsPointerOverNotPressed(DependencyObject obj)
        {
            return (bool)obj.GetValue(IsPointerOverNotPressedProperty);
        }

        public static void SetIsPointerOverNotPressed(DependencyObject obj, bool value)
        {
            obj.SetValue(IsPointerOverNotPressedProperty, value);
        }
        // Using a DependencyProperty as the backing store for IsSelected.  This enables animation, styling, binding, etc...
        public static readonly DependencyProperty IsPointerOverNotPressedProperty =
            DependencyProperty.RegisterAttached("IsPointerOverNotPressed", typeof(bool), typeof(PointerBehaviors), new PropertyMetadata(false));

    }
}
