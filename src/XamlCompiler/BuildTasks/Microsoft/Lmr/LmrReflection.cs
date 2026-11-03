// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

using System;
using System.Globalization;
using System.IO;
using System.Reflection;

namespace Microsoft.UI.Xaml.Markup.Compiler.Lmr
{
    /// <summary>
    /// Reflection over LMR objects (types, assemblies, modules and methods read from metadata) without going through the
    /// members of <see cref="Type"/>, <see cref="Assembly"/>, <see cref="Module"/> and <see cref="MethodBase"/> that are
    /// annotated for trimming, Native AOT and single-file.
    /// </summary>
    /// <remarks>
    /// <para>
    /// The XAML compiler reflects over the types of the projects it compiles, which are LMR types, never runtime types: those
    /// annotations (and the warnings of the analyzers about them) don't apply to them. Each method here does the same thing as the
    /// member of the same name without the "Lmr" suffix (including the argument validation done by the non-virtual overloads of
    /// <see cref="Type"/>), but on an LMR object it calls the unannotated "*Core" implementation (see <see cref="LmrTypeBase"/>,
    /// <see cref="LmrAssemblyBase"/>, <see cref="MetadataOnlyModule"/>, <see cref="LmrMethodInfoBase"/> and <see cref="LmrConstructorInfoBase"/>).
    /// The names are different from the names of the reflection members on purpose: instance members always win over extension
    /// methods in overload resolution.
    /// </para>
    /// <para>
    /// For any other object (e.g. a runtime type), these forward to the reflection member on .NET Framework, so the
    /// behavior there doesn't change at all. In the Native AOT build (.NET 10) that would need the annotations: there, the
    /// members of <see cref="Type"/> only work on the closed set of runtime types in <c>KnownRuntimeTypes</c> (the
    /// built-in types of the XAML language), and everything else throws <see cref="NotSupportedException"/>.
    /// </para>
    /// </remarks>
    internal static class LmrReflection
    {
        // The same as Type.DefaultLookup
        private const BindingFlags DefaultLookup = BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public;

        #region Type: constructors

        public static ConstructorInfo[] GetConstructorsLmr(this Type type)
        {
            if (type is LmrTypeBase lmrType)
            {
                return lmrType.GetConstructorsCore(BindingFlags.Public | BindingFlags.Instance);
            }
            return KnownRuntimeTypes.Get(type, "Type.GetConstructors").GetConstructors();
        }

        public static ConstructorInfo[] GetConstructorsLmr(this Type type, BindingFlags bindingAttr)
        {
            if (type is LmrTypeBase lmrType)
            {
                return lmrType.GetConstructorsCore(bindingAttr);
            }
            return KnownRuntimeTypes.Get(type, "Type.GetConstructors").GetConstructors(bindingAttr);
        }

        public static ConstructorInfo GetConstructorLmr(this Type type, Type[] types)
        {
            if (type is LmrTypeBase)
            {
                return GetConstructorLmr(type, BindingFlags.Public | BindingFlags.Instance, null, CallingConventions.Any, types, null);
            }
            return KnownRuntimeTypes.Get(type, "Type.GetConstructor").GetConstructor(types);
        }

        public static ConstructorInfo GetConstructorLmr(this Type type, BindingFlags bindingAttr, Binder binder, Type[] types, ParameterModifier[] modifiers)
        {
            if (type is LmrTypeBase)
            {
                return GetConstructorLmr(type, bindingAttr, binder, CallingConventions.Any, types, modifiers);
            }
            return KnownRuntimeTypes.Get(type, "Type.GetConstructor").GetConstructor(bindingAttr, binder, types, modifiers);
        }

        public static ConstructorInfo GetConstructorLmr(this Type type, BindingFlags bindingAttr, Binder binder, CallingConventions callConvention, Type[] types, ParameterModifier[] modifiers)
        {
            if (type is LmrTypeBase lmrType)
            {
                ValidateTypes(types);
                return lmrType.GetConstructorImplCore(bindingAttr, binder, callConvention, types, modifiers);
            }
            return KnownRuntimeTypes.Get(type, "Type.GetConstructor").GetConstructor(bindingAttr, binder, callConvention, types, modifiers);
        }

        #endregion

        #region Type: methods

        public static MethodInfo[] GetMethodsLmr(this Type type)
        {
            if (type is LmrTypeBase lmrType)
            {
                return lmrType.GetMethodsCore(DefaultLookup);
            }
            return KnownRuntimeTypes.Get(type, "Type.GetMethods").GetMethods();
        }

