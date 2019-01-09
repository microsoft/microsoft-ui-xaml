using System;
using Microsoft.UI.Xaml.Controls;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Automation.Peers;
using Windows.UI.Xaml.Controls;
using Windows.Foundation;
using System.Diagnostics;
using System.Collections.Generic;

#if !BUILD_WINDOWS
using ItemsRepeater = Microsoft.UI.Xaml.Controls.ItemsRepeater;
using ItemsRepeaterElementIndexChangedEventArgs = Microsoft.UI.Xaml.Controls.ItemsRepeaterElementIndexChangedEventArgs;
using ItemsRepeaterElementClearingEventArgs = Microsoft.UI.Xaml.Controls.ItemsRepeaterElementClearingEventArgs;
using ItemsRepeaterElementPreparedEventArgs = Microsoft.UI.Xaml.Controls.ItemsRepeaterElementPreparedEventArgs;
using muxcp = Microsoft.UI.Xaml.Controls.Primitives;
#endif

namespace DEPControlsTestApp.ItemsViewPrototype
{
    [Windows.UI.Xaml.Markup.ContentProperty(Name = nameof(ViewDefinition))]
    public class ItemsView : ItemsViewBase
    {
        public ItemsView()
        {
            this.DefaultStyleKey = typeof(ItemsView);
        }

        public muxcp.SelectionMode SelectionMode
        {
            get { return (muxcp.SelectionMode)GetValue(SelectionModeProperty); }
            set { SetValue(SelectionModeProperty, value); }
        }

        public ViewDefinitionBase ViewDefinition
        {
            get { return (ViewDefinitionBase)GetValue(ViewDefinitionProperty); }
            set { SetValue(ViewDefinitionProperty, value); }
        }

        public int GetElementIndex(UIElement element)
        {
            return m_repeater == null ? -1 : m_repeater.GetElementIndex(element);
        }

        public UIElement GetElement(int index)
        {
            return m_repeater == null ? null : m_repeater.TryGetElement(index);
        }

        public static readonly DependencyProperty SelectionModeProperty =
            DependencyProperty.Register(
                nameof(SelectionMode),
                typeof(muxcp.SelectionMode),
                typeof(ItemsView),
                new PropertyMetadata(
                    muxcp.SelectionMode.Single,
                    OnSelectionModeChanged));

        public static readonly DependencyProperty ViewDefinitionProperty =
            DependencyProperty.Register(
                nameof(ViewDefinition),
                typeof(ViewDefinitionBase),
                typeof(ItemsView),
                new PropertyMetadata(null, new PropertyChangedCallback(OnViewDefinitionChanged)));

        protected override void OnApplyTemplate()
        {
            base.OnApplyTemplate();

            m_scrollViewer = GetTemplateChild("ScrollViewer") as ScrollViewer;
            m_repeater = GetTemplateChild("Repeater") as ItemsRepeater;
            var viewDefinition = GetViewDefinition();
            if (viewDefinition != null)
            {
                m_repeater.Layout = viewDefinition.GetLayout();
                m_repeater.ItemTemplate = viewDefinition.GetElementFactory();
                m_repeater.ElementPrepared += OnRepeaterElementPrepared;
                m_repeater.ElementIndexChanged += OnRepeaterElementIndexChanged;
                m_repeater.ElementClearing += OnRepeaterElementClearing;

                m_headerPresenter = GetTemplateChild("HeaderPresenter") as ContentPresenter;
                m_headerPresenter.Content = viewDefinition.GetHeader();

                ConfigureScrolling(viewDefinition.GetContentOrientation(), m_scrollViewer);
            }
        }

        protected override AutomationPeer OnCreateAutomationPeer()
        {
            return new ItemsViewAutomationPeer(this);
        }

        private void OnRepeaterElementPrepared(ItemsRepeater sender, ItemsRepeaterElementPreparedEventArgs args)
        {
            var itemAsSelectable = args.Element as muxcp.ISelectable;
            if (itemAsSelectable != null)
            {
                if (Selector != null)
                {
                    // TODO: Need better registration mechanism. Need to unregister to avoid loops.
                    itemAsSelectable.SelectorAttached(Selector);
                }
            }

            UpdateModelIndex(args.Element, args.Index);
        }

