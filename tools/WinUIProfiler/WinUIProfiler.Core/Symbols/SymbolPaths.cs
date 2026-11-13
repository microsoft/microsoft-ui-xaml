// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Text.RegularExpressions;

namespace WinUIProfiler.Engine.Symbols;

// A location SymbolReader searches: a directory of PDBs when Cache is null, else a symbol store or server whose PDBs
// are copied into Cache.
internal sealed record SymbolSource(string? Cache, string Target)
{
    public override string ToString() => Cache is null ? Target : $"srv*{Cache}*{Target}";
}

// Rewrites a symbol path, written as for WPA or _NT_SYMBOL_PATH, into the form TraceEvent's SymbolReader searches.
// Only the locations in the path are searched: _NT_SYMBOL_PATH is never read and no server is added.
// - Elements are separated by ';'. Quotes are dropped and environment variables expanded.
// - srv*cache*server downloads from the server into the cache. srv*server downloads into the path's cache: the first
//   cache*dir or srv*dir*server, else the first local srv*dir, else %TEMP%\SymbolCache.
// - srv*cache*a*b searches a, then b. SymbolReader reads one server per element, so each gets its own.
// - cache*dir is searched as a symbol store, so PDBs downloaded earlier are found.
// - A bare URL is a symbol server. A bare directory is searched flat and as a symbol store.
// - http://symweb is https://symweb.azurefd.net, which needs an Azure CLI sign-in (see SymbolServerHandler).
public static class SymbolPaths
{
    internal const string SymwebHost = "symweb.azurefd.net";

    // A URL in a symbol path or a message, up to the next separator.
    private static readonly Regex Url = new(@"https?://[^;*""\s]+", RegexOptions.IgnoreCase | RegexOptions.CultureInvariant);

    // Where downloads go when the path names no cache.
    public static string DefaultCache { get; } = Path.Combine(Path.GetTempPath(), "SymbolCache");

    // Throws ArgumentException when the path names no location or an element is malformed.
    public static string Normalize(string path, string? defaultCache = null) =>
        string.Join(';', Sources(path, defaultCache));

    // The text with what its URLs may carry as credentials hidden: user info, the query (such as a SAS token) and the
    // fragment. Use it for a symbol path that is stored or shown; searching uses the path as given.
    public static string Redact(string text)
    {
        ArgumentNullException.ThrowIfNull(text);
        return Url.Replace(text, match =>
        {
            var url = match.Value;
            var start = url.IndexOf("://", StringComparison.Ordinal) + 3;
            var end = url.IndexOfAny(['/', '?', '#'], start);
            var authority = end < 0 ? url[start..] : url[start..end];
            var rest = end < 0 ? "" : url[end..];
            var fragment = rest.IndexOf('#', StringComparison.Ordinal);
            if (fragment >= 0)
            {
                rest = rest[..fragment];
            }

            var query = rest.IndexOf('?', StringComparison.Ordinal);
            if (query >= 0)
            {
                rest = rest[..query] + (query + 1 < rest.Length ? "?<redacted>" : "");
            }

            return url[..start] + authority[(authority.LastIndexOf('@') + 1)..] + rest;
        });
    }

    internal static List<SymbolSource> Sources(string path, string? defaultCache = null)
    {
        ArgumentNullException.ThrowIfNull(path);
        var elements = path.Split(';')
            .Select(part => Environment.ExpandEnvironmentVariables(part.Trim().Trim('"').Trim()))
            .Where(part => part.Length > 0)
            .Select(Parse)
            .ToList();
        if (elements.Count == 0)
        {
            throw new ArgumentException("The symbol path names no location.", nameof(path));
        }

        var cache = ChooseCache(elements, defaultCache);
        var sources = new List<SymbolSource>();
        var seen = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        void Add(string? sourceCache, string target)
        {
            var source = new SymbolSource(sourceCache, target);
            if (seen.Add(source.ToString()))
            {
                sources.Add(source);
            }
        }

        foreach (var element in elements)
        {
            switch (element)
            {
                case ServerElement { Cache: var explicitCache, Targets.Count: > 0 } server:
                    foreach (var target in server.Targets)
                    {
                        Add(explicitCache ?? cache, target);
                    }

                    break;
                case ServerElement { Store: { } store }:
                    // A local store is searched in place; anything remote is copied into the cache.
                    Add(IsLocal(store) ? store : cache, store);
                    break;
                case CacheElement { Directory: var directory }:
                    Add(directory, directory);
                    break;
                case DirectoryElement { Location: var location }:
                    if (!IsUrl(location))
                    {
                        Add(null, location);
                    }

                    Add(IsLocal(location) ? location : cache, location);
                    break;
            }
        }

        return sources;
    }

