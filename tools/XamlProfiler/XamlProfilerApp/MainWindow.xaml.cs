using System.Collections.Specialized;
using System.Diagnostics;
using Microsoft.UI.Input;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media;
using Windows.Management.Deployment;
using Windows.Storage.Pickers;
using Windows.System;
using Windows.UI.Core;
using XamlProfiler.Models;
using XamlProfiler.Services;
using XamlProfiler.Helpers;

namespace XamlProfiler;

public sealed partial class MainWindow : Window
{
    public ProfilerTreeStore Store { get; } = new();

    private EtwListenerService? _listener;
    private Process? _targetProcess;
    private Func<Process, bool>? _targetMatch;
    private TapChannel? _tap;
    private readonly List<PackageInfo> _packages = new();

    // Auto-follow state for the Composition Tree: keep the view pinned to the
    // bottom so the most recently added comp nodes are always visible.
    private ScrollViewer? _compScrollViewer;
    private double _compLastExtentHeight;

    // Per-tree "sticky tail" follow state. Auto-scroll (the Composition auto-follow and the
    // passive "spotlight new nodes" reveal) only moves a tree while its ScrollViewer sits at the
    // bottom-left tail. The instant the user scrolls anywhere, following turns off so auto-scroll
    // stops fighting their manual scroll; it resumes once they return to the tail. Without this,
    // streaming nodes constantly yank every tree (vertical "lock" + one tree scrolling another).
    private readonly Dictionary<TreeView, ScrollViewer> _treeScrollViewers = new();
    private readonly Dictionary<TreeView, bool> _treeFollowing = new();

    // --- "Spotlight new nodes" feature ---
    // Newly-created nodes are buffered and debounced; the first settled batch after a
    // connection is the initial tree flood and is silently consumed (baseline), so only
    // SUBSEQUENT structural changes glow + auto-scroll.
    private readonly List<TreeNode> _newNodeBuffer = new();
    private DispatcherTimer? _spotlightDebounceTimer;   // fires once node creation goes quiet
    private DispatcherTimer? _spotlightGlowTimer;        // clears the glow after a few seconds
    private bool _spotlightArmed;                        // false until the initial flood is consumed
    // Batches larger than this are treated as a flood/navigation and skipped (still arm).
    private const int SpotlightMaxBatch = 50;

    public MainWindow()
     {
        this.InitializeComponent();
        this.Title = "XAML Profiler — Tree Visualizer";
        ExePathTextBox.Text = DefaultExePath;
        LoadPackages();

        // When a deferred app→profiler pick highlight finally lands (the picked IVisual arrived
        // over ETW a frame after the pick), select/scroll to it so the user sees the result.
        Store.VisualSubtreeHighlightApplied += node =>
            DispatcherQueue.TryEnqueue(() =>
            {
                SelectNodeInTree(node);
                StatusText.Text = $"Picked visual subtree: [{node.Kind}] {node.DisplayName}";
            });

        // When a deferred app→profiler ELEMENT pick finally resolves (the element's PeerHandle
        // arrived over ETW a frame after the pick force-created its DXaml peer), apply the pick
        // exactly as an immediate hit would — so the user never has to pick a second time.
        Store.PendingPickResolved += node =>
            DispatcherQueue.TryEnqueue(() => ApplyElementPick(node));

        // After any highlight op (Ctrl+Click linkage, element pick, or IVisual subtree pick),
        // the store hands us the per-tree anchor nodes to reveal. Expand+scroll EVERY tree to
        // its anchor — not just the source tree — so all four trees unfold and auto-scroll to
        // the related node, even when it is deep inside a virtualized subtree.
        Store.RevealRequested += (anchors, passive) =>
            DispatcherQueue.TryEnqueue(() => RevealAnchors(anchors, passive));

        // Spotlight freshly-added tree nodes: buffer creations, debounce, and once node
        // creation settles, glow + auto-scroll to the new nodes (skipping the initial flood).
        _spotlightDebounceTimer = new DispatcherTimer { Interval = TimeSpan.FromMilliseconds(500) };
        _spotlightDebounceTimer.Tick += SpotlightDebounceTick;
        _spotlightGlowTimer = new DispatcherTimer { Interval = TimeSpan.FromSeconds(3) };
        _spotlightGlowTimer.Tick += SpotlightGlowTick;
        Store.NodeAdded += OnStoreNodeAdded;
    }

    // A node was just created in the store. Buffer it (on the UI thread) and (re)start the
    // debounce so we only act once a burst of creations has gone quiet.
    private void OnStoreNodeAdded(TreeNode node)
    {
        DispatcherQueue.TryEnqueue(() =>
        {
            if (SpotlightNewNodesCheck?.IsChecked != true) return;
            _newNodeBuffer.Add(node);
            _spotlightDebounceTimer?.Stop();
            _spotlightDebounceTimer?.Start();
        });
    }

