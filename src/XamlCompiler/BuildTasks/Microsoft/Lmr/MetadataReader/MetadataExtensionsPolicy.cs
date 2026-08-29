// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.Globalization;
using System.Reflection.Adds;
using System.Reflection.Metadata;
using System.Diagnostics;
using Debug=Microsoft.UI.Xaml.Markup.Compiler.Lmr.Internal.Debug;
using System.Linq;

using System.Reflection;  

namespace Microsoft.UI.Xaml.Markup.Compiler.Lmr
{    
    /// <summary>
    /// Callback object to encapsulate information that's exposed in reflection but not explicitly defined in
    /// the metadata. This could be due to members injected by the loader, or impedence mismatch between reflection
    /// object model vs. metadata object model.
    /// This provides a centralized location for such hooks in the reflection implementation.
    /// 
    /// Possible things controlled by this:
    /// - Pseudo members on Arrays
    /// - Pseudo custom attributes
    /// - whether we hide Transparent Proxy 
    /// - whether we expose  __ComObject as a base class
    /// 
    /// </summary>
    internal interface IMetadataExtensionsPolicy
    {   
        /// <summary>
        /// An array T[] inherits from System.Array so it implements those interfaces, but the loader
        /// may add additional interfaces too. This returns any additional interfaces not already on System.Array.
        /// </summary>
        /// <param name="elementType">type of the array</param>
        /// <returns>non-null array of types (may be 0-length) of unique interfaces. </returns>
        Type[] GetExtraArrayInterfaces(Type elementType);

       
        /// <summary>
        /// Get the rutime-added methods on an array.
        /// See Ecma IIb 14.2. The runtime adds extra methods to an array type which aren't represented in the metadata.
        /// For example, for CLR 1.0, these include:
        ///   T Get(...)
        ///   Void Set(..., T)
        ///   T& Address(...)
        ///   
        /// This does not include the methods from runtime explicitly added interfaces (via GetExtraArrayInterfaces)
        /// </summary>
        /// <param name="arrayType">the array type. This should be an array type and will serve as the declaring type for the returned methods</param>
        /// <returns>a set of MethodInfos for the array helpers.</returns>
        MethodInfo[] GetExtraArrayMethods(Type arrayType);

        /// <summary>
        /// Get the rutime-added constructors on an array.
        /// Constructor counterpart to GetExtraArrayMethods
        /// Since Clr 1.0, these include:
        ///     .ctor(...)
        ///     .ctor(... ...)
        /// </summary>
        /// <param name="arrayType">the array type. This should be an array type and will serve as the declaring type for the returned methods</param>
        /// <returns>a set of ConstructorInfos for the array.</returns>
        ConstructorInfo[] GetExtraArrayConstructors(Type arrayType);

        /// <summary>
        /// Get a ParameterInfo based only from the method signature. Use this when there is no parameter metadata available. 
        /// Many of the ParameterInfo properties will return arbitrary (usually 0) values. This is
        /// considered policy because these arbitrary values are not represented in the metadata.
        /// </summary>
        /// <param name="member">method containing the parameter </param>
        /// <param name="paramType">type of the parameter</param>
        /// <param name="position">0-based position of the parameter</param>
        /// <returns>a ParameterInfo with the supplied properties. </returns>
        ParameterInfo GetFakeParameterInfo(MemberInfo member, Type paramType, int position);

        /// <summary>
        /// Gets list of pseudo-custom attributes on an object.
        /// </summary>
        /// <param name="module">Module in which object is valid.</param>
        /// <param name="handle">EntityHandle representing an object we want PCAs on.</param>
        /// <returns>List of pseudo-custom attributes on a given object.</returns>
        /// <remarks>This is considered policy since list of PCAs depends on the runtime version.
        /// Extracting PCAs is also expensive so hosts might want to skip this steps if they
        /// don't need any of PCAs.</remarks>
        IEnumerable<CustomAttributeData> GetPseudoCustomAttributes(MetadataOnlyModule module, EntityHandle handle);


       
        /// <summary>
        ///  Try to resolve a type name which may be forwarded. 
        /// </summary>
        /// <param name="assembly">assembly that containing the TypeForwardedTo attribute</param>
        /// <param name="fullname">full name of type, as specified in the attribute</param>
        /// <param name="ignoreCase">true for a case in-sensitive comparison of fullname against the type name in each attribute, false for a case-sensitive comparsion.</param>
        /// <returns>Null if the type name is not specified in a type forwareded attribute</returns>
        Type TryTypeForwardResolution(MetadataOnlyAssembly assembly, string fullname, bool ignoreCase);
    }


