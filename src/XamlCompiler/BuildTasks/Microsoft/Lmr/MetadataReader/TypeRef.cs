// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

using Debug = Microsoft.UI.Xaml.Markup.Compiler.Lmr.Internal.Debug;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System;
using System.Text;
using System.Globalization;
using System.Reflection.Adds;
using System.Reflection.Metadata;
using System.Reflection.Metadata.Ecma335;
using System.Diagnostics;

using System.Reflection;  

namespace Microsoft.UI.Xaml.Markup.Compiler.Lmr
{
    /// <summary>
    /// TypeReference (TypeRef row in metadata).
    /// This implements the TypeReference interface to allow getting TypeRef specific data.
    /// It also implements the regular Type interface, and then forwards on that to the resolved type.     
    /// The base implementations will call these derived properties which will resolve the proxy.
    /// </summary>
    [DebuggerDisplay(@"\{Name = {Name} FullName = {FullName}\}")]
    internal class MetadataOnlyTypeReference : TypeProxy, ITypeReference
    {
        // A TypeReference handle. This is the primary key.
        // This is valid in the this.m_resolver scope.
        readonly TypeReferenceHandle m_typeRef;

        /// <summary>
        /// Encapsulate a type reference from a metadata scope. 
        /// </summary>
        /// <param name="resolver">metadata scope that the typeref is in (this is likely not the 
        /// scope that the type will actually resolve to.</param>
        /// <param name="typeRef">TypeReference handle for the typeref.</param>
        public MetadataOnlyTypeReference(MetadataOnlyModule resolver, TypeReferenceHandle typeRef)
            : base(resolver)
        {
            m_typeRef = typeRef;
        }

       

        #region ITypeReference Members

        // Resolves the type, invoking callbacks into the Assembly resolver if needed.
        // This can fail / throw, but should not return null.
        protected override Type GetResolvedTypeWorker()
        {
            // TypeRef handles aren't generic. If this had instantiated generic, 
            // args, it would have been a TypeSpec instead of a TypeRef.

            return m_resolver.ResolveTypeRef(this);
        }

        public Module DeclaringScope
        {
            get { return m_resolver; }
        }

        public TypeReferenceHandle TypeRefHandle
        {
            get { return m_typeRef; }
        }

        public EntityHandle ResolutionScope
        {
            get
            {
                var typeRef = m_resolver.RawReader.GetTypeReference(m_typeRef);
                return typeRef.ResolutionScope;
            }
        }

        // Return just the name (which includes namespace), does not return enclosing types.
        public virtual string RawName
        {
            get
            {
                var typeRef = m_resolver.RawReader.GetTypeReference(m_typeRef);
                string ns = m_resolver.RawReader.GetString(typeRef.Namespace);
                string name = m_resolver.RawReader.GetString(typeRef.Name);
                if (string.IsNullOrEmpty(ns))
                    return name;
                return ns + "." + name;
            }
        }

        // We can get names without resolving.
        public override string Namespace
        {
            get
            {
                if (DeclaringType != null)
                {
                    return DeclaringType.Namespace;
                }
                return Utility.GetNamespaceHelper(FullName);
            }
        }
        public override string Name
        {
            get
            {
                return Utility.GetTypeNameFromFullNameHelper(FullName, IsNested);
            }
        }

        // Returns the full name. Note that the TypeRef property includes the fullname, so we can get that
        // without having to resolve. 
        // This also implements the Type.FullName. Normally Type implementations require resolving.
        public override string FullName
        {
            get
            {
                // Walk the nesting chain without resolution
                TypeReferenceHandle current = m_typeRef;
                string name = String.Empty;
                string suffix = String.Empty;

                while (true)
                {
                    var typeRef = m_resolver.RawReader.GetTypeReference(current);
                    string ns = m_resolver.RawReader.GetString(typeRef.Namespace);
                    string n = m_resolver.RawReader.GetString(typeRef.Name);
                    string rawName = string.IsNullOrEmpty(ns) ? n : ns + "." + n;

                    EntityHandle scope = typeRef.ResolutionScope;

                    // Nested TypeRefs classes can only be nested in other TypeRefs.
                    if (scope.Kind == HandleKind.TypeReference)
                    {
                        suffix = "+" + rawName + suffix;
                        current = (TypeReferenceHandle)scope;
                        continue;
                    }

                    name = rawName + suffix;
                    break;
                }

                return name;
            }
        }

       