    private void SpotlightDebounceTick(object? sender, object e)
    {
        _spotlightDebounceTimer?.Stop();

        var batch = _newNodeBuffer.ToList();
        _newNodeBuffer.Clear();
        if (batch.Count == 0) return;

        // The first settled batch after a (re)connect is the initial tree flood. Establish
        // the baseline silently so we don't spotlight the entire tree on connect.
        if (!_spotlightArmed)
        {
            _spotlightArmed = true;
            return;
        }

        // Huge batches are floods/navigations — glowing dozens of nodes is noise, not signal.
        if (batch.Count > SpotlightMaxBatch)
        {
            StatusText.Text = $"✨ {batch.Count} nodes added (too many to spotlight)";
            return;
        }

        Store.SpotlightNewNodes(batch);
        _spotlightGlowTimer?.Stop();
        _spotlightGlowTimer?.Start();

        int n = batch.Count;
        StatusText.Text = $"✨ {n} new node{(n == 1 ? "" : "s")} added";
    }

    private void SpotlightGlowTick(object? sender, object e)
    {
        _spotlightGlowTimer?.Stop();
        Store.ClearSpotlight();
    }

    // Toggle whether tree node names include the raw 0x memory address.
    private void IncludeMemoryAddressCheck_Click(object sender, RoutedEventArgs e)
    {
        TreeNode.IncludeMemoryAddress = IncludeMemoryAddressCheck?.IsChecked == true;
        Store.RefreshAllDisplayNames();
    }

    // Toggle whether composition nodes show their responsible UIElement's address.
    private void ResponsibleElementAddressCheck_Click(object sender, RoutedEventArgs e)
    {
        TreeNode.ShowResponsibleElementAddress = ResponsibleElementAddressCheck?.IsChecked == true;
        Store.RefreshAllDisplayNames();
    }

    // Toggling the checkbox off clears any pending/active spotlight immediately.
    private void SpotlightNewNodesCheck_Click(object sender, RoutedEventArgs e)
    {
        if (SpotlightNewNodesCheck?.IsChecked == true) return;
        _spotlightDebounceTimer?.Stop();
        _spotlightGlowTimer?.Stop();
        _newNodeBuffer.Clear();
        Store.ClearSpotlight();
    }

    // Reset the spotlight baseline so the next connection's initial tree flood is skipped
    // again rather than glowing wholesale. Call on Clear Trees / new session start.
    private void ResetSpotlightArming()
    {
        _spotlightArmed = false;
        _spotlightDebounceTimer?.Stop();
        _spotlightGlowTimer?.Stop();
        _newNodeBuffer.Clear();
        Store.ClearSpotlight();
    }

    private void LoadPackages()
    {
        try
        {
            var pm = new PackageManager();
            var packages = pm.FindPackagesForUser(string.Empty);
            _packages.Clear();

            foreach (var pkg in packages)
            {
                try
                {
                    // Only show packages that have an app entry (not frameworks/resources)
                    if (pkg.IsFramework || pkg.IsResourcePackage) continue;

                    var appListEntries = pkg.GetAppListEntries();
                    if (appListEntries.Count > 0)
                    {
                        _packages.Add(new PackageInfo
                        {
                            DisplayName = pkg.DisplayName,
                            PackageFamilyName = pkg.Id.FamilyName,
                            AppUserModelId = appListEntries[0].AppUserModelId
                        });
                    }
                }
                catch
                {
                    // Skip packages that can't be enumerated
                }
            }

            _packages.Sort((a, b) => string.Compare(a.DisplayName, b.DisplayName, StringComparison.OrdinalIgnoreCase));

            PackageComboBox.Items.Clear();
            foreach (var p in _packages)
            {
                PackageComboBox.Items.Add($"{p.DisplayName}  ({p.PackageFamilyName})");
            }
        }
        catch (Exception ex)
        {
            StatusText.Text = $"Could not enumerate packages: {ex.Message}";
        }
    }

    private void LaunchModeChanged(object sender, RoutedEventArgs e)
    {
        if (ExePickerPanel == null || PackageComboBox == null) return;

        if (ExeRadioButton.IsChecked == true)
        {
            ExePickerPanel.Visibility = Visibility.Visible;
            PackageComboBox.Visibility = Visibility.Collapsed;
        }
        else
        {
            ExePickerPanel.Visibility = Visibility.Collapsed;
            PackageComboBox.Visibility = Visibility.Visible;
        }
    }

    private const string DefaultExePath = @"C:\Users\t-rdiggavi\source\repos\microsoft-ui-xaml-lift\BuildOutput\obj\amd64chk\Samples\FolderTreeApp\FolderTreeApp\AppX\FolderTreeApp.exe";

    private void DefaultButton_Click(object sender, RoutedEventArgs e)
    {
        ExePathTextBox.Text = DefaultExePath;
    }

    private async void BrowseButton_Click(object sender, RoutedEventArgs e)
    {
        var picker = new FileOpenPicker();
        picker.FileTypeFilter.Add(".exe");
        picker.SuggestedStartLocation = PickerLocationId.Desktop;

        // Initialize picker with the window handle
        var hwnd = WinRT.Interop.WindowNative.GetWindowHandle(this);
        WinRT.Interop.InitializeWithWindow.Initialize(picker, hwnd);

        var file = await picker.PickSingleFileAsync();
        if (file != null)
        {
            ExePathTextBox.Text = file.Path;
        }
    }

