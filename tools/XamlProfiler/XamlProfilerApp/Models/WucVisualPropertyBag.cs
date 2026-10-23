using System.Globalization;
using System.Text;

namespace XamlProfiler.Models;

/// <summary>
/// Concrete WUC visual type carried by <c>WucVisualCreated</c>, <c>WucVisualChildInserted</c> and
/// <c>WucVisualRootSet</c>. Mirrors the producer's <c>WucVisualTreeProfiler::VisualKind</c>
/// (WucVisualTreeProfiler.h).
/// </summary>
public enum WucVisualKind : uint
{
    Unknown = 0,
    Visual = 1,
    ContainerVisual = 2,
    SpriteVisual = 3,
    LayerVisual = 4,
    ShapeVisual = 5,
    RedirectVisual = 6,
}

/// <summary>
/// Property ids carried by <c>WucVisualPropertyChanged</c> (components in X/Y/Z/W) and
/// <c>WucVisualObjectChanged</c> (object-valued). Mirrors the producer's
/// <c>WucVisualTreeProfiler::VisualProperty</c> (WucVisualTreeProfiler.h).
/// </summary>
public enum WucVisualPropertyId : uint
{
    Offset = 1,                     // X, Y, Z
    Size = 2,                       // X, Y
    Opacity = 3,                    // X
    IsVisible = 4,                  // X (0/1)
    IsHitTestVisible = 5,           // X (0/1)
    IsPixelSnappingEnabled = 6,     // X (0/1)
    CompositeMode = 7,              // X (CompositionCompositeMode value)
    BorderMode = 8,                 // X (CompositionBorderMode value)
    Scale = 9,                      // X, Y, Z
    CenterPoint = 10,               // X, Y, Z
    RotationAngleInDegrees = 11,    // X
    RotationAxis = 12,              // X, Y, Z
    AnchorPoint = 13,               // X, Y
    RelativeOffsetAdjustment = 14,  // X, Y, Z
    RelativeSizeAdjustment = 15,    // X, Y
    BackfaceVisibility = 16,        // X (CompositionBackfaceVisibility value)
    Orientation = 17,               // X, Y, Z, W (quaternion)
    ShapeCount = 18,                // X

    Clip = 20,                      // object (CompositionClip)
    Brush = 21,                     // object (CompositionBrush on a SpriteVisual)
    Shadow = 22,                    // object (CompositionShadow on a SpriteVisual / LayerVisual)
    Effect = 23,                    // object (CompositionEffectBrush on a LayerVisual)
    Source = 24,                    // object (source visual of a RedirectVisual; ObjectId is its VisualId)
    ViewBox = 25,                   // object (CompositionViewBox on a ShapeVisual)
}

/// <summary>
/// The latest known Windows.UI.Composition properties of one IVisual, accumulated from the
/// granular events (<c>WucVisualCreated</c>, <c>WucVisualPropertyChanged</c>,
/// <c>WucVisualTransformChanged</c>, <c>WucVisualObjectChanged</c>). A property the producer never
/// reported keeps the composition default.
/// <para>
/// <see cref="ToPropertiesString"/> renders the bag in the <c>key="value"</c> format the insert
/// and root-set events used to carry, so <see cref="WucVisualProperties.Parse"/> and the detail
/// rows work unchanged. That string was a snapshot taken when the visual was inserted; a bag
/// holds the current values.
/// </para>
/// </summary>
public sealed class WucVisualPropertyBag
{
    private readonly Dictionary<WucVisualPropertyId, (float X, float Y, float Z, float W)> _values = new();
    private readonly Dictionary<WucVisualPropertyId, (ulong Id, string Type)> _objects = new();
    private float[]? _transform;

    public WucVisualPropertyBag(WucVisualKind kind = WucVisualKind.Unknown) => Kind = kind;

    /// <summary>Concrete visual type, from WucVisualCreated or the latest insert/root-set event.</summary>
    public WucVisualKind Kind { get; set; }

    /// <summary>
    /// Start again from the composition defaults: a new visual can reuse a released visual's
    /// address. Resets in place so tree nodes holding this bag see the new visual.
    /// </summary>
    public void Reset(WucVisualKind kind)
    {
        _values.Clear();
        _objects.Clear();
        _transform = null;
        Kind = kind;
    }

    public void SetValue(WucVisualPropertyId property, float x, float y, float z, float w)
        => _values[property] = (x, y, z, w);

    /// <summary>Row-major M11..M44.</summary>
    public void SetTransform(ReadOnlySpan<float> matrix)
    {
        if (matrix.Length < 16) return;
        _transform ??= new float[16];
        matrix[..16].CopyTo(_transform);
    }

    /// <summary>Set an object-valued property; <paramref name="objectId"/> 0 clears it.</summary>
    public void SetObject(WucVisualPropertyId property, ulong objectId, string? typeName)
    {
        if (objectId == 0) _objects.Remove(property);
        else _objects[property] = (objectId, typeName ?? string.Empty);
    }