    /// <summary>
    /// Policy to implement semantics from .Net 2.0
    /// </summary>
    internal class MetadataExtensionsPolicy20 : IMetadataExtensionsPolicy
    {
        protected ITypeUniverse m_universe;

        public MetadataExtensionsPolicy20(ITypeUniverse u)
        {
            Debug.Assert(u != null);
            m_universe = u;
        }

        /// <summary>
        /// With generics, arrays implemenent generic interfaces.          
        /// </summary>
        public virtual  Type[] GetExtraArrayInterfaces(Type elementType)
        {
            // All arrays T[] implicitly inherit from System.Array. But the System.Array implements a static
            // set of interfaces, and so it can implement a generalized IEnumerable, but not IEnumerable<T>. 
            // The CLR loader adds the IEnum<T> interface to T[]. Since T[] doesn't have a TypeDef, this is
            // not represented in the metadata.

            // CLR does not add the following interfaces when the element type is a pointer type.
            if (elementType.IsPointer) {
                return Type.EmptyTypes;
            }

            Type[] typeArgs = { elementType };

            Type[] interfaces = new Type[3] { 
                m_universe.GetTypeXFromName("System.Collections.Generic.IList`1").MakeGenericType(typeArgs),
                m_universe.GetTypeXFromName("System.Collections.Generic.ICollection`1").MakeGenericType(typeArgs),
                m_universe.GetTypeXFromName("System.Collections.Generic.IEnumerable`1").MakeGenericType(typeArgs),
            };

            return interfaces;
        }

                
        public virtual MethodInfo[] GetExtraArrayMethods(Type arrayType)
        {
            Debug.Assert(arrayType != null);
            Debug.Assert(arrayType.IsArray);

            return new MethodInfo[] { 
                new ArrayFabricatedGetMethodInfo(arrayType),
                new ArrayFabricatedSetMethodInfo(arrayType),
                new ArrayFabricatedAddressMethodInfo(arrayType),
            };

        }

        public virtual ConstructorInfo[] GetExtraArrayConstructors(Type arrayType)
        {
            int rank = arrayType.GetArrayRank();

            return new ConstructorInfo[] {
                new ArrayFabricatedConstructorInfo(arrayType, rank),
                new ArrayFabricatedConstructorInfo(arrayType, rank * 2),
            };
        }

        public virtual ParameterInfo GetFakeParameterInfo(MemberInfo member, Type paramType, int position)
        {
            return new SimpleParameterInfo(member, paramType, position);
        }

        /// <summary>
        /// Only returns TypeForwardedToAttribute and SerializableAttribute currently.
        /// </summary>
        public virtual IEnumerable<CustomAttributeData> GetPseudoCustomAttributes(MetadataOnlyModule module, EntityHandle handle)
        {
            List<CustomAttributeData> result = new List<CustomAttributeData>();
            IEnumerable<CustomAttributeData> typeForwardedAttributes = PseudoCustomAttributes.GetTypeForwardedToAttributes(module, handle);
            if (typeForwardedAttributes.Any())
            {
                result.AddRange(typeForwardedAttributes);
            }

            CustomAttributeData attribute = PseudoCustomAttributes.GetSerializableAttribute(module, handle);
            if (attribute != null)
            {
                result.Add(attribute);
            }

            return result;
        }

        /// <summary>
        /// Inspects TypeForwardedTo attributes on the assembly and if the name matches (using a case comparsion specified by ignoreCase)
        /// resolves it. If names don't match no resolution is performed.
        /// A TypeForwardTo attribute looks like:
        ///   [assembly: TypeForwardedTo(typeof(Widget))]
        /// </summary>
        public virtual Type TryTypeForwardResolution(MetadataOnlyAssembly assembly, string fullname, bool ignoreCase)
        {
            // Query the attributes with the fullname of the type.
            UnresolvedTypeName utn = PseudoCustomAttributes.GetRawTypeForwardedToAttribute(assembly, fullname, ignoreCase);

            if (utn != null)
            {
                // We found one, resolve it...
                // ConvertToType can throw if the type's assembly wasn't resolvable.  That isn't necessarily an error unless the type is actually used later
                // in markup, so don't fail out.
                Type t = null;
                try
                {
                    t = utn.ConvertToType(assembly.TypeUniverse, assembly.ManifestModuleInternal);
                }
                catch (UnresolvedAssemblyException)
                { }
                catch (TypeLoadException)
                { }
                return t;
            }

            return null;
        }
    }
}
