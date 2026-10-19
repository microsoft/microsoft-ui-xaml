// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.ObjectModel;
using System.Text;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using RecyclePoolSampleApp.Common;

namespace RecyclePoolSampleApp.Pages
{
    public sealed partial class RecyclingElementFactoryPage : Page, IScenarioPage
    {
        public RecyclingElementFactoryPage()
        {
            this.InitializeComponent();

            m_items = SampleData.CreateMixed(200);
            m_stats = new FactoryStats();

            // Three templates means three independent sub-pools inside the one RecyclePool: a
            // "Header" element is never handed back for an "Item".
            m_factory = new TrackingRecyclingElementFactory(m_stats)
            {
                RecyclePool = new RecyclePool(),
            };

            m_factory.Templates["Header"] = (DataTemplate)Resources["HeaderTemplate"];
            m_factory.Templates["Item"] = (DataTemplate)Resources["ItemTemplate"];
            m_factory.Templates["Footer"] = (DataTemplate)Resources["FooterTemplate"];

            // With more than one template the factory has to be told which key each item wants.
            m_factory.SelectTemplateKey += OnSelectTemplateKey;
            m_factory.Changed += (s, e) => RefreshVisuals();

            Repeater.ItemTemplate = m_factory;
            Repeater.ItemsSource = m_items;

            RefreshVisuals();
        }

        public void ApplyScenario(string scenario)
        {
            switch (scenario)
            {
                case "warm":
                    // Scroll far enough that every element on screen came out of the pool rather
                    // than out of a DataTemplate.
                    ScrollHelper.ScrollToWhenReady(Scroller, 4000, RefreshVisuals);
                    break;

                case "cold":
                default:
                    Scroller.ChangeView(null, 0, null, true);
                    break;
            }

            ResultText.Text = scenario == "warm"
                ? "After scrolling, 'Elements created' has stopped climbing: every new row is\na recycled element from the pool."
                : "Freshly loaded. Every visible row had to be built from a DataTemplate.";

            RefreshVisuals();
        }

        /// <summary>
        /// Raised once per item that needs an element, unless exactly one template is registered -
        /// in that case RecyclingElementFactory short-circuits and uses it without asking.
        /// </summary>
        private void OnSelectTemplateKey(RecyclingElementFactory sender, SelectTemplateEventArgs args)
        {
            var entry = (Entry)args.DataContext;

            // The handler's job is to fill in TemplateKey. Leaving it empty is an error: the factory
            // throws "Please provide a valid template identifier in the handler for the
            // SelectTemplateKey event."
            args.TemplateKey = entry.Kind;

            m_selectCount++;
            if (m_selectCount <= 40)
            {
                Log($"#{m_selectCount} DataContext={entry.Label,-12} -> TemplateKey=\"{args.TemplateKey}\"");
            }
        }

        private void OnScrollDown(object sender, RoutedEventArgs e)
            => Scroller.ChangeView(null, Scroller.VerticalOffset + 1200, null, false);

        private void OnScrollTop(object sender, RoutedEventArgs e)
            => Scroller.ChangeView(null, 0, null, false);

        private void OnResetCounters(object sender, RoutedEventArgs e)
        {
            m_stats.Reset();
            m_selectCount = 0;
            m_log.Clear();
            LogText.Text = string.Empty;
            ResultText.Text = "Counters reset. Scroll again - creations should stay flat.";
            RefreshVisuals();
        }

        private void Log(string line)
        {
            m_log.Insert(0, line + Environment.NewLine);
            if (m_log.Length > 1400)
            {
                m_log.Length = 1400;
            }

            LogText.Text = m_log.ToString();
        }

        private void RefreshVisuals()
        {
            StateText.Text =
                m_stats.Describe() + Environment.NewLine +
                $"SelectTemplateKey event : {m_selectCount}" + Environment.NewLine +
                $"OnSelectTemplateKeyCore : {m_factory.SelectTemplateKeyCoreHits}";

            var builder = new StringBuilder();
            foreach (var key in m_factory.Templates.Keys)
            {
                builder.AppendLine($"  \"{key}\"");
            }

            TemplatesText.Text =
                $"Templates.Size = {m_factory.Templates.Count}" + Environment.NewLine +
                builder.ToString().TrimEnd() + Environment.NewLine +
                $"RecyclePool    = {(m_factory.RecyclePool == null ? "null" : "set")}";
        }

        private readonly ObservableCollection<Entry> m_items;
        private readonly TrackingRecyclingElementFactory m_factory;
        private readonly FactoryStats m_stats;
        private readonly StringBuilder m_log = new StringBuilder();
        private int m_selectCount;
    }
}
