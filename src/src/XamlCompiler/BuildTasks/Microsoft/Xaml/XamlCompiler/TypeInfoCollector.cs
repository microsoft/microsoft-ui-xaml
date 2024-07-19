// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Xaml;

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    using CodeGen;
    using DirectUI;
    using RootLog;
    using Tracing;
    using XamlDom;

    internal class TypeInfoCollector
    {
        private DirectUISchemaContext _schemaContext;
        private XamlSchemaCodeInfo _schemaInfo;
        private RootLogBuilder _rootLogBuilder;
        private Platform _targPlat;
        internal ClassName AppXamlInfo { get; set; }

        public TypeInfoCollector(DirectUISchemaContext schemaContext, Platform targPlat)
        {
            _schemaContext = schemaContext;
            _targPlat = targPlat;
            _schemaInfo = new XamlSchemaCodeInfo();
            _rootLogBuilder = new RootLogBuilder();
        }

        public void Collect(XamlDomObject domRoot)
        {
            var iterator = new XamlDomIterator(domRoot);
            foreach (XamlDomObject domObject in iterator.DescendantsAndSelf())
            {
                // Skip validation/collection of objects which won't appear on the targeted platform
                if (DomHelper.IsObjectInvalidForPlatform(domObject, _targPlat))
                {
                    continue;
                }

                if (!domObject.IsGetObject)
                {
                    XamlType xamlType = domObject.Type;
                    if (xamlType.IsUnknown)
                    {
                        // Not a user error message.
                        // The Validator should protect the collector from unknowns.
                        throw new ArgumentOutOfRangeException(xamlType.Name);
                    }

                    if (xamlType.UnderlyingType == null)
                    {
                        // few odd things like the StaticResource proxy need to be skipped.
                        continue;
                    }

                    // Unknown Types are not DirectUIXamlTypes so delay till here to cast.
                    DirectUIXamlType duiXamlType = (DirectUIXamlType)domObject.Type;
                    if (duiXamlType.IsAssignableToStyle)
                    {
                        if (!CollectSettersInStyle(domObject))
                        {
                            _schemaContext.SchemaErrors.Add(new XamlCompilerErrorProcessingStyle(domObject));
                        }
                        continue;
                    }

                    // Add the members of all custom (user created) XamlTypes
                    if (duiXamlType.IsCodeGenType)
                    {
                        _schemaInfo.AddTypeAndProperties(duiXamlType);
                    }

                    // Add just the Type to the Roots Log.  (properties are added as we see them).
                    _rootLogBuilder.AddTypeBuilder(duiXamlType);

                    if (duiXamlType.IsAssignableToBinding)
                    {
                        CollectBindingCtorParam(domObject);
                    }

                    // Also check for custom Attachable properties on any type, even system types.
                    foreach (XamlDomMember domMember in domObject.MemberNodes)
                    {
                        if (domMember.Member.IsDirective)
                        {
                            continue;
                        }
                        if (domMember.Member.IsUnknown)
                        {
                            // Not a user error message.
                            // Validator should protect the collector from unknowns.
                            throw new ArgumentOutOfRangeException(domMember.Member.Name);
                        }

                        DirectUIXamlMember xamlMember = (DirectUIXamlMember)domMember.Member;
                        if (xamlMember.IsAttachable)
                        {
                            DirectUIXamlType attachedDeclaringType = (DirectUIXamlType)xamlMember.DeclaringType;
                            if (attachedDeclaringType.IsCodeGenType)
                            {
                                _schemaInfo.AddTypeAndProperties(attachedDeclaringType);
                            }
                        }

                        // For properties of type System.Type, if they're assigned to a type value in markup,
                        // we need to generate type info for the type value.  E.g. for:
                        // <local:MyControl TypeProperty="local:ACustomType" />
                        // We need to ensure ACustomType is generated in type info so the framework can create the value
                        if (_schemaContext.DirectUISystem.Type.IsAssignableFrom(domMember.Member.Type.UnderlyingType))
                        {
                            string typeName = DomHelper.GetStringValueOfProperty(domMember);
                            if (!String.IsNullOrEmpty(typeName))
                            {
                                XamlType propertyValueType = domObject.ResolveXmlName(typeName);
                                if (propertyValueType != null)
                                {
                                    _schemaInfo.AddTypeAndProperties(propertyValueType);
                                }
                            }
                        }

                        AddMemberToRootLog(duiXamlType, xamlMember);

                        // Examine all properties of type PropertyPath
                        //
                        if (_schemaContext.DirectUISystem.PropertyPath.IsAssignableFrom(domMember.Member.Type.UnderlyingType)
                            || this.ShouldTreatAsPropertyPath(domMember))
                        {
                            CollectPropertyPath(domMember);
                            continue;
                        }
                    }
                }
                else
                {
                    // Extra code gen to help out Phone / Maps
                    // They have some readonly collection properties that appear to be IList<DO> or IVector<DO>
                    // which the runtime cannot do by itself, so we need to add them to the code gen.
                    XamlType xamlType = domObject.Type;
                    if (xamlType.IsUnknown)
                    {
                        // Not a user error message.  Just a guard in case of something bad, we get a better bug report.
                        // The Validator should protect the collector from unknowns.
                        throw new ArgumentOutOfRangeException(xamlType.Name);
                    }
                    XamlDomMember parentMember = domObject.Parent;
                    string parentTypeName = parentMember.Member.DeclaringType.Name;
                    string parentMemberName = parentMember.Member.Name;
                }
            }
        }

        public XamlSchemaCodeInfo SchemaInfo
        {
            get { return _schemaInfo; }
        }

        public Roots RootLog
        {
            get { return _rootLogBuilder.GetRoots(); }
        }

        public void AddTypeToRootLog(DirectUIXamlType duiXamlType)
        {
            _rootLogBuilder.AddTypeBuilder(duiXamlType);
        }

        public void AddMetadataAndBindableTypes(List<Assembly> loadedAssemblies, Assembly localAssembly)
        {
            PerformanceUtility.FireCodeMarker(CodeMarkerEvent.perfXC_SearchIxmpAndBindableStart);
            List<Type> otherProviders;
            List<Type> bindableTypes;
            FindAllMetadataAndBindableTypes(loadedAssemblies, localAssembly, out otherProviders, out bindableTypes);

            // This selects all types we found that implement IXMP, 
            // and excludes"local types", which are types that we may have 
            // received in a reference to a static libray (which is treated as local).
            _schemaInfo.OtherMetadataProviders = otherProviders
                .Select(p => new DirectUIXamlType(p, _schemaContext))
                .Where(p => !DomHelper.IsLocalType(p))
                .Select(p => new TypeForCodeGen(p))
                .ToList();

            foreach (Type type in bindableTypes)
            {
                var xamlType = new DirectUIXamlType(type, _schemaContext);
                InternalTypeEntry bindableType = _schemaInfo.AddBindableType(xamlType);
            }
            PerformanceUtility.FireCodeMarker(CodeMarkerEvent.perfXC_SearchIxmpAndBindableEnd);
        }

        public void AddAllConstructibleTypesFromLocalAssembly(Assembly localAssembly)
        {
            _schemaInfo.TypeInfoReflectionEnabled = true;
            // Note: this was for CX type info reflection, to generate activation codegen
            //  for local types since local types in a native app can't be activated by reflection.
            // However, this could include classes that aren't declared in the Xaml header files or pch,
            // so generating code for them in XamlTypeInfo could leak to build breaks.
            // We don't want to pull in all the header files to solve this, so don't do any special work here.
            // Re-enable this if we figure out a way to map local classes to the headers they're defined in, and whether
            // that header isn't part of the pch or local Xaml header files.
            /*
            foreach (Type type in localAssembly.GetTypes())
            {
                // Check if the type is activatable to determine if we should add it to our type list
                // The presence of the ActivatableAttribute is enough to confirm it's activatable, we don't care about any of its arguments
                var activatableAttributeEnum = Microsoft.UI.Xaml.Markup.Compiler.DirectUI.ReflectionHelper.GetCustomAttributeData(type, false, DUI.ActivatableAttribute);

                // We only want to add activatable types, and also want to avoid adding our own locally defined metadata and type info providers
                if (activatableAttributeEnum != null && activatableAttributeEnum.Any() && !type.FullName.Contains("_XamlTypeInfo"))
                {
                    var xamlType = new DirectUIXamlType(type, _schemaContext);

                    _schemaInfo.AddType(xamlType);
                }
            }
            */
        }

        // ------------------- private -------------------

        private void AddMemberToRootLog(DirectUIXamlType duiType, DirectUIXamlMember duiMember)
        {
            if (duiMember.IsAttachable)
            {
                DirectUIXamlType attachedDeclaringType = (DirectUIXamlType)duiMember.DeclaringType;
                RootTypeBuilder rTypeBuilder = _rootLogBuilder.AddTypeBuilder(attachedDeclaringType);
                if (duiMember.HasPublicGetter)
                {
                    rTypeBuilder.AddMethod("Get" + duiMember.Name);
                }
                if (duiMember.HasPublicSetter)
                {
                    rTypeBuilder.AddMethod("Set" + duiMember.Name);
                }
            }
            else if (duiMember.IsEvent)
            {
                _rootLogBuilder.AddEvent(duiType, duiMember.Name);
            }
            else
            {
                _rootLogBuilder.AddProperty(duiType, duiMember.Name);
            }
        }

        private bool CollectSettersInStyle(XamlDomObject StyleObject)
        {
            // The validator should protect us from these failures (with better error messages)

            XamlDomMember domTargetTypeMember = StyleObject.GetMemberNode(KnownMembers.TargetType);
            if (domTargetTypeMember == null)
            {
                return false;
            }
            string xTypeName = DomHelper.GetStringValueOfProperty(domTargetTypeMember);
            if (xTypeName == null)
            {
                return false;
            }
            XamlType xamlTargetType = StyleObject.ResolveXmlName(xTypeName);
            if (xamlTargetType == null)
            {
                return false;
            }

            var duiTargetType = xamlTargetType as DirectUIXamlType;
            if (duiTargetType != null && duiTargetType.IsCodeGenType)
            {
                _schemaInfo.AddTypeAndProperties(duiTargetType);
            }

            _rootLogBuilder.AddTypeBuilder(duiTargetType);

            // Continue Searching the Setters in case there are attachable properties.

            XamlDomMember settersCollectionProperty = StyleObject.GetMemberNode("Setters");
            if (settersCollectionProperty == null || settersCollectionProperty.Item == null)
            {
                return true;    // No "Setters" Property is allowed
            }
            XamlDomObject setterBaseCollectionObject = settersCollectionProperty.Item as XamlDomObject;
            if (setterBaseCollectionObject == null)
            {
                return false;
            }
            XamlDomMember itemsProperty = setterBaseCollectionObject.GetMemberNode(XamlLanguage.Items);
            if (itemsProperty == null || itemsProperty.Items.Count == 0)
            {
                return true;    // Empty collection of Setters is allowed.
            }

            foreach (XamlDomItem domItem in itemsProperty.Items)
            {
                var domSetterObject = domItem as XamlDomObject;
                if (domSetterObject == null || domSetterObject.Type.IsUnknown)
                {
                    // Only checking Setters
                    continue;
                }
                if (!domSetterObject.Type.UnderlyingType.IsAssignableFrom(_schemaContext.DirectUISystem.Setter))
                {
                    // Only checking Setters
                    continue;
                }
                CollectSingleSetter(xamlTargetType, domSetterObject);
            }
            return true;
        }

        private bool CollectSingleSetter(XamlType xamlTargetType, XamlDomObject domSetterObject)
        {
            // Find the Property Property.
            XamlDomMember domPropertyMember = domSetterObject.GetMemberNode("Property");
            if (domPropertyMember == null)
            {
                // Target and Property are interchangable.
                domPropertyMember = domSetterObject.GetMemberNode("Target");
                if (domPropertyMember == null)
                {
                    return false;
                }
            }
            String propertyName = DomHelper.GetStringValueOfProperty(domPropertyMember);
            if (propertyName == null)
            {
                return false;
            }

            // ResolveMemberName will handle Attachable Property (aka "dotted") syntax.
            XamlMember member = domSetterObject.ResolveMemberName(xamlTargetType, propertyName);
            if (member == null)
            {
                return false;
            }

            // Find the Value Property
            XamlDomMember domValueMember = domSetterObject.GetMemberNode("Value");
            if (domValueMember == null)
            {
                return false;
            }
            if (domValueMember.Item == null)
            {
                return false;
            }
            DirectUIXamlType declaringType = (DirectUIXamlType)member.DeclaringType;
            if (declaringType.IsCodeGenType)
            {
                InternalTypeEntry declaringTypeEntry = _schemaInfo.AddType(declaringType);
                _schemaInfo.AddMember(declaringTypeEntry, member);
            }

            DirectUIXamlMember duiMember = (DirectUIXamlMember)member;
            AddMemberToRootLog(declaringType, duiMember);
            return true;
        }

        // Binding's One Ctor argument, is the PropertyPath
        private void CollectBindingCtorParam(XamlDomObject domBinding)
        {
            XamlDomMember parameterMember = domBinding.GetMemberNode(XamlLanguage.PositionalParameters);
            if (parameterMember == null)
            {
                return;
            }
            XamlDomItem firstItem = parameterMember.Item;
            XamlDomValue pathValue = parameterMember.Item as XamlDomValue;
            if (pathValue != null)
            {
                String pathString = pathValue.Value as String;
                if (pathString != null)
                {
                    CollectPropertyPath(pathString, domBinding);
                }
            }
        }

        private void CollectPropertyPath(XamlDomMember pathProperty)
        {
            string pathString = DomHelper.GetStringValueOfProperty(pathProperty);
            if (pathString == null)
            {
                return;
            }
            XamlDomObject domBinding = pathProperty.Parent;
            CollectPropertyPath(pathString, domBinding);
        }

        private void CollectPropertyPath(String pathString, XamlDomObject domBinding)
        {
            List<String> qualifiedMemberNames;
            List<String> names;
            if (!PropertyPathParser.Parse(pathString, out qualifiedMemberNames, out names))
            {
                return; // property path failed to Parse correctly.
            }

            if (names != null)
            {
                foreach (String name in names)
                {
                    _rootLogBuilder.AddPropertyPathName(name);
                }
            }
            if (qualifiedMemberNames != null)
            {
                foreach (String qName in qualifiedMemberNames)
                {
                    // we can pass null because we guarentee there is a dot in the name.
                    XamlMember xamlMember = domBinding.ResolveMemberName(qName);
                    DirectUIXamlType duiType = (DirectUIXamlType)xamlMember.DeclaringType;
                    DirectUIXamlMember duiMember = (DirectUIXamlMember)xamlMember;

                    AddMemberToRootLog(duiType, duiMember);
                }
            }
        }

        private void FindAllMetadataAndBindableTypes(IList<Assembly> loadedAssemblies, Assembly localAssembly,
                                                    out List<Type> otherProviders, out List<Type> bindableTypes)
        {
            otherProviders = new List<Type>();
            bindableTypes = new List<Type>();
            foreach (Assembly asm in loadedAssemblies)
            {
                Type[] allTypes = null;
                try
                {
                    allTypes = asm.GetTypes();
                }
                catch (Exception ex)
                {
                    String error = ex.Message;
                    var refEx = ex as ReflectionTypeLoadException;
                    if (refEx != null)
                    {
                        Exception[] exs = refEx.LoaderExceptions;
                        allTypes = refEx.Types;
                    }
                }

                bool isWinMd = asm.GetName().ContentType == AssemblyContentType.WindowsRuntime;

                foreach (Type type in allTypes)
                {
                    try
                    {
                        // Ignore non-public types or cs/winrt types that start with "ABI." - which is an 
                        // implementation detail
                        if (type == null || !type.IsPublic || type.FullName.StartsWith("ABI."))
                        {
                            continue;
                        }

                        if (!isWinMd)
                        {
                            if (!type.IsClass || type.IsAbstract)
                            {
                                continue;
                            }
                        }

                        if (HasBindableAttribute(type))
                        {
                            if (type.IsGenericType)
                            {
                                _schemaContext.SchemaErrors.Add(new XamlSchemaError_BindableNotSupportedOnGeneric(type.FullName));
                            }
                            else
                            {
                                bindableTypes.Add(type);
                            }
                        }
                        if (asm != localAssembly)
                        {
                            if (HasInterface(type, KnownTypes.IXamlMetadataProvider))
                            {
                                // If there is an App type in a referenced assembly, we don't want to include that in the OtherProviders because
                                // only one MUX.Application object can be created in a process so we don't want to try to instantiate it.
                                var isAppType = DerivesFromBaseType(type, KnownTypes.Application);
                                if (!isAppType)
                                {
                                    otherProviders.Add(type);
                                }
                            }
                        }
                    }
                    catch (Exception)
                    {
                        // When checking for bindable types and metadata providers, we may stumble across types that aren't used by our code but wouldn't be able to resolve
                        // due to misssing/incorrect dependencies.  This technically shouldn't be an error for us as long as the user's code doesn't try to use the type also,
                        // which would be caught by the code compiler.  https://github.com/microsoft/microsoft-ui-xaml/issues/5143 is an instance of this occurring,
                        // where an otherwise unused type depends on WPF and System.Windows.DependencyObject which isn't present for the WinUI app.
                        // As for why this nasty try/catch, our type system assumes all types can be resolved, and refactoring it to support unresolved types is prohibitively expensive.
                    }
                }
            }
        }

        private bool HasBindableAttribute(Type type)
        {
            foreach (CustomAttributeData attr in Microsoft.UI.Xaml.Markup.Compiler.DirectUI.ReflectionHelper.GetCustomAttributeData(type, false, KnownTypes.WindowsBindableAttribute))
            {
                return true;
            }
            foreach (CustomAttributeData attr in Microsoft.UI.Xaml.Markup.Compiler.DirectUI.ReflectionHelper.GetCustomAttributeData(type, false, KnownTypes.BindableAttribute))
            {
                return true;
            }
            return false;
        }

        private bool HasInterface(Type type, String InterfaceName)
        {
            Type iface = null;
            try
            {
                iface = type.GetInterface(InterfaceName);
            }
            catch (Exception)
            {
                // no it does not have this interface.
            }

            return iface != null;
        }

        private bool DerivesFromBaseType(Type type, String baseTypeName)
        {
            Type basetype = type;
            while (basetype != null)
            {
                if (basetype.FullName == baseTypeName)
                {
                    return true;
                }
                basetype = basetype.BaseType;
            }
            return false;
        }

        /// <summary>
        /// A workaround for OSG-2051809. Some Properties are defined as strings instead of PropertyPaths.
        /// </summary>
        private bool ShouldTreatAsPropertyPath(XamlDomMember domMember)
        {
            string name = null;
            if (domMember != null && domMember.Member != null && domMember.Member.UnderlyingMember != null)
            {
                name = domMember.Member.UnderlyingMember.Name;
            }
            switch (name)
            {
                case "DisplayMemberPath":
                    return (domMember.Member.DeclaringType.UnderlyingType.FullName == KnownNamespaces.XamlControls + ".ItemsControl") ||
                        (domMember.Member.DeclaringType.UnderlyingType.FullName == KnownNamespaces.XamlControls + ".ListPickerFlyout");
                case "TextMemberPath":
                    return domMember.Member.DeclaringType.UnderlyingType.FullName == KnownNamespaces.XamlControls + ".AutoSuggestBox";
                case "SelectedValuePath":
                    return domMember.Member.DeclaringType.UnderlyingType.FullName == KnownNamespaces.XamlControls + ".Primitives.Selector";
                default:
                    return false;
            }
        }
    }
}
