// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System.Linq;
using System.Text;
using Microsoft.UI.Xaml.Controls;

namespace SelectionModelSampleApp.Common
{
    /// <summary>
    /// Formats the observable state of a SelectionModel so every screenshot in the API spec
    /// shows exactly what the model reports after an operation.
    /// </summary>
    public static class SelectionState
    {
        public static string Describe(SelectionModel model)
        {
            var builder = new StringBuilder();
            builder.AppendLine($"SelectedIndex   : {Format(model.SelectedIndex)}");
            builder.AppendLine($"SelectedItem    : {model.SelectedItem ?? "(null)"}");
            builder.AppendLine($"AnchorIndex     : {Format(model.AnchorIndex)}");
            builder.AppendLine($"SingleSelect    : {model.SingleSelect}");
            builder.AppendLine($"SelectedIndices : {Indices(model)}");
            builder.Append($"SelectedItems   : {FormatItems(model)}");
            return builder.ToString();
        }

        public static string Format(IndexPath path) => path == null ? "(null)" : path.ToString();

        public static string Format(bool? isSelected) => isSelected switch
        {
            true => "true (selected)",
            false => "false (not selected)",
            _ => "null (partially selected)",
        };

        public static string Indices(SelectionModel model)
        {
            var indices = model.SelectedIndices;
            if (indices == null || indices.Count == 0)
            {
                return "(empty)";
            }

            return string.Join(", ", indices.Select(i => i.ToString()));
        }

        private static string FormatItems(SelectionModel model)
        {
            var items = model.SelectedItems;
            if (items == null || items.Count == 0)
            {
                return "(empty)";
            }

            return string.Join(", ", items.Select(i => i?.ToString() ?? "(null)"));
        }
    }
}
