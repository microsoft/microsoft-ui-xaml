namespace XamlProfiler.Services;

/// <summary>
/// Restores timestamp order to a real-time ETW stream. ETW buffers events per CPU and hands a
/// real-time consumer each buffer when it fills, or when the session's flush timer fires (about
/// once a second), so when a thread moves between CPUs its later events can arrive before its
/// earlier ones. The router needs them in order: an element's <c>ElementSetName</c> must come
/// before its <c>ElementEnteredTree</c>.
/// <para>
/// Each event is held until <c>holdBack</c> has passed since it arrived, then released in
/// timestamp order. An event that arrives later than that is released out of order once its own
/// hold-back ends. Holding events by arrival time means the event timestamps never have to be
/// compared with the consumer's clock.
/// </para>
/// Not thread-safe.
/// </summary>
public sealed class EventReorderBuffer<T>
{
    private readonly PriorityQueue<(T Item, long Arrival), (long Timestamp, long Sequence)> _queue = new();
    private readonly long _holdBack;
    private long _sequence;

    /// <param name="holdBack">How long to hold each event, in the units of the arrival times
    /// passed to <see cref="Add"/> and <see cref="Release"/>.</param>
    public EventReorderBuffer(long holdBack) => _holdBack = holdBack;

    public int Count => _queue.Count;

    /// <summary>
    /// Hold an event. <paramref name="timestamp"/> orders events (ties keep arrival order);
    /// <paramref name="arrival"/> is when the consumer received it.
    /// </summary>
    public void Add(T item, long timestamp, long arrival)
        => _queue.Enqueue((item, arrival), (timestamp, _sequence++));

    /// <summary>
    /// Append to <paramref name="released"/>, in timestamp order, up to <paramref name="max"/>
    /// events whose hold-back has ended at <paramref name="now"/>. Returns how many.
    /// </summary>
    public int Release(long now, int max, List<T> released)
    {
        int count = 0;
        while (count < max && _queue.TryPeek(out var head, out _) && now - head.Arrival >= _holdBack)
        {
            _queue.Dequeue();
            released.Add(head.Item);
            count++;
        }
        return count;
    }

    /// <summary>Append every held event to <paramref name="released"/>, in timestamp order.</summary>
    public void ReleaseAll(List<T> released)
    {
        while (_queue.TryDequeue(out var entry, out _))
            released.Add(entry.Item);
    }
}
