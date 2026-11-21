// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Markup.Compiler.XBF
{
    using System;
    using System.Runtime.InteropServices.Marshalling;
    using DirectUI;

    // A member of a XbfXamlType, see XbfMetadataProvider
    [GeneratedComClass]
    internal sealed partial class XbfXamlMember : IXbfMember
    {
        private DirectUIXamlMember _xamlMember;
        private XbfMetadataProvider _xmp;

        public XbfXamlMember(DirectUIXamlMember member, XbfMetadataProvider xmp)
        {
            _xamlMember = member;
            _xmp = xmp;
        }

        public bool IsAttachable => _xamlMember.IsAttachable;

        public bool IsDependencyProperty => _xamlMember.IsDependencyProperty;

        public bool IsReadOnly => _xamlMember.IsReadOnly;

        public string Name => _xamlMember.Name;

        public XbfXamlType TargetType => _xmp.GetXmpXamlType((DirectUIXamlType)_xamlMember.TargetType);

        public XbfXamlType Type => _xmp.GetXmpXamlType((DirectUIXamlType)_xamlMember.Type);

        bool IXbfMember.get_IsAttachable() => IsAttachable;

        bool IXbfMember.get_IsDependencyProperty() => IsDependencyProperty;

        bool IXbfMember.get_IsReadOnly() => IsReadOnly;

        string IXbfMember.get_Name() => Name;

        IXbfType IXbfMember.get_TargetType() => TargetType;

        IXbfType IXbfMember.get_Type() => Type;

        IntPtr IXbfMember.GetValue(IntPtr instance) => throw new NotImplementedException();

        void IXbfMember.SetValue(IntPtr instance, IntPtr value) => throw new NotImplementedException();
    }
}