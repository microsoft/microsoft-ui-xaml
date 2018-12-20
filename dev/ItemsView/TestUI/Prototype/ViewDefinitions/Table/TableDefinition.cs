using Microsoft.UI.Xaml.Controls;
using System;
using System.Collections.ObjectModel;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Markup;

#if !BUILD_WINDOWS
using VirtualizingLayoutBase = Microsoft.UI.Xaml.Controls.VirtualizingLayoutBase;
using ViewGenerator = Microsoft.UI.Xaml.Controls.ViewGenerator;
using Repeater = Microsoft.UI.Xaml.Controls.Repeater;
using ElementFactoryGetContext = Microsoft.UI.Xaml.Controls.ElementFactoryGetContext;
using ElementFactoryRecycleContext = Microsoft.UI.Xaml.Controls.ElementFactoryRecycleContext;
using StackLayout = Microsoft.UI.Xaml.Controls.StackLayout;
using RecyclePool = Microsoft.UI.Xaml.Controls.RecyclePool;
using ElementClearingEventArgs = Microsoft.UI.Xaml.Controls.ElementClearingEventArgs;
using ElementIndexChangedEventArgs = Microsoft.UI.Xaml.Controls.ElementIndexChangedEventArgs;
using ElementPreparedEventArgs = Microsoft.UI.Xaml.Controls.ElementPreparedEventArgs;
#endif

namespace MUXControlsTestApp.ItemsViewPrototype
{
    [ContentProperty(Name = "ColumnDefinitions")]
    public class TableDefinition : ViewDefinitionBase
    {
        public delegate void IsGroupEventHandler(object sender, IsGroupEventArgs args);

        public event IsGroupEventHandler IsGroup;

        public TableDefinition()
        {
            m_layout = new StackLayout();
            m_viewGenerator = new TableViewGenerator(this);
            m_rowLayout = new TableRowLayout();

            m_headerRow = new TableHeaderRow()
            {
                Layout = m_rowLayout,
                ColumnDefinitions = ColumnDefinitions
            };
        }

        public ObservableCollection<ItemsViewColumnDefinitionBase> ColumnDefinitions
        {
            get
            {
                if (m_columnDefinitions == null)
                {
                    m_columnDefinitions = new ObservableCollection<ItemsViewColumnDefinitionBase>();
                }

                return m_columnDefinitions;
            }
        }

        public DataTemplate GroupHeaderTemplate
        {
            get { return (DataTemplate)GetValue(GroupHeaderTemplateProperty); }
            set { SetValue(GroupHeaderTemplateProperty, value); }
        }

        public static readonly DependencyProperty GroupHeaderTemplateProperty =
            DependencyProperty.Register("GroupHeaderTemplate", typeof(DataTemplate), typeof(TableDefinition), new PropertyMetadata(0));

        protected override UIElement GetHeaderCore()
        {
            return m_headerRow;
        }

        protected override VirtualizingLayoutBase GetLayoutCore()
        {
            return m_layout;
        }

        protected override ViewGenerator GetViewGeneratorCore()
        {
            return m_viewGenerator;
        }

        #region ViewGenerator stuff
        // TODO: Is there a way perhaps to re-use recycling view generator?
        // Perhaps a base class that has the interesting overrides.
        internal UIElement GetElement(ElementFactoryGetContext context)
        {
            UIElement element = null;

            var data = context.Data;
            var owner = context.Parent as Repeater;
            bool isGroupItem = IsGroupItem(-1, owner, data);
            var pool = GetRecyclePool();

            // The key needs to be unique per TableDefinition since the 
            // recycle pool is on ItemsView and is shared with other definitions.
            string key = isGroupItem ? "TableGroup" : "TableItem";
            key += this.GetHashCode().ToString();

            // Check in recyclePool
            element = pool.TryGetElement(key, owner);

            if (element == null)
            {
                if (isGroupItem)
                {
                    var e = GroupHeaderTemplate.LoadContent() as UIElement;
                    var stack = new StackPanel();
                    stack.Children.Add(e);
                    var repeater = new Repeater();
                    repeater.MaximumHorizontalCacheLength = 0;
                    repeater.MaximumVerticalCacheLength = 0;
                    // TODO:Binding?? Unfortunately UWP does allow this outside of the data template.
                    repeater.ItemsSource = data;
                    repeater.ViewGenerator = m_viewGenerator;
                    repeater.Layout = m_layout;
                    repeater.ElementPrepared += OnGroupRepeaterElementPrepared;
                    repeater.ElementIndexChanged += OnGroupRepeaterElementIndexChanged;
                    repeater.ElementClearing += OnGroupRepeaterElementClearing;
                    stack.Children.Add(repeater);
                    element = stack;
                }
                else
                {
                    var row = new TableRow()
                    {
                        Layout = m_rowLayout,
                        ColumnDefinitions = ColumnDefinitions
                    };

                    element = row;
                }

                // TODO: Get cached instance in cpp winrt.
                Microsoft.UI.Xaml.Controls.RecyclePool.SetReuseKey(element, key);
            }
            else
            {
                // recycling.
                if (isGroupItem)
                {
                    var stack = element as StackPanel;
                    var repeater = stack.Children[1] as Repeater;
                    repeater.MaximumHorizontalCacheLength = 0;
                    repeater.MaximumVerticalCacheLength = 0;
                    // TODO:Binding?? Unfortunately UWP does allow this outside of the data template.
                    repeater.ItemsSource = data;
                    repeater.ViewGenerator = m_viewGenerator;
                    repeater.Layout = m_layout;
                    repeater.ElementPrepared += OnGroupRepeaterElementPrepared;
                    repeater.ElementIndexChanged += OnGroupRepeaterElementIndexChanged;
                    repeater.ElementClearing += OnGroupRepeaterElementClearing;
                }
            }

            (element as FrameworkElement).DataContext = data;
            return element;
        }