    private async void StartButton_Click(object sender, RoutedEventArgs e)
    {
        // Stop any existing session
        StopSession();
        _compLastExtentHeight = 0;

        var mode = ExeRadioButton.IsChecked == true ? "exe" : "package";

        // Validate inputs BEFORE we start listening, so we never leave a session
        // running when the launch can't proceed.
        string exePath = "";
        PackageInfo? pkgInfo = null;
        if (mode == "exe")
        {
            exePath = ExePathTextBox.Text.Trim();
            if (string.IsNullOrEmpty(exePath) || !System.IO.File.Exists(exePath))
            {
                StatusText.Text = "Please select a valid .exe file.";
                return;
            }
        }
        else
        {
            if (PackageComboBox.SelectedIndex < 0)
            {
                StatusText.Text = "Please select an app package.";
                return;
            }
            pkgInfo = _packages[PackageComboBox.SelectedIndex];
        }

        // Start the ETW listener BEFORE launching the target. We enable the provider in
        // all-process mode (pid 0) first so it is already live when the target executes its
        // very first instruction, then narrow the filter to the target PID the moment we
        // know it. This closes the boot-capture race: previously the app was launched first
        // and the session came up afterwards, so tree events emitted during startup (which
        // are only emitted while a session has the provider enabled) were lost.
        _listener?.Dispose();
        // Reset the spotlight baseline so this session's initial tree flood is consumed
        // silently rather than glowing every node on connect.
        ResetSpotlightArming();
        _listener = new EtwListenerService(Store, this.DispatcherQueue, 0);

        _listener.StatusChanged += msg =>
            DispatcherQueue.TryEnqueue(() => StatusText.Text = msg);

        _listener.EventCountChanged += count =>
            DispatcherQueue.TryEnqueue(() => EventCountText.Text = $"Events: {count}");

        _listener.Start();

        // Now launch the target — the listener is already capturing.
        int targetPid;
        if (mode == "exe")
        {
            try
            {
                var psi = new ProcessStartInfo(exePath)
                {
                    UseShellExecute = true,
                    WorkingDirectory = System.IO.Path.GetDirectoryName(exePath) ?? ""
                };
                _targetProcess = Process.Start(psi);
                if (_targetProcess == null)
                {
                    StatusText.Text = "Failed to start process.";
                    StopSession();
                    return;
                }
                targetPid = _targetProcess.Id;
                // Remember how to find the live-window process at Tap time (no startup latency).
                string exeName = System.IO.Path.GetFileNameWithoutExtension(exePath);
                _targetMatch = p => string.Equals(p.ProcessName, exeName, StringComparison.OrdinalIgnoreCase);
                ProcessInfoText.Text = $"PID: {targetPid} | {System.IO.Path.GetFileName(exePath)}";
            }
            catch (Exception ex)
            {
                StatusText.Text = $"Launch failed: {ex.Message}";
                StopSession();
                return;
            }
        }
        else // package
        {
            try
            {
                // Launch the packaged app via shell and find its process
                var psi = new ProcessStartInfo
                {
                    FileName = "explorer.exe",
                    Arguments = $"shell:AppsFolder\\{pkgInfo!.AppUserModelId}",
                    UseShellExecute = false
                };
                Process.Start(psi);

                // Wait briefly for the app to start, then find its process. The listener is
                // already running (all-process mode), so boot events fired during this window
                // are captured, not missed.
                await Task.Delay(2000);

                _targetProcess = ProcessResolver.FindProcessByPackageFamily(pkgInfo.PackageFamilyName);
                // Remember how to find the live-window process at Tap time (no startup latency).
                _targetMatch = p => ProcessResolver.ProcessMatchesPackageFamily(p, pkgInfo.PackageFamilyName);
                if (_targetProcess == null)
                {
                    StatusText.Text = $"App launched but could not find its process. Listening to all processes.";
                    targetPid = 0;
                }
                else
                {
                    targetPid = _targetProcess.Id;
                }
                ProcessInfoText.Text = targetPid > 0
                    ? $"PID: {targetPid} | {pkgInfo.DisplayName}"
                    : $"{pkgInfo.DisplayName} (all PIDs)";
            }
            catch (Exception ex)
            {
                StatusText.Text = $"Package launch failed: {ex.Message}";
                StopSession();
                return;
            }
        }

        // Narrow the listener to the target process now that we know its PID. Events already
        // captured from the target during the all-process window are kept; only future events
        // from other processes are filtered out. (0 = keep listening to all processes.)
        if (targetPid != 0)
        {
            _listener.SetTargetProcess(targetPid);
        }

        // The live-highlight tap is now injected on demand via the Tap button, so the
        // user can trigger it once the observed app's window is fully up — which is when
        // its XAML Diagnostics port ("WinUIVisualDiagConnection1") is registered. This
        // avoids InitializeXamlDiagnosticsEx failing with ERROR_NOT_FOUND (0x80070490)
        // when injecting too early or into a transient launcher pid. Enabled only when we
        // resolved a single target process ("all PIDs" package mode has nothing to inject).
        TapButton.IsEnabled = _targetProcess != null;

        StartButton.IsEnabled = false;
        StopButton.IsEnabled = true;
    }

