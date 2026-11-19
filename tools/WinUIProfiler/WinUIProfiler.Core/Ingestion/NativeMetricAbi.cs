using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using WinUIProfiler.Contracts;

namespace WinUIProfiler.Engine.Ingestion;
// Mirrors WinUIProfiler.Native\Exports.h. Fixed-width fields and pack 8 apply on x64 and ARM64.
internal static class NativeMetricAbi
{
    internal const uint CpuVersion = 2;
    internal const uint CaptureVersion = 1;
    internal const uint MemoryVersion = 1;
    internal const uint GpuVersion = 1;
    internal const int MaxCpuGroups = 64;
    internal const int MaxLogicalProcessorsPerGroup = 64;
    internal const int MaxGpuRows = 256;
    internal const uint MaxGpuInstances = 4096;
    internal const uint Perf100nsTimer = 0x20510500;
    internal enum MetricStatus : uint
    {
        Ok,
        Warmup,
        Unavailable,
        Partial,
        CounterReset,
        TargetMissing,
        IdentityChanged
    }

    internal enum MetricSource : uint
    {
        NtGroupProcessTimesCompatibility = 2,
        ProcessMemoryEx2 = 4,
        ProcessMemoryEx = 5,
        EnglishPdhGpuEngine = 6,
        NtProcessSnapshotCpu = 9,
        NtProcessSnapshotMemory = 10
    }

    internal enum Clock : ulong
    {
        Boot100ns = 2,
        FileTime100ns = 3
    }

    internal enum ErrorDomain : uint
    {
        None,
        Win32,
        NtStatus
    }

    internal enum ByteUnit : uint
    {
        Bytes = 1
    }

    [StructLayout(LayoutKind.Sequential, Pack = 8)]
    internal struct ProcessIdentity
    {
        public uint ProcessId;
        public uint Reserved;
        public ulong CreationTime100ns;
    }

    [StructLayout(LayoutKind.Sequential, Pack = 8)]
    internal struct CaptureRequest
    {
        public uint Size;
        public uint Version;
        public ProcessIdentity Process;
    }

    [StructLayout(LayoutKind.Sequential, Pack = 8)]
    internal struct CpuGroup
    {
        public uint Size;
        public uint Version;
        public uint GroupId;
        public uint LogicalProcessorCount;
        public ulong Kernel;
        public ulong User;
        public ulong Idle;
        public ulong Dpc;
        public ulong Isr;
    }

    [InlineArray(MaxCpuGroups)]
    internal struct CpuGroups
    {
        private CpuGroup element;
    }

    [StructLayout(LayoutKind.Sequential, Pack = 8)]
    internal struct CpuSnapshot
    {
        public uint Size;
        public uint Version;
        public MetricSource Source;
        public MetricStatus Status;
        public Clock ClockId;
        public ulong Timestamp100ns;
        public ProcessIdentity Process;
        public uint GroupCount;
        public uint ExpectedGroupCount;
        public uint TargetPresent;
        public uint Reserved;
        public ulong ProcessKernel;
        public ulong ProcessUser;
    }

    [StructLayout(LayoutKind.Sequential, Pack = 8)]
    internal struct PercentMetric
    {
        public MetricStatus Status;
        public uint Reserved;
        public double Percent;
    }

    [StructLayout(LayoutKind.Sequential, Pack = 8)]
    internal struct CpuResult
    {
        public uint Size;
        public uint Version;
        public MetricSource Source;
        public MetricStatus Status;
        public ProcessIdentity Process;
        public Clock ClockId;
        public ulong Begin100ns;
        public ulong End100ns;
        public PercentMetric SystemUtilization;
        public PercentMetric ProcessUtilization;
        public PercentMetric InterruptUtilization;
        public PercentMetric OtherOrUnattributedUtilization;
        public PercentMetric ProcessUtility;
    }

    [StructLayout(LayoutKind.Sequential, Pack = 8)]
    internal struct CpuCaptureResult
    {
        public uint Size;
        public uint Version;
        public uint ErrorCode;
        public ErrorDomain ErrorDomain;
        public ulong Begin100ns;
        public ulong End100ns;
        public CpuSnapshot Snapshot;
        public CpuGroups Groups;
    }

