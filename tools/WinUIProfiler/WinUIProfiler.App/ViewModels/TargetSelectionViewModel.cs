using System.ComponentModel;
using System.Diagnostics;
using WinUIProfiler.Engine.Target;

namespace WinUIProfiler.App.ViewModels;

public sealed record TargetCandidate(string Name, string Value, TargetLaunchKind Kind,
    long? CreationTimeFileTime = null, string? Error = null)
{
    public bool CanSelect => Error is null && (Kind != TargetLaunchKind.Attach || CreationTimeFileTime > 0);
    public string Description => Error is null ? Value : $"{Value} - {Error}";
    public string DisplayName => $"{Name} ({Description})";
}

public sealed class TargetSelectionViewModel(
    Func<CancellationToken, Task<IReadOnlyList<TargetCandidate>>> discover) : INotifyPropertyChanged
{
    private IReadOnlyList<TargetCandidate> all = [];
    private string searchText = "";
    private TargetCandidate? selected;
    public event PropertyChangedEventHandler? PropertyChanged;
    public IReadOnlyList<TargetCandidate> Candidates { get; private set; } = [];
    public string SearchText
    {
        get => searchText;
        set
        {
            if (searchText == value) return;
            searchText = value;
            Filter();
        }
    }
    public TargetCandidate? Selected
    {
        get => selected;
        set
        {
            if (selected == value) return;
            selected = value;
            PropertyChanged?.Invoke(this, new(nameof(Selected)));
            PropertyChanged?.Invoke(this, new(nameof(CanSelect)));
        }
    }
    public bool IsBusy { get; private set; }
    public bool CanRefresh => !IsBusy;
    public bool CanSelect => !IsBusy && Selected?.CanSelect == true && Candidates.Contains(Selected);
    public bool IsEmpty => !IsBusy && !HasError && Candidates.Count == 0;
    public string ErrorMessage { get; private set; } = "";
    public bool HasError => ErrorMessage.Length > 0;

    public async Task RefreshAsync(CancellationToken token = default)
    {
        if (IsBusy) throw new InvalidOperationException("Target discovery is already running.");
        IsBusy = true;
        ErrorMessage = "";
        Selected = null;
        all = [];
        Filter();
        try
        {
            var result = await Task.Run(() => discover(token), token);
            token.ThrowIfCancellationRequested();
            all = result.OrderBy(candidate => candidate.Name, StringComparer.CurrentCultureIgnoreCase)
                .ThenBy(candidate => candidate.Value, StringComparer.OrdinalIgnoreCase).ToArray();
            Filter();
        }
        catch (OperationCanceledException) when (token.IsCancellationRequested) { }
        catch (Exception error)
        {
            ErrorMessage = error.Message;
            Trace.TraceError(error.ToString());
        }
        finally { IsBusy = false; Notify(); }
    }

    private void Filter()
    {
        var text = SearchText.Trim();
        Candidates = all.Where(candidate => candidate.Name.Contains(text, StringComparison.CurrentCultureIgnoreCase) ||
            candidate.Value.Contains(text, StringComparison.OrdinalIgnoreCase)).ToArray();
        if (selected is not null && !Candidates.Contains(selected)) selected = null;
        Notify();
    }

    private void Notify() => PropertyChanged?.Invoke(this, new(null));
}