    private void StopButton_Click(object sender, RoutedEventArgs e)
    {
        StopSession();
        StartButton.IsEnabled = true;
        StopButton.IsEnabled = false;
        TapButton.IsEnabled = false;
        TapButton.Content = "Tap";
        _targetMatch = null;
    }

    /// <summary>
    /// Injects the live-highlight tap into the observed app on demand. Triggered manually
    /// (like WinUISnoop's connect gesture) so it runs only once the target's window is up
    /// and its XAML Diagnostics port is registered — the reliable moment to call
    /// InitializeXamlDiagnosticsEx.
    /// </summary>
    private async void TapButton_Click(object sender, RoutedEventArgs e)
    {
        if (_targetProcess == null)
        {
            StatusText.Text = "No target process to tap into. Click 'Launch and Listen' first.";
            return;
        }

        try
        {
            // Resolve the live-window process now (at click time the window is already up,
            // so this returns immediately and adds no startup latency). For packaged apps
            // the launch pid is often a helper; targeting the window owner is what makes
            // InitializeXamlDiagnosticsEx find the diagnostics port instead of failing with
            // ERROR_NOT_FOUND (0x80070490).
            if (_targetMatch != null)
            {
                TapButton.IsEnabled = false;
                StatusText.Text = "Resolving app window...";
                var live = await ProcessResolver.ResolveLiveWindowProcessAsync(_targetMatch, _targetProcess);
                if (live != null)
                {
                    _targetProcess = live;
                }
            }

            _targetProcess.Refresh();
            if (_targetProcess.HasExited)
            {
                StatusText.Text = "Target process has exited. Relaunch and try again.";
                TapButton.IsEnabled = false;
                return;
            }

            // Re-tap cleanly if a previous tap is lingering.
            if (_tap != null)
            {
                try { _tap.Stop(); } catch { }
                _tap = null;
            }

            _tap = new TapChannel(_targetProcess);
            _tap.Error += msg =>
                DispatcherQueue.TryEnqueue(() =>
                {
                    StatusText.Text = $"Live-highlight tap error: {msg}";
                    TapButton.Content = "Tap";
                    TapButton.IsEnabled = true;
                    PickToggle.IsChecked = false;
                    PickToggle.IsEnabled = false;
                });
            _tap.Log += msg =>
                DispatcherQueue.TryEnqueue(() => StatusText.Text = $"Tap: {msg}");
            _tap.Connected += () =>
                DispatcherQueue.TryEnqueue(() =>
                {
                    StatusText.Text = "Live-highlight tap connected.";
                    TapButton.Content = "Tapped";
                    PickToggle.IsEnabled = true;
                });
            _tap.ElementPicked += handle =>
                DispatcherQueue.TryEnqueue(() => OnElementPicked(handle));
            _tap.VisualPicked += visualId =>
                DispatcherQueue.TryEnqueue(() => OnVisualPicked(visualId));

            TapButton.IsEnabled = false;
            StatusText.Text = $"Tapping into PID {_targetProcess.Id}...";
            _tap.Start();
        }
        catch (Exception ex)
        {
            _tap = null;
            TapButton.IsEnabled = true;
            StatusText.Text = $"Live-highlight unavailable: {ex.Message}";
        }
    }

    private void StopSession()
    {
        if (_tap != null)
        {
            try { _tap.Stop(); } catch { }
            _tap = null;
        }
        PickToggle.IsChecked = false;
        PickToggle.IsEnabled = false;
        _listener?.Stop();
        _listener?.Dispose();
        _listener = null;
    }

    private void ClearButton_Click(object sender, RoutedEventArgs e)
    {
        Store.ClearLinkHighlights();
        ResetSpotlightArming();
        Store.LogicalRoots.Clear();
        Store.VisualRoots.Clear();
        Store.CompositionRoots.Clear();
        Store.WucVisualRoots.Clear();
        Store.EventCount = 0;
        _compLastExtentHeight = 0;
        EventCountText.Text = "Events: 0";
        StatusText.Text = "Trees cleared.";
    }

    /// <summary>
    /// Plain click on a row toggles expansion (so the user can drill into the
    /// tree without ever touching the chevron). Ctrl+click is a separate
    /// "probe" gesture that highlights the linked peer in the other two trees
    /// and dim-glows every ancestor on the path to it — without changing the
    /// expansion state.
    /// <para>
    /// Either gesture populates the detail pane with the clicked node's event
    /// history, since inspecting a row's provenance is useful whether or not
    /// <summary>
    /// Click gestures on tree rows:
    /// <list type="bullet">
    ///   <item><b>Click</b> — expand/collapse + show detail pane</item>
    ///   <item><b>Shift+Click</b> — show detail pane only (no expand/collapse)</item>
    ///   <item><b>Ctrl+Click</b> — cross-tree linkage highlight</item>
    /// </list>
    /// </summary>
    // ---- Clearing highlights (cross-tree glow + live app adorner) ---------------

    /// <summary>
    /// Remove every active highlight: the in-profiler cross-tree glow (Peer/Path) AND the live
    /// adorner the tap draws in the target app. Invoked by the "Clear Highlights" button and the
    /// Esc accelerator. Distinct from <see cref="ClearButton_Click"/>, which wipes the trees.
    /// </summary>
    private void ClearAllHighlights()
    {
        Store.ClearLinkHighlights();   // profiler glow + any armed pending pick
        _tap?.ClearHighlight();        // live box / in-place adorner in the target app
        StatusText.Text = "Highlights cleared.";
    }

