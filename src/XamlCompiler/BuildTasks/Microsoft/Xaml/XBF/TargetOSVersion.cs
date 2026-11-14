// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Markup.Compiler.XBF
{
    using System;

    public class TargetOSVersion
    {
        [CLSCompliant(false)]
        public ushort major;
        [CLSCompliant(false)]
        public ushort minor;
        [CLSCompliant(false)]
        public ushort build;
        [CLSCompliant(false)]
        public ushort revision;
    }

    internal static class TargetOSVersionExtension
    {
        public static TargetOSVersion ToTargetOSVersion(this Version version)
        {
            return new TargetOSVersion()
            {
                major = (ushort)version.Major,
                minor = (ushort)version.Minor,
                build = (ushort)version.Build,
                revision = (ushort)version.Revision
            };
        }
    }
}