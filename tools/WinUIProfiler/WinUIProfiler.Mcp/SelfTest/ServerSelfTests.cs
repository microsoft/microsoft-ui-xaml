// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Text.Json;
using System.Text.RegularExpressions;

namespace WinUIProfiler.Mcp.SelfTest;

/// <summary>Checks the server's version, the plugin manifests, the tool descriptions and the skills.</summary>
internal static partial class ServerSelfTests
{
    private const string ScriptPrefix = "${PLUGIN_ROOT}/";
    private const string SkillsFolder = "skills/";
    private const int MaxSkillNameLength = 64;
    private const int MaxSkillDescriptionLength = 1024;
    private const int MaxSkillLines = 500;

    // The command in the README's and the skills' code blocks that writes the providers a trace is recorded with.
    private const string ProvidersCommand = "Set-Content providers.txt";

    public static IEnumerable<SelfTestCase> Tests =>
    [
        new("server.version", _ => Version()),
        new("server.plugin-manifest", _ => PluginManifest()),
        new("server.tools", _ => Tools()),
        new("server.skills", _ => Skills()),
        new("server.providers", _ => Providers()),
    ];

    private static void Version()
    {
        var parts = ServerVersion.Value.Split('.');
        Expect.True(
            parts.Length == 3 && parts.All(part => part.Length > 0 && part.All(char.IsAsciiDigit)),
            $"The server version '{ServerVersion.Value}' has the form major.minor.patch.");
    }

    private static void PluginManifest()
    {
        var root = FindPluginRoot()
            ?? throw Expect.Skip($"No plugin.json named '{ServerVersion.ServerName}' above the current folder or the server's folder.");

        using var plugin = JsonDocument.Parse(File.ReadAllText(Path.Combine(root, "plugin.json")));
        var manifest = plugin.RootElement;
        Expect.Equal(ServerVersion.Value, manifest.GetProperty("version").GetString(), "plugin.json version (keep it equal to VersionPrefix in WinUIProfiler.Mcp.csproj)");

        var server = manifest.GetProperty("mcpServers").GetProperty(ServerVersion.ServerName);
        var script = server.GetProperty("args").EnumerateArray()
            .Select(argument => argument.GetString())
            .SingleOrDefault(argument => argument?.StartsWith(ScriptPrefix, StringComparison.Ordinal) == true);
        Expect.True(script is not null, $"The '{ServerVersion.ServerName}' server in plugin.json runs a script under {ScriptPrefix}.");
        Expect.True(File.Exists(Path.Combine(root, script![ScriptPrefix.Length..])), $"{script} exists.");

        using var marketplace = JsonDocument.Parse(File.ReadAllText(Path.Combine(root, "marketplace.json")));
        var entries = marketplace.RootElement.GetProperty("plugins").EnumerateArray()
            .Where(entry => entry.GetProperty("name").GetString() == ServerVersion.ServerName)
            .ToList();
        Expect.Equal(1, entries.Count, $"marketplace.json entries named '{ServerVersion.ServerName}'");
        Expect.Equal("./", entries[0].GetProperty("source").GetString(), "marketplace.json source");
    }

    private static void Tools()
    {
        var names = new HashSet<string>(StringComparer.Ordinal);
        foreach (var tool in Program.ListTools().Select(tool => tool.ProtocolTool))
        {
            Expect.True(names.Add(tool.Name), $"Only one tool is named '{tool.Name}'.");
            Expect.True(
                tool.Name.Length > 0 && char.IsAsciiLetterLower(tool.Name[0]) && tool.Name.All(c => char.IsAsciiLetterLower(c) || char.IsAsciiDigit(c) || c == '_'),
                $"The tool name '{tool.Name}' is lower-case letters, digits and underscores.");
            Expect.True(!string.IsNullOrWhiteSpace(tool.Description), $"{tool.Name} has a description.");
            Expect.True(tool.Annotations?.ReadOnlyHint is not null, $"{tool.Name} says whether it's read-only.");
            if (tool.InputSchema.TryGetProperty("properties", out var properties))
            {
                foreach (var parameter in properties.EnumerateObject())
                {
                    Expect.True(
                        parameter.Value.TryGetProperty("description", out var description) && !string.IsNullOrWhiteSpace(description.GetString()),
                        $"{tool.Name}.{parameter.Name} has a description.");
                }
            }
        }
    }

