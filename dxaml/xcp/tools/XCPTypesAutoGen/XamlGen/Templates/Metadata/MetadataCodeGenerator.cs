// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace XamlGen.Templates.Metadata
{
    public struct SearchRange
    {
        public int TypeNameLength;
        public int StartIndex;
        public int EndIndex;
    }

    /// <summary>
    /// Base class for all metadata code generators.
    /// </summary>
    public abstract class MetadataCodeGenerator<TModel> : XamlCodeGenerator<TModel>
    {
        const string MetaDataTypeInfoFlags = "MetaDataTypeInfoFlags";
        const string MetaDataTypeInfoFlagsPrefix = MetaDataTypeInfoFlags + "::";
        const string MetaDataPropertyInfoFlags = "MetaDataPropertyInfoFlags";
        const string MetaDataPropertyInfoFlagsPrefix = MetaDataPropertyInfoFlags + "::";
        const string MetaDataEnterPropertyInfoFlags = "MetaDataEnterPropertyInfoFlags";
        const string MetaDataEnterPropertyInfoFlagsPrefix = MetaDataEnterPropertyInfoFlags + "::";

        protected string AsMethodPointer(string callback)
        {
            if (!string.IsNullOrEmpty(callback))
            {
                return callback;
            }
            return "nullptr";
        }

        protected string IncludeActivationEntry(TypeDefinition type)
        {
            if (type is ClassDefinition)
            {
                return IncludeTemplate<ClassActivationEntry>(type);
            }
            else if (type is EnumDefinition)
            {
                return IncludeTemplate<EnumActivationEntry>(type);
            }
            else
            {
                // We only support class and enum entries.
                throw new InvalidOperationException();
            }
        }

        protected string IncludeTypeEntry(TypeDefinition type)
        {
            if (type is ClassDefinition)
            {
                return IncludeTemplate<ClassTypeEntry>(type);
            }
            else if (type is EnumDefinition)
            {
                return IncludeTemplate<EnumTypeEntry>(type);
            }
            else
            {
                // We only support class and enum entries.
                throw new InvalidOperationException();
            }
        }

        protected string IncludeTypeNameInfoEntry(TypeDefinition type)
        {
            if (type is ClassDefinition)
            {
                return IncludeTemplate<ClassNameInfoEntry>(type);
            }
            else if (type is EnumDefinition)
            {
                return IncludeTemplate<EnumNameInfoEntry>(type);
            }
            else
            {
                // We only support class and enum entries.
                throw new InvalidOperationException();
            }
        }

        protected string IncludeTypePropertiesEntry(TypeDefinition type)
        {
            if (type is ClassDefinition)
            {
                return IncludeTemplate<ClassPropertiesEntry>(type);
            }
            else if (type is EnumDefinition)
            {
                return IncludeTemplate<EnumPropertiesEntry>(type);
            }
            else
            {
                // We only support class and enum entries.
                throw new InvalidOperationException();
            }
        }

        protected string IncludeTypeCheckEntry(TypeDefinition type)
        {
            if (type is ClassDefinition)
            {
                return IncludeTemplate<ClassTypeCheckEntry>(type);
            }
            else if (type is EnumDefinition)
            {
                return IncludeTemplate<EnumTypeCheckEntry>(type);
            }
            else
            {
                // We only support class and enum entries.
                throw new InvalidOperationException();
            }
        }

        protected IEnumerable<SearchRange> GetSearchRanges(IEnumerable<TypeDefinition> types)
        {
            TypeDefinition[] indexedTypes = types.OrderBy(t => t.TypeTableName).OrderBy(t => t.TypeTableName.Length).ToArray();

            for (int i = 0; i < indexedTypes.Length; i++)
            {
                TypeDefinition currentType = indexedTypes[i];

                int startIndex = i + 1 /* UnknownType */;
                int numberOfTypesWithThisLength = 0;
                for (; i < indexedTypes.Length && indexedTypes[i].TypeTableName.Length == currentType.TypeTableName.Length; i++)
                {
                    numberOfTypesWithThisLength++;
                }
                i--;

                yield return new SearchRange()
                {
                    StartIndex = startIndex,
                    EndIndex = startIndex + numberOfTypesWithThisLength,
                    TypeNameLength = currentType.TypeTableName.Length,
                };
            }
        }

        protected string GetTargetTypeString(DependencyPropertyDefinition property)
        {
            AttachedPropertyDefinition attachedProperty = property as AttachedPropertyDefinition;
            if (attachedProperty != null)
            {
                return attachedProperty.TargetType.Type.IndexName;
            }
            return property.DeclaringType.IndexName;
        }

        protected string GetDefaultInterfaceGuidString(TypeDefinition type)
        {
            if (type is ClassDefinition)
            {
                return GetDefaultInterfaceGuidString((ClassDefinition)type);
            }
            else if (type is EnumDefinition)
            {
                return GetDefaultInterfaceGuidString((EnumDefinition)type);
            }
            else
            {
                throw new InvalidOperationException("Unsupported type.");
            }
        }

        protected string GetDefaultInterfaceGuidString(EnumDefinition type)
        {
            // For now, don't include internal types. We probably never return boxes of internal types.
            // Also, exclude [PrivateIdl] enums for now, because they're still  using old code-gen and 
            // don't have GUIDs associated with them.
            if (!type.ExcludeGuidFromTypeTable && !type.IdlEnumInfo.IsExcluded && !type.IdlEnumInfo.IsPrivateIdlOnly)
            {
                return string.Format("__uuidof({0}<{1}>)", PrefixAbi("Windows::Foundation::IReference"), AsCppType(type.AbiReferenceFullName));
            }
            return "GUID{} /*GUID_NULL*/";
        }

        protected string GetDefaultInterfaceGuidString(ClassDefinition type)
        {
            if (!type.ExcludeGuidFromTypeTable)
            {
                if (type.IsValueType)
                {
                    return string.Format("__uuidof({0}<{1}>)", PrefixAbi("Windows::Foundation::IReference"), AsCppType(type.IdlClassInfo.AbiReferenceFullName));
                }
                else if (type.IdlClassInfo.HasRuntimeClass || type.IsObjectType || type.IsInterface)
                {
                    return string.Format("__uuidof({0})", AsCppType(type.IdlClassInfo.AbiReferenceFullName));
                }
                if (type.IsCollectionImplementationClass && type.IdlClassInfo.AbiReferenceFullName.Contains("IObservableVector"))
                {
                    return string.Format("__uuidof({0})", AsCppType(type.IdlClassInfo.AbiReferenceFullName));
                }
            }

            return "GUID{} /*GUID_NULL*/";
        }

        protected string GetOffsetString(DependencyPropertyDefinition property)
        {
            if (!string.IsNullOrEmpty(property.FieldName))
            {
                return string.Format("OFFSET({0}, {1})", property.FieldClassName, property.FieldName);
            }
            return "0";
        }

        protected string GetPropertyMethodString(DependencyPropertyDefinition property)
        {
            if (property.HasPropertyMethod)
            {
                return string.Format("&{0}", property.MethodName);
            }
            return "nullptr";
        }

        protected string GetStorageEnsureString(PropertyDefinition property)
        {
            if (!string.IsNullOrEmpty(property.StorageGroupEnsureMethodName))
            {
                return string.Format("&{0}::{1}", property.EffectiveTargetType.IdlInfo.Type.CoreName, property.StorageGroupEnsureMethodName);
            }
            return "nullptr";
        }

        protected string GetStorageGroupOffsetString(PropertyDefinition property)
        {
            if (!string.IsNullOrEmpty(property.StorageGroupEnsureMethodName))
            {
                return string.Format("OFFSET({0}, {1})", property.StorageGroupOffsetClassName, property.StorageGroupOffsetFieldName);
            }
            return "0";
        }

        protected string GetEnterPropertyFlagsString(DependencyPropertyDefinition property)
        {
            List<string> flags = new List<string>();

            if (property.XamlPropertyFlags.DoNotEnterOrLeaveValue) { flags.Add("DoNotEnterLeave"); }
            if (property.XamlPropertyFlags.NeedsInvoke) { flags.Add("NeedsInvoke"); }
            if (property.IsObjectProperty) { flags.Add("IsObjectProperty"); }

            if (flags.Count == 0)
            {
                return MetaDataEnterPropertyInfoFlagsPrefix + "None";
            }

            return string.Format("static_cast<{0}>({1})",
                MetaDataEnterPropertyInfoFlags,
                flags.Select(f => CastToInt(MetaDataEnterPropertyInfoFlagsPrefix + f)).Aggregate((left, right) => left + " | " + right));
        }

        protected string GetPropertyFlagsString(DependencyPropertyDefinition property)
        {
            List<string> flags = new List<string>();

            if (property.IsSparse) { flags.Add("IsSparse"); }
            if (property.IsSimpleProperty) { flags.Add("IsSimpleProperty"); }
            if (property.DeclaringClass.IsStrict || (property.Strictness != null && property.Strictness.Value == Strictness.StrictOnly)) { flags.Add("IsStrictOnlyProperty"); }
            if (property.Strictness != null && property.Strictness.Value == Strictness.NonStrictOnly) { flags.Add("IsNonStrictOnlyProperty"); }
            if (property.XamlPropertyFlags.AffectsMeasure) { flags.Add("AffectMeasure"); }
            if (property.XamlPropertyFlags.AffectsArrange) { flags.Add("AffectArrange"); }
            if (property is AttachedPropertyDefinition) { flags.Add("IsAttached"); }
            if (!property.IdlMemberInfo.IsExcluded) { flags.Add("IsPublic"); }
            if (property.IsReadOnly && !property.XamlPropertyFlags.IsReadOnlyExceptForParser) { flags.Add("IsReadOnlyProperty"); }
            if (property.IdlPropertyInfo.IsReadOnly && !property.XamlPropertyFlags.IsReadOnlyExceptForParser) { flags.Add("IsExternalReadOnlyProperty"); }
            if (property.XamlPropertyFlags.IsValueCreatedOnDemand) { flags.Add("IsOnDemandProperty"); }
            if (property.XamlPropertyFlags.IsValueInherited) { flags.Add("IsInheritedProperty"); }
            if (property.PropertyType.IsNullable) { flags.Add("IsNullable"); }
            if (property.HasPropertyMethod) { flags.Add("IsPropMethodCall"); }
            if (property.IsInStorageGroup) { flags.Add("IsStorageGroup"); }
            if (property.XamlPropertyFlags.RequiresMultipleAssociationCheck) { flags.Add("RequiresMultipleAssociationCheck"); }
            if (property.XamlPropertyFlags.NeedsInvoke) { flags.Add("NeedsInvoke"); }
            if (property.XamlPropertyFlags.HadFieldInBlue) { flags.Add("HadFieldInBlue"); }
            if (property.XamlPropertyFlags.StoreDoubleAsFloat) { flags.Add("StoreDoubleAsFloat"); }
            if (property.IsVisualTreeProperty) { flags.Add("IsVisualTreeProperty"); }
            
            if (flags.Count == 0)
            {
                return MetaDataPropertyInfoFlagsPrefix + "None";
            }

            return string.Format("static_cast<{0}>({1})",
                MetaDataPropertyInfoFlags,
                flags.Select(f => CastToInt(MetaDataPropertyInfoFlagsPrefix + f)).Aggregate((left, right) => left + " | " + right));
        }

        protected string GetTypeFlagsString(TypeDefinition type)
        {
            List<string> flags = new List<string>();

            if (!type.IdlTypeInfo.IsExcluded) { flags.Add("IsPublic"); }

            ClassDefinition typeAsClass = type as ClassDefinition;
            if (typeAsClass != null)
            {
                flags.Add("ExecutedClassConstructor");
                if (typeAsClass.IsActivatableInCore && typeAsClass.IsCreateableFromXAML) { flags.Add("IsConstructible"); }
                if (typeAsClass.IsInterface) { flags.Add("IsInterface"); }
                if (typeAsClass.IsCollectionImplementationClass || typeAsClass.XamlClassFlags.IsICollection) { flags.Add("IsCollection"); }
                if (typeAsClass.XamlClassFlags.IsIDictionary) { flags.Add("IsDictionary"); }
                if (typeAsClass.IsISupportInitialize) { flags.Add("IsISupportInitialize"); }
                if (typeAsClass.XamlClassFlags.IsMarkupExtension) { flags.Add("IsMarkupExtension"); }
                if (typeAsClass.XamlClassFlags.HasTypeConverter) { flags.Add("HasTypeConverter"); }
                if (typeAsClass.XamlClassFlags.IsWhitespaceSignificant) { flags.Add("IsWhitespaceSignificant"); }
                if (typeAsClass.XamlClassFlags.TrimSurroundingWhitespace) { flags.Add("TrimSurroundingWhitespace"); }
                if (typeAsClass.RequiresPeerActivation) { flags.Add("RequiresPeerActivation"); }
                if (typeAsClass.IsStrict) { flags.Add("IsStrict"); }
            }

            if (type.IsValueType)
            {
                // In IDL and our ABI we deal with HSTRING, which is a handle, which is a value type. The underlying 
                // string however is treated as a reference type, and that's the behavior our type table needs to describe.
                if (!type.IsStringType)
                {
                    flags.Add("IsValueType");
                }

                if (type is EnumDefinition)
                {
                    flags.Add("HasTypeConverter");
                    flags.Add("IsEnum");
                    if ((type as EnumDefinition).IsCompact())
                    {
                        flags.Add("IsCompactEnum");
                    }
                }

                if (type.IsPrimitive)
                {
                    flags.Add("IsPrimitive");
                }
            }

            if (flags.Count == 0)
            {
                return MetaDataTypeInfoFlagsPrefix + "None";
            }

            return string.Format("static_cast<{0}>({1})",
                MetaDataTypeInfoFlags,
                flags.Select(f => CastToInt(MetaDataTypeInfoFlagsPrefix + f)).Aggregate((left, right) => left + " | " + right));
        }

        private static string CastToInt(string value)
        {
            return "static_cast<UINT>(" + value + ")";
        }
    }
}
