namespace RenderBench;

// Turns successive snapshots of per-thread CPU into cumulative per-category totals. Threads are keyed by
// (id, creation time) so recycled thread ids are not merged, and threads that exit keep their last-seen CPU, so the
// totals never go backwards. The harness feeds it from Toolhelp32 + GetThreadTimes (ThreadCpuSampler).
public sealed class ThreadCpuAccumulator
{
    public readonly record struct Observation(int ThreadId, long CreationTime, string? StartModule, double CpuMs);

    public readonly record struct ModuleSummary(string Module, ThreadCategory Category, int Threads, double CpuMs);

    sealed class ThreadState(string module, ThreadCategory category)
    {
        public string Module { get; } = module;
        public ThreadCategory Category { get; } = category;
        public double CpuMs { get; set; }
    }

    readonly Dictionary<(int, long), ThreadState> _threads = [];
    readonly object _lock = new();

    public int UiThreadId { get; set; }

    // Records one snapshot and returns cumulative CPU ms per category (indexed by ThreadCategory) plus the number of
    // threads seen in this snapshot.
    public (double[] CategoryMs, int LiveThreads) Add(IEnumerable<Observation> snapshot)
    {
        lock (_lock)
        {
            int live = 0;
            foreach (var o in snapshot)
            {
                live++;
                var key = (o.ThreadId, o.CreationTime);
                if (!_threads.TryGetValue(key, out var state))
                {
                    bool isUi = o.ThreadId == UiThreadId && UiThreadId != 0;
                    string module = ThreadCategories.ModuleName(o.StartModule);
                    state = new ThreadState(module.Length == 0 ? "(unknown)" : module, ThreadCategories.Classify(o.StartModule, isUi));
                    _threads.Add(key, state);
                }
                state.CpuMs = Math.Max(state.CpuMs, o.CpuMs);
            }
            return (Totals(), live);
        }
    }

    public double[] Totals()
    {
        lock (_lock)
        {
            var totals = new double[ThreadCategories.All.Count];
            foreach (var state in _threads.Values)
            {
                totals[(int)state.Category] += state.CpuMs;
            }
            return totals;
        }
    }

    // One row per (start module, category), busiest first.
    public IReadOnlyList<ModuleSummary> Modules()
    {
        lock (_lock)
        {
            return _threads.Values
                .GroupBy(s => (Module: s.Module.ToLowerInvariant(), s.Category))
                .Select(g => new ModuleSummary(g.First().Module, g.Key.Category, g.Count(), g.Sum(s => s.CpuMs)))
                .OrderByDescending(m => m.CpuMs)
                .ThenBy(m => m.Module, StringComparer.OrdinalIgnoreCase)
                .ToList();
        }
    }
}