        public static MethodInfo[] GetMethodsLmr(this Type type, BindingFlags bindingAttr)
        {
            if (type is LmrTypeBase lmrType)
            {
                return lmrType.GetMethodsCore(bindingAttr);
            }
            return KnownRuntimeTypes.Get(type, "Type.GetMethods").GetMethods(bindingAttr);
        }

        public static MethodInfo GetMethodLmr(this Type type, string name)
        {
            if (type is LmrTypeBase)
            {
                return GetMethodLmr(type, name, DefaultLookup);
            }
            return KnownRuntimeTypes.Get(type, "Type.GetMethod").GetMethod(name);
        }

        public static MethodInfo GetMethodLmr(this Type type, string name, BindingFlags bindingAttr)
        {
            if (type is LmrTypeBase lmrType)
            {
                ValidateName(name);
                return lmrType.GetMethodImplCore(name, bindingAttr, null, CallingConventions.Any, null, null);
            }
            return KnownRuntimeTypes.Get(type, "Type.GetMethod").GetMethod(name, bindingAttr);
        }

        public static MethodInfo GetMethodLmr(this Type type, string name, Type[] types)
        {
            if (type is LmrTypeBase)
            {
                return GetMethodLmr(type, name, DefaultLookup, null, CallingConventions.Any, types, null);
            }
            return KnownRuntimeTypes.Get(type, "Type.GetMethod").GetMethod(name, types);
        }

        public static MethodInfo GetMethodLmr(this Type type, string name, BindingFlags bindingAttr, Binder binder, Type[] types, ParameterModifier[] modifiers)
        {
            if (type is LmrTypeBase)
            {
                return GetMethodLmr(type, name, bindingAttr, binder, CallingConventions.Any, types, modifiers);
            }
            return KnownRuntimeTypes.Get(type, "Type.GetMethod").GetMethod(name, bindingAttr, binder, types, modifiers);
        }

        public static MethodInfo GetMethodLmr(this Type type, string name, BindingFlags bindingAttr, Binder binder, CallingConventions callConvention, Type[] types, ParameterModifier[] modifiers)
        {
            if (type is LmrTypeBase lmrType)
            {
                ValidateName(name);
                ValidateTypes(types);
                return lmrType.GetMethodImplCore(name, bindingAttr, binder, callConvention, types, modifiers);
            }
            return KnownRuntimeTypes.Get(type, "Type.GetMethod").GetMethod(name, bindingAttr, binder, callConvention, types, modifiers);
        }

        #endregion

        #region Type: properties

        public static PropertyInfo[] GetPropertiesLmr(this Type type)
        {
            if (type is LmrTypeBase lmrType)
            {
                return lmrType.GetPropertiesCore(DefaultLookup);
            }
            return KnownRuntimeTypes.Get(type, "Type.GetProperties").GetProperties();
        }

        public static PropertyInfo[] GetPropertiesLmr(this Type type, BindingFlags bindingAttr)
        {
            if (type is LmrTypeBase lmrType)
            {
                return lmrType.GetPropertiesCore(bindingAttr);
            }
            return KnownRuntimeTypes.Get(type, "Type.GetProperties").GetProperties(bindingAttr);
        }

        public static PropertyInfo GetPropertyLmr(this Type type, string name)
        {
            if (type is LmrTypeBase)
            {
                return GetPropertyLmr(type, name, DefaultLookup);
            }
            return KnownRuntimeTypes.Get(type, "Type.GetProperty").GetProperty(name);
        }

        public static PropertyInfo GetPropertyLmr(this Type type, string name, BindingFlags bindingAttr)
        {
            if (type is LmrTypeBase lmrType)
            {
                ValidateName(name);
                return lmrType.GetPropertyImplCore(name, bindingAttr, null, null, null, null);
            }
            return KnownRuntimeTypes.Get(type, "Type.GetProperty").GetProperty(name, bindingAttr);
        }

        public static PropertyInfo GetPropertyLmr(this Type type, string name, Type returnType)
        {
            if (type is LmrTypeBase lmrType)
            {
                ValidateName(name);
                if (returnType == null)
                {
                    throw new ArgumentNullException(nameof(returnType));
                }
                return lmrType.GetPropertyImplCore(name, DefaultLookup, null, returnType, null, null);
            }
            return KnownRuntimeTypes.Get(type, "Type.GetProperty").GetProperty(name, returnType);
        }

