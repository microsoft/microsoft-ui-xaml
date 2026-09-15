// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;

namespace RecyclePoolSampleApp.Common
{
    /// <summary>
    /// Counters shown in the "Factory state" panel on every page. The sample tracks these at the
    /// call site rather than inferring them, so what the panel prints is what actually happened.
    /// </summary>
    public sealed class FactoryStats
    {
        public int Created { get; private set; }

        public int RecycledIn { get; private set; }

        public int PoolHits { get; private set; }

        public int PoolMisses { get; private set; }

        /// <summary>
        /// Incremented from an <c>ElementFactory</c> / <c>RecyclePool</c> subclass override. If this
        /// stays at zero while elements are clearly flowing, the override was never reached.
        /// </summary>
        public int OverrideHits { get; private set; }

        public void OnCreated() => Created++;

        public void OnRecycledIn() => RecycledIn++;

        public void OnPoolHit() => PoolHits++;

        public void OnPoolMiss() => PoolMisses++;

        public void OnOverrideReached() => OverrideHits++;

        public void Reset()
        {
            Created = 0;
            RecycledIn = 0;
            PoolHits = 0;
            PoolMisses = 0;
            OverrideHits = 0;
        }

        public string Describe()
        {
            var builder = new StringBuilder();
            builder.AppendLine($"Elements created   : {Created}");
            builder.AppendLine($"Elements recycled  : {RecycledIn}");
            builder.AppendLine($"Pool hits          : {PoolHits}");
            builder.AppendLine($"Pool misses        : {PoolMisses}");
            builder.Append($"Core override hits : {OverrideHits}");
            return builder.ToString();
        }
    }

    /// <summary>
    /// A <see cref="RecyclePool"/> subclass that mirrors what it is asked to store so the sample can
    /// print the pool contents, and that overrides both <c>Core</c> methods so the sample can show
    /// whether a managed override is reached at all.
    /// </summary>
    public partial class ObservableRecyclePool : RecyclePool
    {
        public ObservableRecyclePool(FactoryStats stats)
        {
            m_stats = stats;
        }

        public event EventHandler Changed;

        /// <summary>
        /// The sample's own mirror of the pool, keyed the same way. <see cref="RecyclePool"/> exposes
        /// no way to enumerate its contents, so a UI that wants to show them has to shadow them.
        /// </summary>
        public IReadOnlyDictionary<string, int> Contents => m_contents;

        public string DescribeContents()
        {
            if (m_contents.Count == 0 || m_contents.Values.All(count => count == 0))
            {
                return "(pool is empty)";
            }

            return string.Join(
                Environment.NewLine,
                m_contents.OrderBy(pair => pair.Key).Select(pair => $"  \"{pair.Key}\" : {pair.Value} element(s)"));
        }

        /// <summary>Wraps <c>PutElement(element, key, owner)</c> and keeps the mirror in step.</summary>
        public void Put(UIElement element, string key, UIElement owner)
        {
            PutElement(element, key, owner);
            Track(key, +1);
            m_stats.OnRecycledIn();
            Changed?.Invoke(this, EventArgs.Empty);
        }

        /// <summary>Wraps <c>PutElement(element, key)</c>, the no-owner overload.</summary>
        public void Put(UIElement element, string key)
        {
            PutElement(element, key);
            Track(key, +1);
            m_stats.OnRecycledIn();
            Changed?.Invoke(this, EventArgs.Empty);
        }

        /// <summary>Wraps <c>TryGetElement(key, owner)</c> and keeps the mirror in step.</summary>
        public UIElement TryGet(string key, UIElement owner)
        {
            var element = TryGetElement(key, owner);
            Settle(key, element);
            return element;
        }

        /// <summary>Wraps <c>TryGetElement(key)</c>, the no-owner overload.</summary>
        public UIElement TryGet(string key)
        {
            var element = TryGetElement(key);
            Settle(key, element);
            return element;
        }

        // The two overrides below exist to probe the Core dispatch. RecyclePool's PutElement and
        // TryGetElement forward to the Core methods internally, so if that forwarding reaches a
        // managed subclass these run and OverrideHits climbs.
        protected override void PutElementCore(UIElement element, string key, UIElement owner)
        {
            m_stats.OnOverrideReached();
            base.PutElementCore(element, key, owner);
        }

        protected override UIElement TryGetElementCore(string key, UIElement owner)
        {
            m_stats.OnOverrideReached();
            return base.TryGetElementCore(key, owner);
        }

        private void Settle(string key, UIElement element)
        {
            if (element != null)
            {
                Track(key, -1);
                m_stats.OnPoolHit();
            }
            else
            {
                m_stats.OnPoolMiss();
            }

            Changed?.Invoke(this, EventArgs.Empty);
        }

        private void Track(string key, int delta)
        {
            m_contents.TryGetValue(key ?? string.Empty, out int count);
            m_contents[key ?? string.Empty] = Math.Max(0, count + delta);
        }

        private readonly Dictionary<string, int> m_contents = new Dictionary<string, int>();
        private readonly FactoryStats m_stats;
    }
}
