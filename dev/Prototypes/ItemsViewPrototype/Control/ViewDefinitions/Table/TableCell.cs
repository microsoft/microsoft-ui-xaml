using Windows.UI.Xaml;
using Windows.UI.Xaml.Automation.Peers;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Input;

namespace DEPControlsTestApp.ItemsViewPrototype
{
    public class TableCell : ContentControl
    {
        public int ColumnIndex { get; internal set; }

        public bool IsPressed
        {
            get
            {
                return m_isPressed;
            }

            set
            {
                m_isPressed = value;
            }
        }

        public TableCell()
        {
            this.DefaultStyleKey = typeof(TableCell);
        }

        protected override AutomationPeer OnCreateAutomationPeer()
        {
            return new TableCellAutomationPeer(this);
        }

        protected override void OnPointerPressed(PointerRoutedEventArgs e)
        {
            m_isPressed = true;
            UpdateVisualStates();
            base.OnPointerPressed(e);
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

        protected override void OnPointerReleased(PointerRoutedEventArgs e)
        {
            m_isPressed = false;
            UpdateVisualStates();
            base.OnPointerReleased(e);
        }

        protected override void OnPointerCanceled(PointerRoutedEventArgs e)
        {
            m_isPressed = false;
            m_isPointerOver = false;
            UpdateVisualStates();
            base.OnPointerCanceled(e);
        }

        protected override void OnPointerCaptureLost(PointerRoutedEventArgs e)
        {
            m_isPressed = false;
            UpdateVisualStates();
            base.OnPointerCaptureLost(e);
        }

        private void UpdateVisualStates()
        {
            if (m_isPressed)
            {
                VisualStateManager.GoToState(this, "Pressed", true);
            }
            else if (m_isPointerOver)
            {
                VisualStateManager.GoToState(this, "PointerOver", true);
            }
            else
            {
                VisualStateManager.GoToState(this, "Normal", true);
            }

            // TODO: Not wise to walk the tree every time.
            var itemsView = TreeHelper.FindItemsView(this);

            if (m_isPointerOver && itemsView != null && itemsView.FilterFunc != null)
            {
                VisualStateManager.GoToState(this, "FilterEnabled", true);
            }
            else
            {
                VisualStateManager.GoToState(this, "FilterDisabled", true);
            }
        }

        bool m_isPressed = false;
        bool m_isPointerOver = false;
    }
}