    /// <summary>
    /// Checks each skill's front matter, and that the skills' text names only tools the server has and
    /// links only to files that exist. Code blocks aren't checked.
    /// </summary>
    private static void Skills()
    {
        var root = FindPluginRoot()
            ?? throw Expect.Skip($"No plugin.json named '{ServerVersion.ServerName}' above the current folder or the server's folder.");

        using (var plugin = JsonDocument.Parse(File.ReadAllText(Path.Combine(root, "plugin.json"))))
        {
            Expect.Equal(SkillsFolder, plugin.RootElement.GetProperty("skills").GetString(), "plugin.json skills");
        }

        var skills = Path.Combine(root, SkillsFolder);
        var folders = Directory.GetDirectories(skills).Order(StringComparer.Ordinal).ToList();
        Expect.True(folders.Count > 0, $"{skills} has a folder for each skill.");
        foreach (var folder in folders)
        {
            var name = Path.GetFileName(folder);
            var where = $"{SkillsFolder}{name}/SKILL.md";
            var file = Path.Combine(folder, "SKILL.md");
            Expect.True(File.Exists(file), $"{where} exists.");

            var lines = File.ReadAllLines(file);
            Expect.True(lines.Length < MaxSkillLines, $"{where} has fewer than {MaxSkillLines} lines; it has {lines.Length}.");
            var fields = ReadFrontMatter(lines, where);
            Expect.Equal(name, fields.GetValueOrDefault("name"), $"{where} name (its folder's name)");
            Expect.True(
                name.Length <= MaxSkillNameLength && SkillName().IsMatch(name),
                $"The skill name '{name}' is at most {MaxSkillNameLength} lower-case letters and digits, in words joined by hyphens.");
            var description = fields.GetValueOrDefault("description") ?? string.Empty;
            Expect.True(
                description.Trim().Length > 0 && description.Length <= MaxSkillDescriptionLength,
                $"{where} has a description of 1 to {MaxSkillDescriptionLength} characters; it has {description.Length}.");
        }

        var tools = Program.ListTools().Select(tool => tool.ProtocolTool.Name).ToHashSet(StringComparer.Ordinal);
        foreach (var file in Directory.GetFiles(skills, "*.md", SearchOption.AllDirectories).Order(StringComparer.Ordinal))
        {
            var where = Path.GetRelativePath(root, file).Replace('\\', '/');
            var folder = Path.GetDirectoryName(file)!;
            var inCodeBlock = false;
            var number = 0;
            foreach (var line in File.ReadLines(file))
            {
                number++;
                if (line.TrimStart().StartsWith("```", StringComparison.Ordinal))
                {
                    inCodeBlock = !inCodeBlock;
                    continue;
                }

                if (inCodeBlock)
                {
                    continue;
                }

                foreach (Match code in CodeSpan().Matches(line))
                {
                    var text = code.Groups["text"].Value;
                    Expect.True(
                        !ToolLikeName().IsMatch(text) || tools.Contains(text),
                        $"{where}:{number} names `{text}`, which isn't one of the server's tools.");
                }

                foreach (Match link in Link().Matches(line))
                {
                    var target = link.Groups["target"].Value;
                    if (target.StartsWith('#') || target.Contains("://", StringComparison.Ordinal))
                    {
                        continue;
                    }

                    Expect.True(
                        File.Exists(Path.Combine(folder, target.Split('#')[0])),
                        $"{where}:{number} links to {target}, which doesn't exist.");
                }
            }

            Expect.True(!inCodeBlock, $"{where} closes every code block.");
        }
    }

    /// <summary>Checks that each skill that shows how to record a trace enables the same providers as the README.</summary>
    private static void Providers()
    {
        var root = FindPluginRoot()
            ?? throw Expect.Skip($"No plugin.json named '{ServerVersion.ServerName}' above the current folder or the server's folder.");

        var readme = Path.Combine(root, "README.md");
        var expected = File.Exists(readme) ? RecipeProviders(readme) : null;
        Expect.True(expected is { Count: > 0 }, $"{readme} has a code block that writes providers.txt.");

        var compared = 0;
        foreach (var file in Directory.GetFiles(Path.Combine(root, SkillsFolder), "*.md", SearchOption.AllDirectories)
            .Order(StringComparer.Ordinal))
        {
            if (RecipeProviders(file) is { } actual)
            {
                compared++;
                Expect.Equal(string.Join(" | ", expected!), string.Join(" | ", actual),
                    $"The providers {Path.GetRelativePath(root, file).Replace('\\', '/')} records a trace with (keep them equal to " +
                    "the README's)");
            }
        }

        Expect.True(compared > 0, $"A skill under {SkillsFolder} has a code block that writes providers.txt.");
    }

