// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Markup.Compiler.XBF
{
    using System;
    using DirectUI;

    internal class XbfXamlMember : IXbfMember
    {
        private DirectUIXamlMember _xamlMember;
        private IXmpXamlType _xmp;

        public XbfXamlMember(DirectUIXamlMember member, IXmpXamlType xmp)
        {
            _xamlMember = member;
            _xmp = xmp;
        }

        public bool IsAttachable
        {
            get { return _xamlMember.IsAttachable; }
        }

        public bool IsDependencyProperty
        {
            get { return _xamlMember.IsDependencyProperty; }
        }

        public bool IsReadOnly
        {
            get { return _xamlMember.IsReadOnly; }
        }

        public string Name
        {
            get { return _xamlMember.Name; }
        }

        public IXbfType TargetType
        {
            get
            {
                return _xmp.GetXmpXamlType((DirectUIXamlType)_xamlMember.TargetType);
            }
        }

        public IXbfType Type
        {
            get
            {
                return _xmp.GetXmpXamlType((DirectUIXamlType)_xamlMember.Type);
            }
        }

        public void SetValue(object instance, object value)
        {
            throw new NotImplementedException();    // not used
        }

        public object GetValue(object instance)
        {
            throw new NotImplementedException();    // not used
        }
    }
}