// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Collections.Specialized;
using System.ComponentModel;
using Microsoft.UI.Dispatching;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using TableViewSampleApp.Controls;

namespace TableViewSampleApp.Pages;

/// <summary>
/// Base of every sample page: the generic plumbing (Status, Shaping, Last action, Loaded/Unloaded
/// bookkeeping) so the page itself holds only the TableView behaviour it demonstrates.
/// <para>
/// A page calls <see cref="InitializeSample"/> once in its constructor, overrides
/// <see cref="RefreshReadouts"/> (in its <c>&lt;X&gt;Page.Status.cs</c> partial) and, where its
/// feature reacts to shaping, <see cref="OnShapingApplying"/> / <see cref="OnShapingApplied"/>.
/// </para>
/// <para>
/// Non-abstract with a public parameterless constructor so it can be a XAML root
/// (<c>&lt;pages:SamplePageBase x:Class="…"&gt;</c>); partial for CsWinRT.
/// </para>
/// </summary>
public partial class SamplePageBase : Page
{
    private readonly List<Action> _onLoaded = new();
    private readonly List<Action> _onUnloaded = new();
    private StatusPanel? _status;
    private ShapingOptions? _shaping;
    private bool _lifetimeHooked;
    private int _bulkUpdateDepth;

    /// <summary>True after a GroupBy returned in Grouped mode (never while it is being applied).</summary>
    protected bool IsGrouped => _shaping?.IsGrouped ?? false;

    /// <summary>The group key last applied (the group-key ComboBoxItem Tag).</summary>
    protected string AppliedGroupKey => _shaping?.AppliedKey ?? string.Empty;

    /// <summary>True when the active sort was declared before the grouping, so it orders the groups.</summary>
    protected bool SortOrdersGroups => _shaping?.SortOrdersGroups ?? false;

    /// <summary>True inside a <see cref="BeginBulkUpdate"/> scope: per-item change handlers should skip.</summary>
    protected bool IsBulkUpdating => _bulkUpdateDepth > 0;

    /// <summary>
    /// Connects the page's Status section and, if it has one, its Shaping section (already
    /// <see cref="ShapingOptions.Attach"/>ed). Applies the initial shaping, then refreshes the readouts.
    /// </summary>
    protected void InitializeSample(StatusPanel status, ShapingOptions? shaping = null)
    {
        _status = status;
        _shaping = shaping;
        if (shaping is not null)
        {
            shaping.ShapingApplying += (_, e) => OnShapingApplying(e);
            shaping.ShapingApplied += OnShapingAppliedCore;
            shaping.ActionPerformed += (_, e) =>
            {
                OnShapingAction(e);
                SetLastAction(e.Message);
            };
            shaping.SelectionRestored += (_, _) => RefreshReadouts();
        }

        HookLifetime();
        shaping?.ApplyInitialState();
        RefreshReadouts();
    }

    /// <summary>
    /// The only writer of the Last action readout. Also refreshes every readout, then calls
    /// <see cref="OnLastActionSet"/>.
    /// </summary>
    protected void SetLastAction(string message)
    {
        if (_status is not null)
        {
            _status.LastAction = message;
        }

        RefreshReadouts();
        OnLastActionSet(message);
    }

    /// <summary>
    /// Runs after every Last action write and its readout refresh (e.g. to read the readouts again
    /// once layout has settled).
    /// </summary>
    protected virtual void OnLastActionSet(string message)
    {
    }

    /// <summary>Writes the page's readouts. Override in <c>&lt;X&gt;Page.Status.cs</c>.</summary>
    protected virtual void RefreshReadouts()
    {
    }

    /// <summary>Runs before GroupBy / ClearGroupBy (e.g. a pre-sort, capturing state).</summary>
    protected virtual void OnShapingApplying(ShapingApplyingEventArgs e)
    {
    }

    /// <summary>
    /// Runs after the reshape, before the readouts refresh. Order: Shaping readout ← e.Text, this
    /// hook, then Last action ← e.Message (announced) or RefreshReadouts.
    /// </summary>
    protected virtual void OnShapingApplied(ShapingAppliedEventArgs e)
    {
    }

    /// <summary>
    /// Runs after Expand all / Collapse all (Shaping section), before Last action ← e.Message.
    /// <see cref="ShapingActionEventArgs.Action"/> says which; a page may extend e.Message.
    /// </summary>
    protected virtual void OnShapingAction(ShapingActionEventArgs e)
    {
    }

    /// <summary>Applies the grouping again if it is on <paramref name="propertyName"/>.</summary>
    protected void ReapplyIfGroupedOn(string? propertyName) => _shaping?.ReapplyIfGroupedOn(propertyName);

