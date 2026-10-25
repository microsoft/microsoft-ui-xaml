namespace WinUIProfiler.Contracts;

public static class QueryResultValidation
{
    public static T Freeze<T>(T value, ResultContext context) where T : IQueryValue
    {
        if (value is null) throw new ArgumentNullException(nameof(value));
        Require.NotNull(context).Scope.Validate();
        IQueryValue frozen = value switch
        {
            SessionInfo row => Session(row, context),
            MetricSample row => Metric(row, context),
            StartupEntry row => Startup(row, context),
            FrameInfo row => Frame(row, context),
            ObjectHistoryEntry row => History(row, context),
            TreeEntry row => Tree(row, context),
            CompositionTreeEntry row => Composition(row, context),
            CorrelationEntry row => Correlation(row, context),
            ResourceUsageEntry row => Resource(row, context),
            SnapshotMetadata row => Snapshot(row, context),
            SnapshotDelta row => Comparison(row, context),
            RetentionPath row => Retention(row, context),
            _ => throw new ArgumentException("Unsupported typed query result.")
        };
        return (T)frozen;
    }

    private static void Key(ObjectKey key, ResultContext context, IdentityDomain? domain = null)
    {
        key.Validate();
        if (key.Process != context.Scope.Process || domain is { } expected && key.Domain != expected)
            throw new ArgumentException("Query row identity does not belong to the committed process/domain.");
    }

    private static void Timestamp(long timestamp, ResultContext context)
    {
        Require.Nonnegative(timestamp);
        if (timestamp > context.Watermark.TimestampQpc) throw new ArgumentException("Row extends past its committed watermark.");
    }

    private static void Range(TimeRange range, ResultContext context)
    {
        Timestamp(range.StartQpc, context); Timestamp(range.EndQpc, context);
    }

    private static void Ordinal(long ordinal, ResultContext context)
    {
        Require.Positive(ordinal);
        if (ordinal > context.Watermark.IngestOrdinal) throw new ArgumentException("Row ordinal extends past its committed watermark.");
    }

    private static SessionInfo Session(SessionInfo row, ResultContext context)
    {
        Require.Enum(row.Mode); Require.Enum(row.Status); Require.Positive(row.QpcFrequency);
        if (row.ProcessCreationQpc is { } creation) Timestamp(creation, context);
        if (row.Status is SessionStatus.Failed or SessionStatus.StoppedAtLimit or SessionStatus.Stopped) Require.Text(row.StopReason);
        Require.NotNull(row.Producers);
        if (row.Producers.Count > 4096) throw new ArgumentException("Producer metadata exceeds its bound.");
        var producers = row.Producers.Select(producer =>
        {
            Require.NotNull(producer); Require.Id(producer.ProviderId); Require.Id(producer.Epoch);
            Require.Positive(producer.SchemaVersion); Require.Text(producer.Build); Require.Enum(producer.Architecture);
            return producer with { Capabilities = ObservationRules.FreezeCapabilities(producer.Capabilities) };
        }).ToArray();
        IReadOnlyList<ListenerDomainInterval>? intervals = null;
        if (row.ListenerIntervals is { } source)
        {
            if (source.Count > 8192) throw new ArgumentException("Listener coverage metadata exceeds its bound.");
            foreach (var interval in source)
            {
                Require.NotNull(interval); Require.Id(interval.ListenerId); Require.Id(interval.ContinuityId); Require.NotNull(interval.Quality);
                interval.Domain.Validate(); Range(interval.Interval, context);
                if (interval.Domain.Target != context.Scope.Process) throw new ArgumentException("Listener coverage belongs to a different target process instance.");
            }
            intervals = Array.AsReadOnly(source.ToArray());
        }
        return row with { Producers = Array.AsReadOnly(producers), ListenerIntervals = intervals };
    }

    private static MetricSample Metric(MetricSample row, ResultContext context)
    {
        Timestamp(row.TimestampQpc, context); Range(row.Interval, context);
        if (row.Interval.EndQpc > row.TimestampQpc) throw new ArgumentException("Sample cannot precede the end of its observation interval.");
        ObservationRules.Metric(row.Metric, row.Value, row.Interval, row.Source, row.SourceVersion, row.Gpu, row.SourceObservation);
        return row;
    }

    private static StartupEntry Startup(StartupEntry row, ResultContext context)
    {
        Require.Text(row.Phase); Range(row.Range, context); Require.NotNull(row.Quality);
        if (row.SubmittedFrameId is { } id) Require.Positive(id);
        return row;
    }

