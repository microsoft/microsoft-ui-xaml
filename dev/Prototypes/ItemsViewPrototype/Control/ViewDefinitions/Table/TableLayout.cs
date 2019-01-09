using Microsoft.UI.Xaml.Controls;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Windows.Foundation;
using Windows.UI.Xaml;

namespace DEPControlsTestApp.ItemsViewPrototype
{
    /// <summary>
    /// Layout for the table
    /// </summary>
    /// <remarks>
    /// A TableLayout is the same as a StackLayout, but can't always limit its desired size
    /// to what was available since StackLayout constrains one axis and always returns that constrained size
    /// in its desired size.  With a table there may be more columns than can fit within the viewport.
    /// When that happens a user should be able to scroll horizontally.
    /// This TableLayout relies on StackLayout to do all the virtualization in the vertical direction
    /// and uses the cached column sizes for the TableRowLayout to return its width.
    /// </remarks>
    public class TableLayout : StackLayout
    {
        private TableRowLayout m_rowLayout;
        public TableRowLayout RowLayout
        {
            get { return m_rowLayout; }
            set { SetValue(RowLayoutProperty, value); }
        }

        public static readonly DependencyProperty RowLayoutProperty =
            DependencyProperty.Register(
                nameof(RowLayout),
                typeof(TableRowLayout),
                typeof(TableLayout),
                new PropertyMetadata(null, OnRowLayoutChanged));

        private static void OnRowLayoutChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            var me = d as TableLayout;

            if (me.m_rowLayout != null)
            {
                me.m_rowLayout.MeasureInvalidated -= me.RowLayout_MeasureInvalidated;
            }

            me.m_rowLayout = e.NewValue as TableRowLayout;

            if (me.m_rowLayout != null)
            {
                me.m_rowLayout.MeasureInvalidated += me.RowLayout_MeasureInvalidated;
            }
        }

        public TableLayout() { }

        public TableLayout(TableRowLayout rowLayout)
        {
            this.RowLayout = rowLayout;
        }

        private void RowLayout_MeasureInvalidated(Layout sender, object args)
        {
            this.InvalidateMeasure();
        }

        protected override Size MeasureOverride(VirtualizingLayoutContext context, Size availableSize)
        {
            var desiredSize = base.MeasureOverride(context, availableSize);
            var desiredWidth = 0d;
            if (this.RowLayout != null && this.RowLayout.ColumnWidths.Count > 0)
            {
                foreach (var column in this.RowLayout.ColumnWidths)
                {
                    desiredWidth += column.Value;
                }
                desiredSize.Width = desiredWidth;
            }

            return desiredSize;
        }
    }
}
