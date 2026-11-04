using System;
using System.IO;
using Windows.Storage;

namespace ExceptionContextLab;

internal sealed class SessionLog
{
    private readonly string _path = Path.Combine(ApplicationData.Current.LocalFolder.Path, "sessions.log");

    internal void Write(string message) => File.AppendAllText(_path,
        $"{DateTimeOffset.UtcNow:O} pid={Environment.ProcessId} {message}{Environment.NewLine}");
}
