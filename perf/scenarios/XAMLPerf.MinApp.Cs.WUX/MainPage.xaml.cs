using System;
using System.Timers;
using Windows.UI.Xaml.Controls;

// The Blank Page item template is documented at https://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409

namespace XAMLPerf.MinApp.Cs.WUX
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class MainPage : Page
    {
        Timer t;

        public MainPage()
        {
            InitializeComponent();

            t = new Timer(1000.0);
            t.Elapsed += T_Elapsed;
            t.Start();
        }

        private void T_Elapsed(object sender, ElapsedEventArgs e)
        {
            t.Stop();
            System.GC.Collect(2, GCCollectionMode.Forced, true);
        }
    }
}
