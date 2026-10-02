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
    public sealed partial class MarkupPage : Page, IScenarioPage
    {
        public MarkupPage()
        {
            this.InitializeComponent();

            m_items = SampleData.CreateMixed(200);
            m_factory = (RecyclingElementFactory)Resources["MarkupFactory"];

            // Everything else came from markup. Only the key-selection callback has to be code.
            m_factory.SelectTemplateKey += OnSelectTemplateKey;

            Repeater.ItemsSource = m_items;

            RefreshVisuals();
        }

        public void ApplyScenario(string scenario)
        {
            switch (scenario)
            {
                case "scrolled":
                    ScrollHelper.ScrollToWhenReady(Scroller, 2000, RefreshVisuals);
                    break;

                default:
                    Scroller.ChangeView(null, 0, null, true);
                    break;
            }

            ResultText.Text =
                "The factory instance below was built by the XAML parser, not by code-behind.";
            RefreshVisuals();
        }

        private void OnSelectTemplateKey(RecyclingElementFactory sender, SelectTemplateEventArgs args)
        {
            args.TemplateKey = ((Entry)args.DataContext).Kind;
            m_calls++;
            RefreshVisuals();
        }

        private void OnScrollDown(object sender, RoutedEventArgs e)
            => Scroller.ChangeView(null, Scroller.VerticalOffset + 1200, null, false);

        private void OnScrollTop(object sender, RoutedEventArgs e)
            => Scroller.ChangeView(null, 0, null, false);

        private void RefreshVisuals()
        {
            var builder = new StringBuilder();
            builder.AppendLine($"Templates.Size : {m_factory.Templates.Count}");
            foreach (var key in m_factory.Templates.Keys)
            {
                builder.AppendLine($"  \"{key}\"");
            }

            builder.AppendLine($"RecyclePool    : {(m_factory.RecyclePool == null ? "null" : "set from markup")}");
            builder.Append($"SelectTemplateKey calls : {m_calls}");

            StateText.Text = builder.ToString();
        }

        private readonly ObservableCollection<Entry> m_items;
        private readonly RecyclingElementFactory m_factory;
        private int m_calls;
    }
}
