// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;

namespace Private.Infrastructure
{
    public static class KeyboardHelperExtensions
    {
        public static IDisposable CreateKeyboardWaitKindGuard(
            this KeyboardHelper @this,
            KeyboardWaitKind waitKind)
        {
            return new KeyboardWaitKindGuard(@this, waitKind);
        }

        private class KeyboardWaitKindGuard : IDisposable
        {
            public KeyboardWaitKindGuard(KeyboardHelper helper, KeyboardWaitKind waitKind)
            {
                this.helper = helper;
                this.helper.SetWaitKind(waitKind);
            }

            public void Dispose()
            {
                this.helper.SetWaitKind(KeyboardWaitKind.Default);
            }

            private readonly KeyboardHelper helper;
        }

    }
}
