using System;
using System.IO;
using System.Collections.Generic;
using System.Linq;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using XamlTelemtryViewerWInui3.ViewModels;
using XamlTelemtryViewerWInui3.Models.Timeline;
using XamlTelemtryViewerWInui3.Services;
using XamlTelemtryViewerWInui3.Helpers;

namespace XamlTelemtryViewerWInui3
{
    public sealed partial class MainWindow : Window
    {
        private MainViewModel _viewModel;
        private FilterWindow? _filterWindow;
        private readonly ColumnResizer _columnResizer = new();

        public MainWindow()
        {
            InitializeComponent();
            _viewModel = new MainViewModel();

            // The header and every (virtualized) data row bind to a single ColumnWidths
            // instance declared in XAML (SharedColumnWidths). Point the view-model and the
            // drag-resizer at that same instance so a header drag updates them all at once.
            if (this.Content is Grid rootGrid &&
                rootGrid.Resources.TryGetValue("SharedColumnWidths", out var sharedRes) &&
                sharedRes is Models.ColumnWidths shared)
            {
                _viewModel.ColumnWidths = shared;
            }

            _viewModel.SetWindow(this);

            // Store reference in App
            if (App.Current is App app)
            {
                app.MainViewModel = _viewModel;
            }

            // Set DataContext on the root Grid
            if (this.Content is Grid grid)
            {
                grid.DataContext = _viewModel;
            }

            // Attach drag-to-resize on the header separators; it writes into the shared
            // ColumnWidths, which the header and every row are bound to (no manual sync needed).
            DispatcherQueue.TryEnqueue(() =>
            {
                _columnResizer.Initialize(HeaderGrid, _viewModel.ColumnWidths);
            });

            // Close child windows when MainWindow closes
            this.Closed += (s, e) =>
            {
                _filterWindow?.Close();
                _viewModel.GetTimelineWindow()?.Close();
            };
        }

        private void SelectTraceFileButton_Click(object sender, RoutedEventArgs e)
        {
            _viewModel.SelectTraceFile();
        }

        private async void LoadTraceButton_Click(object sender, RoutedEventArgs e)
        {
            await _viewModel.LoadTraceAsync();
        }

        private void FiltersButton_Click(object sender, RoutedEventArgs e)
        {
            // If filter window already exists and is visible, just activate it
            if (_filterWindow != null)
            {
                try
                {
                    _filterWindow.Activate();
                    return;
                }
                catch
                {
                    // Window was closed, clear the reference
                    _filterWindow = null;
                }
            }

            // Create new filter window
            _filterWindow = new FilterWindow(_viewModel.Filter, this);
            
            if (_filterWindow.ViewModel != null)
            {
                _filterWindow.ViewModel.FilterApplied += () => _viewModel.ApplyFilters();
            }

            // Clean up when window closes
            _filterWindow.Closed += (_, _) =>
            {
                if (_filterWindow?.ViewModel != null)
                {
                    _filterWindow.ViewModel.FilterApplied -= () => _viewModel.ApplyFilters();
                }
                _filterWindow = null;
            };

            _filterWindow.Activate();
        }

        private void PreviousButton_Click(object sender, RoutedEventArgs e)
        {
            _viewModel.PreviousPage();
        }

        private void NextButton_Click(object sender, RoutedEventArgs e)
        {
            _viewModel.NextPage();
        }

        private void TimelineButton_Click(object sender, RoutedEventArgs e)
        {
            _viewModel.OpenTimelineWindow();
        }

        private void TraceTabs_TabCloseRequested(Microsoft.UI.Xaml.Controls.TabView sender, Microsoft.UI.Xaml.Controls.TabViewTabCloseRequestedEventArgs args)
        {
            if (args.Item is Models.TraceData trace)
            {
                _viewModel.CloseTab(trace);
            }
        }
    }
}
