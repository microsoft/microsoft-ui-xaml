// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Microsoft.CSharp;

namespace XamlGen.Templates.ModernIDL
{
    public static class ModernlIdlCodeGeneratorExtensions
    {
        public static string GetDeprecationContractsAttributeString(this DeprecationDefinition deprecation)
        {
            var contracts = deprecation.Contracts;
            if (contracts.Any())
            {
                var sb = new StringBuilder();
                sb.AppendFormat("[deprecated(\"{0}\", deprecate", deprecation.Comment);
                foreach (var contract in contracts)
                {
                    sb.AppendFormat(", {0}, {1}", contract.FullName, contract.Version);
                }
                sb.Append(")]");
                return sb.ToString();
            }

            return string.Empty;
        }
    }

    /// <summary>
    /// Base class for all IDL code generators.
    /// </summary>
    public abstract class ModernIdlCodeGenerator<TModel> : XamlCodeGenerator<TModel>
    {
        protected string DefaultIdlNamespace = Helper.GetRootNamespace();
        public StableXbfIndexGenerator StableIndexes
        {
            get;
            set;
        }

        public bool UndefGetCurrentTime
        {
            get;
            set;
        }

        protected const string Tab = "    ";
        protected void IncludeMember(MemberDefinition member, RequestedInterface requestedInterface, int modelVersion)
        {
            if (member is EventDefinition)
            {
                WriteEvent((EventDefinition)member, requestedInterface);
            }
            else if (member is PropertyDefinition)
            {
                var prop = (PropertyDefinition)member;
                switch (requestedInterface)
                {
                    case RequestedInterface.PublicMembers:
                        WriteProperty(PropertyModel.Public(prop), modelVersion);
                        break;
                    case RequestedInterface.ProtectedMembers:
                        WriteProperty(PropertyModel.Protected(prop), modelVersion);
                        break;
                    case RequestedInterface.VirtualMembers:
                        WriteProperty(PropertyModel.Virtual(prop), modelVersion);
                        break;
                    case RequestedInterface.StaticMembers:
                        WriteProperty(PropertyModel.Static(prop), modelVersion);
                        break;
                }
            }
            else if (member is MethodDefinition)
            {
                var method = (MethodDefinition)member;
                switch (requestedInterface)
                {
                    case RequestedInterface.PublicMembers:
                        WriteMethod(MethodModel.Public(method));
                        break;
                    case RequestedInterface.ProtectedMembers:
                        WriteMethod(MethodModel.Protected(method));
                        break;
                    case RequestedInterface.VirtualMembers:
                        WriteMethod(MethodModel.Virtual(method));
                        break;
                    case RequestedInterface.StaticMembers:
                        WriteMethod(MethodModel.Static(method));
                        break;
                }
            }
            else
            {
                throw new NotSupportedException("Unknown member type");
            }

        }

        protected void WriteImplementedInterfaces(ClassDefinition model)
        {
            if (model.IsInterface)
            {
                // Interfaces are fundamentally different than runtime classes. They can't be properly versioned
                // and so we don't need to tag them with the velocity or contract attributes. They also use the "requires"
                // keyword for any interface that is also required, whereas that is not used for class definitions.
                WriteImplementedInterfacesForInterface(model);
            }
            else
            {
                PushIndent(Tab);
                string separator = model.IdlClassInfo.HasBaseClass ? "," : ":";

                foreach (var version in model.Versions.Select(v => v.GetProjection()).OrderBy(v => v.Version))
                {
                    var modelContract = version.SupportedContracts.GetConcreteContractReference();
                    string velocityString = null;
                    if (Helper.ShouldGenerateContracts())
                    {
                        if (version.VelocityVersion != 0)
                        {
                            velocityString = $"[feature({VelocityFeatures.GetFeatureName(version.VelocityVersion)})]{Environment.NewLine}";
                        }
                    }
                    foreach (var implementedInterface in version.IdlClassInfo.ImplementedInterfaces)
                    {
                        string contractString = null;

                        if (Helper.ShouldGenerateContracts())
                        {
                            var versionContract = version.SupportedContracts.GetConcreteContractReference();
                            if (versionContract != null && model.IdlClassInfo.HasPrimaryInterface)
                            {
                                contractString = $"{versionContract.ToString()}{Environment.NewLine}";
                            }
                        }

                        string defaultString = !model.IdlClassInfo.HasPrimaryInterface ? "[default]" : null;
                        Write($"{separator}{defaultString} {velocityString}{contractString}{implementedInterface.IdlTypeName}{Environment.NewLine}");

                        separator = ",";
                    }
                }
                PopIndent();
            }
        }

