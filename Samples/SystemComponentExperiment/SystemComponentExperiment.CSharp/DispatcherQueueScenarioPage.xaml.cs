using Microsoft.UI.Dispatching;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;

namespace SystemComponentExperiment.CSharp;

public sealed partial class DispatcherQueueScenarioPage : Page
{
    public DispatcherQueueScenarioPage()
    {
        InitializeComponent();
    }

    private void RunScenario_Click(object sender, RoutedEventArgs args)
    {
        DispatcherQueue? queue = DispatcherQueue.GetForCurrentThread();

        if (queue is null)
        {
            ResultText.Text = "Failed: no DispatcherQueue for the UI thread.";
            return;
        }

        bool enqueued = queue.TryEnqueue(
            DispatcherQueuePriority.Normal,
            () => ResultText.Text = "Passed");

        if (!enqueued)
        {
            ResultText.Text = "Failed: TryEnqueue returned false.";
        }
    }
}
