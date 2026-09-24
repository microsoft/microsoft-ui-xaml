using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using CommunityToolkit.Mvvm.ComponentModel;
using Windows.Storage.Pickers;
using XamlTelemtryViewerWInui3.Models;
using XamlTelemtryViewerWInui3.Models.Timeline;
using XamlTelemtryViewerWInui3.Services;

namespace XamlTelemtryViewerWInui3.ViewModels
{
    public class MainViewModel : ObservableObject
    {
        private string _traceFilePath = string.Empty;
        private string _statusMessage = "Select an ETL file to start.";
        private bool _isLoading;
        private float _progressValue;
        private MainWindow? _mainWindow;
        private TimelineWindow? _timelineWindow;
        private CancellationTokenSource? _loadCts;
        private CancellationTokenSource? _filterCts;
        private readonly EtlTraceFileLoader _traceFileLoader = new EtlTraceFileLoader();
        private readonly RegionsLoader _regionsLoader = new();
        private ObservableCollection<TelemetryEvent> _pagedEvents = new();
        private List<TelemetryEvent> _filteredEvents = new();
        private TraceData _traceData = TraceData.Empty;
        private List<string> _selectedPaths = new();
        private List<TraceData> _loadedTraces = new();
        private IReadOnlyList<TelemetryEvent> _allEvents = Array.Empty<TelemetryEvent>();
        private IReadOnlyList<PhaseDefinition>? _regionDefinitions;
        private int _currentPage = 1;
        private int _totalPages = 1;
        private int _pageSize = 200;
        private int _totalCount = 0;
        private int _filteredCount = 0;
        private bool _isFiltering;
        private int _selectedTraceIndex = -1;

        public MainViewModel()
        {
            Filter = new EventFilter();
            ColumnWidths = new ColumnWidths();
        }

        /// <summary>One tab per loaded trace; selecting a tab repoints the event grid.</summary>
        public ObservableCollection<TraceData> Tabs { get; } = new();

        public int SelectedTraceIndex
        {
            get => _selectedTraceIndex;
            set
            {
                if (SetProperty(ref _selectedTraceIndex, value))
                {
                    SwitchActiveTrace(value);
                }
            }
        }

        private void SwitchActiveTrace(int index)
        {
            if (index < 0 || index >= _loadedTraces.Count)
            {
                return;
            }

            _traceData = _loadedTraces[index];
            _allEvents = _traceData.Events;
            TotalCount = _allEvents.Count;
            _ = ApplyFilterAsync(resetPage: true);
        }

        public void CloseTab(TraceData trace)
        {
            var index = _loadedTraces.IndexOf(trace);
            if (index < 0)
            {
                return;
            }

            _loadedTraces.RemoveAt(index);
            Tabs.Remove(trace);

            if (_loadedTraces.Count == 0)
            {
                _traceData = TraceData.Empty;
                _allEvents = Array.Empty<TelemetryEvent>();
                TotalCount = 0;
                _selectedTraceIndex = -1;
                _ = ApplyFilterAsync(resetPage: true);
                StatusMessage = "All traces closed.";
                return;
            }

            var next = Math.Min(index, _loadedTraces.Count - 1);
            SelectedTraceIndex = next;
        }

        public string TraceFilePath
        {
            get => _traceFilePath;
            set => SetProperty(ref _traceFilePath, value);
        }

        public string StatusMessage
        {
            get => _statusMessage;
            set => SetProperty(ref _statusMessage, value);
        }

        public bool IsLoading
        {
            get => _isLoading;
            set => SetProperty(ref _isLoading, value);
        }

        public float ProgressValue
        {
            get => _progressValue;
            set
            {
                // Only notify when the change exceeds a small epsilon to avoid a flood of
                // near-identical progress updates; SetProperty then raises the change.
                if (Math.Abs(_progressValue - value) > 0.001)
                {
                    SetProperty(ref _progressValue, value);
                }
            }
        }

        public IReadOnlyList<TelemetryEvent> PagedEvents
        {
            get => _pagedEvents;
        }

        public int CurrentPage
        {
            get => _currentPage;
            set
            {
                if (SetProperty(ref _currentPage, value))
                {
                    UpdatePagedEvents();
                }
            }
        }

        public int TotalPages
        {
            get => _totalPages;
            set => SetProperty(ref _totalPages, value);
        }

        public int PageSize
        {
            get => _pageSize;
            set
            {
                if (value > 0 && SetProperty(ref _pageSize, value))
                {
                    _ = ApplyFilterAsync(resetPage: true);
                }
            }
        }

        public int TotalCount
        {
            get => _totalCount;
            set => SetProperty(ref _totalCount, value);
        }

        public IReadOnlyList<int> PageSizeOptions => new[] { 100, 200, 500, 1000 };

        public int FilteredCount
        {
            get => _filteredCount;
            set => SetProperty(ref _filteredCount, value);
        }

        public void PreviousPage()
        {
            if (CurrentPage > 1)
            {
                CurrentPage--;
            }
        }