    /// <summary>
    /// Render in the legacy <c>key="value"</c> format. Like the old producer, the type-specific
    /// fields come only from the members of <paramref name="kind"/>, and <c>wucChildCount</c> is
    /// reported only for container kinds (every kind except <see cref="WucVisualKind.Visual"/>).
    /// </summary>
    public string ToPropertiesString(WucVisualKind kind, int childCount)
    {
        var offset = Get(WucVisualPropertyId.Offset, (0f, 0f, 0f, 0f));
        var size = Get(WucVisualPropertyId.Size, (0f, 0f, 0f, 0f));
        var anchor = Get(WucVisualPropertyId.AnchorPoint, (0f, 0f, 0f, 0f));
        var center = Get(WucVisualPropertyId.CenterPoint, (0f, 0f, 0f, 0f));
        var scale = Get(WucVisualPropertyId.Scale, (1f, 1f, 1f, 0f));
        var rotation = Get(WucVisualPropertyId.RotationAngleInDegrees, (0f, 0f, 0f, 0f));
        var axis = Get(WucVisualPropertyId.RotationAxis, (0f, 0f, 1f, 0f));
        var orientation = Get(WucVisualPropertyId.Orientation, (0f, 0f, 0f, 1f));
        var opacity = Get(WucVisualPropertyId.Opacity, (1f, 0f, 0f, 0f));
        var isVisible = Get(WucVisualPropertyId.IsVisible, (1f, 0f, 0f, 0f));
        var isHitTestVisible = Get(WucVisualPropertyId.IsHitTestVisible, (1f, 0f, 0f, 0f));
        var isPixelSnapping = Get(WucVisualPropertyId.IsPixelSnappingEnabled, (0f, 0f, 0f, 0f));
        var compositeMode = Get(WucVisualPropertyId.CompositeMode, (0f, 0f, 0f, 0f));
        var borderMode = Get(WucVisualPropertyId.BorderMode, (0f, 0f, 0f, 0f));
        var backface = Get(WucVisualPropertyId.BackfaceVisibility, (0f, 0f, 0f, 0f));
        var relOffset = Get(WucVisualPropertyId.RelativeOffsetAdjustment, (0f, 0f, 0f, 0f));
        var relSize = Get(WucVisualPropertyId.RelativeSizeAdjustment, (0f, 0f, 0f, 0f));
        var m = _transform ?? Identity;

        bool hasClip = _objects.TryGetValue(WucVisualPropertyId.Clip, out var clip);

        int wucChildCount = kind is WucVisualKind.Unknown or WucVisualKind.Visual ? -1 : childCount;
        int shapeCount = -1;
        bool hasBrush = false, hasShadow = false, hasEffect = false, hasViewBox = false;
        string brushType = string.Empty, shadowType = string.Empty;
        ulong sourceVisual = 0;
        switch (kind)
        {
            case WucVisualKind.SpriteVisual:
                hasBrush = TryGetObject(WucVisualPropertyId.Brush, out brushType);
                hasShadow = TryGetObject(WucVisualPropertyId.Shadow, out shadowType);
                break;
            case WucVisualKind.LayerVisual:
                hasEffect = _objects.ContainsKey(WucVisualPropertyId.Effect);
                hasShadow = TryGetObject(WucVisualPropertyId.Shadow, out shadowType);
                break;
            case WucVisualKind.ShapeVisual:
                // A ShapeVisual always has a Shapes collection, so the old producer always reported a count.
                shapeCount = _values.TryGetValue(WucVisualPropertyId.ShapeCount, out var shapes) ? (int)shapes.X : 0;
                hasViewBox = _objects.ContainsKey(WucVisualPropertyId.ViewBox);
                break;
            case WucVisualKind.RedirectVisual:
                sourceVisual = _objects.TryGetValue(WucVisualPropertyId.Source, out var source) ? source.Id : 0;
                break;
        }

        var sb = new StringBuilder(1024);
        sb.Append("offsetX=\"").Append(F(offset.X, "F1")).Append("\" offsetY=\"").Append(F(offset.Y, "F1"))
          .Append("\" offsetZ=\"").Append(F(offset.Z, "F1")).Append("\" width=\"").Append(F(size.X, "F1"))
          .Append("\" height=\"").Append(F(size.Y, "F1")).Append("\" ")
          .Append("anchorPointX=\"").Append(F(anchor.X, "F2")).Append("\" anchorPointY=\"").Append(F(anchor.Y, "F2")).Append("\" ")
          .Append("centerPointX=\"").Append(F(center.X, "F2")).Append("\" centerPointY=\"").Append(F(center.Y, "F2"))
          .Append("\" centerPointZ=\"").Append(F(center.Z, "F2")).Append("\" ")
          .Append("scaleX=\"").Append(F(scale.X, "F3")).Append("\" scaleY=\"").Append(F(scale.Y, "F3"))
          .Append("\" scaleZ=\"").Append(F(scale.Z, "F3")).Append("\" ")
          .Append("rotationAngleInDegrees=\"").Append(F(rotation.X, "F2")).Append("\" rotationAxisX=\"").Append(F(axis.X, "F2"))
          .Append("\" rotationAxisY=\"").Append(F(axis.Y, "F2")).Append("\" rotationAxisZ=\"").Append(F(axis.Z, "F2")).Append("\" ")
          .Append("orientationX=\"").Append(F(orientation.X, "F3")).Append("\" orientationY=\"").Append(F(orientation.Y, "F3"))
          .Append("\" orientationZ=\"").Append(F(orientation.Z, "F3")).Append("\" orientationW=\"").Append(F(orientation.W, "F3")).Append("\" ");

        sb.Append("transformMatrix=\"");
        for (int i = 0; i < 16; i++)
        {
            if (i > 0) sb.Append(i % 4 == 0 ? ", " : ",");
            sb.Append(F(m[i], "F3"));
        }
        sb.Append("\" ")
          .Append("opacity=\"").Append(F(opacity.X, "F3")).Append("\" isVisible=\"").Append(Bool(isVisible.X != 0f))
          .Append("\" isHitTestVisible=\"").Append(Bool(isHitTestVisible.X != 0f))
          .Append("\" isPixelSnappingEnabled=\"").Append(Bool(isPixelSnapping.X != 0f)).Append("\" ")
          .Append("compositeMode=\"").Append(CompositeModeName(compositeMode.X)).Append("\" borderMode=\"").Append(BorderModeName(borderMode.X))
          .Append("\" backfaceVisibility=\"").Append(BackfaceVisibilityName(backface.X)).Append("\" hasClip=\"").Append(Bool(hasClip))
          .Append("\" clipType=\"").Append(hasClip ? clip.Type : string.Empty).Append("\" ")
          .Append("relativeOffsetAdjustmentX=\"").Append(F(relOffset.X, "F2")).Append("\" relativeOffsetAdjustmentY=\"").Append(F(relOffset.Y, "F2"))
          .Append("\" relativeOffsetAdjustmentZ=\"").Append(F(relOffset.Z, "F2")).Append("\" ")
          .Append("relativeSizeAdjustmentX=\"").Append(F(relSize.X, "F2")).Append("\" relativeSizeAdjustmentY=\"").Append(F(relSize.Y, "F2")).Append("\" ");

        sb.Append("wucChildCount=\"").Append(wucChildCount.ToString(CultureInfo.InvariantCulture))
          .Append("\" shapeCount=\"").Append(shapeCount.ToString(CultureInfo.InvariantCulture))
          .Append("\" hasBrush=\"").Append(Bool(hasBrush)).Append("\" brushType=\"").Append(brushType).Append("\" ")
          .Append("hasShadow=\"").Append(Bool(hasShadow)).Append("\" shadowType=\"").Append(shadowType)
          .Append("\" hasEffect=\"").Append(Bool(hasEffect)).Append("\" hasViewBox=\"").Append(Bool(hasViewBox))
          .Append("\" sourceVisual=\"0x").Append(sourceVisual.ToString("X16", CultureInfo.InvariantCulture)).Append('"');
        return sb.ToString();
    }

