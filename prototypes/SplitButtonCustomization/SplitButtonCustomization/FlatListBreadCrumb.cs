using Microsoft.UI.Xaml.Controls;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Numerics;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Documents;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using SplitButton = Microsoft.UI.Xaml.Controls.SplitButton;
using SplitButtonClickEventArgs = Windows.UI.Xaml.Controls.SplitButtonClickEventArgs;

// The Templated Control item template is documented at https://go.microsoft.com/fwlink/?LinkId=234235

namespace SplitButtonCustomization
{
    public sealed class FlatListBreadCrumb : Control
    {

        private ItemsRepeater ItemsRepeater;
        private FlatBreadCrumbNode LastItem;
        internal float widthAcum = 0;



        public TreeStructureConverter TreeConverter
        {
            get { return (TreeStructureConverter)GetValue(TreeConverterProperty); }
            set { SetValue(TreeConverterProperty, value); }
        }

        // Using a DependencyProperty as the backing store for TreeConverter.  This enables animation, styling, binding, etc...
        public static readonly DependencyProperty TreeConverterProperty =
            DependencyProperty.Register("TreeConverter", typeof(TreeStructureConverter), typeof(FlatListBreadCrumb), new PropertyMetadata(0));

        public object Tree
        {
            get { return (object)GetValue(TreeProperty); }
            set { SetValue(TreeProperty, value); }
        }

        // Using a DependencyProperty as the backing store for Tree.  This enables animation, styling, binding, etc...
        public static readonly DependencyProperty TreeProperty =
            DependencyProperty.Register("Tree", typeof(object), typeof(FlatListBreadCrumb), new PropertyMetadata(0));

        public IReadOnlyList<object> ItemsSource
        {
            get { return (IReadOnlyList<object>)GetValue(ItemsSourceProperty); }
            set { SetValue(ItemsSourceProperty, value); }
        }

        public static readonly DependencyProperty ItemsSourceProperty =
            DependencyProperty.Register("ItemsSource", typeof(IReadOnlyList<object>), typeof(FlatListBreadCrumb), new PropertyMetadata(null));

        public object CurrentItem
        {
            get { return (object)GetValue(CurrentItemProperty); }
            internal set { SetValue(CurrentItemProperty, value); }
        }

        // Using a DependencyProperty as the backing store for CurrentItem.  This enables animation, styling, binding, etc...
        public static readonly DependencyProperty CurrentItemProperty =
            DependencyProperty.Register("CurrentItem", typeof(object), typeof(FlatListBreadCrumb), new PropertyMetadata(0));

        public DataTemplate ItemTemplate
        {
            get { return (DataTemplate)GetValue(ItemTemplateProperty); }
            set { SetValue(ItemTemplateProperty, value); }
        }

        public static readonly DependencyProperty ItemTemplateProperty =
            DependencyProperty.Register("ItemTemplate", typeof(DataTemplate), typeof(FlatListBreadCrumb), new PropertyMetadata(""));

        public DataTemplate DropDownItemTemplate
        {
            get { return (DataTemplate)GetValue(DropDownItemTemplateProperty); }
            set { SetValue(DropDownItemTemplateProperty, value); }
        }

        // Using a DependencyProperty as the backing store for DropDownItemTemplate.  This enables animation, styling, binding, etc...
        public static readonly DependencyProperty DropDownItemTemplateProperty =
            DependencyProperty.Register("DropDownItemTemplate", typeof(DataTemplate), typeof(FlatListBreadCrumb), new PropertyMetadata(0));

        public DataTemplateSelector ItemTemplateSelector
        {
            get { return (DataTemplateSelector)GetValue(ItemTemplateSelectorProperty); }
            set { SetValue(ItemTemplateSelectorProperty, value); }
        }

        // Using a DependencyProperty as the backing store for ItemTemplateSelector.  This enables animation, styling, binding, etc...
        public static readonly DependencyProperty ItemTemplateSelectorProperty =
            DependencyProperty.Register("ItemTemplateSelector", typeof(DataTemplateSelector), typeof(FlatListBreadCrumb), new PropertyMetadata(0));

        public DataTemplateSelector DropDownItemTemplateSelector
        {
            get { return (DataTemplateSelector)GetValue(DropDownItemTemplateSelectorProperty); }
            set { SetValue(DropDownItemTemplateSelectorProperty, value); }
        }

        // Using a DependencyProperty as the backing store for DropDownItemTemplateSelector.  This enables animation, styling, binding, etc...
        public static readonly DependencyProperty DropDownItemTemplateSelectorProperty =
            DependencyProperty.Register("DropDownItemTemplateSelector", typeof(DataTemplateSelector), typeof(FlatListBreadCrumb), new PropertyMetadata(0));

        public bool LastItemHasChildren
        {
            get { return (bool)GetValue(LastItemHasChildrenProperty); }
            set { SetValue(LastItemHasChildrenProperty, value); }
        }

        // Using a DependencyProperty as the backing store for LastItemHasChildren.  This enables animation, styling, binding, etc...
        public static readonly DependencyProperty LastItemHasChildrenProperty =
            DependencyProperty.Register("LastItemHasChildren", typeof(bool), typeof(FlatListBreadCrumb), 
                new PropertyMetadata(true, new PropertyChangedCallback(OnPropertyChanged)));

