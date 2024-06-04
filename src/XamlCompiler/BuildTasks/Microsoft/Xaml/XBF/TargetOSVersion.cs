// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Markup.Compiler.XBF
{
    using System;
    using System.Runtime.InteropServices;

    [StructLayout(LayoutKind.Explicit, Size = 8)]
    public class TargetOSVersion
    {
        [CLSCompliant(false)]
        [FieldOffset(0)]
        public ushort major;
        [CLSCompliant(false)]
        [FieldOffset(2)]
        public ushort minor;
        [CLSCompliant(false)]
        [FieldOffset(4)]
        public ushort build;
        [CLSCompliant(false)]
        [FieldOffset(6)]
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