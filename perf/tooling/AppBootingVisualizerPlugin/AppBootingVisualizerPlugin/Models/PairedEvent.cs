using System;

namespace AppBootingVisualizerPlugin.Models
{
    // A Start/Stop pair produced by EventPairer. Mirrors the WinUI reference app's
    // per-event "occurrence" model (one Opcode==1 paired with one Opcode==2 on the
    // same ThreadId and the same base EventName). Each pair has a real duration
    // and becomes one row in Track 2's Gantt — instead of two separate markers.
    public sealed class PairedEvent
    {
        // Base event name (everything before "/win:" stripped off). E.g.
        // "AddProcessToHeliumContainer/win:Start" -> "AddProcessToHeliumContainer".
        public string EventName { get; set; } = string.Empty;

        public string ProviderName { get; set; } = string.Empty;

        public Guid ProviderGuid { get; set; }

        public string ProcessName { get; set; } = string.Empty;

        public int ProcessId { get; set; }

        public int ThreadId { get; set; }

        // Level is copied from the Start event (same as the WinUI app).
        public string Level { get; set; } = string.Empty;

        public DateTime Start { get; set; }

        public DateTime Stop { get; set; }

        public TimeSpan Duration => this.Stop - this.Start;

        // Payloads are kept separate so the user can see what triggered the start
        // and what the stop reported.
        public string StartPayload { get; set; } = string.Empty;

        public string StopPayload { get; set; } = string.Empty;
    }
}
