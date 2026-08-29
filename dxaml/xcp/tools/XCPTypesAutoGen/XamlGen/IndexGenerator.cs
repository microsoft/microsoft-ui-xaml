// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace XamlGen
{
    /// <summary>
    /// Assigns default indexes to types that participate in the type table.
    /// </summary>
    class IndexGenerator
    {
        internal const string KnownNamespaceIndexEnumName = "KnownNamespaceIndex";
        internal const string KnownTypeIndexEnumName = "KnownTypeIndex";
        internal const string KnownPropertyIndexEnumName = "KnownPropertyIndex";
        internal const string KnownEventIndexEnumName = "KnownEventIndex";
        internal const string KnownMethodIndexEnumName = "KnownMethodIndex";

        private OMContext _context;

        private IndexGenerator(OMContext context)
        {
            _context = context;
        }

        internal static void GenerateIndexes(OMContext context)
        {
            new IndexGenerator(context).Run();
        }

        private void AssignIndices(IEnumerable<DependencyPropertyDefinition> properties, ref int currentIndex)
        {
            foreach (DependencyPropertyDefinition property in properties)
            {
                if (property.PropertyType.Type.IsExcludedFromTypeTable)
                {
                    throw new InvalidOperationException(string.Format(
                        "Property '{0}.{1}' has a property type '{2}' which does not exist in the type table.",
                        property.DeclaringType.GenericClrName,
                        property.Name,
                        property.PropertyType.Type.GenericClrFullName));
                }

                property.IndexName = KnownPropertyIndexEnumName + "::" + GetDefaultIndexName(property);
                property.IndexNameWithoutPrefix = GetDefaultIndexName(property);
                property.TypeTableIndex = currentIndex++;
            }
        }

        private void Run()
        {
            // Start at 1, so we can use 0 as a nullptr.
            int nextNamespaceIndex = 1;
            int nextTypeIndex = 1;
            int nextPropertyIndex = 1;
            int nextObjectPropertyIndex = 1;
            int nextRenderPropertyIndex = 1;
            int nextEnterPropertyIndex = 1;

            Func<NamespaceDefinition, bool> includeNamespaceInTypeTable = ns => !ns.IsExcludedFromTypeTable;
            Func<TypeDefinition, bool> excludePhoneTypes = (type) => type.IdlTypeInfo.Group == null || !type.IdlTypeInfo.Group.Equals("Phone", StringComparison.OrdinalIgnoreCase);
            Func<TypeDefinition, bool> includeTypeInTypeTable = type => !type.IsExcludedFromTypeTable && excludePhoneTypes(type);
            OMContextView view = _context.GetView(includeNamespaceInTypeTable, includeTypeInTypeTable);

            // Null entries.
            NamespaceDefinition.UnknownNamespace.IndexName = KnownNamespaceIndexEnumName + "::UnknownNamespace";
            NamespaceDefinition.UnknownNamespace.IndexNameWithoutPrefix = "UnknownNamespace";
            ClassDefinition.UnknownType.IndexName = KnownTypeIndexEnumName + "::UnknownType";
            ClassDefinition.UnknownType.IndexNameWithoutPrefix = "UnknownType";
            PropertyDefinition.UnknownProperty.IndexName = KnownPropertyIndexEnumName + "::UnknownType_UnknownProperty";
            PropertyDefinition.UnknownProperty.IndexNameWithoutPrefix = "UnknownType_UnknownProperty";
            EventDefinition.UnknownEvent.IndexName = KnownEventIndexEnumName + "::UnknownType_UnknownEvent";
            EventDefinition.UnknownEvent.IndexNameWithoutPrefix = "UnknownType_UnknownEvent";
            MethodDefinition.UnknownMethod.IndexName = KnownMethodIndexEnumName + "::UnknownType_UnknownMethod";
            MethodDefinition.UnknownMethod.IndexNameWithoutPrefix = "UnknownType_UnknownMethod";

            // Walk all the namespaces.
            foreach (NamespaceDefinition ns in _context.Namespaces.Where(includeNamespaceInTypeTable).OrderBy(ns => ns.Name.Length))
            {
                ns.TypeTableIndex = nextNamespaceIndex++;
                ns.IndexName = KnownNamespaceIndexEnumName + "::" + GetDefaultIndexName(ns);
                ns.IndexNameWithoutPrefix = GetDefaultIndexName(ns);
            }

            // Walk all the classes.
            foreach (ClassDefinition type in view.GetAllClasses().OrderBy(t => t.TypeTableName).OrderByHierarchy())
            {
                type.IndexName = KnownTypeIndexEnumName + "::" + GetDefaultIndexName(type);
                type.IndexNameWithoutPrefix = GetDefaultIndexName(type);
                type.TypeTableIndex = nextTypeIndex++;

                AssignIndices(
                    type.TypeTableProperties.Where((p) => !p.IsSimpleProperty).OrderBy(p => p.Name),
                    ref nextPropertyIndex);

                // Assign indexes to the object properties.
                foreach (DependencyPropertyDefinition property in type.EffectiveTypeTableProperties.Where(p => p.IsObjectProperty).OrderBy(p => p.Name))
                {
                    property.TypeTableObjectPropertyIndex = nextObjectPropertyIndex++;
                }

                // Assign indexes to the render properties.
                foreach (DependencyPropertyDefinition property in type.EffectiveTypeTableProperties.Where(p => p.IsRenderProperty).OrderBy(p => p.Name))
                {
                    property.TypeTableRenderPropertyIndex = nextRenderPropertyIndex++;
                }

                // Assign indexes to the Enter/Leave properties.
                foreach (DependencyPropertyDefinition property in type.EffectiveTypeTableProperties.Where(p => p.IsEnterProperty).OrderBy(p => p.Name))
                {
                    property.TypeTableEnterPropertyIndex = nextEnterPropertyIndex++;
                }

                // Assign indexes to forwarding DPs (currently only FrameworkElement.NameProperty).
                foreach (DependencyPropertyDefinition property in type.DependencyProperties.Where(dp => dp.UnderlyingDependencyProperty != null))
                {
                    property.IndexName = property.UnderlyingDependencyProperty.IndexName;
                    property.IndexNameWithoutPrefix = property.UnderlyingDependencyProperty.IndexNameWithoutPrefix;
                }
            }

            // Process simple properties after, so indices are continuous and at the end.
            foreach (ClassDefinition type in view.GetAllClasses().OrderBy(t => t.TypeTableName).OrderByHierarchy())
            {
                AssignIndices(
                    type.SimpleProperties.OrderBy(p => p.Name),
                    ref nextPropertyIndex);

                // Assign slots after all property indices are set.

                int nextSlot = 0;

                foreach (DependencyPropertyDefinition property in type.GetSlottedProperties())
                {
                    if (property.Slot == 0)
                    {
                        property.Slot = nextSlot;
                    }
                    else
                    {
                        // Properties are expected to share slot numbers across all types.
                        if (property.Slot != nextSlot)
                        {
                            throw new InvalidOperationException("Property has different slot numbers for different types.");
                        }
                    }

                    nextSlot++;
                }
            }
            // Walk all the enums.
            foreach (EnumDefinition type in view.GetAllEnums().OrderBy(t => t.TypeTableName))
            {
                type.IndexName = KnownTypeIndexEnumName + "::" + GetDefaultIndexName(type);
                type.IndexNameWithoutPrefix = GetDefaultIndexName(type);
                type.TypeTableIndex = nextTypeIndex++;
            }

            // Walk all the events. We directly go to the context now because we don't want to filter by type table types.
            foreach (EventDefinition ev in _context.GetView().GetAllEvents())
            {
                ev.IndexName = KnownEventIndexEnumName + "::" + GetDefaultIndexName(ev);
                ev.IndexNameWithoutPrefix = GetDefaultIndexName(ev);
            }

            // Walk all the methods.
            foreach (MethodDefinition m in _context.GetView().GetAllMethods())
            {
                m.IndexName = KnownMethodIndexEnumName + "::" + GetDefaultIndexName(m);
                m.IndexNameWithoutPrefix = GetDefaultIndexName(m);
            }
        }

        private static string GetDefaultIndexName(NamespaceDefinition ns)
        {
            return ns.Name.Replace('.', '_');
        }

        private static string GetDefaultIndexName(TypeDefinition type)
        {
            if (type.IsGenericType)
            {
                StringBuilder builder = new StringBuilder(type.Name);
                builder.Append("Of");
                foreach (TypeReference argType in type.GenericArguments)
                {
                    builder.Append(GetDefaultIndexName(argType.Type));
                }
                return builder.ToString();
            }
            return type.Name;
        }

        private static string GetDefaultIndexName(PropertyDefinition member)
        {
            return GetDefaultIndexName(member.DeclaringType) + "_" + member.Name;
        }

        private static string GetDefaultIndexName(EventDefinition member)
        {
            return GetDefaultIndexName(member.DeclaringType) + "_" + member.Name;
        }

        private static string GetDefaultIndexName(MethodDefinition member)
        {
            return GetDefaultIndexName(member.DeclaringType) + "_" + member.Name;
        }
    }
}