        private void WriteImplementedInterfacesForInterface(ClassDefinition interfaceModel)
        {
            string separator = "requires";
            foreach (var implementedInterface in interfaceModel.IdlClassInfo.ImplementedInterfaces)
            {
                Write($"{separator} {implementedInterface.IdlTypeName}");

                separator = ",";
            }

            Write($"{Environment.NewLine}");
        }


        // Write out the parameter list for a constructor as an array of strings
        protected List<string> GetConstructorParameterListAsStrings(ConstructorDefinition ctor)
        {
            List<string> strings;

            if (ctor.Parameters.Any())
            {
                strings = GetParameterListAsStrings(ctor.Parameters);
            }
            else
            {
                strings = new List<string>();
            }

            return strings;
        }

        // Write out the parameter list for a constructor as one long string
        protected string GetConstructorParameterListAsString(ConstructorDefinition ctor)
        {
            List<string> strings = GetConstructorParameterListAsStrings(ctor);
            if (strings.Count == 0)
            {
                return string.Empty;
            }

            var builder = new StringBuilder();
            foreach (var str in strings)
            {
                builder.Append(str);
            }

            return builder.ToString();
        }

        // Write out the parameter list for a method as an array of strings
        protected List<string> GetParameterListAsStrings(IEnumerable<ParameterDefinition> parameters)
        {
            var strings = new List<string>();

            var parameterList = parameters.ToList();
            for (int i = 0; i < parameterList.Count; i++)
            {
                var builder = new StringBuilder();

                builder.Append(parameterList[i].ParameterType.IdlInfo.IdlParameterName);
                if (i < parameterList.Count - 1)
                {
                    builder.Append(", ");
                }

                strings.Add(builder.ToString());
            }

            return strings;
        }

        // Write out an array of strings as a parameter list (including appropriate parens)
        protected void WriteParameters(List<string> parameters)
        {
            if (parameters == null || parameters.Count == 0)
            {
                WriteLine("();");
                return;
            }

            Write($"(");

            for (int i = 0; i < parameters.Count; i++)
            {
                Write($"{parameters[i]}");
            }
            WriteLine(");");
        }

        protected void WriteRuntimeClassAttributes(ClassDefinition classDef)
        {
            WriteAttributes(classDef);

            if (classDef.IdlClassInfo.HasContentProperty)
            {
                WriteLine($"[contentproperty(\"{classDef.ContentProperty.IdlMemberInfo.Name}\")]");
            }

            if (classDef.IdlClassInfo.HasInputProperty)
            {
                WriteLine($"[inputproperty(\"{classDef.InputProperty.IdlMemberInfo.Name}\")]");
            }

            // All our struct helpers have a default empty interface
            bool hasDefaultEmptyInterface = classDef.InitialVersionProjection.IdlClassInfo.HasStructHelperClass || !classDef.InitialVersionProjection.IdlClassInfo.HasPublicInstanceMembers;
            WriteIdlInterfaceNameAttributes(classDef.InitialVersionProjection.IdlClassInfo, hasDefaultEmptyInterface);
        }

        protected void WriteIdlInterfaceNameAttributes(OM.Idl.IdlClassInfo info, bool writeDefault = false)
        {
            if (info.ForceIncludeStaticMembersInterfaceName)
            {
                WriteLine($"[static_name(\"{info.FullStaticMembersInterfaceName}\")]");
            }

            if (info.ForceIncludeFactoryInterfaceName)
            {
                WriteLine($"[constructor_name(\"{info.FullFactoryInterfaceName}\")]");
            }
            if (info.ForceIncludeVirtualMembersInterfaceName)
            {
               WriteLine($"[overridable_name(\"{info.FullVirtualMembersInterfaceName}\")]");
            }

            if (info.ForceIncludeProtectedMembersInterfaceName)
            {
                WriteLine($"[protected_name(\"{info.FullProtectedMembersInterfaceName}\")]");
            }

            if (info.HasPrimaryInterface)
            {
                if (writeDefault)
                {
                    // "default_interface" says "normally, this interface with no methods would be declared as a static-only interface.
                    // However we want you to create an empty interface with no methods for the class"
                    WriteLine("[default_interface]");
                }
                if (info.ForceIncludeInterfaceName)
                {
                    WriteLine($"[interface_name(\"{info.FullInterfaceName}\")]");
                }
            }
        }

