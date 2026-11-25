// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Runtime.InteropServices;
using Microsoft.Data.Sqlite;
using WinUIProfiler.Engine.Events;
using WinUIProfiler.Engine.Ingestion;
using WinUIProfiler.Engine.Storage;

internal static class CpuStorageFailureTests
{
    internal static void Run()
    {
        var directory = Path.Combine(Path.GetTempPath(), $"WinUIProfiler-CpuFailures-{Guid.NewGuid():N}");
        Directory.CreateDirectory(directory);
        var paths = new[] { "initialize", "publish", "clock", "retention", "samples", "finish" }
            .Select(name => Path.Combine(directory, name + ".sqlite")).ToArray();
        try
        {
            InitializationFailure(paths[0]);
            PublicationFailure(paths[1]);
            CanonicalClock(paths[2]);
            RetentionFailure(paths[3]);
            SampleWriteFailure(paths[4]);
            TrimmedFinishFailure(paths[5]);
            Console.WriteLine("Passed: CPU WAL failures preserve ordered publication, canonical timing, retained samples, storage sizes and loss notes.");
        }
        finally
        {
            foreach (var file in paths.SelectMany(path => new[] { path, CpuSchema.PathFor(path) })
                .SelectMany(path => new[] { path, path + "-wal", path + "-shm", path + "-journal" }))
            {
                File.Delete(file);
            }

            Directory.Delete(directory);
        }
    }

    private static void InitializationFailure(string path)
    {
        using var fault = new WriteFault(path, cpu: true);
        using var writer = Create(path);
        fault.Arm();
        Sample(writer, 1_000);
        writer.Close("Test complete");
        Failed(writer, fault);
        using var main = SessionFile.Open(path, includeCpu: true);
        Expect(Sql.Scalar(main, "SELECT COUNT(*) FROM SessionInfo WHERE CpuDatabaseVersion IS NOT NULL") == 0 &&
            CpuProfileReader.ReadSampling(path) is null,
            "A failed CPU initialization must not publish a CPU requirement in the main recording.");
    }

    private static void PublicationFailure(string path)
    {
        using var fault = new WriteFault(path, cpu: false);
        using var writer = Create(path);
        fault.Arm();
        Sample(writer, 1_000);
        writer.Close("Test complete");
        Failed(writer, fault);
        using var main = SessionFile.Open(path);
        CpuSchema.Attach(main, CpuSchema.PathFor(path));
        Expect(fault.CpuFlushedBeforeFailure &&
            Sql.Scalar(main, "PRAGMA cpu.user_version") == CpuSchema.Version &&
            Sql.Scalar(main, "SELECT COUNT(*) FROM main.SessionInfo s JOIN cpu.CpuSessionInfo c USING (SessionId)") == 1 &&
            Sql.Scalar(main, "SELECT COUNT(*) FROM main.SessionInfo WHERE CpuDatabaseVersion IS NOT NULL") == 0,
            "CPU initialization must be flushed before publishing it; a failed publication must leave a valid, unadvertised CPU file.");
    }

    private static void CanonicalClock(string path)
    {
        using var fault = new WriteFault(path, cpu: true);
        using var writer = SessionWriter.Create(path, new(1, "CpuFailures", null, null, null, SessionCoverage.Trace));
        Sample(writer, 1_000_000);
        Wait(writer, () => CpuProfileReader.ReadSampling(path)?.Samples == 1);
        fault.Arm();
        writer.Process(1, new(1_000_000, 1000, 1, 5, XamlProvider.Xaml, true, "Clock", 0, []));
        writer.Process(2, new(2_000_000, 2000, 1, 5, XamlProvider.Xaml, true, "Clock", 0, []));
        Wait(writer, () => writer.Status.StoredEvents == 2);
        Expect(!fault.Fired, "Deriving the recording clock must not write to the CPU database.");
        fault.Disarm();
        writer.Close("Test complete");
        using var connection = SessionFile.Open(path, includeCpu: true);
        Expect(writer.Failure is null &&
            Sql.Scalar(connection, "SELECT QpcFrequency FROM main.SessionInfo") == 1_000_000 &&
            Sql.Scalar(connection, "SELECT CAST(Ms AS INTEGER) FROM CpuSampleDetails") == 0,
            "Reopened CPU readers must calculate sample times using the recording's canonical clock.");
    }