    private static FrameInfo Frame(FrameInfo row, ResultContext context)
    {
        row.Attempt.Validate(); Require.Enum(row.AttemptStatus); Range(row.Range, context); Require.NotNull(row.Changes);
        if (row.AttemptStatus == FrameAttemptStatus.Failed && row.HResult is not < 0 ||
            row.AttemptStatus == FrameAttemptStatus.Completed && row.HResult is not >= 0 ||
            row.AttemptStatus == FrameAttemptStatus.InProgress && row.HResult is not null)
            throw new ArgumentException("Frame attempt status and result disagree.");
        if (row.Submission is { } submission)
        {
            Require.Positive(submission.SubmittedFrameId); Require.Id(submission.ProducerEpoch); Timestamp(submission.SubmittedQpc, context);
            if (submission.ProducerEpoch != row.Attempt.ProducerEpoch || submission.SubmittedQpc < row.Range.StartQpc || submission.SubmittedQpc > row.Range.EndQpc)
                throw new ArgumentException("Submission must retain its own successful timestamp/epoch within the attempt.");
        }
        if (row.Changes.Count > 4096) throw new ArgumentException("Frame changes exceed their bound.");
        FrameChange? previousChange = null;
        foreach (var change in row.Changes)
        {
            Require.NotNull(change); Timestamp(change.TimestampQpc, context); Ordinal(change.IngestOrdinal, context);
            Require.Enum(change.Relation); Require.NotNull(change.Quality); Require.NotNull(change.Cause).Validate(change.Object?.ProducerEpoch);
            Observation(change.Object, change.Observation, change.Quality, context);
            if (change.TimestampQpc < row.Range.StartQpc || change.TimestampQpc > row.Range.EndQpc ||
                change.Relation == FrameChangeRelation.Causal && (change.Cause.Kind != CauseKind.ExplicitAttempt || change.Cause.Attempt != row.Attempt))
                throw new ArgumentException("Causal frame changes require the exact epoch/root/dispatcher/attempt; time proximity alone is not causality.");
            if (previousChange is { } previous && (change.TimestampQpc < previous.TimestampQpc ||
                change.TimestampQpc == previous.TimestampQpc && change.IngestOrdinal <= previous.IngestOrdinal))
                throw new ArgumentException("Frame changes must be chronological by QPC then ingestion ordinal.");
            previousChange = change;
        }
        return row with { Changes = Array.AsReadOnly(row.Changes.ToArray()) };
    }

    private static ObjectHistoryEntry History(ObjectHistoryEntry row, ResultContext context)
    {
        Key(row.Object, context); Timestamp(row.TimestampQpc, context); Ordinal(row.IngestOrdinal, context);
        Require.NotNull(row.Quality); Require.NotNull(row.Cause).Validate(row.Object.ProducerEpoch);
        Observation(row.Object, row.Observation, row.Quality, context);
        return row;
    }

    private static void Observation(ObjectKey? key, ObjectObservation observation, DataQuality quality, ResultContext context)
    {
        Require.NotNull(observation);
        if (key is { } actual) Key(actual, context);
        else if (observation is not (AllocationObservation or BoundaryObservation)) throw new ArgumentException("This observation requires an object identity.");
        switch (observation)
        {
            case LifetimeObservation row:
                Require.Enum(row.Operation);
                ObservationRules.Classification(key!.Value.Domain, row.Classification);
                if (row.Address is 0) throw new ArgumentException("Unknown address must be null.");
                if (row.TypeName is not null) Require.Text(row.TypeName);
                break;
            case TypeObservation row:
                Require.Positive(row.TypeId); Require.Text(row.Name); ObservationRules.Classification(key!.Value.Domain, row.Classification);
                break;
            case BoundaryObservation row:
                Require.Enum(row.Boundary); Require.Enum(row.Phase); Require.Positive(row.OperationId);
                if (row.Phase == BoundaryPhase.Begin && row.HResult is not null) throw new ArgumentException("Layout boundary begin has no terminal result.");
                break;
            case PropertyObservation row:
                Key(key!.Value, context, IdentityDomain.XamlCore); ObservationRules.Property(row.Property, row.Previous, row.Current);
                break;
            case MembershipObservation row:
                ObservationRules.Tree(key!.Value, row.Parent, row.Relationship, row.Order, row.RootId, row.DispatcherId, row.Membership);
                if (row.Parent is { } parent) Key(parent, context);
                break;
            case VisualPropertyObservation row:
                Key(key!.Value, context, IdentityDomain.CompositionVisual); ObservationRules.Visual(row.Property, row.Value);
                break;
            case ResourceObservation row:
                Key(key!.Value, context, IdentityDomain.Resource); Require.Enum(row.Operation); Require.Id(row.DeviceEpoch);
                Require.NotNull(row.Classification).Validate();
                if (row.Owner is { } owner) Key(owner, context);
                ObservationRules.Provenance(row.Provenance, row.Size, row.BackingAllocationId, quality);
                break;
            case CorrelationObservation row:
                Key(row.Other, context);
                ObservationRules.Correlation(row.IsFrom ? key!.Value : row.Other, row.IsFrom ? row.Other : key!.Value, row.Relationship);
                break;
            case AllocationObservation row:
                ObservationRules.Allocation(row.AllocationId, row.Operation, row.PreviousAllocationId, row.RequestedBytes, row.ActualBytes, row.AllocatorEpoch);
                break;
            default: throw new ArgumentException("Unsupported typed object observation.");
        }
    }

