// The timeline core types (TelemetryEvent, PhaseDefinition, EventMatcher,
// TimelineItem, TimelineTrack, RegionKind, PayloadFilter, RegionsLoader,
// TimelineBuilder) now live in the shared XamlTimeline assembly. Importing the
// namespace globally keeps the rest of the viewer's source unchanged.
global using XamlTimeline;
