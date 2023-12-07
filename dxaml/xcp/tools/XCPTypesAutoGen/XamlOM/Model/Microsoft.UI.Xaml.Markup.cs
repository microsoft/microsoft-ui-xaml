// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using System;
using XamlOM;

namespace Microsoft.UI.Xaml.Markup
{
    [DXamlIdlGroup("coretypes2")]
    [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true, IsExcludedFromNewTypeTable = true)]
    [Guids(ClassGuid = "3c1b0d18-b6ec-4773-9b81-e56a53c3923e")]
    public static class XamlReader
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public static Windows.Foundation.Object Load(Windows.Foundation.String xaml)
        {
            return default(Windows.Foundation.Object);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public static Windows.Foundation.Object LoadWithInitialTemplateValidation(Windows.Foundation.String xaml)
        {
            return default(Windows.Foundation.Object);
        }
    }

    [DXamlIdlGroup("coretypes2")]
    [CodeGen(CodeGenLevel.LookupOnly)]
    [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true, IsExcludedFromNewTypeTable = true)]
    [HandWritten]
    [Guids(ClassGuid = "6bffe1c5-d5c6-4b6a-aaaf-373e86fd65aa")]
    public static class XamlBinaryWriter
    {
        public static XamlBinaryWriterErrorInformation Write(
            Windows.Foundation.Collections.IVector<Windows.Storage.Streams.IRandomAccessStream> inputStreams,
            Windows.Foundation.Collections.IVector<Windows.Storage.Streams.IRandomAccessStream> outputStreams,
            IXamlMetadataProvider xamlMetadataProvider)
        {
            return default(XamlBinaryWriterErrorInformation);
        }
    }

    [DXamlIdlGroup("coretypes2")]
    [CodeGen(CodeGenLevel.LookupOnly)]
    [TypeTable(IsExcludedFromNewTypeTable = true)]
    public interface IComponentConnector
    {
        void Connect(int connectionId, Windows.Foundation.Object target);

        Microsoft.UI.Xaml.Markup.IComponentConnector GetBindingConnector(int connectionId, Windows.Foundation.Object target);
    }

    [DXamlIdlGroup("coretypes2")]
    public interface IDataTemplateComponent
    {
        void Recycle();

        void ProcessBindings(Windows.Foundation.Object item, int itemIndex, int phase, out int nextPhase);
    }

    [DXamlIdlGroup("coretypes2")]
    [CodeGen(CodeGenLevel.LookupOnly)]
    [TypeTable(IsExcludedFromNewTypeTable = true)]
    public interface IXamlType
    {
        IXamlType BaseType { get; }
        IXamlMember ContentProperty { get; }
        string FullName { get; }
        bool IsArray { get; }
        bool IsCollection { get; }
        bool IsConstructible { get; }
        bool IsDictionary { get; }
        bool IsMarkupExtension { get; }
        bool IsBindable { get; }
        IXamlType ItemType { get; }
        IXamlType KeyType { get; }
        IXamlType BoxedType { get; }
        Windows.UI.Xaml.Interop.TypeName UnderlyingType { get; }

        [ReturnTypeParameterName("instance")]
        object ActivateInstance();

        [ReturnTypeParameterName("instance")]
        object CreateFromString(string value);

        [ReturnTypeParameterName("xamlMember")]
        IXamlMember GetMember(string name);

        void AddToVector(object instance, object value);
        void AddToMap(object instance, object key, object value);
        void RunInitializer();
    }

    [DXamlIdlGroup("coretypes2")]
    [CodeGen(CodeGenLevel.LookupOnly)]
    [TypeTable(IsExcludedFromNewTypeTable = true)]
    public interface IXamlMember
    {
        bool IsAttachable { get; }
        bool IsDependencyProperty { get; }
        bool IsReadOnly { get; }
        string Name { get; }
        IXamlType TargetType { get; }
        IXamlType Type { get; }

        [ReturnTypeParameterName("value")]
        object GetValue(object instance);

        void SetValue(object instance, object value);
    }

    [DXamlIdlGroup("coretypes2")]
    [CodeGen(CodeGenLevel.LookupOnly)]
    [TypeTable(IsExcludedFromNewTypeTable = true)]
    public interface IXamlMetadataProvider
    {
        [DXamlOverloadName("GetXamlType")]
        [DefaultOverload]
        [ReturnTypeParameterName("xamlType")]
        IXamlType GetXamlType(Windows.UI.Xaml.Interop.TypeName type);

        [DXamlOverloadName("GetXamlType")]
        [ReturnTypeParameterName("xamlType")]
        IXamlType GetXamlTypeByFullName(string fullName);

        [ReturnTypeParameterName("definitions")]
        [CountParameterName("length")]
        XmlnsDefinition[] GetXmlnsDefinitions();
    }

    [DXamlIdlGroup("coretypes2")]
    [CodeGen(CodeGenLevel.LookupOnly)]
    [TypeTable(IsExcludedFromNewTypeTable = true)]
    public interface IXamlBindScopeDiagnostics
    {
        void Disable(int lineNumber, int columnNumber);
    }

    [DXamlIdlGroup("coretypes2")]
    [HideFromOldCodeGen]
    [TypeTable(IsExcludedFromNewTypeTable = true)]
    public struct XmlnsDefinition
    {
        public string XmlNamespace { get; set; }
        public string Namespace { get; set; }
    }

    [DXamlIdlGroup("coretypes2")]
    [HideFromOldCodeGen]
    [TypeTable(IsExcludedFromNewTypeTable = true)]
    public struct XamlBinaryWriterErrorInformation
    {
        public uint InputStreamIndex { get; set; }
        public uint LineNumber { get; set; }
        public uint LinePosition { get; set; }
    }

    [Platform(typeof(Microsoft.UI.Xaml.WinUIContract), 1, ForcePrimaryInterfaceGeneration = true)]
    [DXamlIdlGroup("coretypes2")]
    [Guids(ClassGuid = "09866282-4cdb-49c8-8c7d-9d40363c4f96")]
    public static class XamlMarkupHelper
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        static public void UnloadObject(Microsoft.UI.Xaml.DependencyObject element)
        {
        }
    }

    [Platform(typeof(Microsoft.UI.Xaml.WinUIContract), 1, ForcePrimaryInterfaceGeneration = true)]
    [PartialFactory]
    [DXamlIdlGroup("coretypes2")]
    [Guids(ClassGuid = "5907bcb4-ff97-47ad-8049-fa5b5da86032")]
    public static class XamlBindingHelper
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public static void SuspendRendering(UIElement target)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public static void ResumeRendering(UIElement target)
        {
        }

        [Attached(TargetType = typeof(DependencyObject))]
        [NativeStorageType(OM.ValueType.valueObject)]
        public static Microsoft.UI.Xaml.Markup.IDataTemplateComponent AttachedDataTemplateComponent
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public static Windows.Foundation.Object ConvertValue(Type type, Windows.Foundation.Object value)
        {
            return default(Windows.Foundation.Object);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public static void SetPropertyFromString(Windows.Foundation.Object dependencyObject, DependencyProperty propertyToSet, Windows.Foundation.String value)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public static void SetPropertyFromBoolean(Windows.Foundation.Object dependencyObject, DependencyProperty propertyToSet, Windows.Foundation.Boolean value)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public static void SetPropertyFromChar16(Windows.Foundation.Object dependencyObject, DependencyProperty propertyToSet, Windows.Foundation.Char16 value)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public static void SetPropertyFromDateTime(Windows.Foundation.Object dependencyObject, DependencyProperty propertyToSet, Windows.Foundation.DateTime value)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public static void SetPropertyFromDouble(Windows.Foundation.Object dependencyObject, DependencyProperty propertyToSet, Windows.Foundation.Double value)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public static void SetPropertyFromInt32(Windows.Foundation.Object dependencyObject, DependencyProperty propertyToSet, Windows.Foundation.Int32 value)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public static void SetPropertyFromUInt32(Windows.Foundation.Object dependencyObject, DependencyProperty propertyToSet, Windows.Foundation.UInt32 value)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public static void SetPropertyFromInt64(Windows.Foundation.Object dependencyObject, DependencyProperty propertyToSet, Windows.Foundation.Int64 value)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public static void SetPropertyFromUInt64(Windows.Foundation.Object dependencyObject, DependencyProperty propertyToSet, Windows.Foundation.UInt64 value)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public static void SetPropertyFromSingle(Windows.Foundation.Object dependencyObject, DependencyProperty propertyToSet, Windows.Foundation.Float value)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public static void SetPropertyFromPoint(Windows.Foundation.Object dependencyObject, DependencyProperty propertyToSet, Windows.Foundation.Point value)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public static void SetPropertyFromRect(Windows.Foundation.Object dependencyObject, DependencyProperty propertyToSet, Windows.Foundation.Rect value)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public static void SetPropertyFromSize(Windows.Foundation.Object dependencyObject, DependencyProperty propertyToSet, Windows.Foundation.Size value)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public static void SetPropertyFromTimeSpan(Windows.Foundation.Object dependencyObject, DependencyProperty propertyToSet, Windows.Foundation.TimeSpan value)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public static void SetPropertyFromByte(Windows.Foundation.Object dependencyObject, DependencyProperty propertyToSet, Windows.Foundation.Byte value)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public static void SetPropertyFromUri(Windows.Foundation.Object dependencyObject, DependencyProperty propertyToSet, Windows.Foundation.Uri value)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public static void SetPropertyFromObject(Windows.Foundation.Object dependencyObject, DependencyProperty propertyToSet, Windows.Foundation.Object value)
        {
        }
    }

    [DXamlIdlGroup("coretypes2")]
    internal interface IXamlPredicate
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Windows.Foundation.Boolean Evaluate(Windows.Foundation.Collections.IVectorView<string> arguments);
    }

    [DXamlIdlGroup("coretypes2")]
    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [Guids(ClassGuid = "43622c65-6db6-42bc-be67-2c34a6df463b")]
    [NativeName("CIsApiContractPresentPredicate")]
    [Implements(typeof(Microsoft.UI.Xaml.Markup.IXamlPredicate))]
    internal sealed class IsApiContractPresent
        : Microsoft.UI.Xaml.DependencyObject
    {
        public IsApiContractPresent() { }
    }

    [DXamlIdlGroup("coretypes2")]
    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [Guids(ClassGuid = "ca2964a2-d3da-4aed-b266-f937028a3d63")]
    [NativeName("CIsApiContractNotPresentPredicate")]
    [Implements(typeof(Microsoft.UI.Xaml.Markup.IXamlPredicate))]
    internal sealed class IsApiContractNotPresent
        : Microsoft.UI.Xaml.DependencyObject
    {
        public IsApiContractNotPresent() { }
    }

    [DXamlIdlGroup("coretypes2")]
    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [Guids(ClassGuid = "6328c17a-af24-4f3f-b99e-6813bd0b14ee")]
    [NativeName("CIsPropertyPresentPredicate")]
    [Implements(typeof(Microsoft.UI.Xaml.Markup.IXamlPredicate))]
    internal sealed class IsPropertyPresent
        : Microsoft.UI.Xaml.DependencyObject
    {
        public IsPropertyPresent() { }
    }

    [DXamlIdlGroup("coretypes2")]
    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [Guids(ClassGuid = "7405ce51-1c13-45a8-b7b4-3859c1dd4749")]
    [NativeName("CIsPropertyNotPresentPredicate")]
    [Implements(typeof(Microsoft.UI.Xaml.Markup.IXamlPredicate))]
    internal sealed class IsPropertyNotPresent
        : Microsoft.UI.Xaml.DependencyObject
    {
        public IsPropertyNotPresent() { }
    }

    [DXamlIdlGroup("coretypes2")]
    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [Guids(ClassGuid = "01da2943-8f8d-42ca-96ef-79cbc4d083ab")]
    [NativeName("CIsTypePresentPredicate")]
    [Implements(typeof(Microsoft.UI.Xaml.Markup.IXamlPredicate))]
    internal sealed class IsTypePresent
        : Microsoft.UI.Xaml.DependencyObject
    {
        public IsTypePresent() { }
    }

    [DXamlIdlGroup("coretypes2")]
    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [Guids(ClassGuid = "e8c566d6-0e2e-414d-81a8-eb1be51a045e")]
    [NativeName("CIsTypeNotPresentPredicate")]
    [Implements(typeof(Microsoft.UI.Xaml.Markup.IXamlPredicate))]
    internal sealed class IsTypeNotPresent
        : Microsoft.UI.Xaml.DependencyObject
    {
        public IsTypeNotPresent() { }
    }

    [AttributeUsage(AttributeTargets.Class, AllowMultiple = false, Inherited = true)]
    [IdlAttributeTarget(AttributeTargets.Class)]
    [DXamlIdlGroup("coretypes2")]
    public class MarkupExtensionReturnTypeAttribute : Attribute
    {
        public Type ReturnType { get; set; }
    }

    [CodeGen(CodeGenLevel.IdlAndPartialStub)]
    [ClassFlags(IsMarkupExtension = true)]
    [TypeTable(IsExcludedFromCore = true, ForceInclude = true)]
    [Guids(ClassGuid = "26e480bc-a928-43c6-9077-673514724e8d")]
    [DXamlIdlGroup("coretypes2")]
    public class MarkupExtension
        : Windows.Foundation.Object
    {
        public MarkupExtension() { }

        protected virtual Windows.Foundation.Object ProvideValue()
        {
            return default(Windows.Foundation.Object);
        }

        [DXamlName("ProvideValueWithIXamlServiceProvider")]
        [DXamlOverloadName("ProvideValue")]
        protected virtual Windows.Foundation.Object ProvideValue(IXamlServiceProvider serviceProvider)
        {
            return default(Windows.Foundation.Object);
        }
    }

    [AttributeUsage(AttributeTargets.Class, AllowMultiple = false, Inherited = true)]
    [IdlAttributeTarget(AttributeTargets.Class)]
    [DXamlIdlGroup("coretypes2")]
    public class FullXamlMetadataProviderAttribute : Attribute
    {
    }

    [TypeTable(ForceInclude = true)]
    [DXamlIdlGroup("coretypes2")]
    public interface IProvideValueTarget
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Windows.Foundation.Object TargetObject
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Windows.Foundation.Object TargetProperty
        {
            get;
        }
    }

    [TypeTable(ForceInclude = true)]
    [DXamlIdlGroup("coretypes2")]
    public interface IXamlTypeResolver
    {
        Windows.UI.Xaml.Interop.TypeName Resolve(Windows.Foundation.String qualifiedTypeName);
    }

    [TypeTable(ForceInclude = true)]
    [DXamlIdlGroup("coretypes2")]
    public interface IUriContext
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Windows.Foundation.Uri BaseUri
        {
            get;
        }
    }

    [TypeTable(ForceInclude = true)]
    [DXamlIdlGroup("coretypes2")]
    public interface IRootObjectProvider
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Windows.Foundation.Object RootObject
        {
            get;
        }
    }

    [CodeGen(partial: true, Level = CodeGenLevel.IdlAndPartialStub)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [TypeTable(IsXbfType = false, IsExcludedFromCore = true)]
    [Guids(ClassGuid = "13337e47-68e0-404f-848a-3b5cf375242b")]
    [DXamlIdlGroup("coretypes2")]
    public sealed class ProvideValueTargetProperty
        : Windows.Foundation.Object
    {
        public ProvideValueTargetProperty() { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public string Name
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Windows.UI.Xaml.Interop.TypeName Type
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Windows.UI.Xaml.Interop.TypeName DeclaringType
        {
            get;
        }
    }

    [CodeGen(partial: true, Level = CodeGenLevel.IdlAndPartialStub)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [TypeTable(IsXbfType = false, IsExcludedFromCore = true)]
    [Guids(ClassGuid = "4958850a-79a7-4dfc-86a5-46709302212b")]
    [Implements(typeof(Microsoft.UI.Xaml.IXamlServiceProvider))]
    [Implements(typeof(Microsoft.UI.Xaml.Markup.IProvideValueTarget))]
    [Implements(typeof(Microsoft.UI.Xaml.Markup.IXamlTypeResolver))]
    [Implements(typeof(Microsoft.UI.Xaml.Markup.IUriContext))]
    [Implements(typeof(Microsoft.UI.Xaml.Markup.IRootObjectProvider))]
    internal sealed class ParserServiceProvider
        : Windows.Foundation.Object
    {
        public ParserServiceProvider() { }
    }
}