    private static TreeEntry Tree(TreeEntry row, ResultContext context)
    {
        Key(row.Object, context); if (row.Parent is { } parent) Key(parent, context);
        ObservationRules.Tree(row.Object, row.Parent, row.Relationship, row.Order, row.RootId, row.DispatcherId, row.Membership);
        Require.NotNull(row.Quality); Require.NotNull(row.Classification).Validate();
        if (row.TypeName is not null) Require.Text(row.TypeName);
        if (row.Object.Domain == IdentityDomain.XamlCore && row.Classification.Kind == XamlObjectKind.NotApplicable ||
            row.Object.Domain != IdentityDomain.XamlCore && row.Classification.Kind is not (XamlObjectKind.NotApplicable or XamlObjectKind.Unknown))
            throw new ArgumentException("XAML UIElement evidence cannot be applied to arbitrary object domains.");
        return row;
    }

    private static CompositionTreeEntry Composition(CompositionTreeEntry row, ResultContext context)
    {
        var tree = Tree(Require.NotNull(row.Topology), context);
        if (tree.Object.Domain is not (IdentityDomain.CompNode or IdentityDomain.CompositionVisual)) throw new ArgumentException("Composition query needs composition topology.");
        Require.NotNull(row.Properties);
        if (row.Properties.Count > 256 || tree.Object.Domain == IdentityDomain.CompNode && row.Properties.Count != 0)
            throw new ArgumentException("Visual properties are bounded and belong to canonical IVisuals, not CompNodes.");
        foreach (var property in row.Properties)
        {
            Require.NotNull(property); ObservationRules.Visual(property.Property, property.Value); Timestamp(property.TimestampQpc, context);
            Require.NotNull(property.Cause).Validate(tree.Object.ProducerEpoch);
        }
        if (row.Properties.Select(p => p.Property).Distinct().Count() != row.Properties.Count) throw new ArgumentException("A visual projection has one observed value per property.");
        return row with { Topology = tree, Properties = Array.AsReadOnly(row.Properties.ToArray()) };
    }

    private static CorrelationEntry Correlation(CorrelationEntry row, ResultContext context)
    {
        Key(row.From, context); Key(row.To, context); Range(row.Validity, context); Require.NotNull(row.Quality);
        ObservationRules.Correlation(row.From, row.To, row.Kind);
        return row;
    }

    private static ResourceUsageEntry Resource(ResourceUsageEntry row, ResultContext context)
    {
        Key(row.Resource, context, IdentityDomain.Resource); Require.Id(row.DeviceEpoch); Require.NotNull(row.Quality);
        if (row.Owner is { } owner) Key(owner, context);
        Require.NotNull(row.Classification).Validate();
        ObservationRules.Provenance(row.Provenance, row.Size, row.BackingAllocationId, row.Quality);
        return row;
    }

    private static SnapshotMetadata Snapshot(SnapshotMetadata row, ResultContext context)
    {
        if (row.SessionId != context.Scope.SessionId || row.Process != context.Scope.Process || row.CommittedOrdinal > context.Watermark.IngestOrdinal)
            throw new ArgumentException("Snapshot attempt does not belong to this committed session/process.");
        Timestamp(row.Extent.StartedQpc, context);
        if (row.Extent.EndedQpc is { } end) Timestamp(end, context);
        foreach (var boundary in row.Extent.Boundaries) Range(boundary.Interval, context);
        return row;
    }

