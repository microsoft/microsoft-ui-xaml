using System.Globalization;
using System.Numerics;
using System.Text.RegularExpressions;

namespace XamlProfiler.Models;

/// <summary>
/// The full, parsed property set of a single Windows.UI.Composition <c>IVisual</c>,
/// as shipped in the <c>Properties</c> string of the <c>WucVisualChildInserted</c> /
/// <c>WucVisualRootSet</c> ETW events.
/// <para>
/// The producer (<c>WucVisualTreeProfiler.cpp</c> / <c>DCompTreeHelper.cpp</c>)
/// serializes these as XML-style <c>key="value"</c> attributes. <see cref="Parse"/>
/// turns that string back into this strongly-typed model. Every field is best-effort:
/// a missing or malformed attribute falls back to its default rather than throwing,
/// so a schema drift on the producer never crashes the consumer.
/// </para>
/// </summary>
public sealed class WucVisualProperties
{
    // ---- Base transform ------------------------------------------------------
    public Vector3 Offset { get; init; }
    public Vector2 Size { get; init; }

    public Vector2 AnchorPoint { get; init; }

    public Vector3 CenterPoint { get; init; }

    public Vector3 Scale { get; init; } = Vector3.One;

    public float RotationAngleInDegrees { get; init; }
    public Vector3 RotationAxis { get; init; } = new(0f, 0f, 1f);

    public Quaternion Orientation { get; init; } = Quaternion.Identity;

    /// <summary>Raw 16-float row-major matrix string exactly as emitted (e.g. "1.000,0.000,...").</summary>
    public string TransformMatrix { get; init; } = string.Empty;

    // ---- Render flags --------------------------------------------------------
    public float Opacity { get; init; } = 1f;
    public bool IsVisible { get; init; } = true;
    public bool IsHitTestVisible { get; init; } = true;
    public bool IsPixelSnappingEnabled { get; init; }

    public string CompositeMode { get; init; } = "Inherit";
    public string BorderMode { get; init; } = "Inherit";
    public string BackfaceVisibility { get; init; } = "Inherit";

    public bool HasClip { get; init; }
    public string ClipType { get; init; } = string.Empty;

    public Vector3 RelativeOffsetAdjustment { get; init; }
    public Vector2 RelativeSizeAdjustment { get; init; }

    // ---- Type-specific -------------------------------------------------------
    /// <summary>Child count when the visual is a ContainerVisual, else -1.</summary>
    public int WucChildCount { get; init; } = -1;
    /// <summary>Shape count when the visual is a ShapeVisual, else -1.</summary>
    public int ShapeCount { get; init; } = -1;

    public bool HasBrush { get; init; }
    public string BrushType { get; init; } = string.Empty;
    public bool HasShadow { get; init; }
    public string ShadowType { get; init; } = string.Empty;
    public bool HasEffect { get; init; }
    public bool HasViewBox { get; init; }

    /// <summary>Source visual pointer for a RedirectVisual ("0x0" when none).</summary>
    public string SourceVisual { get; init; } = "0x0";

    /// <summary>The raw attribute string this instance was parsed from (for the detail pane / diffing).</summary>
    public string Raw { get; init; } = string.Empty;

    // Matches key="value" pairs; value may contain commas/spaces (e.g. transformMatrix).
    private static readonly Regex AttrRegex =
        new("(\\w+)=\"([^\"]*)\"", RegexOptions.Compiled);

    /// <summary>
    /// Parse a <c>key="value"</c> attribute string (as emitted by the producer) into
    /// a <see cref="WucVisualProperties"/>. Unknown keys are ignored; absent keys keep
    /// their defaults.
    /// </summary>
    public static WucVisualProperties Parse(string? properties)
    {
        var map = new Dictionary<string, string>(StringComparer.Ordinal);
        if (!string.IsNullOrEmpty(properties))
        {
            foreach (Match m in AttrRegex.Matches(properties))
            {
                map[m.Groups[1].Value] = m.Groups[2].Value;
            }
        }

        return new WucVisualProperties
        {
            Offset = new Vector3(F(map, "offsetX"), F(map, "offsetY"), F(map, "offsetZ")),
            Size = new Vector2(F(map, "width"), F(map, "height")),

            AnchorPoint = new Vector2(F(map, "anchorPointX"), F(map, "anchorPointY")),

            CenterPoint = new Vector3(F(map, "centerPointX"), F(map, "centerPointY"), F(map, "centerPointZ")),

            Scale = new Vector3(F(map, "scaleX", 1f), F(map, "scaleY", 1f), F(map, "scaleZ", 1f)),

            RotationAngleInDegrees = F(map, "rotationAngleInDegrees"),
            RotationAxis = new Vector3(F(map, "rotationAxisX"), F(map, "rotationAxisY"), F(map, "rotationAxisZ", 1f)),

            Orientation = new Quaternion(F(map, "orientationX"), F(map, "orientationY"), F(map, "orientationZ"), F(map, "orientationW", 1f)),

            TransformMatrix = S(map, "transformMatrix"),

            Opacity = F(map, "opacity", 1f),
            IsVisible = B(map, "isVisible", true),
            IsHitTestVisible = B(map, "isHitTestVisible", true),
            IsPixelSnappingEnabled = B(map, "isPixelSnappingEnabled"),

            CompositeMode = S(map, "compositeMode", "Inherit"),
            BorderMode = S(map, "borderMode", "Inherit"),
            BackfaceVisibility = S(map, "backfaceVisibility", "Inherit"),

            HasClip = B(map, "hasClip"),
            ClipType = S(map, "clipType"),

            RelativeOffsetAdjustment = new Vector3(F(map, "relativeOffsetAdjustmentX"), F(map, "relativeOffsetAdjustmentY"), F(map, "relativeOffsetAdjustmentZ")),
            RelativeSizeAdjustment = new Vector2(F(map, "relativeSizeAdjustmentX"), F(map, "relativeSizeAdjustmentY")),

            WucChildCount = I(map, "wucChildCount", -1),
            ShapeCount = I(map, "shapeCount", -1),

            HasBrush = B(map, "hasBrush"),
            BrushType = S(map, "brushType"),
            HasShadow = B(map, "hasShadow"),
            ShadowType = S(map, "shadowType"),
            HasEffect = B(map, "hasEffect"),
            HasViewBox = B(map, "hasViewBox"),

            SourceVisual = S(map, "sourceVisual", "0x0"),

            Raw = properties ?? string.Empty,
        };
    }

