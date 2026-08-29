// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.Globalization;
using System.Reflection.Adds;
using System.Reflection.Metadata;
using System.Text;
using Debug=Microsoft.UI.Xaml.Markup.Compiler.Lmr.Internal.Debug;

using System.Reflection;  

namespace Microsoft.UI.Xaml.Markup.Compiler.Lmr
{
    /// <summary>
    /// Utility class that parses metadata for information about pseudo-custom attributes.
    /// </summary>
    internal static class PseudoCustomAttributes
    {
        public const string TypeForwardedToAttributeName = "System.Runtime.CompilerServices.TypeForwardedToAttribute";
        public const string SerializableAttributeName = "System.SerializableAttribute";

                
        /// <summary>
        /// Get the TypeForwardedToAttributes on an assembly.
        /// </summary>
        /// <param name="assembly">assembly to look for attributes on.</param>
        /// <returns>List of CustomAttributeData instances describing TypeForwardedToAttributes if present.
        /// Empty list if TypeForwardedToAttribute is not present.</returns>
        /// <remarks>
        /// TypeForwardedTo attributes only occur on the assembly. So this overload is useful when we're explicitly operating with the assembly and
        /// looking for these attributes.
        /// </remarks>
        public static IEnumerable<CustomAttributeData> GetTypeForwardedToAttributes(MetadataOnlyAssembly assembly)
        {
            return GetTypeForwardedToAttributes(assembly.ManifestModuleInternal);
        }

        /// <summary>
        /// Finds info about TypeForwardedToAttributes if present.
        /// </summary>
        /// <param name="module">Module in which a given handle is valid.</param>
        /// <param name="handle">EntityHandle representing object that's target of attributes. Must be an AssemblyDefinition.</param>
        /// <returns>List of CustomAttributeData instances describing TypeForwardedToAttributes if present.
        /// Empty list if TypeForwardedToAttribute is not present.</returns>
        public static IEnumerable<CustomAttributeData> GetTypeForwardedToAttributes(MetadataOnlyModule manifestModule, EntityHandle handle)
        {
            // TypeForwardedToAttribute's only target is Assembly object.
            if (handle.Kind != HandleKind.AssemblyDefinition)
            {
                return new CustomAttributeData[0]; // empty
            }
            return GetTypeForwardedToAttributes(manifestModule);
        }

        public static IEnumerable<CustomAttributeData> GetTypeForwardedToAttributes(MetadataOnlyModule manifestModule)
        {
            ITypeUniverse itu = manifestModule.AssemblyResolver;

            // Get reflection objects for TypeForwardToAttribute. Cache this outside the loop.
            // Since these refer to types defined in mscorlib, these will be typerefs for any other module.
            // The only constructor argument is of type System.Type. 
            Type argumentType = itu.GetBuiltInType(CorElementType.Type);

            // Get constructor for TypeForwardedToAttribute.
            Assembly systemAssembly = itu.GetSystemAssembly();
            Type attributeType = systemAssembly.GetType(TypeForwardedToAttributeName, false, false);

            // If system assembly doesn't have TypeForwardedTo defined then there is no support
            // for type forwarding.
            if (attributeType == null) yield break;

            // Get the raw TypeForwardedTo data from the metadata.
            IEnumerable<UnresolvedTypeName> raw = GetRawTypeForwardedToAttributes(manifestModule);

            foreach (UnresolvedTypeName utn in raw)
            {
                ConstructorInfo[] constructors = attributeType.GetConstructors();
                Debug.Assert(constructors.Length == 1, "TypeForwardedToAttribute should have only one constructor.");

                //ConvertToType can throw if the type's assembly wasn't resolvable.  In that case we just don't add its TypeForwardedTo attribute.
                Type argumentValue = null;
                try
                {
                    argumentValue = utn.ConvertToType(itu, manifestModule);
                }
                catch (UnresolvedAssemblyException)
                { }
                catch (TypeLoadException)
                { }

                if (argumentValue != null)
                {
                    // package the raw TypeForwardTo data as a custom attribute to follow the reflection API protocols.
                    CustomAttributeTypedArgument forwardedTypeInfo = new CustomAttributeTypedArgument(argumentType, argumentValue);

                    List<CustomAttributeTypedArgument> typedArguments = new List<CustomAttributeTypedArgument>(1);
                    typedArguments.Add(forwardedTypeInfo);

                    List<CustomAttributeNamedArgument> namedArguments = new List<CustomAttributeNamedArgument>(0);

                    MetadataOnlyCustomAttributeData attribute = new MetadataOnlyCustomAttributeData(
                        constructors[0],
                        typedArguments,
                        namedArguments);

                    yield return attribute;
                }
            }
        }

