using System.Runtime.InteropServices;
using System.Security.Cryptography;
using System.Text.Json;

namespace WinUIProfiler.Engine.Target;

public sealed record ConnectionPayloadHash(string Architecture, string HelperSHA256, string TapSHA256);
public sealed record ConnectionPayloadManifest(int SchemaVersion, IReadOnlyList<ConnectionPayloadHash> Payloads);

public static class ConnectionPayloadCatalog
{
    public static (string Helper, string Tap) Resolve(string directory, Architecture architecture)
    {
        var name = architecture switch
        {
            Architecture.X64 => "x64",
            Architecture.Arm64 => "ARM64",
            _ => throw new PlatformNotSupportedException("Only x64 and ARM64 targets are supported.")
        };
        var manifestPath = Path.Combine(directory, "manifest.json");
        if (new FileInfo(manifestPath).Length > 16384)
            throw new InvalidDataException("Connection payload manifest exceeds its bound.");
        var manifest = JsonSerializer.Deserialize<ConnectionPayloadManifest>(File.ReadAllBytes(manifestPath)) ??
            throw new InvalidDataException("Connection payload manifest is empty.");
        if (manifest.SchemaVersion != 1 || manifest.Payloads is null || manifest.Payloads.Count is < 1 or > 2 ||
            manifest.Payloads.Any(p => p is null || p.Architecture is not ("x64" or "ARM64")) ||
            manifest.Payloads.Select(p => p.Architecture).Distinct(StringComparer.Ordinal).Count() != manifest.Payloads.Count)
            throw new InvalidDataException("Connection payload manifest version or architecture entries are invalid.");
        var entry = manifest.Payloads.SingleOrDefault(p => p.Architecture == name) ??
            throw new FileNotFoundException($"The deployment does not contain {name} injector/TAP payloads.");
        string Verify(string file, string expected)
        {
            if (expected is null || expected.Length != 64 || !expected.All(char.IsAsciiHexDigit))
                throw new InvalidDataException("Connection payload hash is invalid.");
            var path = Path.GetFullPath(Path.Combine(directory, name, file));
            using var stream = File.OpenRead(path);
            if (!string.Equals(Convert.ToHexString(SHA256.HashData(stream)), expected, StringComparison.OrdinalIgnoreCase))
                throw new InvalidDataException($"Connection payload hash mismatch: {path}");
            return path;
        }
        return (Verify("WinUIProfiler.Injector.exe", entry.HelperSHA256), Verify("WinUIProfiler.Tap.dll", entry.TapSHA256));
    }
}
