namespace WinUIProfiler.Engine.Processing;

internal sealed class BoundedQueue<T>(int maximumItems, long maximumBytes)
{
    private readonly Queue<(T Value, long Bytes)> items = new();
    private readonly object gate = new();
    private long bytes;
    private bool complete;
    internal (int Items, long Bytes) Usage { get { lock (gate) return (items.Count, bytes); } }
    internal bool TryAdd(T item, long size)
    {
        lock (gate)
        {
            if (complete || size > maximumBytes || items.Count >= maximumItems || size > maximumBytes - bytes) return false;
            items.Enqueue((item, size)); bytes += size; Monitor.PulseAll(gate); return true;
        }
    }
    internal bool Add(T item, long size, TimeSpan timeout)
    {
        var started = System.Diagnostics.Stopwatch.GetTimestamp();
        lock (gate)
        {
            while (!complete && size <= maximumBytes && (items.Count >= maximumItems || size > maximumBytes - bytes))
            {
                var remaining = timeout - System.Diagnostics.Stopwatch.GetElapsedTime(started);
                if (remaining <= TimeSpan.Zero) return false;
                Monitor.Wait(gate, remaining);
            }
            return TryAdd(item, size);
        }
    }
    internal bool Take(out T item, out long size, bool wait = true, long byteAllowance = long.MaxValue)
    {
        lock (gate)
        {
            while (wait && items.Count == 0 && !complete) Monitor.Wait(gate);
            if (items.Count == 0 || items.Peek().Bytes > byteAllowance) { item = default!; size = 0; return false; }
            (item, size) = items.Dequeue(); bytes -= size; Monitor.PulseAll(gate); return true;
        }
    }
    internal void Complete() { lock (gate) { complete = true; Monitor.PulseAll(gate); } }
}
