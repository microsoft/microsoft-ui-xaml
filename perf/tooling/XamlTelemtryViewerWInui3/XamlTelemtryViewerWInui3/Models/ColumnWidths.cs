using CommunityToolkit.Mvvm.ComponentModel;
using Microsoft.UI.Xaml;

namespace XamlTelemtryViewerWInui3.Models
{
    public sealed partial class ColumnWidths : ObservableObject
    {
        [ObservableProperty]
        public partial GridLength Timestamp { get; set; } = new(120);

        [ObservableProperty]
        public partial GridLength ProviderGuid { get; set; } = new(200);

        [ObservableProperty]
        public partial GridLength ProcessName { get; set; } = new(100);

        [ObservableProperty]
        public partial GridLength ProcessId { get; set; } = new(70);

        [ObservableProperty]
        public partial GridLength ProviderName { get; set; } = new(150);

        [ObservableProperty]
        public partial GridLength EventName { get; set; } = new(140);

        [ObservableProperty]
        public partial GridLength Level { get; set; } = new(60);

        [ObservableProperty]
        public partial GridLength Opcode { get; set; } = new(70);

        [ObservableProperty]
        public partial GridLength ThreadId { get; set; } = new(70);

        [ObservableProperty]
        public partial GridLength Payload { get; set; } = new(1, GridUnitType.Star);
    }
}
