using Windows.UI.Xaml;
using Windows.UI.Xaml.Automation.Peers;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Input;

namespace DEPControlsTestApp.ItemsViewPrototype
{
    internal class TableHeaderCell : TableCell
    {
        public TableHeaderCell()
        {
            this.DefaultStyleKey = typeof(TableHeaderCell);
        }

        protected override AutomationPeer OnCreateAutomationPeer()
        {
            return new TableHeaderCellAutomationPeer(this);
        }

        protected override void OnApplyTemplate()
        {
            if (m_filterTb != null)
            {
                m_filterTb.KeyUp -= OnFilterTextKeyUp;
            }

            m_filterTb = GetTemplateChild("FilterText") as TextBox;
            if (m_filterTb != null)
            {
                m_filterTb.KeyUp += OnFilterTextKeyUp;
            }

            base.OnApplyTemplate();
        }

        protected override void OnPointerPressed(PointerRoutedEventArgs e)
        {
            CapturePointerPrivate(e.Pointer);
            base.OnPointerEntered(e);
        }

        protected override void OnPointerMoved(PointerRoutedEventArgs e)
        {
            // If pointer moved on a captured pointer id, then we resize.
            if (e.Pointer.PointerId == m_capturedPointerId && 
                e.Pointer.IsInContact)
            {
                var point = e.GetCurrentPoint(this);
                if (m_previousX != 0)
                {
                    UpdateColumnWidth(point.Position.X - m_previousX);
                }
                m_previousX = point.Position.X;
            }

            base.OnPointerMoved(e);
        }

        protected override void OnPointerReleased(PointerRoutedEventArgs e)
        {
            ReleasePointerCapturePrivate(e.Pointer);

            // do a sort operation on this column
            if (!e.Handled)
            {
                // hacky way to get to the tableview.
                var itemsView = TreeHelper.FindItemsView(this);
                if (itemsView != null)
                {
                    if (itemsView.SortFunc != null)
                    {
                        m_currentSortState = m_currentSortState == SortState.Unsorted || m_currentSortState == SortState.SortedDescending ? SortState.SortedAscending : SortState.SortedDescending;

                        // TODO:Temporary sort call here is hacky. Need to move to new UX 
                        // that we come up with. Task 15867030: TableView: Sorting/Filtering
                        itemsView.SortFunc(ColumnIndex, m_currentSortState == SortState.SortedAscending);
                        UpdateVisualStates();
                    }
                }
            }

            base.OnPointerReleased(e);
        }

        protected override void OnPointerCaptureLost(PointerRoutedEventArgs e)
        {
            ReleasePointerCapturePrivate(e.Pointer);
            base.OnPointerCaptureLost(e);
        }

        protected override void OnPointerCanceled(PointerRoutedEventArgs e)
        {
            ReleasePointerCapturePrivate(e.Pointer);
            base.OnPointerCanceled(e);
        }

        private void UpdateVisualStates()
        {
            if (m_currentSortState == SortState.Unsorted)
            {
                VisualStateManager.GoToState(this, "UnSorted", true);
            }
            else if (m_currentSortState == SortState.SortedAscending)
            {
                VisualStateManager.GoToState(this, "SortedAscending", true);
            }
            else
            {
                VisualStateManager.GoToState(this, "SortedDescending", true);
            }
        }

        private void OnFilterTextKeyUp(object sender, KeyRoutedEventArgs e)
        {
            // do a filter operation on this column
            if (!e.Handled)
            {
                // hacky way to get to the tableview.
                var itemsView = TreeHelper.FindItemsView(this);
                if (itemsView != null)
                {
                    if (itemsView.FilterFunc != null)
                    {
                        itemsView.FilterFunc(ColumnIndex, m_filterTb.Text);
                    }
                }
            }
        }

        private void UpdateColumnWidth(double delta)
        {
            //var panel = Parent as LayoutPanel;
            var panel = Parent as Microsoft.UI.Xaml.Controls.LayoutPanel;
            if (panel != null)
            {
                var layout = panel.Layout as TableRowLayout;
                if (layout != null)
                {
                    if (layout.ColumnWidths.ContainsKey(ColumnIndex))
                    {
                        layout.ColumnWidths[ColumnIndex] += delta;
                        layout.InvalidateLayout();
                    }
                }
            }
        }

        private void CapturePointerPrivate(Pointer pointer)
        {
            if (m_capturedPointerId == uint.MaxValue)
            {
                // New Pointer press, capture.
                // We ignore pointer pressed if one pointer is already captured.
                m_previousX = 0;
                CapturePointer(pointer);
                m_capturedPointerId = pointer.PointerId;
            }
        }

        private void ReleasePointerCapturePrivate(Pointer pointer)
        {
            if (pointer.PointerId == m_capturedPointerId)
            {
                m_capturedPointerId = uint.MaxValue;
                ReleasePointerCapture(pointer);
            }
        }

        uint m_capturedPointerId = uint.MaxValue;
        double m_previousX = 0.0;
        TextBox m_filterTb = null;
        SortState m_currentSortState = SortState.Unsorted;
    }
}
