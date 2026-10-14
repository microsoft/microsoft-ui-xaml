// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

using Microsoft.UI.Xaml;

namespace InkParityLab
{
    public partial class App : Application
    {
        public static string CompositorStatus { get; private set; } = "lifted (default)";

        public App()
        {
            // Must run before any compositor is created. Set INKPARITY_COMPOSITOR=system to opt in.
            // CompositionEngine only exists in the ADO lift; resolve it reflectively so this sample
            // also builds against the GitHub product, where the opt-in is simply unavailable.
            var requested = System.Environment.GetEnvironmentVariable("INKPARITY_COMPOSITOR");
            if (string.Equals(requested, "system", System.StringComparison.OrdinalIgnoreCase))
            {
                try
                {
                    var engineType = System.Type.GetType(
                        "Microsoft.UI.Composition.CompositionEngine, Microsoft.WinUI", throwOnError: false);
                    var enumType = System.Type.GetType(
                        "Microsoft.UI.Composition.CompositionEngineType, Microsoft.WinUI", throwOnError: false);
                    if (engineType == null || enumType == null)
                    {
                        CompositorStatus = "system requested, but CompositionEngine is not available in this build";
                    }
                    else
                    {
                        var result = engineType.GetMethod("TrySetProcessEngine")
                            ?.Invoke(null, new object[] { System.Enum.Parse(enumType, "System") });
                        CompositorStatus = $"system requested, TrySetProcessEngine => {result}";
                    }
                }
                catch (System.Exception ex)
                {
                    CompositorStatus = $"system requested, TrySetProcessEngine THREW 0x{ex.HResult:X8} {ex.GetType().Name}";
                }
            }

            this.InitializeComponent();
        }

        protected override void OnLaunched(LaunchActivatedEventArgs args)
        {
            var window = new MainWindow { Title = "WinUI 3 Ink Parity Lab" };
            window.Activate();
        }
    }
}
