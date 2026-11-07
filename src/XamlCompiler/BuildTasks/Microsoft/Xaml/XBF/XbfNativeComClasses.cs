// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// Native AOT (net10.0) only: exposes the XBF metadata provider objects to GenXbf.dll through source-generated COM.
// The objects themselves are the COM servers (rather than separate wrapper objects), so that every managed instance
// maps to a single, stable COM identity, like with the built-in COM interop used by the other target frameworks.
// Not implemented members throw NotImplementedException, which is returned to native code as E_NOTIMPL.

namespace Microsoft.UI.Xaml.Markup.Compiler.XBF
{
    using System;
    using System.Runtime.InteropServices.Marshalling;

    [GeneratedComClass]
    internal partial class XbfMetadataProvider : IXbfMetadataProviderNative
    {
        IXbfTypeNative IXbfMetadataProviderNative.GetXamlType(XbfTypeName type)
        {
            throw new NotImplementedException();
        }

        IXbfTypeNative IXbfMetadataProviderNative.GetXamlTypeByFullName(string fullName)
        {
            return (IXbfTypeNative)GetXamlType(fullName);
        }

        unsafe void IXbfMetadataProviderNative.GetXmlnsDefinitions(uint* length, void** definitions)
        {
            throw new NotImplementedException();
        }
    }

    [GeneratedComClass]
    internal partial class XbfXamlType : IXbfTypeNative
    {
        IXbfTypeNative IXbfTypeNative.get_BaseType() => (IXbfTypeNative)BaseType;

        IXbfMemberNative IXbfTypeNative.get_ContentProperty() => (IXbfMemberNative)ContentProperty;

        string IXbfTypeNative.get_FullName() => FullName;

        bool IXbfTypeNative.get_IsArray() => IsArray;

        bool IXbfTypeNative.get_IsCollection() => IsCollection;

        bool IXbfTypeNative.get_IsConstructible() => IsConstructible;

        bool IXbfTypeNative.get_IsDictionary() => IsDictionary;

        bool IXbfTypeNative.get_IsMarkupExtension() => IsMarkupExtension;

        bool IXbfTypeNative.get_IsBindable() => IsBindable;

        IXbfTypeNative IXbfTypeNative.get_ItemType() => (IXbfTypeNative)ItemType;

        IXbfTypeNative IXbfTypeNative.get_KeyType() => (IXbfTypeNative)KeyType;

        IXbfTypeNative IXbfTypeNative.get_BoxedType() => (IXbfTypeNative)BoxedType;

        unsafe void IXbfTypeNative.get_UnderlyingType(XbfTypeName* value) => throw new NotImplementedException();

        IntPtr IXbfTypeNative.ActivateInstance() => throw new NotImplementedException();

        IntPtr IXbfTypeNative.CreateFromString(string value) => throw new NotImplementedException();

        IXbfMemberNative IXbfTypeNative.GetMember(string name) => (IXbfMemberNative)GetMember(name);

        void IXbfTypeNative.AddToVector(IntPtr instance, IntPtr value) => throw new NotImplementedException();

        void IXbfTypeNative.AddToMap(IntPtr instance, IntPtr key, IntPtr value) => throw new NotImplementedException();

        void IXbfTypeNative.RunInitializer() => throw new NotImplementedException();
    }

    [GeneratedComClass]
    internal partial class XbfXamlMember : IXbfMemberNative
    {
        bool IXbfMemberNative.get_IsAttachable() => IsAttachable;

        bool IXbfMemberNative.get_IsDependencyProperty() => IsDependencyProperty;

        bool IXbfMemberNative.get_IsReadOnly() => IsReadOnly;

        string IXbfMemberNative.get_Name() => Name;

        IXbfTypeNative IXbfMemberNative.get_TargetType() => (IXbfTypeNative)TargetType;

        IXbfTypeNative IXbfMemberNative.get_Type() => (IXbfTypeNative)Type;

        IntPtr IXbfMemberNative.GetValue(IntPtr instance) => throw new NotImplementedException();

        void IXbfMemberNative.SetValue(IntPtr instance, IntPtr value) => throw new NotImplementedException();
    }
}
