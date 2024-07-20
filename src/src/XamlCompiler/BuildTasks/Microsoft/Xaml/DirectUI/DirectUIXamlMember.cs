// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Diagnostics;
using System.Reflection;
using System.Xaml;
using System.Xaml.Schema;

namespace Microsoft.UI.Xaml.Markup.Compiler.DirectUI
{
    internal class DirectUIXamlMember : XamlMember, IXamlMemberMeta
    {
        XamlType _eventArgType;
        private bool? _isDependencyProperty;
        private bool? _isIndexer;
        private bool? _hasPublicGetter;
        private bool? _hasPublicSetter;
        private bool? _isDeprecated;
        private bool? _isTemplate;
        private bool _isHardDeprecated;
        private string _deprecatedMessage;
        private bool? isExperimental;
        private MethodInfo _attachablePropertySetter;
        private MethodInfo _attachablePropertyGetter;
        public ApiInformation ApiInformation { get; set; }

        public DirectUIXamlMember(PropertyInfo propertyInfo, DirectUISchemaContext schemaContext, ApiInformation apiInformation)
            : base(propertyInfo, schemaContext)
        {
            this.ApiInformation = apiInformation;
        }

        public DirectUIXamlMember(EventInfo eventInfo, DirectUISchemaContext schemaContext, ApiInformation apiInformation)
            : base(eventInfo, schemaContext)
        {
            this.ApiInformation = apiInformation;
        }

        public DirectUIXamlMember(string name, DirectUIXamlType declaringType, bool isAttachable, ApiInformation apiInformation)
            : base(name, declaringType, isAttachable)
        {
            this.ApiInformation = apiInformation;
        }

        public DirectUIXamlMember(string name, DirectUIXamlType declaringType)
            : base(name, declaringType, false /*isAttachable*/)
        {
            this.ApiInformation = declaringType.ApiInformation;
        }

        public DirectUIXamlMember(string attachablePropertyName, XamlMemberInvoker invoker, XamlSchemaContext schemaContext, ApiInformation apiInformation)
            : base(attachablePropertyName, invoker.UnderlyingGetter, invoker.UnderlyingSetter, schemaContext, invoker)
        {
            _attachablePropertyGetter = invoker.UnderlyingGetter;
            _attachablePropertySetter = invoker.UnderlyingSetter;
            this.ApiInformation = apiInformation;
        }

        // ---- Additional public properties

        public bool IsDependencyProperty
        {
            // In WinRT the Dependency Properties are "properties" on
            // the Static interface.  (in WPF they are fields not properties)
            get
            {
                if (!_isDependencyProperty.HasValue)
                {
                    _isDependencyProperty = LookupIsDependencyProperty();
                }
                return _isDependencyProperty.Value;
            }
        }

        public bool IsDeprecated
        {
            get
            {
                if (!_isDeprecated.HasValue)
                {
                    _isDeprecated = LookupIsDeprecated();
                }
                return _isDeprecated.Value;
            }
        }

        public bool IsHardDeprecated
        {
            get
            {   // Tied to IsDeprecated
                return IsDeprecated ? _isHardDeprecated : false;
            }
        }

