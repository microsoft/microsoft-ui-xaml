using Microsoft.UI.Xaml.Controls;
using System.Collections.Generic;
using Windows.UI.Xaml;

namespace ItemsRepeaterExperiments.AttachedBehaviors
{
    public class SelectionBehavior : DependencyObject
    {
        private static Dictionary<SelectionModel, List<UIElement>> SelectionModelContainers = new Dictionary<SelectionModel, List<UIElement>>();

        public static bool? GetIsSelected(DependencyObject obj)
        {
            return (bool?)obj.GetValue(IsSelectedProperty);
        }

        public static void SetIsSelected(DependencyObject obj, bool? value)
        {
            obj.SetValue(IsSelectedProperty, value);
        }

        public static readonly DependencyProperty IsSelectedProperty =
            DependencyProperty.RegisterAttached("IsSelected", typeof(bool?), typeof(SelectionBehavior), new PropertyMetadata(null));

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
            if (value != null)
            {
                value.SelectionChanged -= SelectionModel_SelectionChanged;
                value.SelectionChanged += SelectionModel_SelectionChanged;
            }
        }

        public static readonly DependencyProperty SelectionModelProperty =
            DependencyProperty.RegisterAttached("SelectionModel", typeof(SelectionModel), typeof(SelectionBehavior), new PropertyMetadata(default(SelectionModel), new PropertyChangedCallback(OnSelectionModelChanged)));

        private static void OnSelectionModelChanged(DependencyObject element, DependencyPropertyChangedEventArgs args)
        {
            var oldSelectionModel = args.OldValue as SelectionModel;
            if (oldSelectionModel != null)
            {
                UnInitializeSelectionModelOnContainer(oldSelectionModel, element);
            }

            var selectionModel = args.NewValue as SelectionModel;
            if (selectionModel != null)
            {
                if (element is ItemsRepeater)
                {
                    var repeater = element as ItemsRepeater;
                    repeater.ElementPrepared += Repeater_ElementPrepared;
                    repeater.ElementClearing += Repeater_ElementClearing;
                }
                else
                {
                    InitializeSelectionModelOnContainer(selectionModel, element);
                }
            }
        }

        private static void InitializeSelectionModelOnContainer(SelectionModel selectionModel, DependencyObject element)
        {
            if (!SelectionModelContainers.ContainsKey(selectionModel))
            {
                SelectionModelContainers.Add(selectionModel, new List<UIElement>());
            }

            var containersList = SelectionModelContainers[selectionModel];
            containersList.Add(element as UIElement);

            (element as UIElement).PointerPressed += Element_PointerPressed;
        }

        private static void UnInitializeSelectionModelOnContainer(SelectionModel selectionModel, DependencyObject element)
        {
            SelectionModelContainers[selectionModel].Remove(element as UIElement);
            (element as UIElement).PointerPressed -= Element_PointerPressed;
        }

        private static void Element_PointerPressed(object sender, Windows.UI.Xaml.Input.PointerRoutedEventArgs e)
        {
            SelectionBehavior_PointerPressed(sender, e);
        }

        private static void Repeater_ElementClearing(ItemsRepeater sender, ItemsRepeaterElementClearingEventArgs args)
        {
            UnInitializeSelectionModelOnContainer(GetSelectionModel(sender), args.Element);
            SetIndex(args.Element, -1);
            SetSelectionModel(args.Element, null);
        }

        private static void Repeater_ElementPrepared(ItemsRepeater sender, ItemsRepeaterElementPreparedEventArgs args)
        {
            var selectionModel = GetSelectionModel(sender);
            SetSelectionModel(args.Element, selectionModel);
            SetIndex(args.Element, args.Index);
           
            var index = GetIndexPath(args.Element);
            var isSelectedTristate = selectionModel.IsSelectedAt(index);
            SetIsSelected(args.Element, isSelectedTristate.HasValue && isSelectedTristate.Value);
            
            InitializeSelectionModelOnContainer(GetSelectionModel(sender), args.Element);
        }

        private static void SelectionModel_SelectionChanged(SelectionModel sender, SelectionModelSelectionChangedEventArgs args)
        {
            if (SelectionModelContainers.ContainsKey(sender))
            {
                var containerList = SelectionModelContainers[sender];
                foreach (var element in containerList)
                {
                    var index = GetIndexPath(element);
                    var isSelected = sender.IsSelectedAt(index);
                    SetIsSelected(element, isSelected.HasValue && isSelected.Value);
                }
            }
        }

        private static void SelectionBehavior_PointerPressed(object sender, Windows.UI.Xaml.Input.PointerRoutedEventArgs e)
        {
            var selectionModel = GetSelectionModel(sender as DependencyObject);
            var elementIndex = GetIndexPath(sender as DependencyObject);
            if (selectionModel != null && elementIndex.GetSize() > 0)
            {
                if (selectionModel.SingleSelect)
                {
                    if (e.KeyModifiers == Windows.System.VirtualKeyModifiers.Control)
                    {
                        selectionModel.DeselectAt(elementIndex);
                    }
                    else
                    {
                        selectionModel.SelectAt(elementIndex);
                    }
                }
                else
                {
                    switch (e.KeyModifiers)
                    {
                        case Windows.System.VirtualKeyModifiers.Shift:
                            selectionModel.SelectRangeFromAnchorTo(elementIndex);
                            break;
                        case Windows.System.VirtualKeyModifiers.Control:
                            selectionModel.DeselectAt(elementIndex);
                            break;
                        default:
                            selectionModel.SelectAt(elementIndex);
                            break;
                    }
                }

                e.Handled = true;

                selectionModel.AnchorIndex = elementIndex;
            }
        }

        public static int GetIndex(DependencyObject obj)
        {
            return (int)obj.GetValue(IndexProperty);
        }

        public static void SetIndex(DependencyObject obj, int value)
        {
            obj.SetValue(IndexProperty, value);
        }

        public static readonly DependencyProperty IndexProperty =
            DependencyProperty.RegisterAttached("Index", typeof(int), typeof(SelectionBehavior), new PropertyMetadata(-1));


        public static IndexPath GetIndexPath(DependencyObject element)
        {
            var child = element as FrameworkElement;
            var parentContainer = GetParentContainer(child.Parent);
            List<int> path = new List<int>();
            path.Add(GetIndex(element));

            while (parentContainer != null)
            {
                path.Insert(0, GetIndex(parentContainer));
                child = parentContainer as FrameworkElement;
                parentContainer = GetParentContainer(child.Parent);
            }

            return IndexPath.CreateFromIndices(path);
        }


        public static UIElement GetParentContainer(DependencyObject obj)
        {
            return (UIElement)obj.GetValue(ParentContainerProperty);
        }

        public static void SetParentContainer(DependencyObject obj, UIElement value)
        {
            obj.SetValue(ParentContainerProperty, value);
        }

        public static readonly DependencyProperty ParentContainerProperty =
            DependencyProperty.RegisterAttached("ParentContainer", typeof(UIElement), typeof(SelectionBehavior), new PropertyMetadata(null));
    }
}