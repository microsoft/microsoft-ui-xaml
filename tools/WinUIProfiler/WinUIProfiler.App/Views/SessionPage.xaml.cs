using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;

namespace WinUIProfiler.App.Views;

public sealed partial class SessionPage : Page
{
    private SnapshotViewerWindow? snapshotViewer;

    public SessionPage()
    {
        InitializeComponent();
    }

    public void SetOwnerWindow(Window owner) =>
        owner.Closed += (_, _) => snapshotViewer?.Close();

    private void SnapshotViewer_Click(object sender, RoutedEventArgs args)
    {
        if (snapshotViewer is null)
        {
            snapshotViewer = new SnapshotViewerWindow();
            snapshotViewer.Closed += (_, _) => snapshotViewer = null;
        }
        snapshotViewer.Activate();
    }
}
