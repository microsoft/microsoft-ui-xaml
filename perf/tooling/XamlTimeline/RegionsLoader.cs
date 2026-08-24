using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Xml.Linq;

namespace XamlTimeline
{
    /// <summary>
    /// Reads the regions definition file shared by both apps. The WinUI viewer loads
    /// region files from disk via <see cref="Load(string)"/>; the WPA plugin reads its
    /// embedded resource and passes the stream to <see cref="Load(Stream)"/>.
    /// </summary>
    /// <remarks>
    /// <para>
    /// The on-disk format is the standard WPA "Regions of Interest" envelope, so the
    /// same file can be loaded directly in Windows Performance Analyzer
    /// (Trace &gt; Trace Properties &gt; Add). Tool-only data WPA has no concept of
    /// (track, marker-vs-phase, colour, special-provider allow-lists, the XAML end
    /// heuristic, etc.) is carried in the standard WPA <c>&lt;Metadata&gt;</c> node, which WPA
    /// ignores for processing and merely surfaces in its UI.
    /// <code>
    /// &lt;InstrumentationManifest&gt;
    ///   &lt;Instrumentation&gt;&lt;Regions&gt;
    ///     &lt;RegionRoot Guid="{...}" Name="..." FriendlyName="..."&gt;
    ///       &lt;Region Guid="{...}" Name="UniqueId" FriendlyName="Display name"&gt;
    ///         &lt;Start&gt;
    ///           &lt;Event Provider="{guid}" Id="31" Version="0" Name="..." /&gt;
    ///           &lt;PayloadIdentifier FieldName="..." FieldValue="..." /&gt;   &lt;!-- optional --&gt;
    ///         &lt;/Start&gt;
    ///         &lt;Stop&gt;                                                       &lt;!-- omitted for Markers --&gt;
    ///           &lt;Event Provider="{guid}" Name="..." /&gt;
    ///           &lt;PayloadIdentifier FieldName="..." FieldValue="..." /&gt;
    ///         &lt;/Stop&gt;
    ///         &lt;Metadata&gt;
    ///           &lt;Track&gt;1&lt;/Track&gt; &lt;Kind&gt;Phase&lt;/Kind&gt; &lt;Color&gt;#RRGGBB&lt;/Color&gt;
    ///           &lt;ProcessDependent&gt;true&lt;/ProcessDependent&gt; &lt;XamlEndHeuristic&gt;true&lt;/XamlEndHeuristic&gt;
    ///           &lt;SpecialProviders&gt;ProviderA;ProviderB&lt;/SpecialProviders&gt;
    ///         &lt;/Metadata&gt;
    ///       &lt;/Region&gt;
    ///     &lt;/RegionRoot&gt;
    ///   &lt;/Regions&gt;&lt;/Instrumentation&gt;
    /// &lt;/InstrumentationManifest&gt;
    /// </code>
    /// </para>
    /// <para>
    /// <c>${ProcessName}</c> in a payload value is substituted at evaluation time with the
    /// selected process's image name (case-insensitive contains match).
    /// </para>
    /// </remarks>
    public sealed class RegionsLoader
    {
        /// <summary>Loads and parses a regions file from disk.</summary>
        public IReadOnlyList<PhaseDefinition> Load(string path)
        {
            if (!File.Exists(path))
            {
                throw new FileNotFoundException("Regions file not found.", path);
            }

            return Parse(XDocument.Load(path));
        }

        /// <summary>Loads and parses regions from an open stream.</summary>
        public IReadOnlyList<PhaseDefinition> Load(Stream stream)
        {
            if (stream is null)
            {
                throw new ArgumentNullException(nameof(stream));
            }

            return Parse(XDocument.Load(stream));
        }

        /// <summary>
        /// Parses regions from a raw XML string. This cannot be an overload of
        /// <see cref="Load(string)"/> — both a file path and XML are strings, so the
        /// overload would be ambiguous; the distinct name disambiguates intent.
        /// </summary>
        public IReadOnlyList<PhaseDefinition> LoadFromString(string xml)
        {
            return Parse(XDocument.Parse(xml));
        }

        /// <summary>
        /// Loads the canonical <c>XamlAppLaunch.regions.xml</c> embedded in this shared
        /// assembly. This is the single source of region definitions used by both the
        /// WinUI viewer and the WPA plugin.
        /// </summary>
        public IReadOnlyList<PhaseDefinition> LoadEmbeddedDefault()
        {
            const string resourceName = "XamlTimeline.Regions.XamlAppLaunch.regions.xml";
            var assembly = typeof(RegionsLoader).Assembly;
            using (var stream = assembly.GetManifestResourceStream(resourceName))
            {
                if (stream is null)
                {
                    throw new FileNotFoundException("Embedded regions resource not found: " + resourceName);
                }

                return Load(stream);
            }
        }

