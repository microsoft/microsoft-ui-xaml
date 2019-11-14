using Microsoft.UI.Xaml.Controls;
using System;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;

#if !BUILD_WINDOWS
using RecyclePool = Microsoft.UI.Xaml.Controls.RecyclePool;
using Selector = Microsoft.UI.Xaml.Controls.Primitives.Selector;
#endif

namespace DEPControlsTestApp.ItemsViewPrototype
{
    public class ItemsViewBase : Control
    {
        protected ItemsViewBase()
        {
        }

        private object m_itemsSource;
        public object ItemsSource
        {
            get { return m_itemsSource; }
            set { SetValue(ItemsSourceProperty, value); }
        }

        public RecyclePool RecyclePool
        {
            get
            {
                if (m_recyclePool == null)
                {
                    m_recyclePool = new RecyclePool();
                }

                return m_recyclePool;
            }

            set { m_recyclePool = value; }
        }

        // TODO:  Update these to CanSort/CanFilter and Sorting/Filtering events.
        public Action<int, bool /* ascending */ > SortFunc { get; set; }

        public Action<int, string> FilterFunc { get; set; }

        public static readonly DependencyProperty ItemsSourceProperty =
            DependencyProperty.Register(
                nameof(ItemsSource),
                typeof(object),
                typeof(ItemsViewBase),
                new PropertyMetadata(
                    null,
                    new PropertyChangedCallback(OnItemsSourceChanged)));

        // public event EventHandler<SelectionModelChildrenRequestedEventArgs> ChildrenRequested;

        protected ViewDefinitionBase GetViewDefinition()
        {
            return m_viewDefinition;
        }

        internal Selector Selector
        {
            get
            {
                if (m_selectionManager == null)
                {
                    m_selectionManager = new Selector();
                
                    // TODO: Figure out how to handle has-a case 
                    // m_selectionManager.ChildrenRequested += OnSelectionManagerChildrenRequested;
                }

                return m_selectionManager;
            }

            private set
            {
                m_selectionManager = value;
            }
        }

        //private void OnSelectionManagerChildrenRequested(SelectionModel sender, SelectionModelChildrenRequestedEventArgs args)
        //{
        //    if(ChildrenRequested != null)
        //    {
        //        ChildrenRequested(sender, args);
        //    }
        //}

        protected void SetViewDefinition(ViewDefinitionBase value)
        {
            m_viewDefinition = value;
            m_viewDefinition.RecyclePool(RecyclePool);
            
            Selector.SelectorView = this;
            m_viewDefinition.Selector(Selector);
        }

        private static void OnItemsSourceChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            var me = d as ItemsViewBase;
            me.m_itemsSource = e.NewValue;
            if (me.m_viewDefinition != null && me.m_viewDefinition.Selector() != null)
            {
                me.m_viewDefinition.Selector().Source = me.ItemsSource;
            }
        }

        ViewDefinitionBase m_viewDefinition;
        RecyclePool m_recyclePool;
        Selector m_selectionManager;
    }
}