    private void ClearHighlightsButton_Click(object sender, RoutedEventArgs e) => ClearAllHighlights();

    private void ClearHighlightsAccelerator_Invoked(
        Microsoft.UI.Xaml.Input.KeyboardAccelerator sender,
        Microsoft.UI.Xaml.Input.KeyboardAcceleratorInvokedEventArgs args)
    {
        ClearAllHighlights();
        args.Handled = true;
    }

    private void OnTreeItemInvoked(TreeView sender, TreeViewItemInvokedEventArgs args)
    {
        if (args.InvokedItem is not TreeNode node) return;

        var ctrlState = InputKeyboardSource.GetKeyStateForCurrentThread(VirtualKey.Control);
        var shiftState = InputKeyboardSource.GetKeyStateForCurrentThread(VirtualKey.Shift);
        bool ctrlDown = (ctrlState & CoreVirtualKeyStates.Down) != 0;
        bool shiftDown = (shiftState & CoreVirtualKeyStates.Down) != 0;

        if (ctrlDown)
        {
            // Ctrl+Click: cross-tree linkage highlight (in-profiler glow)...
            Store.HighlightLinkedFor(node);

            // ...and live cross-app highlight: tell the injected tap to overlay the
            // matching live element in the target app. Composition/WUC nodes that
            // never resolved a DXaml peer carry PeerHandle == 0 and are skipped.
            Debug.WriteLine($"[Profiler] Ctrl+Click '{node.DisplayName}' kind={node.Kind} PeerHandle=0x{node.PeerHandle:X16} tapConnected={_tap?.IsConnected == true}");
            if (_tap != null)
            {
                if (node.Kind == TreeNodeKind.WucVisual)
                {
                    // WUC IVisual node: node.Id == the live IVisual* the producer stamped
                    // into Visual.Comment ("xpid:<hex>"). The tap walks the live composition
                    // tree to find and adorn that exact visual in place.
                    _tap.HighlightVisual(node.Id);
                }
                else if (node.PeerHandle != 0)
                {
                    _tap.Highlight(node.PeerHandle);
                }
                else
                {
                    _tap.ClearHighlight();
                }
            }
        }
        else if (shiftDown)
        {
            // Shift+Click: detail pane only, no expand/collapse
            UpdateDetailPane(node);
            _tap?.ClearHighlight();
        }
        else
        {
            // Plain click: expand/collapse + detail pane
            node.IsExpanded = !node.IsExpanded;
            UpdateDetailPane(node);
            if (_tap != null && _tap.IsConnected)
                _tap.ClearHighlight();
        }
    }

    // Tracks the per-container IsExpanded change-callback token so we register exactly once
    // per realized TreeViewItem (containers are reused as the TreeView virtualizes). -1 = none.
    private static readonly DependencyProperty ExpandedCallbackTokenProperty =
        DependencyProperty.RegisterAttached(
            "ExpandedCallbackToken", typeof(long), typeof(MainWindow), new PropertyMetadata(-1L));

    // Holds the Children.CollectionChanged handler (and the node it is subscribed to) that
    // re-applies expansion directly onto the container when children arrive after it was
    // realized empty. Stored per-container so we can detach precisely on recycle/unload.
    private static readonly DependencyProperty ChildrenHandlerProperty =
        DependencyProperty.RegisterAttached(
            "ChildrenHandler", typeof(object), typeof(MainWindow), new PropertyMetadata(null));
    private static readonly DependencyProperty ChildrenHandlerNodeProperty =
        DependencyProperty.RegisterAttached(
            "ChildrenHandlerNode", typeof(object), typeof(MainWindow), new PropertyMetadata(null));

    // Re-assert the model's expansion state onto the container once it (and its child items)
    // are realized. WinUI's TreeViewItem can ignore IsExpanded=true applied at bind time
    // (before its children are attached), which left nodes looking collapsed on launch despite
    // the model defaulting to expanded. Doing it on Loaded guarantees every node is expanded by
    // default and survives container virtualization/recycling.
    //
    // The IsExpanded binding is OneWay (model -> view). A TwoWay binding would write the
    // container's transient IsExpanded=false (reset during virtualization/recycle) back into the
    // model, whose collapse-cascade then nuked the whole subtree -- that was collapsing the
    // Logical/Visual trees the moment the Composition/WUC trees populated and forced a relayout.
    // Instead we sync USER toggles view -> model via a guarded callback below.
    private void TreeItem_Loaded(object sender, RoutedEventArgs e)
    {
        if (sender is not Microsoft.UI.Xaml.Controls.TreeViewItem tvi) return;
        if (tvi.DataContext is not Models.TreeNode node) return;

        tvi.IsExpanded = node.IsExpanded;

        if ((long)tvi.GetValue(ExpandedCallbackTokenProperty) < 0)
        {
            long token = tvi.RegisterPropertyChangedCallback(
                Microsoft.UI.Xaml.Controls.TreeViewItem.IsExpandedProperty,
                OnContainerIsExpandedChanged);
            tvi.SetValue(ExpandedCallbackTokenProperty, token);
        }

        // Directly re-apply expansion when this node's children arrive AFTER the container was
        // realized empty. The IsExpanded binding is OneWay (model -> view) and x:Bind does NOT
        // re-push a same-value (true) when PropertyChanged fires, so a node added empty (e.g. the
        // logical/visual tree roots, realized before AddChild/ElementEnteredTree stream their
        // children in) would render collapsed forever. We can't rely on the binding here, so we
        // drive the container's IsExpanded property directly. Detach any stale subscription first
        // (containers virtualize/recycle across nodes), then subscribe to THIS node's children.
        DetachChildrenHandler(tvi);
        NotifyCollectionChangedEventHandler handler = (s, args) =>
        {
            if (args.Action == NotifyCollectionChangedAction.Add &&
                tvi.DataContext is Models.TreeNode current &&
                current.IsExpanded)
            {
                tvi.IsExpanded = true;
            }
        };
        node.Children.CollectionChanged += handler;
        tvi.SetValue(ChildrenHandlerProperty, handler);
        tvi.SetValue(ChildrenHandlerNodeProperty, node);
        tvi.Unloaded -= TreeItem_Unloaded;
        tvi.Unloaded += TreeItem_Unloaded;
    }

