// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM.Visitors;
using System;
using System.Collections.Generic;
using System.Linq;

namespace OM
{
    public class OMContextView
    {
        private OMContext _context;
        private List<string> _customImports = new List<string>();
        private string _idlGroupName;
        private IEnumerable<NamespaceDefinition> _namespaces;

        public string IdlGroupName
        {
            get
            {
                return _idlGroupName;
            }
        }

        public static HashSet<TypeDefinition> IdlForwardDeclaredTypes { get; private set; } = new HashSet<TypeDefinition>();

        public IEnumerable<NamespaceDefinition> Namespaces
        {
            get
            {
                return _namespaces;
            }
        }

        internal OMContextView(OMContext context, string idlGroupName = null)
        {
            _context = context;
            _namespaces = context.Namespaces;
            _idlGroupName = idlGroupName;
        }

        internal OMContextView(OMContext context, IEnumerable<NamespaceDefinition> filteredNamespaces, string idlGroupName = null)
        {
            _context = context;
            _namespaces = filteredNamespaces;
            _idlGroupName = idlGroupName;
        }

        public IEnumerable<AttributeDefinition> GetAllAttributes()
        {
            return _namespaces.SelectMany(ns => ns.Attributes);
        }

        public IEnumerable<ClassDefinition> GetAllClasses()
        {
            return _namespaces.SelectMany(ns => ns.Classes);
        }

        public IEnumerable<EnumDefinition> GetAllEnums()
        {
            return _namespaces.SelectMany(ns => ns.Enums);
        }

        public IEnumerable<DelegateDefinition> GetAllDelegates()
        {
            return _namespaces.SelectMany(ns => ns.Delegates);
        }

        public IEnumerable<TypeDefinition> GetAllClassesAndEnums()
        {
            return GetAllClasses().Cast<TypeDefinition>().Concat(GetAllEnums().Cast<TypeDefinition>()).Distinct();
        }

        public IEnumerable<TypeDefinition> GetAllTypes()
        {
            return GetAllClasses().Cast<TypeDefinition>().Concat(GetAllEnums().Cast<TypeDefinition>()).Concat(GetAllDelegates().Cast<TypeDefinition>());
        }

        public IEnumerable<TypeReference> GetDeclares()
        {
            Tuple<string, IEnumerable<TypeReference>> result = _context.DeclareSections.FirstOrDefault(d => d.Item1 == _idlGroupName);
            if (result != null)
            {
                return result.Item2;
            }
            return Enumerable.Empty<TypeReference>();
        }

        public IEnumerable<ClassDefinition> GetEventArgsClasses(bool ordered = true)
        {
            var result = _namespaces.SelectMany(ns => ns.Classes.Where(c => c.IsAEventArgs));
            return ordered ? result.OrderBy(c => c.TypeTableIndex) : result;
        }

        public IEnumerable<NamespaceDefinition> GetTypeTableNamespaces()
        {
            return _namespaces.Where(ns => !ns.IsExcludedFromTypeTable).OrderBy(t => t.TypeTableIndex);
        }

        public IEnumerable<TypeDefinition> GetAllTypeTableTypes()
        {
            return GetTypeTableNamespaces().SelectMany(ns => 
                ns.Classes.OfType<TypeDefinition>()
                .Concat<TypeDefinition>(ns.Enums.OfType<TypeDefinition>())
                .Concat<TypeDefinition>(ns.Delegates.OfType<TypeDefinition>())
                .Where(t => !t.IsExcludedFromTypeTable)).OrderBy(t => t.TypeTableIndex);
        }

        public IEnumerable<TypeDefinition> GetAllTypeTableSimpleTypes()
        {
            return GetAllTypeTableTypes().Where((t) => t.IsSimpleType).OrderBy((t) => t.TypeTableIndex);
        }

        public IEnumerable<TypeDefinition> GetTypeTableTypesWithNames()
        {
            return GetAllTypeTableTypes().Where(t => t.CanLookupByName);
        }

        public IEnumerable<EnumDefinition> GetTypeTableEnums()
        {
            return GetAllTypeTableTypes().OfType<EnumDefinition>();
        }

        public IEnumerable<ClassDefinition> GetTypeTableClasses()
        {
            return GetAllTypeTableTypes().OfType<ClassDefinition>();
        }

        public IEnumerable<ClassDefinition> GetAllTypeTableClassesWithSlottedProperties()
        {
            return GetTypeTableClasses().Where(c => !c.IsInterface && !c.IsValueType && c.GetSlottedProperties().Any());
        }