        public static PropertyInfo GetPropertyLmr(this Type type, string name, BindingFlags bindingAttr, Binder binder, Type returnType, Type[] types, ParameterModifier[] modifiers)
        {
            if (type is LmrTypeBase lmrType)
            {
                ValidateName(name);
                if (types == null)
                {
                    throw new ArgumentNullException(nameof(types));
                }
                return lmrType.GetPropertyImplCore(name, bindingAttr, binder, returnType, types, modifiers);
            }
            return KnownRuntimeTypes.Get(type, "Type.GetProperty").GetProperty(name, bindingAttr, binder, returnType, types, modifiers);
        }

        #endregion

        #region Type: fields

        public static FieldInfo[] GetFieldsLmr(this Type type)
        {
            if (type is LmrTypeBase lmrType)
            {
                return lmrType.GetFieldsCore(DefaultLookup);
            }
            return KnownRuntimeTypes.Get(type, "Type.GetFields").GetFields();
        }

        public static FieldInfo[] GetFieldsLmr(this Type type, BindingFlags bindingAttr)
        {
            if (type is LmrTypeBase lmrType)
            {
                return lmrType.GetFieldsCore(bindingAttr);
            }
            return KnownRuntimeTypes.Get(type, "Type.GetFields").GetFields(bindingAttr);
        }

        public static FieldInfo GetFieldLmr(this Type type, string name)
        {
            if (type is LmrTypeBase lmrType)
            {
                return lmrType.GetFieldCore(name, DefaultLookup);
            }
            return KnownRuntimeTypes.Get(type, "Type.GetField").GetField(name);
        }

        public static FieldInfo GetFieldLmr(this Type type, string name, BindingFlags bindingAttr)
        {
            if (type is LmrTypeBase lmrType)
            {
                return lmrType.GetFieldCore(name, bindingAttr);
            }
            return KnownRuntimeTypes.Get(type, "Type.GetField").GetField(name, bindingAttr);
        }

        #endregion

        #region Type: events

        public static EventInfo[] GetEventsLmr(this Type type)
        {
            if (type is LmrTypeBase lmrType)
            {
                return lmrType.GetEventsCore(DefaultLookup);
            }
            return KnownRuntimeTypes.Get(type, "Type.GetEvents").GetEvents();
        }

        public static EventInfo[] GetEventsLmr(this Type type, BindingFlags bindingAttr)
        {
            if (type is LmrTypeBase lmrType)
            {
                return lmrType.GetEventsCore(bindingAttr);
            }
            return KnownRuntimeTypes.Get(type, "Type.GetEvents").GetEvents(bindingAttr);
        }

        public static EventInfo GetEventLmr(this Type type, string name)
        {
            if (type is LmrTypeBase lmrType)
            {
                return lmrType.GetEventCore(name, DefaultLookup);
            }
            return KnownRuntimeTypes.Get(type, "Type.GetEvent").GetEvent(name);
        }

        public static EventInfo GetEventLmr(this Type type, string name, BindingFlags bindingAttr)
        {
            if (type is LmrTypeBase lmrType)
            {
                return lmrType.GetEventCore(name, bindingAttr);
            }
            return KnownRuntimeTypes.Get(type, "Type.GetEvent").GetEvent(name, bindingAttr);
        }

        #endregion

        #region Type: nested types

        public static Type[] GetNestedTypesLmr(this Type type)
        {
            if (type is LmrTypeBase lmrType)
            {
                return lmrType.GetNestedTypesCore(BindingFlags.Public);
            }
            return KnownRuntimeTypes.Get(type, "Type.GetNestedTypes").GetNestedTypes();
        }

        public static Type[] GetNestedTypesLmr(this Type type, BindingFlags bindingAttr)
        {
            if (type is LmrTypeBase lmrType)
            {
                return lmrType.GetNestedTypesCore(bindingAttr);
            }
            return KnownRuntimeTypes.Get(type, "Type.GetNestedTypes").GetNestedTypes(bindingAttr);
        }

        public static Type GetNestedTypeLmr(this Type type, string name)
        {
            if (type is LmrTypeBase lmrType)
            {
                return lmrType.GetNestedTypeCore(name, BindingFlags.Public);
            }
            return KnownRuntimeTypes.Get(type, "Type.GetNestedType").GetNestedType(name);
        }

