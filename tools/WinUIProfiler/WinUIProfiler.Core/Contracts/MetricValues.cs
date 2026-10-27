namespace WinUIProfiler.Contracts;

public enum MeasurementKind
{
    Measured, Estimated, Unavailable
}
public enum CoverageState
{
    Complete, Partial, Unavailable
}
public sealed record DataQuality(MeasurementKind Kind, CoverageState Coverage, string? Reason)
{
    public MeasurementKind Kind { get; } = Require.Enum(Kind);
    public CoverageState Coverage { get; } = Require.Enum(Coverage);
    public string? Reason
    {
        get;
    } = Kind != MeasurementKind.Measured || Coverage != CoverageState.Complete
        ? Require.Text(Reason) : Reason;
    public static DataQuality Complete { get; } = new(MeasurementKind.Measured, CoverageState.Complete, null);
}
public enum ValueUnit
{
    Count, Bytes, Percent, QpcTicks, Pixels, Boolean, Scalar, String
}
public enum ValueKind
{
    Integer, Number, Boolean, Text, Missing
}
public sealed record TypedValue(ValueKind Kind, ValueUnit Unit, long? Integer, double? Number, bool? Boolean, string? Text, DataQuality Quality)
{
    public void Validate()
    {
        Require.Enum(Kind);
        Require.Enum(Unit);
        Require.NotNull(Quality);
        var count = (Integer.HasValue ? 1 : 0) + (Number.HasValue ? 1 : 0) + (Boolean.HasValue ? 1 : 0) + (Text is not null ? 1 : 0);
        if (Kind == ValueKind.Missing)
        {
            if (count != 0 || Quality.Kind != MeasurementKind.Unavailable)
            {
                throw new ArgumentException("Missing values require unavailable quality and no value.");
            }

            return;
        }
        if (count != 1 ||
Quality.Kind == MeasurementKind.Unavailable ||
            Kind is ValueKind.Integer or ValueKind.Number &&
Unit is ValueUnit.Boolean or ValueUnit.String ||
            Kind == ValueKind.Integer &&
!Integer.HasValue ||
            Kind == ValueKind.Number &&
(!Number.HasValue ||
!double.IsFinite(Number.Value)) ||
            Kind == ValueKind.Boolean &&
(!Boolean.HasValue ||
Unit != ValueUnit.Boolean) ||
            Kind == ValueKind.Text &&
(Text is null ||
Unit != ValueUnit.String))
        {
            throw new ArgumentException("Value discriminator, unit and value must agree.");
        }
    }
}