        bool TryGetInterfaceString(ClassDefinition model, RequestedInterface requestedInterface, out string value)
        {
            switch (requestedInterface)
            {
                case RequestedInterface.PublicMembers:
                    if (model.Version > 1)
                    {
                        if (model.IdlClassInfo.ForceIncludeInterfaceName)
                        {
                            value = $"[interface_name(\"{model.IdlClassInfo.FullInterfaceName}\")]";
                            return true;
                        }
                    }
                    break;
                case RequestedInterface.ProtectedMembers:
                    if (model.IdlClassInfo.ForceIncludeProtectedMembersInterfaceName)
                    {
                        value = $"[protected_name(\"{model.IdlClassInfo.FullProtectedMembersInterfaceName}\")]";
                        return true;
                    }
                    break;
                case RequestedInterface.VirtualMembers:
                    if (model.IdlClassInfo.ForceIncludeVirtualMembersInterfaceName)
                    {
                        value = $"[overridable_name(\"{model.IdlClassInfo.FullVirtualMembersInterfaceName}\")]";
                        return true;
                    }
                    break;
                case RequestedInterface.StaticMembers:
                    if (model.IdlClassInfo.ForceIncludeStaticMembersInterfaceName)
                    {
                        value = $"[static_name(\"{model.IdlClassInfo.FullStaticMembersInterfaceName}\")]";
                        return true;
                    }
                    break;
                case RequestedInterface.CustomFactories:
                    if (model.IdlClassInfo.ForceIncludeFactoryInterfaceName)
                    {
                        value = $"[constructor_name(\"{model.IdlClassInfo.FullFactoryInterfaceName}\")]";
                        return true;
                    }
                    break;
            }
            value = null;
            return false;
        }

        bool IsOriginalRuntimeClassContractGroup(ClassDefinition model, RequestedInterface requestedInterface)
        {
            // Always have virtual or protected members in their own group. It seems there is a bug with MIDL that doesn't appreciate them
            // being with the other
            if (requestedInterface == RequestedInterface.ProtectedMembers || requestedInterface == RequestedInterface.VirtualMembers)
            {
                return false;
            }
            return model.Version == 1;
        }

