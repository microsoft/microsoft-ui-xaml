// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

using System;

namespace System.Xaml
{
    internal static class CriticalExceptions
    {
        // these are all the exceptions considered critical by PreSharp
        internal static bool IsCriticalException(Exception ex)
        {
            ex = Unwrap(ex);

            return ex is NullReferenceException ||
                   ex is StackOverflowException ||
                   ex is OutOfMemoryException   ||
                   ex is System.Threading.ThreadAbortException ||
                   ex is System.Runtime.InteropServices.SEHException ||
                   ex is System.Security.SecurityException;
        }

        // these are exceptions that we should treat as critical when they
        // arise during callbacks into application code

        internal static Exception Unwrap(Exception ex)
        {
            // for certain types of exceptions, we care more about the inner
            // exception
            while (ex.InnerException != null &&
                    (   ex is System.Reflection.TargetInvocationException
                    ))
            {
                ex = ex.InnerException;
            }

            return ex;
        }
    }
}
