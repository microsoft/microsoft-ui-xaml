// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System.Collections.Generic;
using System.Linq;
using System.Text.RegularExpressions;

namespace OM
{
    public abstract class TypeDefinition
    {
        private HashSet<TypeDefinition> _dependencies = new HashSet<TypeDefinition>();
        private List<DeprecationDefinition> _deprecations = new List<DeprecationDefinition>();
        private List<TypeReference> _genericArguments = new List<TypeReference>();
        private List<ContractReference> _supportedContracts = new List<ContractReference>();

        /// <summary>
        /// Gets or sets the name to use when this type is referenced in an ABI generic type (e.g. IReference&lt;T&gt;).
        /// Example: BOOLEAN needs to be represented as 'bool'.
        /// </summary>
        public string AbiGenericArgumentName
        {
            get;
            set;
        }

        /// <summary>
        /// Some of our code gen types have incorrect names (i.e. Char16 instead of Char and Byte instead of UInt8). For those, this will be set.
        /// </summary>
        public string CorrectedTypeName
        {
            get;
            set;
        }

        /// <summary>
        /// Gets the runtime class full name. If this type does not exist in IDL, it will return the ABI implementation name.
        /// </summary>
        public string AbiReferenceFullName
        {
            get
            {
                return Helper.GetAbiReferenceFullName(this);
            }
        }

        public string AbiReferenceFullNameNoABIPrefix => Regex.Replace(Helper.GetAbiReferenceFullName(this), @"\bABI(.|::)", "");

        public bool CanLookupByName
        {
            get
            {
                return !IsGenericType && !string.IsNullOrEmpty(TypeTableName);
            }
        }

        public string Comment
        {
            get;
            set;
        }

        public ModuleDefinition Module
        {
            get;
            set;
        }

        public string CoreName
        {
            get;
            set;
        }

        public NamespaceDefinition DeclaringNamespace
        {
            get;
            set;
        }

        /// <summary>
        /// Gets a list of immediate dependencies that this class has through its members. For example, a class 
        /// UIElement with a method "Automationpeer OnCreateAutomationPeer()" will have an immediate dependency 
        /// on AutomationPeer.
        /// </summary>
        public HashSet<TypeDefinition> Dependencies
        {
            get
            {
                return _dependencies;
            }
        }

        public List<DeprecationDefinition> Deprecations
        {
            get
            {
                return _deprecations;
            }
        }

        public bool ExcludeGuidFromTypeTable
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets whether to include this type in the type table, even if it appears this type doesn't need to be in the 
        /// type table because nothing references it.
        /// </summary>
        public bool ForceIncludeInTypeTable
        {
            get;
            set;
        }

        public string FullName
        {
            get
            {
                if (IsPrimitive || DeclaringNamespace.IsUnknown)
                {
                    return Name;
                }

                return DeclaringNamespace.Name + "." + Name;
            }
        }

        public bool GenerateStableIndex { get; set; } = true;

        public List<TypeReference> GenericArguments
        {
            get
            {
                return _genericArguments;
            }
        }

        /// <summary>
        /// Gets a CLR-formatted generic full name, such as Windows.Foundation.Collections.IObservableVector`[string].
        /// </summary>
        public string GenericClrFullName
        {
            get
            {
                if (string.IsNullOrEmpty(GenericClrName) || DeclaringNamespace.IsUnknown)
                {
                    return GenericClrName;
                }

                return DeclaringNamespace.Name + "." + GenericClrName;
            }
        }

        public string FriendlyGenericFullName
        {
            get;
            set;
        }


        /// <summary>
        /// Gets a CLR-formatted generic name, such as IObservableVector`[UIElement].
        /// </summary>
        public string GenericClrName
        {
            get;
            set;
        }
        /// <summary>
        /// Gets a friendly formatted generic name, such as IObservableVector<UIElement>.
        /// </summary>
        public string FriendlyGenericName
        {
            get;
            set;
        }


        public Guids Guids
        {
            get;
            set;
        }