    private static void RetentionFailure(string path)
    {
        using var fault = new WriteFault(path, cpu: true);
        using var writer = Create(path, limitBytes: 1024 * 1024);
        Events(writer, first: 1, count: 350, padding: 1_000);
        Wait(writer, () => writer.Status.StoredEvents == 350);
        Sample(writer, 1_001);
        Sample(writer, 5_000);
        Wait(writer, () => CpuProfileReader.ReadSampling(path)?.Samples == 2);
        fault.Arm();
        Events(writer, first: 351, count: 400, padding: 2_000);
        writer.Close("Test complete");
        Failed(writer, fault);
        using var connection = SessionFile.Open(path, includeCpu: true);
        Expect(Sql.Scalar(connection, "SELECT CompleteFromQpc FROM main.SessionInfo") > 1_001 &&
            writer.Status.StoredEvents == Sql.Scalar(connection, "SELECT COUNT(*) FROM main.Events") &&
            Sql.Scalar(connection, "SELECT COUNT(*) FROM cpu.CpuSamples") == 2 &&
            Sql.Scalar(connection, "SELECT COUNT(*) FROM CpuSampleDetails") == 1 &&
            Sql.Scalar(connection, "SELECT SUM(Samples) FROM HotFunctions") == 1 &&
            CpuProfileReader.ReadSampling(path) is { Samples: 1, Note: CpuProfileReader.FilteredNote } &&
            CpuProfileReader.ReadStacks(path) is { Samples: 1 },
            "Failed CPU cleanup must leave the committed event boundary authoritative, filter stale samples and report their omission.");
    }

    private static void SampleWriteFailure(string path)
    {
        using var fault = new WriteFault(path, cpu: true);
        using var writer = Create(path, limitBytes: 1024 * 1024);
        Events(writer, first: 1, count: 500, padding: 1_000);
        Wait(writer, () => writer.Status.StoredEvents == 500);
        Sample(writer, 1_000_000);
        Wait(writer, () => CpuProfileReader.ReadSampling(path)?.Samples == 1 && writer.Status.FileBytes == SavedBytes());
        var originalStatus = writer.Status;
        var sample = new CompletedSample(2_000_000, 1, 5, [new(null, 0xDEF0)]);
        // One large stack forces event retention regardless of how the writer batches queued samples.
        sample = sample with { Frames = Enumerable.Repeat(sample.Frames[0], 8_000).ToArray() };
        fault.Arm();
        writer.PostSample(sample, wait: true);
        Expect(SpinWait.SpinUntil(() => writer.Failure is not null, TimeSpan.FromSeconds(10)),
            "The sample commit must reach the injected CPU WAL failure.");
        var error = writer.Failure;
        var failedStatus = writer.Status;
        writer.Close("Test complete");
        Failed(writer, fault);
        var bytes = SavedBytes();
        Expect(bytes < originalStatus.FileBytes && writer.Status.StoredEvents < 500,
            "Event retention must commit and reclaim pages before the CPU sample write fails.");
        Expect(failedStatus.FileBytes == bytes && writer.Status.FileBytes == bytes,
            $"A failed sample commit must report {bytes} committed bytes, not {failedStatus.FileBytes}.");
        Expect(ReferenceEquals(writer.Failure, error) && CpuProfileReader.ReadSampling(path) is { Samples: 1 },
            "Size refresh and Close must preserve the original error and the last committed CPU samples.");

        long SavedBytes()
        {
            using var connection = SessionFile.Open(path, includeCpu: true);
            return (Sql.Scalar(connection, "PRAGMA main.page_count") + Sql.Scalar(connection, "PRAGMA cpu.page_count")) *
                Sql.Scalar(connection, "PRAGMA main.page_size");
        }
    }