        private void OnRepeaterElementIndexChanged(ItemsRepeater sender, ItemsRepeaterElementIndexChangedEventArgs args)
        {
            UpdateModelIndex(args.Element, args.NewIndex);
        }

        private void OnRepeaterElementClearing(ItemsRepeater sender, ItemsRepeaterElementClearingEventArgs args)
        {
            var itemAsSelectable = args.Element as muxcp.ISelectable;
            if (itemAsSelectable != null)
            {
                if (Selector != null)
                {
                    // TODO: Need better registration mechanism. Need to unregister to avoid loops.
                    itemAsSelectable.SelectorDetached(Selector);
                }

                itemAsSelectable.IsSelected = false;
            }
        }

        private void UpdateModelIndex(UIElement element, int index)
        {
            var selectable = element as muxcp.IModelIndex;
            if (selectable != null)
            {
                selectable.ModelIndex = index;
            }
        }

        private static void ConfigureScrolling(ContentOrientation contentOrientation, ScrollViewer scrollViewer)
        {
            if (scrollViewer == null) return;

            if (contentOrientation == ContentOrientation.Both)
            {
                scrollViewer.SetValue(ScrollViewer.VerticalScrollBarVisibilityProperty, ScrollBarVisibility.Auto);
                scrollViewer.SetValue(ScrollViewer.VerticalScrollModeProperty, ScrollMode.Auto);
                scrollViewer.SetValue(ScrollViewer.HorizontalScrollBarVisibilityProperty, ScrollBarVisibility.Auto);
                scrollViewer.SetValue(ScrollViewer.HorizontalScrollModeProperty, ScrollMode.Auto);
            }
            else if (contentOrientation == ContentOrientation.Horizontal)
            {
                scrollViewer.ClearValue(ScrollViewer.VerticalScrollBarVisibilityProperty);
                scrollViewer.ClearValue(ScrollViewer.VerticalScrollModeProperty);
                scrollViewer.SetValue(ScrollViewer.HorizontalScrollBarVisibilityProperty, ScrollBarVisibility.Auto);
                scrollViewer.SetValue(ScrollViewer.HorizontalScrollModeProperty, ScrollMode.Auto);
            }
            else
            {
                scrollViewer.SetValue(ScrollViewer.VerticalScrollBarVisibilityProperty, ScrollBarVisibility.Auto);
                scrollViewer.SetValue(ScrollViewer.VerticalScrollModeProperty, ScrollMode.Auto);
                scrollViewer.ClearValue(ScrollViewer.HorizontalScrollBarVisibilityProperty);
                scrollViewer.ClearValue(ScrollViewer.HorizontalScrollModeProperty);
            }
        }

        private static void OnSelectionModeChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            var me = d as ItemsView;
            me.Selector.Mode = (muxcp.SelectionMode)e.NewValue;
        }

        private static void OnViewDefinitionChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            var me = d as ItemsView;
            me.SetViewDefinition(me.ViewDefinition);

            // SetViewDefinition will ensure there is a selector
            if (me.ItemsSource != null)
            {
                me.Selector.Source = me.ItemsSource;
            }

            if (me != null & me.ViewDefinition != null & me.m_repeater != null)
            {
                // ItemTemplate has to be updated before layout so that we don't end up 
                // clearing elements into the stable reset pool.
                me.m_repeater.ItemTemplate = me.ViewDefinition.GetElementFactory();
                me.m_repeater.Layout = me.ViewDefinition.GetLayout();
                me.m_headerPresenter.Content = me.ViewDefinition.GetHeader();
            }

            ConfigureScrolling(me.ViewDefinition.GetContentOrientation(), me.m_scrollViewer);
        }

        private ScrollViewer m_scrollViewer;
        private ItemsRepeater m_repeater;
        private ContentPresenter m_headerPresenter;
    }
}