        public void NextPage()
        {
            if (CurrentPage < TotalPages)
            {
                CurrentPage++;
            }
        }

        public EventFilter Filter { get; }

        public ColumnWidths ColumnWidths { get; set; }

        public void SetWindow(MainWindow mainWindow)
        {
            _mainWindow = mainWindow;
        }

        public async void SelectTraceFile()
        {
            try
            {
                var picker = new FileOpenPicker();
                picker.FileTypeFilter.Add(".etl");
                //picker.FileTypeFilter.Add("*");
                picker.SuggestedStartLocation = PickerLocationId.ComputerFolder;

                if (_mainWindow != null)
                {
                    var hwnd = WinRT.Interop.WindowNative.GetWindowHandle(_mainWindow);
                    WinRT.Interop.InitializeWithWindow.Initialize(picker, hwnd);
                }

                var files = await picker.PickMultipleFilesAsync();
                if (files != null && files.Count > 0)
                {
                    _selectedPaths = files.Select(f => f.Path).ToList();
                    TraceFilePath = _selectedPaths.Count == 1
                        ? _selectedPaths[0]
                        : $"{_selectedPaths.Count} files: " + string.Join(", ", _selectedPaths.Select(Path.GetFileName));
                    StatusMessage = $"{_selectedPaths.Count} trace file(s) selected. Click Load trace to process.";
                }
            }
            catch (Exception ex)
            {
                StatusMessage = $"Error selecting trace file: {ex.Message}";
            }
        }

        public async Task LoadTraceAsync()
        {
            IsLoading = true;
            StatusMessage = "Loading trace file(s)...";
            var previousCts = _loadCts;
            _loadCts = new CancellationTokenSource();
            previousCts?.Cancel();
            previousCts?.Dispose();

            // Fall back to a single explicit TraceFilePath (e.g. drag/drop or older flow).
            var paths = _selectedPaths.Count > 0
                ? _selectedPaths
                : (string.IsNullOrWhiteSpace(TraceFilePath) ? new List<string>() : new List<string> { TraceFilePath });

            try
            {
                var loaded = new List<TraceData>(paths.Count);
                for (var i = 0; i < paths.Count; i++)
                {
                    var path = paths[i];
                    StatusMessage = $"Loading trace {i + 1} of {paths.Count}: {Path.GetFileName(path)}...";
                    var data = await _traceFileLoader.LoadAsync(path, _loadCts.Token).ConfigureAwait(true);
                    loaded.Add(data);
                }

                _loadedTraces = loaded;

                // One tab per loaded trace; the selected tab drives the event grid.
                Tabs.Clear();
                foreach (var t in loaded) Tabs.Add(t);

                // The first trace drives the existing paginated event grid / filtering.
                _traceData = loaded.Count > 0 ? loaded[0] : TraceData.Empty;
                _allEvents = _traceData.Events;
                _selectedTraceIndex = loaded.Count > 0 ? 0 : -1;
                OnPropertyChanged(nameof(SelectedTraceIndex));
                TotalCount = _allEvents.Count;
                StatusMessage = $"Loaded {loaded.Count} trace(s); {TotalCount:N0} events in primary. Applying filters...";
                await ApplyFilterAsync(resetPage: true);
                StatusMessage = $"Loaded {loaded.Count} trace(s). Primary: {TotalCount:N0} events, {FilteredCount:N0} filtered.";
            }
            catch (OperationCanceledException)
            {
                StatusMessage = "Trace load cancelled.";
            }
            catch (Exception ex)
            {
                _loadedTraces = new List<TraceData>();
                Tabs.Clear();
                _selectedTraceIndex = -1;
                _traceData = TraceData.Empty;
                _allEvents = Array.Empty<TelemetryEvent>();
                TotalCount = 0;
                TotalPages = 1;
                CurrentPage = 1;
                _pagedEvents = new();
                OnPropertyChanged(nameof(PagedEvents));
                StatusMessage = "Failed to load trace: " + ex.Message;
            }
            finally
            {
                IsLoading = false;
            }
        }

        public void ApplyFilters()
        {
            StatusMessage = $"Filter applied — {Filter.Groups.Count} group(s). Refiltering...";
            _ = ApplyFilterAsync(resetPage: true);
        }