        #endregion


        /// <summary>
        /// Get the assembly name that this TypeRef requests to be resolved to.
        /// </summary>
        private AssemblyName RequestedAssemblyName
        {
            get
            {
                var scope = this.ResolutionScope;
                switch (scope.Kind)
                {
                    case HandleKind.TypeReference:
                        // TypeRef is nested in another TypeRef. Both are in the same assembly, so 
                        // get the assembly name for the outer type ref.
                        var tr = new MetadataOnlyTypeReference(this.m_resolver, (TypeReferenceHandle)scope);
                        return tr.RequestedAssemblyName;

                    case HandleKind.AssemblyReference:
                        // TypeRef resolves to another assembly. Get the assembly name from the metadata.
                        AssemblyName an = this.m_resolver.GetAssemblyNameFromAssemblyRef((AssemblyReferenceHandle)scope);
                        return an;

                    // TypeRef resolves to the same assembly that it's declared in.
                    case HandleKind.ModuleReference:
                    case HandleKind.ModuleDefinition:
                        return this.m_resolver.Assembly.GetName();

                    default:
                        // The Ecma spec states that a typeRef scope handle must be one
                        // of the types listed above. If it's something else, then this likely means corrupted metadata. 
                        Debug.Assert(false, "Unexpected resolution scope kind");
                        throw new InvalidOperationException(Resources.InvalidMetadata);
                }
            }
        }


        // Override to get a proxy object without resolution.
        public override Assembly Assembly
        {
            get
            {
                AssemblyName name = this.RequestedAssemblyName;
                Assembly a = new AssemblyRef(name, this.TypeUniverse);
                return a;
            }
        }


        // Override to get the AQN without doing resolution.
        public override string AssemblyQualifiedName
        {
            get
            {
                string aqn = this.RequestedAssemblyName.ToString();
                string t = this.FullName;
                return System.Reflection.Assembly.CreateQualifiedName(aqn, t);
            }
        }


        // We know that a TypeRef is not a composite type (like a ByRef, Array, Pointer, etc)
        // We could have a TypeRef to a generic type (eg, IEnumerable`1)
        protected override bool IsByRefImpl()
        {
            return false;
        }
        protected override bool IsArrayImpl()
        {
            return false;
        }
        public override bool IsGenericParameter
        {
            get
            {
                return false;
            }
        }
        protected override bool IsPointerImpl()
        {
            return false;
        }

        protected override bool IsPrimitiveImpl()
        {
            // Primitive types can only occur in the system assembly - avoid resolution by checking
            // for that case first.
            if (!TypeUniverse.WouldResolveToAssembly(RequestedAssemblyName, TypeUniverse.GetSystemAssembly()))
                return false;

            // This is in the system assembly so must not be a problem to resolve
            return GetResolvedType().IsPrimitive;
        }

        // Override to avoid resolution.
        public override Type DeclaringType
        {
            get
            {
                // If the type is nested, then the resolution scope will be a TypeReference.
                var scope = this.ResolutionScope;
                if (scope.Kind == HandleKind.TypeReference)
                {
                    return this.m_resolver.Factory.CreateTypeRef(m_resolver, (TypeReferenceHandle)scope);
                }
                return null;
            }
        }

    } // end class TypeRef


