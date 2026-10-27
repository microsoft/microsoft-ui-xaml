// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Text;
using Microsoft.UI;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media;
using RecyclePoolSampleApp.Common;

namespace RecyclePoolSampleApp.Pages
{
    public sealed partial class RecyclePoolBasicsPage : Page, IScenarioPage
    {
        public RecyclePoolBasicsPage()
        {
            this.InitializeComponent();

            m_stats = new FactoryStats();
            m_pool = new ObservableRecyclePool(m_stats);
            m_pool.Changed += (s, e) => RefreshVisuals();

            RefreshVisuals();
        }

        public void ApplyScenario(string scenario)
        {
            ResetState();

            switch (scenario)
            {
                case "affinity":
                    // Park one element per owner under the same key, then ask for each owner back.
                    // The pool hands each owner the element that already belongs to it, so neither
                    // element has to be unparented and re-parented.
                    var a = CreateElement("Item");
                    var b = CreateElement("Item");
                    OwnerA.Children.Add(a);
                    OwnerB.Children.Add(b);
                    m_pool.Put(a, "Item", OwnerA);
                    m_pool.Put(b, "Item", OwnerB);

                    var forA = m_pool.TryGet("Item", OwnerA);
                    var forB = m_pool.TryGet("Item", OwnerB);
                    Log($"TryGetElement(\"Item\", OwnerA) -> {Describe(forA)}");
                    Log($"TryGetElement(\"Item\", OwnerB) -> {Describe(forB)}");
                    ResultText.Text =
                        $"Each owner got its own element back, and neither had to be reparented.\n" +
                        $"OwnerA element is still in : {ParentName(forA)}\n" +
                        $"OwnerB element is still in : {ParentName(forB)}";
                    break;

                case "keys":
                default:
                    // Keys partition the pool: an element parked under "Header" is invisible to a
                    // request for "Item", no matter how many Headers are waiting.
                    for (int i = 0; i < 3; i++)
                    {
                        m_pool.Put(CreateElement("Header"), "Header", OwnerA);
                    }

                    m_pool.Put(CreateElement("Footer"), "Footer", OwnerA);

                    var miss = m_pool.TryGet("Item", OwnerA);
                    var hit = m_pool.TryGet("Header", OwnerA);
                    if (hit != null)
                    {
                        Loose.Children.Add(hit);
                    }

                    Log("TryGetElement(\"Item\", OwnerA)   -> null");
                    Log($"TryGetElement(\"Header\", OwnerA) -> {Describe(hit)}");
                    ResultText.Text =
                        "Three \"Header\" elements are parked, but a request for \"Item\"\n" +
                        $"still returns null: keys never fall back to each other.\n\n" +
                        $"TryGetElement(\"Item\") -> {(miss == null ? "null" : "an element")}";
                    break;
            }

            RefreshVisuals();
        }

        private string CurrentKey => (KeyBox.SelectedItem as ComboBoxItem)?.Content as string ?? "Item";

        private UIElement CurrentOwner => (OwnerBox.SelectedIndex) switch
        {
            0 => OwnerA,
            1 => OwnerB,
            _ => null,
        };

        private void OnCreate(object sender, RoutedEventArgs e)
        {
            var element = CreateElement(CurrentKey);
            var owner = CurrentOwner as Panel ?? Loose;
            owner.Children.Add(element);
            Log($"new element \"{CurrentKey} #{m_created}\" -> {NameOf(owner)}");
            RefreshVisuals();
        }

        private void OnPut(object sender, RoutedEventArgs e)
        {
            var element = TakeVisibleElement();
            if (element == null)
            {
                ResultText.Text = "Create an element first.";
                return;
            }

            m_pool.Put(element, CurrentKey, CurrentOwner);
            Log($"PutElement(element, \"{CurrentKey}\", {NameOf(CurrentOwner)})");
            ResultText.Text =
                "The element stays in its owner's Children while it is parked. It is only removed\n" +
                "if some other owner later asks for it.";
        }

