using System;
using System.IO;

namespace AppBootingVisualizerPlugin.Services
{
    /// <summary>
    /// Lightweight per-load diagnostic logger. Writes to
    /// <c>%TEMP%\AppBootingVisualizerPlugin.log</c>. The file is overwritten at the
    /// start of each trace load (call <see cref="Reset"/>) and appended to during
    /// processing. Designed for debugging "no data" scenarios in WPA, where we
    /// don't have an interactive console.
    /// </summary>
    internal static class Diag
    {
        private static readonly object _lock = new object();
        private static readonly string LogPath = System.IO.Path.Combine(System.IO.Path.GetTempPath(), "AppBootingVisualizerPlugin.log");

        public static string FilePath => LogPath;

        public static void Reset()
        {
            lock (_lock)
            {
                try
                {
                    File.WriteAllText(LogPath, "[" + DateTime.Now.ToString("o") + "] Trace load started\n");
                }
                catch
                {
                }
            }
        }

        public static void Log(string message)
        {
            lock (_lock)
            {
                try
                {
                    File.AppendAllText(LogPath, "[" + DateTime.Now.ToString("HH:mm:ss.fff") + "] " + message + "\n");
                }
                catch
                {
                }
            }
        }
    }
}