    private static void TrimmedFinishFailure(string path)
    {
        using var fault = new WriteFault(path, cpu: true);
        using var writer = Create(path, limitBytes: 1024 * 1024);
        Expect(!LastSampleWritten(), "Polling must allow a recording whose CPU database has not been published yet.");
        for (var index = 0; index < 16_000; index++)
        {
            writer.PostSample(new(1_000 + index, 1, 5, [new(null, 0xABC0UL + (ulong)index)]), wait: true);
        }

        Wait(writer, LastSampleWritten);
        Expect(CpuProfileReader.ReadSampling(path) is { Samples: > 0 and < 16_000, Note: CpuSampleIndex.TrimmedNote },
            "Committed CPU-only trimming must report lost samples even before finalization.");
        writer.SetCpuSampling(new(1.0, false, 16_000, []), null);
        fault.Arm();
        writer.Close("Test complete");
        Failed(writer, fault);
        Expect(CpuProfileReader.ReadSampling(path) is { Samples: > 0 and < 16_000, Note: CpuSampleIndex.TrimmedNote },
            "A failed finalization must not lose the durable CPU trimming note.");

        bool LastSampleWritten()
        {
            using var connection = SessionFile.Open(path, includeCpu: true);
            return Sql.Scalar(connection, "SELECT COUNT(*) FROM pragma_database_list WHERE name = 'cpu'") != 0 &&
                Sql.Scalar(connection, "SELECT MAX(Qpc) FROM cpu.CpuSamples") == 16_999;
        }
    }

    private static SessionWriter Create(string path, long limitBytes = SessionWriter.DefaultLimitBytes) =>
        SessionWriter.Create(path, new(1, "CpuFailures", null, 1_000_000, 0, SessionCoverage.Trace), limitBytes);

    private static void Sample(SessionWriter writer, long qpc) =>
        writer.PostSample(new(qpc, 1, 5, [new(null, 0xABC0)]), wait: true);

    private static void Events(SessionWriter writer, int first, int count, int padding)
    {
        var payload = new string('x', padding);
        for (var sequence = first; sequence < first + count; sequence++)
        {
            writer.Process(sequence, new(999 + sequence, sequence / 1000.0, 1, 5, XamlProvider.Xaml, true, "Retention", 0,
                [new("Padding", payload)]));
        }
    }

    private static void Wait(SessionWriter writer, Func<bool> ready)
    {
        var deadline = Environment.TickCount64 + 10_000;
        while (writer.Failure is null && !ready() && Environment.TickCount64 < deadline)
        {
            Thread.Sleep(20);
        }

        Expect(writer.Failure is null && ready(), $"The fixture must reach its next write boundary: {writer.Failure}");
    }

    private static void Failed(SessionWriter writer, WriteFault fault) =>
        Expect(fault.Fired && writer.Failure is SqliteException { SqliteErrorCode: 10 },
            $"The managed writer must report the injected SQLite write failure: {writer.Failure}");

    private static void Expect(bool condition, string message)
    {
        if (!condition)
        {
            throw new InvalidOperationException(message);
        }
    }

    // The wrapper is process-local and forwards every operation except one armed write to an owned fixture's WAL.
    // Copies and delegates outlive all fixture connections; restore the original default before freeing them.
    internal sealed class WriteFault : IDisposable
    {
        private const int IoErrorWrite = 10 | (3 << 8);
        private readonly nint originalVfs;
        private readonly nint wrapperVfs;
        private readonly nint name;
        private readonly Open originalOpen;
        private readonly List<Delegate> callbacks = [];
        private readonly List<nint> methods = [];
        private readonly string mainWal;
        private readonly string cpuWal;
        private readonly bool targetCpu;
        private readonly object gate = new();
        private int armed;
        private int fired;
        private long serial;
        private long cpuWrite;
        private long cpuSync;

        internal WriteFault(string path, bool cpu)
        {
            mainWal = Path.GetFullPath(path) + "-wal";
            cpuWal = CpuSchema.PathFor(Path.GetFullPath(path)) + "-wal";
            targetCpu = cpu;
            originalVfs = sqlite3_vfs_find(null);
            Expect(originalVfs != 0, "SQLite must have initialized its default VFS.");
            var wrapper = Marshal.PtrToStructure<Vfs>(originalVfs);
            originalOpen = Marshal.GetDelegateForFunctionPointer<Open>(wrapper.Open);
            Open open = OpenFile;
            callbacks.Add(open);
            wrapper.Open = Marshal.GetFunctionPointerForDelegate(open);
            wrapper.Next = 0;
            name = Marshal.StringToCoTaskMemUTF8($"WinUIProfiler-Fault-{Guid.NewGuid():N}");
            wrapper.Name = name;
            wrapperVfs = Marshal.AllocHGlobal(Marshal.SizeOf<Vfs>());
            Marshal.StructureToPtr(wrapper, wrapperVfs, false);
            Expect(sqlite3_vfs_register(wrapperVfs, 1) == 0, "The fixture VFS must register.");
        }

