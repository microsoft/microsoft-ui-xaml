using System;
using Microsoft.UI.Xaml.Controls;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Automation.Peers;
using Windows.UI.Xaml.Controls;

#if !BUILD_WINDOWS
using Repeater = Microsoft.UI.Xaml.Controls.Repeater;
using ElementIndexChangedEventArgs = Microsoft.UI.Xaml.Controls.ElementIndexChangedEventArgs;
using ElementClearingEventArgs = Microsoft.UI.Xaml.Controls.ElementClearingEventArgs;
using ElementPreparedEventArgs = Microsoft.UI.Xaml.Controls.ElementPreparedEventArgs;
#endif

namespace MUXControlsTestApp.ItemsViewPrototype
{
    public class ItemsView : ItemsViewBase
    {
        public ItemsView()
        {
            this.DefaultStyleKey = typeof(ItemsView);
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

        public Selector Selector
        {
            get
            {
                return GetSelector();
            }
        }

        public UIElement GetElement(int index)
        {
            return m_repeater == null ? null : m_repeater.GetElement(index);
        }


        public static readonly DependencyProperty ViewDefinitionProperty =
            DependencyProperty.Register("ViewDefinition", typeof(ViewDefinitionBase), typeof(ItemsView), new PropertyMetadata(null, new PropertyChangedCallback(OnViewDefinitionChanged)));

        protected override void OnApplyTemplate()
        {
            m_repeater = GetTemplateChild("Repeater") as Repeater;
            var viewDefinition = GetViewDefinition();
            m_repeater.Layout = viewDefinition.GetLayout();
            m_repeater.ViewGenerator = viewDefinition.GetViewGenerator();
            m_repeater.ElementPrepared += OnRepeaterElementPrepared;
            m_repeater.ElementIndexChanged += OnRepeaterElementIndexChanged;
            m_repeater.ElementClearing += OnRepeaterElementClearing;

            m_headerPresenter = GetTemplateChild("HeaderPresenter") as ContentPresenter;
            m_headerPresenter.Content = viewDefinition.GetHeader();
        }

        protected override AutomationPeer OnCreateAutomationPeer()
        {
            return new ItemsViewAutomationPeer(this);
        }

        private void OnRepeaterElementPrepared(Repeater sender, ElementPreparedEventArgs args)
        {
            var itemAsSelectable = args.Element as ISelectable;
            if (itemAsSelectable != null)
            {
                var selector = GetSelector();
                if (selector != null)
                {
                    // TODO: Need better registration mechanism. Need to unregister to avoid loops.
                    itemAsSelectable.SelectorAttached(selector);
                }
            }

            UpdateRepeatedIndex(args.Element, args.Index);
        }

        private void OnRepeaterElementIndexChanged(Repeater sender, ElementIndexChangedEventArgs args)
        {
            UpdateRepeatedIndex(args.Element, args.NewIndex);
        }

        private void OnRepeaterElementClearing(Repeater sender, ElementClearingEventArgs args)
        {
            var itemAsSelectable = args.Element as ISelectable;
            if (itemAsSelectable != null)
            {
                var selector = GetSelector();
                if (selector != null)
                {
                    // TODO: Need better registration mechanism. Need to unregister to avoid loops.
                    itemAsSelectable.SelectorDetached(selector);
                }

                itemAsSelectable.IsSelected = false;
            }
        }

        private void UpdateRepeatedIndex(UIElement element, int index)
        {
            var selectable = element as IRepeatedIndex;
            if (selectable != null)
            {
                selectable.RepeatedIndex = index;
            }
        }

        private static void OnViewDefinitionChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            var me = d as ItemsView;
            me.SetViewDefinition(me.ViewDefinition);

            // SetViewDefinition will ensure there is a selector
            if (me.ItemsSource != null)
            {
                me.GetSelector().Source = me.ItemsSource;
            }

            if (me != null & me.ViewDefinition != null & me.m_repeater != null)
            {
                // ViewGenerator has to be updated before layout so that we don't end up 
                // clearing elements into the stable reset pool.
                me.m_repeater.ViewGenerator = me.ViewDefinition.GetViewGenerator();
                me.m_repeater.Layout = me.ViewDefinition.GetLayout();
                me.m_headerPresenter.Content = me.ViewDefinition.GetHeader();
            }
        }

        private Repeater m_repeater;
        private ContentPresenter m_headerPresenter;
    }
}
