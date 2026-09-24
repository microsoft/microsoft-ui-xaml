namespace DisableXamlGeneratedMainNoCtorCs
{
    static class Program
    {
        // This sample defines DISABLE_XAML_GENERATED_MAIN AND intentionally omits a
        // parameterless App constructor. The developer supplies their own entry point below
        // and constructs the App explicitly using its parameterized constructor.
        //
        // The XamlCompiler still generates a XamlGeneratedProgram.XamlGeneratedMain() helper,
        // but because the App has no parameterless constructor it must NOT emit the
        // "new App()" call there (otherwise this project would fail to compile). This project
        // exists to verify that behavior.
        [global::System.STAThreadAttribute]
        static void Main(string[] args)
        {
            global::WinRT.ComWrappersSupport.InitializeComWrappers();
            global::Microsoft.UI.Xaml.Application.Start((p) =>
            {
                var context = new global::Microsoft.UI.Dispatching.DispatcherQueueSynchronizationContext(
                    global::Microsoft.UI.Dispatching.DispatcherQueue.GetForCurrentThread());
                global::System.Threading.SynchronizationContext.SetSynchronizationContext(context);
                _ = new App(42);
            });
        }
    }
}