    internal static bool IsUrl(string location) =>
        Uri.TryCreate(location, UriKind.Absolute, out var uri) &&
        (uri.Scheme == Uri.UriSchemeHttp || uri.Scheme == Uri.UriSchemeHttps);

    internal static bool IsUnc(string location) =>
        location.StartsWith(@"\\", StringComparison.Ordinal) &&
        !location.StartsWith(@"\\?\", StringComparison.Ordinal) &&
        !location.StartsWith(@"\\.\", StringComparison.Ordinal);

    private static bool IsLocal(string location) => !IsUrl(location) && !IsUnc(location);

    private abstract record Element;

    // srv*cache*target...: Targets in order, with Cache null when the element leaves it empty. srv*store has a single
    // Store and no Targets.
    private sealed record ServerElement(string? Cache, List<string> Targets, string? Store) : Element;

    private sealed record CacheElement(string Directory) : Element;

    private sealed record DirectoryElement(string Location) : Element;

    private static Element Parse(string text)
    {
        var segments = text.Split('*').Select(segment => segment.Trim()).ToArray();
        var prefix = segments[0];
        if (segments.Length > 1 && prefix.Equals("srv", StringComparison.OrdinalIgnoreCase))
        {
            return Server(segments[1..], text);
        }

        // symsrv*symsrv.dll*cache*server names the DLL that reads the store; SymbolReader is the reader here.
        if (segments.Length > 2 && prefix.Equals("symsrv", StringComparison.OrdinalIgnoreCase))
        {
            return Server(segments[2..], text);
        }

        if (segments.Length == 2 && segments[1].Length > 0 && prefix.Equals("cache", StringComparison.OrdinalIgnoreCase))
        {
            return new CacheElement(CacheDirectory(segments[1], text));
        }

        if (segments.Length > 1)
        {
            throw new ArgumentException(
                $"'{Redact(text)}' is not a symbol path element: use a directory, a URL, srv*cache*server or cache*directory.");
        }

        return new DirectoryElement(Location(text));
    }

    private static ServerElement Server(string[] segments, string text)
    {
        var targets = segments[1..].Where(segment => segment.Length > 0).Select(Location).ToList();
        if (targets.Count > 0)
        {
            return new ServerElement(segments[0].Length > 0 ? CacheDirectory(segments[0], text) : null, targets, null);
        }

        // srv*store, or srv*store* with nothing after the last '*'.
        if (segments[0].Length == 0)
        {
            throw new ArgumentException($"'{Redact(text)}' names no symbol server or store.");
        }

        return new ServerElement(null, [], Location(segments[0]));
    }

    private static string ChooseCache(List<Element> elements, string? defaultCache)
    {
        foreach (var element in elements)
        {
            switch (element)
            {
                case CacheElement { Directory: var directory }:
                    return directory;
                case ServerElement { Cache: { } cache }:
                    return cache;
            }
        }

        foreach (var element in elements)
        {
            if (element is ServerElement { Store: { } store } && IsLocal(store))
            {
                return store;
            }
        }

        return Location(defaultCache ?? DefaultCache);
    }

    // A cache must be a directory, local or on a share.
    private static string CacheDirectory(string segment, string text) =>
        IsUrl(segment)
            ? throw new ArgumentException(
                $"In '{Redact(text)}', the cache {Redact(segment)} is a URL; a cache must be a local or network directory.")
            : Location(segment);

    private static string Location(string text)
    {
        if (Uri.TryCreate(text, UriKind.Absolute, out var uri) &&
            (uri.Scheme == Uri.UriSchemeHttp || uri.Scheme == Uri.UriSchemeHttps))
        {
            if (uri.Host.Equals("symweb", StringComparison.OrdinalIgnoreCase) ||
                uri.Host.Equals(SymwebHost, StringComparison.OrdinalIgnoreCase))
            {
                text = $"https://{SymwebHost}{uri.PathAndQuery}";
            }

            // SymbolReader appends '/' and the PDB's index to the path, before any query (such as a SAS token).
            var query = text.IndexOf('?', StringComparison.Ordinal);
            return query < 0 ? text.TrimEnd('/') : text[..query].TrimEnd('/') + text[query..];
        }

        return Path.TrimEndingDirectorySeparator(IsUnc(text) ? text : Path.GetFullPath(text));
    }
}
