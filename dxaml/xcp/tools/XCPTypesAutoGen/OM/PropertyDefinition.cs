// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace OM
{
    public class PropertyDefinition : MemberDefinition
    {
        public static readonly DependencyPropertyDefinition UnknownProperty;

        private Idl.IdlPropertyInfo _idlPropertyInfo;
        private Dictionary<ClassDefinition, int> _slotInfo = new Dictionary<ClassDefinition,int>();
        private TypeReference _getterReturnType = null;

        public OM.ValueType? NativeStorageType { get; set; }

        public string CoreFieldTypeName
        {
            get
            {
                // REVIEW: Can we just have the model use a TypeReference with Type=float and IdlType=double?
                if (XamlPropertyFlags.StoreDoubleAsFloat)
                {
                    return "FLOAT";
                }

                return PropertyType.CoreFullName;
            }
        }

        /// <summary>
        /// Gets or sets whether to generate code in the framework that delegates the call to the core.
        /// </summary>
        public bool DelegateToCore
        {
            get;
            set;
        }

        /// <summary>
        /// Returns PropertyDefinition.DeclaringType for normal properties, and AttachedPropertyDefinition.TargetType for attached properties.
        /// </summary>
        public virtual TypeReference EffectiveTargetType
        {
            get
            {
                return new TypeReference(DeclaringType);
            }
        }

        /// <summary>
        /// Gets the name of the class that declares the backing field for this property.
        /// </summary>
        public virtual string FieldClassName
        {
            get
            {
                return DeclaringType.CoreName;
            }
        }

        /// <summary>
        /// Gets or sets the name of the underlying field for this property.
        /// This property is optional.
        /// </summary>
        public string FieldName
        {
            get;
            set;
        }

        public string FrameworkFieldTypeName
        {
            get
            {
                if (PropertyType.IdlInfo.Type.IsStringType)
                {
                    return "Microsoft::WRL::Wrappers::HString";
                }
                else if (PropertyType.Type.IsTypeNameType)
                {
                    return "TypeNamePtr";
                }
                else if (PropertyType.IsValueType)
                {
                    return PropertyType.AbiFullName;
                }
                else if (XamlPropertyFlags.UseComPtr)
                {
                    return string.Format("ctl::ComPtr<{0}>", PropertyType.AbiFullName);
                }
                else
                {
                    return string.Format("TrackerPtr<{0}>", PropertyType.AbiFullName);
                }
            }
        }
        public string PhoneImplTypeName
        {
            get
            {
                if (PropertyType.IdlInfo.Type.IsStringType)
                {
                    return "Microsoft::WRL::Wrappers::HString";
                }
                else if (PropertyType.IdlInfo.Type.IsValueType)
                {
                    return PropertyType.AbiFullName;
                }
                else
                {
                    return string.Format("wrl::ComPtr<{0}>", PropertyType.AbiFullName);
                }
            }
        }

        /// <summary>
        /// Gets or sets the name of the underlying field for this property in the framework.
        /// </summary>
        public string FrameworkFieldName
        {
            get;
            set;
        }

        public virtual string GetterName
        {
            get
            {
                return "get_" + IdlPropertyInfo.Name;
            }
        }

        public virtual TypeReference GetterReturnType
        {
            get
            {
                if (_getterReturnType == null)
                {
                    _getterReturnType = GetGetMethod().ReturnType;
                }
                return _getterReturnType;
            }
        }

        public bool HasBackingFieldInFramework
        {
            get
            {
                return GenerateDefaultBody && !string.IsNullOrEmpty(FrameworkFieldName) && !DelegateToCore;
            }
        }

        public bool HasCallbackRetrievedValue
        {
            get;
            set;
        }

        public bool HasPropertyChangeCallback
        {
            get;
            set;
        }

        public Idl.IdlPropertyInfo IdlPropertyInfo
        {
            get
            {
                if (_idlPropertyInfo == null)
                {
                    _idlPropertyInfo = CreateIdlPropertyInfo();
                }
                return _idlPropertyInfo;
            }
        }

        public override Idl.IdlMemberInfo IdlMemberInfo
        {
            get
            {
                return IdlPropertyInfo;
            }
        }

        public string ImplGetterName
        {
            get
            {
                return "get_" + ImplName;
            }
        }

        public string ImplSetterName
        {
            get
            {
                return "put_" + ImplName;
            }
        }

        public string IndexName
        {
            get;
            set;
        }

        public string IndexNameWithCastToInt
        {
            get
            {
                return "static_cast<int>(" + IndexName + ")";
            }
        }

        public string IndexNameWithoutPrefix
        {
            get;
            set;
        }

        public bool IsContentProperty
        {
            get;
            set;
        }

        public bool IsCustomValueTypedProperty
        {
            get
            {
                return PropertyType.Type.Name == "MapZoomLevelRange";
            }
        }

        public bool IsImplicitContentProperty
        {
            get;
            set;
        }

        public bool IsInStorageGroup
        {
            get
            {
                return !string.IsNullOrEmpty(StorageGroupEnsureMethodName);
            }
        }

        public bool IsReadOnly
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets whether this property is a helper property on a struct.
        /// </summary>
        public bool IsStructHelper
        {
            get;
            set;
        }

        public string MethodName
        {
            get;
            set;
        }

        public TypeReference PropertyType
        {
            get;
            set;
        }

        public virtual string SetterName
        {
            get
            {
                return "put_" + IdlPropertyInfo.Name;
            }
        }

        public Modifier SetterModifier
        {
            get;
            set;
        }

        /// <summary>
        /// Example: EnsureTextFormatting
        /// </summary>
        public string StorageGroupEnsureMethodName
        {
            get;
            set;
        }

        /// <summary>
        /// Example: TextFormatting
        /// </summary>
        public string StorageGroupOffsetClassName
        {
            get;
            set;
        }

        /// <summary>
        /// Example: m_nFontWeight
        /// </summary>
        public string StorageGroupOffsetFieldName
        {
            get;
            set;
        }

        public XamlPropertyFlags XamlPropertyFlags
        {
            get;
            set;
        }

        static PropertyDefinition()
        {
            UnknownProperty = new DependencyPropertyDefinition()
            {
                Name = "UnknownProperty",
                IsVisualTreeProperty = true,
                DeclaringType = ClassDefinition.UnknownType,
                PropertyType = new TypeReference(ClassDefinition.UnknownType),
                TypeTableIndex = 0,
                TypeTableObjectPropertyIndex = 0
            };
            UnknownProperty.IdlMemberInfo.IsExcluded = true;
            UnknownProperty.PropertyType.IdlInfo.Type = ClassDefinition.UnknownType;
        }

        public PropertyDefinition()
        {
            XamlPropertyFlags = new XamlPropertyFlags();
            HasPropertyChangeCallback = true;
        }

        protected virtual Idl.IdlPropertyInfo CreateIdlPropertyInfo()
        {
            return new Idl.IdlPropertyInfo(this);
        }

        public MethodDefinition GetGetMethod()
        {
            MethodDefinition getMethod = new MethodDefinition();
            Helper.ShallowCopyProperties<MemberDefinition>(this, getMethod);
            Helper.ShallowCopyProperties(IdlMemberInfo, getMethod.IdlMemberInfo);
            getMethod.Name = IdlMemberInfo.Name;
            getMethod.DelegateToCore = DelegateToCore;
            getMethod.IdlMethodInfo.Name = GetterName;
            getMethod.IsImplVirtual = XamlPropertyFlags.IsGetterImplVirtual;
            getMethod.ReturnType = PropertyType;
            getMethod.EmitStrictCheck = false;
            getMethod.Version = GetterVersion;
            return getMethod;
        }

        public MethodDefinition GetSetMethod()
        {
            if (IsReadOnly)
            {
                throw new InvalidOperationException("Property is read-only.");
            }
            MethodDefinition setMethod = new MethodDefinition();
            Helper.ShallowCopyProperties<MemberDefinition>(this, setMethod);
            Helper.ShallowCopyProperties(IdlMemberInfo, setMethod.IdlMemberInfo);
            setMethod.Name = IdlMemberInfo.Name;
            setMethod.DelegateToCore = DelegateToCore;
            setMethod.IdlMethodInfo.Name = SetterName;
            setMethod.IsImplVirtual = XamlPropertyFlags.IsSetterImplVirtual;
            setMethod.IdlMethodInfo.IsExcluded |= IdlPropertyInfo.IsReadOnly;
            setMethod.Version = SetterVersion;
            setMethod.Parameters.Add(new ParameterDefinition()
            {
                ParameterType = PropertyType
            });
            return setMethod;
        }

        public override string ToString()
        {
            return $"{GetterReturnType} {Name}";
        }
    }
}
