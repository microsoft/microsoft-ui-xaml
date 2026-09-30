using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Reflection;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
// tabular-namespace TableView aliases: disambiguate from the (stale-mock) base Microsoft.UI.Xaml.Controls.TableView projection
using TableView = Microsoft.UI.Xaml.Controls.Tabular.TableView;
using TableViewColumn = Microsoft.UI.Xaml.Controls.Tabular.TableViewColumn;
using TableViewTextColumn = Microsoft.UI.Xaml.Controls.Tabular.TableViewTextColumn;
// #44 enum unification: TableViewSortDirection was removed in favor of the shared Data enum.
using TableViewSortDirection = Microsoft.UI.Xaml.Controls.Tabular.SortDirection;
using Microsoft.UI.Xaml.Controls.Primitives;

namespace TableViewSampleApp.Data;

internal static class SampleShape
{
    private static readonly ConcurrentDictionary<Type, ConcurrentDictionary<string, Func<object, IComparable?>?>> s_selectorCache = new();

    public static void EnableDefaults<T>(TableView table, ObservableCollection<T> source) where T : class => EnableDefaultSort(table, source);

    public static void EnableDefaultSort<T>(TableView table, ObservableCollection<T> source) where T : class
    {
        if (table is null || source is null) return;
        table.Sorted += (_, _) =>
        {
            var sorted = ApplySort(table, source.ToList());
            ApplyToObservable(source, sorted);
        };
    }

    public static void EnableDefaultFilter<T>(TableView table, ObservableCollection<T> source) where T : class
    {
    }

    // Sort an arbitrary item set by the table's active sort, reusing the same
    // reflection key-selector the flat default sort uses.
    public static List<T> SortByActiveSort<T>(TableView table, IEnumerable<T> items) where T : class
        => ApplySort(table, items.ToList());

    /// <summary>
    /// The column carrying the active sort, or null when nothing is sorted.
    ///
    /// Sort state lives on the COLUMN - TableViewColumn.SortDirection - rather than on scalar
    /// SortColumn / SortDirection properties on the table. Sorting is single-column, so at most
    /// one column is ever not None.
    /// </summary>
    public static TableViewColumn? ActiveSortColumn(TableView table)
    {
        foreach (var column in table.Columns)
        {
            if (column.SortDirection != TableViewSortDirection.None)
            {
                return column;
            }
        }

        return null;
    }

    /// <summary>The active sort direction, or None when nothing is sorted.</summary>
    public static TableViewSortDirection ActiveSortDirection(TableView table)
        => ActiveSortColumn(table)?.SortDirection ?? TableViewSortDirection.None;

    private static List<T> ApplySort<T>(TableView table, List<T> items) where T : class
    {
        // Sort state lives on the COLUMN now - TableViewColumn.SortDirection - rather than on a
        // pair of scalar DPs on the table, so the active sort is whichever column is not None.
        // Sorting is single-column, so there is at most one.
        TableViewColumn? column = null;
        foreach (var candidate in table.Columns)
        {
            if (candidate.SortDirection != TableViewSortDirection.None)
            {
                column = candidate;
                break;
            }
        }

        if (column is null)
        {
            return items;
        }

        var selector = BuildKeySelector<T>(column);
        if (selector is null)
        {
            return items;
        }

        return column.SortDirection == TableViewSortDirection.Descending
            ? items.OrderByDescending(selector).ToList()
            : items.OrderBy(selector).ToList();
    }

    private static void ApplyToObservable<T>(ObservableCollection<T> source, List<T> target) where T : class
    {
        source.Clear();
        foreach (var item in target) source.Add(item);
    }

    private static Func<T, IComparable?>? BuildKeySelector<T>(TableViewColumn col) where T : class
    {
        var path = ResolveSortMemberPath(col);
        if (string.IsNullOrEmpty(path)) return null;
        var rawSelector = GetCachedSelector(typeof(T), path);
        return rawSelector is null ? null : item => item is null ? null : rawSelector(item);
    }

    private static Func<object, IComparable?>? GetCachedSelector(Type rootType, string path)
    {
        var perType = s_selectorCache.GetOrAdd(rootType, _ => new ConcurrentDictionary<string, Func<object, IComparable?>?>(StringComparer.Ordinal));
        return perType.GetOrAdd(path, p => BuildSelector(rootType, p));
    }

    private static string? ResolveSortMemberPath(TableViewColumn col)
    {
        if (!string.IsNullOrEmpty(col.SortMemberPath)) return col.SortMemberPath;
        return col is TableViewTextColumn t ? t.Binding?.Path?.Path : null;
    }

    private static Func<object, IComparable?>? BuildSelector(Type rootType, string path)
    {
        var parts = path.Split('.');
        var props = new PropertyInfo[parts.Length];
        var cur = rootType;
        for (int i = 0; i < parts.Length; i++)
        {
            var pi = cur.GetProperty(parts[i], BindingFlags.Public | BindingFlags.Instance | BindingFlags.IgnoreCase);
            if (pi is null) return null;
            props[i] = pi;
            cur = pi.PropertyType;
        }

        return obj =>
        {
            object? v = obj;
            foreach (var pi in props)
            {
                if (v is null) return null;
                v = pi.GetValue(v);
            }
            return v as IComparable;
        };
    }
}
