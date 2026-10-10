// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.ObjectModel;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using RecyclePoolSampleApp.Common;

namespace RecyclePoolSampleApp.Pages
{
    public sealed partial class SharedPoolPage : Page, IScenarioPage
    {
        public SharedPoolPage()
        {
            this.InitializeComponent();

            m_items = SampleData.CreateUniform(200);
            m_stats = new FactoryStats();
            m_template = (DataTemplate)Resources["SharedTemplate"];

            // The pool is attached to the DataTemplate rather than to either factory, so any code
            // holding the template can reach it - that is the whole point of the attached property.
            m_pool = new RecyclePool();
            RecyclePool.SetPoolInstance(m_template, m_pool);

            m_factory1 = CreateFactory();
            m_factory2 = CreateFactory();

            Repeater1.ItemTemplate = m_factory1;
            Repeater1.ItemsSource = m_items;
            Repeater2.ItemTemplate = m_factory2;
            Repeater2.ItemsSource = m_items;

            RefreshVisuals();
        }

        public void ApplyScenario(string scenario)
        {
            switch (scenario)
            {
                case "shared":
                    ScrollHelper.ScrollToWhenReady(Scroller1, 3000);
                    ScrollHelper.ScrollToWhenReady(Scroller2, 3000);
                    ResultText.Text =
                        "Both repeaters are pulling from one pool. Because the pool prefers an\n" +
                        "element that already belongs to the asking owner, each repeater keeps\n" +
                        "reusing its own elements and neither steals from the other.";
                    break;

                default:
                    ResultText.Text =
                        "One RecyclePool is attached to the shared DataTemplate and both\n" +
                        "RecyclingElementFactory instances were given it.";
                    break;
            }

            RefreshVisuals();
        }

        private TrackingRecyclingElementFactory CreateFactory()
        {
            var factory = new TrackingRecyclingElementFactory(m_stats)
            {
                // Read the pool back off the template instead of capturing the field, to show the
                // round trip actually works.
                RecyclePool = RecyclePool.GetPoolInstance(m_template),
            };

            factory.Templates["Item"] = m_template;

            // With exactly one template registered, RecyclingElementFactory skips
            // OnSelectTemplateKeyCore entirely and uses the single key, so no SelectTemplateKey
            // handler is needed here.
            factory.Changed += (s, e) => RefreshVisuals();
            return factory;
        }

        private void OnScrollBoth(object sender, RoutedEventArgs e)
        {
            Scroller1.ChangeView(null, Scroller1.VerticalOffset + 1200, null, false);
            Scroller2.ChangeView(null, Scroller2.VerticalOffset + 1200, null, false);
        }

        private void OnResetCounters(object sender, RoutedEventArgs e)
        {
            m_stats.Reset();
            RefreshVisuals();
        }

        private void OnGetPoolInstance(object sender, RoutedEventArgs e)
        {
            var fromTemplate = RecyclePool.GetPoolInstance(m_template);
            var onAnotherTemplate = RecyclePool.GetPoolInstance(new DataTemplate());

            ResultText.Text =
                $"GetPoolInstance(SharedTemplate)  -> {(ReferenceEquals(fromTemplate, m_pool) ? "the same pool we set" : "a different instance")}\n" +
                $"GetPoolInstance(new DataTemplate) -> {(onAnotherTemplate == null ? "null (nothing attached)" : "an instance")}\n" +
                $"factory1.RecyclePool == factory2.RecyclePool -> {ReferenceEquals(m_factory1.RecyclePool, m_factory2.RecyclePool)}";

            RefreshVisuals();
        }

        private void RefreshVisuals()
        {
            StateText.Text = m_stats.Describe();

            AttachedText.Text =
                $"PoolInstanceProperty : {(RecyclePool.PoolInstanceProperty == null ? "null" : "registered")}" + Environment.NewLine +
                $"Attached to template : {(RecyclePool.GetPoolInstance(m_template) != null ? "yes" : "no")}" + Environment.NewLine +
                $"Same pool both sides : {ReferenceEquals(m_factory1.RecyclePool, m_factory2.RecyclePool)}";
        }

        private readonly ObservableCollection<Entry> m_items;
        private readonly TrackingRecyclingElementFactory m_factory1;
        private readonly TrackingRecyclingElementFactory m_factory2;
        private readonly FactoryStats m_stats;
        private readonly DataTemplate m_template;
        private readonly RecyclePool m_pool;
    }
}
