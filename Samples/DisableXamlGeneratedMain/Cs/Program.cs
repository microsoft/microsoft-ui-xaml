namespace DisableXamlGeneratedMainCs
{
    static class Program
    {
        // Recorded by our custom entry point below and displayed by MainWindow so that
        // automated tests can verify that this Main - and not a XamlCompiler-generated
        // one - is what started the app.
        public static string LaunchMarker { get; private set; } = string.Empty;

        [global::System.STAThreadAttribute]
        static void Main(string[] args)
        {
            LaunchMarker = "CustomMain";

            global::WinRT.ComWrappersSupport.InitializeComWrappers();
            global::Microsoft.UI.Xaml.Application.Start((p) =>
            {
                var context = new global::Microsoft.UI.Dispatching.DispatcherQueueSynchronizationContext(
                    global::Microsoft.UI.Dispatching.DispatcherQueue.GetForCurrentThread());
                global::System.Threading.SynchronizationContext.SetSynchronizationContext(context);
                _ = new App();
            });
        }
    }
}