        internal bool Fired => Volatile.Read(ref fired) != 0;
        internal bool CpuFlushedBeforeFailure { get; private set; }
        internal void Arm() => Volatile.Write(ref armed, 1);
        internal void Disarm() => Volatile.Write(ref armed, 0);

        private int OpenFile(nint vfs, nint filename, nint file, int flags, nint outFlags)
        {
            var result = originalOpen(originalVfs, filename, file, flags, outFlags);
            var path = Marshal.PtrToStringUTF8(filename)?.Replace('/', '\\');
            var isCpu = string.Equals(path, cpuWal, StringComparison.OrdinalIgnoreCase);
            var isMain = string.Equals(path, mainWal, StringComparison.OrdinalIgnoreCase);
            if (result != 0 || (!isCpu && !isMain))
            {
                return result;
            }

            var io = Marshal.PtrToStructure<IoMethods>(Marshal.ReadIntPtr(file));
            var originalWrite = Marshal.GetDelegateForFunctionPointer<Write>(io.Write);
            var originalSync = Marshal.GetDelegateForFunctionPointer<Sync>(io.Sync);
            Write write = (handle, buffer, size, offset) =>
            {
                if (isCpu == targetCpu && Interlocked.CompareExchange(ref armed, 0, 1) == 1)
                {
                    CpuFlushedBeforeFailure = Interlocked.Read(ref cpuSync) > Interlocked.Read(ref cpuWrite) &&
                        Interlocked.Read(ref cpuWrite) > 0;
                    Volatile.Write(ref fired, 1);
                    return IoErrorWrite;
                }

                var code = originalWrite(handle, buffer, size, offset);
                if (code == 0 && isCpu)
                {
                    Interlocked.Exchange(ref cpuWrite, Interlocked.Increment(ref serial));
                }

                return code;
            };
            Sync sync = (handle, syncFlags) =>
            {
                var code = originalSync(handle, syncFlags);
                if (code == 0 && isCpu)
                {
                    Interlocked.Exchange(ref cpuSync, Interlocked.Increment(ref serial));
                }

                return code;
            };
            io.Write = Marshal.GetFunctionPointerForDelegate(write);
            io.Sync = Marshal.GetFunctionPointerForDelegate(sync);
            var copy = Marshal.AllocHGlobal(Marshal.SizeOf<IoMethods>());
            Marshal.StructureToPtr(io, copy, false);
            lock (gate)
            {
                callbacks.Add(write);
                callbacks.Add(sync);
                methods.Add(copy);
            }

            Marshal.WriteIntPtr(file, copy);
            return result;
        }

        public void Dispose()
        {
            Expect(sqlite3_vfs_register(originalVfs, 1) == 0 && sqlite3_vfs_unregister(wrapperVfs) == 0,
                "The original SQLite VFS must be restored.");
            foreach (var pointer in methods)
            {
                Marshal.FreeHGlobal(pointer);
            }

            Marshal.FreeHGlobal(wrapperVfs);
            Marshal.FreeCoTaskMem(name);
            GC.KeepAlive(callbacks);
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct Vfs
        {
            internal int Version, FileSize, MaxPath;
            internal nint Next, Name, AppData, Open, Delete, Access, FullPath, DlOpen, DlError, DlSym, DlClose;
            internal nint Randomness, Sleep, CurrentTime, LastError, CurrentTime64, SetSystemCall, GetSystemCall, NextSystemCall;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct IoMethods
        {
            internal int Version;
            internal nint Close, Read, Write, Truncate, Sync, FileSize, Lock, Unlock, ReservedLock, FileControl, SectorSize;
            internal nint DeviceCharacteristics, ShmMap, ShmLock, ShmBarrier, ShmUnmap, Fetch, Unfetch;
        }

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        private delegate int Open(nint vfs, nint name, nint file, int flags, nint outFlags);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        private delegate int Write(nint file, nint buffer, int size, long offset);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        private delegate int Sync(nint file, int flags);
        [DllImport("e_sqlite3", CallingConvention = CallingConvention.Cdecl)]
        private static extern nint sqlite3_vfs_find([MarshalAs(UnmanagedType.LPUTF8Str)] string? name);
        [DllImport("e_sqlite3", CallingConvention = CallingConvention.Cdecl)]
        private static extern int sqlite3_vfs_register(nint vfs, int makeDefault);
        [DllImport("e_sqlite3", CallingConvention = CallingConvention.Cdecl)]
        private static extern int sqlite3_vfs_unregister(nint vfs);
    }
}
