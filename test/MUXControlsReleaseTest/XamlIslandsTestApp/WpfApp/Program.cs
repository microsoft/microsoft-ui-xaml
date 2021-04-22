using System;
using System.Collections.Generic;
using System.Text;

namespace WpfApp
{
    public class Program
    {
        [System.STAThreadAttribute()]
        public static void Main()
        {
            UwpApp.App uwpApp = new UwpApp.App(); // Needed for the native application object it creates.
            App app = new App();
            app.Run();
            GC.SuppressFinalize(uwpApp);
        }
    }
}