    private static SnapshotDelta Comparison(SnapshotDelta row, ResultContext context)
    {
        Require.Id(row.SnapshotId); Require.Enum(row.Compatibility); var key = Require.NotNull(row.Key);
        Require.Enum(key.Attribute); Require.Enum(key.Grouping);
        if (key.Object is { } obj) Key(obj, context);
        if (key.RootId is { } root) Require.Id(root);
        if (key.ResourceKind is { } kind) Require.Enum(kind);
        if (key.TypeName is not null) Require.Text(key.TypeName);
        var count = (key.Object is null ? 0 : 1) + (key.TypeName is null ? 0 : 1) + (key.RootId is null ? 0 : 1) + (key.ResourceKind is null ? 0 : 1);
        var validGrouping = key.Grouping switch
        {
            ComparisonGrouping.Session => count == 0,
            ComparisonGrouping.Object => count == 1 && key.Object is not null,
            ComparisonGrouping.Type => count == 1 && key.TypeName is not null,
            ComparisonGrouping.Root => count == 1 && key.RootId is not null,
            ComparisonGrouping.ResourceKind => count == 1 && key.ResourceKind is not null,
            _ => false
        };
        if (!validGrouping) throw new ArgumentException("Comparison grouping fields do not form a distinct typed row key.");
        var unit = key.Attribute is SnapshotAttribute.ObjectCount or SnapshotAttribute.ConnectedElementCount or SnapshotAttribute.CanonicalVisualCount ? ValueUnit.Count : ValueUnit.Bytes;
        foreach (var value in new[] { row.Baseline, row.Current, row.Delta })
        {
            Require.NotNull(value).Validate();
            if (value.Kind is not (ValueKind.Integer or ValueKind.Number or ValueKind.Missing)) throw new ArgumentException("Comparison attributes are numeric or explicitly missing.");
            if (value.Unit == ValueUnit.Count && value.Kind == ValueKind.Number) throw new ArgumentException("Counts retain integer discriminators, including signed count deltas.");
        }
        if (row.Baseline.Integer is < 0 || row.Baseline.Number is < 0 || row.Current.Integer is < 0 || row.Current.Number is < 0 ||
            row.Delta.Unit != unit) throw new ArgumentException("Baseline/current attribute values are nonnegative; signed deltas retain attribute units.");
        if (row.Compatibility == ComparisonCompatibility.Compatible)
        {
            if (row.Baseline.Kind == ValueKind.Missing || row.Current.Kind == ValueKind.Missing || row.Delta.Kind == ValueKind.Missing ||
                row.Baseline.Unit != unit || row.Current.Unit != unit) throw new ArgumentException("Compatible comparison requires the same available attribute units.");
        }
        else
        {
            Require.Text(row.Reason);
            if (row.Delta.Kind != ValueKind.Missing ||
                row.Compatibility == ComparisonCompatibility.MissingBaseline && row.Baseline.Kind != ValueKind.Missing ||
                row.Compatibility == ComparisonCompatibility.MissingCurrent && row.Current.Kind != ValueKind.Missing)
                throw new ArgumentException("Missing/incompatible attributes cannot produce a fabricated numeric delta.");
        }
        return row;
    }

    private static RetentionPath Retention(RetentionPath row, ResultContext context)
    {
        Key(row.Object, context); Require.Text(row.RootKind); Require.Enum(row.Evidence); Require.NotNull(row.Quality); Require.NotNull(row.Edges);
        if (row.Edges.Count > 256) throw new ArgumentException("Retention path exceeds its depth bound.");
        for (var i = 0; i < row.Edges.Count; i++)
        {
            var edge = Require.NotNull(row.Edges[i]); Key(edge.From, context); Key(edge.To, context);
            Require.Text(edge.Relationship); Require.Enum(edge.Evidence); Require.NotNull(edge.Quality);
            if (i > 0 && row.Edges[i - 1].To != edge.From || row.Evidence == OwnershipEvidence.Proven && edge.Evidence != OwnershipEvidence.Proven)
                throw new ArgumentException("Retention path continuity/evidence cannot claim an unproven root chain.");
        }
        if (row.Edges.Count > 0 && row.Edges[^1].To != row.Object) throw new ArgumentException("Retention path must reach its selected object.");
        return row with { Edges = Array.AsReadOnly(row.Edges.ToArray()) };
    }
}