    private void TreeItem_Unloaded(object sender, RoutedEventArgs e)
    {
        if (sender is Microsoft.UI.Xaml.Controls.TreeViewItem tvi)
            DetachChildrenHandler(tvi);
    }

    // Unsubscribe the container's Children.CollectionChanged handler from whichever node it was
    // last bound to, so a recycled container never keeps expanding on a stale node's changes.
    private static void DetachChildrenHandler(Microsoft.UI.Xaml.Controls.TreeViewItem tvi)
    {
        if (tvi.GetValue(ChildrenHandlerProperty) is NotifyCollectionChangedEventHandler h &&
            tvi.GetValue(ChildrenHandlerNodeProperty) is Models.TreeNode n)
        {
            n.Children.CollectionChanged -= h;
        }
        tvi.ClearValue(ChildrenHandlerProperty);
        tvi.ClearValue(ChildrenHandlerNodeProperty);
    }

    // Sync a container's IsExpanded back to its model, but ONLY for genuine user toggles: while
    // the container is loaded. Virtualization/recycle resets IsExpanded while the container is
    // detached (IsLoaded == false), and those must not touch the model.
    private void OnContainerIsExpandedChanged(DependencyObject d, DependencyProperty dp)
    {
        if (d is Microsoft.UI.Xaml.Controls.TreeViewItem tvi &&
            tvi.IsLoaded &&
            tvi.DataContext is Models.TreeNode node)
        {
            node.IsExpanded = tvi.IsExpanded;
        }
    }

    // ---- Pick mode (app -> profiler): hover the target app, click to select ----

    /// <summary>Toggle "Pick from app" mode on/off.</summary>
    private void PickToggle_Click(object sender, RoutedEventArgs e)
    {
        if (_tap == null || !_tap.IsConnected)
        {
            PickToggle.IsChecked = false;
            return;
        }

        if (PickToggle.IsChecked == true)
        {
            _tap.StartPick();
            StatusText.Text = "Pick mode: hover the target app, click an element to select it here.";
        }
        else
        {
            _tap.StopPick();
            StatusText.Text = "Pick mode off.";
        }
    }

    /// <summary>
    /// The tap reported an element the user clicked in the target app. Map its handle
    /// to a tree node, glow + reveal it (and its cross-tree peers), and leave pick mode.
    /// </summary>
    private void OnElementPicked(ulong handle)
    {
        // The tap auto-exits pick mode on click; reflect that in the toggle.
        PickToggle.IsChecked = false;

        // Clear prior highlights once up front. The companion OnVisualPicked (raised from
        // the same click) glows the visual subtree additively, so both can coexist.
        Store.ClearLinkHighlights();

        var node = Store.FindByPeerHandle(handle);
        if (node == null)
        {
            // First-pick race: the tap force-created the element's DXaml peer when it picked,
            // but the ETW event that stamps that handle onto the node lands a frame later. Arm a
            // deferred pick so it resolves the instant the handle arrives (see SetPeerHandle /
            // PendingPickResolved) instead of forcing the user to pick a second time.
            Store.ArmPendingPick(handle);
            StatusText.Text = $"Picked element 0x{handle:X}: resolving…";
            return;
        }

        ApplyElementPick(node);
    }

    /// <summary>Glow, expand, reveal and describe a picked element node. Shared by the immediate
    /// pick path and the deferred <see cref="ProfilerTreeStore.PendingPickResolved"/> path.</summary>
    private void ApplyElementPick(TreeNode node)
    {
        Store.HighlightAndExpandFor(node);
        UpdateDetailPane(node);
        SelectNodeInTree(node);
        StatusText.Text = $"Picked: [{node.Kind}] {node.DisplayName}";
    }