    /// <summary>The provider lines of a file's code block that writes providers.txt; null when it has no such block.</summary>
    private static List<string>? RecipeProviders(string file)
    {
        var block = new List<string>();
        var inCodeBlock = false;
        foreach (var line in File.ReadLines(file))
        {
            if (line.TrimStart().StartsWith("```", StringComparison.Ordinal))
            {
                if (inCodeBlock && block.Any(text => text.Contains(ProvidersCommand, StringComparison.Ordinal)))
                {
                    return block.SelectMany(text => ProviderLine().Matches(text)).Select(match => match.Value).ToList();
                }

                inCodeBlock = !inCodeBlock;
                block.Clear();
                continue;
            }

            if (inCodeBlock)
            {
                block.Add(line);
            }
        }

        return null;
    }

    /// <summary>
    /// Reads the "key: value" lines between a skill's two "---" lines. A value of "&gt;-", "&gt;", "|" or "|-"
    /// starts a block of the indented lines that follow, joined with spaces ("&gt;") or line breaks ("|").
    /// </summary>
    private static Dictionary<string, string> ReadFrontMatter(string[] lines, string where)
    {
        Expect.True(lines.Length > 0 && lines[0] == "---", $"{where} starts with front matter (a '---' line).");
        var fields = new Dictionary<string, string>(StringComparer.Ordinal);
        string? blockKey = null;
        var blockLines = new List<string>();
        var literal = false;

        void EndBlock()
        {
            if (blockKey is not null)
            {
                fields[blockKey] = string.Join(literal ? "\n" : " ", blockLines.Where(line => line.Length > 0));
            }

            blockKey = null;
            blockLines.Clear();
        }

        for (var index = 1; index < lines.Length; index++)
        {
            var line = lines[index];
            if (line == "---")
            {
                EndBlock();
                return fields;
            }

            if (blockKey is not null && (line.Length == 0 || char.IsWhiteSpace(line[0])))
            {
                blockLines.Add(line.Trim());
                continue;
            }

            EndBlock();
            var colon = line.IndexOf(':');
            Expect.True(colon > 0, $"{where}:{index + 1} is a 'key: value' line.");
            var key = line[..colon].Trim();
            var value = line[(colon + 1)..].Trim();
            Expect.True(fields.TryAdd(key, value), $"{where} sets {key} once.");
            if (value is ">-" or ">" or "|" or "|-")
            {
                blockKey = key;
                literal = value[0] == '|';
            }
        }

        throw new SelfTestFailedException($"{where} ends its front matter with a '---' line.");
    }

    [GeneratedRegex("^[a-z0-9]+(-[a-z0-9]+)*$", RegexOptions.CultureInvariant)]
    private static partial Regex SkillName();

    [GeneratedRegex("^[a-z][a-z0-9]*(_[a-z0-9]+)+$", RegexOptions.CultureInvariant)]
    private static partial Regex ToolLikeName();

    [GeneratedRegex("`(?<text>[^`]+)`", RegexOptions.CultureInvariant)]
    private static partial Regex CodeSpan();

    [GeneratedRegex(@"\]\((?<target>[^)\s]+)\)", RegexOptions.CultureInvariant)]
    private static partial Regex Link();

    // A logman provider line: the provider's GUID, its keywords and its level.
    [GeneratedRegex(@"\{[0-9A-Fa-f-]{36}\} 0x[0-9A-Fa-f]+ \d+", RegexOptions.CultureInvariant)]
    private static partial Regex ProviderLine();

    /// <summary>
    /// Finds the plugin folder: the one Copilot CLI names in PLUGIN_ROOT, or the first folder above
    /// the current folder or the server's folder that holds this server's plugin.json.
    /// </summary>
    internal static string? FindPluginRoot()
    {
        var starts = new[] { Environment.GetEnvironmentVariable("PLUGIN_ROOT"), Environment.CurrentDirectory, AppContext.BaseDirectory };
        foreach (var start in starts)
        {
            for (var folder = string.IsNullOrEmpty(start) ? null : new DirectoryInfo(start); folder is not null; folder = folder.Parent)
            {
                var manifest = Path.Combine(folder.FullName, "plugin.json");
                if (File.Exists(manifest) && IsThisPlugin(manifest))
                {
                    return folder.FullName;
                }
            }
        }

        return null;
    }

    private static bool IsThisPlugin(string manifest)
    {
        try
        {
            using var document = JsonDocument.Parse(File.ReadAllText(manifest));
            return document.RootElement.ValueKind == JsonValueKind.Object
                && document.RootElement.TryGetProperty("name", out var name)
                && name.ValueKind == JsonValueKind.String
                && name.GetString() == ServerVersion.ServerName;
        }
        catch (JsonException)
        {
            return false;
        }
    }
}