    [StructLayout(LayoutKind.Sequential, Pack = 8)]
    internal struct ByteMetric
    {
        public MetricStatus Status;
        public ByteUnit Units;
        public ulong Value;
    }

    [StructLayout(LayoutKind.Sequential, Pack = 8)]
    internal struct MemoryResult
    {
        public uint Size;
        public uint Version;
        public MetricSource Source;
        public MetricStatus Status;
        public ProcessIdentity Process;
        public Clock ClockId;
        public ulong Begin100ns;
        public ulong End100ns;
        public uint Win32Error;
        public uint Reserved;
        public ByteMetric PrivateWorkingSet;
        public ByteMetric TotalWorkingSet;
        public ByteMetric PrivateCommit;
    }

    [StructLayout(LayoutKind.Sequential, Pack = 8)]
    internal struct GpuRawEngine
    {
        public uint LuidHigh;
        public uint LuidLow;
        public uint Physical;
        public uint Engine;
        public uint DuplicateIndex;
        public MetricStatus Status;
        public uint PdhStatus;
        public uint CounterType;
        public ulong RunningTime;
        public ulong Timer;
        public ulong Timestamp100ns;
    }

    [InlineArray(MaxGpuRows)]
    internal struct GpuRawEngines
    {
        private GpuRawEngine element;
    }

    [StructLayout(LayoutKind.Sequential, Pack = 8)]
    internal struct GpuSnapshot
    {
        public uint Size;
        public uint Version;
        public MetricSource Source;
        public MetricStatus Status;
        public ProcessIdentity Process;
        public Clock ClockId;
        public ulong Sample100ns;
        public ulong Begin100ns;
        public ulong End100ns;
        public Clock AcquisitionClockId;
        public uint PdhError;
        public uint RowCount;
        public uint Win32Error;
        public uint ObservedInstanceCount;
        public GpuRawEngines Rows;
    }

    [StructLayout(LayoutKind.Sequential, Pack = 8)]
    internal struct GpuEngineUsage
    {
        public uint LuidHigh;
        public uint LuidLow;
        public uint Physical;
        public uint Engine;
        public MetricStatus Status;
        public uint Reserved;
        public double Percent;
    }

    [InlineArray(MaxGpuRows)]
    internal struct GpuEngineUsages
    {
        private GpuEngineUsage element;
    }

    [StructLayout(LayoutKind.Sequential, Pack = 8)]
    internal struct GpuResult
    {
        public uint Size;
        public uint Version;
        public MetricSource Source;
        public MetricStatus Status;
        public ProcessIdentity Process;
        public Clock ClockId;
        public ulong Begin100ns;
        public ulong End100ns;
        public PercentMetric Utilization;
        public uint EngineCount;
        public uint Reserved;
        public GpuEngineUsages Engines;
        public ByteMetric DedicatedCommitted;
        public ByteMetric SharedCommitted;
        public ByteMetric DedicatedResident;
        public ByteMetric SharedResident;
    }

    internal static uint SizeOf<T>()
        where T : unmanaged => checked((uint)Unsafe.SizeOf<T>());
    internal static CaptureRequest Request(ProcessInstanceKey target, uint version)
    {
        target.Validate();
        if (target.CreationTimeFileTime is not > 0)
        {
            throw new ArgumentException("Native sampling requires the target's actual creation time.", nameof(target));
        }

        return new()
        {
            Size = SizeOf<CaptureRequest>(),
            Version = version,
            Process = new()
            {
                ProcessId = target.ProcessId,
                CreationTime100ns = checked((ulong)target.CreationTimeFileTime.Value)
            }
        };
    }

    internal static void RequireProcess(in ProcessIdentity process, ProcessInstanceKey target)
    {
        target.Validate();
        if (target.CreationTimeFileTime is not > 0 ||
process.Reserved != 0 ||
process.ProcessId != target.ProcessId ||
process.CreationTime100ns != checked((ulong)target.CreationTimeFileTime.Value))
        {
            throw new InvalidDataException("Native metric data does not match the selected process instance.");
        }
    }
}