    /// <summary>
    /// The tap reported the composition visual of the clicked element. Find that visual
    /// node in the IVisual/Composition tree and glow its whole subtree.
    /// </summary>
    private void OnVisualPicked(ulong visualId)
    {
        var node = Store.HighlightSubtreeForVisual(visualId);
        if (node == null)
        {
            // The visual isn't in the store yet (its hand-in visual was just created by the
            // pick's GetElementVisual call and the producer emits it next frame). The store has
            // armed a pending highlight; VisualSubtreeHighlightApplied will fire when it lands.
            StatusText.Text = $"Picked visual 0x{visualId:X}: waiting for it to appear in the IVisual tree…";
            return;
        }

        SelectNodeInTree(node);
        StatusText.Text = $"Picked visual subtree: [{node.Kind}] {node.DisplayName}";
    }

    // Select the node in the TreeView matching its kind so it scrolls into view.
    private void SelectNodeInTree(TreeNode node)
    {
        var tree = TreeForKind(node.Kind);
        if (tree == null) return;
        // Defer so ancestor TreeViewItems realized by the IsExpanded changes exist
        // before we set the selection (which scrolls the item into view).
        DispatcherQueue.TryEnqueue(
            Microsoft.UI.Dispatching.DispatcherQueuePriority.Low,
            () =>
            {
                try { tree.SelectedItem = node; } catch { }
                RevealNodeInTree(tree, node);
            });
    }

    private TreeView? TreeForKind(TreeNodeKind kind) => kind switch
    {
        TreeNodeKind.Logical => LogicalTreeView,
        TreeNodeKind.Visual => VisualTreeView,
        TreeNodeKind.Composition => CompositionTreeView,
        TreeNodeKind.WucVisual => WucTreeView,
        _ => null
    };

    // Expand+scroll every tree to its anchor node. The store already set IsExpanded along each
    // path; here we force the (possibly virtualized) container to realize and bring it on-screen.
    private void RevealAnchors(IReadOnlyList<TreeNode> anchors, bool passive)
    {
        // Keep only the deepest/most-specific anchor per tree kind (last write wins).
        var byKind = new Dictionary<TreeNodeKind, TreeNode>();
        foreach (var a in anchors) byKind[a.Kind] = a;

        foreach (var kv in byKind)
        {
            var tree = TreeForKind(kv.Key);
            if (tree == null) continue;
            // Late-realized ScrollViewers: attach the tail tracker now if we couldn't at Loaded.
            AttachScrollTracker(tree);
            // A passive (streaming "spotlight") reveal must not yank a tree the user has scrolled
            // away from — that is what made scrolling feel locked and one tree scroll another.
            // An explicit pick / Ctrl+Click reveal (passive == false) always scrolls.
            if (passive && !IsTreeFollowing(tree)) continue;
            RevealNodeInTree(tree, kv.Value);
        }
    }

    // Bring a node's container into view. The four trees are built from NESTED TreeViewItem
    // ItemsControls (each TreeViewItem in the ItemTemplate hosts its own Children), NOT the
    // TreeView's flat node list. That means tree.ContainerFromItem() and the TreeView's inner
    // ListView only know about ROOT items — a deep node's container lives inside its parent's
    // nested TreeViewItem and does not exist until every ancestor has been realized and brought
    // on-screen. So we can't scroll straight to a deep node; we must walk the ancestor chain
    // root→node, scrolling (and expanding) each level in turn, deferring a layout pass between
    // levels so the next level's containers virtualize in. This reaches any depth in one gesture.
    private void RevealNodeInTree(TreeView tree, TreeNode node)
    {
        var chain = Store.GetAncestorChain(node);
        RevealChainStep(tree, chain, 0);
    }

    private void RevealChainStep(TreeView tree, IReadOnlyList<TreeNode> chain, int index, int attempt = 0)
    {
        const int maxAttempts = 12;
        if (index >= chain.Count) return;

        bool isLast = index == chain.Count - 1;
        var target = chain[index];

        var container = VisualTreeUtils.FindContainerByDataContext(tree, target);
        if (container != null)
        {
            // Make sure ancestors are expanded so the NEXT level realizes its children.
            if (!isLast && container is TreeViewItem tvi)
                tvi.IsExpanded = true;

            container.StartBringIntoView(new BringIntoViewOptions { AnimationDesired = isLast });

            if (!isLast)
            {
                // Let layout run (realize+expand) before descending to the next level.
                DispatcherQueue.TryEnqueue(
                    Microsoft.UI.Dispatching.DispatcherQueuePriority.Low,
                    () => RevealChainStep(tree, chain, index + 1));
            }
            return;
        }

        // This level's container isn't realized yet (its parent is still laying out).
        // Retry the SAME level a few times before giving up.
        if (attempt < maxAttempts)
        {
            DispatcherQueue.TryEnqueue(
                Microsoft.UI.Dispatching.DispatcherQueuePriority.Low,
                () => RevealChainStep(tree, chain, index, attempt + 1));
        }
    }

