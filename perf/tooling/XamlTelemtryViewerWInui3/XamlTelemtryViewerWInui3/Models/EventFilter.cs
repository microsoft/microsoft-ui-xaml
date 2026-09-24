using System;
using System.Collections.ObjectModel;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using CommunityToolkit.Mvvm.ComponentModel;

namespace XamlTelemtryViewerWInui3.Models
{
    public sealed partial class EventFilter : ObservableObject
    {
        public ObservableCollection<FilterGroup> Groups { get; } = new();

        /// <summary>
        /// Evaluates all groups against the event using inter-group joins.
        /// </summary>
        public bool Matches(TelemetryEvent ev)
        {
            var activeGroups = Groups
                .Where(g => g.IsEnabled)
                .ToList();

            if (activeGroups.Count == 0)
            {
                return true;
            }

            var result = activeGroups[0].Matches(ev);
            for (var i = 1; i < activeGroups.Count; i++)
            {
                var group = activeGroups[i];
                var match = group.Matches(ev);
                result = group.JoinWithPrevious == FilterJoin.And
                    ? result && match
                    : result || match;
            }

            return result;
        }

        /// <summary>
        /// Serializes the filter to a human-readable query string with explicit parentheses around groups.
        /// Format: (groupConditions) AND/OR (groupConditions)
        /// Example: ([Field] op "value" AND [Field] op "value") AND ([Field] op "value" OR [Field] op "value")
        /// </summary>
        public string ToQueryText()
        {
            var sb = new StringBuilder();
            for (var gi = 0; gi < Groups.Count; gi++)
            {
                var group = Groups[gi];
                if (gi > 0)
                {
                    sb.AppendLine();
                    sb.AppendLine(group.JoinWithPrevious == FilterJoin.Or ? "OR" : "AND");
                }

                sb.Append(group.ToQueryText());
            }

            return sb.ToString();
        }

        /// <summary>
        /// Parses a query text string and replaces the current groups.
        /// Supports both formats:
        ///   1. With parentheses: ([Field] op "value") AND ([Field] op "value")
        ///   2. Legacy format: [Field] op "value" AND [Field] op "value"
        /// </summary>
        public void FromQueryText(string text)
        {
            Groups.Clear();
            if (string.IsNullOrWhiteSpace(text))
            {
                return;
            }

            // First, try to parse groups separated by standalone AND/OR on their own lines
            // This handles the explicit parentheses format
            var groupSeparator = new Regex(@"^\s*(AND|OR)\s*$", RegexOptions.Multiline | RegexOptions.IgnoreCase);
            var parts = groupSeparator.Split(text);

            // parts alternates: groupText, separator, groupText, separator, ...
            var groupJoin = FilterJoin.And;
            for (var i = 0; i < parts.Length; i++)
            {
                var part = parts[i].Trim();
                if (string.IsNullOrEmpty(part))
                {
                    continue;
                }

                if (string.Equals(part, "AND", StringComparison.OrdinalIgnoreCase))
                {
                    groupJoin = FilterJoin.And;
                    continue;
                }

                if (string.Equals(part, "OR", StringComparison.OrdinalIgnoreCase))
                {
                    groupJoin = FilterJoin.Or;
                    continue;
                }

                // Remove surrounding parentheses if present (for explicit bracketed format)
                var groupText = part.Trim();
                if (groupText.StartsWith("(") && groupText.EndsWith(")"))
                {
                    groupText = groupText.Substring(1, groupText.Length - 2).Trim();
                }

                var group = ParseGroup(groupText, groupJoin);
                if (group != null)
                {
                    Groups.Add(group);
                }

                groupJoin = FilterJoin.And;
            }
        }

        private static FilterGroup? ParseGroup(string text, FilterJoin groupJoin)
        {
            // Match conditions: [Field] op "value"
            var condPattern = new Regex(
                @"\[(\w+)\]\s*(~=|=|\^=|\$=)\s*""((?:[^""\\]|\\.)*)""",
                RegexOptions.IgnoreCase);

            // Find all conditions and the joins between them
            var matches = condPattern.Matches(text);
            if (matches.Count == 0)
            {
                return null;
            }

            var group = new FilterGroup { JoinWithPrevious = groupJoin };

            for (var i = 0; i < matches.Count; i++)
            {
                var m = matches[i];
                var fieldStr = m.Groups[1].Value;
                var opStr = m.Groups[2].Value;
                var value = UnescapeValue(m.Groups[3].Value);

                if (!Enum.TryParse<FilterField>(fieldStr, ignoreCase: true, out var field))
                {
                    continue;
                }

                var op = SymbolToOperator(opStr);
                var join = FilterJoin.And;

                if (i > 0)
                {
                    // Look at text between this match and the previous one
                    var prevEnd = matches[i - 1].Index + matches[i - 1].Length;
                    var between = text.Substring(prevEnd, m.Index - prevEnd).Trim();
                    if (between.EndsWith("OR", StringComparison.OrdinalIgnoreCase))
                    {
                        join = FilterJoin.Or;
                    }
                }

                group.Conditions.Add(new FilterCondition
                {
                    Field = field,
                    Operator = op,
                    Value = value,
                    JoinWithPrevious = join,
                    IsEnabled = true,
                });
            }

            return group.Conditions.Count > 0 ? group : null;
        }

        private static FilterOperator SymbolToOperator(string symbol) => symbol switch
        {
            "~=" => FilterOperator.Contains,
            "=" => FilterOperator.Equals,
            "^=" => FilterOperator.StartsWith,
            "$=" => FilterOperator.EndsWith,
            _ => FilterOperator.Contains,
        };

        private static string UnescapeValue(string value) =>
            value.Replace("\\\"", "\"").Replace("\\\\", "\\");
    }
}
