// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
// Debug helpers

namespace Microsoft.UI.Xaml.Markup.Compiler.Lmr.Internal
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    // Very basic implementation of an assert dialog so that we don't need to link against System.dll.
    internal class Debug
    {
        // Dialog Box Command IDs, From WinUser.h
        enum MessageBoxResult
        {
            IDABORT = 3,
            IDRETRY = 4,
            IDIGNORE = 5,
        }

        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Usage", "CA2205:UseManagedEquivalentsOfWin32Api")]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1060:MovePInvokesToNativeMethodsClass")]
        [System.Runtime.InteropServices.DllImport("user32.dll", BestFitMapping = false)]
        static extern int MessageBoxA(int h, string m, string c, int type);

        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Usage", "CA1806:DoNotIgnoreMethodResults", MessageId = "Microsoft.UI.Xaml.Markup.Compiler.Lmr.Internal.Debug.MessageBoxA(System.Int32,System.String,System.String,System.Int32)")]
        static MessageBoxResult MessageBox(string message)
        {
            const int MB_ICONEXCLAMATION = 0x30;
            const int MB_ABORTRETRYIGNORE = 0x00000002;
            int res = MessageBoxA(0, message, "LMR Assert failed", MB_ICONEXCLAMATION | MB_ABORTRETRYIGNORE);
            return (MessageBoxResult)res;            
        }
               

        [Conditional("DEBUG")]
        public static void Assert(bool f)
        {
            Assert(f, "Assert failed");
        }

        [Conditional("DEBUG")]
        public static void Assert(bool f, string message)
        {
            if (!f)
            {
                // If you stop here, the assert failed.
                Debugger.Log(0, "assert", message);

                // Show a message box UI before we break into the debugger.
                string stack = System.Environment.StackTrace;
                var result = MessageBox(message + "\r\n" + stack + 
@"
Abort - terminate the process
Retry - break into the debugger
Ignore - ignore the assert and continue running"
);
                if (result == MessageBoxResult.IDABORT)
                {
                    Environment.Exit(1);
                }
                if (result == MessageBoxResult.IDRETRY)
                {
                    Debugger.Break();
                }
            }
        }

        [Conditional("DEBUG")]
        public static void Fail(string message)
        {
            Assert(false, message);
        }
    }
   

}