    /// <summary>
    /// Repoints the detail pane at the given node — header text, creation-event
    /// badge, and event history list. The history collection is bound directly
    /// (the node owns it) so subsequent ETW events automatically stream into
    /// the list while the user is looking at it.
    /// </summary>
    private void UpdateDetailPane(TreeNode node)
    {
        DetailTitleText.Text = $"[{node.Kind}]  {node.DisplayName}";

        if (string.IsNullOrEmpty(node.CreatedByEvent))
        {
            DetailCreatedBadge.Visibility = Visibility.Collapsed;
        }
        else
        {
            DetailCreatedBadge.Visibility = Visibility.Visible;
            DetailCreatedBadge.Background = EventBadgeHelper.GetBrush(node.CreatedByEvent);
            DetailCreatedText.Text = $"created by {node.CreatedByEvent}";
        }

        DetailEventCountText.Text = $"{node.EventHistory.Count} event(s) recorded";
        DetailHistoryList.ItemsSource = node.EventHistory;

        // Right panel: WUC IVisual properties (only WucVisual nodes carry these).
        if (node.Kind == TreeNodeKind.WucVisual && node.VisualProperties != null)
        {
            DetailPropertiesList.ItemsSource = node.VisualProperties.ToRows();
            DetailPropertiesList.Visibility = Visibility.Visible;
            DetailPropertiesEmpty.Visibility = Visibility.Collapsed;
        }
        else
        {
            DetailPropertiesList.ItemsSource = null;
            DetailPropertiesList.Visibility = Visibility.Collapsed;
            DetailPropertiesEmpty.Text = node.Kind == TreeNodeKind.WucVisual
                ? "No properties were captured for this IVisual node."
                : "Select an IVisual node to see its properties.";
            DetailPropertiesEmpty.Visibility = Visibility.Visible;
        }
    }

    // =====================================================================
    // Composition Tree auto-follow (always scroll to end)
    // =====================================================================

    /// <summary>
    /// Once the Composition <see cref="TreeView"/> has loaded its template, grab
    /// its internal <see cref="ScrollViewer"/> and start following the bottom.
    /// New comp nodes stream in deep in the hierarchy as ETW events arrive, so we
    /// can't rely on a single collection-changed event; instead we watch the
    /// ScrollViewer's content height and snap to the end whenever it grows.
    /// </summary>
    private void CompositionTreeView_Loaded(object sender, RoutedEventArgs e)
    {
        AttachScrollTracker(CompositionTreeView);
        _compScrollViewer = VisualTreeUtils.FindDescendant<ScrollViewer>(CompositionTreeView);
        if (_compScrollViewer != null)
        {
            _compScrollViewer.LayoutUpdated += CompScrollViewer_LayoutUpdated;
        }
    }

    // The Logical/Visual/WUC trees share the sticky-tail tracker (the Composition tree wires it in
    // its own Loaded handler above, since it additionally drives the auto-follow-to-bottom).
    private void Tree_Loaded(object sender, RoutedEventArgs e)
    {
        if (sender is TreeView tree) AttachScrollTracker(tree);
    }

    // Attach the "sticky tail" tracker to a tree's inner ScrollViewer. Idempotent. Starts in the
    // following state; each settled ViewChanged recomputes whether the view is still at the tail.
    private void AttachScrollTracker(TreeView tree)
    {
        if (_treeScrollViewers.ContainsKey(tree)) return;
        var sv = VisualTreeUtils.FindDescendant<ScrollViewer>(tree);
        if (sv == null) return;
        _treeScrollViewers[tree] = sv;
        _treeFollowing[tree] = true;
        sv.ViewChanged += (s, e) =>
        {
            if (e.IsIntermediate) return;
            _treeFollowing[tree] = IsAtTail(sv);
        };
    }

    // A ScrollViewer is "at the tail" when it sits at the bottom AND flush-left — the resting
    // position auto-scroll targets. Any manual scroll away from it means the user has taken over.
    private static bool IsAtTail(ScrollViewer sv)
    {
        const double eps = 4.0;
        bool atBottom = sv.ScrollableHeight <= eps || sv.VerticalOffset >= sv.ScrollableHeight - eps;
        bool atLeft = sv.HorizontalOffset <= eps;
        return atBottom && atLeft;
    }

    // Trees whose ScrollViewer isn't tracked yet default to following (preserves prior behavior).
    private bool IsTreeFollowing(TreeView tree)
        => !_treeFollowing.TryGetValue(tree, out var following) || following;

    /// <summary>
    /// Keeps the Composition Tree pinned to the bottom. Fires on every layout
    /// pass, but only forces a scroll when the extent (total content height)
    /// actually grew — i.e. new nodes were added. This leaves the user free to
    /// scroll up between additions, while always snapping back to the newest
    /// nodes the moment more arrive.
    /// </summary>
    private void CompScrollViewer_LayoutUpdated(object? sender, object e)
    {
        if (_compScrollViewer == null) return;

        if (_compScrollViewer.ExtentHeight > _compLastExtentHeight + 0.5)
        {
            _compLastExtentHeight = _compScrollViewer.ExtentHeight;
            // Only snap to the newest nodes while the user is parked at the tail. If they scrolled
            // up to inspect something, respect it — otherwise vertical scrolling feels "locked".
            if (IsTreeFollowing(CompositionTreeView))
                _compScrollViewer.ChangeView(null, _compScrollViewer.ScrollableHeight, null, true);
        }
    }

    private class PackageInfo
    {
        public string DisplayName { get; set; } = string.Empty;
        public string PackageFamilyName { get; set; } = string.Empty;
        public string AppUserModelId { get; set; } = string.Empty;
    }
}
