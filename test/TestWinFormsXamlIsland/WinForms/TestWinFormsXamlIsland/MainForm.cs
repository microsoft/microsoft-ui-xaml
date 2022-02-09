using Helpers;
using System;
using System.Windows;
using System.Windows.Forms;
using Windows.ApplicationModel.DataTransfer;

namespace TestWinFormsXamlIsland
{
    public partial class MainForm : Form
    {
        public MainForm()
        {
            InitializeComponent();

            this.windowsXamlHost1.Child = new ManagedUWP.MainPage();
            this.Load += MainForm_Load;
        }

        private void MainForm_Load(object sender, EventArgs e)
        {
            //Using WinRT DataTransferManager in Win32
            IntPtr hwnd = this.Handle;
            var dtm = DataTransferManagerHelper.GetForWindow(hwnd);
            dtm.DataRequested += OnDataRequested;
            UWPApplication.App.ShowShareUIForWindow += ShowShareUI;

            //Detect Orientation
            Microsoft.Win32.SystemEvents.DisplaySettingsChanged += SystemEvents_DisplaySettingsChanged;

            //Pass the WinForm's Hwmd to the UWP Application
            (UWPApplication.App.Current as UWPApplication.App).WindowHandle = hwnd;
        }

        #region Using WinRT DataTransferManager in Win32

        string _text;
        
        private void ShowShareUI(string text)
        {
            IntPtr hwnd = this.Handle;
            DataTransferManagerHelper.ShowShareUIForWindow(hwnd);
            _text = text;
        }

        private async void OnDataRequested(DataTransferManager sender, DataRequestedEventArgs args)
        {
            var deferral = args.Request.GetDeferral();

            try
            {
                DataPackage dp = args.Request.Data;
                dp.Properties.Title = _text;
                dp.SetText(_text);
            }
            finally
            {
                deferral.Complete();
            }
        }
        #endregion

        #region Detect Orientation

        private void SystemEvents_DisplaySettingsChanged(object sender, EventArgs e)
        {
            if (SystemParameters.PrimaryScreenWidth > SystemParameters.PrimaryScreenHeight)
            {
                UWPApplication.App.OrientationChanged(new UWPApplication.OrientationChangedEventArgs() { IsLandscape = true });
            }
            else
            {
                UWPApplication.App.OrientationChanged(new UWPApplication.OrientationChangedEventArgs() { IsLandscape = false });
            }
        }
        #endregion
    }
}
