// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    using System;
    //Currently versions only have a minor and major version.  So a version of "5.4.0.0" in Platform.xml has 5 as its major version and 4 as its minor,
    //and the other two numbers must be 0.  The version is represented in ContractVersionAttribute as (major << 16) | minor.
    //The ContractVersion stores the version as it would appear in ContractVersionAttribute.
    internal class ContractVersion
    {
        public static Version ToVersion(uint contractVersion)
        {
            uint major = (contractVersion >> 16) & 0xffff;
            uint minor = contractVersion & 0xffff;

            //We explicitly want to state the build and revision are 0 since that's the format in Platform.xml
            return new Version((int)major, (int)minor, 0, 0);
        }
    }
}