    /// <summary>
    /// SignatureTypeReference (TypeRef handle that occurs in a signature).
    /// This is a special case of TypeReference in which we know the raw type kind
    /// (CLASS vs VALUETYPE) from the signature encoding.
    /// </summary>
    /// <remarks>
    /// The rawTypeKind uses SRM SignatureTypeKind values: 0=unknown, 18=class, 17=valuetype.
    /// This is useful to satisfy some requests without resolving the type.
    /// </remarks>
    [DebuggerDisplay(@"\{Name = {Name} FullName = {FullName} RawTypeKind = {m_rawTypeKind}\}")]
    internal class MetadataOnlySignatureTypeReference : MetadataOnlyTypeReference
    {
        /// <summary>
        /// Encapsulate a type reference from a metadata scope. 
        /// </summary>
        /// <param name="resolver">metadata scope that the typeref is in (this is likely not the 
        /// scope that the type will actually resolve to.</param>
        /// <param name="typeRef">TypeReference handle for the typeref.</param>
        /// <param name="rawTypeKind">raw type kind from the signature (SignatureTypeKind: 0=unknown, 18=class, 17=valuetype)</param>
        public MetadataOnlySignatureTypeReference(MetadataOnlyModule resolver, TypeReferenceHandle typeRef, byte rawTypeKind)
            : base(resolver, typeRef)
        {
            // rawTypeKind: 0=unknown, or a SignatureTypeKind value (Class=18, ValueType=17)
            Debug.Assert(rawTypeKind == 0 || rawTypeKind == (byte)SignatureTypeKind.Class || rawTypeKind == (byte)SignatureTypeKind.ValueType);
            m_rawTypeKind = rawTypeKind;
        }

        protected override bool IsValueTypeImpl()
        {
            if (m_rawTypeKind == (byte)SignatureTypeKind.ValueType)
                return true;
            if (m_rawTypeKind == (byte)SignatureTypeKind.Class)
                return false;
            // Unknown: fall back to resolution
            return base.IsValueTypeImpl();
        }

        private readonly byte m_rawTypeKind;
    }


    /// <summary>
    /// Represents raw data from the metadata, prior to being converted into the reflection object model. 
    /// This class is useful for representing typerefs that are encoded as strings names, such as how TypeRefs 
    /// are stored in custom attributes.
    /// 
    /// This is semantically equivalent to a TypeRef, but this form guarantees we don't do any eager resolution.
    /// 
    /// This is converted to a System.Type by parsing it.
    /// </summary>
    [DebuggerDisplay("{m_TypeName},{m_AssemblyName}")]
    internal class UnresolvedTypeName
    {
        // Full type name (excluding assembly) which can be passed to a type parser.
        readonly private string m_TypeName;

        // Assembly name that the type name resides in. 
        readonly private AssemblyName m_AssemblyName;

        /// <summary>
        /// Constructor 
        /// </summary>
        /// <param name="typeName">The full type name, which will eventually be parsed.</param>
        /// <param name="assemblyName">the assembly that the type will be resolved to.</param>
        public UnresolvedTypeName(string typeName, AssemblyName assemblyName)
        {
            Debug.Assert(!string.IsNullOrEmpty(typeName));
            Debug.Assert(assemblyName != null);

            m_TypeName = typeName;
            m_AssemblyName = assemblyName;
        }


        private static Microsoft.UI.Xaml.Markup.Compiler.Core.InstanceCache<AssemblyName, string> _assemblyNameCache = new Microsoft.UI.Xaml.Markup.Compiler.Core.InstanceCache<AssemblyName, string>();

        /// <summary>
        /// Parse the name to convert to a reflection System.Type.
        /// </summary>
        /// <param name="universe">type universe that resulting type will be valid in.</param>
        /// <param name="moduleContext">module that the typeName was obtained from. This is passed to the parser
        /// and may be required to disambiguate the type name. See type name parser for details.</param>
        /// <returns>a System.Type for the given type.</returns>
        public Type ConvertToType(ITypeUniverse universe, Module moduleContext)
        {
            string assemblyTypeName = null;
            if (!_assemblyNameCache.TryGetValue(m_AssemblyName, out assemblyTypeName))
            {
                assemblyTypeName = m_AssemblyName.FullName;
                _assemblyNameCache[m_AssemblyName] = assemblyTypeName;
            }

            string fullTypeName = string.Format(CultureInfo.InvariantCulture, "{0},{1}", m_TypeName, assemblyTypeName);

            // This requires resolution.
            Type type = TypeNameParser.ParseTypeName(universe, moduleContext, fullTypeName, false);

            return type;
        }

        /// <summary>
        /// Get the full type name, excluding assembly
        /// </summary>
        public string TypeName
        {
            get { return m_TypeName; }
        }
    }
}
