namespace WinUIProfiler.Engine.Storage;

public sealed partial class SessionStore
{
    /// <summary>
    /// Opens a closed, checkpointed session without creating or changing SQLite sidecars.
    /// Windows read-only sharing leases exclude writers for the lifetime of the store.
    /// Use OpenReadOnly instead when concurrent live recording is required.
    /// </summary>
    public static SessionStore OpenSavedReadOnly(string path)
    {
        if (!OperatingSystem.IsWindows()) throw new PlatformNotSupportedException("Saved immutable leases require Windows file sharing.");
        path = Existing(path);
        var streams = new List<FileStream>();
        try
        {
            streams.Add(new(path, FileMode.Open, FileAccess.Read, FileShare.Read));
            foreach (var suffix in new[] { ".writer", "-wal", "-shm", ".failure.json" })
                if (File.Exists(path + suffix))
                    streams.Add(new(path + suffix, FileMode.Open, FileAccess.Read, FileShare.Read));
            // SQLite immutable readers ignore WAL contents; never hide committed data.
            if (File.Exists(path + "-wal") && new FileInfo(path + "-wal").Length != 0)
                throw new InvalidDataException("Saved immutable queries require a closed, checkpointed database without a nonempty WAL.");
            using var db = Connect(path, readOnly: true, immutable: true);
            ValidateDatabase(db);
            return new(path, db, null) { immutableReaders = true, savedReadLeases = streams };
        }
        catch
        {
            foreach (var stream in streams) stream.Dispose();
            throw;
        }
    }
}
