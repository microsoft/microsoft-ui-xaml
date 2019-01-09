using System.Collections.ObjectModel;
using Windows.UI.Xaml.Markup;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls;
using Windows.Foundation;
using System.Diagnostics;

#if !BUILD_WINDOWS
using VirtualizingLayout = Microsoft.UI.Xaml.Controls.VirtualizingLayout;
using ItemsRepeater = Microsoft.UI.Xaml.Controls.ItemsRepeater;
using ElementFactoryGetArgs = Microsoft.UI.Xaml.Controls.ElementFactoryGetArgs;
using ElementFactoryRecycleArgs = Microsoft.UI.Xaml.Controls.ElementFactoryRecycleArgs;
using StackLayout = Microsoft.UI.Xaml.Controls.StackLayout;
using RecyclePool = Microsoft.UI.Xaml.Controls.RecyclePool;
using ElementFactory = Microsoft.UI.Xaml.Controls.ElementFactory;
using ItemsRepeaterElementClearingEventArgs = Microsoft.UI.Xaml.Controls.ItemsRepeaterElementClearingEventArgs;
using ItemsRepeaterElementIndexChangedEventArgs = Microsoft.UI.Xaml.Controls.ItemsRepeaterElementIndexChangedEventArgs;
using ItemsRepeaterElementPreparedEventArgs = Microsoft.UI.Xaml.Controls.ItemsRepeaterElementPreparedEventArgs;
using ISelectable = Microsoft.UI.Xaml.Controls.Primitives.ISelectable;
using IModelIndex = Microsoft.UI.Xaml.Controls.Primitives.IModelIndex;
#endif

namespace DEPControlsTestApp.ItemsViewPrototype
{
    [ContentProperty(Name = "ColumnDefinitions")]
    public class TableDefinition : ViewDefinitionBase
    {
        public delegate void IsGroupEventHandler(object sender, IsGroupEventArgs args);

        public event IsGroupEventHandler IsGroup;

        public TableDefinition()
        {
            m_rowLayout = new TableRowLayout();
            m_layout = new TableLayout(m_rowLayout); //new StackLayout();
            m_elementFactory = new TableElementFactory(this);

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
            DependencyProperty.Register(
                nameof(GroupHeaderTemplate),
                typeof(DataTemplate),
                typeof(TableDefinition),
                new PropertyMetadata(0));

        protected override UIElement GetHeaderCore()
        {
            return m_headerRow;
        }

        protected override VirtualizingLayout GetLayoutCore()
        {
            return m_layout;
        }

        protected override ContentOrientation GetContentOrientationCore()
        {
            return ContentOrientation.Both;
        }

        protected override object GetElementFactoryCore()
        {
            return m_elementFactory;
        }

        #region ElementFactory stuff
        // TODO: Fix the bug with changing view definition where items created from a different
        // generator are asked to be recycled by this view definition, but it doesn't know what
        // to do with them since they don't have a key.  If a reference to the template was 
        // available then the recycle pool could be retrieved.
        internal UIElement GetElement(ElementFactoryGetArgs context)
        {
            UIElement element = null;

            var data = context.Data;
            var owner = context.Parent as ItemsRepeater;
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
                    var repeater = new ItemsRepeater
                    {
                        HorizontalCacheLength = 0,
                        VerticalCacheLength = 0,
                        // TODO:Binding?? Unfortunately UWP does allow this outside of the data template.
                        ItemsSource = data,
                        ItemTemplate = m_elementFactory,
                        Layout = m_layout
                    };
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
                ItemsViewPrototype.RecyclePool.SetReuseKey(element, key);
            }
            else
            {
                // recycling.
                if (isGroupItem)
                {
                    var stack = element as StackPanel;
                    var repeater = stack.Children[1] as ItemsRepeater;
                    repeater.HorizontalCacheLength = 0;
                    repeater.VerticalCacheLength = 0;
                    // TODO:Binding?? Unfortunately UWP does allow this outside of the data template.
                    repeater.ItemsSource = data;
                    repeater.ItemTemplate = m_elementFactory;
                    repeater.Layout = m_layout;
                    repeater.ElementPrepared += OnGroupRepeaterElementPrepared;
                    repeater.ElementIndexChanged += OnGroupRepeaterElementIndexChanged;
                    repeater.ElementClearing += OnGroupRepeaterElementClearing;
                }
            }

            (element as FrameworkElement).DataContext = data;
            return element;
        }

        internal void RecycleElement(ElementFactoryRecycleArgs context)
        {
            var element = context.Element;
            var owner = context.Parent as ItemsRepeater;
            var key = ItemsViewPrototype.RecyclePool.GetReuseKey(element);
            if (key == "TableGroup" + this.GetHashCode().ToString())
            {
                var stack = element as StackPanel;
                var repeater = stack.Children[1] as ItemsRepeater;
                // TODO:Binding?? Unfortunately UWP does allow this outside of the data template.
                repeater.Layout = null;
                repeater.ItemsSource = null;

                repeater.ElementPrepared -= OnGroupRepeaterElementPrepared;
                repeater.ElementIndexChanged -= OnGroupRepeaterElementIndexChanged;
                repeater.ElementClearing -= OnGroupRepeaterElementClearing;
            }

            // TODO: Get cached instance in cpp winrt.
            // BUGBUG: This is hitting an exception when 'key' is null.  Need to take a look at who is 
            // generating that item since it must not be coming from the TableDefinition's GetElement method
            // What other recycle pools are involved?  If the RecyclePool.GetReuseKey method was public so
            // that everyone could be consistent then it 
            Debug.Assert(!string.IsNullOrWhiteSpace(key));
            GetRecyclePool().PutElement(element, key, owner);
        }

        private void OnGroupRepeaterElementPrepared(ItemsRepeater sender, ItemsRepeaterElementPreparedEventArgs args)
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

            UpdateModelIndex(args.Element, args.Index);
        }

        private void OnGroupRepeaterElementIndexChanged(ItemsRepeater sender, ItemsRepeaterElementIndexChangedEventArgs args)
        {
            UpdateModelIndex(args.Element, args.NewIndex);
        }

        private void OnGroupRepeaterElementClearing(ItemsRepeater sender, ItemsRepeaterElementClearingEventArgs args)
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

        private void UpdateModelIndex(UIElement element, int index)
        {
            var modelView = element as IModelIndex;
            if (modelView != null)
            {
                modelView.ModelIndex = index;
            }
        }

        #endregion

        private bool IsGroupItem(int index, ItemsRepeater owner, object data)
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

        private VirtualizingLayout m_layout;
        private ObservableCollection<ItemsViewColumnDefinitionBase> m_columnDefinitions;
        private TableRowLayout m_rowLayout;
        private TableHeaderRow m_headerRow;
        private Microsoft.UI.Xaml.Controls.IElementFactoryShim m_elementFactory;
        private IsGroupEventArgs m_isGroupEventArgs;
    }
}
