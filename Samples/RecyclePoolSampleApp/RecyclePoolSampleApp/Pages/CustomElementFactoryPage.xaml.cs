// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using RecyclePoolSampleApp.Common;

namespace RecyclePoolSampleApp.Pages
{
    public sealed partial class CustomElementFactoryPage : Page, IScenarioPage
    {
        public CustomElementFactoryPage()
        {
            this.InitializeComponent();

            m_items = SampleData.CreateMixed(200);
            m_stats = new FactoryStats();

            var templates = new Dictionary<string, DataTemplate>
            {
                ["Header"] = (DataTemplate)Resources["HeaderTemplate"],
                ["Item"] = (DataTemplate)Resources["ItemTemplate"],
                ["Footer"] = (DataTemplate)Resources["FooterTemplate"],
            };

            m_factory = new CountingElementFactory(m_stats, templates)
            {
                SelectKey = data => ((Entry)data).Kind,
            };
            m_factory.Changed += (s, e) => RefreshVisuals();

            // ItemsRepeater.ItemTemplate accepts anything that implements IElementFactory, which is
            // why a hand-written factory can be assigned here directly.
            Repeater.ItemTemplate = m_factory;
            Repeater.ItemsSource = m_items;

            RefreshVisuals();
        }

        public void ApplyScenario(string scenario)
        {
            switch (scenario)
            {
                case "warm":
                    ScrollHelper.ScrollToWhenReady(Scroller, 4000, RefreshVisuals);
                    ResultText.Text =
                        "GetElementCore is answering from the factory's own RecyclePool.\n" +
                        "'Elements created' has stopped climbing.";
                    break;

                case "cold":
                default:
                    Scroller.ChangeView(null, 0, null, true);
                    ResultText.Text =
                        "Every visible row was built by GetElementCore from a DataTemplate,\n" +
                        "because the pool was empty.";
                    break;
            }

            RefreshVisuals();
        }

        private void OnScrollDown(object sender, RoutedEventArgs e)
            => Scroller.ChangeView(null, Scroller.VerticalOffset + 1200, null, false);

        private void OnScrollTop(object sender, RoutedEventArgs e)
            => Scroller.ChangeView(null, 0, null, false);

        private void OnResetCounters(object sender, RoutedEventArgs e)
        {
            m_stats.Reset();
            ResultText.Text = "Counters reset.";
            RefreshVisuals();
        }

        private void RefreshVisuals()
        {
            StateText.Text = m_stats.Describe();
            PoolText.Text = m_factory.Pool.DescribeContents();

            // ElementFactory.GetElement calls overridable().GetElementCore, so a managed override is
            // reached and this page works at all. RecyclePool's PutElement / TryGetElement call
            // their Core methods directly instead, so the counter below reports what actually
            // happens for the pool half of the contract.
            DispatchText.Text =
                $"ElementFactory.GetElementCore reached : {(m_stats.Created + m_stats.PoolHits > 0 ? "yes" : "not yet")}" +
                Environment.NewLine +
                $"RecyclePool Core overrides reached    : {(m_stats.OverrideHits > 0 ? $"yes ({m_stats.OverrideHits})" : "no")}";
        }

        private readonly ObservableCollection<Entry> m_items;
        private readonly CountingElementFactory m_factory;
        private readonly FactoryStats m_stats;
    }
}
