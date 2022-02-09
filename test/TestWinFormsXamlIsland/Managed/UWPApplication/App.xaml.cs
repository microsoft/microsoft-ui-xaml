using Microsoft.Toolkit.Win32.UI.XamlHost;
using System;

namespace UWPApplication
{
    sealed partial class App : XamlApplication
    {
        public static Action<string> ShowShareUIForWindow;
        public static Action<OrientationChangedEventArgs> OrientationChanged;

        IntPtr windowHandle;

        public IntPtr WindowHandle
        {
            get
            {
                return windowHandle;
            }
            set
            {
                windowHandle = value;
            }
        }


        public App()
        {
            this.Initialize();
        }
    }

    public class OrientationChangedEventArgs
    {
        public bool IsLandscape { get; set; }
    }
}
