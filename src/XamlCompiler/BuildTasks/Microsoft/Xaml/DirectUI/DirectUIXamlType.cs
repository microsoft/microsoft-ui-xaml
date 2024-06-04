// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Markup.Compiler.DirectUI
{
    using System;
    using System.Collections.Generic;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.Reflection;
    using System.Xaml;
    using System.Xaml.Schema;
    using Lmr;

    internal class DirectUIXamlType : XamlType, IXamlTypeMeta
    {
        private List<XamlType> inlineCollectionAllowedContentTypes;
        private bool? isValueType;
        private bool? isCodeGenType;
        private bool? hasWinUIContract;
        private bool? isSignedChar;
        private bool? isEnumBaseType;
        private bool? isEnum;
        private bool? isTemplateType;
        private List<string> enumNamesList;
        private bool? isNullableGeneric;
        private Type nullableGenericInnerType;
        private bool? isDeprecated;
        private bool isHardDeprecated;
        private string deprecatedMessage;
        private bool? isExperimental;
        private MethodInfo addMethod;
        MemberProxyMetadata frameworkTemplateProxyMetadata;
        private bool? _hasValueConverter;
        private bool? _hasMetadataProvider;
        private bool? _isStyle;
        private bool? _isBinding;
        private bool? _isPropertyPath;
        private CreateFromStringMethod _createFromStringMethod;
        private bool? _hasINotifyPropertyChanged;
        private bool? _hasINotifyCollectionChanged;
        private bool? _hasINotifyDataErrorInfo;
        private bool? _hasObservableVector;
        private bool? _hasObservableMap;
        private bool? _isDelegate;
        private bool? _isDerivedFromFrameworkTemplate;
        private bool? _isDerivedFromValidationCommand;
        private bool? _isDerivedFromResourceDictionary;
        private bool? _isDerivedFromUIElement;
        private bool? _isDerivedFromFlyoutBase;
        private bool? _isDerivedFromMarkupExtension;
        private bool? _isDerivedFromTextBox;

        public ApiInformation ApiInformation { get; }
        public bool HasApiInformation => ApiInformation != null;
        public Platform TargetPlatform { get; }

        public DirectUIXamlType(Type underlyingType, XamlSchemaContext schemaContext)
            : this(underlyingType, schemaContext, null, Platform.Any)
        { }

        public DirectUIXamlType(Type underlyingType, XamlSchemaContext schemaContext, ApiInformation apiInformation, Platform targetPlatform)
            : base(underlyingType, schemaContext)
        {
            this.ApiInformation = apiInformation;
            this.TargetPlatform = targetPlatform;
        }

        public DirectUIXamlType(string name, IList<XamlType> typeArgs, XamlSchemaContext schemaContext)
            : base(name, typeArgs, schemaContext)
        { }

        public bool IsValueType
        {
            get
            {
                if (!this.isValueType.HasValue)
                {
                    this.isValueType = this.LookupIsValueType();
                }
                return this.isValueType.Value;
            }
        }

        public bool IsCodeGenType
        {
            get
            {
                if (!this.isCodeGenType.HasValue)
                {
                    this.isCodeGenType = this.LookupIsCodeGenType();
                }
                return this.isCodeGenType.Value;
            }
        }

        public bool HasWinUIContract
        {
            get
            {
                if (!this.hasWinUIContract.HasValue)
                {
                    this.hasWinUIContract = this.LookupHasWinUIContract();
                }
                return this.hasWinUIContract.Value;
            }
        }

        public bool IsSignedChar
        {
            get
            {
                if (!this.isSignedChar.HasValue)
                {
                    this.isSignedChar = this.LookupIsSignedChar();
                }
                return this.isSignedChar.Value;
            }
        }

        public bool IsEnumBaseType
        {
            get
            {
                if (!this.isEnumBaseType.HasValue)
                {
                    this.isEnumBaseType = this.LookupIsEnumBaseType();
                }
                return this.isEnumBaseType.Value;
            }
        }

        public bool IsTemplateType
        {
            get
            {
                if (!this.isTemplateType.HasValue)
                {
                    this.isTemplateType = this.LookupIsTemplateType();
                }
                return this.isTemplateType.Value;
            }
        }

        public bool IsEnum
        {
            get
            {
                if (!this.isEnum.HasValue)
                {
                    this.isEnum = this.LookupIsEnum();
                }
                return this.isEnum.Value;
            }
        }

        public List<string> EnumNames
        {
            get
            {
                if (this.enumNamesList == null)
                {
                    this.enumNamesList = this.LookupEnumNames();
                }
                return this.enumNamesList;
            }
        }

        public bool IsInvalidType
        {
            // SignedChar is invalid because C++ doesn't allow it.
            // Enum (the base class) is invalid because the Jupiter runtime passes
            // the Setter Boxed<int> and casting int to Enum doesn't work like
            // casting MyEnum to Enum might have worked.
            get { return this.IsSignedChar || this.IsEnumBaseType; }
        }

        public bool IsDeprecated
        {
            get
            {
                if (!this.isDeprecated.HasValue)
                {
                    this.isDeprecated = this.LookupIsDeprecated();
                }
                return this.isDeprecated.Value;
            }
        }

        public bool IsHardDeprecated
        {
            get
            {   // Tied to IsDeprecated
                return this.IsDeprecated ? this.isHardDeprecated : false;
            }
        }

        public string DeprecatedMessage
        {
            get
            {   // Tied to IsDeprecated
                return this.IsDeprecated ? this.deprecatedMessage : string.Empty;
            }
        }

        public bool IsExperimental
        {
            get
            {
                if (!this.isExperimental.HasValue)
                {
                    this.isExperimental = this.LookupIsExperimental();
                }
                return this.isExperimental.Value;
            }
        }

        public string AddMethodName
        {
            get
            {
                if (this.IsCollection || this.IsDictionary)
                {
                    if (this.addMethod != null)
                    {
                        return this.addMethod.Name;
                    }
                    return KnownMembers.Add;
                }
                return null;
            }
        }

        public bool IsValueConverter
        {
            get
            {
                if (!this._hasValueConverter.HasValue)
                {
                    this._hasValueConverter = UnderlyingType.GetInterface(KnownTypes.IValueConverter) != null;
                }
                return this._hasValueConverter.Value;
            }
        }

        public bool IsMetadataProvider
        {
            get
            {
                if (!this._hasMetadataProvider.HasValue)
                {
                    this._hasMetadataProvider = UnderlyingType.GetInterface(KnownTypes.IXamlMetadataProvider) != null;
                }
                return this._hasMetadataProvider.Value;
            }
        }


        public bool IsAssignableToStyle
        {
            get
            {
                if (!this._isStyle.HasValue)
                {
                    this._isStyle = ((DirectUISchemaContext)SchemaContext).DirectUISystem.Style.IsAssignableFrom(UnderlyingType);
                }
                return this._isStyle.Value;
            }
        }

        public bool IsAssignableToBinding
        {
            get
            {
                if (!this._isBinding.HasValue)
                {
                    this._isBinding = ((DirectUISchemaContext)SchemaContext).DirectUISystem.Binding.IsAssignableFrom(UnderlyingType);
                }
                return this._isBinding.Value;
            }
        }

        public bool IsAssignableToPropertyPath
        {
            get
            {
                if (!this._isPropertyPath.HasValue)
                {
                    this._isPropertyPath = ((DirectUISchemaContext)SchemaContext).DirectUISystem.PropertyPath.IsAssignableFrom(UnderlyingType);
                }
                return this._isPropertyPath.Value;
            }
        }

        public bool HasInterface(string fullName)
        {
            System.Type underlyingType = this.UnderlyingType;
            if (fullName == underlyingType.Namespace + "." + underlyingType.Name)
            {
                return true;
            }
            return underlyingType.GetInterface(fullName) != null;
        }

        public bool ImplementsINotifyPropertyChanged
        {
            get
            {
                if (!this._hasINotifyPropertyChanged.HasValue)
                {
                    this._hasINotifyPropertyChanged = this.HasInterface(KnownTypes.INotifyPropertyChanged) || this.HasInterface(KnownTypes.XamlINotifyPropertyChanged);
                }
                return this._hasINotifyPropertyChanged.Value;
            }
        }

        public bool ImplementsINotifyDataErrorInfo
        {
            get
            {
                if (!this._hasINotifyDataErrorInfo.HasValue)
                {
                    this._hasINotifyDataErrorInfo = this.HasInterface(KnownTypes.INotifyDataErrorInfo) || this.HasInterface(KnownTypes.XamlINotifyDataErrorInfo);
                }
                return this._hasINotifyDataErrorInfo.Value;
            }
        }

        public bool ImplementsINotifyCollectionChanged
        {
            get
            {
                if (!this._hasINotifyCollectionChanged.HasValue)
                {
                    this._hasINotifyCollectionChanged = this.HasInterface(KnownTypes.INotifyCollectionChanged) || this.HasInterface(KnownTypes.XamlINotifyCollectionChanged);
                }
                return this._hasINotifyCollectionChanged.Value;
            }
        }

        public bool ImplementsIObservableVector
        {
            get
            {
                if (!this._hasObservableVector.HasValue)
                {
                    this._hasObservableVector = this.HasInterface(KnownTypes.IObservableVector);
                }
                return this._hasObservableVector.Value;
            }
        }

        public bool ImplementsIObservableMap
        {
            get
            {
                if (!this._hasObservableMap.HasValue)
                {
                    this._hasObservableMap = this.HasInterface(KnownTypes.IObservableMap);
                }
                return this._hasObservableMap.Value;
            }
        }

        public bool IsDelegate
        {
            get
            {
                if (!this._isDelegate.HasValue)
                {
                    DirectUISchemaContext schema = this.SchemaContext as DirectUISchemaContext;
                    this._isDelegate = UnderlyingType.IsSubclassOf(schema.DirectUISystem.Delegate);
                }
                return this._isDelegate.Value;
            }
        }

        public bool IsDerivedFromFrameworkTemplate
        {
            get
            {
                if (!this._isDerivedFromFrameworkTemplate.HasValue)
                {
                    DirectUISchemaContext schema = this.SchemaContext as DirectUISchemaContext;
                    this._isDerivedFromFrameworkTemplate = schema.DirectUISystem.FrameworkTemplate.IsAssignableFrom(UnderlyingType);
                }
                return this._isDerivedFromFrameworkTemplate.Value;
            }
        }

        public bool IsDerivedFromValidationCommand
        {
            get
            {
                if (!this._isDerivedFromValidationCommand.HasValue)
                {
                    DirectUISchemaContext schema = this.SchemaContext as DirectUISchemaContext;
                    this._isDerivedFromValidationCommand = schema.DirectUISystem.FrameworkTemplate.IsAssignableFrom(UnderlyingType);
                }
                return this._isDerivedFromFrameworkTemplate.Value;
            }
        }

        public bool IsDerivedFromResourceDictionary
        {
            get
            {
                if (!this._isDerivedFromResourceDictionary.HasValue)
                {
                    DirectUISchemaContext schema = this.SchemaContext as DirectUISchemaContext;
                    this._isDerivedFromResourceDictionary = schema.DirectUISystem.ResourceDictionary.IsAssignableFrom(UnderlyingType);
                }
                return this._isDerivedFromResourceDictionary.Value;
            }
        }

        public bool IsDerivedFromUIElement
        {
            get
            {
                if (!this._isDerivedFromUIElement.HasValue)
                {
                    DirectUISchemaContext schema = this.SchemaContext as DirectUISchemaContext;
                    this._isDerivedFromUIElement = schema.DirectUISystem.UIElement.IsAssignableFrom(UnderlyingType);
                }
                return this._isDerivedFromUIElement.Value;
            }
        }

        public bool IsDerivedFromFlyoutBase
        {
            get
            {
                if (!this._isDerivedFromFlyoutBase.HasValue)
                {
                    DirectUISchemaContext schema = this.SchemaContext as DirectUISchemaContext;
                    this._isDerivedFromFlyoutBase = schema.DirectUISystem.FlyoutBase.IsAssignableFrom(UnderlyingType);
                }
                return this._isDerivedFromFlyoutBase.Value;
            }
        }

        public bool IsDerivedFromMarkupExtension
        {
            get
            {
                if (!this._isDerivedFromMarkupExtension.HasValue)
                {
                    Type meType = (this.SchemaContext as DirectUISchemaContext)?.DirectUISystem.MarkupExtension;
                    this._isDerivedFromMarkupExtension = (meType == null ? false : meType.IsAssignableFrom(UnderlyingType));
                }
                return this._isDerivedFromMarkupExtension.Value;
            }
        }

        public bool IsDerivedFromTextBox
        {
            get
            {
                if (!this._isDerivedFromTextBox.HasValue)
                {
                    Type meType = (this.SchemaContext as DirectUISchemaContext)?.DirectUISystem.TextBox;
                    this._isDerivedFromTextBox = (meType == null ? false : meType.IsAssignableFrom(UnderlyingType));
                }
                return this._isDerivedFromTextBox.Value;
            }
        }

        public XamlMember LookupMember_SkipReadOnlyCheck(string propertyName)
        {
            return this.LookupMember(propertyName, true);
        }

        public bool IsNullableGeneric(out Type innerType)
        {
            if (!this.isNullableGeneric.HasValue)
            {
                this.isNullableGeneric = LookupIsNullableGeneric(out this.nullableGenericInnerType);
            }
            innerType = this.nullableGenericInnerType;
            return this.isNullableGeneric.Value;
        }

#if DEBUG
        public override bool CanAssignTo(XamlType xamlType)
        {
            this.CheckIsLmrType(this.UnderlyingType);
            this.CheckIsLmrType(xamlType.UnderlyingType);
            return base.CanAssignTo(xamlType);
        }

        private void CheckIsLmrType(Type t)
        {
            if (t != null)
            {
                MetadataOnlyCommonType lmrT = t as MetadataOnlyCommonType;
                Debug.Assert(lmrT != null);
            }
        }
#endif

        protected virtual bool LookupIsSignedChar()
        {
            if (this.UnderlyingType == null)
            {
                return false;  // SignedChar is known.
            }

            string typeName = this.UnderlyingType.FullName;
            return (typeName == "System.SByte" || typeName == "System.Int8");
        }

        protected virtual bool LookupIsEnumBaseType()
        {
            if (this.UnderlyingType == null)
            {
                return false;  // Enum is known.
            }

            string typeName = UnderlyingType.FullName;
            return typeName == "System.Enum";
        }

        protected virtual bool LookupIsValueType()
        {
            if (this.UnderlyingType == null)
            {
                return false;  // all Value types are known.
            }

            return this.UnderlyingType.IsValueType;
        }

        private CustomAttributeData GetDirectAttribute(string name)
        {
            return GetAttribute(name, false);
        }

        private CustomAttributeData GetAttribute(string attrName, bool inherited = true)
        {
            return GetAttribute(this.UnderlyingType, attrName, inherited);
        }

        private static CustomAttributeData GetAttribute(Type type, string attrName, bool inherited = true)
        {
            CustomAttributeData attr = null;

            if (type != null)
            {
                attr = ReflectionHelper.FindAttributeByTypeName(type, inherited, attrName);
            }

            return attr;
        }

        //Note: this is hardcoded for attributes which have their deprecation message as the first argument of their constructor.
        private bool CheckDeprecationAttribute(string attrName, string defaultMessage)
        {
            CustomAttributeData attr = GetAttribute(attrName);

            if (attr != null)
            {
                Type attrType = attr.Constructor.DeclaringType;

                this.deprecatedMessage = ReflectionHelper.GetAttributeConstructorArgument(attr, 0, null) as string;
                // in case there is some problem reading the message, insert a simple replacement.
                if (string.IsNullOrWhiteSpace(deprecatedMessage))
                {
                    this.deprecatedMessage = defaultMessage;
                }

                //Windows.Foundation.Metadata.DeprecatedAttribute has an additional second argument which specifies whether
                //this is hard deprecated.
                if (attrName.Equals(KnownTypes.DeprecatedAttribute))
                {
                    int level = (int)ReflectionHelper.GetAttributeConstructorArgument(attr, 1, null);
                    if (level != 0)
                    {
                        this.isHardDeprecated = true;
                    }
                }
                return true;
            }
            return false;
        }

        protected virtual bool LookupIsDeprecated()
        {
            bool hasDeprecatedAttribute = CheckDeprecationAttribute(KnownTypes.DeprecatedAttribute, KnownStrings.DeprecatedAttributeDefaultMessage);
            if (!hasDeprecatedAttribute)
            {
                return CheckDeprecationAttribute(KnownTypes.ObsoleteAttribute, KnownStrings.ObsoleteAttributeDefaultMessage);
            }

            return hasDeprecatedAttribute;
        }

        protected virtual bool LookupIsExperimental()
        {
            return GetAttribute(KnownTypes.ExperimentalAttribute) != null;
        }

        protected virtual bool LookupIsTemplateType()
        {
            return this.DirectUISystem.FrameworkTemplate.IsAssignableFrom(this.UnderlyingType);
        }

        // This is the list of types in the "Windows.Foundation" namespace that
        // the runtime knows about. These are types for which we do not need to generate type info.
        internal static List<string> WindowsFoundationSystemTypes = new List<string>()
        {
            "Point", "Size", "Rect", "Uri", "TimeSpan"
        };

        // This is the list of types in the "System" namespace that
        // the runtime knows about. These are types for which we do not need to generate type info.
        private static List<string> PrimitiveSystemTypes = new List<string>()
        {
            "Object", "Double", "Single", "Int16", "Char16", "Int32", "Int64", "UInt32", "UInt64", "Boolean", "String"
        };

        protected virtual bool LookupIsCodeGenType()
        {
            if (this.UnderlyingType == null)
            {
                return false;
            }

            if (HasWinUIContract)
            {
                return false;
            }

            string typeName = this.UnderlyingType.FullName;
            if (typeName.StartsWith(KnownNamespaces.WindowsFoundationPrefix))
            {
                string baseName = typeName.Substring(KnownNamespaces.WindowsFoundationPrefix.Length);
                if (DirectUIXamlType.WindowsFoundationSystemTypes.Contains(baseName))
                {
                    return false;
                }
            }

            if (typeName.StartsWith(KnownNamespaces.SystemPrefix))
            {
                string baseName = typeName.Substring(KnownNamespaces.SystemPrefix.Length);
                if (DirectUIXamlType.PrimitiveSystemTypes.Contains(baseName))
                {
                    return false;
                }
            }

            if (typeName.StartsWith(KnownNamespaces.ProxyTypes))
            {
                return false;
            }

            return true;
        }

        protected bool LookupIsEnum()
        {
            if (this.UnderlyingType == null)
            {
                return false;
            }
            return this.UnderlyingType.IsEnum;
        }

        protected List<string> LookupEnumNames()
        {
            List<string> stringList = null;
            if (this.IsEnum)
            {
                stringList = new List<string>();
                foreach (string enumValue in Enum.GetNames(this.UnderlyingType))
                {
                    stringList.Add(enumValue);
                }
            }
            return stringList;
        }

        protected override XamlType LookupBaseType()
        {
            Type underlyingType = this.UnderlyingType;

            if (underlyingType != null)
            {
                if (underlyingType.BaseType != null)
                {
                    // special case Runtime Class Wrappers whose BaseType is __ComObject as
                    // this is a CLR injected type which essentially means the same as Object base type
                    if (underlyingType.BaseType.FullName == "System.__ComObject" ||
                        underlyingType.BaseType.FullName == "System.Runtime.InteropServices.WindowsRuntime.RuntimeClass")
                    {
                        return this.SchemaContext.GetXamlType(this.DirectUISystem.Object);
                    }
                    else
                    {
                        return this.SchemaContext.GetXamlType(underlyingType.BaseType);
                    }
                }
            }
            return null;
        }

        protected override XamlCollectionKind LookupCollectionKind()
        {
            // The System.Xaml base class looks at typeof(IEnumerable) etc
            // which won't work with LMR.  We must do it all ourselves.

            if (this.UnderlyingType.IsArray)
            {
                return XamlCollectionKind.Array;
            }

            XamlCollectionKind collectionKind;
            if (this.UnderlyingType.IsInterface)
            {
                if (this.GetCollectionKind(this.UnderlyingType, out collectionKind))
                {
                    return collectionKind;
                }
            }
            Type type = this.GetCollectionReleventInterface();
            if (type != null)
            {
                if (this.GetCollectionKind(type, out collectionKind))
                {
                    return collectionKind;
                }
            }
            return XamlCollectionKind.None;
        }

        protected override XamlType LookupItemType()
        {
            XamlType itemType = base.LookupItemType();
            if (itemType == null)
            {
                Type result = this.DirectUISystem.Object;
                if (this.IsCollection && this.addMethod != null)
                {
                    ParameterInfo[] addParams = this.addMethod.GetParameters();
                    if (addParams.Length == 1)
                    {
                        result = addParams[0].ParameterType;
                    }
                }
                if (this.IsDictionary && this.addMethod != null)
                {
                    ParameterInfo[] addParams = this.addMethod.GetParameters();
                    if (addParams.Length == 2)
                    {
                        result = addParams[1].ParameterType;
                    }
                }
                itemType = this.SchemaContext.GetXamlType(result);
            }
            return itemType;
        }

        protected override XamlType LookupKeyType()
        {
            XamlType itemType = base.LookupKeyType();
            if (itemType == null)
            {
                Type result = this.DirectUISystem.Object;
                if (this.IsDictionary && this.addMethod != null)
                {
                    ParameterInfo[] addParams = this.addMethod.GetParameters();
                    if (addParams.Length == 2)
                    {
                        result = addParams[0].ParameterType;
                    }
                }
                itemType = this.SchemaContext.GetXamlType(result);
            }
            return itemType;
        }

        protected override bool LookupIsConstructible()
        {
            if (this.UnderlyingType == null)
            {
                return true;
            }

            bool isConstructable = base.LookupIsConstructible();
            if (!isConstructable)
            {
                return false;
            }

            // If System.Xaml says it is Constructable
            // For Jupiter: we don't allow Constructor Arguments.
            if (this.ConstructionRequiresArguments)
            {
                return false;
            }

            if (this.IsValueType)
            {
                return false;
            }
            return true;
        }

        protected override XamlMember LookupAttachableMember(string name)
        {
            XamlMember member = base.LookupAttachableMember(name);
            if (member != null)
            {
                member = new DirectUIXamlMember(member.Name, member.Invoker, this.SchemaContext, this.ApiInformation);
            }
            return member;
        }

        protected override IEnumerable<XamlMember> LookupAllAttachableMembers()
        {
            const BindingFlags AttachableProperties_BF = BindingFlags.Static | BindingFlags.FlattenHierarchy | BindingFlags.Public;

            List<XamlMember> allAttachableMembers = new List<XamlMember>();

            if (this.UnderlyingType != null)
            {
                // System.Xaml has a sophisticated and complex approach to resolving Ambiguous Setters and Getters.
                // public overrides internal, matched get/set pairs allow extra setters to be ignored,
                // more derived getters and setters overrides base class methods...
                // That was necessary for WPF backward compat.
                // Here we find all the static GetXXX and SetXXX methods, with the correct argument signature
                // just to collect likely names and call down into System.Xaml to resolve them (or not)
                MethodInfo[] allMethods = this.UnderlyingType.GetMethods(AttachableProperties_BF);
                HashSet<string> attachedProperties = new HashSet<string>();
                foreach (MethodInfo method in allMethods)
                {
                    if (method.IsPublic)
                    {
                        string name;
                        if (this.IsAttachablePropertySetter(method, out name) || this.IsAttachablePropertyGetter(method, out name))
                        {
                            if (!attachedProperties.Contains(name))
                            {
                                attachedProperties.Add(name);
                            }
                        }
                    }
                }

                foreach (string name in attachedProperties)
                {
                    XamlMember member = this.LookupAttachableMember(name);
                    if (member != null)
                    {
                        // There is a bug in System.Xaml where Set-only properties will have Type == "void"
                        // Which renders them unusable for code gen.  (and wrong in general)
                        if (member.Type.UnderlyingType != this.DirectUISystem.Void)
                        {
                            allAttachableMembers.Add(member);
                        }
                    }
                }
            }
            return (allAttachableMembers.Count == 0) ? null : allAttachableMembers;
        }

        protected override XamlMember LookupMember(string propertyName, bool skipReadOnlyCheck)
        {
            XamlMember member = base.LookupMember(propertyName, skipReadOnlyCheck);
            if (member == null)
            {
                if (this.IsFrameworkTemplateProperty(propertyName))
                {
                    member = new ProxyDirectUIXamlMember(this.FrameworkTemplateProxyMetadata, this);
                }
            }
            else
            {
                DirectUISchemaContext schema = this.SchemaContext as DirectUISchemaContext;
                Debug.Assert(schema != null);
                if (member.IsEvent)
                {
                    EventInfo eventInfo = member.UnderlyingMember as EventInfo;
                    if (eventInfo != null && schema != null)
                    {
                        member = new DirectUIXamlMember(eventInfo, schema, this.ApiInformation);
                    }
                }
                else
                {
                    PropertyInfo propertyInfo = member.UnderlyingMember as PropertyInfo;
                    if (propertyInfo != null && schema != null)
                    {
                        member = new DirectUIXamlMember(propertyInfo, schema, this.ApiInformation);
                    }
                }
            }
            return member;
        }

        protected override IEnumerable<XamlMember> LookupAllMembers()
        {
            List<XamlMember> allMembers = new List<XamlMember>();
            if (this.UnderlyingType != null)
            {
                if (this.DirectUISystem.FrameworkTemplate.IsAssignableFrom(this.UnderlyingType))
                {
                    XamlMember template = this.GetPropertyOrUnknown(KnownMembers.Template, false);
                    allMembers.Add(template);
                }
                DirectUISchemaContext directSchema = this.SchemaContext as DirectUISchemaContext;
                BindingFlags AllProperties_BF = BindingFlags.Instance | BindingFlags.Public;

                PropertyInfo[] propertyList = this.UnderlyingType.GetProperties(AllProperties_BF);
                foreach (PropertyInfo propertyInfo in propertyList)
                {
                    DirectUIXamlMember xamlMember = new DirectUIXamlMember(propertyInfo, directSchema, this.ApiInformation);
                    allMembers.Add(xamlMember);
                }
                EventInfo[] eventList = this.UnderlyingType.GetEvents(AllProperties_BF);
                foreach (EventInfo eventInfo in eventList)
                {
                    DirectUIXamlMember xamlMember = new DirectUIXamlMember(eventInfo, directSchema, this.ApiInformation);
                    allMembers.Add(xamlMember);
                }
            }
            return allMembers;
        }

        protected override XamlMember LookupContentProperty()
        {
            CustomAttributeData customAttr = ReflectionHelper.FindAttributeByTypeName(this.UnderlyingType, true, this.DirectUISystem.ContentPropertyAttribute.FullName);
            XamlMember contentProperty = null;

            if (customAttr != null)
            {
                Type attrType = customAttr.Constructor.DeclaringType;

                string contentPropertyName = ReflectionHelper.GetAttributeConstructorArgument(customAttr, -1, "Name") as string;
                if (contentPropertyName != null)
                {
                    contentProperty = this.GetPropertyOrUnknown(contentPropertyName, false);
                }
            }

            if (contentProperty == null)
            {
                if (this.DirectUISystem.FrameworkTemplate.IsAssignableFrom(this.UnderlyingType))
                {
                    // we override LookupMember to supply the fictional "Template" property.
                    contentProperty = this.GetPropertyOrUnknown(KnownMembers.Template, false);
                }
            }
            return contentProperty;
        }

        // Base implemenation of LookupAllowedContentTypes works fine.
        // However, we need to add [ContentWrapperAttribute] on InlineCollection
        // to return the right allowed content types for InlineCollection.
        //
        protected override IList<XamlType> LookupAllowedContentTypes()
        {
            if (this.UnderlyingType == this.DirectUISystem.InlineCollection)
            {
                if (this.inlineCollectionAllowedContentTypes == null)
                {
                    this.inlineCollectionAllowedContentTypes = new List<XamlType>();
                    this.inlineCollectionAllowedContentTypes.Add(SchemaContext.GetXamlType(DirectUISystem.Inline));
                    this.inlineCollectionAllowedContentTypes.Add(SchemaContext.GetXamlType(DirectUISystem.String));
                    this.inlineCollectionAllowedContentTypes.Add(SchemaContext.GetXamlType(DirectUISystem.UIElement));
                }
                return this.inlineCollectionAllowedContentTypes;
            }
            return null;   // base doesn't do anything usefull for Jupiter
        }

        protected override XamlType LookupMarkupExtensionReturnType()
        {
            if (this.UnderlyingType == this.DirectUISystem.RelativeSource)
            {
                //RelativeSource returns a RelativeSource object.
                return this;
            }
            return this.DirectUIXamlLanguage.Object;
        }

        private IDirectUIXamlLanguage DirectUIXamlLanguage
        {
            get
            {
                DirectUISchemaContext schema = this.SchemaContext as DirectUISchemaContext;
                return schema.DirectUIXamlLanguage;
            }
        }

        private DirectUISystem DirectUISystem
        {
            get
            {
                DirectUISchemaContext schema = this.SchemaContext as DirectUISchemaContext;
                return schema.DirectUISystem;
            }
        }


        // Markup Extensions are a weak point in the API compat in DirectUI
        // Binding and RelativeSource really exist as classes but are not derived
        // from MarkupExtension (there is no MarkupExtension base class).
        // TemplateBinding and StaticResource don't  exists at all as objects
        // in the DirectUI OM and are faked by the DirectUI Schema.
        // [See: ProxyMetaData.cs]
        protected override bool LookupIsMarkupExtension()
        {
            return
                this.UnderlyingType == this.DirectUISystem.Binding ||
                this.UnderlyingType == this.DirectUISystem.RelativeSource ||
                (DirectUISystem.MarkupExtension != null && DirectUISystem.MarkupExtension.IsAssignableFrom(this.UnderlyingType));
        }

        protected override bool LookupIsNameScope()
        {
            return false;   // base doesn't do anything usefull for Jupiter
        }

        protected override bool LookupIsXData()
        {
            return false;   // base doesn't do anything usefull for Jupiter
        }

        protected override bool LookupTrimSurroundingWhitespace()
        {
            return this.UnderlyingType == this.DirectUISystem.LineBreak;
        }

        protected override bool LookupIsWhitespaceSignificantCollection()
        {
            return this.UnderlyingType == this.DirectUISystem.InlineCollection;
        }

        protected override XamlMember LookupAliasedProperty(XamlDirective directive)
        {
            if (directive == XamlLanguage.Key)
            {
                if (this.UnderlyingType == this.DirectUISystem.Style)
                {
                    return this.LookupMember(KnownMembers.TargetType, true);
                }
            }
            else if (directive == XamlLanguage.Lang)
            {
                if (this.DirectUISystem.FrameworkElement.IsAssignableFrom(this.UnderlyingType) ||
                    this.DirectUISystem.Inline.IsAssignableFrom(this.UnderlyingType))
                {
                    return this.LookupMember(KnownMembers.Language, true);
                }
            }
            else if (directive == XamlLanguage.Name)
            {
                if (this.DirectUISystem.FrameworkElement.IsAssignableFrom(this.UnderlyingType))
                {
                    return this.LookupMember(KnownMembers.Name, true);
                }
            }

            XamlMember xamlMember = null;
            try
            {
                xamlMember = base.LookupAliasedProperty(directive);
            }
            catch (TypeLoadException ex)
            {
                Debug.WriteLine("Attribute Reflection Error (LookupAliasedProperty): " + ex.Message);
            }
            return xamlMember;
        }

        protected override XamlValueConverter<TypeConverter> LookupTypeConverter()
        {
            XamlValueConverter<TypeConverter> typeConverter = this.FindTypeConverter(this.UnderlyingType);
            if (typeConverter == null)
            {
                Type innerType;
                if (this.IsNullableGeneric(out innerType))
                {
                    typeConverter = this.FindTypeConverter(innerType);
                }
            }
            return typeConverter;
        }

        protected XamlValueConverter<TypeConverter> FindTypeConverter(Type underlyingType)
        {
            if (underlyingType.IsEnum)
            {
                return new XamlValueConverter<TypeConverter>(typeof(DirectUINativeTypeConverter), this);
            }
            else if (DirectUIXamlType._namesOfTypesWithConverters.Contains(underlyingType.FullName))
            {
                return new XamlValueConverter<TypeConverter>(typeof(DirectUINativeTypeConverter), this);
            }
            else if (this.CreateFromStringMethod.Exists)
            {
                return new XamlValueConverter<TypeConverter>(typeof(DirectUINativeTypeConverter), this);
            }
            
            return null;
        }

        protected override bool LookupIsNullable()
        {
            if (this.UnderlyingType == null)
            {
                return base.LookupIsNullable();
            }
            if (this.IsString())
            {
                return DirectUIXamlLanguage.IsStringNullable;
            }
            Type innerType;
            return (!this.UnderlyingType.IsValueType || this.IsNullableGeneric(out innerType));
        }

        protected virtual bool LookupIsNullableGeneric(out Type innerType)
        {
            innerType = null;
            if (this.UnderlyingType == null || !this.UnderlyingType.IsGenericType)
            {
                return false;
            }

            Type tNull = this.UnderlyingType.GetGenericTypeDefinition();
            if (tNull == this.DirectUISystem.Nullable || tNull == this.DirectUISystem.IReference)
            {
                Type[] typeArgs = this.UnderlyingType.GetGenericArguments();
                Debug.Assert(typeArgs.Length == 1);
                innerType = typeArgs[0];
                return true;
            }
            return false;
        }

        private bool IsFrameworkTemplateProperty(string propertyName)
        {
            if (KS.Eq(propertyName, KnownMembers.Template))
            {
                if (this.DirectUISystem.FrameworkTemplate.IsAssignableFrom(this.UnderlyingType))
                {
                    return true;
                }
            }
            return false;
        }

        private MemberProxyMetadata FrameworkTemplateProxyMetadata
        {
            get
            {
                if (this.frameworkTemplateProxyMetadata == null)
                {
                    this.frameworkTemplateProxyMetadata = new MemberProxyMetadata(KnownMembers.Template, this.DirectUIXamlLanguage.Object);
                    this.frameworkTemplateProxyMetadata.DeferringLoader = new XamlValueConverter<XamlDeferringLoader>(this.DirectUISystem.Object, this.DirectUIXamlLanguage.UIElement);
                }
                return this.frameworkTemplateProxyMetadata;
            }
        }

        private Type GetCollectionReleventInterface()
        {
            List<Type> outList = new List<Type>();
            Type[] interfaceList = this.UnderlyingType.GetInterfaces();

            // search for Dictionaries first because Dictionaries are also Collections.
            foreach (Type type in interfaceList)
            {
                string fullName = (type.IsGenericType) ? type.GetGenericTypeDefinition().FullName : type.FullName;
                if (fullName == KnownTypes.IDictionary || fullName == KnownTypes.IMap)
                {
                    return type;
                }
            }

            foreach (Type type in interfaceList)
            {
                string fullName = (type.IsGenericType) ? type.GetGenericTypeDefinition().FullName : type.FullName;
                if (fullName == KnownTypes.ICollection || fullName == KnownTypes.IVector)
                {
                    return type;
                }
            }
            return null;
        }

        private bool GetCollectionKind(Type type, out XamlCollectionKind collectionKind)
        {
            collectionKind = XamlCollectionKind.None;

            string fullName = (type.IsGenericType) ? type.GetGenericTypeDefinition().FullName : type.FullName;
            string addMethodName = string.Empty;
            int paramCount = 0;

            if (fullName == KnownTypes.ICollection)
            {
                collectionKind = XamlCollectionKind.Collection;
                addMethodName = KnownMembers.Add;
                paramCount = 1;
            }
            else if (fullName == KnownTypes.IVector)
            {
                collectionKind = XamlCollectionKind.Collection;
                addMethodName = KnownMembers.Append;
                paramCount = 1;
            }
            else if (fullName == KnownTypes.IDictionary)
            {
                collectionKind = XamlCollectionKind.Dictionary;
                addMethodName = KnownMembers.Add;
                paramCount = 2;
            }
            else if (fullName == KnownTypes.IMap)
            {
                collectionKind = XamlCollectionKind.Dictionary;
                addMethodName = KnownMembers.Insert;
                paramCount = 2;
            }
            if (collectionKind != XamlCollectionKind.None)
            {
                bool moreThanOne;
                this.addMethod = this.GetMethodWithNParameters(type, addMethodName, paramCount, out moreThanOne);

                if (moreThanOne)
                {
                    DirectUISchemaContext directUiSchemaContext = this.SchemaContext as DirectUISchemaContext;
                    directUiSchemaContext.SchemaErrors.Add(new XamlSchemaError_AmbiguousCollectionAdd(UnderlyingType.FullName, addMethodName, paramCount));
                }
                return (this.addMethod != null);
            }
            return false;
        }

        private MethodInfo GetMethodWithNParameters(Type type, string methodName, int paramCount, out bool hasMoreThanOne)
        {
            MethodInfo result = null;
            BindingFlags flags = BindingFlags.Instance | BindingFlags.Public;
            MemberInfo[] addMembers = type.GetMember(methodName, MemberTypes.Method, flags);
            if (addMembers != null)
            {
                foreach (MemberInfo mi in addMembers)
                {
                    MethodInfo method = (MethodInfo)mi;
                    ParameterInfo[] paramInfos = method.GetParameters();
                    if (paramInfos == null || paramInfos.Length != paramCount)
                    {
                        continue;
                    }
                    if (result != null)
                    {
                        // More than one Add method
                        hasMoreThanOne = true;
                        return result;
                    }
                    result = method;
                }
            }
            hasMoreThanOne = false;
            return result;
        }

        private bool IsAttachablePropertyGetter(MethodInfo methodInfo, out string name)
        {
            name = null;
            if (!KS.StartsWith(methodInfo.Name, KnownStrings.Get))
            {
                return false;
            }

            // Static Getter has one argument and does not return void
            ParameterInfo[] parameterInfo = methodInfo.GetParameters();
            if ((parameterInfo.Length != 1) || (methodInfo.ReturnType == typeof(void)))
            {
                return false;
            }

            if (methodInfo.IsGenericMethod || methodInfo.ReturnType.IsGenericParameter || parameterInfo[0].ParameterType.IsGenericType)
            {
                return false;
            }

            name = methodInfo.Name.Substring(KnownStrings.Get.Length);
            return true;
        }

        private bool IsAttachablePropertySetter(MethodInfo methodInfo, out string name)
        {
            name = null;
            if (!KS.StartsWith(methodInfo.Name, KnownStrings.Set))
            {
                return false;
            }

            // Static Setter has two arguments 
            ParameterInfo[] parameterInfo = methodInfo.GetParameters();
            if (parameterInfo.Length != 2)
            {
                return false;
            }

            if (methodInfo.IsGenericMethod ||
                methodInfo.ReturnType.IsGenericParameter ||
                parameterInfo[0].ParameterType.IsGenericType ||
                parameterInfo[1].ParameterType.IsGenericType)
            {
                return false;
            }

            name = methodInfo.Name.Substring(KnownStrings.Set.Length);
            return true;
        }

        private static List<string> _namesOfTypesWithConverters = new List<string>()
        {
            "System.Double",
            "System.Boolean",
            "System.Numerics.Matrix3x2",
            "System.Numerics.Matrix4x4",
            "System.Numerics.Vector2",
            "System.Numerics.Vector3",
            "System.String",
            "System.TimeSpan",
            "System.Single",
            "Windows.Foundation.TimeSpan",
            "System.Int32",
            "System.EventHandler",
            "System.Type",
            "Windows.Foundation.Uri",
            "System.Uri",
            "Windows.Foundation.Numerics.Matrix3x2",
            "Windows.Foundation.Numerics.Matrix4x4",
            "Windows.Foundation.Numerics.Vector2",
            "Windows.Foundation.Numerics.Vector3",
            "Windows.Foundation.Point",
            "Windows.Foundation.Size",
            "Windows.Foundation.Rect",
            "Windows.Media.Playback.IMediaPlaybackSource",
            KnownNamespaces.WindowsUI + ".Color",
            KnownNamespaces.Text + ".FontWeight",
            KnownNamespaces.Text + ".FontStyle",
            KnownNamespaces.Text + ".FontStretch",
            KnownNamespaces.Xaml + ".RoutedEvent",
            KnownNamespaces.Xaml + ".Thickness",
            KnownNamespaces.Xaml + ".CornerRadius",
            KnownNamespaces.Xaml + ".TextWrapping",
            KnownNamespaces.Xaml + ".TextAlignment",
            KnownNamespaces.Xaml + ".FontWeight",
            KnownNamespaces.Xaml + ".FontStyle",
            KnownNamespaces.Xaml + ".FontStretch",
            KnownNamespaces.Xaml + ".Duration",
            KnownNamespaces.Xaml + ".DependencyProperty",
            KnownNamespaces.Xaml + ".DependencyProperty",
            KnownNamespaces.Xaml + ".Visibility",
            KnownNamespaces.Xaml + ".HorizontalAlignment",
            KnownNamespaces.Xaml + ".PropertyPath",
            KnownNamespaces.Xaml + ".TargetPropertyPath",
            KnownNamespaces.Xaml + ".VerticalAlignment",
            KnownNamespaces.Xaml + ".GridLength",
            KnownNamespaces.Xaml + ".GridUnitType",
            KnownNamespaces.Xaml + ".DurationType",
            KnownNamespaces.Xaml + ".FlowDirection",
            KnownNamespaces.XamlAutomation + ".DockPosition",
            KnownNamespaces.XamlAutomation + ".ExpandCollapseState",
            KnownNamespaces.XamlAutomation + ".ScrollAmount",
            KnownNamespaces.XamlAutomation + ".RowOrColumnMajor",
            KnownNamespaces.XamlAutomation + ".ToggleState",
            KnownNamespaces.XamlAutomation + ".WindowVisualState",
            KnownNamespaces.XamlAutomation + ".WindowInteractionState",
            KnownNamespaces.XamlAutomation + ".SupportedTextSelection",
            KnownNamespaces.XamlAutomationPeers + ".AutomationControlType",
            KnownNamespaces.XamlAutomationPeers + ".AutomationEvents",
            KnownNamespaces.XamlAutomationPeers + ".PatternInterface",
            KnownNamespaces.XamlAutomationPeers + ".AutomationOrientation",
            KnownNamespaces.XamlAutomationText + ".TextUnit",
            KnownNamespaces.XamlAutomationText + ".TextPatternRangeEndpoint",
            KnownNamespaces.XamlControls + ".Orientation",
            KnownNamespaces.XamlControls + ".IncrementalLoadingTrigger",
            KnownNamespaces.XamlControls + ".StretchDirection",
            KnownNamespaces.XamlControls + ".ScrollBarVisibility",
            KnownNamespaces.XamlControls + ".ClickMode",
            KnownNamespaces.XamlControls + ".SelectionMode",
            KnownNamespaces.XamlControls + ".VirtualizationMode",
            KnownNamespaces.XamlControls + ".ScrollMode",
            KnownNamespaces.XamlControls + ".ZoomMode",
            KnownNamespaces.XamlControls + ".SnapPointsType",
            KnownNamespaces.XamlControls + ".IconElement",
            KnownNamespaces.XamlControls + ".SymbolIcon",
            KnownNamespaces.XamlControls + ".ColumnDefinition",
            KnownNamespaces.XamlControls + ".RowDefinition",
            KnownNamespaces.XamlControlsPrimitives + ".GeneratorPosition",
            KnownNamespaces.XamlControlsPrimitives + ".GeneratorDirection",
            KnownNamespaces.XamlControlsPrimitives + ".ScrollEventType",
            KnownNamespaces.XamlControlsPrimitives + ".SnapPointsAlignment",
            KnownNamespaces.XamlDocuments + ".Run",
            KnownNamespaces.XamlInput + ".KeyboardNavigationMode",
            KnownNamespaces.XamlInput + ".ManipulationModes",
            KnownNamespaces.XamlInput + ".ModifierKeys",
            KnownNamespaces.XamlInput + ".Key",
            KnownNamespaces.XamlInput + ".InputScope",
            KnownNamespaces.WindowsXamlInterop + ".TypeName",
            KnownNamespaces.XamlMedia + ".Color",        // old Color moved to Windows.UI
            KnownNamespaces.XamlMedia + ".FillRule",
            KnownNamespaces.XamlMedia + ".PenLineCap",
            KnownNamespaces.XamlMedia + ".PenLineJoin",
            KnownNamespaces.XamlMedia + ".SweepDirection",
            KnownNamespaces.XamlMedia + ".ColorInterpolationMode",
            KnownNamespaces.XamlMedia + ".GradientSpreadMethod",
            KnownNamespaces.XamlMedia + ".BrushMappingMode",
            KnownNamespaces.XamlMedia + ".AlignmentX",
            KnownNamespaces.XamlMedia + ".AlignmentY",
            KnownNamespaces.XamlMedia + ".Stretch",
            KnownNamespaces.XamlMedia + ".DoubleCollection",
            KnownNamespaces.XamlMedia + ".PointCollection",
            KnownNamespaces.XamlMedia + ".Transform",
            KnownNamespaces.XamlMedia + ".Brush",
            KnownNamespaces.XamlMedia + ".SolidColorBrush",
            KnownNamespaces.XamlMedia + ".Geometry",
            KnownNamespaces.XamlMedia + ".ImageSource",
            KnownNamespaces.XamlMedia + ".TransformGroup",
            KnownNamespaces.XamlMedia + ".MatrixTransform",
            KnownNamespaces.XamlMedia + ".Matrix",
            KnownNamespaces.XamlMedia + ".FontFamily",
            KnownNamespaces.XamlMedia + ".MediaCanPlayResponse",
            KnownNamespaces.XamlMedia + ".Video3DMode",
            KnownNamespaces.XamlMedia + ".CacheMode",
            KnownNamespaces.XamlMediaAnimation + ".FillBehavior",
            KnownNamespaces.XamlMediaAnimation + ".EasingMode",
            KnownNamespaces.XamlMediaAnimation + ".ClockState",
            KnownNamespaces.XamlMediaAnimation + ".RepeatBehavior",
            KnownNamespaces.XamlMediaAnimation + ".KeyTime",
            KnownNamespaces.XamlMediaAnimation + ".KeySpline",
            KnownNamespaces.XamlMediaAnimation + ".RepeatBehaviorType",
            KnownNamespaces.XamlMediaImaging + ".BitmapCreateOptions",
            KnownNamespaces.XamlMediaMedia3D + ".Matrix3D",
            KnownNamespaces.Xaml + ".Vector3TransitionComponents",
        };

        private XamlMember GetPropertyOrUnknown(string propertyName, bool skipReadOnlyCheck)
        {
            XamlMember result = skipReadOnlyCheck ? this.LookupMember(propertyName, true) : this.GetMember(propertyName);
            if (result == null)
            {
                result = new DirectUIXamlMember(propertyName, this /*declaringType*/);
            }
            return result;
        }

        public CreateFromStringMethod CreateFromStringMethod
        {
            get
            {
                if (_createFromStringMethod == null)
                {
                    string method = LookupCreateFromStringMethod();
                    if (!string.IsNullOrEmpty(method))
                    {
                        if (method.HasAtLeastTwo('.'))
                        {
                            // Full name is provided - has at least 2 dots.
                            _createFromStringMethod = new CreateFromStringMethod(method);
                        }
                        else
                        {
                            // Local name is provided.
                            _createFromStringMethod = new CreateFromStringMethod(this, method);
                        }
                    }
                    else
                    {
                        // There is no CreateFromString method.
                        _createFromStringMethod = new CreateFromStringMethod();
                    }
                }
                return _createFromStringMethod;
            }
        }

        private string LookupCreateFromStringMethod()
        {
            foreach (var attr in ReflectionHelper.GetCustomAttributeData(UnderlyingType, false, KnownTypes.CreateFromStringAttribute))
            {
                foreach (var namedArg in attr.NamedArguments)
                {
                    if (namedArg.MemberName.Equals("MethodName", StringComparison.InvariantCultureIgnoreCase))
                    {
                        return namedArg.TypedValue.Value.ToString();
                    }
                }
            }
            return String.Empty;
        }

        internal static bool LookupHasWinUIContract(Type type)
        {
            var contractVersion = GetAttribute(type, KnownTypes.ContractVersionAttribute, false);
            if (contractVersion != null)
            {
                try
                {
                    var contractType = contractVersion?.ConstructorArguments?[0].Value as Type;
                    if (contractType != null)
                    {
                        return contractType.FullName == KnownTypes.WinUIContract;
                    }
                }
                catch (TypeLoadException)
                {
                    // Some types have a contract associated with them defined in a WinMD file that we aren't referencing -
                    // e.g., Windows.Foundation.UniversalApiContract.  If we fail to resolve those contracts, we'll just ignore them.
                }
            }
            return false;
        }

        private bool LookupHasWinUIContract()
        {
            return LookupHasWinUIContract(this.UnderlyingType);
        }
    }
}
