// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.ObjectModel;
using System.Runtime.CompilerServices;
using System.Text;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using RecyclePoolSampleApp.Common;

namespace RecyclePoolSampleApp.Pages
{
    public sealed partial class SelectTemplateArgsPage : Page, IScenarioPage
    {
        public SelectTemplateArgsPage()
        {
            this.InitializeComponent();

            m_items = SampleData.CreateMixed(200);
            m_stats = new FactoryStats();

            m_factory = new TrackingRecyclingElementFactory(m_stats)
            {
                RecyclePool = new RecyclePool(),
            };

            m_factory.Templates["Header"] = (DataTemplate)Resources["HeaderTemplate"];
            m_factory.Templates["Item"] = (DataTemplate)Resources["ItemTemplate"];
            m_factory.Templates["Footer"] = (DataTemplate)Resources["FooterTemplate"];
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
                case "reuse":
                    // scrolling forces many more calls, all of which land on the same args instance.
                    ScrollHelper.ScrollToWhenReady(Scroller, 2500, RefreshVisuals);
                    break;

                case "default":
                default:
                    Scroller.ChangeView(null, 0, null, true);
                    break;
            }

            RefreshVisuals();
        }

        private void OnSelectTemplateKey(RecyclingElementFactory sender, SelectTemplateEventArgs args)
        {
            var entry = args.DataContext as Entry;

            // TemplateKey arrives cleared on every call, so a handler that only sets it
            // conditionally will hit the "provide a valid template identifier" failure.
            string incoming = args.TemplateKey;
            args.TemplateKey = entry?.Kind ?? "Item";

            m_calls++;

            // RecyclingElementFactory reuses one SelectTemplateEventArgs for the lifetime of the
            // factory rather than allocating one per call, so the instance identity never changes.
            if (m_firstArgs == null)
            {
                m_firstArgs = args;
            }

            m_sameInstance = ReferenceEquals(m_firstArgs, args);

            ArgsText.Text =
                $"TemplateKey (on entry) : \"{incoming}\"" + Environment.NewLine +
                $"TemplateKey (on exit)  : \"{args.TemplateKey}\"" + Environment.NewLine +
                $"DataContext            : {entry?.Label ?? "null"}" + Environment.NewLine +
                $"DataContext type       : {args.DataContext?.GetType().Name ?? "null"}" + Environment.NewLine +
                $"Owner                  : {DescribeOwner(args.Owner)}";

            if (m_calls <= 40)
            {
                Log($"#{m_calls,-3} {entry?.Label,-12} -> \"{args.TemplateKey}\"  owner={DescribeOwner(args.Owner)}");
            }

            ResultText.Text = m_sameInstance
                ? $"The same SelectTemplateEventArgs instance has now served all {m_calls} calls.\nDo not cache it."
                : "A new args instance was handed to this call.";
        }

        private static string DescribeOwner(UIElement owner) => owner switch
        {
            null => "null",
            ItemsRepeater => "ItemsRepeater",
            FrameworkElement fe => fe.GetType().Name,
            _ => owner.GetType().Name,
        };

        private void OnScrollDown(object sender, RoutedEventArgs e)
            => Scroller.ChangeView(null, Scroller.VerticalOffset + 1200, null, false);

        private void OnClearLog(object sender, RoutedEventArgs e)
        {
            m_log.Clear();
            LogText.Text = string.Empty;
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
                $"SelectTemplateKey  : {m_calls}" + Environment.NewLine +
                $"Args instance      : {(m_sameInstance ? "reused" : "per call")}";
        }

        private readonly ObservableCollection<Entry> m_items;
        private readonly TrackingRecyclingElementFactory m_factory;
        private readonly FactoryStats m_stats;
        private readonly StringBuilder m_log = new StringBuilder();
        private SelectTemplateEventArgs m_firstArgs;
        private bool m_sameInstance = true;
        private int m_calls;
    }
}