        private static void OnPropertyChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            if (e.Property == LastItemHasChildrenProperty)
            {
                FlatListBreadCrumb breadCrumb = d as FlatListBreadCrumb;
                breadCrumb.LastItem.SetSecondaryButtonVisibility((bool)e.NewValue);
            }
        }

        public event TypedEventHandler<FlatListBreadCrumb, BreadcrumbItemExpandingEventArgs> ItemExpanding;

        public event TypedEventHandler<FlatListBreadCrumb, BreadcrumbItemClickedEventArgs> ItemClicked;

        public delegate List<object> ExpandingEventHandler(FlatListBreadCrumb sender, RoutedEventArgs e);
        public event ExpandingEventHandler Expanding;
        public static readonly DependencyProperty ExpandingProperty =
            DependencyProperty.Register("Expanded", typeof(ExpandingEventHandler), typeof(FlatListBreadCrumb), new PropertyMetadata(""));

        public delegate List<object> SelectedEventHandler(FlatListBreadCrumb sender, RoutedEventArgs e);
        public event SelectedEventHandler Selected;
        public static readonly DependencyProperty SelectedProperty =
            DependencyProperty.Register("Selected", typeof(SelectedEventHandler), typeof(FlatListBreadCrumb), new PropertyMetadata(""));

        public FlatListBreadCrumb()
        {
            this.DefaultStyleKey = typeof(FlatListBreadCrumb);
        }

        protected override void OnApplyTemplate()
        {
            base.OnApplyTemplate();

            ItemsRepeater = GetTemplateChild("Repeater") as ItemsRepeater;

            if (ItemsSource == null)
            {
                TreeConverter.RootNode = this.Tree;

                Binding binding = new Binding();
                binding.Source = TreeConverter.BreadCrumbPath;
                BindingOperations.SetBinding(this, ItemsSourceProperty, binding);
            }

            if (ItemsRepeater != null)
            {
                ItemsRepeater.ElementIndexChanged += ItemsRepeater_ElementIndexChanged;
                ItemsRepeater.ElementPrepared += ItemsRepeater_ElementPrepared;
                ItemsRepeater.ElementClearing += ItemsRepeater_ElementClearing;
            }
        }

        private void ItemsRepeater_ElementIndexChanged(ItemsRepeater sender, ItemsRepeaterElementIndexChangedEventArgs args)
        {
            // throw new NotImplementedException();
        }

        private void ItemsRepeater_ElementClearing(ItemsRepeater sender, ItemsRepeaterElementClearingEventArgs args)
        {
            FlatBreadCrumbNode node = args.Element as FlatBreadCrumbNode;
            node.ResetVisualProperties();
        }

        private void ItemsRepeater_ElementPrepared(ItemsRepeater sender, ItemsRepeaterElementPreparedEventArgs args)
        {
            FlatBreadCrumbNode node = args.Element as FlatBreadCrumbNode;
            node.m_parentContainer = this;
            node.ContentTemplate = this.ItemTemplate;
            node.GotFocus += Node_GotFocus;

            Vector2 renderedSize = node.ActualSize;
            widthAcum += renderedSize.X;

            if (args.Index == (sender.ItemsSource as System.Collections.IList).Count - 1)
            {
                LastItem = node;
                LastItem.SetSecondaryButtonVisibility(this.LastItemHasChildren);
            }
        }

        private void Node_GotFocus(object sender, RoutedEventArgs e)
        {
            // sender is FlatBreadCrumbNode
            CurrentItem = (sender as FlatBreadCrumbNode).Content;
        }

        internal void RaiseSelectedEvent(FlatBreadCrumbNode sender, BreadcrumbItemClickedEventArgs args)
        {
            if (ItemClicked != null)
            {
                ItemClicked.Invoke(this, args);
            }
        }

        internal void RaiseExpandingEvent(FlatBreadCrumbNode sender, BreadcrumbItemExpandingEventArgs args)
        {
            if (ItemExpanding != null)
            {
                ItemExpanding.Invoke(this, args);
            }
        }

        public void ResetCurrentItem()
        {
            CurrentItem = null;
        }

    }

    // Argument for the ItemExpanding event. 
    // Event handlers can use sender.CurrentItem to identify the object from  
    // the ItemsSource associated with the node requesting the dropdown. 
    public sealed class BreadcrumbItemExpandingEventArgs
    {
        // Default: Null 
        // Represents the children items to be shown in the dropdown. Is originally null 
        // when event is raised. Can be set by handler synchronously or asynchronously.  
        public IReadOnlyList<object> Children { get; set; }

        // Allows to provide a childen list asynchronously. 
        public void GetDeferral() { }
    }

    // Argument for the ItemClicked event. 
    public sealed class BreadcrumbItemClickedEventArgs
    {
        // Returns the clicked item. It is either sender.CurrentItem or an item  
        // in the lastest BreadcrumbItemExpandingEventArgs.Children list. 
        public object Item { get; set; }
    }
}
