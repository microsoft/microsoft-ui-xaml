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

            // Delegate to generated main method
            XamlGeneratedProgram.XamlGeneratedMain();
        }
    }
}