        private void OnPutNoOwner(object sender, RoutedEventArgs e)
        {
            var element = TakeVisibleElement();
            if (element == null)
            {
                ResultText.Text = "Create an element first.";
                return;
            }

            m_pool.Put(element, CurrentKey);
            Log($"PutElement(element, \"{CurrentKey}\")");
            ResultText.Text = "Parked with no owner. It is then equally available to every owner.";
        }

        private void OnTryGet(object sender, RoutedEventArgs e)
        {
            var element = m_pool.TryGet(CurrentKey, CurrentOwner);
            Log($"TryGetElement(\"{CurrentKey}\", {NameOf(CurrentOwner)}) -> {Describe(element)}");
            Show(element);
        }

        private void OnTryGetNoOwner(object sender, RoutedEventArgs e)
        {
            var element = m_pool.TryGet(CurrentKey);
            Log($"TryGetElement(\"{CurrentKey}\") -> {Describe(element)}");
            Show(element);
        }

        private void Show(UIElement element)
        {
            if (element == null)
            {
                ResultText.Text = "The pool had nothing under that key. Returned null.";
                RefreshVisuals();
                return;
            }

            ResultText.Text = $"Got an element back.\nIts parent is now : {ParentName(element)}";

            if ((element as FrameworkElement)?.Parent == null)
            {
                Loose.Children.Add(element);
            }

            RefreshVisuals();
        }

        private void OnReset(object sender, RoutedEventArgs e)
        {
            ResetState();
            RefreshVisuals();
        }

        private void ResetState()
        {
            OwnerA.Children.Clear();
            OwnerB.Children.Clear();
            Loose.Children.Clear();
            m_pool = new ObservableRecyclePool(m_stats);
            m_pool.Changed += (s, e) => RefreshVisuals();
            m_stats.Reset();
            m_created = 0;
            m_log.Clear();
            ResultText.Text = string.Empty;
        }

        private UIElement TakeVisibleElement()
        {
            foreach (var panel in new Panel[] { Loose, OwnerA, OwnerB })
            {
                if (panel.Children.Count > 0)
                {
                    return panel.Children[panel.Children.Count - 1];
                }
            }

            return null;
        }

        private FrameworkElement CreateElement(string kind)
        {
            m_created++;
            m_stats.OnCreated();
            return new Border
            {
                Padding = new Thickness(10, 6, 10, 6),
                CornerRadius = new CornerRadius(4),
                Background = new SolidColorBrush(kind switch
                {
                    "Header" => Colors.SteelBlue,
                    "Footer" => Colors.SeaGreen,
                    _ => Colors.SlateGray,
                }),
                Child = new TextBlock
                {
                    Text = $"{kind} #{m_created}",
                    Foreground = new SolidColorBrush(Colors.White),
                },
                Tag = $"{kind} #{m_created}",
            };
        }

        private static string Describe(UIElement element)
            => element == null ? "null" : ((element as FrameworkElement)?.Tag as string ?? "an element");

        private static string NameOf(UIElement element) => element switch
        {
            null => "null",
            Panel p when p.Name.Length > 0 => p.Name,
            _ => "element",
        };

        private string ParentName(UIElement element)
        {
            if (element == null)
            {
                return "n/a";
            }

            // Panel.Children is the authoritative answer here: FrameworkElement.Parent reports
            // the logical parent (null for children added in code) and the visual parent is only
            // established once the page has been laid out, which has not happened yet when a
            // screenshot scenario runs.
            foreach (var panel in new Panel[] { OwnerA, OwnerB, Loose })
            {
                if (panel.Children.IndexOf(element) >= 0)
                {
                    return panel.Name;
                }
            }

            return "null (detached)";
        }

        private void Log(string line)
        {
            m_log.Insert(0, line + Environment.NewLine);
            if (m_log.Length > 1200)
            {
                m_log.Length = 1200;
            }

            LogText.Text = m_log.ToString();
        }

        private void RefreshVisuals()
        {
            StateText.Text = m_stats.Describe();
            PoolText.Text = m_pool.DescribeContents();
        }

        private ObservableRecyclePool m_pool;
        private readonly FactoryStats m_stats;
        private readonly StringBuilder m_log = new StringBuilder();
        private int m_created;
    }
}