        private static IReadOnlyList<PhaseDefinition> Parse(XDocument doc)
        {
            var root = doc.Root
                ?? throw new InvalidDataException("Regions XML has no root element.");

            var result = new List<PhaseDefinition>();
            foreach (var regionElement in EnumerateRegionElements(root))
            {
                result.Add(ParseRegion(regionElement));
            }

            return result;
        }

        /// <summary>
        /// Yields every <c>&lt;Region&gt;</c> element under the WPA
        /// <c>InstrumentationManifest/Instrumentation/Regions/RegionRoot</c> envelope
        /// (descendants, to also catch nested regions).
        /// </summary>
        private static IEnumerable<XElement> EnumerateRegionElements(XElement root)
        {
            if (!string.Equals(root.Name.LocalName, "InstrumentationManifest", StringComparison.OrdinalIgnoreCase))
            {
                throw new InvalidDataException(
                    "Regions XML root must be <InstrumentationManifest> (WPA Regions of Interest format).");
            }

            var ns = root.Name.Namespace;
            var regionRoot = root.Element(ns + "Instrumentation")
                ?.Element(ns + "Regions")
                ?.Element(ns + "RegionRoot");

            if (regionRoot is null)
            {
                throw new InvalidDataException(
                    "Regions XML has an <InstrumentationManifest> root but no <Instrumentation>/<Regions>/<RegionRoot>.");
            }

            return regionRoot.Descendants(ns + "Region");
        }

        private static PhaseDefinition ParseRegion(XElement element)
        {
            // WPA: Name is the unique id, FriendlyName is the human label.
            var name = ExtAttr(element, "FriendlyName")
                ?? ExtAttr(element, "Name")
                ?? throw new InvalidDataException("<Region> is missing both 'Name' and 'FriendlyName'.");

            var color = ExtAttr(element, "Color") ?? "#607D8B";
            var trackValue = ExtAttr(element, "Track") ?? "1";
            var kindValue = ExtAttr(element, "Kind") ?? "Marker";

            var track = trackValue == "2" ? TimelineTrack.Process : TimelineTrack.Trace;
            var kind = string.Equals(kindValue, "Phase", StringComparison.OrdinalIgnoreCase)
                ? RegionKind.Phase
                : RegionKind.Marker;

            var startElement = StartOrStop(element, "Start")
                ?? throw new InvalidDataException($"Region '{name}' is missing <Start>.");
            var start = ParseMatcher(startElement);

            EventMatcher? stop = null;
            var stopElement = StartOrStop(element, "Stop");
            if (stopElement is not null)
            {
                stop = ParseMatcher(stopElement);
            }

            var startPayload = ParsePayload(startElement);
            var stopPayload = stopElement is not null
                ? ParsePayload(stopElement)
                : null;

            // Fold each payload filter into its own matcher.
            if (startPayload is not null) start = start with { Payload = startPayload };
            if (stopPayload is not null && stop is not null) stop = stop with { Payload = stopPayload };

            // Stop-event heuristics are carried in the region <Metadata>.
            var xamlEndHeuristic = ParseBool(ExtAttr(element, "XamlEndHeuristic"));
            var processDependentEnd = ParseBool(ExtAttr(element, "ProcessDependentEnd"));
            if (stop is not null && (xamlEndHeuristic || processDependentEnd))
            {
                stop = stop with
                {
                    XamlEndHeuristic = xamlEndHeuristic,
                    ProcessDependentEnd = processDependentEnd,
                };
            }

            var parent = ExtAttr(element, "Parent");
            var specialProviders = ParseSpecialProviders(element);

            double thresholdPercentage = 5.0;  // default
            var thresholdAttr = ExtAttr(element, "ThresholdPercentage");
            if (thresholdAttr is not null && double.TryParse(thresholdAttr, out var parsedThresholdAttr))
            {
                thresholdPercentage = parsedThresholdAttr;
            }

            bool processDependent = false;
            var processDependentAttr = ExtAttr(element, "ProcessDependent");
            if (processDependentAttr is not null && bool.TryParse(processDependentAttr, out var parsedProcessDependent))
            {
                processDependent = parsedProcessDependent;
            }

            if (kind == RegionKind.Phase && stop is null)
            {
                throw new InvalidDataException(
                    $"Region '{name}' is Kind=\"Phase\" but has no <Stop> element.");
            }

            return new PhaseDefinition
            {
                Name = name,
                Color = color,
                Track = track,
                Kind = kind,
                Start = start,
                Stop = stop,
                Parent = parent,
                SpecialProviders = specialProviders,
                ThresholdPercentage = thresholdPercentage,
                ProcessDependent = processDependent,
            };
        }