        internal void RecycleElement(ElementFactoryRecycleContext context)
        {
            var element = context.Element;
            var owner = context.Parent as Repeater;
            var key = Microsoft.UI.Xaml.Controls.RecyclePool.GetReuseKey(element);
            if (key == "TableGroup" + this.GetHashCode().ToString())
            {
                var stack = element as StackPanel;
                var repeater = stack.Children[1] as Repeater;
                // TODO:Binding?? Unfortunately UWP does allow this outside of the data template.
                repeater.Layout = null;
                repeater.ItemsSource = null;
                repeater.ViewGenerator = null;

                repeater.ElementPrepared -= OnGroupRepeaterElementPrepared;
                repeater.ElementIndexChanged -= OnGroupRepeaterElementIndexChanged;
                repeater.ElementClearing -= OnGroupRepeaterElementClearing;
            }

            // TODO: Get cached instance in cpp winrt.
            GetRecyclePool().PutElement(element, key, owner);
        }

        private void OnGroupRepeaterElementPrepared(Repeater sender, ElementPreparedEventArgs args)
        {
            var itemAsSelectable = args.Element as ISelectable;
            if (itemAsSelectable != null)
            {
                var selector = Selector();
                if (selector != null)
                {
                    // TODO: Need better registration mechanism. Need to unregister to avoid loops.
                    itemAsSelectable.SelectorAttached(selector);
                }
                else
                {
                    itemAsSelectable.IsSelected = false;
                }
            }

            UpdateSelectionIndex(args.Element, args.Index);
        }

        private void OnGroupRepeaterElementIndexChanged(Repeater sender, ElementIndexChangedEventArgs args)
        {
            UpdateSelectionIndex(args.Element, args.NewIndex);
        }

        private void OnGroupRepeaterElementClearing(Repeater sender, ElementClearingEventArgs args)
        {
            var itemAsSelectable = args.Element as ISelectable;
            if (itemAsSelectable != null)
            {
                var selector = Selector();
                if (selector != null)
                {
                    // TODO: Need better registration mechanism. Need to unregister to avoid loops.
                    itemAsSelectable.SelectorDetached(selector);
                }

                itemAsSelectable.IsSelected = false;
            }
        }

        private void UpdateSelectionIndex(UIElement element, int index)
        {
            var selectable = element as IRepeatedIndex;
            if (selectable != null)
            {
                selectable.RepeatedIndex = index;
            }
        }

        #endregion

        private bool IsGroupItem(int index, Repeater owner, object data)
        {
            bool isGroup = false;
            if (IsGroup != null)
            {
                if (m_isGroupEventArgs == null)
                {
                    m_isGroupEventArgs = new IsGroupEventArgs();
                }

                m_isGroupEventArgs.IsGroup = false;
                m_isGroupEventArgs.Owner = owner;
                m_isGroupEventArgs.Index = index;
                m_isGroupEventArgs.Item = data;

                IsGroup(this, m_isGroupEventArgs);
                isGroup = m_isGroupEventArgs.IsGroup;
            }

            return isGroup;
        }

        private VirtualizingLayoutBase m_layout;
        private ObservableCollection<ItemsViewColumnDefinitionBase> m_columnDefinitions;
        private TableRowLayout m_rowLayout;
        private TableHeaderRow m_headerRow;
        private ViewGenerator m_viewGenerator;
        private IsGroupEventArgs m_isGroupEventArgs;
    }
}
