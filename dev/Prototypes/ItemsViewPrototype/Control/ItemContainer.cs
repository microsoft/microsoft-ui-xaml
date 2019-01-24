using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Automation.Peers;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Markup;
using System.Windows.Input;
using Windows.UI.Xaml.Input;
using Windows.Foundation;
using Windows.System;

#if !BUILD_WINDOWS
using ItemsRepeater = Microsoft.UI.Xaml.Controls.ItemsRepeater;
using Layout = Microsoft.UI.Xaml.Controls.Layout;
using IndexPath = Microsoft.UI.Xaml.Controls.IndexPath;
#endif

using Selector = Microsoft.UI.Xaml.Controls.Primitives.Selector;
using ISelectable = Microsoft.UI.Xaml.Controls.Primitives.ISelectable;
using IModelIndex = Microsoft.UI.Xaml.Controls.Primitives.IModelIndex;

namespace DEPControlsTestApp.ItemsViewPrototype
{
    [ContentProperty(Name = "Children")]
    public class ItemContainer : Control, ISelectable, IModelIndex
    {
        public ItemContainer()
        {
            DefaultStyleKey = typeof(ItemContainer);
        }

        public Layout Layout
        {
            get { return (Layout)GetValue(LayoutProperty); }
            set { SetValue(LayoutProperty, value); }
        }

        public static readonly DependencyProperty LayoutProperty =
            DependencyProperty.Register(
                nameof(Layout),
                typeof(Layout),
                typeof(ItemContainer),
                new PropertyMetadata(null));

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

        #region Command Properties

        public ICommand Command
        {
            get { return (ICommand)GetValue(CommandProperty); }
            set { SetValue(CommandProperty, value); }
        }
        public static readonly DependencyProperty CommandProperty =
            DependencyProperty.Register(nameof(Command), typeof(ICommand), typeof(ItemContainer), new PropertyMetadata(null));

        public object CommandParameter
        {
            get { return (object)GetValue(CommandParameterProperty); }
            set { SetValue(CommandParameterProperty, value); }
        }

        public static readonly DependencyProperty CommandParameterProperty =
            DependencyProperty.Register(nameof(CommandParameter), typeof(object), typeof(ItemContainer), new PropertyMetadata(null));

        #endregion

        #region IModelIndex

        public int ModelIndex
        {
            get { return (int)GetValue(ModelIndexProperty); }
            set
            {
                SetValue(ModelIndexProperty, value);
                // Hmm.. we have to re-evaluate even if the index didnt actually change.
                IndexChanged();
            }
        }

        public static readonly DependencyProperty ModelIndexProperty =
            DependencyProperty.Register(
                nameof(ModelIndex),
                typeof(int),
                typeof(ItemContainer),
                new PropertyMetadata(-1));

        #endregion

        #region ISelectable

        public bool IsSelected
        {
            get { return (bool)GetValue(IsSelectedProperty); }
            set { SetValue(IsSelectedProperty, value); }
        }

        public static readonly DependencyProperty IsSelectedProperty =
            DependencyProperty.Register(nameof(IsSelected),
                typeof(bool),
                typeof(ItemContainer),
                new PropertyMetadata(
                    false,
                    new PropertyChangedCallback(OnIsSelectedChanged)));

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
                if (parent is ItemsRepeater)
                {
                    int index = (parent as ItemsRepeater).GetElementIndex(child);
                    path.Insert(0, index);
                }

                child = parent;
                parent = parent.Parent as FrameworkElement;
            }

            if (path.Count == 0)
            {
                path.Insert(0, this.ModelIndex);
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

        // TODO: Alternative to this public registration method?
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

        #endregion

        public event RoutedEventHandler Invoked;

        protected override void OnApplyTemplate()
        {
            m_layoutPanel = GetTemplateChild("LayoutPanel") as LayoutPanel;

            foreach (var child in Children)
            {
                m_layoutPanel.Children.Add(child);
            }
        }

        protected override AutomationPeer OnCreateAutomationPeer()
        {
            return new ItemContainerAutomationPeer(this);
        }

        private void UpdateVisualStates()
        {
            if (IsSelected)
            {
                VisualStateManager.GoToState(this, "Selected", true);
            }
            else if (m_isPointerOver)
            {
                VisualStateManager.GoToState(this, "PointerOver", true);
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
            var item = d as ItemContainer;
            item.UpdateVisualStates();
        }

        #region Invoke

        bool raiseInvoke = false;
        private bool contextMenuRequested = false;
        protected override void OnPointerPressed(PointerRoutedEventArgs e)
        {
            base.OnPointerPressed(e);

            if (e.Pointer.PointerDeviceType != Windows.Devices.Input.PointerDeviceType.Touch)
            {
                var point = e.GetCurrentPoint(this);
                if (!point.Properties.IsLeftButtonPressed) // only for left-mouse click
                {
                    return;
                }
            }

            raiseInvoke = true;

            // Skip raising the Invoke event in PointerRelease if a context menu is shown.
            contextMenuRequested = false;

            // only added in RS4
            if (Windows.Foundation.Metadata.ApiInformation.IsEventPresent(typeof(UIElement).FullName, nameof(UIElement.ContextRequestedEvent)))
            {
                this.AddHandler(UIElement.ContextRequestedEvent, (TypedEventHandler<UIElement, ContextRequestedEventArgs>)OnContextRequested, true);
            }
        }

        protected override void OnPointerReleased(PointerRoutedEventArgs e)
        {
            base.OnPointerReleased(e);

            // GotFocus is asynchronous and may not have happened yet so if the conditions are met
            // set the flag to trigger an invoke when GotFocus finally happens (assuming it isn't canceled or re-directed
            // by someone else).
            if (!e.Handled && !contextMenuRequested)
            {
                this.Focus(FocusState.Programmatic);
                e.Handled = true; // mark it as handled or else it may bubble up and trigger a focus move

                // Avoid invoking after right-clicking with a mouse to open a context menu and then clicking on
                // the item again to dismiss the context menu
                if (raiseInvoke)
                    this.RaiseInvokedEvent();
            }

            raiseInvoke = false;
        }

        protected override void OnPointerEntered(PointerRoutedEventArgs e)
        {
            m_isPointerOver = true;
            UpdateVisualStates();
            base.OnPointerEntered(e);
        }

        protected override void OnPointerExited(PointerRoutedEventArgs e)
        {
            m_isPointerOver = false;
            UpdateVisualStates();
            base.OnPointerExited(e);
        }

        // Invoke on Enter key down
        protected override void OnKeyDown(KeyRoutedEventArgs e)
        {
            base.OnKeyDown(e);

            if (!e.KeyStatus.WasKeyDown && e.Key == VirtualKey.Enter && this.FocusState != FocusState.Unfocused)
            {
                this.RaiseInvokedEvent();
            }
        }

        private void OnContextRequested(UIElement sender, ContextRequestedEventArgs args)
        {
            contextMenuRequested = true;
            sender.ContextRequested -= OnContextRequested;
            sender.ContextCanceled += OnContextCanceled;
        }

        private void OnContextCanceled(UIElement sender, RoutedEventArgs args)
        {
            contextMenuRequested = false;
            sender.ContextCanceled -= OnContextCanceled;
        }

        internal void RaiseInvokedEvent()
        {
            if (Command != null && Command.CanExecute(this.CommandParameter))
            {
                this.Command.Execute(this.CommandParameter);
            }

            this.Invoked?.Invoke(this, new RoutedEventArgs());
        }

        #endregion


        private LayoutPanel m_layoutPanel;
        private Selector m_selector;
        private bool m_isPointerOver = false;
    }
}