    private static float F(Dictionary<string, string> map, string key, float fallback = 0f) =>
        map.TryGetValue(key, out var v) &&
        float.TryParse(v, NumberStyles.Float, CultureInfo.InvariantCulture, out var f)
            ? f : fallback;

    private static int I(Dictionary<string, string> map, string key, int fallback = 0) =>
        map.TryGetValue(key, out var v) &&
        int.TryParse(v, NumberStyles.Integer, CultureInfo.InvariantCulture, out var i)
            ? i : fallback;

    private static bool B(Dictionary<string, string> map, string key, bool fallback = false) =>
        map.TryGetValue(key, out var v) ? string.Equals(v, "true", StringComparison.OrdinalIgnoreCase) : fallback;

    private static string S(Dictionary<string, string> map, string key, string fallback = "") =>
        map.TryGetValue(key, out var v) ? v : fallback;

    /// <summary>
    /// Emits the property set as an ordered list of <c>name → value</c> attribute
    /// pairs, mirroring exactly the attribute names, order, and number formatting
    /// that the producer (<c>DCompTreeHelper.cpp::AppendVisualPropertyAttributes</c>)
    /// writes onto each <c>&lt;WucVisual&gt;</c> tag in its debug-output dump. This
    /// lets the consumer-side XML export round-trip into the same shape as the
    /// in-process tree walker / DComp tree dump.
    /// </summary>
    public IReadOnlyList<KeyValuePair<string, string>> ToXmlAttributes()
    {
        var ci = CultureInfo.InvariantCulture;
        string F1(float v) => v.ToString("0.0", ci);
        string F2(float v) => v.ToString("0.00", ci);
        string F3(float v) => v.ToString("0.000", ci);
        string Bo(bool v) => v ? "true" : "false";

        var a = new List<KeyValuePair<string, string>>();
        void Add(string k, string v) => a.Add(new KeyValuePair<string, string>(k, v));

        // Base Visual transform.
        Add("offsetX", F1(Offset.X)); Add("offsetY", F1(Offset.Y)); Add("offsetZ", F1(Offset.Z));
        Add("width", F1(Size.X)); Add("height", F1(Size.Y));
        Add("anchorPointX", F2(AnchorPoint.X)); Add("anchorPointY", F2(AnchorPoint.Y));
        Add("centerPointX", F2(CenterPoint.X)); Add("centerPointY", F2(CenterPoint.Y)); Add("centerPointZ", F2(CenterPoint.Z));
        Add("scaleX", F3(Scale.X)); Add("scaleY", F3(Scale.Y)); Add("scaleZ", F3(Scale.Z));
        Add("rotationAngleInDegrees", F2(RotationAngleInDegrees));
        Add("rotationAxisX", F2(RotationAxis.X)); Add("rotationAxisY", F2(RotationAxis.Y)); Add("rotationAxisZ", F2(RotationAxis.Z));
        Add("orientationX", F3(Orientation.X)); Add("orientationY", F3(Orientation.Y));
        Add("orientationZ", F3(Orientation.Z)); Add("orientationW", F3(Orientation.W));

        // Transform matrix + modes + relative adjustments + flags.
        Add("transformMatrix", TransformMatrix);
        Add("opacity", F3(Opacity));
        Add("isVisible", Bo(IsVisible)); Add("isHitTestVisible", Bo(IsHitTestVisible));
        Add("isPixelSnappingEnabled", Bo(IsPixelSnappingEnabled));
        Add("compositeMode", CompositeMode); Add("borderMode", BorderMode); Add("backfaceVisibility", BackfaceVisibility);
        Add("hasClip", Bo(HasClip)); Add("clipType", ClipType);
        Add("relativeOffsetAdjustmentX", F2(RelativeOffsetAdjustment.X));
        Add("relativeOffsetAdjustmentY", F2(RelativeOffsetAdjustment.Y));
        Add("relativeOffsetAdjustmentZ", F2(RelativeOffsetAdjustment.Z));
        Add("relativeSizeAdjustmentX", F2(RelativeSizeAdjustment.X));
        Add("relativeSizeAdjustmentY", F2(RelativeSizeAdjustment.Y));

        // Type-specific.
        Add("wucChildCount", WucChildCount.ToString(ci)); Add("shapeCount", ShapeCount.ToString(ci));
        Add("hasBrush", Bo(HasBrush)); Add("brushType", BrushType);
        Add("hasShadow", Bo(HasShadow)); Add("shadowType", ShadowType);
        Add("hasEffect", Bo(HasEffect)); Add("hasViewBox", Bo(HasViewBox));
        Add("sourceVisual", SourceVisual);

        return a;
    }