        private async Task ApplyFilterAsync(bool resetPage)
        {
            if (_isFiltering)
            {
                return;
            }

            _isFiltering = true;
            StatusMessage = "Filtering events...";
            _filterCts?.Cancel();
            _filterCts = new CancellationTokenSource();
            var token = _filterCts.Token;

            // Snapshot the filter shape on the UI thread: ObservableCollection enumeration is not
            // thread-safe, so copy Groups and each group's Conditions into plain Lists. We keep
            // references to the original FilterGroup/FilterCondition instances; their property
            // reads (atomic) are safe from the background thread.
            var snapshot = Filter.Groups
                .Select(g => (Group: g, Conditions: (IReadOnlyList<FilterCondition>)g.Conditions.ToList()))
                .ToList();

            // Capture the current event list reference on the UI thread. _allEvents is
            // effectively immutable (only ever reassigned, never mutated in place), so we
            // can hand the reference straight to the background thread without copying the
            // whole list. A later reassignment of _allEvents won't affect this local, and
            // superseded runs are cancelled via _filterCts.
            var sourceEvents = _allEvents;
            var pageSize = PageSize;
            var requestedPage = resetPage ? 1 : CurrentPage;

            try
            {
                var result = await Task.Run(() =>
                {
                    var filtered = new List<TelemetryEvent>(sourceEvents.Count);
                    foreach (var ev in sourceEvents)
                    {
                        token.ThrowIfCancellationRequested();
                        if (MatchesSnapshot(ev, snapshot))
                        {
                            filtered.Add(ev);
                        }
                    }

                    var totalPages = Math.Max(1, (int)Math.Ceiling(filtered.Count / (double)pageSize));
                    var page = Math.Min(Math.Max(requestedPage, 1), totalPages);
                    var pageIndex = Math.Max(page - 1, 0);
                    var pageItems = filtered
                        .Skip(pageIndex * pageSize)
                        .Take(pageSize)
                        .ToList();

                    return (filtered, pageItems, totalPages, page);
                }, token).ConfigureAwait(true);

                _filteredEvents = result.filtered;
                FilteredCount = _filteredEvents.Count;
                TotalPages = result.totalPages;
                CurrentPage = result.page;
                UpdatePagedEvents(result.pageItems);
                StatusMessage = $"Filtered {FilteredCount:N0} of {TotalCount:N0} events.";
            }
            catch (OperationCanceledException)
            {
                StatusMessage = "Filtering cancelled.";
            }
            finally
            {
                _isFiltering = false;
            }
        }

        private static bool MatchesSnapshot(
            TelemetryEvent ev,
            List<(FilterGroup Group, IReadOnlyList<FilterCondition> Conditions)> snapshot)
        {
            var activeGroups = snapshot.Where(s => s.Group.IsEnabled).ToList();
            if (activeGroups.Count == 0)
            {
                return true;
            }

            var result = MatchesGroup(ev, activeGroups[0].Conditions);
            for (var i = 1; i < activeGroups.Count; i++)
            {
                var g = activeGroups[i];
                var match = MatchesGroup(ev, g.Conditions);
                result = g.Group.JoinWithPrevious == FilterJoin.And
                    ? result && match
                    : result || match;
            }

            return result;
        }

        private static bool MatchesGroup(TelemetryEvent ev, IReadOnlyList<FilterCondition> conditions)
        {
            var active = conditions
                .Where(c => c.IsEnabled && !string.IsNullOrWhiteSpace(c.Value))
                .ToList();

            if (active.Count == 0)
            {
                return true;
            }

            var result = active[0].IsMatch(ev);
            for (var i = 1; i < active.Count; i++)
            {
                var c = active[i];
                var match = c.IsMatch(ev);
                result = c.JoinWithPrevious == FilterJoin.And
                    ? result && match
                    : result || match;
            }

            return result;
        }

        private void UpdatePagedEvents()
        {
            if (_filteredEvents.Count == 0)
            {
                UpdatePagedEvents(new List<TelemetryEvent>());
                return;
            }

            var pageIndex = Math.Max(CurrentPage - 1, 0);
            var pageItems = _filteredEvents
                .Skip(pageIndex * PageSize)
                .Take(PageSize)
                .ToList();

            UpdatePagedEvents(pageItems);
        }

        private void UpdatePagedEvents(IReadOnlyList<TelemetryEvent> pageItems)
        {
            _pagedEvents.Clear();
            foreach (var item in pageItems)
            {
                _pagedEvents.Add(item);
            }
            OnPropertyChanged(nameof(PagedEvents));
        }

        public void OpenTimelineWindow()
        {
            if (_loadedTraces.Count == 0 || _traceData.Events.Count == 0)
            {
                StatusMessage = "Load a trace before opening the timeline.";
                return;
            }

            IReadOnlyList<PhaseDefinition> definitions;
            try
            {
                definitions = _regionDefinitions ??= LoadBundledRegions();
            }
            catch (Exception ex)
            {
                StatusMessage = "Failed to load regions file: " + ex.Message;
                return;
            }

            var vm = new TimelineViewModel(_loadedTraces, definitions);
            _timelineWindow = new TimelineWindow(vm, _traceData, definitions, _mainWindow);
            _timelineWindow.Closed += (_, _) => { _timelineWindow = null; };
            _timelineWindow.Activate();
        }

        public TimelineWindow? GetTimelineWindow()
        {
            return _timelineWindow;
        }

        private IReadOnlyList<PhaseDefinition> LoadBundledRegions()
        {
            // Region definitions are the single canonical copy embedded in the shared
            // XamlTimeline assembly (same XML the WPA plugin uses).
            return _regionsLoader.LoadEmbeddedDefault();
        }
    }
}


