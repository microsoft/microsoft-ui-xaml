// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Text;
using Windows.Storage;

namespace WindowPlacementApp
{
    // Reads WinUI's private placement record straight from LocalSettings, for debugging only.
    // Mirrors PlacementRecord in dxaml\xcp\dxaml\lib\WindowPlacement.cpp (version 1, 152-byte header).
    public static class SavedRecord
    {
        private const string Container = "Microsoft.UI.Xaml.WindowPlacement";
        private const int HeaderSize = 152;

        private static readonly string[] s_states =
        {
            "Normal", "Maximized", "MinimizedFromNormal", "Snapped",
            "MinimizedFromMaximized", "MinimizedFromSnapped",
        };

        // Same 64-bit FNV-1a over UTF-16LE bytes that WinUI uses.
        public static string KeyFor(string id)
        {
            ulong hash = 14695981039346656037ul;
            foreach (char c in id)
            {
                hash = (hash ^ (byte)(c & 0xff)) * 1099511628211ul;
                hash = (hash ^ (byte)(c >> 8)) * 1099511628211ul;
            }
            return $"P_{hash:x16}";
        }

        public static string Describe(string id)
        {
            if (string.IsNullOrEmpty(id))
            {
                return "(no PlacementId)";
            }
            string key = KeyFor(id);
            if (!ApplicationData.Current.LocalSettings.Containers.TryGetValue(Container, out ApplicationDataContainer container) ||
                !container.Values.TryGetValue(key, out object value))
            {
                return $"key={key}\n(nothing saved)";
            }
            if (value is not byte[] bytes || bytes.Length < HeaderSize)
            {
                return $"key={key}\n(unexpected value: {value?.GetType().Name})";
            }

            var sb = new StringBuilder();
            sb.AppendLine($"key={key} size={bytes.Length}");
            sb.AppendLine($"version={BitConverter.ToUInt32(bytes, 0)}");
            sb.AppendLine($"savedAt={DateTime.FromFileTimeUtc(BitConverter.ToInt64(bytes, 4)).ToLocalTime():yyyy-MM-dd HH:mm:ss.fff}");
            sb.AppendLine($"virtualDesktopId={new Guid(bytes.AsSpan(12, 16))}");
            sb.AppendLine($"normalRect={Rect(bytes, 28)}");
            sb.AppendLine($"workArea={Rect(bytes, 44)}");
            sb.AppendLine($"snapRect={Rect(bytes, 60)}");
            sb.AppendLine($"dpi={BitConverter.ToUInt32(bytes, 76)}");
            uint state = BitConverter.ToUInt32(bytes, 80);
            sb.AppendLine($"state={state} ({(state < s_states.Length ? s_states[state] : "?")})");
            sb.AppendLine($"deviceName={Encoding.Unicode.GetString(bytes, 84, 64).TrimEnd('\0')}");
            uint idLength = BitConverter.ToUInt32(bytes, 148);
            string storedId = Encoding.Unicode.GetString(bytes, HeaderSize, Math.Min((int)idLength * 2, bytes.Length - HeaderSize));
            sb.Append($"id=\"{storedId}\" (length {idLength})");
            return sb.ToString();
        }

        // Stored as left, top, right, bottom. Shown as x,y wxh to match the other panels.
        private static string Rect(byte[] b, int offset)
        {
            int l = BitConverter.ToInt32(b, offset), t = BitConverter.ToInt32(b, offset + 4);
            int r = BitConverter.ToInt32(b, offset + 8), bottom = BitConverter.ToInt32(b, offset + 12);
            return $"({l},{t} {r - l}x{bottom - t})";
        }
    }
}
