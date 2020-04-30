using Microsoft.UI.Xaml.Controls;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Automation;
using Windows.UI.Xaml.Controls;

namespace ItemsRepeaterExperiments.AttachedBehaviors
{
    public class SelectionBehavior : DependencyObject
    {
        // Should we store this as an attached property on the repeater?
        // What would happen with to repeaters and shift click?
        private static UIElement LastFocused;

        public static bool GetIsEnabled(DependencyObject obj)
        {
            return (bool)obj.GetValue(IsEnabledProperty);
        }

        public static void SetIsEnabled(DependencyObject obj, bool value)
        {
            obj.SetValue(IsEnabledProperty, value);
        }

        public static readonly DependencyProperty IsEnabledProperty =
            DependencyProperty.RegisterAttached("IsEnabled", typeof(bool), typeof(SelectionBehavior), new PropertyMetadata(false,
                new PropertyChangedCallback(OnIsEnabledChanged)));

        private static void OnIsEnabledChanged(DependencyObject sender, DependencyPropertyChangedEventArgs args)
        {
            if((bool)args.NewValue)
            {
                SetIsSelected(sender, GetIsEnabled(sender));
                (sender as UIElement).PointerPressed += SelectionBehavior_PointerPressed;
            }
            else
            {
                (sender as UIElement).PointerPressed -= SelectionBehavior_PointerPressed;
            }
        }


        private static void SelectionBehavior_PointerPressed(object sender, Windows.UI.Xaml.Input.PointerRoutedEventArgs e)
        {
            var repeater = GetParentItemsRepeater(sender as DependencyObject);
            var selectionModel = GetSelectionModel(sender as DependencyObject);
            var elementIndex = repeater.GetElementIndex(sender as UIElement);
            if (selectionModel.SingleSelect)
            {
                if (e.KeyModifiers == Windows.System.VirtualKeyModifiers.Control)
                {
                    selectionModel.Deselect(elementIndex);
                }
                else
                {
                    selectionModel.Select(elementIndex);
                }
            }
            else
            {
                switch (e.KeyModifiers)
                {
                    case Windows.System.VirtualKeyModifiers.Shift:
                        if (PointerBehaviors.LastFocused != null)
                        {
                            var lastFocused = repeater.GetElementIndex(PointerBehaviors.LastFocused as UIElement);
                            selectionModel.SetAnchorIndex(lastFocused);
                        }
                        selectionModel.SelectRangeFromAnchor(elementIndex);
                        break;
                    case Windows.System.VirtualKeyModifiers.Control:
                        selectionModel.Deselect(elementIndex);
                        break;
                    default:
                        selectionModel.Select(elementIndex);
                        break;
                }
            }
            LastFocused = sender as UIElement;
        }

        public static bool GetIsSelected(DependencyObject obj)
        {
            return (bool)obj.GetValue(IsSelectedProperty);
        }

        public static void SetIsSelected(DependencyObject obj, bool value)
        {
            obj.SetValue(IsSelectedProperty, value);
        }

        // Using a DependencyProperty as the backing store for IsSelected.  This enables animation, styling, binding, etc...
        public static readonly DependencyProperty IsSelectedProperty =
            DependencyProperty.RegisterAttached("IsSelected", typeof(bool), typeof(SelectionBehavior), new PropertyMetadata(false));


        public static ItemsRepeater GetParentItemsRepeater(DependencyObject obj)
        {
            return (ItemsRepeater)obj.GetValue(ParentItemsRepeaterProperty);
        }

        public static void SetParentItemsRepeater(DependencyObject obj, ItemsRepeater value)
        {
            obj.SetValue(ParentItemsRepeaterProperty, value);
        }

        // Using a DependencyProperty as the backing store for IsSelected.  This enables animation, styling, binding, etc...
        public static readonly DependencyProperty ParentItemsRepeaterProperty =
            DependencyProperty.RegisterAttached("ParentItemsRepeater", typeof(bool), typeof(SelectionBehavior), new PropertyMetadata(default(ItemsRepeater)));


        public static SelectionModel GetSelectionModel(DependencyObject obj)
        {
            return (SelectionModel)obj.GetValue(SelectionModelProperty);
        }

        public static void SetSelectionModel(DependencyObject obj, SelectionModel value)
        {
            obj.SetValue(SelectionModelProperty, value);
        }

        // Using a DependencyProperty as the backing store for IsSelected.  This enables animation, styling, binding, etc...
        public static readonly DependencyProperty SelectionModelProperty =
            DependencyProperty.RegisterAttached("SelectionModel", typeof(bool), typeof(SelectionBehavior), new PropertyMetadata(default(SelectionModel)));


    }
}
