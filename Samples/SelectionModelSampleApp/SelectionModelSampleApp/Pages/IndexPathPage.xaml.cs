// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;

namespace SelectionModelSampleApp.Pages
{
    public sealed partial class IndexPathPage : Page, Common.IScenarioPage
    {
        public IndexPathPage()
        {
            this.InitializeComponent();
            Evaluate();
        }

        public void ApplyScenario(string scenario) => Evaluate();

        private void OnEvaluate(object sender, RoutedEventArgs e) => Evaluate();

        private void Evaluate()
        {
            var builder = new StringBuilder();

            var flat = IndexPath.CreateFrom(4);
            builder.AppendLine("IndexPath.CreateFrom(4)");
            builder.AppendLine($"  ToString()  -> {flat}");
            builder.AppendLine($"  GetSize()   -> {flat.GetSize()}");
            builder.AppendLine($"  GetAt(0)    -> {flat.GetAt(0)}");
            builder.AppendLine();

            var grouped = IndexPath.CreateFrom(1, 2);
            builder.AppendLine("IndexPath.CreateFrom(1, 2)   // group 1, item 2");
            builder.AppendLine($"  ToString()  -> {grouped}");
            builder.AppendLine($"  GetSize()   -> {grouped.GetSize()}");
            builder.AppendLine($"  GetAt(0)    -> {grouped.GetAt(0)}   (group)");
            builder.AppendLine($"  GetAt(1)    -> {grouped.GetAt(1)}   (item)");
            builder.AppendLine();

            var deep = IndexPath.CreateFromIndices(new List<int> { 0, 3, 1 });
            builder.AppendLine("IndexPath.CreateFromIndices(new List<int> { 0, 3, 1 })");
            builder.AppendLine($"  ToString()  -> {deep}");
            builder.AppendLine($"  GetSize()   -> {deep.GetSize()}");
            builder.AppendLine();

            var root = IndexPath.CreateFromIndices(new List<int>());
            builder.AppendLine("IndexPath.CreateFromIndices(new List<int>())   // the root itself");
            builder.AppendLine($"  ToString()  -> {root}");
            builder.AppendLine($"  GetSize()   -> {root.GetSize()}");
            builder.AppendLine();

            builder.AppendLine("CompareTo returns -1, 0 or 1. Paths sort depth-first; when one path is a");
            builder.AppendLine("prefix of the other, the shorter path sorts first.");
            builder.AppendLine();

            var a = Parse(PathABox.Text);
            var b = Parse(PathBBox.Text);
            if (a == null || b == null)
            {
                builder.AppendLine("Enter dot separated indices, for example 1.2");
            }
            else
            {
                builder.AppendLine($"  a = {a}, b = {b}");
                builder.AppendLine($"  a.CompareTo(b) -> {a.CompareTo(b)}");
                builder.AppendLine($"  b.CompareTo(a) -> {b.CompareTo(a)}");
                builder.AppendLine($"  a.CompareTo(a) -> {a.CompareTo(a)}");
            }

            OutputText.Text = builder.ToString();
        }

        private static IndexPath Parse(string text)
        {
            try
            {
                var indices = text
                    .Split('.', StringSplitOptions.RemoveEmptyEntries)
                    .Select(int.Parse)
                    .ToList();
                return IndexPath.CreateFromIndices(indices);
            }
            catch (Exception)
            {
                return null;
            }
        }
    }
}
