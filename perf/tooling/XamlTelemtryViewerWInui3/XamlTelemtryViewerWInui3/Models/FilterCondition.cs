using System;
using CommunityToolkit.Mvvm.ComponentModel;

namespace XamlTelemtryViewerWInui3.Models
{
    public enum FilterField
    {
        ProcessName,
        ProcessId,
        ProviderName,
        EventName,
        Level,
        Opcode,
        ThreadId,
        Payload,
    }

    public enum FilterOperator
    {
        Contains,
        Equals,
        StartsWith,
        EndsWith,
    }

    public enum FilterJoin
    {
        And,
        Or,
    }

    public sealed partial class FilterCondition : ObservableObject
    {
        public FilterCondition()
        {
        }

        [ObservableProperty]
        public partial bool IsEnabled { get; set; } = true;

        [ObservableProperty]
        public partial FilterJoin JoinWithPrevious { get; set; } = FilterJoin.And;

        [ObservableProperty]
        public partial FilterField Field { get; set; } = FilterField.ProcessName;

        [ObservableProperty]
        public partial FilterOperator Operator { get; set; } = FilterOperator.Contains;

        [ObservableProperty]
        public partial string Value { get; set; } = string.Empty;

        public bool IsMatch(TelemetryEvent ev)
        {
            if (!IsEnabled || string.IsNullOrWhiteSpace(Value))
            {
                return true;
            }

            var comparison = StringComparison.OrdinalIgnoreCase;
            var target = GetTarget(ev);
            if (target == null)
            {
                return false;
            }

            return Operator switch
            {
                FilterOperator.Contains => target.IndexOf(Value, comparison) >= 0,
                FilterOperator.Equals => string.Equals(target, Value, comparison),
                FilterOperator.StartsWith => target.StartsWith(Value, comparison),
                FilterOperator.EndsWith => target.EndsWith(Value, comparison),
                _ => false,
            };
        }

        private string? GetTarget(TelemetryEvent ev)
        {
            return Field switch
            {
                FilterField.ProcessName => ev.ProcessName,
                FilterField.ProcessId => ev.ProcessId.ToString(),
                FilterField.ProviderName => ev.ProviderName,
                FilterField.EventName => ev.EventName,
                FilterField.Level => ev.Level,
                FilterField.Opcode => ev.Opcode.ToString(),
                FilterField.ThreadId => ev.ThreadId.ToString(),
                FilterField.Payload => ev.PayloadText,
                _ => null,
            };
        }

        /// <summary>
        /// Serializes this condition to a query fragment: <c>[Field] op "value"</c>.
        /// </summary>
        public string ToQueryText() =>
            $"[{Field}] {OperatorToSymbol(Operator)} \"{EscapeValue(Value)}\"";

        private static string OperatorToSymbol(FilterOperator op) => op switch
        {
            FilterOperator.Contains => "~=",
            FilterOperator.Equals => "=",
            FilterOperator.StartsWith => "^=",
            FilterOperator.EndsWith => "$=",
            _ => "~=",
        };

        private static string EscapeValue(string value) =>
            value.Replace("\\", "\\\\").Replace("\"", "\\\"");
    }
}
