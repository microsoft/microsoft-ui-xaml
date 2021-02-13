// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#if MSBUILD_TASK
using Microsoft.Build.Framework;
#endif
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.InteropServices.WindowsRuntime;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

// NOTE: The MSBUILD_TASK defines here are so that this file can be cross-compiled for dep.controls as an MSBuild task
// that is packaged into a nuget package (CustomTasks) and then compiled on the fly in the OS repo during RunWUXCCodeGen.
// The on-the-fly compile is using Powershell, hence the limited references and C# 5.0 language support.

namespace CustomTasks
{
    public class DependencyPropertyCodeGen
#if MSBUILD_TASK
        : Microsoft.Build.Utilities.AppDomainIsolatedTask
#endif
    {
#if MSBUILD_TASK
        [Required]
#endif
        public string OutputDirectory
        {
            get;
            set;
        }

#if MSBUILD_TASK
        [Required]
#endif
        public string[] WinMDInput
        {
            get;
            set;
        }

#if MSBUILD_TASK
        [Required]
#endif
        public string[] References
        {
            get;
            set;
        }

#if MSBUILD_TASK
        [Output]
#endif
        public string[] FilesWritten
        {
            get;
            set;
        }

        private List<string> _pendingFilesWritten = new List<string>();

#if MSBUILD_TASK

        public override bool Execute()
#else
        public bool Execute()
#endif
        {
            try
            {
                List<Type> types = WinMDInput.SelectMany(x => GetTypes(x, References)).ToList();

                var collectedMetadata = new List<TypeDefinition>();

                foreach (var type in types)
                {
                    var typeDefinition = new TypeDefinition();
                    typeDefinition.Type = type;

                    typeDefinition.HasCustomActivationFactory = HasAttribute("MUXHasCustomActivationFactoryAttribute", type);

                    typeDefinition.NeedsActivationFactory = NeedsActivationFactory(type);

                    var props = typeDefinition.Properties = CollectProperties(type);

                    var events = typeDefinition.Events = CollectEvents(type);

                    // If this type has DependencyProperty properties then write out the helpers
                    typeDefinition.HasHeaderFile = (props.Count > 0 || events.Count > 0);
                    typeDefinition.HasImplementationFile = typeDefinition.HasHeaderFile || (!typeDefinition.HasCustomActivationFactory && typeDefinition.NeedsActivationFactory);
                    if (typeDefinition.HasHeaderFile || typeDefinition.HasImplementationFile)
                    {
                        collectedMetadata.Add(typeDefinition);
                    }
                }

                foreach (var typeDefinition in collectedMetadata)
                {
                    var type = typeDefinition.Type;
                    string header = WriteHeader(typeDefinition);
                    string impl = WriteImplementation(typeDefinition, collectedMetadata);

                    string headerPath = Path.Combine(OutputDirectory, type.Name + ".properties.h");
                    string implPath = Path.Combine(OutputDirectory, type.Name + ".properties.cpp");

                    try
                    {
                        Directory.CreateDirectory(OutputDirectory);
                    }
                    catch
                    {
                    }

                    if (typeDefinition.HasHeaderFile)
                    {
                        RewriteFileIfNecessary(headerPath, header);
                    }
                    if (typeDefinition.HasImplementationFile)
                    {
                        RewriteFileIfNecessary(implPath, impl);
                    }
                }

                FilesWritten = _pendingFilesWritten.ToArray();

                return true;
            }
            catch (Exception e)
            {
                Console.WriteLine("ERROR: {0}", e.ToString());
                return false;
            }
        }

        private List<PropertyDefinition> CollectProperties(Type type)
        {
            var props = new List<PropertyDefinition>();
            var propInfos = type.GetProperties().OrderBy(x => x.Name);
            // Go through the dependency property properties.
            foreach (var propInfo in propInfos)
            {
                bool IsIgnored = GetPropertyIgnoredByCodeGen(type, propInfo);
                if (!IsIgnored)
                {
                    if (propInfo.PropertyType.Name == "DependencyProperty")
                    {
                        if (!propInfo.Name.EndsWith("Property"))
                        {
                            throw new Exception(String.Format("Unexpected: {0}.{1} does not end with 'Property'", type.Name, propInfo.Name));
                        }

                        string baseName = propInfo.Name.Substring(0, propInfo.Name.Length - "Property".Length);
                        var instanceProperty = propInfos.FirstOrDefault(x => x.Name == baseName);

                        var propDefinition = CollectProperty(type, propInfo, baseName, instanceProperty);
                        props.Add(propDefinition);
                    }
                    else
                    {
                        // If it's not a dependency property but it has the "needs dependency property" attribute then generate the field anyway.
                        string baseName = propInfo.Name;

                        bool needsDependencyPropertyField = NeedsDependencyPropertyField(type, propInfo);
                        if (needsDependencyPropertyField && !props.Any(x => x.Name == baseName))
                        {
                            PropertyDefinition propDefinition = CollectProperty(type, null, baseName, propInfo);
                            propDefinition.NeedsDependencyPropertyField = true;
                            props.Add(propDefinition);
                        }
                    }
                }
            }

            return props.OrderBy(x => x.Name).ToList();
        }

        private PropertyDefinition CollectProperty(Type type, PropertyInfo dependencyProperty, string baseName, PropertyInfo instanceProperty)
        {
            var needsPropChangedCallback = NeedsPropertyChangedCallback(dependencyProperty, instanceProperty, type);
            var defaultValue = GetDefaultValue(dependencyProperty, instanceProperty, type);
            string propertyChangedCallbackMethodName = GetPropertyChangedCallbackMethodName(dependencyProperty, instanceProperty, type);
            string propertyValidationCallback = GetPropertyValidationCallback(dependencyProperty, instanceProperty, type);

            if (instanceProperty != null)
            {
                return new PropertyDefinition
                {
                    Name = baseName,
                    PropertyType = instanceProperty.PropertyType,
                    PropertyCppName = CppName(instanceProperty.PropertyType),
                    InstanceProperty = instanceProperty,
                    DependencyProperty = dependencyProperty,
                    NeedsPropChangedCallback = needsPropChangedCallback ?? false,
                    PropChangedCallbackMethodName = propertyChangedCallbackMethodName,
                    PropertyValidationCallback = propertyValidationCallback,
                    DefaultValue = defaultValue
                };
            }
            else
            {
                // Couldn't find an instance property, this must be an attached property.
                MethodInfo getMethod = type.GetMethod(String.Format("Get{0}", baseName));
                if (getMethod != null && getMethod.GetParameters().Length == 1 && getMethod.IsStatic)
                {
                    Type propertyType = getMethod.ReturnType;
                    return new PropertyDefinition
                    {
                        Name = baseName,
                        PropertyType = propertyType,
                        PropertyCppName = CppName(propertyType),
                        InstanceProperty = null,
                        DependencyProperty = dependencyProperty,
                        AttachedPropertyTargetType = getMethod.GetParameters()[0].ParameterType,
                        NeedsPropChangedCallback = false,
                        PropChangedCallbackMethodName = propertyChangedCallbackMethodName,
                        PropertyValidationCallback = propertyValidationCallback,
                        DefaultValue = defaultValue
                    };
                }
                else
                {
                    String typeOverride = GetPropertyTypeOverride(dependencyProperty);
                    if (typeOverride != null)
                    {
                        // Typeless property definition, just registering the property.
                        return new PropertyDefinition
                        {
                            Name = baseName,
                            PropertyType = null,
                            PropertyCppName = typeOverride,
                            InstanceProperty = null,
                            DependencyProperty = dependencyProperty,
                            AttachedPropertyTargetType = null,
                            NeedsPropChangedCallback = needsPropChangedCallback ?? false,
                            PropChangedCallbackMethodName = propertyChangedCallbackMethodName,
                            PropertyValidationCallback = propertyValidationCallback,
                            DefaultValue = defaultValue
                        };
                    }
                    else
                    {
#if MSBUILD_TASK
                        Log.LogError("Type {0} had a DependencyProperty {1} without a matching instance property, Get{1} method, or [MUX_PROPERTY_TYPE(...)] attribute", type.Name, baseName);
                        return null;
#else
                        throw new Exception(String.Format("Type {0} had a DependencyProperty {1} without a matching instance property, Get{1} method, or [MUX_PROPERTY_TYPE(...)] attribute", type.Name, baseName));
#endif
                    }
                }
            }
        }

        private List<EventDefinition> CollectEvents(Type type)
        {
            var events = new List<EventDefinition>();
            foreach (var eventInfo in type.GetEvents().OrderBy(x => x.Name))
            {
                if (eventInfo.DeclaringType == type && !eventInfo.AddMethod.IsStatic) // Have to do this because BindingFlags.DeclaredOnly doesn't filter to owning type
                {
                    events.Add(new EventDefinition
                    {
                        Name = eventInfo.Name,
                        EventInfo = eventInfo
                    });
                }
            }

            return events.OrderBy(x => x.Name).ToList();
        }

        private class TypeDefinition
        {
            public Type Type;
            public bool HasCustomActivationFactory;
            public bool NeedsActivationFactory;
            public List<PropertyDefinition> Properties;
            public List<EventDefinition> Events;
            public bool HasHeaderFile;
            public bool HasImplementationFile;
        };

        private class PropertyDefinition
        {
            public string Name;
            public Type PropertyType;
            public string PropertyCppName;
            public PropertyInfo InstanceProperty;
            public PropertyInfo DependencyProperty;
            public Type AttachedPropertyTargetType;
            public string DefaultValue;
            public bool NeedsPropChangedCallback;
            public string PropChangedCallbackMethodName;
            public bool NeedsDependencyPropertyField;
            public string PropertyValidationCallback;

            public string GetClassFuncName()
            {
                return $"On{Name}PropertyChanged";
            }
        }

        private struct EventDefinition
        {
            public string Name;
            public EventInfo EventInfo;
        }

        private string CppInputModifier(Type type)
        {
            if (type.IsPrimitive)
            {
                return "";
            }

            return "const& ";
        }

        private string CppName(Type type)
        {
            if (type.Name == "Object") return "winrt::IInspectable";
            if (type.Name == "String") return "winrt::hstring";
            if (type.Name == "Vector2") return "winrt::float2";
            if (type.Name == "Vector3") return "winrt::float3";
            if (type.Name == "Vector4") return "winrt::float4";
            // Special case for Repeater's dual-implemented type, if this becomes more common we can figure
            // out how to do this as an attribute annotation. The generated code expects this to be typedef'd.
            if (type.Name == "IElementFactory" || type.Name == "ViewGenerator") return "ViewGeneratorPropertyType";

            if (type.IsPrimitive)
            {
                switch (type.Name)
                {
                    case "Boolean": return "bool";
                    case "Int32": return "int";
                    case "Single": return "float";
                    case "Double": return "double";

                    default:
                        throw new Exception(String.Format("Unimplemented primitive type {0}", type.Name));
                }
            }
            if (type.IsGenericType)
            {
                var genericArguments = type.GenericTypeArguments;
                string genericName = type.FullName.Substring(0, type.FullName.IndexOf("`"));

                switch (genericName)
                {
                    case "System.Nullable": genericName = "IReference"; break;
                    case "System.Collections.Generic.IList": genericName = "IVector"; break;
                    case "Windows.Foundation.TypedEventHandler": genericName = "TypedEventHandler"; break;
                    case "System.EventHandler": genericName = "EventHandler"; break;
                    default:
                        throw new Exception(String.Format("Unimplemented generic type {0}, type {1}", genericName, type.Name));
                }

                string formattedName = "winrt::" + genericName
                    + "<"
                    + String.Join(", ", genericArguments.Select(x => CppName(x)))
                    + ">";
                return formattedName;
            }
            return "winrt::" + type.Name;
        }

        private string EventFieldName(string name)
        {
            return "m_" + name.Substring(0, 1).ToLowerInvariant() + name.Substring(1) + "EventSource";
        }

        private bool NeedsActivationFactory(Type type)
        {
            foreach (var attribute in type.CustomAttributes)
            {
                var name = attribute.AttributeType.Name;
                if (name == "ComposableAttribute")
                {
                    // Is it public?
                    var value = (int)attribute.ConstructorArguments[1].Value;
                    if (value == 2) // public
                    {
                        return true;
                    }
                    // Or protected with publicly visible create method?
                    var composableInterface = (Type)attribute.ConstructorArguments[0].Value;
                    if (composableInterface.GetMethods().Length > 0)
                    {
                        return true;
                    }
                }
                if (name == "ActivatableAttribute")
                {
                    return true;
                }
                if (name == "StaticAttribute")
                {
                    return true;
                }
            }

            return false;
        }

        private T GetAttributeValue<T>(string name, params MemberInfo[] members)
        {
            foreach (var member in members)
            {
                if (member == null) continue;

                foreach (var attribute in member.CustomAttributes)
                {
                    if (attribute.AttributeType.Name == name)
                    {
                        return (T)attribute.NamedArguments[0].TypedValue.Value;
                    }
                }
            }

            return default(T);
        }

        private bool HasAttribute(string name, params MemberInfo[] members)
        {
            foreach (var member in members)
            {
                if (member == null) continue;

                foreach (var attribute in member.CustomAttributes)
                {
                    if (attribute.AttributeType.Name == name)
                    {
                        return true;
                    }
                }
            }

            return false;
        }

        private bool? NeedsPropertyChangedCallback(params MemberInfo[] members)
        {
            return GetAttributeValue<bool?>("MUXPropertyChangedCallbackAttribute", members);
        }

        private bool NeedsDependencyPropertyField(params MemberInfo[] members)
        {
            return HasAttribute("MUXPropertyNeedsDependencyPropertyFieldAttribute", members);
        }

        private string GetPropertyChangedCallbackMethodName(params MemberInfo[] members)
        {
            return GetAttributeValue<string>("MUXPropertyChangedCallbackMethodNameAttribute", members);
        }

        private string GetPropertyValidationCallback(params MemberInfo[] members)
        {
            return GetAttributeValue<string>("MUXPropertyValidationCallbackAttribute", members);
        }

        private string GetDefaultValue(params MemberInfo[] members)
        {
            return GetAttributeValue<string>("MUXPropertyDefaultValueAttribute", members);
        }

        private string GetPropertyTypeOverride(params MemberInfo[] members)
        {
            return GetAttributeValue<string>("MUXPropertyTypeAttribute", members);
        }

        private string GetPropertyIgnoredByCodeGen(params MemberInfo[] members)
        {
            return GetAttributeValue<string>("MUXPropertyIgnoredByCodeGen", members);
        }

        private string WriteHeader(TypeDefinition typeDefinition)
        {
            var typeName = typeDefinition.Type.Name;
            var props = typeDefinition.Properties;
            var events = typeDefinition.Events;

            //System.Console.WriteLine("Type: {0}", typeDefinition.Type.Name);

            StringBuilder sb = new StringBuilder();

            sb.AppendLine("// Copyright (c) Microsoft Corporation. All rights reserved.");
            sb.AppendLine("// Licensed under the MIT License. See LICENSE in the project root for license information.");
            sb.AppendLine("");
            sb.AppendLine("// DO NOT EDIT! This file was generated by CustomTasks.DependencyPropertyCodeGen");

            if (props.Count > 0 || events.Count > 0)
            {
                // Header
                sb.Append(String.Format(
    @"#pragma once

class {0}Properties
{{
public:
    {0}Properties();

", typeName));

                // Instance property methods
                foreach (var prop in props)
                {
                    if (prop.InstanceProperty != null)
                    {
                        // All dependency properties can be read or written, the policy is set by the ABI.
                        sb.AppendLine(String.Format("    void {0}({1} {2}value);", prop.Name, prop.PropertyCppName, CppInputModifier(prop.PropertyType)));
                        sb.AppendLine(String.Format("    {0} {1}();", prop.PropertyCppName, prop.Name));
                        sb.AppendLine();
                    }
                    else if (prop.AttachedPropertyTargetType != null)
                    {
                        sb.AppendLine(String.Format("    static void Set{0}({1} const& target, {2} {3}value);",
                            prop.Name, CppName(prop.AttachedPropertyTargetType), prop.PropertyCppName, CppInputModifier(prop.PropertyType)));
                        sb.AppendLine(String.Format("    static {0} Get{1}({2} const& target);", prop.PropertyCppName, prop.Name, CppName(prop.AttachedPropertyTargetType)));
                        sb.AppendLine();
                    }
                }

                // DP methods
                foreach (var prop in props)
                {
                    sb.AppendLine(String.Format("    static winrt::DependencyProperty {0}Property() {{ return s_{0}Property; }}", prop.Name));
                }

                sb.AppendLine();

                // DP fields
                foreach (var prop in props)
                {
                    sb.AppendLine(String.Format("    static GlobalDependencyProperty s_{0}Property;", prop.Name));
                }

                if (events.Count > 0)
                {
                    sb.AppendLine();

                    // Events
                    foreach (var eventInfo in events)
                    {
                        sb.AppendLine(String.Format("    winrt::event_token {0}({1} const& value);", eventInfo.Name, CppName(eventInfo.EventInfo.EventHandlerType)));
                        sb.AppendLine(String.Format("    void {0}(winrt::event_token const& token);", eventInfo.Name));
                    }

                    sb.AppendLine();

                    // Event storage
                    foreach (var eventInfo in events)
                    {
                        sb.AppendLine(String.Format("    event_source<{0}> {1};", CppName(eventInfo.EventInfo.EventHandlerType), EventFieldName(eventInfo.Name)));
                    }
                }

                sb.Append(
    @"
    static void EnsureProperties();
    static void ClearProperties();
");

                var needsPropertyChanged = props.Where(x => x.NeedsPropChangedCallback || x.PropertyValidationCallback != null);
                foreach (var prop in needsPropertyChanged)
                {
                    sb.Append($@"
    static void {prop.GetClassFuncName()}(
        winrt::DependencyObject const& sender,
        winrt::DependencyPropertyChangedEventArgs const& args);
");
                }

                sb.AppendLine("};");
            }

            return sb.ToString();
        }


        private string WriteImplementation(TypeDefinition typeDefinition, List<TypeDefinition> allTypes)
        {
            var ownerType = typeDefinition.Type;
            var props = typeDefinition.Properties;
            var events = typeDefinition.Events;
            var baseType = typeDefinition.Type.BaseType;
            var baseTypeIsInComponent = allTypes.Any(x => x.Type == baseType && x.Properties.Count > 0);

            StringBuilder sb = new StringBuilder();

            sb.AppendLine("// Copyright (c) Microsoft Corporation. All rights reserved.");
            sb.AppendLine("// Licensed under the MIT License. See LICENSE in the project root for license information.");
            sb.AppendLine("");
            sb.AppendLine("// DO NOT EDIT! This file was generated by CustomTasks.DependencyPropertyCodeGen");

            // Header
            sb.AppendLine("#include \"pch.h\"");
            sb.AppendLine("#include \"common.h\"");
            sb.AppendLine("#include \"" + ownerType.Name + ".h\"");
            sb.AppendLine();

            // Activation factory
            if (!typeDefinition.HasCustomActivationFactory && typeDefinition.NeedsActivationFactory)
            {
                var cppNamespace = ownerType.Namespace.Replace(".", "::");
                sb.AppendLine($"namespace winrt::{cppNamespace}");
                sb.AppendLine($"{{");
                if (props.Count > 0)
                {
                    sb.AppendLine(String.Format("    CppWinRTActivatableClassWithDPFactory({0})", ownerType.Name));
                }
                else
                {
                    sb.AppendLine(String.Format("    CppWinRTActivatableClassWithBasicFactory({0})", ownerType.Name));
                }
                sb.AppendLine($"}}");
                sb.AppendLine();
                sb.AppendLine($"#include \"{ownerType.Name}.g.cpp\"");
                sb.AppendLine();
            }

            // Field declarations
            foreach (var prop in props)
            {
                sb.AppendLine(String.Format("GlobalDependencyProperty {0}Properties::s_{1}Property{{ nullptr }};", ownerType.Name, prop.Name));
            }

            sb.AppendLine();

            if (props.Count > 0 || events.Count > 0)
            {
                sb.AppendLine(String.Format("{0}Properties::{0}Properties()", ownerType.Name));
                if (events.Count > 0)
                {
                    bool isFirst = true;
                    foreach (var eventInfo in events)
                    {
                        sb.AppendLine(String.Format("    {2} {0}{{static_cast<{1}*>(this)}}", EventFieldName(eventInfo.Name), ownerType.Name, isFirst ? ":" : ","));
                        isFirst = false;
                    }
                }
                sb.AppendLine("{");
                if (props.Count > 0)
                {
                    sb.AppendLine("    EnsureProperties();");
                }
                sb.AppendLine("}");
                sb.AppendLine();

                // EnsureProperties
                sb.AppendLine(String.Format("void {0}Properties::EnsureProperties()", ownerType.Name));
                sb.AppendLine("{");
                if (baseTypeIsInComponent)
                {
                    sb.AppendLine(String.Format("    {0}::EnsureProperties();", baseType.Name));
                }

                foreach (var prop in props)
                {
                    string defaultValue = String.Format("ValueHelper<{0}>::", prop.PropertyCppName);
                    if (prop.DefaultValue == null)
                    {
                        defaultValue += "BoxedDefaultValue()";
                    }
                    else
                    {
                        if (prop.PropertyType != null && prop.PropertyType.Name == "String" && !(prop.DefaultValue.StartsWith("\"") && prop.DefaultValue.EndsWith("\"")))
                        {
                            // Strings are special and need to be quoted, check first that the provided string is not quoted.
                            defaultValue += String.Format("BoxValueIfNecessary(L\"{0}\")", prop.DefaultValue);
                        }
                        else
                        {
                            defaultValue += String.Format("BoxValueIfNecessary({0})", prop.DefaultValue);
                        }
                    }

                    string callback = "nullptr";
                    if (prop.PropChangedCallbackMethodName != null && prop.AttachedPropertyTargetType != null)
                    {
                        if (prop.PropertyValidationCallback != null)
                        {
#if MSBUILD_TASK
                            Log.LogError("Custom property changed callback and validation callback are not supported, type {0} property {1}", ownerType.Name, prop.Name);
#else
                        throw new Exception("Custom property changed callback and validation callback are not supported, type {0} property {1}", ownerType.Name, prop.Name);
#endif
                        }
                        callback = String.Format("&{0}::{1}", ownerType.Name, prop.PropChangedCallbackMethodName);
                    }
                    else if (prop.NeedsPropChangedCallback || prop.PropertyValidationCallback != null)
                    {
                        callback = $"winrt::PropertyChangedCallback(&On{prop.Name}PropertyChanged)";
                    }

                    sb.AppendLine(String.Format(
    @"    if (!s_{0}Property)
    {{
        s_{0}Property =
            InitializeDependencyProperty(
                L""{0}"",
                winrt::name_of<{1}>(),
                winrt::name_of<{2}>(),
                {5} /* isAttached */,
                {3},
                {4});
    }}", prop.Name, prop.PropertyCppName, CppName(ownerType), defaultValue, callback, (prop.InstanceProperty == null) ? "true" : "false"));
                }
                sb.AppendLine("}");
                sb.AppendLine();

                // ClearProperties
                sb.AppendLine(String.Format("void {0}Properties::ClearProperties()", ownerType.Name));
                sb.AppendLine("{");
                foreach (var prop in props)
                {
                    sb.AppendLine(String.Format("    s_{0}Property = nullptr;", prop.Name));
                }
                if (baseTypeIsInComponent)
                {
                    sb.AppendLine(String.Format("    {0}::ClearProperties();", baseType.Name));
                }
                sb.AppendLine("}");
            }

            bool needsPropertyChanged = props.Any(x => x.NeedsPropChangedCallback);
            bool hasValidationCallback = props.Any(x => x.PropertyValidationCallback != null);
            if (needsPropertyChanged || hasValidationCallback)
            {
                foreach (var prop in props.Where(x => x.NeedsPropChangedCallback || x.PropertyValidationCallback != null))
                {
                    sb.AppendLine();
                    // PropertyChanged callback
                    sb.AppendLine(
$@"void {ownerType.Name}Properties::{prop.GetClassFuncName()}(
    winrt::DependencyObject const& sender,
    winrt::DependencyPropertyChangedEventArgs const& args)
{{
    auto owner = sender.as<{CppName(ownerType)}>();");
                    
                    if (prop.PropertyValidationCallback != null)
                    {
                        string comparison = "if (value != coercedValue)";
                        string propertyCppName = prop.PropertyCppName;
                        if (propertyCppName == "double" || propertyCppName == "float")
                        {
                            comparison = "if (std::memcmp(&value, &coercedValue, sizeof(value)) != 0) // use memcmp to avoid tripping over nan";
                        }
                        sb.AppendLine(String.Format(@"
    auto value = winrt::unbox_value<{2}>(args.NewValue());
    auto coercedValue = value;
    winrt::get_self<{0}>(owner)->{1}(coercedValue);
    {3}
    {{
        sender.SetValue(args.Property(), winrt::box_value<{2}>(coercedValue));
        return;
    }}
", ownerType.Name, prop.PropertyValidationCallback, propertyCppName, comparison));
                    }

                    if (prop.NeedsPropChangedCallback)
                    {
                        string ownerFuncName = prop.PropChangedCallbackMethodName ?? prop.GetClassFuncName();
                        sb.AppendLine(
$@"    winrt::get_self<{ownerType.Name}>(owner)->{ownerFuncName}(args);");
                    }

                    sb.AppendLine("}");
                }
            }

            // Instance property methods
            foreach (var prop in props)
            {
                sb.AppendLine();
                if (prop.InstanceProperty != null)
                {
                    sb.AppendLine(String.Format(
@"void {0}Properties::{1}({2} {3}value)
{{", ownerType.Name, prop.Name, prop.PropertyCppName, CppInputModifier(prop.PropertyType)));
                    string localName = "value";
                    sb.AppendLine(@"    [[gsl::suppress(con)]]
    {");
                    if (prop.PropertyValidationCallback != null)
                    {
                        localName = "coercedValue";
                        sb.AppendLine($@"    {prop.PropertyCppName} {localName} = value;");
                        sb.AppendLine($@"    static_cast<{ownerType.Name}*>(this)->{prop.PropertyValidationCallback}({localName});");
                    }
                    sb.AppendLine($@"    static_cast<{ownerType.Name}*>(this)->SetValue(s_{prop.Name}Property, ValueHelper<{prop.PropertyCppName}>::BoxValueIfNecessary({localName}));
    }}
}}");
                    sb.AppendLine(String.Format(@"
{0} {1}Properties::{2}()
{{
    return ValueHelper<{0}>::CastOrUnbox(static_cast<{1}*>(this)->GetValue(s_{2}Property));
}}", prop.PropertyCppName, ownerType.Name, prop.Name));
                }
                else if (prop.AttachedPropertyTargetType != null)
                {
                    sb.AppendLine(String.Format(@"
void {0}Properties::Set{1}({2} const& target, {3} {4}value)
{{
    target.SetValue(s_{1}Property, ValueHelper<{3}>::BoxValueIfNecessary(value));
}}", ownerType.Name, prop.Name, CppName(prop.AttachedPropertyTargetType), prop.PropertyCppName, CppInputModifier(prop.PropertyType)));
                    sb.AppendLine(String.Format(@"
{0} {1}Properties::Get{2}({3} const& target)
{{
    return ValueHelper<{0}>::CastOrUnbox(target.GetValue(s_{2}Property));
}}", prop.PropertyCppName, ownerType.Name, prop.Name, CppName(prop.AttachedPropertyTargetType)));
                }
            }

            // Events
            foreach (var eventInfo in events)
            {
                sb.AppendLine();

                sb.AppendLine(String.Format(
@"winrt::event_token {0}Properties::{1}({2} const& value)
{{
    return {3}.add(value);
}}

void {0}Properties::{1}(winrt::event_token const& token)
{{
    {3}.remove(token);
}}", ownerType.Name, eventInfo.Name, CppName(eventInfo.EventInfo.EventHandlerType), EventFieldName(eventInfo.Name)));
            }

            return sb.ToString();
        }


        static List<Type> GetTypes(string assemblyPath, IList<string> references)
        {
            AppDomain.CurrentDomain.ReflectionOnlyAssemblyResolve += (sender, eventArgs) =>
            {
                Assembly assembly = null;
                string assemblyName = eventArgs.Name.Split(',')[0];

                foreach (var reference in references)
                {
                    var referenceName = System.IO.Path.GetFileNameWithoutExtension(reference);
                    if (referenceName == assemblyName)
                    {
                        assembly = Assembly.ReflectionOnlyLoadFrom(reference);
                        break;
                    }
                }

                return assembly ?? Assembly.ReflectionOnlyLoad(eventArgs.Name);
            };

            WindowsRuntimeMetadata.ReflectionOnlyNamespaceResolve += (sender, eventArgs) =>
            {
                foreach (var refer in references)
                {
                    eventArgs.ResolvedAssemblies.Add(Assembly.ReflectionOnlyLoadFrom(refer));
                }

                return;
            };

            string fullAssemblyPath = Path.GetFullPath(assemblyPath);
            Assembly loadFrom = null;
            foreach (var loadedAssembly in AppDomain.CurrentDomain.GetAssemblies())
            {
                try
                {
                    if (String.Equals(loadedAssembly.Location, fullAssemblyPath, StringComparison.OrdinalIgnoreCase))
                    {
                        loadFrom = loadedAssembly;
                        break;
                    }
                }
                catch // Can't always do this on reflection-only assemblies
                {

                }
            }

            if (loadFrom == null)
            {
                loadFrom = Assembly.ReflectionOnlyLoadFrom(fullAssemblyPath);
            }

            return loadFrom.GetExportedTypes().ToList();
        }

        private void RewriteFileIfNecessary(string path, string contents)
        {
            var fullPath = Utils.RewriteFileIfNecessary(path, contents);

            _pendingFilesWritten.Add(fullPath);
        }
    }
}