        public string DeprecatedMessage
        {
            get
            {   // Tied to IsDeprecated
                return IsDeprecated ? _deprecatedMessage : String.Empty;
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

        public XamlType EventArgumentType
        {
            get
            {
                // If it isn't an event then no event args.
                if (!IsEvent)
                {
                    return null;
                }

                if (_eventArgType == null)
                {
                    _eventArgType = LookupEventArgType();
                }

                return _eventArgType;
            }
        }

        public bool IsTemplate
        {
            get
            {
                if (!_isTemplate.HasValue)
                {
                    _isTemplate = LookupIsTemplate();
                }
                return _isTemplate.Value;
            }
        }

        public bool IsIndexer
        {
            get
            {
                if (!_isIndexer.HasValue)
                {
                    _isIndexer = LookupIsIndexer();
                }
                return _isIndexer.Value;
            }
        }

        public bool HasPublicGetter
        {
            get
            {
                if (!_hasPublicGetter.HasValue)
                {
                    _hasPublicGetter = LookupHasPublicGetter();
                }
                return _hasPublicGetter.Value;
            }
        }

        public bool HasPublicSetter
        {
            get
            {
                if (!_hasPublicSetter.HasValue)
                {
                    _hasPublicSetter = LookupHasPublicSetter();
                }
                return _hasPublicSetter.Value;
            }
        }

        //  ---- protected virtual (new lookup methods) to support the above additional properties -----

        protected virtual bool LookupIsTemplate()
        {
            var duiType = DeclaringType as DirectUIXamlType;
            return duiType != null && duiType.IsTemplateType && duiType.ContentProperty == this;
        }

        protected virtual bool LookupIsIndexer()
        {
            PropertyInfo pi = UnderlyingMember as PropertyInfo;
            // if property is Attachable the Underlying Member will be
            // the Static Set() method and the pi will be null
            if (pi != null)
            {
                ParameterInfo[] indexParams = pi.GetIndexParameters();
                return indexParams.Length > 0;
            }
            return false;
        }

        protected virtual bool LookupIsDependencyProperty()
        {
            var duiSchema = DeclaringType.SchemaContext as DirectUISchemaContext;
            var declaringType = DeclaringType.UnderlyingType;

            return declaringType.IsDependencyProperty(Name);
        }

        private CustomAttributeData GetAttribute(string attrName)
        {
            CustomAttributeData attr = null;
            if (!IsAttachable)
            {
                PropertyInfo pi = UnderlyingMember as PropertyInfo;
                if (pi != null)
                {
                    // look on the Set/Put method before the property itself.
                    MethodInfo smi = pi.GetSetMethod();
                    if (smi != null)
                    {
                        attr = ReflectionHelper.FindAttributeByTypeName(smi, attrName);
                    }
                    if (attr == null)
                    {
                        attr = ReflectionHelper.FindAttributeByTypeName(pi, attrName);
                    }
                }
            }
            else    // Attachable properties
            {
                if (_attachablePropertySetter != null)
                {
                    attr = ReflectionHelper.FindAttributeByTypeName(_attachablePropertySetter, attrName);
                }
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

                _deprecatedMessage = ReflectionHelper.GetAttributeConstructorArgument(attr, 0, null) as String;
                // in case there is some problem reading the message, insert a simple replacement.
                if (String.IsNullOrWhiteSpace(_deprecatedMessage))
                {
                    _deprecatedMessage = defaultMessage;
                }

                //Windows.Foundation.Metadata.DeprecatedAttribute has an additional second argument which specifies whether
                //this is hard deprecated.
                if (attrName.Equals(KnownTypes.DeprecatedAttribute))
                {
                    int level = (int)ReflectionHelper.GetAttributeConstructorArgument(attr, 1, null);
                    if (level != 0)
                    {
                        _isHardDeprecated = true;
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

        protected virtual XamlType LookupEventArgType()
        {
            XamlType eventArgType = null;
            if (IsEvent)
            {
                Type d = this.Type.UnderlyingType;
                if (d.BaseType == typeof(MulticastDelegate))
                {
                    MethodInfo handler = d.GetMethod(KnownMembers.Invoke);
                    if (handler != null)
                    {
                        ParameterInfo[] handlerParameters = handler.GetParameters();
                        if (handlerParameters != null && handlerParameters.Length == 2)
                        {
                            Type type = handlerParameters[1].ParameterType;
                            eventArgType = DeclaringType.SchemaContext.GetXamlType(type);
                        }
                    }
                }
            }
            return eventArgType;
        }

        protected virtual bool LookupHasPublicGetter()
        {
            PropertyInfo pi = UnderlyingMember as PropertyInfo;
            if (pi == null)
            {
                return base.IsReadPublic;
            }
            return (pi.GetGetMethod() != null);
        }

        protected virtual bool LookupHasPublicSetter()
        {
            PropertyInfo pi = UnderlyingMember as PropertyInfo;
            if (pi == null)
            {
                return base.IsWritePublic;
            }
            return (pi.GetSetMethod() != null);
        }

        // --- Overrides of base class Lookup* methods.
        protected override XamlType LookupType()
        {
            XamlType xamlType = base.LookupType();
            var duiXamlType = xamlType as DirectUIXamlType;
            if (duiXamlType != null)
            {
                return xamlType;
            }

            var schema = DeclaringType.SchemaContext as DirectUISchemaContext;
            Debug.Assert(schema != null);
            xamlType = schema.GetXamlType(xamlType.UnderlyingType);
            return xamlType;
        }
    }
}
