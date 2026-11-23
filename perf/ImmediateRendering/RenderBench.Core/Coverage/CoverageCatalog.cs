using System.Reflection;
using System.Text.Json;
using System.Text.Json.Serialization;
using System.Text.RegularExpressions;

namespace RenderBench.Coverage;

// One page of the XAML coverage corpus as declared by its [CoveragePage] attribute (RenderBench\Coverage\
// CoverageContract.cs). The attribute, CoverageContext and CoveragePage types live in the WinUI app; discovery here
// matches them by name so it can run (and be tested) without loading XAML.
public sealed record CoveragePageInfo(string Id, string Family, string Name, string[] ExpectedUnsupported, bool ExpectFallback,
    double MaxMeanAbsDiff)
{
    [JsonIgnore] public MethodInfo? Method { get; init; }
    // Family number, e.g. 8 for "F8-controls".
    public int FamilyNumber => int.TryParse(Regex.Match(Family, @"^F(\d+)").Groups[1].Value, out int n) ? n : 0;
    // F11 (interop) is measured separately: those pages are expected to fall back.
    public bool IsInterop => FamilyNumber == 11 || ExpectFallback;
}

public static partial class CoverageCatalog
{
    public const string AttributeTypeName = "CoveragePageAttribute";
    public const string ContextTypeName = "CoverageContext";
    public const string PageTypeName = "CoveragePage";

    [GeneratedRegex(@"^F\d+-[A-Za-z0-9-]+$")] private static partial Regex FamilyPattern();

    // Every static method carrying a CoveragePageAttribute in `types`. Malformed declarations (wrong signature, bad
    // family id, duplicate id, threshold outside (0,1]) are reported in Errors and left out of Pages.
    public static (List<CoveragePageInfo> Pages, List<string> Errors) Discover(IEnumerable<Type> types)
    {
        var pages = new List<CoveragePageInfo>();
        var errors = new List<string>();
        var ids = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        const BindingFlags flags = BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Static | BindingFlags.Instance |
            BindingFlags.DeclaredOnly;
        foreach (var type in types)
        {
            foreach (var method in type.GetMethods(flags))
            {
                var attribute = method.GetCustomAttributes(inherit: false).FirstOrDefault(a => a.GetType().Name == AttributeTypeName);
                if (attribute is null)
                {
                    continue;
                }
                string where = $"{type.FullName}.{method.Name}";
                string family = Read<string>(attribute, "Family") ?? "";
                string name = Read<string>(attribute, "Name") ?? "";
                var info = new CoveragePageInfo($"{family}/{name}", family, name, Read<string[]>(attribute, "ExpectedUnsupported") ?? [],
                    Read<bool>(attribute, "ExpectFallback"), Read<double>(attribute, "MaxMeanAbsDiff")) { Method = method };
                var parameters = method.GetParameters();
                if (!method.IsStatic)
                {
                    errors.Add($"{where}: coverage pages must be static");
                }
                else if (parameters.Length != 1 || parameters[0].ParameterType.Name != ContextTypeName || method.ReturnType.Name != PageTypeName)
                {
                    errors.Add($"{where}: expected 'static {PageTypeName} {method.Name}({ContextTypeName} context)'");
                }
                else if (!FamilyPattern().IsMatch(family))
                {
                    errors.Add($"{where}: family '{family}' should look like 'F8-controls'");
                }
                else if (name.Length == 0 || name.IndexOfAny(['/', '\\', '*', '?', ',', ' ']) >= 0)
                {
                    errors.Add($"{where}: name '{name}' must be non-empty without / \\ * ? , or spaces");
                }
                else if (!(info.MaxMeanAbsDiff > 0 && info.MaxMeanAbsDiff <= 1))
                {
                    errors.Add($"{where}: MaxMeanAbsDiff {info.MaxMeanAbsDiff} must be in (0, 1]");
                }
                else if (!ids.Add(info.Id))
                {
                    errors.Add($"{where}: duplicate page id '{info.Id}'");
                }
                else
                {
                    pages.Add(info);
                }
            }
        }
        pages.Sort((a, b) => a.FamilyNumber != b.FamilyNumber ? a.FamilyNumber.CompareTo(b.FamilyNumber) : string.CompareOrdinal(a.Id, b.Id));
        return (pages, errors);
    }

    static T? Read<T>(object attribute, string property)
        => attribute.GetType().GetProperty(property)?.GetValue(attribute) is T value ? value : default;

    // Comma-separated globs (* and ?), each matched against the page id ("F8-controls/Button"), its family
    // ("F8-controls") or the family number ("F8"). Null, empty or "*" selects everything; "!glob" excludes.
    public static List<CoveragePageInfo> Select(IEnumerable<CoveragePageInfo> pages, string? filter)
    {
        var patterns = (filter ?? "").Split(',', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries);
        var include = patterns.Where(p => !p.StartsWith('!')).ToArray();
        var exclude = patterns.Where(p => p.StartsWith('!')).Select(p => p[1..]).ToArray();
        bool Matches(CoveragePageInfo page, string pattern) =>
            GlobMatch(pattern, page.Id) || GlobMatch(pattern, page.Family) || GlobMatch(pattern, $"F{page.FamilyNumber}");
        return pages.Where(p => (include.Length == 0 || include.Any(i => Matches(p, i))) && !exclude.Any(e => Matches(p, e))).ToList();
    }

    public static bool GlobMatch(string pattern, string text)
        => Regex.IsMatch(text, "^" + Regex.Escape(pattern).Replace(@"\*", ".*").Replace(@"\?", ".") + "$", RegexOptions.IgnoreCase);

    // Results directory of one page: <family>\<name>.
    public static string RelativeDirectory(CoveragePageInfo page) => Path.Combine(page.Family, page.Name);

    static readonly JsonSerializerOptions s_json = new() { WriteIndented = true };

    public static string Serialize(IEnumerable<CoveragePageInfo> pages) => JsonSerializer.Serialize(pages.ToList(), s_json);

    public static List<CoveragePageInfo> Deserialize(string json) => JsonSerializer.Deserialize<List<CoveragePageInfo>>(json) ?? [];
}
