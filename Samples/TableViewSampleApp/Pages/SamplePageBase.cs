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

// Non-abstract with a public parameterless constructor so it can be a XAML root.
public partial class SamplePageBase : Page
{
    private readonly List<Action> _onLoaded = new();
    private readonly List<Action> _onUnloaded = new();
    private StatusPanel? _status;
    private ShapingOptions? _shaping;
    private bool _lifetimeHooked;
    private bool _isLoadedHooked;
    private int _bulkUpdateDepth;

    protected bool IsGrouped => _shaping?.IsGrouped ?? false;

    protected string AppliedGroupKey => _shaping?.AppliedKey ?? string.Empty;

    protected bool SortOrdersGroups => _shaping?.SortOrdersGroups ?? false;

    protected bool IsBulkUpdating => _bulkUpdateDepth > 0;

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

            // A header-click sort raises Sorted to the page's own XAML handler BEFORE ShapingOptions
            // records it (XAML subscribes first), so refresh again once the sort state is current.
            shaping.SortStateChanged += (_, _) => RefreshReadouts();
        }

        HookLifetime();
        shaping?.ApplyInitialState();
        RefreshReadouts();
    }

    protected void SetLastAction(string message)
    {
        if (_status is not null)
        {
            _status.LastAction = message;
        }

        RefreshReadouts();
        OnLastActionSet(message);
    }

    protected virtual void OnLastActionSet(string message)
    {
    }

    protected virtual void RefreshReadouts()
    {
    }

    protected virtual void OnShapingApplying(ShapingApplyingEventArgs e)
    {
    }

    protected virtual void OnShapingApplied(ShapingAppliedEventArgs e)
    {
    }

    protected virtual void OnShapingAction(ShapingActionEventArgs e)
    {
    }

    protected void ReapplyIfGroupedOn(string? propertyName) => _shaping?.ReapplyIfGroupedOn(propertyName);

    protected void OnSortRedeclared() => _shaping?.OnSortRedeclared();

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

    // Loaded and Unloaded are paired: a second Loaded without an Unloaded (reparenting) does not run
    // onLoaded again.
    protected void TrackLifetime(Action onLoaded, Action? onUnloaded = null)
    {
        HookLifetime();
        _onLoaded.Add(onLoaded);
        if (onUnloaded is not null)
        {
            _onUnloaded.Add(onUnloaded);
        }

        if (_isLoadedHooked)
        {
            onLoaded();
        }
    }

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

    // A queued callback must never touch a page that was navigated away from.
    protected bool EnqueueIfLoaded(Action action, DispatcherQueuePriority priority = DispatcherQueuePriority.Normal) =>
        DispatcherQueue.TryEnqueue(priority, () =>
        {
            if (!IsLoaded)
            {
                return;
            }

            action();
        });

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
        if (_isLoadedHooked)
        {
            return;
        }

        _isLoadedHooked = true;
        foreach (var action in _onLoaded)
        {
            action();
        }

        RefreshReadouts();
    }

    private void OnSampleUnloaded(object sender, RoutedEventArgs e)
    {
        if (!_isLoadedHooked)
        {
            return;
        }

        _isLoadedHooked = false;
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
