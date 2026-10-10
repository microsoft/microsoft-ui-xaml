// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;

namespace RecyclePoolSampleApp.Common
{
    /// <summary>
    /// One row of sample data. <see cref="Kind"/> is what the sample maps onto a template key,
    /// so a single collection can exercise multi-template recycling.
    /// </summary>
    public sealed class Entry : INotifyPropertyChanged
    {
        public Entry(int index, string kind, string label)
        {
            Index = index;
            Kind = kind;
            Label = label;
        }

        public int Index { get; }

        /// <summary>"Header", "Item" or "Footer". Used as the template key by the sample pages.</summary>
        public string Kind { get; }

        public string Label { get; }

        public override string ToString() => Label;

        public event PropertyChangedEventHandler PropertyChanged;

        private void Raise(string name) => PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
    }

    public static class SampleData
    {
        /// <summary>
        /// A collection with three kinds of row interleaved, so that a
        /// <c>RecyclingElementFactory</c> with three templates has to pick a key per item and
        /// therefore maintains three independent sub-pools.
        /// </summary>
        public static ObservableCollection<Entry> CreateMixed(int count)
        {
            var items = new ObservableCollection<Entry>();
            for (int i = 0; i < count; i++)
            {
                string kind = (i % 5) switch
                {
                    0 => "Header",
                    4 => "Footer",
                    _ => "Item",
                };

                items.Add(new Entry(i, kind, $"{kind} {i}"));
            }

            return items;
        }

        public static ObservableCollection<Entry> CreateUniform(int count)
        {
            var items = new ObservableCollection<Entry>();
            for (int i = 0; i < count; i++)
            {
                items.Add(new Entry(i, "Item", $"Item {i}"));
            }

            return items;
        }

        public static IReadOnlyList<string> Kinds { get; } = new[] { "Header", "Item", "Footer" };
    }
}
