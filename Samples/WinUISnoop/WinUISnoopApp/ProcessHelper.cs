using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace WinUISnoopApp
{
    internal static class ProcessHelper
    {
        internal static bool Is64Bit(this Process process)
        {
            return GetProcessBitness(process) == ProcessBitness.B64;
        }

        internal enum ProcessBitness { B32, B64 };
        internal static ProcessBitness GetProcessBitness(this Process process)
        {
            bool isWow64;
            if (!IsWow64Process(process.Handle, out isWow64))
            {
                isWow64 = false;
            }

            if (isWow64)
            {
                // process is wow64, so it must be 32-bit
                return ProcessBitness.B32;
            }

            // Not wow64. Determine the native bitness.
            if (IntPtr.Size == 8)
            {
                // Snoop is 64-bit, and since the target isn't wow64, it must also be 64-bit.
                return ProcessBitness.B64;
            }
            else
            {
                System.Diagnostics.Debug.Assert(IntPtr.Size == 4);
                // Snoop is 32-bit, but is it native 32-bit or itself wow64?
                bool isSnoopWow64;
                if (!IsWow64Process(Process.GetCurrentProcess().Handle, out isSnoopWow64))
                {
                    isSnoopWow64 = false;
                }
                if (isSnoopWow64)
                {
                    return ProcessBitness.B64; // Snoop is wow64 but target is not, so target is native 64-bit
                }
                else
                {
                    return ProcessBitness.B32; // Snoop isn't wow64, so it is native 32-bit as is the target.
                }
            }
        }

        [DllImport("kernel32.dll", SetLastError = true, CallingConvention = CallingConvention.Winapi)]
        [return: MarshalAs(UnmanagedType.Bool)]
        internal static extern bool IsWow64Process([In] IntPtr process, [Out] out bool wow64Process);
    }

}