        /// <summary>
        /// Convenience overload for assemblies.
        /// </summary>        
        internal static IEnumerable<UnresolvedTypeName> GetRawTypeForwardedToAttributes(MetadataOnlyAssembly assembly)
        {
            return GetRawTypeForwardedToAttributes(assembly.ManifestModuleInternal);
        }

        /// <summary>
        /// Enumerate type forwarders. This provides the raw metadata and explicitly avoids doing any resolution.
        /// Returns UnresolvedTypeNames instead of Type to avoid doing an eager resolution.
        /// </summary>
        /// <param name="manifestModule">the manifest module to search. TypeForward data is only on a manifest module</param>
        /// <returns>enumerate of TypeForward data in the module.</returns>
        internal static IEnumerable<UnresolvedTypeName> GetRawTypeForwardedToAttributes(MetadataOnlyModule manifestModule)
        {
            var reader = manifestModule.RawReader;
            foreach (var exportedTypeHandle in reader.ExportedTypes)
            {
                var exportedType = reader.GetExportedType(exportedTypeHandle);
                if (exportedType.Implementation.Kind == HandleKind.AssemblyReference)
                {
                    // This is a type forwarder
                    string typeName = reader.GetString(exportedType.Name);
                    string ns = reader.GetString(exportedType.Namespace);
                    string fullName = string.IsNullOrEmpty(ns) ? typeName : ns + "." + typeName;
                    
                    var assemblyRefHandle = (AssemblyReferenceHandle)exportedType.Implementation;
                    AssemblyName assemblyName = AssemblyNameHelper.GetAssemblyNameFromRef(assemblyRefHandle, manifestModule);
                    yield return new UnresolvedTypeName(fullName, assemblyName);
                }
            }
        }


        /// <summary>
        /// Convenience overload for assemblies.
        /// </summary>        
        internal static UnresolvedTypeName GetRawTypeForwardedToAttribute(MetadataOnlyAssembly assembly, string fullname, bool ignoreCase)
        {
            return GetRawTypeForwardedToAttribute(assembly.ManifestModuleInternal, fullname, ignoreCase);
        }

        /// <summary>
        /// Enumerate type forwarders and finds one that matches given full name. 
        /// Returns UnresolvedTypeNames instead of Type to avoid doing an eager resolution.
        /// </summary>
        /// <param name="manifestModule">The module to search in.</param>
        /// <param name="fullname">Type's full name.</param>
        /// <param name="ignoreCase">If true, the fullname comparison will be not be case sensitive.</param>
        /// <returns>An unresolved type retrieved from type forwarded attributes or null if type with given name 
        /// cannot be found.</returns>
        /// <remarks>
        /// Having this version of API saves us from converting StringBuilder content to string when we are only looking
        /// for one specific forwarder.
        /// </remarks>
        internal static UnresolvedTypeName GetRawTypeForwardedToAttribute(MetadataOnlyModule manifestModule, string fullname, bool ignoreCase)
        {
            if (string.IsNullOrEmpty(fullname))
            {
                return null;
            }

            var reader = manifestModule.RawReader;
            foreach (var exportedTypeHandle in reader.ExportedTypes)
            {
                var exportedType = reader.GetExportedType(exportedTypeHandle);
                if (exportedType.Implementation.Kind == HandleKind.AssemblyReference)
                {
                    string typeName = reader.GetString(exportedType.Name);
                    string ns = reader.GetString(exportedType.Namespace);
                    string typeFullName = string.IsNullOrEmpty(ns) ? typeName : ns + "." + typeName;

                    if (!Utility.Compare(typeFullName, fullname, ignoreCase))
                    {
                        continue;
                    }

                    var assemblyRefHandle = (AssemblyReferenceHandle)exportedType.Implementation;
                    AssemblyName assemblyName = AssemblyNameHelper.GetAssemblyNameFromRef(assemblyRefHandle, manifestModule);
                    return new UnresolvedTypeName(typeFullName, assemblyName);
                }
            }

            return null;
        }


