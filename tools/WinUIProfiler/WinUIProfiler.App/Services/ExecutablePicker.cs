using Microsoft.UI;
using Microsoft.Windows.Storage.Pickers;

namespace WinUIProfiler.App.Services;

internal static class ExecutablePicker
{
    public static async Task<string?> PickAsync(WindowId owner)
    {
        if (owner.Value == 0)
        {
            throw new ArgumentException("The executable picker requires its owning window.", nameof(owner));
        }

        var picker = new FileOpenPicker(owner);
        picker.FileTypeFilter.Add(".exe");
        var selected = await picker.PickSingleFileAsync();
        if (selected is null)
        {
            return null;
        }

        var path = selected.Path;
        if (!Path.GetExtension(path).Equals(".exe", StringComparison.OrdinalIgnoreCase))
        {
            throw new ArgumentException("Select a Windows executable (.exe).");
        }

        if (!File.Exists(path))
        {
            throw new FileNotFoundException("The selected executable no longer exists.", path);
        }

        return path;
    }
}
