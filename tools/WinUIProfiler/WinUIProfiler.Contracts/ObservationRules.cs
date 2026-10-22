namespace WinUIProfiler.Contracts;

public static class ObservationRules
{
    public static void Classification(IdentityDomain domain, XamlTypeClassification? classification)
    {
        if (classification is null) return;
        classification.Validate();
        if (domain == IdentityDomain.XamlCore && classification.Kind == XamlObjectKind.NotApplicable ||
            domain is not (IdentityDomain.XamlCore or IdentityDomain.XamlPeer) && classification.Kind is XamlObjectKind.UIElement or XamlObjectKind.DependencyObject)
            throw new ArgumentException("Authoritative XAML type evidence does not match the object's identity domain.");
    }

    public static bool IsGpuMemory(MetricKind metric) => metric is MetricKind.GpuDedicatedCommittedBytes or MetricKind.GpuSharedCommittedBytes or MetricKind.GpuDedicatedResidentBytes or MetricKind.GpuSharedResidentBytes;

    public static void Metric(MetricKind metric, TypedValue value, TimeRange interval, string source, string sourceVersion, GpuSampleIdentity? gpu)
    {
        Require.Enum(metric); Require.NotNull(value).Validate(); Require.Text(source); Require.Text(sourceVersion);
        var unit = metric switch
        {
            MetricKind.CpuUtilization or MetricKind.GpuUtilization => ValueUnit.Percent,
            MetricKind.ConnectedXamlElements or MetricKind.CanonicalCompositionVisuals => ValueUnit.Count,
            _ => ValueUnit.Bytes
        };
        if (value.Unit != unit || value.Kind is not (ValueKind.Integer or ValueKind.Number or ValueKind.Missing) ||
            value.Integer is < 0 || value.Number is < 0 ||
            unit == ValueUnit.Count && value.Kind == ValueKind.Number ||
            unit == ValueUnit.Percent && (value.Integer is > 100 || value.Number is > 100))
            throw new ArgumentException("Metric units, discriminator or value are invalid.");
        var isGpu = metric == MetricKind.GpuUtilization || IsGpuMemory(metric);
        if (!isGpu && gpu is not null || isGpu && value.Kind != ValueKind.Missing && gpu is null)
            throw new ArgumentException("Measured GPU samples require typed GPU scope; non-GPU samples cannot claim GPU identity.");
        gpu?.Validate(metric);
        if (gpu is { Adapter: null } && value.Kind != ValueKind.Missing && value.Quality.Coverage == CoverageState.Complete)
            throw new ArgumentException("Unknown winning adapter identity requires explicit partial coverage.");
    }

    public static void Property(XamlPropertyKey property, TypedValue previous, TypedValue current)
    {
        Require.NotNull(property); Require.Positive(property.PropertyId); Require.Text(property.DeclaringType); Require.Text(property.Name);
        Require.NotNull(previous).Validate(); Require.NotNull(current).Validate();
        if (previous.Unit != current.Unit) throw new ArgumentException("Property changes retain the declared property's units, including missing values.");
    }

    public static void Visual(VisualProperty property, TypedValue value)
    {
        Require.Enum(property); Require.NotNull(value).Validate();
        var unit = property switch
        {
            VisualProperty.IsVisible => ValueUnit.Boolean,
            VisualProperty.OffsetX or VisualProperty.OffsetY or VisualProperty.Width or VisualProperty.Height => ValueUnit.Pixels,
            _ => ValueUnit.Scalar
        };
        if (value.Unit != unit ||
            property == VisualProperty.IsVisible && value.Kind is not (ValueKind.Boolean or ValueKind.Missing) ||
            property != VisualProperty.IsVisible && value.Kind is not (ValueKind.Integer or ValueKind.Number or ValueKind.Missing) ||
            property == VisualProperty.Opacity && (value.Integer is < 0 or > 1 || value.Number is < 0 or > 1) ||
            property is VisualProperty.Width or VisualProperty.Height && (value.Integer is < 0 || value.Number is < 0))
            throw new ArgumentException("Visual property units, discriminator or value are invalid.");
    }