    /// <summary>
    /// Flattens this property set into an ordered list of display rows (grouped
    /// by category, with section headers) for the detail-pane properties panel.
    /// Type-specific rows (brush/shadow/shape/source/child counts) are only
    /// emitted when relevant, so the panel stays compact.
    /// </summary>
    public IReadOnlyList<WucPropertyRow> ToRows()
    {
        var rows = new List<WucPropertyRow>();
        void Header(string h) => rows.Add(new WucPropertyRow(h, string.Empty, true));
        void Row(string k, string v) => rows.Add(new WucPropertyRow(k, v, false));
        string Vec2(float x, float y) => $"{x:0.###}, {y:0.###}";
        string Vec3(float x, float y, float z) => $"{x:0.###}, {y:0.###}, {z:0.###}";

        Header("Transform");
        Row("Offset", Vec3(Offset.X, Offset.Y, Offset.Z));
        Row("Size", Vec2(Size.X, Size.Y));
        Row("AnchorPoint", Vec2(AnchorPoint.X, AnchorPoint.Y));
        Row("CenterPoint", Vec3(CenterPoint.X, CenterPoint.Y, CenterPoint.Z));
        Row("Scale", Vec3(Scale.X, Scale.Y, Scale.Z));
        Row("Rotation°", $"{RotationAngleInDegrees:0.###}");
        Row("RotationAxis", Vec3(RotationAxis.X, RotationAxis.Y, RotationAxis.Z));
        Row("Orientation", $"{Orientation.X:0.###}, {Orientation.Y:0.###}, {Orientation.Z:0.###}, {Orientation.W:0.###}");
        if (!string.IsNullOrEmpty(TransformMatrix)) Row("Matrix", TransformMatrix);
        Row("RelativeOffsetAdj", Vec3(RelativeOffsetAdjustment.X, RelativeOffsetAdjustment.Y, RelativeOffsetAdjustment.Z));
        Row("RelativeSizeAdj", Vec2(RelativeSizeAdjustment.X, RelativeSizeAdjustment.Y));

        Header("Render");
        Row("Opacity", $"{Opacity:0.###}");
        Row("IsVisible", IsVisible ? "true" : "false");
        Row("IsHitTestVisible", IsHitTestVisible ? "true" : "false");
        Row("IsPixelSnapping", IsPixelSnappingEnabled ? "true" : "false");
        Row("CompositeMode", CompositeMode);
        Row("BorderMode", BorderMode);
        Row("BackfaceVisibility", BackfaceVisibility);
        Row("HasClip", HasClip ? (string.IsNullOrEmpty(ClipType) ? "true" : ClipType) : "false");

        bool anyTypeSpecific = HasBrush || HasShadow || HasEffect || HasViewBox
            || WucChildCount >= 0 || ShapeCount >= 0
            || (!string.IsNullOrEmpty(SourceVisual) && SourceVisual != "0x0");
        if (anyTypeSpecific)
        {
            Header("Type-specific");
            if (WucChildCount >= 0) Row("ChildCount", WucChildCount.ToString(CultureInfo.InvariantCulture));
            if (ShapeCount >= 0) Row("ShapeCount", ShapeCount.ToString(CultureInfo.InvariantCulture));
            if (HasBrush) Row("Brush", string.IsNullOrEmpty(BrushType) ? "true" : BrushType);
            if (HasShadow) Row("Shadow", string.IsNullOrEmpty(ShadowType) ? "true" : ShadowType);
            if (HasEffect) Row("Effect", "true");
            if (HasViewBox) Row("ViewBox", "true");
            if (!string.IsNullOrEmpty(SourceVisual) && SourceVisual != "0x0") Row("SourceVisual", SourceVisual);
        }

        return rows;
    }
}

/// <summary>
/// One row in the detail-pane properties panel. When <see cref="IsHeader"/> is
/// true the row is a section header (rendered bold, no value).
/// </summary>
public sealed class WucPropertyRow
{
    public WucPropertyRow(string name, string value, bool isHeader)
    {
        Name = name;
        Value = value;
        IsHeader = isHeader;
    }

    public string Name { get; }
    public string Value { get; }
    public bool IsHeader { get; }
}