        public static Type GetNestedTypeLmr(this Type type, string name, BindingFlags bindingAttr)
        {
            if (type is LmrTypeBase lmrType)
            {
                return lmrType.GetNestedTypeCore(name, bindingAttr);
            }
            return KnownRuntimeTypes.Get(type, "Type.GetNestedType").GetNestedType(name, bindingAttr);
        }

        #endregion

        #region Type: interfaces

        public static Type[] GetInterfacesLmr(this Type type)
        {
            if (type is LmrTypeBase lmrType)
            {
                return lmrType.GetInterfacesCore();
            }
            return KnownRuntimeTypes.Get(type, "Type.GetInterfaces").GetInterfaces();
        }

        public static Type GetInterfaceLmr(this Type type, string name)
        {
            if (type is LmrTypeBase lmrType)
            {
                return lmrType.GetInterfaceCore(name, false);
            }
            // Not supported on the known runtime types either: the compiler never needs it, and that keeps Type.GetInterface
            // unused in the native binary (see LmrTypeBase.GetInterface).
            throw NotLmr(type, "Type.GetInterface");
        }

        public static Type GetInterfaceLmr(this Type type, string name, bool ignoreCase)
        {
            if (type is LmrTypeBase lmrType)
            {
                return lmrType.GetInterfaceCore(name, ignoreCase);
            }
            // See GetInterfaceLmr(Type, string)
            throw NotLmr(type, "Type.GetInterface");
        }

        #endregion

        #region Type: members

        public static MemberInfo[] GetMembersLmr(this Type type)
        {
            if (type is LmrTypeBase lmrType)
            {
                return lmrType.GetMembersCore(DefaultLookup);
            }
            return KnownRuntimeTypes.Get(type, "Type.GetMembers").GetMembers();
        }

        public static MemberInfo[] GetMembersLmr(this Type type, BindingFlags bindingAttr)
        {
            if (type is LmrTypeBase lmrType)
            {
                return lmrType.GetMembersCore(bindingAttr);
            }
            return KnownRuntimeTypes.Get(type, "Type.GetMembers").GetMembers(bindingAttr);
        }

        public static MemberInfo[] GetMemberLmr(this Type type, string name)
        {
            if (type is LmrTypeBase lmrType)
            {
                return lmrType.GetMemberCore(name, MemberTypes.All, DefaultLookup);
            }
            return KnownRuntimeTypes.Get(type, "Type.GetMember").GetMember(name);
        }

        public static MemberInfo[] GetMemberLmr(this Type type, string name, BindingFlags bindingAttr)
        {
            if (type is LmrTypeBase lmrType)
            {
                return lmrType.GetMemberCore(name, MemberTypes.All, bindingAttr);
            }
            return KnownRuntimeTypes.Get(type, "Type.GetMember").GetMember(name, bindingAttr);
        }

        public static MemberInfo[] GetMemberLmr(this Type type, string name, MemberTypes memberType, BindingFlags bindingAttr)
        {
            if (type is LmrTypeBase lmrType)
            {
                return lmrType.GetMemberCore(name, memberType, bindingAttr);
            }
            return KnownRuntimeTypes.Get(type, "Type.GetMember").GetMember(name, memberType, bindingAttr);
        }

        public static MemberInfo[] GetDefaultMembersLmr(this Type type)
        {
            if (type is LmrTypeBase lmrType)
            {
                return lmrType.GetDefaultMembersCore();
            }
            return KnownRuntimeTypes.Get(type, "Type.GetDefaultMembers").GetDefaultMembers();
        }

        public static object InvokeMemberLmr(this Type type, string name, BindingFlags invokeAttr, Binder binder, object target, object[] args, ParameterModifier[] modifiers, CultureInfo culture, string[] namedParameters)
        {
            if (type is LmrTypeBase lmrType)
            {
                return lmrType.InvokeMemberCore(name, invokeAttr, binder, target, args, modifiers, culture, namedParameters);
            }
            throw NotLmr(type, "Type.InvokeMember");
        }

        #endregion

        #region Type: type algebra

        public static Type MakeGenericTypeLmr(this Type type, params Type[] typeArguments)
        {
            if (type is LmrTypeBase lmrType)
            {
                return lmrType.MakeGenericTypeCore(typeArguments);
            }
            throw NotLmr(type, "Type.MakeGenericType");
        }

        public static Type MakeArrayTypeLmr(this Type type)
        {
            if (type is LmrTypeBase lmrType)
            {
                return lmrType.MakeArrayTypeCore();
            }
            throw NotLmr(type, "Type.MakeArrayType");
        }

