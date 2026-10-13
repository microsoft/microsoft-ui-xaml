// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.Diagnostics.Tracing;
using Microsoft.Diagnostics.Tracing.Session;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Text.Json;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    internal sealed class TableViewTelemetryCapture : IDisposable
    {
        internal static readonly Guid SdkProvider = new Guid("80a0b321-304c-576e-c0f8-c593c3e7cf86");
        internal static readonly Guid DebugProvider = new Guid("03dc0588-a8a2-5562-b209-03918bcc6792");
        internal static readonly Guid TipProvider = new Guid("50109fbd-6d85-5815-731e-c907eca1607b");
        private readonly int processId = Process.GetCurrentProcess().Id;
        private readonly string path = Path.Combine(Path.GetTempPath(), "TableViewTelemetry-" + Guid.NewGuid() + ".etl");
        private readonly TraceEventSession session;
        private List<Record> records;
        private bool disposed;

        [DllImport("kernelbase.dll", ExactSpelling = true)]
        private static extern void TestControlReporting(uint request);

        internal TableViewTelemetryCapture()
        {
            TestControlReporting(2);
            try
            {
                session = new TraceEventSession("TableViewTelemetry-" + Guid.NewGuid(), path,
                    TraceEventSessionOptions.NoRestartOnCreate);
                var options = new TraceEventProviderOptions { ProcessIDFilter = new List<int> { processId } };
                session.EnableProvider(SdkProvider, TraceEventLevel.Verbose, 0x400000020000, options);
                session.EnableProvider(DebugProvider, TraceEventLevel.Verbose, 0x20000, options);
                session.EnableProvider(TipProvider, TraceEventLevel.Verbose, ulong.MaxValue, options);
            }
            catch
            {
                session?.Dispose();
                TestControlReporting(1);
                throw;
            }
        }

        internal IReadOnlyList<Record> Stop()
        {
            if (records != null) { return records; }
            if (disposed) { throw new ObjectDisposedException(nameof(TableViewTelemetryCapture)); }
            if (!session.Stop()) { throw new InvalidOperationException("The owned ETW session did not stop."); }
            var decoded = new List<Record>();
            using (var source = new ETWTraceEventSource(path))
            {
                source.Dynamic.All += data =>
                {
                    if (data.ProcessID != processId ||
                        data.ProviderGuid != SdkProvider && data.ProviderGuid != DebugProvider &&
                        data.ProviderGuid != TipProvider) { return; }
                    var fields = new Dictionary<string, object>(StringComparer.Ordinal);
                    for (int index = 0; index < data.PayloadNames.Length; ++index)
                    {
                        fields.Add(data.PayloadNames[index], data.PayloadValue(index));
                    }
                    var record = new Record(data.EventName, data.ProviderGuid,
                        data.TimeStampQPC, data.ActivityID, fields);
                    if (data.ProviderGuid == TipProvider)
                    {
                        if (!record.IsTip(64477764) && !record.IsTip(64477774)) { return; }
                        using var payload = JsonDocument.Parse(record.Text("data"));
                        int expectedVersion = record.IsTip(64477764) ? 4 : 3;
                        if (payload.RootElement.GetProperty("version").GetInt32() != expectedVersion)
                        {
                            throw new InvalidDataException("Unexpected TableView TiP custom data version.");
                        }
                    }
                    decoded.Add(record);
                };
                if (!source.Process() || source.EventsLost != 0)
                {
                    throw new InvalidDataException("Incomplete or lost native ETW events.");
                }
            }
            if (decoded.Count == 0)
            {
                throw new InvalidDataException("No owned native events were captured.");
            }
            records = decoded;
            return records;
        }

        public void Dispose()
        {
            if (disposed) { return; }
            disposed = true;
            try { session.Dispose(); }
            finally
            {
                TestControlReporting(1);
                if (File.Exists(path)) { File.Delete(path); }
            }
        }

        internal sealed class Record
        {
            internal readonly string Name;
            internal readonly Guid Provider;
            internal readonly long TimestampQpc;
            internal readonly Guid ActivityId;
            private readonly Dictionary<string, object> fields;

            internal Record(string name, Guid provider, long timestampQpc, Guid activityId,
                Dictionary<string, object> fields)
            {
                Name = name;
                Provider = provider;
                TimestampQpc = timestampQpc;
                ActivityId = activityId;
                this.fields = fields;
            }

            internal bool IsTip(uint measure) =>
                Provider == TipProvider && fields.ContainsKey("testCaseId") && Number("testCaseId") == measure;

            internal uint Number(string field)
            {
                switch (fields[field])
                {
                    case uint value: return value;
                    case int value: return unchecked((uint)value);
                    case ushort value: return value;
                    case short value when value >= 0: return (uint)value;
                    case byte value: return value;
                    default: throw new InvalidDataException("Unexpected numeric wire type: " + field);
                }
            }

            internal bool Flag(string field) => fields[field] is bool value ? value :
                throw new InvalidDataException("Expected Boolean wire type: " + field);

            internal string Text(string field) => fields[field] is string value ? value :
                throw new InvalidDataException("Expected String wire type: " + field);
        }
    }
}