        protected void WriteAttributes(TypeDefinition typeInfo)
        {
            if (typeInfo.IdlTypeInfo.IsPrivateIdlOnly)
            {
                bool emitInternal = false;
                if (typeInfo is ContractDefinition)
                {
                    emitInternal = true;
                }
                else
                {
                    if (typeInfo is ClassDefinition)
                    {
                        ClassDefinition definition = typeInfo as ClassDefinition;
                        if (definition.IsInterface &&
                              (definition.AllEvents.Count() == 0) &&
                              (definition.AllProperties.Count() == 0) &&
                              (definition.AllMethods.Count() == 0))
                        {
                            emitInternal = true;
                        }
                    }
                }

                if (emitInternal)
                {
                    WriteLine("[internal]");
                }
            }

            if (typeInfo is ClassDefinition)
            {
                var classInfo = (ClassDefinition)typeInfo;

                // Only write in the contracts if we're generating MUX IDLs
                if (Helper.ShouldGenerateContracts())
                {
                    if (classInfo.IsInterface)
                    {
                        WriteLine(classInfo.SupportedContracts.FirstOrDefault()?.ToString());
                    }
                    else
                    {
                        WriteLine(classInfo.InitialVersionProjection.SupportedContracts.FirstOrDefault()?.ToString());
                    }

                    if (classInfo.VelocityVersion != 0)
                    {
                        WriteLine($"[feature({VelocityFeatures.GetFeatureName(classInfo.VelocityVersion)})]");
                    }
                }
                else
                {
                    // All WinRT types declared in IDL require a version.  Put a bogus one in.
                    WriteLine("[version(1)]");
                }

                // Empty marker interfaces require a GUID
                // See https://docs.microsoft.com/en-us/uwp/midl-3/advanced#empty-interfaces
                if (classInfo.IsInterface &&
                    classInfo.AllEvents.Count() == 0 &&
                    classInfo.AllProperties.Count() == 0 &&
                    classInfo.AllMethods.Count() == 0)
                {
                    if (classInfo.IdlClassInfo.InterfaceGuid == Guid.Empty)
                    {
                        throw new InvalidOperationException($"A guid is required for the empty marker interface {classInfo.Name}. Add a Guid by following the existing patterns in the {nameof(OM.EmptyInterfaceGuids)} class.");
                    }
                    WriteLine($"[uuid({classInfo.IdlClassInfo.InterfaceGuid})]");
                }
                else if (typeof(ProjectionCompatGuids).GetProperty(classInfo.Name) != null)
                {
                    if (classInfo.IsInterface)
                    {
                        WriteLine($"[uuid({classInfo.IdlClassInfo.InterfaceGuid})]");
                    }
                    else
                    {
                        WriteLine($"[interface_name(\"{classInfo.IdlClassInfo.FullInterfaceName}\", {classInfo.IdlClassInfo.InterfaceGuid})]");
                        WriteLine($"[constructor_name(\"{classInfo.IdlClassInfo.FullFactoryInterfaceName}\", {classInfo.IdlClassInfo.FactoryInterfaceGuid})]");
                    }
                }

                WriteDeprecations(typeInfo.Deprecations);
            }
            else if (typeInfo is ContractDefinition)
            {
                var con = (ContractDefinition)typeInfo;
                WriteLine($"[contractversion({con.MaxVersion})]");
            }
            else
            {
                if (Helper.ShouldGenerateContracts())
                {
                    WriteLine(typeInfo.SupportedContracts.OrderBy(con => con.Version).FirstOrDefault()?.ToString());
                }
            }

            if (typeInfo is EnumDefinition)
            {
                var enumInfo = (EnumDefinition)typeInfo;
                if (enumInfo.XamlEnumFlags.AreValuesFlags)
                {
                    WriteLine("[flags]");
                }

                if (enumInfo.VelocityVersion != 0)
                {
                    WriteLine($"[feature({VelocityFeatures.GetFeatureName(enumInfo.VelocityVersion)})]");
                }
            }
            if (typeInfo is AttributeDefinition)
            {
                var attributeInfo = (AttributeDefinition)typeInfo;
                WriteLine($"[attributeusage({attributeInfo.ValidOnString})]");
                if (attributeInfo.AllowMultiple)
                {
                    WriteLine("[allowmultiple]");
                }
                WriteLine($"[attributename(\"{attributeInfo.AttributeName}\")]");
            }
            if (typeInfo is DelegateDefinition delegateDefinition)
            {
                if (!typeInfo.IsImported && !typeInfo.IdlTypeInfo.IsExcluded && typeof(ProjectionCompatGuids).GetProperty(delegateDefinition.Name) != null)
                {
                    WriteLine($"[uuid({delegateDefinition.IdlDelegateInfo.DelegateGuid})]");
                }

            }

            if (typeInfo.IsWebHostHidden)
            {
                WriteLine("[webhosthidden]");
            }
        }

        protected void WriteAttributes(MethodModel method)
        {
            // If a method has a return value, as well as an out param, we need to add the return_name attribute if the method was defined before the conversion to modern idl
            if (method.Member.ReturnType.IsReturnType
                && !method.Member.ReturnType.IsVoid
                && method.Member.DeclaringVersion.DefinedBeforeModernIdl
                && method.Member.Parameters.Any( param => param.ParameterType.IsOut))
            {
                Write($"[return_name(\"{method.Member.ReturnType.Name}\")] ");
            }
            if (method.Member.IsOverloaded)
            {
                if (method.Member.IsDefaultOverload)
                {
                    Write($"[method_name(\"{ method.IdlName}\"), default_overload] ");
                }
                else
                {
                    Write($"[method_name(\"{ method.IdlName}\")] ");
                }
            }

            WriteDeprecations(method.Member.Deprecations);
        }

        protected bool WriteDeprecations(IEnumerable<DeprecationDefinition> deprecations)
        {
            bool wrote = false;
            foreach (var deprecation in deprecations.OrderBy(d => d.Comment + d.Type))
            {
                var deprecationString = deprecation.GetDeprecationContractsAttributeString();
                if (!string.IsNullOrEmpty(deprecationString))
                {
                    WriteLine(deprecationString);
                    wrote = true;
                }
            }

            return wrote;
        }