    /// <summary>Map a <c>VisualType</c> name back to its kind.</summary>
    public static WucVisualKind KindFromTypeName(string? typeName)
        => !string.IsNullOrEmpty(typeName) && Enum.TryParse<WucVisualKind>(typeName, ignoreCase: false, out var kind)
           && Enum.IsDefined(kind) && kind != WucVisualKind.Unknown
            ? kind
            : WucVisualKind.Unknown;

    /// <summary>The <c>VisualType</c> name for a kind, or "" for <see cref="WucVisualKind.Unknown"/> / undefined values.</summary>
    public static string TypeNameFromKind(WucVisualKind kind)
        => kind != WucVisualKind.Unknown && Enum.IsDefined(kind) ? kind.ToString() : string.Empty;

    private static readonly float[] Identity = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };

    private (float X, float Y, float Z, float W) Get(WucVisualPropertyId property, (float, float, float, float) fallback)
        => _values.TryGetValue(property, out var v) ? v : fallback;

    private bool TryGetObject(WucVisualPropertyId property, out string typeName)
    {
        if (_objects.TryGetValue(property, out var o))
        {
            typeName = o.Type;
            return true;
        }
        typeName = string.Empty;
        return false;
    }

    // printf("%.Nf", (double)f). .NET rounds an exact decimal tie away from zero where MSVC rounds it
    // to even; the difference is display-only and limited to the last digit.
    private static string F(float value, string format) => ((double)value).ToString(format, CultureInfo.InvariantCulture);

    private static string Bool(bool value) => value ? "true" : "false";

    private static string CompositeModeName(float value) => (int)value switch
    {
        0 => "Inherit",
        1 => "SourceOver",
        2 => "DestinationInvert",
        3 => "MinBlend",
        _ => "Unknown",
    };

    private static string BorderModeName(float value) => (int)value switch
    {
        0 => "Inherit",
        1 => "Soft",
        2 => "Hard",
        _ => "Unknown",
    };

    private static string BackfaceVisibilityName(float value) => (int)value switch
    {
        0 => "Inherit",
        1 => "Visible",
        2 => "Hidden",
        _ => "Unknown",
    };
}