        public static Type MakeArrayTypeLmr(this Type type, int rank)
        {
            if (type is LmrTypeBase lmrType)
            {
                return lmrType.MakeArrayTypeCore(rank);
            }
            throw NotLmr(type, "Type.MakeArrayType");
        }

        #endregion

        #region Methods

        public static MethodInfo MakeGenericMethodLmr(this MethodInfo method, params Type[] typeArguments)
        {
            if (method is LmrMethodInfoBase lmrMethod)
            {
                return lmrMethod.MakeGenericMethodCore(typeArguments);
            }
            throw NotLmr(method, "MethodInfo.MakeGenericMethod");
        }

        public static MethodBody GetMethodBodyLmr(this MethodBase method)
        {
            if (method is LmrMethodInfoBase lmrMethod)
            {
                return lmrMethod.GetMethodBodyCore();
            }
            if (method is LmrConstructorInfoBase lmrConstructor)
            {
                return lmrConstructor.GetMethodBodyCore();
            }
            throw NotLmr(method, "MethodBase.GetMethodBody");
        }

        #endregion

        #region Assembly

        public static Type[] GetTypesLmr(this Assembly assembly)
        {
            if (assembly is LmrAssemblyBase lmrAssembly)
            {
                return lmrAssembly.GetTypesCore();
            }
            throw NotLmr(assembly, "Assembly.GetTypes");
        }

        public static Type[] GetExportedTypesLmr(this Assembly assembly)
        {
            if (assembly is LmrAssemblyBase lmrAssembly)
            {
                return lmrAssembly.GetExportedTypesCore();
            }
            throw NotLmr(assembly, "Assembly.GetExportedTypes");
        }

        public static Type GetTypeLmr(this Assembly assembly, string name)
        {
            if (assembly is LmrAssemblyBase lmrAssembly)
            {
                return lmrAssembly.GetTypeCore(name);
            }
            throw NotLmr(assembly, "Assembly.GetType");
        }

        public static Type GetTypeLmr(this Assembly assembly, string name, bool throwOnError)
        {
            if (assembly is LmrAssemblyBase lmrAssembly)
            {
                return lmrAssembly.GetTypeCore(name, throwOnError);
            }
            throw NotLmr(assembly, "Assembly.GetType");
        }

        public static Type GetTypeLmr(this Assembly assembly, string name, bool throwOnError, bool ignoreCase)
        {
            if (assembly is LmrAssemblyBase lmrAssembly)
            {
                return lmrAssembly.GetTypeCore(name, throwOnError, ignoreCase);
            }
            throw NotLmr(assembly, "Assembly.GetType");
        }

        public static AssemblyName[] GetReferencedAssembliesLmr(this Assembly assembly)
        {
            if (assembly is LmrAssemblyBase lmrAssembly)
            {
                return lmrAssembly.GetReferencedAssembliesCore();
            }
            throw NotLmr(assembly, "Assembly.GetReferencedAssemblies");
        }

        /// <summary>
        /// The same as <see cref="Assembly.Location"/>: the path of the metadata file of an LMR assembly.
        /// </summary>
        public static string GetLocationLmr(this Assembly assembly)
        {
            if (assembly is LmrAssemblyBase lmrAssembly)
            {
                return lmrAssembly.LocationCore;
            }
            throw NotLmr(assembly, "Assembly.Location");
        }

        public static string GetCodeBaseLmr(this Assembly assembly)
        {
            if (assembly is LmrAssemblyBase lmrAssembly)
            {
                return lmrAssembly.CodeBaseCore;
            }
            throw NotLmr(assembly, "Assembly.CodeBase");
        }

        public static FileStream GetFileLmr(this Assembly assembly, string name)
        {
            if (assembly is LmrAssemblyBase lmrAssembly)
            {
                return lmrAssembly.GetFileCore(name);
            }
            throw NotLmr(assembly, "Assembly.GetFile");
        }

        public static FileStream[] GetFilesLmr(this Assembly assembly)
        {
            if (assembly is LmrAssemblyBase lmrAssembly)
            {
                return lmrAssembly.GetFilesCore();
            }
            throw NotLmr(assembly, "Assembly.GetFiles");
        }

        public static FileStream[] GetFilesLmr(this Assembly assembly, bool getResourceModules)
        {
            if (assembly is LmrAssemblyBase lmrAssembly)
            {
                return lmrAssembly.GetFilesCore(getResourceModules);
            }
            throw NotLmr(assembly, "Assembly.GetFiles");
        }