        public abstract Idl.IdlTypeInfo IdlTypeInfo
        {
            get;
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

        /// <summary>
        /// Gets or sets whether this is a DependencyObject class, such as 
        /// Windows.UI.Xaml.DependencyObject and Windows.UI.Xaml.Controls.Button.
        /// TODO: This property exists temporarily to let us incrementally generate more 
        /// code as we move to a new code-gen tool. Eventually we should remove this property.
        /// </summary>
        public bool IsADependencyObject
        {
            get;
            set;
        }

        public bool IsAUIElement
        {
            get;
            set;
        }

        public bool IsConstexprConstructible
        {
            get;
            set;
        }

        public bool IsExcludedFromTypeTable
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets whether properties of this type should be excluded from things like the render type 
        /// table and the enter type table.
        /// </summary>
        public bool IsExcludedFromVisualTreeNodeConsideration
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets whether properties of this type should be excluded from the reference tracker walk.
        /// </summary>
        public bool IsExcludedFromReferenceTrackerWalk
        {
            get;
            set;
        }

        public bool IsGenericType
        {
            get;
            set;
        }

        public bool IsGenericTypeDefinition
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets whether this type is imported from a different library.
        /// Example: Windows.Foundation.Uri.
        /// </summary>
        public bool IsImported
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets whether this is the Windows.Foundation.Object class.
        /// </summary>
        public bool IsObjectType
        {
            get;
            set;
        }

        /// <summary>
        /// Examples: Windows.UI.Xaml.Controls.Canvas, Windows.UI.Xaml.DataTemplate, Boolean, Double
        /// Non system types: Extension (maps) types like Windows.UI.Xaml.Controls.Maps.MapStyle or,
        /// Extension (phone) types like Windows.UI.Xaml.Controls.PickerFlyoutPresenter
        /// </summary>
        public bool IsPhoneSystemType
        {
            get;
            set;
        }

        public bool IsPrimitive
        {
            get;
            set;
        }

        public bool IsSimpleType
        {
            get
            {
                return SimpleTypeKind.HasValue;
            }
        }

        /// <summary>
        /// Gets or sets whether this is the Windows.Foundation.String class.
        /// </summary>
        public bool IsStringType
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets whether this is the Windows.UI.Xaml.Interop.TypeName class.
        /// </summary>
        public bool IsTypeNameType
        {
            get;
            set;
        }

        public bool IsValueType
        {
            get;
            set;
        }

        public bool IsWebHostHidden
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets whether this type needs to be added to the type table just to support legacy XBFs.
        /// </summary>
        public bool IsXbfType
        {
            get;
            set;
        }

        public Modifier Modifier
        {
            get;
            set;
        }

        public string Name
        {
            get;
            set;
        }

        public int PhoneMemberTableIndex
        {
            get;
            set;
        }

        public int PhoneTypeTableIndex
        {
            get;
            set;
        }

        public bool IsSystemType
        {
            get;
            set;
        }

        public string MetadataName => GetMetadataName();
        public string MetadataFullName =>
            IsPrimitive ?
            MetadataName :
            string.Join(".", DeclaringNamespace.Name, MetadataName);


        /// <summary>
        /// The primitive name to use in the core.
        /// Example: For the type String, we want to use "xstring_ptr" in the core.
        /// </summary>
        public string PrimitiveCoreName
        {
            get;
            set;
        }

        /// <summary>
        /// The primitive name to use in C++ APIs.
        /// Example: For the type Boolean, we want to use "BOOLEAN" in our C++ APIs.
        /// </summary>
        public string PrimitiveCppName
        {
            get;
            set;
        }

        public string SimpleDefaultValue
        {
            get;
            set;
        }

        public SimpleTypeKind? SimpleTypeKind
        {
            get;
            set;
        }

        public string SimpleTypeName
        {
            get
            {
                if (!string.IsNullOrEmpty(PrimitiveCppName))
                {
                    return PrimitiveCppName;
                }
                else
                {
                    return TypeTableFullName;
                }
            }
        }

        public List<ContractReference> SupportedContracts
        {
            get
            {
                return _supportedContracts;
            }
        }

        public bool SupportsV2CodeGen
        {
            get
            {
                return SupportedContracts.SupportsV2CodeGen();
            }
        }

        public bool DefinedBeforeModernIdl
        {
            get
            {
                return !SupportedContracts.SupportsModernIdl();
            }
        }

        public int TypeTableIndex
        {
            get;
            set;
        }

        public string TypeTableFullName
        {
            get
            {
                if (DeclaringNamespace.IsUnknown || string.IsNullOrEmpty(TypeTableName))
                {
                    return TypeTableName;
                }
                else if (IsPrimitive)
                {
                    return DeclaringNamespace.TypeTableName + "." + TypeTableName;
                }
                else
                {
                    return IdlTypeInfo.FullName;
                }
            }
        }

        public string TypeTableName
        {
            get;
            set;
        }

        public XamlTypeFlags XamlTypeFlags
        {
            get;
            set;
        }

        public TypeDefinition()
        {
            IsPhoneSystemType = false;
            IsWebHostHidden = true;
            PhoneMemberTableIndex = -1;
            PhoneTypeTableIndex = -1;
            TypeTableIndex = -1;
            XamlTypeFlags = new XamlTypeFlags();
        }

        public override string ToString()
        {
            return base.ToString() + " - " + GenericClrFullName;
        }

        private string GetMetadataName()
        {
            var metadataName = Name;

            if (IsGenericType)
            {
                metadataName += $"`{GenericArguments.Count}<{string.Join(",", GenericArguments.Select(t => t.Type.MetadataFullName))}>";
            }

            return metadataName;
        }
    }
}
