// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using System.Text;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;

namespace SelectionModelSampleApp.Pages
{
    /// <summary>
    /// A hierarchical item that is deliberately NOT a collection, so SelectionModel has to ask the
    /// app for its children through the ChildrenRequested event.
    /// </summary>
    public class Node : INotifyPropertyChanged
    {
        public Node(string name)
        {
            Name = name;
        }

        public string Name { get; }

        public ObservableCollection<Node> Children { get; } = new ObservableCollection<Node>();

        public bool? IsSelected
        {
            get => m_isSelected;
            set
            {
                if (m_isSelected != value)
                {
                    m_isSelected = value;
                    PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(IsSelected)));
                }
            }
        }

        public override string ToString() => Name;

        public event PropertyChangedEventHandler PropertyChanged;

        private bool? m_isSelected = false;
    }

    public sealed partial class EventsPage : Page, Common.IScenarioPage
    {
        public EventsPage()
        {
            this.InitializeComponent();

            m_roots = BuildTree();

            m_selectionModel = new SelectionModel();
            m_selectionModel.Source = m_roots;
            m_selectionModel.ChildrenRequested += OnChildrenRequested;
            m_selectionModel.SelectionChanged += OnSelectionChanged;

            RootControl.ItemsSource = m_roots;
            RefreshVisuals();
        }

        public void ApplyScenario(string scenario)
        {
            // Selecting a leaf three levels down forces the model to ask for children on the way,
            // which raises ChildrenRequested once per level.
            PathBox.Text = "1.0.2";
            m_selectionModel.SelectAt(IndexPath.CreateFromIndices(new List<int> { 1, 0, 2 }));
            RefreshVisuals();
        }

        private void OnChildrenRequested(SelectionModel sender, SelectionModelChildrenRequestedEventArgs args)
        {
            // Source and SourceIndex may only be read inside this handler.
            var node = args.Source as Node;
            Log($"ChildrenRequested  Source={node?.Name ?? "(null)"}  SourceIndex={args.SourceIndex}");

            // Assigning null would declare the node a leaf.
            args.Children = node?.Children;
        }

        private void OnSelectionChanged(SelectionModel sender, SelectionModelSelectionChangedEventArgs args)
        {
            // The args type is empty - everything has to be read back off the model.
            Log("SelectionChanged   (args carries no data)");
            RefreshVisuals();
        }

        private void OnSelectAt(object sender, RoutedEventArgs e)
        {
            var path = Parse(PathBox.Text);
            if (path != null)
            {
                m_selectionModel.SelectAt(path);
                RefreshVisuals();
            }
        }

        private void OnDeselectAt(object sender, RoutedEventArgs e)
        {
            var path = Parse(PathBox.Text);
            if (path != null)
            {
                m_selectionModel.DeselectAt(path);
                RefreshVisuals();
            }
        }

        private void OnClearSelection(object sender, RoutedEventArgs e) => m_selectionModel.ClearSelection();

        private void OnClearLog(object sender, RoutedEventArgs e)
        {
            m_log.Clear();
            LogText.Text = string.Empty;
        }

        private void Log(string message)
        {
            m_log.Insert(0, message);
            if (m_log.Count > 12)
            {
                m_log.RemoveAt(m_log.Count - 1);
            }

            LogText.Text = string.Join(Environment.NewLine, m_log);
        }

        private void RefreshVisuals()
        {
            for (int i = 0; i < m_roots.Count; i++)
            {
                var root = m_roots[i];
                root.IsSelected = m_selectionModel.IsSelectedAt(IndexPath.CreateFrom(i));

                for (int j = 0; j < root.Children.Count; j++)
                {
                    var child = root.Children[j];
                    child.IsSelected = m_selectionModel.IsSelectedAt(IndexPath.CreateFrom(i, j));

                    for (int k = 0; k < child.Children.Count; k++)
                    {
                        child.Children[k].IsSelected =
                            m_selectionModel.IsSelectedAt(IndexPath.CreateFromIndices(new List<int> { i, j, k }));
                    }
                }
            }

            StateText.Text = Common.SelectionState.Describe(m_selectionModel);
        }

        private static ObservableCollection<Node> BuildTree()
        {
            var roots = new ObservableCollection<Node>();
            for (int i = 0; i < 2; i++)
            {
                var root = new Node($"Node {i}");
                for (int j = 0; j < 2; j++)
                {
                    var child = new Node($"Node {i}.{j}");
                    for (int k = 0; k < 3; k++)
                    {
                        child.Children.Add(new Node($"Node {i}.{j}.{k}"));
                    }

                    root.Children.Add(child);
                }

                roots.Add(root);
            }

            return roots;
        }

        private static IndexPath Parse(string text)
        {
            try
            {
                var indices = text
                    .Split('.', StringSplitOptions.RemoveEmptyEntries)
                    .Select(int.Parse)
                    .ToList();
                return indices.Count == 0 ? null : IndexPath.CreateFromIndices(indices);
            }
            catch (Exception)
            {
                return null;
            }
        }

        private readonly ObservableCollection<Node> m_roots;
        private readonly SelectionModel m_selectionModel;
        private readonly List<string> m_log = new List<string>();
    }
}