        /// <summary>
        /// Given the representation of a TypeForwarededToAttribute, get the Type parameter from it.
        /// </summary>
        /// <param name="data">a custom attribute representation for a TypeForwardedAttribute</param>
        /// <returns>the System.Type parameter stored in the attribute. This encapsulates where the type is forwarded to.</returns>
        /// <remarks>
        /// This is the inverse on GetTypeForwardedToAttributes.
        /// Given an attribute of: [assembly: TypeForwardedTo(typeof(Widget))]
        /// This returns Typeof(Widget).
        /// </remarks>
        public static Type GetTypeFromTypeForwardToAttribute(CustomAttributeData data)
        {
            Debug.Assert(data.Constructor.DeclaringType.FullName.Equals(TypeForwardedToAttributeName, StringComparison.Ordinal));
            CustomAttributeTypedArgument argument = data.ConstructorArguments[0];
            Type result = (Type)argument.Value;
            return result; 
        }


        /// <summary>
        /// Finds info about SerializableAttribute if present on a type.
        /// </summary>
        /// <param name="module">Module in which a given handle is valid.</param>
        /// <param name="handle">EntityHandle representing object that's target of attribute.</param>
        /// <returns>CustomAttributeData instance describing SerializableAttribute if present.
        /// Null otherwise.</returns>
        public static CustomAttributeData GetSerializableAttribute(MetadataOnlyModule module, EntityHandle handle)
        {
            // SerializableAttribute's only target is TypeDef object.
            if (handle.Kind != HandleKind.TypeDefinition)
            {
                return null;
            }

            var typeDef = module.RawReader.GetTypeDefinition((TypeDefinitionHandle)handle);
            TypeAttributes flags = typeDef.Attributes;

            // If flags do not contain Serializable, the attribute is not present.
            if ((flags & TypeAttributes.Serializable) == 0)
            {
                return null;
            }

            // Type is serializable, construct attribute data. 
            return GetSerializableAttribute(module, /*isRequired*/ false);
        }

        /// <summary>
        /// Creates attribute data for SerializableAttribute.
        /// </summary>
        /// <param name="module">Module to be used to get appropriate type universe and system assembly.</param>
        /// <param name="isRequired">If true, SerializableAttribute must be present in the system assembly (mscorlib).
        /// If false, returns null if SerializableAttribute cannot be found.
        /// </param>
        internal static CustomAttributeData GetSerializableAttribute(MetadataOnlyModule module, bool isRequired)
        {
            // Get constructor for SerializableAttribute(). No args.
            Assembly systemAssembly = module.AssemblyResolver.GetSystemAssembly();
            Type attributeType = systemAssembly.GetType(SerializableAttributeName, isRequired, false);
            if (attributeType == null)
            {
                return null;
            }

            ConstructorInfo[] constructors = attributeType.GetConstructors();
            Debug.Assert(constructors.Length == 1, "SerializableAttribute should have only one constructor.");

            var typedArguments = new List<CustomAttributeTypedArgument>(0);
            var namedArguments = new List<CustomAttributeNamedArgument>(0);

            MetadataOnlyCustomAttributeData result = new MetadataOnlyCustomAttributeData(
                constructors[0], typedArguments, namedArguments);

            return result;
        }
    }
}
