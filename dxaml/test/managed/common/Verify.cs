// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Threading.Tasks;
using System.Runtime.InteropServices;

namespace Microsoft.UI.Xaml.Tests.Common
{
    public static class VerifyExtensions
    {
        public static T Throws<T>(Action operation)
            where T: Exception
        {
            var exception = default(T);
            try
            {
                operation();
                WEX.TestExecution.Verify.Fail("Operation did not throw");
            }
            catch(T e)
            {
                exception = e;
            }
            return exception;
        }
    }
}
