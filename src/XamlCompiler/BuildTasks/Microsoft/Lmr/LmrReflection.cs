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
    /// For any other object (e.g. a runtime type), these forward to the reflection member (runtime types go through
    /// <see cref="KnownRuntimeTypes"/> first), so the behavior of the compiler doesn't change at all.
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
            return type.GetInterface(name);
        }

        public static Type GetInterfaceLmr(this Type type, string name, bool ignoreCase)
        {
            if (type is LmrTypeBase lmrType)
            {
                return lmrType.GetInterfaceCore(name, ignoreCase);
            }
            return type.GetInterface(name, ignoreCase);
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
            return type.InvokeMember(name, invokeAttr, binder, target, args, modifiers, culture, namedParameters);
        }

        #endregion

        #region Type: type algebra

        public static Type MakeGenericTypeLmr(this Type type, params Type[] typeArguments)
        {
            if (type is LmrTypeBase lmrType)
            {
                return lmrType.MakeGenericTypeCore(typeArguments);
            }
            return type.MakeGenericType(typeArguments);
        }

        public static Type MakeArrayTypeLmr(this Type type)
        {
            if (type is LmrTypeBase lmrType)
            {
                return lmrType.MakeArrayTypeCore();
            }
            return type.MakeArrayType();
        }

        public static Type MakeArrayTypeLmr(this Type type, int rank)
        {
            if (type is LmrTypeBase lmrType)
            {
                return lmrType.MakeArrayTypeCore(rank);
            }
            return type.MakeArrayType(rank);
        }

        #endregion

        #region Methods

        public static MethodInfo MakeGenericMethodLmr(this MethodInfo method, params Type[] typeArguments)
        {
            if (method is LmrMethodInfoBase lmrMethod)
            {
                return lmrMethod.MakeGenericMethodCore(typeArguments);
            }
            return method.MakeGenericMethod(typeArguments);
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
            return method.GetMethodBody();
        }

        #endregion

        #region Assembly

        public static Type[] GetTypesLmr(this Assembly assembly)
        {
            if (assembly is LmrAssemblyBase lmrAssembly)
            {
                return lmrAssembly.GetTypesCore();
            }
            return assembly.GetTypes();
        }

        public static Type[] GetExportedTypesLmr(this Assembly assembly)
        {
            if (assembly is LmrAssemblyBase lmrAssembly)
            {
                return lmrAssembly.GetExportedTypesCore();
            }
            return assembly.GetExportedTypes();
        }

        public static Type GetTypeLmr(this Assembly assembly, string name)
        {
            if (assembly is LmrAssemblyBase lmrAssembly)
            {
                return lmrAssembly.GetTypeCore(name);
            }
            return assembly.GetType(name);
        }

        public static Type GetTypeLmr(this Assembly assembly, string name, bool throwOnError)
        {
            if (assembly is LmrAssemblyBase lmrAssembly)
            {
                return lmrAssembly.GetTypeCore(name, throwOnError);
            }
            return assembly.GetType(name, throwOnError);
        }

        public static Type GetTypeLmr(this Assembly assembly, string name, bool throwOnError, bool ignoreCase)
        {
            if (assembly is LmrAssemblyBase lmrAssembly)
            {
                return lmrAssembly.GetTypeCore(name, throwOnError, ignoreCase);
            }
            return assembly.GetType(name, throwOnError, ignoreCase);
        }

        public static AssemblyName[] GetReferencedAssembliesLmr(this Assembly assembly)
        {
            if (assembly is LmrAssemblyBase lmrAssembly)
            {
                return lmrAssembly.GetReferencedAssembliesCore();
            }
            return assembly.GetReferencedAssemblies();
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
            return assembly.Location;
        }

        public static string GetCodeBaseLmr(this Assembly assembly)
        {
            if (assembly is LmrAssemblyBase lmrAssembly)
            {
                return lmrAssembly.CodeBaseCore;
            }
            return assembly.CodeBase;
        }

        public static FileStream GetFileLmr(this Assembly assembly, string name)
        {
            if (assembly is LmrAssemblyBase lmrAssembly)
            {
                return lmrAssembly.GetFileCore(name);
            }
            return assembly.GetFile(name);
        }

        public static FileStream[] GetFilesLmr(this Assembly assembly)
        {
            if (assembly is LmrAssemblyBase lmrAssembly)
            {
                return lmrAssembly.GetFilesCore();
            }
            return assembly.GetFiles();
        }

        public static FileStream[] GetFilesLmr(this Assembly assembly, bool getResourceModules)
        {
            if (assembly is LmrAssemblyBase lmrAssembly)
            {
                return lmrAssembly.GetFilesCore(getResourceModules);
            }
            return assembly.GetFiles(getResourceModules);
        }

        #endregion

        #region Module

        public static Type[] GetTypesLmr(this Module module)
        {
            if (module is MetadataOnlyModule lmrModule)
            {
                return lmrModule.GetTypesCore();
            }
            return module.GetTypes();
        }

        public static Type GetTypeLmr(this Module module, string className)
        {
            if (module is MetadataOnlyModule lmrModule)
            {
                return lmrModule.GetTypeCore(className, false, false);
            }
            return module.GetType(className);
        }

        public static Type GetTypeLmr(this Module module, string className, bool ignoreCase)
        {
            if (module is MetadataOnlyModule lmrModule)
            {
                return lmrModule.GetTypeCore(className, false, ignoreCase);
            }
            return module.GetType(className, ignoreCase);
        }

        public static Type GetTypeLmr(this Module module, string className, bool throwOnError, bool ignoreCase)
        {
            if (module is MetadataOnlyModule lmrModule)
            {
                return lmrModule.GetTypeCore(className, throwOnError, ignoreCase);
            }
            return module.GetType(className, throwOnError, ignoreCase);
        }

        public static Type ResolveTypeLmr(this Module module, int metadataToken)
        {
            if (module is MetadataOnlyModule lmrModule)
            {
                return lmrModule.ResolveTypeCore(metadataToken, null, null);
            }
            return module.ResolveType(metadataToken);
        }

        public static Type ResolveTypeLmr(this Module module, int metadataToken, Type[] genericTypeArguments, Type[] genericMethodArguments)
        {
            if (module is MetadataOnlyModule lmrModule)
            {
                return lmrModule.ResolveTypeCore(metadataToken, genericTypeArguments, genericMethodArguments);
            }
            return module.ResolveType(metadataToken, genericTypeArguments, genericMethodArguments);
        }

        public static MethodBase ResolveMethodLmr(this Module module, int metadataToken)
        {
            if (module is MetadataOnlyModule lmrModule)
            {
                return lmrModule.ResolveMethodCore(metadataToken, null, null);
            }
            return module.ResolveMethod(metadataToken);
        }

        public static MethodBase ResolveMethodLmr(this Module module, int metadataToken, Type[] genericTypeArguments, Type[] genericMethodArguments)
        {
            if (module is MetadataOnlyModule lmrModule)
            {
                return lmrModule.ResolveMethodCore(metadataToken, genericTypeArguments, genericMethodArguments);
            }
            return module.ResolveMethod(metadataToken, genericTypeArguments, genericMethodArguments);
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
            return module.Name;
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
            return module.FullyQualifiedName;
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

        #endregion
    }
}