        protected void WriteAttributes(ConstructorDefinition constructor)
        {
            // Special case Setter constructor. This is the only case where we should ever have an issue
            // with the return name. Since we already shipped this, we need to maintain the same parameter
            // names. Modern IDL by default has the return parameter name of "value", the only issue being
            // that our constructor takes the name "value". No one should ever have a constructor that takes
            // the name "value" again.
            if (constructor.DeclaringClass.Name == "Setter" && !constructor.IsParameterless)
            {
                Write($"[return_name(\"instance\")] ");
            }
            Write($"[method_name(\"{constructor.IdlConstructorInfo.FactoryMethodName}\")] ");

            if (constructor.DeclaringClass.IsAbstract)
            {
                Write("protected ");
            }
        }

        protected void WriteProperty(PropertyModel propModel, int modelVersion)
        {
            if (WriteDeprecations(propModel.Member.Deprecations) && propModel.Member.DeclaringVersion.Version > 1)
            {
                PopIndent();
            }
            if (propModel.RequestedInterface == RequestedInterface.StaticMembers)
            {
                Write("static ");
            }
            else if (propModel.RequestedInterface == RequestedInterface.ProtectedMembers)
            {
                Write("protected ");
            }
            else if (propModel.RequestedInterface == RequestedInterface.VirtualMembers)
            {
                Write("overridable ");
            }

            // Find out if getter or setter are versioned different than the main property.
            if(propModel.Member.VersionedAccessors)
            {
                if (propModel.Member.GetterVersion == modelVersion)
                {
                    WriteLine($"{propModel.Member.GetterReturnType.IdlTypeName} {propModel.IdlName}{{ get; }};");
                }
                else if (propModel.HasSet && propModel.Member.SetterVersion == modelVersion)
                {
                    WriteLine($"{propModel.Member.GetterReturnType.IdlTypeName} {propModel.IdlName}{{ set; }};");
                }
            }
            else
            {
                if (propModel.HasSet)
                {
                    WriteLine($"{propModel.Member.GetterReturnType.IdlTypeName} {propModel.IdlName};");
                }
                else
                {
                    WriteLine($"{propModel.Member.GetterReturnType.IdlTypeName} {propModel.IdlName}{{ get; }};");
                }
            }
        }


        protected void WriteMethod(MethodModel methodModel)
        {
            WriteAttributes(methodModel);

            if (methodModel.Member.ReturnType.IsOut)
            {
                // We have 1 special API that requires this.
                WriteLine($"void {methodModel.Name}(out {methodModel.Member.ReturnType.IdlTypeName} returnValue);");
            }
            else
            {
                Write($"{methodModel.IdlReturnTypeName} {methodModel.Name}");
                WriteParameters(GetParameterListAsStrings(methodModel.Member.Parameters));
            }

        }

        protected void WriteEvent(EventDefinition model, RequestedInterface requestedInterface)
        {
            if (model is AttachedEventDefinition)
            {
                if (requestedInterface != RequestedInterface.StaticMembers)
                {
                    throw new Exception("Attached events must be static");
                }
                WriteMethod(MethodModel.Static(model.GetAddMethod()));
                WriteMethod(MethodModel.Static(model.GetRemoveMethod()));
            }
            else
            {
                WriteDeprecations(model.Deprecations);
                if (requestedInterface == RequestedInterface.StaticMembers)
                {
                    Write("static ");
                }
                WriteLine($"event {model.EventHandlerType.IdlTypeName} {model.IdlMemberInfo.Name};");
            }
        }

        protected void WriteDependencyProperty(DependencyPropertyDefinition dp)
        {
            PushIndent(Tab);
            if (dp.XamlPropertyFlags.IsIndependentlyAnimatable)
            {
                WriteLine("[independentlyanimatable]");
            }
            if (dp.XamlPropertyFlags.IsConditionallyIndependentlyAnimatable)
            {
                WriteLine("[conditionallyindependentlyanimatable]");
            }

            WriteDeprecations(dp.Deprecations);
            WriteLine($"static {Helper.GetRootNamespace()}.DependencyProperty {dp.IdlDPInfo.DPName }{{ get; }};");
            PopIndent();
        }

