using Microsoft.UI.Xaml.Controls;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Automation.Peers;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Markup;

#if !BUILD_WINDOWS
using Repeater = Microsoft.UI.Xaml.Controls.Repeater;
using LayoutBase = Microsoft.UI.Xaml.Controls.LayoutBase;
using IndexPath = Microsoft.UI.Xaml.Controls.IndexPath;
#endif

namespace MUXControlsTestApp.ItemsViewPrototype
{
    [ContentProperty(Name = "Children")]
    public class ItemsViewItem : Control, ISelectable, IRepeatedIndex
    {
        public ItemsViewItem()
        {
            DefaultStyleKey = typeof(ItemsViewItem);
        }

        public LayoutBase Layout
        {
            get { return (LayoutBase)GetValue(LayoutProperty); }
            set { SetValue(LayoutProperty, value); }
        }

        public static readonly DependencyProperty LayoutProperty =
            DependencyProperty.Register("Layout", typeof(LayoutBase), typeof(ItemsViewItem), new PropertyMetadata(null));

        ObservableCollection<UIElement> _children;
        public ObservableCollection<UIElement> Children
        {
            get
            {
                if (_children == null)
                {
                    _children = new ObservableCollection<UIElement>();
                }

                return _children;
            }
        }

        #region ISelectable

        public int RepeatedIndex
        {
            get { return (int)GetValue(RepeatedIndexProperty); }
            set
            {
                SetValue(RepeatedIndexProperty, value);
                // Hmm.. we have to re-evaluate even if the index didnt actually change.
                IndexChanged();
            }
        }

        public static readonly DependencyProperty RepeatedIndexProperty =
            DependencyProperty.Register("RepeatedIndex", typeof(int), typeof(ItemsViewItem), new PropertyMetadata(-1));

        public bool IsSelected
        {
            get { return (bool)GetValue(IsSelectedProperty); }
            set { SetValue(IsSelectedProperty, value); }
        }

        public static readonly DependencyProperty IsSelectedProperty =
            DependencyProperty.Register("IsSelected", typeof(bool), typeof(ItemsViewItem), new PropertyMetadata(false, new PropertyChangedCallback(OnIsSelectedChanged)));

        public Selector Selector
        {
            get
            {
                return m_selector;
            }
        }

        // This walk can be expensive. Figure out a way to do this less often.
        public IndexPath GetIndexPath()
        {
            var child = this as FrameworkElement;
            var parent = child.Parent as FrameworkElement;
            List<int> path = new List<int>();

            while (parent != null && !(parent is ItemsViewBase))
            {
                if (parent is Repeater)
                {
                    int index = (parent as Repeater).GetElementIndex(child);
                    path.Insert(0, index);
                }

                child = parent;
                parent = parent.Parent as FrameworkElement;
            }

            EnsurePathIsValid(path);

            var ip = IndexPath.CreateFromIndices(path);
            // Debug.WriteLine(ip);
            return ip;
        }

        private static void EnsurePathIsValid(List<int> path)
        {
            foreach (var index in path)
            {
                if (index < 0)
                {
                    throw new InvalidOperationException("Path requested for a recycled item");
                }
            }
        }

        #endregion

        public void SelectorAttached(Selector selector)
        {
            Debug.Assert(selector != m_selector);
            if (m_selector != null)
            {
                m_selector.UnregisterSelectable(this);
            }

            m_selector = selector;

            if (m_selector != null)
            {
                m_selector.RegisterSelectable(this);
            }
        }

        public void SelectorDetached(Selector selector)
        {
            Debug.Assert(selector == m_selector);
            if (m_selector != null)
            {
                m_selector.UnregisterSelectable(this);
            }

            m_selector = null;
        }

        protected override void OnApplyTemplate()
        {
            m_layoutPanel = GetTemplateChild("LayoutPanel") as LayoutPanel;
            m_layoutPanel.Layout = Layout;

            foreach (var child in Children)
            {
                m_layoutPanel.Children.Add(child);
            }
        }

        protected override AutomationPeer OnCreateAutomationPeer()
        {
            return new ItemsViewItemAutomationPeer(this);
        }

        private void UpdateVisualStates()
        {
            if (IsSelected)
            {
                VisualStateManager.GoToState(this, "Selected", true);
            }
            else
            {
                VisualStateManager.GoToState(this, "Normal", true);
            }
        }

        private void IndexChanged()
        {
            if (m_selector != null)
            {
                m_selector.InvalidateSelectable(this);
            }
            else
            {
                throw new InvalidOperationException("No Selector?");
            }
        }

        private static void OnIsSelectedChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            var item = d as ItemsViewItem;
            item.UpdateVisualStates();
        }

        private LayoutPanel m_layoutPanel;
        private Selector m_selector;
    }
}
