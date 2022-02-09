using System;
using System.Runtime.InteropServices;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.ApplicationModel.DataTransfer;

namespace Helpers
{
    public static class DataTransferManagerHelper
    {
        static readonly Guid _dtm_iid = new Guid(0xa5caee9b, 0x8708, 0x49d1, 0x8d, 0x36, 0x67, 0xd2, 0x5a, 0x8d, 0xa0, 0x0c);

        static IDataTransferManagerInterop DataTransferManagerInterop
        {
            get
            {
                return (IDataTransferManagerInterop)WindowsRuntimeMarshal.GetActivationFactory(typeof(DataTransferManager));
            }
        }

        public static DataTransferManager GetForWindow(IntPtr hwnd)
        {
            return DataTransferManagerInterop.GetForWindow(hwnd, _dtm_iid);
        }

        public static void ShowShareUIForWindow(IntPtr hwnd)
        {
            DataTransferManagerInterop.ShowShareUIForWindow(hwnd);
        }

        [ComImport, Guid("3A3DCD6C-3EAB-43DC-BCDE-45671CE800C8")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        interface IDataTransferManagerInterop
        {
            DataTransferManager GetForWindow([In] IntPtr appWindow, [In] ref Guid riid);
            void ShowShareUIForWindow(IntPtr appWindow);
        }
    }
}
