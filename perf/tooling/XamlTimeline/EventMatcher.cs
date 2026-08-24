using System;

namespace XamlTimeline
{
    /// <summary>
    /// Matches a <see cref="TelemetryEvent"/> by provider GUID and (optionally) event id / name.
    /// Provider GUID is the primary key — the provider's friendly name varies between traces.
    /// <see cref="Payload"/> is an optional payload-field filter for this endpoint. Its
    /// <c>${ProcessName}</c> token is substituted at evaluation time, so callers that want
    /// the payload applied must use <see cref="Matches(TelemetryEvent, string?)"/> and pass
    /// the resolved process-name token. The parameterless
    /// <see cref="Matches(TelemetryEvent)"/> overload is a cheap signature-only check that
    /// deliberately ignores <see cref="Payload"/> (payload needs process context to resolve).
    /// </summary>
    public sealed record EventMatcher(
        Guid ProviderGuid,
        int? EventId = null,
        string? EventName = null,
        bool XamlEndHeuristic = false,
        bool ProcessDependentEnd = false,
        PayloadFilter? Payload = null)
    {
        private const string ProcessNamePlaceholder = "${ProcessName}";

        /// <summary>
        /// Signature-only match (provider GUID + optional event id / name). Does NOT apply
        /// the <see cref="Payload"/> filter — use it as a cheap pre-filter, then call
        /// <see cref="Matches(TelemetryEvent, string?)"/> to also verify the payload.
        /// </summary>
        public bool Matches(TelemetryEvent ev)
        {
            if (ev.ProviderGuid != ProviderGuid)
            {
                return false;
            }

            if (EventId.HasValue && ev.EventId != EventId.Value)
            {
                return false;
            }

            if (!string.IsNullOrEmpty(EventName) &&
                ev.EventName.IndexOf(EventName, StringComparison.OrdinalIgnoreCase) < 0)
            {
                return false;
            }

            return true;
        }

        /// <summary>
        /// Full match: the signature check of <see cref="Matches(TelemetryEvent)"/> AND, when
        /// <see cref="Payload"/> is set, the payload-field filter. The filter's
        /// <c>${ProcessName}</c> token is replaced with <paramref name="processNameToken"/>
        /// before comparison, so an event only matches when its payload also matches.
        /// </summary>
        /// <param name="ev">The event to test.</param>
        /// <param name="processNameToken">
        /// The resolved process-name token substituted into the payload value's
        /// <c>${ProcessName}</c> placeholder (may be empty; null is treated as empty).
        /// </param>
        public bool Matches(TelemetryEvent ev, string? processNameToken)
        {
            if (!Matches(ev))
            {
                return false;
            }

            if (Payload != null)
            {
                var value = Payload.Value.Replace(ProcessNamePlaceholder, processNameToken ?? string.Empty);
                if (!PayloadContains(ev, Payload.Field, value))
                {
                    return false;
                }
            }

            return true;
        }

        private static bool PayloadContains(TelemetryEvent ev, string field, string value)
        {
            if (string.IsNullOrEmpty(value) || string.IsNullOrEmpty(field))
            {
                return false;
            }

            // Direct field lookup against the parsed payload dictionary.
            return ev.Fields.TryGetValue(field, out var fieldValue)
                && ContainsIgnoringChars(fieldValue, value);
        }

        // Character ignored on both sides when matching a payload value against a
        // region's filter value. ETW payloads don't always spell the app id the same
        // way the image name does (e.g. image "BlankWinuiApp" vs payload
        // "appId=blank_winui_app"), so we skip this separator during comparison.
        private const string PayloadMatchIgnoreChar = "_";

        /// <summary>
        /// Case-insensitive substring match that skips the ignored character on both
        /// the payload value and the filter value.
        /// </summary>
        private static bool ContainsIgnoringChars(string haystack, string needle)
        {
            var cleanNeedle = needle.Replace(PayloadMatchIgnoreChar, string.Empty).ToLowerInvariant();
            if (cleanNeedle.Length == 0)
            {
                return false;
            }

            var cleanHaystack = haystack.Replace(PayloadMatchIgnoreChar, string.Empty).ToLowerInvariant();
            return cleanHaystack.Contains(cleanNeedle);
        }
    }
}