        /// <summary>
        /// Returns the <c>&lt;Start&gt;</c> / <c>&lt;Stop&gt;</c> endpoint element, honouring the
        /// region's own namespace or no namespace.
        /// </summary>
        private static XElement? StartOrStop(XElement region, string localName)
        {
            return region.Element(region.Name.Namespace + localName)
                ?? region.Element(localName);
        }

        private static EventMatcher ParseMatcher(XElement endpoint)
        {
            // WPA wraps the matcher in an <Event> child of <Start>/<Stop>.
            var matcher = endpoint.Element(endpoint.Name.Namespace + "Event")
                ?? endpoint.Element("Event")
                ?? throw new InvalidDataException(
                    $"<{endpoint.Name.LocalName}> is missing its <Event> child.");

            var providerString = RequiredAttr(matcher, "Provider");
            var providerGuid = ParseGuid(providerString)
                ?? throw new InvalidDataException(
                    $"Provider attribute '{providerString}' is not a valid GUID.");

            int? id = null;
            var idAttr = matcher.Attribute("Id");
            if (idAttr is not null && int.TryParse(idAttr.Value, out var parsed))
            {
                id = parsed;
            }

            var eventName = OptionalAttr(matcher, "Name");

            // Stop-event heuristics (XamlEndHeuristic / ProcessDependentEnd) live in the
            // region's <Metadata> node and are applied by ParseRegion.
            return new EventMatcher(providerGuid, id, eventName);
        }

        /// <summary>
        /// Reads a payload filter from the WPA <c>&lt;PayloadIdentifier FieldName FieldValue&gt;</c>
        /// child of the <c>&lt;Start&gt;</c>/<c>&lt;Stop&gt;</c> endpoint, if present.
        /// </summary>
        private static PayloadFilter? ParsePayload(XElement endpoint)
        {
            var payloadIdentifier = endpoint.Element(endpoint.Name.Namespace + "PayloadIdentifier")
                ?? endpoint.Element("PayloadIdentifier");
            if (payloadIdentifier is not null)
            {
                return new PayloadFilter(
                    Field: RequiredAttr(payloadIdentifier, "FieldName"),
                    Value: RequiredAttr(payloadIdentifier, "FieldValue"));
            }

            return null;
        }

        /// <summary>
        /// Reads the special-provider allow-list from the <c>&lt;SpecialProviders&gt;</c>
        /// <c>&lt;Metadata&gt;</c> child: a semicolon-separated provider list.
        /// </summary>
        private static List<string> ParseSpecialProviders(XElement element)
        {
            var attr = ExtAttr(element, "SpecialProviders");
            if (string.IsNullOrWhiteSpace(attr))
            {
                return new List<string>();
            }

            return attr!
                .Split(new[] { ';' }, StringSplitOptions.RemoveEmptyEntries)
                .Select(p => p.Trim())
                .Where(p => p.Length > 0)
                .ToList();
        }

        private static Guid? ParseGuid(string value)
        {
            var trimmed = value.Trim().TrimStart('{').TrimEnd('}');
            return Guid.TryParse(trimmed, out var g) ? g : (Guid?)null;
        }

        private static bool ParseBool(string? s) => s is not null && bool.TryParse(s, out var b) && b;

        private static string RequiredAttr(XElement element, string name)
        {
            var attr = element.Attribute(name)
                ?? throw new InvalidDataException(
                    $"<{element.Name.LocalName}> is missing required attribute '{name}'.");
            return attr.Value;
        }

        private static string? OptionalAttr(XElement element, string name)
            => element.Attribute(name)?.Value;

        /// <summary>
        /// Reads a region property from its <c>&lt;Metadata&gt;</c> child element (the standard
        /// WPA metadata node) if present, else from a plain attribute on the element (used by
        /// the <c>Name</c>/<c>FriendlyName</c> attributes WPA puts directly on <c>&lt;Region&gt;</c>).
        /// </summary>
        private static string? ExtAttr(XElement element, string name)
        {
            // <Metadata><Name>value</Name></Metadata> (standard WPA, namespace-agnostic).
            var metadata = element.Element(element.Name.Namespace + "Metadata")
                ?? element.Element("Metadata");
            var meta = metadata?.Elements().FirstOrDefault(e =>
                string.Equals(e.Name.LocalName, name, StringComparison.OrdinalIgnoreCase));
            if (meta is not null) return meta.Value;

            return element.Attribute(name)?.Value;
        }
    }
}
