using System.Collections.ObjectModel;
using System.Linq;
using System.Text;
using CommunityToolkit.Mvvm.ComponentModel;

namespace XamlTelemtryViewerWInui3.Models
{
    /// <summary>
    /// A group of filter conditions evaluated together.
    /// Conditions within a group are joined by their individual <see cref="FilterCondition.JoinWithPrevious"/>.
    /// Groups themselves are joined by <see cref="JoinWithPrevious"/>.
    /// </summary>
    public sealed partial class FilterGroup : ObservableObject
    {
        public FilterGroup()
        {
        }

        [ObservableProperty]
        public partial FilterJoin JoinWithPrevious { get; set; } = FilterJoin.And;

        [ObservableProperty]
        public partial bool IsEnabled { get; set; } = true;

        public ObservableCollection<FilterCondition> Conditions { get; } = new();

        /// <summary>
        /// Evaluates all active conditions in this group against the event.
        /// Returns true if the group has no active conditions.
        /// </summary>
        public bool Matches(TelemetryEvent ev)
        {
            if (!IsEnabled)
            {
                return true;
            }

            var active = Conditions
                .Where(c => c.IsEnabled && !string.IsNullOrWhiteSpace(c.Value))
                .ToList();

            if (active.Count == 0)
            {
                return true;
            }

            var result = active[0].IsMatch(ev);
            for (var i = 1; i < active.Count; i++)
            {
                var match = active[i].IsMatch(ev);
                result = active[i].JoinWithPrevious == FilterJoin.And
                    ? result && match
                    : result || match;
            }

            return result;
        }

        /// <summary>
        /// Serializes this group's conditions to a parenthesized query string, e.g.
        /// <c>([Field] op "value" AND [Field] op "value")</c>. Conditions are joined by
        /// their individual <see cref="FilterCondition.JoinWithPrevious"/>.
        /// </summary>
        public string ToQueryText()
        {
            var sb = new StringBuilder();
            sb.Append("(");

            for (var ci = 0; ci < Conditions.Count; ci++)
            {
                var c = Conditions[ci];
                if (ci > 0)
                {
                    sb.Append(c.JoinWithPrevious == FilterJoin.Or ? " OR " : " AND ");
                }

                sb.Append(c.ToQueryText());
            }

            sb.Append(")");
            return sb.ToString();
        }
    }
}