        protected void WriteAttachedProperty(AttachedPropertyDefinition dp)
        {
            // Write the DP first, we can't change this, otherwise the vtable layout will change
            if (dp.DependencyPropertyModifier == Modifier.Public)
            {
                WriteDependencyProperty(dp);
            }

            PushIndent(Tab);
            WriteLine($"static {dp.GetterReturnType.IdlTypeName} {dp.IdlAPInfo.GetterName}({dp.TargetType.IdlInfo.IdlParameterName});");
            if (!dp.IdlAPInfo.IsReadOnly)
            {
                WriteLine($"static void {dp.IdlAPInfo.SetterName}({dp.TargetType.IdlInfo.IdlParameterName}, {dp.GetterReturnType.IdlInfo.IdlParameterName});");
            }
            PopIndent();
        }

        protected void WriteEnum(EnumValueDefinition value, bool isFlag)
        {
            if (value.Version > 1)
            {
                if (Helper.ShouldGenerateContracts())
                {
                    if (VelocityFeatures.IsVelocityVersion(value.Version))
                    {
                        WriteLine($"[feature({VelocityFeatures.GetFeatureName(value.Version)})]");
                        Write(Tab);
                    }
                    WriteLine(value.SupportedContracts.GetConcreteContractReference().ToString());
                    Write(Tab);
                }
            }
            if (WriteDeprecations(value.Deprecations))
            {
                Write(Tab); // Weird behavior where writing a contract causes the next enum to get pushed back
            }
            if (isFlag && value.Value != 0)
            {
                // Write the enum has: EnumFlag = 0x1
                WriteLine($"{value.Name} = 0x{value.Value:X},");
            }
            else if (!value.IsInOrder)
            {
                // The enum values aren't in order, some jump around so we need to print this value
                WriteLine($"{value.Name} = {value.Value},");
            }
            else
            {
                WriteLine($"{value.Name},");
            }
        }

        protected void WriteContractVersion(MemberDefinition model)
        {
            if (Helper.ShouldGenerateContracts())
            {
                WriteLine(model.SupportedContracts.FirstOrDefault().ToString());
            }
        }

        protected void EnsureEnumValues(EnumDefinition enumDef)
        {
            if (enumDef.Values.Count > 0)
            {
                return;
            }

            if (enumDef.XamlEnumFlags.IsStableTypeIndex && null != StableIndexes)
            {
                int previousIndex = -1; // Start at -1 because 0 is default for first enum
                foreach (StableIndexEntry stableTypeIndex in StableIndexes.GetStableTypeIndexes())
                {
                    if (stableTypeIndex.mappedToKnownType && stableTypeIndex.xamlDirectContractReference != null)
                    {
                        EnumValueDefinition evd = new EnumValueDefinition()
                        {
                            Name = stableTypeIndex.stableIndexName,
                            Value = stableTypeIndex.stableIndex,
                            IsInOrder = stableTypeIndex.stableIndex == (previousIndex + 1),
                            DeclaringType = enumDef,
                        };
                        if (stableTypeIndex.xamlDirectContractReference != null)
                        {
                            evd.SupportedContracts.Add(stableTypeIndex.xamlDirectContractReference);
                            evd.Version = stableTypeIndex.xamlDirectContractReference.Version;
                        }
                        enumDef.Values.Add(evd);
                        previousIndex = stableTypeIndex.stableIndex;
                    }
                }
            }
            else if (enumDef.XamlEnumFlags.IsStablePropertyIndex && null != StableIndexes)
            {
                int previousIndex = -1; // Start at -1 because 0 is default for first enum
                foreach (StableIndexEntry stablePropertyIndex in StableIndexes.GetStablePropertyIndexes())
                {
                    if (stablePropertyIndex.mappedToKnownType && stablePropertyIndex.xamlDirectContractReference != null)
                    {
                        EnumValueDefinition evd = new EnumValueDefinition()
                        {
                            Name = stablePropertyIndex.stableIndexName,
                            Value = stablePropertyIndex.stableIndex,
                            IsInOrder = stablePropertyIndex.stableIndex == (previousIndex + 1),
                            DeclaringType = enumDef,
                        };
                        if (stablePropertyIndex.xamlDirectContractReference != null)
                        {
                            evd.SupportedContracts.Add(stablePropertyIndex.xamlDirectContractReference);
                            evd.Version = stablePropertyIndex.xamlDirectContractReference.Version;
                        }
                        enumDef.Values.Add(evd);
                        previousIndex = stablePropertyIndex.stableIndex;
                    }
                }
            }
            else if (enumDef.XamlEnumFlags.IsStableEventIndex && null != StableIndexes)
            {
                int previousIndex = -1; // Start at -1 because 0 is default for first enum
                foreach (StableIndexEntry stableEventIndex in StableIndexes.GetStableEventIndexes())
                {
                    if (stableEventIndex.mappedToKnownType && stableEventIndex.xamlDirectContractReference != null)
                    {
                        EnumValueDefinition evd = new EnumValueDefinition()
                        {
                            Name = stableEventIndex.stableIndexName,
                            Value = stableEventIndex.stableIndex,
                            IsInOrder = stableEventIndex.stableIndex == (previousIndex + 1),
                            DeclaringType = enumDef,
                        };
                        if (stableEventIndex.xamlDirectContractReference != null)
                        {
                            evd.SupportedContracts.Add(stableEventIndex.xamlDirectContractReference);
                            evd.Version = stableEventIndex.xamlDirectContractReference.Version;
                        }
                        enumDef.Values.Add(evd);
                        previousIndex = stableEventIndex.stableIndex;
                    }
                }
            }
        }

