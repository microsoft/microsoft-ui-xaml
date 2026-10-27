using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;

namespace WinUIProfiler.App.Views;

public sealed partial class LiveMetricChart : UserControl
{
    public static readonly DependencyProperty TitleProperty = DependencyProperty.Register(
        nameof(Title), typeof(string), typeof(LiveMetricChart), new PropertyMetadata(""));

    public string Title { get => (string)GetValue(TitleProperty); set => SetValue(TitleProperty, value); }

    public LiveMetricChart()
    {
        InitializeComponent();
    }
}