        public IEnumerable<ClassDefinition> GetAllTypeTableClassesWithSimpleProperties()
        {
            return GetTypeTableClasses().Where(c => c.SimpleProperties.Any()).OrderBy((c) => c.TypeTableIndex);
        }

        public IEnumerable<DependencyPropertyDefinition> GetAllTypeTableProperties()
        {
            return GetAllClasses().Where(t => !t.IsExcludedFromTypeTable).OrderBy(t => t.TypeTableIndex)
                .SelectMany(t => t.TypeTableProperties).OrderBy(p => p.TypeTableIndex);
        }

        public IEnumerable<DependencyPropertyDefinition> GetAllTypeTableDependencyProperties()
        {
            return GetAllClasses().Where(t => !t.IsExcludedFromTypeTable).OrderBy(t => t.TypeTableIndex)
                .SelectMany(t => t.TypeTableProperties).Where(p => !p.IsSimpleProperty).OrderBy(p => p.TypeTableIndex);
        }

        public IEnumerable<TypeReference> GetAllDiagSimplePropertyTypes()
        {
            return GetAllTypeTableSimpleProperties()
                .Where(simpleProperty => !(simpleProperty.PropertyType.Type is EnumDefinition || simpleProperty.PropertyType.Type.IsPrimitive))
                .Select(simpleProperty => simpleProperty.PropertyType).Distinct(TypeReferenceEqualityByTypeComparer.Instance)
                .Union(GetAllTypeTableTypes()
                    .Where(t => t.FullName == "Windows.Foundation.Numerics.Quaternion")
                    .Select(t => new TypeReference(t)));
        }

        public IEnumerable<DependencyPropertyDefinition> GetAllTypeTableSimpleProperties()
        {
            return GetAllClasses().Where(t => !t.IsExcludedFromTypeTable).OrderBy(t => t.TypeTableIndex)
                .SelectMany(t => t.TypeTableProperties).Where(p => p.IsSimpleProperty).OrderBy(p => p.TypeTableIndex);
        }

        public IEnumerable<PropertyDefinition> GetAllTypeTableEnterProperties()
        {
            return GetAllClasses().Where(t => !t.IsExcludedFromTypeTable).OrderBy(t => t.TypeTableIndex)
                .SelectMany(t => t.EffectiveTypeTableProperties).Where(p => p.IsEnterProperty && !p.IsSimpleProperty && p.AllowEnumeration).OrderBy(p => p.TypeTableEnterPropertyIndex);
        }

        public IEnumerable<PropertyDefinition> GetAllTypeTableObjectProperties()
        {
            return GetAllClasses().Where(t => !t.IsExcludedFromTypeTable).OrderBy(t => t.TypeTableIndex)
                .SelectMany(t => t.EffectiveTypeTableProperties).Where(p => p.IsObjectProperty && !p.IsSimpleProperty && p.AllowEnumeration).OrderBy(p => p.TypeTableObjectPropertyIndex);
        }

        public IEnumerable<PropertyDefinition> GetAllTypeTableRenderProperties()
        {
            return GetAllClasses().Where(t => !t.IsExcludedFromTypeTable).OrderBy(t => t.TypeTableIndex)
                .SelectMany(t => t.EffectiveTypeTableProperties).Where(p => p.IsRenderProperty && !p.IsSimpleProperty && p.AllowEnumeration).OrderBy(p => p.TypeTableRenderPropertyIndex);
        }

        public IEnumerable<EventDefinition> GetAllEvents()
        {
            return GetAllClasses().Where(t => !t.IsInterface).SelectMany(c => c.Events);
        }

        public IEnumerable<EventDefinition> GetAllControlEvents()
        {
            return GetAllEvents().Where(ev => ev.XamlEventFlags.IsControlEvent);
        }

        public IEnumerable<EventDefinition> GetAllNonControlEvents()
        {
            return GetAllEvents().Where(ev => !ev.XamlEventFlags.IsControlEvent);
        }

        public IEnumerable<TypeReference> GetAllForwarDeclaredEventHandlers()
        {
            return GetAllEvents().Where(ev => ev.IdlEventInfo.ForwardDeclareIReference).Select(ev => ev.EventHandlerType).Distinct(TypeReferenceEqualityByTypeComparer.Instance);
        }

        public IEnumerable<MethodDefinition> GetAllMethods()
        {
            return GetAllClasses()
                .Where(t => !t.IsInterface)
                .SelectMany(c => c.Methods);
        }

        public IEnumerable<string> GetAllMethodIndexNamesNoPrefix()
        {
            return GetAllMethods()
                .Where(m => !m.IsPureVirtual)
                .Select(p => p.IndexNameWithoutPrefix)
                .Distinct(); // Distinct is important to only get one name if a method has multiple versions.
        }