        private bool VersionNeedsNewGroup(ClassDefinition model)
        {
            // Only versions after the first one need a new grouping.
            if (model.Version > 1)
            {
                // Some versions just implement a new interface and don't actually declare anything.
                // If that's the case, there is no need for a new block in the idl
                return model.IdlClassInfo.HasCustomConstructors ||
                       model.IdlClassInfo.HasDefaultConstructor ||
                       model.IdlClassInfo.HasPublicStaticMembers ||
                       model.IdlClassInfo.HasPublicInstanceMembers ||
                       model.IdlClassInfo.HasPrimaryInterface ||
                       model.IdlClassInfo.HasProtectedMembers ||
                       model.IdlClassInfo.HasVirtualMembers;
            }

            return false;
        }

        // Starts a new version of the class.
        protected void StartVersion(ClassDefinition model)
        {
            if (VersionNeedsNewGroup(model))
            {
                Write(Environment.NewLine); // Start a new line for a contract
                PushIndent(Tab);
                if (Helper.ShouldGenerateContracts())
                {
                    WriteLine(model.SupportedContracts.GetConcreteContractReference().ToString());
                }
                if (model.VelocityVersion != 0)
                {
                    WriteLine($"[feature({VelocityFeatures.GetFeatureName(model.VelocityVersion)})]");
                }
                WriteIdlInterfaceNameAttributes(model.IdlClassInfo);
                WriteLine("{");
            }
        }

        protected void EndVersion(ClassDefinition model)
        {
            if (VersionNeedsNewGroup(model))
            {
                WriteLine("}");
                PopIndent();
            }
        }

        protected string ConvertTypeNameForAttribute(string typeName)
        {
            // I have no idea why attributes are the only place we have to do this in modern idl...
            if (typeName == "Windows.UI.Xaml.Interop.TypeName" || typeName == "Microsoft.UI.Xaml.Interop.TypeName")
            {
                return "type";
            }

            return typeName;
        }

        private void WriteMember(MemberDefinition member, RequestedInterface requested, int modelVersion)
        {
            PushIndent(Tab);
            IncludeMember(member, requested, modelVersion);
            PopIndent();
        }

