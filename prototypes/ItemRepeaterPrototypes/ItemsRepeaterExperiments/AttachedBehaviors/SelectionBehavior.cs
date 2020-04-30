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
            var repeater = sender as ItemsRepeater;
            repeater.ElementPrepared -= Repeater_ElementPrepared;
            repeater.ElementClearing -= Repeater_ElementClearing;
            var selectionModel = GetSelectionModel(repeater);
            if (selectionModel != null)
            {
                selectionModel.SelectionChanged -= SelectionModel_SelectionChanged;
            }

            if ((bool)args.NewValue)
            {
                repeater.ElementPrepared += Repeater_ElementPrepared;
                repeater.ElementClearing += Repeater_ElementClearing;
                if (selectionModel != null)
                    //{
                    selectionModel.SelectionChanged += SelectionModel_SelectionChanged;
            }
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
            try
            {
                return (SelectionModel)obj.GetValue(SelectionModelProperty);
            }
            catch
            {
                return null;
            }
        }

        public static void SetSelectionModel(DependencyObject obj, SelectionModel value)
        {
            obj.SetValue(SelectionModelProperty, value);
            if (GetIsEnabled(obj))
            {
                value.SelectionChanged -= SelectionModel_SelectionChanged;
                value.SelectionChanged += SelectionModel_SelectionChanged;
            }
        }

        // Using a DependencyProperty as the backing store for IsSelected.  This enables animation, styling, binding, etc...
        public static readonly DependencyProperty SelectionModelProperty =
            DependencyProperty.RegisterAttached("SelectionModel", typeof(bool), typeof(SelectionBehavior), new PropertyMetadata(default(SelectionModel)));

        // ******** Event handler **********//

        private static void Repeater_ElementClearing(ItemsRepeater sender, ItemsRepeaterElementClearingEventArgs args)
        {
            var item = args.Element;
            item.PointerPressed -= SelectionBehavior_PointerPressed;
        }

        private static void Repeater_ElementPrepared(ItemsRepeater sender, ItemsRepeaterElementPreparedEventArgs args)
        {
            var item = args.Element;
            var selectionModel = GetSelectionModel(sender);

            item.PointerPressed += SelectionBehavior_PointerPressed;

            SetParentItemsRepeater(item, sender);
            SetSelectionModel(item, selectionModel);

            if (selectionModel != null)
            {
                SetIsSelected(item,
                    selectionModel.SelectedItems.Contains(
                        sender.ItemsSourceView.GetAt(sender.GetElementIndex(item))
                    )
                );
            }

        }

        private static void SelectionModel_SelectionChanged(SelectionModel sender, SelectionModelSelectionChangedEventArgs args)
        {
            // Figure out if we can do anything here ...
        }

        private static void SelectionBehavior_PointerPressed(object sender, Windows.UI.Xaml.Input.PointerRoutedEventArgs e)
        {
            var repeater = GetParentItemsRepeater(sender as DependencyObject);
            var selectionModel = GetSelectionModel(sender as DependencyObject);
            var elementIndex = repeater.GetElementIndex(sender as UIElement);
            if (selectionModel != null)
            {

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
                selectionModel.SetAnchorIndex(elementIndex);
            }
        }
    }
}