    public static void Tree(ObjectKey child, ObjectKey? parent, RelationshipKind relationship, int order, Guid rootId, ulong dispatcherId, TreeMembershipKind membership)
    {
        child.Validate(); Require.Enum(relationship); Require.Enum(membership); Require.Id(rootId); Require.Positive(dispatcherId);
        var root = relationship is RelationshipKind.Root or RelationshipKind.CompRoot or RelationshipKind.VisualRoot;
        if (order < 0 || parent == child || root && parent is not null || !root && parent is null)
            throw new ArgumentException("Tree parent/order/root relationship is invalid.");
        var domain = relationship switch
        {
            RelationshipKind.CompRoot or RelationshipKind.CompChild => IdentityDomain.CompNode,
            RelationshipKind.VisualRoot or RelationshipKind.VisualChild => IdentityDomain.CompositionVisual,
            _ => IdentityDomain.XamlCore
        };
        if (child.Domain != domain || parent is { } p && (p.Domain != domain || p.Process != child.Process || p.ProducerEpoch != child.ProducerEpoch))
            throw new ArgumentException("Tree identities must share process, producer epoch and declared domain.");
        if (domain == IdentityDomain.XamlCore && membership == TreeMembershipKind.Composition ||
            domain != IdentityDomain.XamlCore && membership is not (TreeMembershipKind.Composition or TreeMembershipKind.Unknown))
            throw new ArgumentException("Logical/visual XAML membership is distinct from composition membership.");
        parent?.Validate();
    }

    public static void Correlation(ObjectKey from, ObjectKey to, CorrelationKind kind)
    {
        from.Validate(); to.Validate(); Require.Enum(kind);
        var domains = kind switch
        {
            CorrelationKind.CorePeer => (IdentityDomain.XamlCore, IdentityDomain.XamlPeer),
            CorrelationKind.XamlCompNode => (IdentityDomain.XamlCore, IdentityDomain.CompNode),
            CorrelationKind.CompNodeVisual => (IdentityDomain.CompNode, IdentityDomain.CompositionVisual),
            CorrelationKind.XamlVisual => (IdentityDomain.XamlCore, IdentityDomain.CompositionVisual),
            CorrelationKind.VisualResource => (IdentityDomain.CompositionVisual, IdentityDomain.Resource),
            CorrelationKind.ResourceBacking => (IdentityDomain.Resource, IdentityDomain.Resource),
            _ => throw new ArgumentException("Unsupported correlation kind.")
        };
        if (from.Process != to.Process || from.ProducerEpoch != to.ProducerEpoch || from.Domain != domains.Item1 || to.Domain != domains.Item2)
            throw new ArgumentException("Correlation identity domains/process/epochs do not match.");
    }

    public static void Allocation(ulong id, AllocationOperation operation, ulong? previous, ulong requested, ulong? actual, Guid epoch)
    {
        Require.Positive(id); Require.Enum(operation); Require.Id(epoch);
        if (operation == AllocationOperation.Reallocate && (previous is null or 0 || previous == id) ||
            operation != AllocationOperation.Reallocate && previous is not null ||
            operation != AllocationOperation.Free && actual < requested)
            throw new ArgumentException("Allocation lifetime/predecessor/requested/actual bytes are invalid.");
    }

    public static void Provenance(SizeProvenance provenance, TypedValue size, Guid? backing, DataQuality quality)
    {
        Require.Enum(provenance); ValueRules.NonnegativeBytes(size); Require.NotNull(quality);
        if (backing is { } id) Require.Id(id);
        if (provenance == SizeProvenance.Unknown && size.Kind != ValueKind.Missing ||
            provenance != SizeProvenance.Unknown && size.Kind == ValueKind.Missing ||
            provenance == SizeProvenance.ActualAllocation && (backing is null || size.Quality.Kind != MeasurementKind.Measured || quality.Kind != MeasurementKind.Measured) ||
            provenance == SizeProvenance.ApiDescriptionEstimate && (size.Quality.Kind != MeasurementKind.Estimated || quality.Kind != MeasurementKind.Estimated))
            throw new ArgumentException("Resource byte provenance cannot invent measurement or known backing.");
    }

    public static IReadOnlyList<CapabilityInfo> FreezeCapabilities(IReadOnlyList<CapabilityInfo> capabilities)
    {
        Require.NotNull(capabilities);
        if (capabilities.Count > 128) throw new ArgumentException("Capability list exceeds its bound.");
        foreach (var item in capabilities)
        {
            Require.NotNull(item); Require.Enum(item.Capability); Require.Positive(item.Version); Require.NotNull(item.Quality);
        }
        if (capabilities.Select(c => c.Capability).Distinct().Count() != capabilities.Count) throw new ArgumentException("Capabilities must be unique.");
        return Array.AsReadOnly(capabilities.ToArray());
    }
}