        protected void WriteInterfaceGroup(ClassDefinition model, RequestedInterface requestedInterface)
        {
            // This is how we wrote interfaces since the start of code-gen back in the OS. Changing how this is done
            // would be a breaking change, and is only something we can do between Major releases of WinUI
            var members = GetRequestedProperties(model, requestedInterface);
            foreach (var member in GetRequestedProperties(model, requestedInterface).Where(m => m.OrderHint == null))
            {
                WriteMember(member, requestedInterface, model.Version);
            }

            if (requestedInterface == RequestedInterface.StaticMembers)
            {
                // Include DependencyProperties and AttachedDependencyProperties for static interfaces
               foreach (var member in model.IdlClassInfo.DependencyProperties.Where(m => m.OrderHint == null))
                {
                    if (Math.Min(member.GetterVersion, member.SetterVersion) == model.Version)
                    {
                        WriteDependencyProperty(member);
                    }
                }
                foreach (var member in model.IdlClassInfo.DeclaredAttachedProperties.Where(m => m.OrderHint == null))
                {
                    WriteAttachedProperty(member);
                }
            }

            foreach (var member in GetRequestedEvents(model, requestedInterface).Where(m => m.OrderHint == null))
            {
                WriteMember(member, requestedInterface, model.Version);
            }
            foreach (var member in GetRequestedMethods(model, requestedInterface).Where(m => m.OrderHint == null))
            {
                WriteMember(member, requestedInterface, model.Version);
            }

            if (requestedInterface == RequestedInterface.StaticMembers)
            {
                // Note these don't order by order hint, IDK why, but that was always how it was done and has to stay that way.
                foreach (var member in model.IdlClassInfo.DependencyProperties.Where(m => m.OrderHint != null))
                {
                    WriteDependencyProperty(member);
                }
                foreach (var member in model.IdlClassInfo.DeclaredAttachedProperties.Where(m => m.OrderHint != null))
                {
                    WriteAttachedProperty(member);
                }
            }

            foreach (var member in GetRequestedPMEs(model,requestedInterface).Where(m => m.OrderHint != null).OrderBy(m => m.OrderHint))
            {
                WriteMember(member, requestedInterface, model.Version);
            }
        }


        private IEnumerable<PropertyDefinition> GetRequestedProperties(ClassDefinition model, RequestedInterface requestedInterface)
        {
            switch (requestedInterface)
            {
                case RequestedInterface.PublicMembers:
                    return model.IdlClassInfo.PublicInstanceProperties;
                case RequestedInterface.ProtectedMembers:
                    return model.IdlClassInfo.ProtectedProperties;
                case RequestedInterface.VirtualMembers:
                    return model.IdlClassInfo.VirtualProperties;
                case RequestedInterface.StaticMembers:
                    return model.IdlClassInfo.StaticProperties;
            }

            throw new Exception($"RequestedInterface type {nameof(requestedInterface)} not supported");
        }
        private IEnumerable<EventDefinition> GetRequestedEvents(ClassDefinition model, RequestedInterface requestedInterface)
        {
            switch (requestedInterface)
            {
                case RequestedInterface.PublicMembers:
                    return model.IdlClassInfo.PublicInstanceEvents;
                case RequestedInterface.ProtectedMembers:
                    return model.IdlClassInfo.ProtectedEvents;
                case RequestedInterface.VirtualMembers:
                    return model.IdlClassInfo.VirtualEvents;
                case RequestedInterface.StaticMembers:
                    return model.IdlClassInfo.StaticEvents;
            }

            throw new Exception($"RequestedInterface type {nameof(requestedInterface)} not supported");
        }
        private IEnumerable<MethodDefinition> GetRequestedMethods(ClassDefinition model, RequestedInterface requestedInterface)
        {
            switch (requestedInterface)
            {
                case RequestedInterface.PublicMembers:
                    return model.IdlClassInfo.PublicInstanceMethods;
                case RequestedInterface.ProtectedMembers:
                    return model.IdlClassInfo.ProtectedMethods;
                case RequestedInterface.VirtualMembers:
                    return model.IdlClassInfo.VirtualMethods;
                case RequestedInterface.StaticMembers:
                    return model.IdlClassInfo.StaticMethods;
            }

            throw new Exception($"RequestedInterface type {nameof(requestedInterface)} not supported");
        }
        private IEnumerable<MemberDefinition> GetRequestedPMEs(ClassDefinition model, RequestedInterface requestedInterface)
        {
            switch (requestedInterface)
            {
                case RequestedInterface.PublicMembers:
                    return model.IdlClassInfo.PublicInstancePMEs;
                case RequestedInterface.ProtectedMembers:
                    return model.IdlClassInfo.ProtectedPMEs;
                case RequestedInterface.VirtualMembers:
                    return model.IdlClassInfo.VirtualPMEs;
                case RequestedInterface.StaticMembers:
                    return model.IdlClassInfo.StaticPMEs;
            }

            throw new Exception($"RequestedInterface type {nameof(requestedInterface)} not supported");
        }

    }
}