        #endregion

        #region Module

        public static Type[] GetTypesLmr(this Module module)
        {
            if (module is MetadataOnlyModule lmrModule)
            {
                return lmrModule.GetTypesCore();
            }
            throw NotLmr(module, "Module.GetTypes");
        }

        public static Type GetTypeLmr(this Module module, string className)
        {
            if (module is MetadataOnlyModule lmrModule)
            {
                return lmrModule.GetTypeCore(className, false, false);
            }
            throw NotLmr(module, "Module.GetType");
        }

        public static Type GetTypeLmr(this Module module, string className, bool ignoreCase)
        {
            if (module is MetadataOnlyModule lmrModule)
            {
                return lmrModule.GetTypeCore(className, false, ignoreCase);
            }
            throw NotLmr(module, "Module.GetType");
        }

        public static Type GetTypeLmr(this Module module, string className, bool throwOnError, bool ignoreCase)
        {
            if (module is MetadataOnlyModule lmrModule)
            {
                return lmrModule.GetTypeCore(className, throwOnError, ignoreCase);
            }
            throw NotLmr(module, "Module.GetType");
        }

        public static Type ResolveTypeLmr(this Module module, int metadataToken)
        {
            if (module is MetadataOnlyModule lmrModule)
            {
                return lmrModule.ResolveTypeCore(metadataToken, null, null);
            }
            throw NotLmr(module, "Module.ResolveType");
        }

        public static Type ResolveTypeLmr(this Module module, int metadataToken, Type[] genericTypeArguments, Type[] genericMethodArguments)
        {
            if (module is MetadataOnlyModule lmrModule)
            {
                return lmrModule.ResolveTypeCore(metadataToken, genericTypeArguments, genericMethodArguments);
            }
            throw NotLmr(module, "Module.ResolveType");
        }

        public static MethodBase ResolveMethodLmr(this Module module, int metadataToken)
        {
            if (module is MetadataOnlyModule lmrModule)
            {
                return lmrModule.ResolveMethodCore(metadataToken, null, null);
            }
            throw NotLmr(module, "Module.ResolveMethod");
        }

        public static MethodBase ResolveMethodLmr(this Module module, int metadataToken, Type[] genericTypeArguments, Type[] genericMethodArguments)
        {
            if (module is MetadataOnlyModule lmrModule)
            {
                return lmrModule.ResolveMethodCore(metadataToken, genericTypeArguments, genericMethodArguments);
            }
            throw NotLmr(module, "Module.ResolveMethod");
        }

        /// <summary>
        /// The same as <see cref="Module.Name"/>: the file name of the metadata file of an LMR module.
        /// </summary>
        public static string GetNameLmr(this Module module)
        {
            if (module is MetadataOnlyModule lmrModule)
            {
                return lmrModule.NameCore;
            }
            throw NotLmr(module, "Module.Name");
        }

        /// <summary>
        /// The same as <see cref="Module.FullyQualifiedName"/>: the path of the metadata file of an LMR module.
        /// </summary>
        public static string GetFullyQualifiedNameLmr(this Module module)
        {
            if (module is MetadataOnlyModule lmrModule)
            {
                return lmrModule.FullyQualifiedNameCore;
            }
            throw NotLmr(module, "Module.FullyQualifiedName");
        }

        #endregion

        #region Helpers

        // The same validation as the non-virtual overloads of Type.GetMethod/GetProperty/GetConstructor.
        private static void ValidateName(string name)
        {
            if (name == null)
            {
                throw new ArgumentNullException(nameof(name));
            }
        }

        private static void ValidateTypes(Type[] types)
        {
            if (types == null)
            {
                throw new ArgumentNullException(nameof(types));
            }
            for (int i = 0; i < types.Length; i++)
            {
                if (types[i] == null)
                {
                    throw new ArgumentNullException(nameof(types));
                }
            }
        }

        private static Exception NotLmr(object value, string member)
        {
            if (value == null)
            {
                // What calling the reflection member on null does
                return new NullReferenceException();
            }

            return new NotSupportedException(string.Format(CultureInfo.InvariantCulture,
                "{0} isn't supported on '{1}' ({2}) in the Native AOT build of the XAML compiler, which can only reflect over the types and assemblies it reads from metadata.",
                member, value, value.GetType().FullName));
        }

        #endregion
    }
}