    /// <summary>
    /// Listens to PropertyChanged of every item while the page is loaded, following adds, removes
    /// and resets (Clear); detaches on Unloaded.
    /// </summary>
    protected void TrackItems<T>(ObservableCollection<T> items, PropertyChangedEventHandler onItemChanged)
        where T : class, INotifyPropertyChanged
    {
        // Reset (Clear) reports no OldItems, so remember what is attached: otherwise the cleared
        // rows keep their handler and restoring the same instances attaches it a second time.
        var attached = new HashSet<T>(ReferenceEqualityComparer.Instance);

        void Attach(T item)
        {
            if (attached.Add(item))
            {
                item.PropertyChanged += onItemChanged;
            }
        }

        void DetachAll()
        {
            foreach (var item in attached)
            {
                item.PropertyChanged -= onItemChanged;
            }

            attached.Clear();
        }

        void OnCollectionChanged(object? sender, NotifyCollectionChangedEventArgs e)
        {
            if (e.Action == NotifyCollectionChangedAction.Reset)
            {
                DetachAll();
                foreach (var item in items)
                {
                    Attach(item);
                }

                return;
            }

            foreach (T item in e.OldItems ?? Array.Empty<T>())
            {
                if (attached.Remove(item))
                {
                    item.PropertyChanged -= onItemChanged;
                }
            }

            foreach (T item in e.NewItems ?? Array.Empty<T>())
            {
                Attach(item);
            }
        }

        TrackLifetime(
            () =>
            {
                items.CollectionChanged += OnCollectionChanged;
                foreach (var item in items)
                {
                    Attach(item);
                }
            },
            () =>
            {
                items.CollectionChanged -= OnCollectionChanged;
                DetachAll();
            });
    }

    /// <summary>
    /// Runs <paramref name="onLoaded"/> on every Loaded (before the readouts refresh) and
    /// <paramref name="onUnloaded"/>, if any, on every Unloaded.
    /// </summary>
    protected void TrackLifetime(Action onLoaded, Action? onUnloaded = null)
    {
        HookLifetime();
        _onLoaded.Add(onLoaded);
        if (onUnloaded is not null)
        {
            _onUnloaded.Add(onUnloaded);
        }
    }

    /// <summary>
    /// Stops <paramref name="timer"/> when the page unloads and, on every Loaded, starts it again
    /// if <paramref name="runWhenLoaded"/> returns true.
    /// </summary>
    protected void TrackTimer(DispatcherQueueTimer timer, Func<bool>? runWhenLoaded = null) =>
        TrackLifetime(
            () =>
            {
                if (runWhenLoaded?.Invoke() == true)
                {
                    timer.Start();
                }
            },
            timer.Stop);

    /// <inheritdoc cref="TrackTimer(DispatcherQueueTimer, Func{bool}?)"/>
    protected void TrackTimer(DispatcherTimer timer, Func<bool>? runWhenLoaded = null) =>
        TrackLifetime(
            () =>
            {
                if (runWhenLoaded?.Invoke() == true)
                {
                    timer.Start();
                }
            },
            timer.Stop);

    /// <summary>
    /// Queues <paramref name="action"/> on the page's dispatcher and runs it only if the page is still
    /// loaded then (a queued callback must never touch a page that was navigated away from).
    /// </summary>
    protected bool EnqueueIfLoaded(Action action, DispatcherQueuePriority priority = DispatcherQueuePriority.Normal) =>
        DispatcherQueue.TryEnqueue(priority, () =>
        {
            if (!IsLoaded)
            {
                return;
            }

            action();
        });

    /// <summary>
    /// A scope in which <see cref="IsBulkUpdating"/> is true: <c>using (BeginBulkUpdate()) { … }</c>.
    /// </summary>
    protected IDisposable BeginBulkUpdate()
    {
        _bulkUpdateDepth++;
        return new BulkUpdateScope(this);
    }

    private void OnShapingAppliedCore(object? sender, ShapingAppliedEventArgs e)
    {
        if (_status is not null)
        {
            _status.ShapingText = e.Text;
        }

        OnShapingApplied(e);
        if (e.Announce)
        {
            SetLastAction(e.Message);
        }
        else
        {
            RefreshReadouts();
        }
    }

    private void HookLifetime()
    {
        if (_lifetimeHooked)
        {
            return;
        }

        _lifetimeHooked = true;
        Loaded += OnSampleLoaded;
        Unloaded += OnSampleUnloaded;
    }

    private void OnSampleLoaded(object sender, RoutedEventArgs e)
    {
        foreach (var action in _onLoaded)
        {
            action();
        }

        RefreshReadouts();
    }

    private void OnSampleUnloaded(object sender, RoutedEventArgs e)
    {
        foreach (var action in _onUnloaded)
        {
            action();
        }
    }

    private sealed class BulkUpdateScope : IDisposable
    {
        private SamplePageBase? _owner;

        public BulkUpdateScope(SamplePageBase owner) => _owner = owner;

        public void Dispose()
        {
            if (_owner is not null)
            {
                _owner._bulkUpdateDepth--;
                _owner = null;
            }
        }
    }
}