        // TODO 22728412 - Remove this list, GetIdlFileNameImportsLiftedOnly, and GetIdlFileNameImportsNonLiftedOnly.
        // We will only need one set of IDLs (the public versions).
        private static IEnumerable<string> _privateIdlFiles = new List<string> {
            "backgroundwork.idl",
            "datapackage.idl",
            "DragDrop.idl",
            "robytestream.idl",
            "rtmediaframe.idl",
            "searchui.idl",
            "sharingextension.idl",
            "storageitem.idl",
            "uriruntimeclass.idl",
            "uritostreamresolver.idl",
            "windows.applicationmodel.activation.events.idl",
            "windows.system.input.idl",
            "windows.ui.applicationwindow.idl",
            "windows.ui.composition.content.idl",
            "windows.ui.core.corewindow.idl",
            "windows.ui.input.gesturerecognizer.idl",
            "windows.ui.input.inking.directink.idl",
            "windows.ui.input.pointerpoint.idl"
        };

    private IEnumerable<string> _allIdlFileNameImports;
        private IEnumerable<string> GetPossibleIdlFileNameImports()
        {
            if (_allIdlFileNameImports == null)
            {
                _allIdlFileNameImports = _customImports.Concat(IdlImportVisitor.GetImports(this)).Distinct().OrderBy(import => import);
            }

            return _allIdlFileNameImports;
        }

        public IEnumerable<string> GetIdlFileNameImports()
        {
            return GetPossibleIdlFileNameImports().Except(_privateIdlFiles);
        }

        /*
        public IEnumerable<string> GetIdlFileNameImportsLiftedOnly()
        {
            return GetPossibleIdlFileNameImports().Except(_privateIdlFiles);
        }

        public IEnumerable<string> GetIdlFileNameImportsNonLiftedOnly()
        {
            return GetPossibleIdlFileNameImports().Intersect(_privateIdlFiles);
        }
        */

        public void AddCustomIdlFileNameImport(string idlFileName)
        {
            _customImports.Add(idlFileName);
        }

        public IEnumerable<PropertyDefinition> GetAllTypeTableObjectFieldBackedProperties()
        {
            return GetAllTypeTableProperties().Where(x => x.HasField).OrderBy(x => x.DeclaringClass.Name);
        }

        public string GetFieldBackedPropertyMemberName(PropertyDefinition definition)
        {
            if (definition.StorageGroupOffsetClassName != null)
            {
                return string.Format("{0}::{1}->{2}", definition.FieldClassName, definition.FieldName, definition.StorageGroupOffsetFieldName);
            }
            else
            {
                return string.Format("{0}::{1}", definition.FieldClassName, definition.FieldName);
            }
        }

        public string GetFieldBackedPropertyMemberType(PropertyDefinition definition)
        {
            if (definition.NativeStorageType.HasValue)
            {
                switch (definition.NativeStorageType.Value)
                {
                    case ValueType.valueFloat:
                        return "float";

                    case ValueType.valueSigned:
                        return "int32_t";

                    case ValueType.valueEnum:
                        {
                            var ts = GetAllEnums().Where(t => t.AbiReferenceFullName == definition.GetterReturnType.AbiFullName);
                            if (ts.Count() == 1 && ts.First().IsCompact())
                            {
                                return "uint8_t";
                            }

                            return "uint32_t";
                        }

                    case ValueType.valueEnum8:
                        return "uint8_t";

                    case ValueType.valueBool:
                        return "bool";

                    case ValueType.valueColor:
                        return "uint32_t";

                    case ValueType.valueTypeHandle:
                        return "KnownTypeIndex";

                    case ValueType.valueString:
                        return "xstring_ptr";

                    case ValueType.valueDouble:
                        return "double";

                    case ValueType.valuePoint:
                        return "XPOINTF";

                    case ValueType.valueSize:
                        return "XSIZEF";

                    case ValueType.valueRect:
                        return "XRECTF";

                    case ValueType.valueCornerRadius:
                        return "XCORNERRADIUS";

                    case ValueType.valueThickness:
                        return "XTHICKNESS";

                    case ValueType.valueGridLength:
                        return "XGRIDLENGTH";

                    case ValueType.valueObject:
                    case ValueType.valueVO:
                        return "nullptr_t";

                    default:
                        throw new Exception("Unhandled value type");
                }
            }

            if (definition.CoreFieldTypeName == "CObject*")
            {
                return "CValue";
            }

            return definition.CoreFieldTypeName.Replace(".", "::");
        }
    }
}
