// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.RepeaterTests.Common
{
    // HRESULTs the Repeater APIs are expected to fail with. Import with
    // "using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.RepeaterTests.Common.HResults;".
    public static class HResults
    {
        public const int E_FAIL = unchecked((int)0x80004005);
        public const int E_NOTIMPL = unchecked((int)0x80004001);
        public const int E_NOINTERFACE = unchecked((int)0x80004002);
        public const int E_INVALIDARG = unchecked((int)0x80070057);
        public const int E_BOUNDS = unchecked((int)0x8000000B);
        public const int E_INVALID_OPERATION = unchecked((int)0x800710DD); // HRESULT_FROM_WIN32(ERROR_INVALID_OPERATION)
        public const int COR_E_INVALIDOPERATION = unchecked((int)0x80131509); // System.InvalidOperationException
    }
}
