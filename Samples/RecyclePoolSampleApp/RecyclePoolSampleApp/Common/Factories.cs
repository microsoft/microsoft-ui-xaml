// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;

namespace RecyclePoolSampleApp.Common
{
    /// <summary>
    /// A <see cref="RecyclingElementFactory"/> subclass that counts what flows through the three
    /// overridable members - <c>GetElementCore</c>, <c>RecycleElementCore</c> and
    /// <c>OnSelectTemplateKeyCore</c> - and optionally supplies the template key itself instead of
    /// letting the base raise <c>SelectTemplateKey</c>.
    /// </summary>
    public partial class TrackingRecyclingElementFactory : RecyclingElementFactory
    {
        public TrackingRecyclingElementFactory(FactoryStats stats)
        {
            m_stats = stats;
        }

        /// <summary>
        /// When set, <c>OnSelectTemplateKeyCore</c> answers from this function and the
        /// <c>SelectTemplateKey</c> event is never raised. When null, the base implementation runs
        /// and the event decides.
        /// </summary>
        public Func<object, UIElement, string> SelectTemplateKeyFunc { get; set; }

        /// <summary>Raised after every element hand-out, so a page can refresh its state panel.</summary>
        public event EventHandler Changed;

        protected override UIElement GetElementCore(ElementFactoryGetArgs args)
        {
            int before = m_seen.Count;
            var element = base.GetElementCore(args);

            // The base either pulled this element out of the pool or built it from a DataTemplate.
            // There is no flag saying which, so the sample decides by whether it has seen the
            // instance before.
            if (m_seen.Add(element))
            {
                m_stats.OnCreated();
                m_stats.OnPoolMiss();
            }
            else
            {
                m_stats.OnPoolHit();
            }

            _ = before;
            Changed?.Invoke(this, EventArgs.Empty);
            return element;
        }

        protected override void RecycleElementCore(ElementFactoryRecycleArgs args)
        {
            base.RecycleElementCore(args);
            m_stats.OnRecycledIn();
            Changed?.Invoke(this, EventArgs.Empty);
        }

        /// <summary>
        /// Incremented by the <c>OnSelectTemplateKeyCore</c> override below. The base
        /// <c>GetElementCore</c> calls <c>OnSelectTemplateKeyCore</c> directly rather than through
        /// the overridable, so this stays at zero even while keys are clearly being selected.
        /// </summary>
        public int SelectTemplateKeyCoreHits { get; private set; }

        protected override string OnSelectTemplateKeyCore(object dataContext, UIElement owner)
        {
            SelectTemplateKeyCoreHits++;

            if (SelectTemplateKeyFunc != null)
            {
                return SelectTemplateKeyFunc(dataContext, owner);
            }

            // Raises the SelectTemplateKey event and validates that a handler filled TemplateKey in.
            return base.OnSelectTemplateKeyCore(dataContext, owner);
        }

        private readonly HashSet<UIElement> m_seen = new HashSet<UIElement>();
        private readonly FactoryStats m_stats;
    }

    /// <summary>
    /// A hand-written <see cref="ElementFactory"/>: it owns its own <see cref="RecyclePool"/> and
    /// implements the whole get/recycle contract itself, which is what an app writes when
    /// <see cref="RecyclingElementFactory"/>'s "one DataTemplate per key" model does not fit.
    /// </summary>
    public partial class CountingElementFactory : ElementFactory
    {
        public CountingElementFactory(FactoryStats stats, IReadOnlyDictionary<string, DataTemplate> templates)
        {
            m_stats = stats;
            m_templates = templates;
            m_pool = new ObservableRecyclePool(stats);
        }

        public ObservableRecyclePool Pool => m_pool;

        public Func<object, string> SelectKey { get; set; }

        public event EventHandler Changed;

        protected override UIElement GetElementCore(ElementFactoryGetArgs args)
        {
            string key = SelectKey?.Invoke(args.Data) ?? "Item";

            // Ask the pool first. A null answer means nothing suitable was parked under this key.
            var element = m_pool.TryGet(key, args.Parent) as FrameworkElement;

            if (element == null)
            {
                element = (FrameworkElement)m_templates[key].LoadContent();
                m_stats.OnCreated();
            }

            // RecyclePool's ReuseKey attached property is not part of the public surface, so a
            // custom factory keeps its own element-to-key map in order to recycle under the same
            // key it handed the element out under.
            m_keys[element] = key;

            element.DataContext = args.Data;
            Changed?.Invoke(this, EventArgs.Empty);
            return element;
        }

        protected override void RecycleElementCore(ElementFactoryRecycleArgs args)
        {
            if (args.Element is FrameworkElement element && m_keys.TryGetValue(element, out string key))
            {
                m_pool.Put(element, key, args.Parent);
            }

            Changed?.Invoke(this, EventArgs.Empty);
        }

        private readonly Dictionary<UIElement, string> m_keys = new Dictionary<UIElement, string>();
        private readonly IReadOnlyDictionary<string, DataTemplate> m_templates;
        private readonly ObservableRecyclePool m_pool;
        private readonly FactoryStats m_stats;
    }
}
