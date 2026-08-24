using System;
using Microsoft.UI.Input;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Input;
using XamlTelemtryViewerWInui3.Models;

namespace XamlTelemtryViewerWInui3.Helpers
{
    public class ColumnResizer
    {
        private Border? _currentSeparator;
        private Grid? _headerGrid;
        private ColumnWidths? _columnWidths;
        private int _resizingColumnIndex;
        private double _startX;
        private double _startWidth;
        private bool _isResizing;

        public void Initialize(Grid headerGrid, ColumnWidths columnWidths)
        {
            _headerGrid = headerGrid;
            _columnWidths = columnWidths;

            // Find all separators and add drag handlers
            // Separators are at odd indices: 1, 3, 5, 7, 9, 11, 13, 15, 17
            int[] separatorIndices = { 1, 3, 5, 7, 9, 11, 13, 15, 17 };
            
            foreach (int idx in separatorIndices)
            {
                if (idx < headerGrid.Children.Count && headerGrid.Children[idx] is Border separator)
                {
                    separator.PointerEntered += Separator_PointerEntered;
                    separator.PointerExited += Separator_PointerExited;
                    separator.PointerPressed += Separator_PointerPressed;
                    separator.PointerMoved += Separator_PointerMoved;
                    separator.PointerReleased += Separator_PointerReleased;
                }
            }
        }

        private void Separator_PointerEntered(object sender, PointerRoutedEventArgs e)
        {
            if (sender is Border separator)
            {
                separator.Background = new Microsoft.UI.Xaml.Media.SolidColorBrush(Windows.UI.Color.FromArgb(255, 100, 150, 255));
            }
        }

        private void Separator_PointerExited(object sender, PointerRoutedEventArgs e)
        {
            if (sender is Border separator && !_isResizing)
            {
                separator.Background = new Microsoft.UI.Xaml.Media.SolidColorBrush(Windows.UI.Color.FromArgb(255, 221, 221, 221));
            }
        }

        private void Separator_PointerPressed(object sender, PointerRoutedEventArgs e)
        {
            if (sender is Border separator && _headerGrid != null && _columnWidths != null)
            {
                _currentSeparator = separator;
                int separatorIndex = _headerGrid.Children.IndexOf(separator);
                _resizingColumnIndex = separatorIndex - 1; // Column is before separator
                
                var point = e.GetCurrentPoint(_headerGrid);
                _startX = point.Position.X;
                
                if (_resizingColumnIndex >= 0 && _resizingColumnIndex < _headerGrid.ColumnDefinitions.Count)
                {
                    _startWidth = _headerGrid.ColumnDefinitions[_resizingColumnIndex].Width.Value;
                    _isResizing = true;
                    separator.CapturePointer(e.Pointer);
                }
            }
        }

        private void Separator_PointerMoved(object sender, PointerRoutedEventArgs e)
        {
            if (!_isResizing || _currentSeparator == null || _headerGrid == null || _columnWidths == null)
                return;

            var point = e.GetCurrentPoint(_headerGrid);
            double delta = point.Position.X - _startX;
            double newWidth = Math.Max(40, _startWidth + delta); // Minimum width of 40px

            if (_resizingColumnIndex >= 0 && _resizingColumnIndex < _headerGrid.ColumnDefinitions.Count)
            {
                // Write the new width into the shared ColumnWidths; the header column and
                // every data row are bound to it, so both update together.
                switch (_resizingColumnIndex)
                {
                    case 0: _columnWidths.Timestamp = new GridLength(newWidth); break;
                    case 2: _columnWidths.ProviderGuid = new GridLength(newWidth); break;
                    case 4: _columnWidths.ProcessName = new GridLength(newWidth); break;
                    case 6: _columnWidths.ProcessId = new GridLength(newWidth); break;
                    case 8: _columnWidths.ProviderName = new GridLength(newWidth); break;
                    case 10: _columnWidths.EventName = new GridLength(newWidth); break;
                    case 12: _columnWidths.Level = new GridLength(newWidth); break;
                    case 14: _columnWidths.Opcode = new GridLength(newWidth); break;
                    case 16: _columnWidths.ThreadId = new GridLength(newWidth); break;
                    case 18: _columnWidths.Payload = new GridLength(newWidth); break;
                }
            }
        }

        private void Separator_PointerReleased(object sender, PointerRoutedEventArgs e)
        {
            if (sender is Border separator)
            {
                _isResizing = false;
                separator.ReleasePointerCapture(e.Pointer);
                
                if (!_isResizing)
                {
                    separator.Background = new Microsoft.UI.Xaml.Media.SolidColorBrush(Windows.UI.Color.FromArgb(255, 221, 221, 221));
                }
            }
        }
    }
}

