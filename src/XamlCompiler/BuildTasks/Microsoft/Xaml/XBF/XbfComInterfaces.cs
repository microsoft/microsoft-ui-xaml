// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// Source-generated COM declarations of the metadata provider interfaces that GenXbf.dll consumes, implemented by
// XbfMetadataProvider, XbfXamlType and XbfXamlMember. They mirror the native ABI in dxaml\xcp\tools\GenXbfDLL\XbfMetadataProvider.h
// exactly (vtable order and signatures). The COM source generator only supports methods, so property getters are declared as
// get_X methods in vtable order. All methods return an HRESULT, with the managed return value (if any) being the native
// [out, retval] parameter.

namespace Microsoft.UI.Xaml.Markup.Compiler.XBF
{
    using System;
    using System.Runtime.InteropServices;
    using System.Runtime.InteropServices.Marshalling;

    /// <summary>
    /// Blittable equivalent of the native <c>wxaml_interop::TypeName</c> struct (<c>{ HSTRING Name; TypeKind Kind; }</c>).
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    internal struct XbfTypeName
    {
        public IntPtr Name;
        public int Kind;
    }

    [GeneratedComInterface]
    [Guid("d0aa6fc8-087f-46cf-b36a-7e68f8295ceb")]
    internal partial interface IXbfMember
    {
        [return: MarshalAs(UnmanagedType.U1)]
        bool get_IsAttachable();

        [return: MarshalAs(UnmanagedType.U1)]
        bool get_IsDependencyProperty();

        [return: MarshalAs(UnmanagedType.U1)]
        bool get_IsReadOnly();

        [return: MarshalUsing(typeof(BStrStringMarshaller))]
        string get_Name();

        IXbfType get_TargetType();

        IXbfType get_Type();

        // IInspectable* parameters/results (not used by GenXbf.dll, and not implemented)
        IntPtr GetValue(IntPtr instance);

        void SetValue(IntPtr instance, IntPtr value);
    }

    [GeneratedComInterface]
    [Guid("a50fc345-4c61-411b-8a68-13da7b7c4ee4")]
    internal unsafe partial interface IXbfType
    {
        IXbfType get_BaseType();

        IXbfMember get_ContentProperty();

        [return: MarshalUsing(typeof(BStrStringMarshaller))]
        string get_FullName();

        [return: MarshalAs(UnmanagedType.U1)]
        bool get_IsArray();

        [return: MarshalAs(UnmanagedType.U1)]
        bool get_IsCollection();

        [return: MarshalAs(UnmanagedType.U1)]
        bool get_IsConstructible();

        [return: MarshalAs(UnmanagedType.U1)]
        bool get_IsDictionary();

        [return: MarshalAs(UnmanagedType.U1)]
        bool get_IsMarkupExtension();

        [return: MarshalAs(UnmanagedType.U1)]
        bool get_IsBindable();

        IXbfType get_ItemType();

        IXbfType get_KeyType();

        IXbfType get_BoxedType();

        // Native: get_UnderlyingType(TypeName*). GenXbf.dll doesn't call it (XamlTypeAdapter builds the TypeName from get_FullName).
        void get_UnderlyingType(XbfTypeName* value);

        IntPtr ActivateInstance();

        IntPtr CreateFromString([MarshalUsing(typeof(BStrStringMarshaller))] string value);

        IXbfMember GetMember([MarshalUsing(typeof(BStrStringMarshaller))] string name);

        void AddToVector(IntPtr instance, IntPtr value);

        void AddToMap(IntPtr instance, IntPtr key, IntPtr value);

        void RunInitializer();
    }

    [GeneratedComInterface]
    [Guid("ef46679c-4ec5-447a-bd26-e04f1d2c2551")]
    internal unsafe partial interface IXbfMetadataProvider
    {
        // Native: GetXamlType(TypeName by value, IXbfType**). GenXbf.dll doesn't call it (XamlMetadataProviderAdapter::GetXamlType
        // forwards to GetXamlTypeByFullName).
        IXbfType GetXamlType(XbfTypeName type);

        IXbfType GetXamlTypeByFullName([MarshalUsing(typeof(BStrStringMarshaller))] string fullName);

        // Native: GetXmlnsDefinitions(UINT* length, XmlnsDefinition** definitions). Not used by GenXbf.dll.
        void GetXmlnsDefinitions(uint* length, void** definitions);
    }
}
