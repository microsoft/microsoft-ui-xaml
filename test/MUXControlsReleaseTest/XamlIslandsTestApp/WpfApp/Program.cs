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
            App app = new App();
            app.InitializeComponent();
            app.Run();
        }
    }
}
