// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
// Provide implementation of System.Reflection.Module around a System.Reflection.Metadata.MetadataReader.

using System;
using Debug = Microsoft.UI.Xaml.Markup.Compiler.Lmr.Internal.Debug;
using System.Text;

using System.Collections.Generic;
using System.Collections.Concurrent;
using System.Collections.Immutable;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Reflection.Adds;
using System.Reflection.Metadata;
using System.Reflection.Metadata.Ecma335;
using System.Reflection.PortableExecutable;
using System.Threading;

using System.Reflection;  

namespace Microsoft.UI.Xaml.Markup.Compiler.Lmr
{
    /// <summary>
    /// MetadataOnlyModule represents a module backed by a PEReader + MetadataReader (SRM).
    /// </summary>
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Maintainability", "CA1506:AvoidExcessiveClassCoupling")]
    internal partial class MetadataOnlyModule : Module, IModule2, IDisposable
    {
        readonly private IMetadataExtensionsPolicy m_policy;
        readonly private IReflectionFactory m_factory;
        readonly private string m_modulePath;

        // SRM-based metadata access. PEReader owns the file stream; MetadataReader piggybacks.
        readonly private PEReader m_peReader;
        readonly private MetadataReader m_reader;

        // Lazy-initialized signature type provider
        private LmrTypeProvider m_typeProvider;

        // Cache of ScopeName property.
        private string m_scopeName;

        // Lazy-initialized mapping from TypeCode ordinal to TypeDefinitionHandle (system assembly only).
        private TypeDefinitionHandle[] m_typeCodeMapping;

        // Lazy reverse-lookup caches: member handle --> declaring TypeDefinitionHandle.
        // Built once on first use, then O(1) per lookup.
        private Dictionary<PropertyDefinitionHandle, TypeDefinitionHandle> m_propertyToType;
        private Dictionary<EventDefinitionHandle, TypeDefinitionHandle> m_eventToType;

        private Dictionary<ParameterHandle, MethodDefinitionHandle> m_paramToMethod;


        public MetadataOnlyModule(ITypeUniverse universe, PEReader peReader, MetadataReader reader, string modulePath)
            : this(universe, peReader, reader, new DefaultFactory(), modulePath)
        {
        }


        // universe - type universe that the module is valid in.
        // peReader - PEReader wrapping the PE file stream
        // reader - MetadataReader for the PE's metadata
        // factory - used to create reflection objects (Type, MethodInfo, etc)
        public MetadataOnlyModule(ITypeUniverse universe, PEReader peReader, MetadataReader reader, IReflectionFactory factory, string modulePath)
        {
            Debug.Assert(universe != null);
            Debug.Assert(peReader != null);
            Debug.Assert(reader != null);
            Debug.Assert(factory != null);

            m_assemblyResolver = universe;
            m_peReader = peReader;
            m_reader = reader;
            m_factory = factory;

            m_policy = new MetadataExtensionsPolicy20(universe);

            m_modulePath = modulePath;
        }

        public override string FullyQualifiedName
        {
            get
            {
                return m_modulePath;
            }
        }

        /// <summary>
        /// Get policy object that specifies reflection behavior not directly corresponding to metadata.
        /// </summary>
        internal IMetadataExtensionsPolicy Policy
        {
            get
            {
                return m_policy;
            }
        }

        internal IReflectionFactory Factory
        {
            get { return m_factory; }
        }

        public override string ToString()
        {
            if (m_reader == null)
                return "uninitialized";

            return this.ScopeName;
        }

        public override bool Equals(object obj)
        {
            if (obj == (object)this)
                return true;

            MetadataOnlyModule resolver = obj as MetadataOnlyModule;
            if (resolver != null)
            {
                // Modules must be in the same type universe.
                if (!m_assemblyResolver.Equals(resolver.AssemblyResolver))
                {
                    return false;
                }

                return ScopeName == resolver.ScopeName;
            }
            else
            {
                return false;
            }
        }

        public override int GetHashCode()
        {
            return m_assemblyResolver.GetHashCode() ^ (ScopeName?.GetHashCode() ?? 0);
        }

        readonly private ITypeUniverse m_assemblyResolver;

        public ITypeUniverse AssemblyResolver { get { return m_assemblyResolver; } }

        /// <summary>
        /// Get the raw SRM MetadataReader. Thread-safe for concurrent reads.
        /// </summary>
        internal MetadataReader RawReader { get { return m_reader; } }

        /// <summary>
        /// Get the raw PEReader for IL/RVA access.
        /// </summary>
        internal PEReader RawPEReader { get { return m_peReader; } }

        /// <summary>
        /// Get the LmrTypeProvider for signature decoding. Lazy-initialized.
        /// </summary>
        internal LmrTypeProvider TypeProvider
        {
            get
            {
                if (m_typeProvider == null)
                {
                    Interlocked.CompareExchange(ref m_typeProvider, new LmrTypeProvider(this), null);
                }
                return m_typeProvider;
            }
        }


        // Returns the name of the scope stored in the metadata.
        public override string ScopeName
        {
            get
            {
                if (m_scopeName == null)
                {
                    var moduleDef = m_reader.GetModuleDefinition();
                    m_scopeName = m_reader.GetString(moduleDef.Name);
                }
                return m_scopeName;
            }
        }

        // Get the GUID for this module.
        public override Guid ModuleVersionId
        {
            get
            {
                var moduleDef = m_reader.GetModuleDefinition();
                return m_reader.GetGuid(moduleDef.Mvid);
            }
        }

        public override string Name
        {
            get
            {
                return System.IO.Path.GetFileName(m_modulePath);
            }
        }

        #region Module Members

        // Resolve a TypeDefinitionHandle within this module. 
        internal MetadataOnlyCommonType ResolveTypeDef(TypeDefinitionHandle handle)
        {
            MetadataOnlyCommonType type = m_factory.CreateSimpleType(this, handle);
            return type;
        }

        // Resolve a TypeSpecificationHandle via SRM's DecodeSignature.
        internal Type ResolveTypeSpec(TypeSpecificationHandle handle, GenericContext context)
        {
            var typeSpec = m_reader.GetTypeSpecification(handle);
            var decoded = typeSpec.DecodeSignature(this.TypeProvider, context);
            return SignatureUnwrap.Unwrap(decoded).Type;
        }

        // Create a method type variable (!!index) for SRM callbacks
        internal Type CreateMethodTypeVariable(int index, GenericContext context)
        {
            // If we have context, use it
            if (context != null && context.MethodArgs != null && index < context.MethodArgs.Length)
            {
                return context.MethodArgs[index];
            }
            // Create a synthetic type variable reference
            return new MetadataOnlyTypeVariableRef(this, index, /*isMethodVar:*/ true);
        }

        // Create a type type variable (!index) for SRM callbacks
        internal Type CreateTypeTypeVariable(int index, GenericContext context)
        {
            if (context != null && context.TypeArgs != null && index < context.TypeArgs.Length)
            {
                return context.TypeArgs[index];
            }
            return new MetadataOnlyTypeVariableRef(this, index, /*isMethodVar:*/ false);
        }

        // Get the underlying type of an enum (for custom attribute decoding)
        internal CorElementType GetEnumUnderlyingType(Type type)
        {
            // If the type is defined in this module, use the efficient metadata-based approach
            if (type.Module == this && type is MetadataOnlyCommonType)
            {
                var typeDefHandle = MetadataTokens.TypeDefinitionHandle(type.MetadataToken);
                return GetEnumUnderlyingType(typeDefHandle);
            }
            // Fallback: get the underlying type via reflection and map to CorElementType.
            // We compare against the universe's built-in types rather than using Type.GetTypeCode,
            // because GetTypeCode relies on IsSystemModule() which can fail when the system assembly
            // is type-forwarded (e.g., System.Runtime --> System.Private.CoreLib in modern .NET).
            Type underlyingType = MetadataOnlyModule.GetUnderlyingType(type);
            var u = AssemblyResolver;
            if (underlyingType.Equals(u.GetBuiltInType(CorElementType.Bool  ))) return CorElementType.Bool;
            if (underlyingType.Equals(u.GetBuiltInType(CorElementType.Char  ))) return CorElementType.Char;
            if (underlyingType.Equals(u.GetBuiltInType(CorElementType.SByte ))) return CorElementType.SByte;
            if (underlyingType.Equals(u.GetBuiltInType(CorElementType.Byte  ))) return CorElementType.Byte;
            if (underlyingType.Equals(u.GetBuiltInType(CorElementType.Short ))) return CorElementType.Short;
            if (underlyingType.Equals(u.GetBuiltInType(CorElementType.UShort))) return CorElementType.UShort;
            if (underlyingType.Equals(u.GetBuiltInType(CorElementType.Int   ))) return CorElementType.Int;
            if (underlyingType.Equals(u.GetBuiltInType(CorElementType.UInt  ))) return CorElementType.UInt;
            if (underlyingType.Equals(u.GetBuiltInType(CorElementType.Long  ))) return CorElementType.Long;
            if (underlyingType.Equals(u.GetBuiltInType(CorElementType.ULong ))) return CorElementType.ULong;
            return CorElementType.Int; // fallback
        }

        /// <summary>
        /// Internal API to resolve EntityHandles to Type objects.
        /// This handles TypeDefs, TypeRefs and TypeSpecs.        
        /// </summary>
        internal Type ResolveTypeTokenInternal(EntityHandle handle, GenericContext context)
        {
            switch (handle.Kind)
            {
                case HandleKind.TypeDefinition:
                    return ResolveTypeDef((TypeDefinitionHandle)handle);

                case HandleKind.TypeReference:
                    return this.Factory.CreateTypeRef(this, (TypeReferenceHandle)handle);

                case HandleKind.TypeSpecification:
                    Type[] typeArgs = null;
                    Type[] methodArgs = null;
                    if (context != null)
                    {
                        typeArgs = context.TypeArgs;
                        methodArgs = context.MethodArgs;
                    }
                    return this.Factory.CreateTypeSpec(this, (TypeSpecificationHandle)handle, typeArgs, methodArgs);

                default:
                    throw new ArgumentException(Resources.TypeTokenExpected);
            }
        }

        /// <summary>
        /// Helper method to create a generic type. 
        /// For example: List<T> + { T='int'} --> List<int>.
        /// </summary>
        internal Type GetGenericType(EntityHandle handle, GenericContext context)
        {
            Type[] typeArgs = null;
            Type[] methodArgs = null;
            if (context != null)
            {
                typeArgs = context.TypeArgs;
                methodArgs = context.MethodArgs;
            }

            switch (handle.Kind)
            {
                case HandleKind.TypeDefinition:
                    var typeDef = (TypeDefinitionHandle)handle;
                    if ((typeArgs != null) && (typeArgs.Length > 0))
                    {
                        return m_factory.CreateGenericType(this, typeDef, typeArgs);
                    }
                    else
                    {
                        return m_factory.CreateSimpleType(this, typeDef);
                    }

                case HandleKind.TypeReference:
                    var t = m_factory.CreateTypeRef(this, (TypeReferenceHandle)handle);
                    if ((typeArgs != null) && (typeArgs.Length > 0))
                    {
                        t = t.MakeGenericType(typeArgs);
                    }
                    return t;

                case HandleKind.TypeSpecification:
                    return m_factory.CreateTypeSpec(this, (TypeSpecificationHandle)handle, typeArgs, methodArgs);

                default:
                    throw new ArgumentException(Resources.TypeTokenExpected);
            }
        }

        private MethodBase ResolveMethodTokenInternal(EntityHandle methodHandle, GenericContext context)
        {
            switch (methodHandle.Kind)
            {
                case HandleKind.MethodDefinition:
                    return ResolveMethodDef((MethodDefinitionHandle)methodHandle);

                case HandleKind.MemberReference:
                    return ResolveMethodRef((MemberReferenceHandle)methodHandle, context, null);

                case HandleKind.MethodSpecification:
                    return ResolveMethodSpec((MethodSpecificationHandle)methodHandle, context);

                default:
                    throw new ArgumentException(Resources.MethodTokenExpected);
            }
        }

        /// <summary>
        /// Resolves a reference to an instantiated generic method.
        /// </summary>
        /// <param name="methodToken">MethodSpec token.</param>
        /// <param name="context">Generic context containing types used for intantiation.</param>
        /// <returns>MethodInfo instance containing info about an instantiated generic method.</returns>
        private MethodInfo ResolveMethodSpec(MethodSpecificationHandle handle, GenericContext context)
        {
            var methodSpec = m_reader.GetMethodSpecification(handle);

            // Decode the generic method arguments from the signature, unwrapping any modreq/modopt wrappers
            var genericArgs = methodSpec.DecodeSignature(this.TypeProvider, context);

            Type[] parameters = new Type[genericArgs.Length];
            for (int i = 0; i < genericArgs.Length; i++)
            {
                parameters[i] = SignatureUnwrap.Unwrap(genericArgs[i]).Type;
            }

            var parentMethod = methodSpec.Method;
            MethodInfo result;
            switch (parentMethod.Kind)
            {
                case HandleKind.MethodDefinition:
                    result = GetGenericMethodInfo((MethodDefinitionHandle)parentMethod, new GenericContext(null, parameters));
                    break;

                case HandleKind.MemberReference:
                    result = (MethodInfo)ResolveMethodRef((MemberReferenceHandle)parentMethod, context, parameters);
                    break;

                default:
                    Debug.Assert(false, "invalid handle kind");
                    throw new InvalidOperationException();
            }

            return result;
        }

        // Returns a MethodBase for a given MethodDefinitionHandle.
        private MethodBase ResolveMethodDef(MethodDefinitionHandle handle)
        {
            // If the method contains generic parameters, add them to the context
            List<Type> args = GetTypeParameters(handle);
            GenericContext context = null;
            if (args.Count > 0)
            {
                context = new GenericContext(null, args.ToArray());
            }

            var m = MetadataOnlyMethodInfo.Create(this, handle, context);
            return m;
        }

        // Wrapper to get a MethodInfo, when the method is known that it's not a constructor.
        internal MethodInfo GetGenericMethodInfo(MethodDefinitionHandle handle, GenericContext genericContext)
        {
            return (MethodInfo)GetGenericMethodBase(handle, genericContext);
        }

        // Get a method which may or may not be a constructor.
        internal MethodBase GetGenericMethodBase(MethodDefinitionHandle handle, GenericContext genericContext)
        {
            if (genericContext != null)
            {
                if (((genericContext.TypeArgs == null) || (genericContext.TypeArgs.Length == 0))
                    && ((genericContext.MethodArgs == null) || (genericContext.MethodArgs.Length == 0))
                )
                {
                    genericContext = null;
                }
            }
            var m = MetadataOnlyMethodInfo.Create(this, handle, genericContext);
            return m;
        }

        /// <summary>
        /// Resolves a MethodRef token to a method/constructor.
        /// </summary>
        /// <param name="memberRef">member ref token to resolve</param>
        /// <param name="context">generic context of caller. </param>
        /// <param name="genericMethodParameters">generic parameters to method, or null</param>        
        internal MethodBase ResolveMethodRef(MemberReferenceHandle memberRefHandle, GenericContext context, Type[] genericMethodParameters)
        {
            string methodName;
            EntityHandle declaringTypeHandle;
            BlobReader signatureReader;
            GetMemberRefData(memberRefHandle, out declaringTypeHandle, out methodName, out signatureReader);

            // Check for varargs
            {
                var sigCopy = signatureReader;
                var header = sigCopy.ReadSignatureHeader();
                if (header.CallingConvention == SignatureCallingConvention.VarArgs)
                {
                    throw new NotImplementedException(Resources.VarargSignaturesNotImplemented);
                }
            }

            // Resolve the type that contains the method
            Type declaringType = ResolveTypeTokenInternal(declaringTypeHandle, context);

            MethodSignatureDescriptor descriptor;

            if (declaringType.IsArray)
            {
                var memberRef = m_reader.GetMemberReference(memberRefHandle);
                var sig = memberRef.DecodeMethodSignature(this.TypeProvider, context);
                descriptor = SignatureUtil.FromSrmSignature(sig);
            }
            else
            {
                GenericContext openMethodContext = new OpenGenericContext(this, declaringType, memberRefHandle);
                var memberRef = m_reader.GetMemberReference(memberRefHandle);
                var sig = memberRef.DecodeMethodSignature(this.TypeProvider, openMethodContext);
                descriptor = SignatureUtil.FromSrmSignature(sig);
            }

            GenericContext closedMethodContext = new GenericContext(declaringType.GetGenericArguments(), genericMethodParameters);

            MethodBase matchingMethod = SignatureComparer.FindMatchingMethod(
                methodName,
                declaringType,
                descriptor,
                closedMethodContext);

            if (matchingMethod == null)
            {
                throw new MissingMethodException(declaringType.Name, methodName);
            }

            return matchingMethod;
        }

        // Given a field reference handle, get the field.
        internal FieldInfo ResolveFieldRef(MemberReferenceHandle memberRefHandle, GenericContext context)
        {
            string name;
            EntityHandle declaringTypeHandle;
            BlobReader signatureReader;
            GetMemberRefData(memberRefHandle, out declaringTypeHandle, out name, out signatureReader);

            Type declaringType = ResolveTypeTokenInternal(declaringTypeHandle, context);

            FieldInfo f = declaringType.GetField(name, BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Static | BindingFlags.Instance);
            Debug.Assert(f != null);
            return f;
        }

        // Given a field handle (def or ref), resolve it to a FieldInfo object.
        internal FieldInfo ResolveFieldTokenInternal(EntityHandle fieldHandle, GenericContext context)
        {
            switch (fieldHandle.Kind)
            {
                case HandleKind.FieldDefinition:
                    return this.Factory.CreateField(this, (FieldDefinitionHandle)fieldHandle, null, null);

                case HandleKind.MemberReference:
                    return ResolveFieldRef((MemberReferenceHandle)fieldHandle, context);

                default:
                    throw new ArgumentException(string.Format(
                        CultureInfo.InvariantCulture, Resources.InvalidMetadataToken, fieldHandle.ToString()));
            }
        }

        public override string ResolveString(int metadataToken)
        {
            var handle = MetadataTokens.UserStringHandle(metadataToken);
            return m_reader.GetUserString(handle);
        }

        public override IList<CustomAttributeData> GetCustomAttributesData()
        {
            return GetCustomAttributeData(MetadataTokens.EntityHandle(this.MetadataToken));
        }

        #endregion


        #region Resolution
        // Resolution for TokenRefs. Requires calling back to a resolver object.
        // ICorDebug API can inspect the target to see how Refs were actually resolved. 

        // Resolve a TypeReference token to a System.Type object. 
        // The resulting Type is not necessarily implemented as a LMR type.
        // TypeRefs don't have generic parameters. A generic reference would be encoded in a TypeSpec which would have a
        // TypeRef in it. 
        internal Type ResolveTypeRef(ITypeReference typeReference)
        {
            EntityHandle resScope = typeReference.ResolutionScope;
            string name = typeReference.RawName;
            Type result;

            switch (resScope.Kind)
            {
                case HandleKind.TypeReference:
                    //This happens if tokenTypeRef is a nested class.  We need to resolve 
                    //the enclosing class and then go back and get the class we are interested in.

                    Type tOuter = Factory.CreateTypeRef(this, (TypeReferenceHandle)resScope);


                    // Lookup by name. We already resolved for nested classes, so don't include nested classes in
                    // the name lookup here (so use Name instead of FullName)
                    // This lookup will trigger a Type resolution on tOuter.

                    result = tOuter.GetNestedType(name, BindingFlags.Public | BindingFlags.NonPublic);
                    return result;

                case HandleKind.AssemblyReference:
                    Assembly assembly = m_assemblyResolver.ResolveAssembly(this, (AssemblyReferenceHandle)resScope);

                    // Validate results.
                    if (assembly == null)
                    {
                        Debug.Assert(false);
                        throw new UnresolvedAssemblyException(Resources.ResolverMustResolveToValidAssembly);
                    }
                    {
                        // Validate that the assembly is still in the same universe.
                        IAssembly2 ia2 = (IAssembly2)assembly;

                        if (ia2.TypeUniverse != m_assemblyResolver)
                        {
                            Debug.Assert(false);
                            throw new UnresolvedAssemblyException(Resources.ResolvedAssemblyMustBeWithinSameUniverse);
                        }
                    }

                    result = assembly.GetType(name, true);
                    return result;

                case HandleKind.ModuleReference:
                    Module module = ResolveModuleRef(resScope);
                    result = module.GetType(typeReference.FullName);
                    return result;

                case HandleKind.ModuleDefinition:
                    // The token specifies the current module. This can happen when compiler does not optimize
                    // metadata it emits. This is allowed.
                    // The spec in TypeRef table definition (II.22.38 “TypeRef : 0x01”) mentions ResolutionScope value:
                    //      d. a Module token, if the target type is defined in the current module - 
                    //         this should not occur in a CLI (“compressed metadata”) module [WARNING]
                    return this.GetType(typeReference.FullName);
            }

            // The Ecma spec states that a typeRef scope token must be one
            // of the types listed above. If it's something else, then this likely means corrupted metadata. 
            Debug.Assert(false, "Unexpected tokResScope");
            throw new InvalidOperationException(Resources.InvalidMetadata);
        }


        internal Module ResolveModuleRef(EntityHandle moduleRefHandle)
        {
            Debug.Assert(moduleRefHandle.Kind == HandleKind.ModuleReference);

            if (this.Assembly == null)
            {
                throw new InvalidOperationException(Resources.CannotResolveModuleRefOnNetModule);
            }

            var moduleRef = m_reader.GetModuleReference((ModuleReferenceHandle)moduleRefHandle);
            string text = m_reader.GetString(moduleRef.Name);
            return this.Assembly.GetModule(text);
        }

        /// <summary>
        /// Lookup a TypeDef token for a given top-level type name. Does not handle nested classes.
        /// This is useful for looking up system types (Enum, Int32, etc)</summary>
        /// <param name="className">top-level type name to lookup</param>
        /// <returns>typedef token of type. Throws it type not found.</returns>
        internal TypeDefinitionHandle LookupTypeDefHandle(string className)
        {
            return this.FindTypeDefByName(null, className, true);
        }

        // Find the TypeDef token in the given scope for a given class name.
        // Throw on error.
        internal TypeDefinitionHandle FindTypeDefByName(Type outerType, string className, bool fThrow)
        {
            TypeDefinitionHandle outerTypeHandle = default;
            if (outerType != null)
            {
                if (outerType.Module != this)
                {
                    Debug.Assert(false, "Outer type has different token resolver");
                    throw new InvalidOperationException(Resources.DifferentTokenResolverForOuterType);
                }
                outerTypeHandle = MetadataTokens.TypeDefinitionHandle(outerType.MetadataToken);
            }
            return FindTypeDefByName(outerTypeHandle, className, fThrow);
        }

        // fThrow - true iff throw when the requested type is missing. 
        internal TypeDefinitionHandle FindTypeDefByName(TypeDefinitionHandle outerTypeDefHandle, string className, bool fThrow)
        {
            // Parse namespace and name from className
            string ns = null;
            string name = className;
            int lastDot = className.LastIndexOf('.');
            if (lastDot >= 0 && outerTypeDefHandle.IsNil)
            {
                ns = className.Substring(0, lastDot);
                name = className.Substring(lastDot + 1);
            }

            foreach (var typeDefHandle in m_reader.TypeDefinitions)
            {
                var typeDef = m_reader.GetTypeDefinition(typeDefHandle);

                // Check nesting: if looking for nested type, parent must match
                if (!outerTypeDefHandle.IsNil)
                {
                    if (typeDef.GetDeclaringType() != outerTypeDefHandle)
                        continue;
                }
                else
                {
                    // Skip nested types when searching for top-level
                    if (!typeDef.GetDeclaringType().IsNil)
                        continue;
                }

                string typeName = m_reader.GetString(typeDef.Name);
                if (typeName != name)
                    continue;

                if (ns != null)
                {
                    string typeNs = m_reader.GetString(typeDef.Namespace);
                    if (typeNs != ns)
                        continue;
                }

                return typeDefHandle;
            }

            if (fThrow)
            {
                throw new TypeLoadException(string.Format(
                    CultureInfo.InvariantCulture, Resources.CannotFindTypeInModule, className, this.ToString()));
            }
            return default;
        }

        #endregion // Resolution

        #region IMetadataScope Members



        internal void GetMemberRefData(MemberReferenceHandle handle, out EntityHandle declaringTypeHandle, out string nameMember, out BlobReader signatureReader)
        {
            var memberRef = m_reader.GetMemberReference(handle);
            declaringTypeHandle = memberRef.Parent;
            nameMember = m_reader.GetString(memberRef.Name);
            signatureReader = m_reader.GetBlobReader(memberRef.Signature);
        }

        internal int GetMethodRva(MethodDefinitionHandle handle)
        {
            var methodDef = m_reader.GetMethodDefinition(handle);
            return methodDef.RelativeVirtualAddress;
        }

        internal System.Reflection.MethodImplAttributes GetMethodImplFlags(MethodDefinitionHandle handle)
        {
            var methodDef = m_reader.GetMethodDefinition(handle);
            return (System.Reflection.MethodImplAttributes)methodDef.ImplAttributes;
        }

        internal void GetMethodAttrs(
            MethodDefinitionHandle handle,
            out TypeDefinitionHandle declaringTypeDef,
            out MethodAttributes attrs)
        {
            var methodDef = m_reader.GetMethodDefinition(handle);
            declaringTypeDef = methodDef.GetDeclaringType();
            attrs = methodDef.Attributes;
        }

        internal BlobReader GetMethodSignature(MethodDefinitionHandle handle)
        {
            var methodDef = m_reader.GetMethodDefinition(handle);
            return m_reader.GetBlobReader(methodDef.Signature);
        }

        internal string GetMethodName(MethodDefinitionHandle handle)
        {
            var methodDef = m_reader.GetMethodDefinition(handle);
            return m_reader.GetString(methodDef.Name);
        }

        //Get the underlying type of an Enum type.
        //The input parameter represents a type token of the Enum type.
        // Get the underlying type of an Enum via its TypeDefinitionHandle.
        internal CorElementType GetEnumUnderlyingType(TypeDefinitionHandle typeDefHandle)
        {
            var typeDef = m_reader.GetTypeDefinition(typeDefHandle);
            foreach (var fieldHandle in typeDef.GetFields())
            {
                var field = m_reader.GetFieldDefinition(fieldHandle);
                if ((field.Attributes & FieldAttributes.Static) == 0)
                {
                    // This is the instance field - decode its type to get the underlying enum type
                    var sigReader = m_reader.GetBlobReader(field.Signature);
                    var header = sigReader.ReadSignatureHeader();
                    Debug.Assert(header.Kind == SignatureKind.Field);
                    return (CorElementType)sigReader.ReadByte();
                }
            }
            Debug.Fail("Should never get here.");
            throw new ArgumentException(Resources.OperationValidOnEnumOnly);
        }

        // Raw helper to get TypeAttributes from the metadata tables.
        internal void GetTypeAttributes(TypeDefinitionHandle handle, out EntityHandle baseTypeHandle, out TypeAttributes attr)
        {
            var typeDef = m_reader.GetTypeDefinition(handle);
            attr = typeDef.Attributes;
            baseTypeHandle = typeDef.BaseType;
        }

        // Raw helper to get the TypeNames from the metadata tables. 
        // Raw helper to get type name from the metadata tables.
        internal string GetTypeName(TypeDefinitionHandle handle)
        {
            var typeDef = m_reader.GetTypeDefinition(handle);
            string ns = m_reader.GetString(typeDef.Namespace);
            string n = m_reader.GetString(typeDef.Name);
            string fullName = string.IsNullOrEmpty(ns) ? n : ns + "." + n;
            return TypeNameQuoter.GetQuotedTypeName(fullName);
        }

        /// <summary>
        /// Get the constructors that match flags on a given type. 
        /// </summary>
        internal static ConstructorInfo[] GetConstructorsOnType(MetadataOnlyCommonType type, System.Reflection.BindingFlags flags)
        {
            CheckBindingFlagsInMethod(flags, "GetConstructorsOnType");

            List<ConstructorInfo> result = new List<ConstructorInfo>();
            const bool isInherited = false;

            var constructors = type.GetDeclaredConstructors();
            
            foreach (ConstructorInfo constructorInfo in constructors)
            {
                if (Utility.IsBindingFlagsMatching(constructorInfo, isInherited, flags))
                {
                    result.Add(constructorInfo);
                }
            }

            return result.ToArray();
        }

        internal static ConstructorInfo GetConstructorOnType(
            MetadataOnlyCommonType type, BindingFlags bindingAttr, Binder binder,
            CallingConventions callConvention, Type[] types, ParameterModifier[] modifiers)
        {
            CheckBinderAndModifiersforLMR(binder, modifiers);

            var methods = GetConstructorsOnType(type, bindingAttr);
            foreach (ConstructorInfo m in methods)
            {
                if (!SignatureUtil.IsCallingConventionMatch(m, callConvention))
                {
                    continue;
                }

                if (!SignatureUtil.IsParametersTypeMatch(m, types))
                {
                    continue;
                }
                return m;
            }

            return null;
        }

        static private void CheckBinderAndModifiersforLMR(Binder binder, ParameterModifier[] modifiers)
        {
            //binder must be null for LMR.
            if (binder != null)
            {
                throw new NotSupportedException();
            }

            //ParameterModifier is not handled by LMR
            if (modifiers != null && modifiers.Length != 0)
            {
                throw new NotSupportedException();
            }
        }

        // Common helper for multiple Type classes to use. GetMethodImpl is really just a filter on
        // GetMethods(). This does the filtering and returns the match. 
        internal static MethodInfo GetMethodImplHelper(
            Type type,
            String name, 
            BindingFlags bindingAttr, 
            Binder binder, 
            CallingConventions callConv,
            Type[] types, 
            ParameterModifier[] modifiers)
        {
            //ParameterModifier is not handled by LMR but binder is.
            if (modifiers != null && modifiers.Length != 0)
            {
                throw new NotSupportedException();
            }

            //methods are sorted in the order of inheritance,
            //the most derived is at the first.

            var methods = type.GetMethods(bindingAttr);

            if (binder == null)
            {
                return FilterMethod(methods, name, bindingAttr, callConv, types);
            }

            // Create array of candidates for custom binder based on method name and 
            // calling convention. 
            List<MethodBase> candidates = new List<MethodBase>();
            StringComparison comparison = SignatureUtil.GetStringComparison(bindingAttr);
            foreach (MethodInfo m in methods)
            {
                if (!m.Name.Equals(name, comparison))
                {
                    continue;
                }

                if (!SignatureUtil.IsCallingConventionMatch(m, callConv))
                {
                    continue;
                }

                candidates.Add(m);
            }

            return binder.SelectMethod(bindingAttr, candidates.ToArray(), types, modifiers) as MethodInfo;
        }

        /// <summary>
        /// Find the method matching all the criteria in the method array.
        /// </summary>
        private static MethodInfo FilterMethod(
            MethodInfo[] methods,
            String name,
            BindingFlags bindingAttr,
            CallingConventions callConv,
            Type[] types)
        {
            bool found = false;
            MethodInfo match = null;

            StringComparison comparison = SignatureUtil.GetStringComparison(bindingAttr);

            foreach (MethodInfo m in methods)
            {
                //if already found in the most derived type, no need to go further
                if (found && match.DeclaringType != null && !match.DeclaringType.Equals(m.DeclaringType))
                {
                    break;
                }

                if (!m.Name.Equals(name, comparison))
                {
                    continue;
                }

                if (!SignatureUtil.IsCallingConventionMatch(m, callConv))
                {
                    continue;
                }

                if (!SignatureUtil.IsParametersTypeMatch(m, types))
                {
                    continue;
                }

                if (!found)
                {
                    match = m;
                    found = true;
                }
                else
                {
                    throw new AmbiguousMatchException();
                }
            }

            return match;
        }

        /// <summary>
        /// Returns the methods (but not constructors) on the given Type and its base types (if requested in the flags).
        /// </summary>
        /// <remarks>
        /// This a helper function shared by multiple MetadataOnlyCommonType derivations to implement: 
        ///     MethodInfo[] Type.GetMethods(...) 
        /// </remarks>
        internal static MethodInfo[] GetMethodsOnType(MetadataOnlyCommonType type, System.Reflection.BindingFlags flags)
        {
            CheckBindingFlagsInMethod(flags, "GetMethodsOnType");

            List<MethodInfo> result = new List<MethodInfo>();
            const bool isInherited = false;

            // Get the methods that match binding flags on the type itself first.
            foreach (MethodInfo methodInfo in type.GetDeclaredMethods())
            {
                if (Utility.IsBindingFlagsMatching(methodInfo, isInherited, flags))
                {
                    result.Add(methodInfo);
                }
            }

            // Get the methods on the base type if there is a base type and if flag requests them.
            // We can't use base type's Resolver since base type might not be a LMR type.
            //
            if (WalkInheritanceChain(flags) && (type.BaseType != null))
            {
                MethodInfo[] inheritedMethods = type.BaseType.GetMethods(flags);
                List<MethodInfo> filteredInheritedMembers = new List<MethodInfo>();

                // Filter out any methods that don't match flags or that are overriden by methods on the type
                // itself. There should be no overrides in inheritedMethods array if base type(s) correctly 
                // implement GetMethods(...) method.
                foreach (MethodInfo methodInfo in inheritedMethods)
                {
                    if (IncludeInheritedMethod(methodInfo, result, flags))
                    {
                        filteredInheritedMembers.Add(methodInfo);
                    }
                }

                result.AddRange(filteredInheritedMembers);
            }

            return result.ToArray();
        }

        /// <summary>
        /// Determines if walk up the inheritance chain is requested based on flags. 
        /// </summary>
        private static bool WalkInheritanceChain(System.Reflection.BindingFlags flags)
        {
            // If DeclaredOnly is specified, inheritance chain doesn't need to be examined.
            if ((flags & BindingFlags.DeclaredOnly) != 0)
            {
                return false;
            }
            else
            {
                return true;
            }
        }


        /// <summary>
        /// Filters inherited properties by eliminating overloads.
        /// </summary>
        private static IList<PropertyInfo> FilterInheritedProperties(
            IList<PropertyInfo> inheritedProperties,
            IList<PropertyInfo> properties,
            System.Reflection.BindingFlags flags)
        {
            if ((properties == null) || (properties.Count == 0))
            {
                // If there is no properties to filter against, just return 
                // original list.
                return inheritedProperties;
            }

            List<PropertyInfo> result = new List<PropertyInfo>();
            List<MethodInfo> getters = new List<MethodInfo>();
            List<MethodInfo> setters = new List<MethodInfo>();

            // Create separate lists of getters and setters for efficient 
            // overload checking.
            foreach (PropertyInfo property in properties)
            {
                MethodInfo getter = property.GetGetMethod();
                if (getter != null)
                {
                    getters.Add(getter);
                }

                MethodInfo setter = property.GetSetMethod();
                if (setter != null)
                {
                    setters.Add(setter);
                }
            }

            // If either setter or getter doesn't match flags or is overloaded,
            // we don't include the whole property. 
            foreach (PropertyInfo property in inheritedProperties)
            {
                MethodInfo getter = property.GetGetMethod();
                if ((getter != null) && !IncludeInheritedAccessor(getter, getters, flags))
                {
                    continue;
                }

                MethodInfo setter = property.GetSetMethod();
                if ((setter != null) && !IncludeInheritedAccessor(setter, setters, flags))
                {
                    continue;
                }

                result.Add(property);
            }

            return result;
        }

        /// <summary>
        /// Filters inherited events by eliminating overloads. Overload in case of events is 
        /// simply any inherited event that has the same name as an event directly on a type. 
        /// </summary>
        private static IList<EventInfo> FilterInheritedEvents(IList<EventInfo> inheritedEvents, IList<EventInfo> events)
        {
            if ((events == null) || (events.Count == 0))
            {
                // If there is no events to filter against, just return 
                // original list.
                return inheritedEvents;
            }

            List<EventInfo> result = new List<EventInfo>();

            // Compare name of each inherited event with events on a type itself and 
            // skip any that already exists on type.
            foreach (EventInfo inheritedEvent in inheritedEvents)
            {
                bool nameMatchFound = false;
                foreach (EventInfo directEvent in events)
                {
                    if (inheritedEvent.Name.Equals(directEvent.Name, StringComparison.Ordinal))
                    {
                        nameMatchFound = true;
                        break;
                    }
                }

                if (!nameMatchFound)
                {
                    result.Add(inheritedEvent);
                }
            }

            return result;
        }

        /// <summary>
        /// Determines if an inherited method should be included when walking up inheritance chain.
        /// </summary>
        private static bool IncludeInheritedMethod(MethodInfo inheritedMethod, IEnumerable<MethodInfo> methods, System.Reflection.BindingFlags flags)
        {
            if (!inheritedMethod.IsStatic)
            {
                // Inherited instance members should always be included unless
                // they are virtual and explicitly overriden.
                if (inheritedMethod.IsVirtual)
                {
                    return !IsOverride(methods, inheritedMethod);
                }
                else
                {
                    return true;
                }
            }
            else
            {
                // Inherited static members should be included only when 
                // FlattenHierarchy is specified. It doesn't matter if there
                // are methods with matching signature on the derived class already. 
                if ((flags & BindingFlags.FlattenHierarchy) != 0)
                {
                    return true;
                }
                else
                {
                    return false;
                }
            }
        }

        /// <summary>
        /// Determines if an inherited property getter/setter should be included when walking up inheritance chain.
        /// </summary>
        private static bool IncludeInheritedAccessor(
            MethodInfo inheritedMethod,
            IEnumerable<MethodInfo> methods,
            System.Reflection.BindingFlags flags)
        {
            if (!inheritedMethod.IsStatic)
            {
                // Inherited instance getters/setters are included unless
                // they are explicitly overriden or hidden. For properties, it
                // does not matter if they are virtual or not (as opposed to methods). 
                return !IsOverride(methods, inheritedMethod);
            }
            else
            {
                // Inherited static getters/setters should be included only when 
                // FlattenHierarchy is specified and there are no other properties
                // with matching signature already. 
                if ((flags & BindingFlags.FlattenHierarchy) != 0)
                {
                    return !IsOverride(methods, inheritedMethod);
                }
                else
                {
                    return false;
                }
            }
        }

        /// <summary>
        /// Determines if an inherited field should be included when walking up inheritance chain.
        /// </summary>
        private static bool IncludeInheritedField(FieldInfo inheritedField, System.Reflection.BindingFlags flags)
        {
            if (inheritedField.IsPrivate)
            {
                // Private inherited fields should never be included.
                return false;
            }
            if (!inheritedField.IsStatic)
            {
                // Inherited public/protected instance fields should be included. 
                return true;
            }
            else if ((flags & BindingFlags.FlattenHierarchy) != 0)
            {
                // Public/protected static fields should be included only when FlattenHierarchy is requested.
                return true;
            }
            else
            {
                return false;
            }
        }


        /// <summary>
        /// Filter to use with code:GetMethodsOnDeclaredTypeOnly
        /// </summary>
        internal enum EMethodKind
        {
            Constructor,
            Methods
        }

        /// <summary>
        /// Common helper function for
        ///   MethodInfo[] Type.GetMethods(...) and 
        ///   ConstructorInfo[] Type.GetConstructor().
        /// Gets just the methods and constructors that this type implements, not the ones it inherits. 
        /// </summary>
        /// <remarks>
        /// This is on the TokenResolver so that it can be shared by multiple Type implementations. 
        /// </remarks>
        internal IEnumerable<MethodBase> GetMethodBasesOnDeclaredTypeOnly(TypeDefinitionHandle typeDefHandle, GenericContext context, EMethodKind kind)
        {
            var typeDef = m_reader.GetTypeDefinition(typeDefHandle);
            foreach (var methodHandle in typeDef.GetMethods())
            {
                List<Type> genericParams = GetTypeParameters(methodHandle);
                GenericContext newContext = new GenericContext(context.TypeArgs, genericParams.ToArray());

                MethodBase methodBase = this.GetGenericMethodBase(methodHandle, newContext);

                if ((methodBase is ConstructorInfo) != (kind == EMethodKind.Constructor))
                    continue;

                yield return methodBase;
            }
        }

        // Get generic type parameters for a method definition
        private List<Type> GetTypeParameters(MethodDefinitionHandle methodHandle)
        {
            List<Type> result = new List<Type>();
            var methodDef = m_reader.GetMethodDefinition(methodHandle);
            foreach (var gpHandle in methodDef.GetGenericParameters())
            {
                result.Add(this.Factory.CreateTypeVariable(this, gpHandle));
            }
            return result;
        }

        // Get generic type parameters for a type definition
        internal List<Type> GetTypeParameters(TypeDefinitionHandle typeDefHandle)
        {
            List<Type> result = new List<Type>();
            var typeDef = m_reader.GetTypeDefinition(typeDefHandle);
            foreach (var gpHandle in typeDef.GetGenericParameters())
            {
                result.Add(this.Factory.CreateTypeVariable(this, gpHandle));
            }
            return result;
        }

        // Signature comparer helper GetMethodsOnType() 
        // m1 may have unresolved generic args. 
        // methodCandidate should always be a fully resolved type.
        static bool MatchSignatures(MethodBase m1, MethodBase methodCandidate)
        {
            Debug.Assert(m1 != null);
            Debug.Assert(methodCandidate != null);

            if (m1.Name != methodCandidate.Name)
            {
                // Check for explicit interfaces. Explicit interface impls may contain the interface type in
                // the method name. See Type.GetInterfaceMapping().
                bool fIsExplicitInterface = (m1.Name.Length > methodCandidate.Name.Length && 
                    m1.Name[m1.Name.Length - methodCandidate.Name.Length - 1] == '.' &&  
                    m1.Name.EndsWith(methodCandidate.Name, StringComparison.Ordinal));

                if (!fIsExplicitInterface)
                {
                    return false;
                }
            }

            if (m1.IsStatic != methodCandidate.IsStatic)
                return false;

            ParameterInfo[] p1 = m1.GetParameters();
            ParameterInfo[] pCandidate = methodCandidate.GetParameters();

            if (p1.Length != pCandidate.Length)
                return false;

            // This may need to be ContainsGenericArguments.
            if (m1.IsGenericMethodDefinition)
            {
                Type[] args = methodCandidate.GetGenericArguments();
                m1 = (m1 as MethodInfo).MakeGenericMethod(args);
                p1 = m1.GetParameters();
            }

            // In order to compare parameter types, we need both methods fully resolved. 
            Debug.Assert(!m1.IsGenericMethodDefinition);

            // Match parameter types.
            for (int i = 0; i < p1.Length; i++)
            {
                Type t1 = p1[i].ParameterType;
                Type t2 = pCandidate[i].ParameterType;
                Debug.Assert(t1 != null);
                Debug.Assert(t2 != null);

                if (!t1.Equals(t2))
                    return false;
            }

            //check on return types. 
            MethodInfo mi1 = m1 as MethodInfo;
            MethodInfo mi2 = methodCandidate as MethodInfo;
            if ((mi1 != null && mi2 == null) ||
                (mi1 == null && mi2 != null))
            {
                return false;
            }
            else if (mi1 != null)
            {
                Type retType1 = mi1.ReturnType;
                if (!retType1.Equals(mi2.ReturnType))
                {
                    return false;
                }
            }

            return true;
        }

        /// <summary>
        ///Check if method m overrides any of passed methods.       
        /// </summary>
        static private bool IsOverride(IEnumerable<MethodInfo> methods, MethodInfo m)
        {
            foreach (MethodInfo method in methods)
            {
                if (IsOverride(method, m))
                    return true;
            }

            return false;
        }

        /// <summary>
        ///Check if two methods override each other.       
        /// </summary>
        static private bool IsOverride(MethodInfo m1, MethodInfo m2)
        {
            return MatchSignatures(m1, m2);
        }

        /// <summary>
        /// Returns the fields on the given Type and its base types (if requested in the flags).
        /// </summary>
        /// <remarks>
        /// This a helper function shared by multiple MetadataOnlyCommonType derivations to implement: 
        ///     FieldInfo[] Type.GetFields(...) 
        /// </remarks>
        internal static FieldInfo[] GetFieldsOnType(MetadataOnlyCommonType type, System.Reflection.BindingFlags flags)
        {
            CheckBindingFlagsInMethod(flags, "GetFieldsOnType");

            List<FieldInfo> result = new List<FieldInfo>();
            const bool isInherited = false;

            // Get the fields that match binding flags on the type itself first.
            foreach (FieldInfo fieldInfo in type.Resolver.GetFieldsOnDeclaredTypeOnly(MetadataTokens.TypeDefinitionHandle(type.MetadataToken), type.GenericContext))
            {
                if (Utility.IsBindingFlagsMatching(fieldInfo, isInherited, flags))
                {
                    result.Add(fieldInfo);
                }
            }

            // Get the fields on the base type if there is a base type and if flag requests them.
            // We can't use base type's Resolver since base type might not be a LMR type.
            //
            if (WalkInheritanceChain(flags) && (type.BaseType != null))
            {
                FieldInfo[] inheritedFields = type.BaseType.GetFields(flags);
                List<FieldInfo> filteredInheritedFields = new List<FieldInfo>();

                // Filter out any fields that don't match flags.
                foreach (FieldInfo fieldInfo in inheritedFields)
                {
                    if (IncludeInheritedField(fieldInfo, flags))
                    {
                        filteredInheritedFields.Add(fieldInfo);
                    }
                }

                result.AddRange(filteredInheritedFields);
            }

            return result.ToArray();
        }

        /// <summary>
        /// Gets fields on a specified type. Does not get fields on base classes.
        /// </summary>
        private IEnumerable<FieldInfo> GetFieldsOnDeclaredTypeOnly(TypeDefinitionHandle typeDefHandle, GenericContext context)
        {
            var typeArgs = Type.EmptyTypes;
            var methodArgs = Type.EmptyTypes;
            if (context != null)
            {
                typeArgs = context.TypeArgs;
                methodArgs = context.MethodArgs;
            }

            var typeDef = m_reader.GetTypeDefinition(typeDefHandle);
            foreach (var fieldHandle in typeDef.GetFields())
            {
                FieldInfo fieldInfo = this.Factory.CreateField(this, fieldHandle, typeArgs, methodArgs);
                yield return fieldInfo;
            }
        }

        /// Returns the properties on the given Type and its base types (if requested in the flags).
        /// </summary>
        /// <remarks>
        /// This a helper function shared by multiple MetadataOnlyCommonType derivations to implement: 
        ///     MethodInfo[] Type.GetProperties(...) 
        /// </remarks>
        internal static PropertyInfo[] GetPropertiesOnType(MetadataOnlyCommonType type, System.Reflection.BindingFlags flags)
        {
            CheckBindingFlagsInMethod(flags, "GetPropertiesOnType");

            List<PropertyInfo> result = new List<PropertyInfo>();
            bool isInherited = false;

            // Get the properties that match binding flags on the type itself first.
            foreach (PropertyInfo propertyInfo in type.GetDeclaredProperties())
            {
                bool isStatic = false;
                bool isPublic = false;
                CheckIsStaticAndIsPublicOnProperty(propertyInfo, ref isStatic, ref isPublic);

                if (Utility.IsBindingFlagsMatching(propertyInfo, isStatic, isPublic, isInherited, flags))
                {
                    result.Add(propertyInfo);
                }
            }

            // Get the properties on the base type if there is a base type and if flag requests them.
            // We can't use base type's Resolver since base type might not be a LMR type.
            //
            if (WalkInheritanceChain(flags) && (type.BaseType != null))
            {
                PropertyInfo[] inheritedProperties = type.BaseType.GetProperties(flags);

                // Filter out any properties that are overriden by properties on the type
                // itself. There should be no overrides in inheritedProperties array if base type(s) correctly 
                // implement GetProperties(...) method.
                IList<PropertyInfo> filteredInheritedProperties = FilterInheritedProperties(inheritedProperties, result, flags);
                result.AddRange(filteredInheritedProperties);
            }

            return result.ToArray();
        }

        /// <summary>
        /// Common helper function for
        ///   PropertyInfo[] Type.GetProperties(...) 
        /// Gets just properties that this type implements, not the ones it inherits. 
        /// </summary>
        /// <remarks>
        /// This is on the TokenResolver so that it can be shared by multiple Type implementations. 
        /// </remarks>
        internal IEnumerable<PropertyInfo> GetPropertiesOnDeclaredTypeOnly(TypeDefinitionHandle typeDefHandle, GenericContext context)
        {
            var typeDef = m_reader.GetTypeDefinition(typeDefHandle);
            foreach (var propertyHandle in typeDef.GetProperties())
            {
                PropertyInfo property = this.Factory.CreatePropertyInfo(this, propertyHandle, context.TypeArgs, context.MethodArgs);
                yield return property;
            }
        }

        /// <summary>
        /// Returns the events on the given Type and its base types (if requested in the flags).
        /// </summary>
        /// <remarks>
        /// This a helper function shared by multiple MetadataOnlyCommonType derivations to implement: 
        ///     MethodInfo[] Type.GetEvents(...) 
        /// </remarks>
        static internal EventInfo[] GetEventsOnType(MetadataOnlyCommonType type, System.Reflection.BindingFlags flags)
        {
            CheckBindingFlagsInMethod(flags, "GetEventsOnType");

            List<EventInfo> result = new List<EventInfo>();
            const bool isInherited = false;

            // Get the events that match binding flags on the type itself first.
            foreach (EventInfo eventInfo in type.Resolver.GetEventsOnDeclaredTypeOnly(MetadataTokens.TypeDefinitionHandle(type.MetadataToken), type.GenericContext))
            {
                bool isStatic = false;
                bool isPublic = false;
                CheckIsStaticAndIsPublicOnEvent(eventInfo, ref isStatic, ref isPublic);

                if (Utility.IsBindingFlagsMatching(eventInfo, isStatic, isPublic, isInherited, flags))
                {
                    result.Add(eventInfo);
                }
            }

            // Get the events on the base type if there is a base type and if flag requests them.
            // We can't use base type's Resolver since base type might not be a LMR type.
            //
            if (WalkInheritanceChain(flags) && (type.BaseType != null))
            {
                EventInfo[] inheritedEvents = type.BaseType.GetEvents(flags);

                // Filter out any events that are hidden by events on the type
                // itself. There should be no overrides in inheritedEvents array if base type(s) correctly 
                // implement GetEvents(...) method.
                IList<EventInfo> filteredInheritedEvents = FilterInheritedEvents(inheritedEvents, result);
                result.AddRange(filteredInheritedEvents);
            }

            return result.ToArray();
        }

        /// <summary>
        /// Common helper function for
        ///   PropertyInfo[] Type.GetEvents(...) 
        /// Gets just events that this type implements, not the ones it inherits. 
        /// </summary>
        /// <remarks>
        /// This is on the TokenResolver so that it can be shared by multiple Type implementations. 
        /// </remarks>
        private IEnumerable<EventInfo> GetEventsOnDeclaredTypeOnly(TypeDefinitionHandle typeDefHandle, GenericContext context)
        {
            var typeDef = m_reader.GetTypeDefinition(typeDefHandle);
            foreach (var eventHandle in typeDef.GetEvents())
            {
                EventInfo eventInfo = this.Factory.CreateEventInfo(this, eventHandle, context.TypeArgs, context.MethodArgs);
                yield return eventInfo;
            }
        }


        internal IEnumerable<Type> GetNestedTypesOnType(MetadataOnlyCommonType type, BindingFlags flags)
        {
            return GetNestedTypesOnType(MetadataTokens.TypeDefinitionHandle(type.MetadataToken), flags);
        }

        //Lazily calculate all the nested type information and store in the field.
        //The key of the dictionary is the nesting type token.
        //The value of the dictionary is the nested types in the type of the key token.
        //If a type does not have nested types, its token doesn't appear in the dictionary.        
        //This is readonly data. Wrap the dictionary in its own class to enforce that.
        internal class NestedTypeCache
        {
            readonly private Dictionary<int, List<int>> m_cache;

            // Initialize the cache for the given module
            public NestedTypeCache(MetadataOnlyModule outer)
            {
                // Operate on local copy so that creation is thread-safe.
                m_cache = new Dictionary<int, List<int>>();

                var typeDefHandles = outer.GetTypeDefHandles();

                foreach (var handle in typeDefHandles)
                {
                    int token = MetadataTokens.GetToken(handle);
                    var enclosingTypeToken = outer.GetDeclaringType(MetadataTokens.TypeDefinitionHandle(token));
                    if (enclosingTypeToken.IsNil)
                    {
                        continue;
                    }
                    int enclosingTokenValue = MetadataTokens.GetToken(enclosingTypeToken);
                    if (m_cache.ContainsKey(enclosingTokenValue))
                    {
                        //if the type already has an entry in the dictionary, add the nested type
                        Debug.Assert(m_cache[enclosingTokenValue] != null);
                        m_cache[enclosingTokenValue].Add(token);
                    }
                    else
                    {
                        //create an entry for the type
                        List<int> nestedTypes = new List<int>();
                        nestedTypes.Add(token);
                        m_cache.Add(enclosingTokenValue, nestedTypes);
                    }
                }
            }

            // Get the typedef tokens for types nested in tokenTypeDef
            // Return null if there are no nestings.
            public IEnumerable<int> GetNestedTokens(TypeDefinitionHandle typeDefHandle)
            {
                List<int> list;
                int tokenValue = MetadataTokens.GetToken(typeDefHandle);
                if (this.m_cache.TryGetValue(tokenValue, out list))
                {
                    return list;
                }
                return null;
            }
        }

        // This is conceptually read-only, but it's lazily initialized.
        NestedTypeCache m_nestedTypeInfo;

        // Ensure the dictionary containing the nested type information for all the types in
        // the module is initialized.
        void EnsureNestedTypeCacheExists()
        {
            if (m_nestedTypeInfo == null)
            {
                // In a race, we just double-initialize the data. 
                m_nestedTypeInfo = new NestedTypeCache(this);
            }
            Debug.Assert(m_nestedTypeInfo != null);
        }

        // This is on the TokenResolver so that it can be shared by multiple Type implementations. 
        // Get the nested types in this type definition.
        internal IEnumerable<Type> GetNestedTypesOnType(TypeDefinitionHandle typeDefHandle, BindingFlags flags)
        {
            CheckBindingFlagsInMethod(flags, "GetNestedTypesOnType");

            EnsureNestedTypeCacheExists();

            var e = m_nestedTypeInfo.GetNestedTokens(typeDefHandle);
            if (e != null)
            {
                foreach (int typeToken in e)
                {
                    Type type = ResolveType(typeToken);
                    const bool isStatic = false;
                    bool isPublic = type.IsPublic || type.IsNestedPublic;

                    if (Utility.IsBindingFlagsMatching(type, isStatic, isPublic, false, flags))
                    {
                        yield return type;
                    }
                }
            }
        }

        /// <summary>
        /// Gets all custom attributes on a member.
        /// </summary>
        /// <param name="memberTokenValue">Member's metadata token.</param>
        /// <returns>List of CustomAttributeData instances describing all custom attributes.</returns>
        public IList<CustomAttributeData> GetCustomAttributeData(EntityHandle memberHandle)
        {
            List<CustomAttributeData> result = new List<CustomAttributeData>();

            foreach (var caHandle in m_reader.GetCustomAttributes(memberHandle))
            {
                var ca = m_reader.GetCustomAttribute(caHandle);

                // Resolve constructor. This must be a memberRef or methodDef handle.
                ConstructorInfo ctor = ResolveCustomAttributeConstructor(ca.Constructor);

                CustomAttributeData data = new MetadataOnlyCustomAttributeData(this, caHandle, ctor);
                result.Add(data);
            }

            // Get PCAs if policy requires them.
            IEnumerable<CustomAttributeData> pseudoCustomAttributes =
                m_policy.GetPseudoCustomAttributes(this, memberHandle);
            result.AddRange(pseudoCustomAttributes);

            return result;
        }




        /// <summary>
        /// Return a ConstructorInfo proxy for the ctor token which can be used in a Custom Attribute information.  
        /// This proxy just supports getting the custom attribute name without resolving (eg, calling ctor.DeclaringType.FullName.)
        /// </summary>
        /// <param name="customAttributeConstructorTokenValue">token from custom attr  representing the contstructor. </param>
        /// <returns></returns>
        /// <remarks>
        /// Reflection API has Custom Attributes expose the constructor Info, but what clients really want
        /// is a fast way to get to the string name without resolution. 
        ///
        /// We want to do lazy resolution here:
        /// - performance: faster filtering of custom attributes. Clients just need the string name and
        ///    not the fully resolved constructor Info (which would require resolving all the type parameters
        ///    too).
        /// - avoid eager assembly resolution. CustomAttr args require assembly resolution, but not 
        ///    if we're just getting the attribute name,
        /// </remarks>
        ConstructorInfo ResolveCustomAttributeConstructor(EntityHandle constructorHandle)
        {
            switch (constructorHandle.Kind)
            {
                case HandleKind.MethodDefinition:
                    return (ConstructorInfo)ResolveMethodDef((MethodDefinitionHandle)constructorHandle);

                case HandleKind.MemberReference:
                    var memberRefHandle = (MemberReferenceHandle)constructorHandle;
                    EntityHandle declaringTypeHandle;
                    string methodName;
                    BlobReader signatureReader;
                    GetMemberRefData(memberRefHandle, out declaringTypeHandle, out methodName, out signatureReader);
                    Type declaringType = ResolveTypeTokenInternal(declaringTypeHandle, null);
                    return new ConstructorInfoRef(declaringType, this, memberRefHandle);

                default:
                    throw new ArgumentException(Resources.MethodTokenExpected);
            }
        }

        /// <summary>
        /// Parses the custom attribute blob using SRM's DecodeValue and returns constructor arguments
        /// and named arguments. This allows lazily parsing the attribute blob.
        /// </summary>
        internal void LazyAttributeParse(
            CustomAttributeHandle caHandle,
            ConstructorInfo constructorInfo,
            out IList<CustomAttributeTypedArgument> constructorArguments,
            out IList<CustomAttributeNamedArgument> namedArguments)
        {
            var ca = m_reader.GetCustomAttribute(caHandle);
            var typeProvider = new LmrTypeProvider(this);
            var decoded = ca.DecodeValue(typeProvider);

            constructorArguments = ConvertFixedArguments(decoded.FixedArguments);
            namedArguments = ConvertNamedArguments(decoded.NamedArguments, constructorInfo);
        }

        private static IList<CustomAttributeTypedArgument> ConvertFixedArguments(
            ImmutableArray<CustomAttributeTypedArgument<Type>> fixedArgs)
        {
            var result = new List<CustomAttributeTypedArgument>(fixedArgs.Length);
            foreach (var arg in fixedArgs)
                result.Add(ConvertTypedArgument(arg));
            return result.AsReadOnly();
        }

        private static CustomAttributeTypedArgument ConvertTypedArgument(
            CustomAttributeTypedArgument<Type> srmArg)
        {
            object value = srmArg.Value;

            // SRM represents arrays as ImmutableArray<CustomAttributeTypedArgument<Type>>
            if (value is ImmutableArray<CustomAttributeTypedArgument<Type>> arrayElements)
            {
                var converted = new List<CustomAttributeTypedArgument>(arrayElements.Length);
                foreach (var elem in arrayElements)
                    converted.Add(ConvertTypedArgument(elem));
                value = converted.AsReadOnly();
            }

            return new CustomAttributeTypedArgument(srmArg.Type, value);
        }

        private static IList<CustomAttributeNamedArgument> ConvertNamedArguments(
            ImmutableArray<CustomAttributeNamedArgument<Type>> namedArgs,
            ConstructorInfo constructorInfo)
        {
            var result = new List<CustomAttributeNamedArgument>(namedArgs.Length);
            foreach (var arg in namedArgs)
            {
                MemberInfo member;
                if (arg.Kind == CustomAttributeNamedArgumentKind.Field)
                    member = constructorInfo.DeclaringType.GetField(arg.Name, BindingFlags.Instance | BindingFlags.Public);
                else
                    member = constructorInfo.DeclaringType.GetProperty(arg.Name);

                Debug.Assert(member != null, "Expected to find appropriate field/property on an attribute instance.");

                object value = arg.Value;
                // SRM represents arrays as ImmutableArray<CustomAttributeTypedArgument<Type>>
                if (value is ImmutableArray<CustomAttributeTypedArgument<Type>> arrayElements)
                {
                    var converted = new List<CustomAttributeTypedArgument>(arrayElements.Length);
                    foreach (var elem in arrayElements)
                        converted.Add(ConvertTypedArgument(elem));
                    value = converted.AsReadOnly();
                }

                var typedArg = new CustomAttributeTypedArgument(arg.Type, value);
                result.Add(new CustomAttributeNamedArgument(member, typedArg));
            }
            return result.AsReadOnly();
        }

        /// <summary>
        /// Finds the underlying type of an enum type.
        /// </summary>
        /// <remarks>We might need to expose this on our own Enum derivation if it turns out that our types
        /// can't be passed to System.Enum.GetUnderlyingType.</remarks>
        internal static Type GetUnderlyingType(Type enumType)
        {
            // We need an implementation of System.Enum.GetUnderlyingType that works on non-CLR types.
            Debug.Assert(enumType.IsEnum, "enumType must be an Enum.");

            // We can't rely on enum's field name to always be "value__" since non-MS code generators
            // can use any name. The name is not part of the standard. The CLS just says that there must be
            // one and only one instance field on an enum type. 

            FieldInfo[] valueFields = enumType.GetFields(BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance);
            Debug.Assert(valueFields != null, "Enums must have public instance fields.");
            Debug.Assert(valueFields.Length == 1, "Enums must have exactly one public instance field.");

            return valueFields[0].FieldType;
        }

        // This is on the TokenResolver so that it can be shared by multiple Type implementations. 
        // Get the nested types in this type definition.
        internal Type GetEnclosingType(TypeDefinitionHandle typeDefHandle)
        {
            var enclosingHandle = GetDeclaringType(typeDefHandle);
            if (enclosingHandle.IsNil)
            {
                return null;
            }
            return this.ResolveTypeDef(enclosingHandle);
        }
        #endregion


        /// <summary>
        /// Gets the Assembly Name for the given assembly ref token.
        /// </summary>
        public AssemblyName GetAssemblyNameFromAssemblyRef(AssemblyReferenceHandle assemblyRefHandle)
        {
            var assemblyRef = m_reader.GetAssemblyReference(assemblyRefHandle);
            return assemblyRef.GetAssemblyName();
        }


        // If this is a nested type, returns the token for the outer type.
        // Else returns a 0 token if the type is not nested. 
        // If this is a nested type, returns the handle for the outer type.
        // Returns default handle if the type is not nested.
        internal TypeDefinitionHandle GetDeclaringType(TypeDefinitionHandle handle)
        {
            var typeDef = m_reader.GetTypeDefinition(handle);
            return typeDef.GetDeclaringType();
        }

        /// <summary>
        /// Gets the TypeDefinitionHandle that declares the given property.
        /// SRM doesn't provide a direct parent accessor, so we walk TypeDefinition property maps.
        /// </summary>
        internal TypeDefinitionHandle GetDeclaringTypeForProperty(PropertyDefinitionHandle propHandle)
        {
            if (m_propertyToType == null)
                BuildPropertyToTypeCache();
            m_propertyToType.TryGetValue(propHandle, out var result);
            return result;
        }

        /// <summary>
        /// Gets the TypeDefinitionHandle that declares the given event.
        /// </summary>
        internal TypeDefinitionHandle GetDeclaringTypeForEvent(EventDefinitionHandle eventHandle)
        {
            if (m_eventToType == null)
                BuildEventToTypeCache();
            m_eventToType.TryGetValue(eventHandle, out var result);
            return result;
        }

        /// <summary>
        /// Gets the MethodDefinitionHandle that declares the given parameter.
        /// </summary>
        internal MethodDefinitionHandle GetDeclaringMethodForParameter(ParameterHandle paramHandle)
        {
            if (m_paramToMethod == null)
                BuildParamToMethodCache();
            m_paramToMethod.TryGetValue(paramHandle, out var result);
            return result;
        }

        private void BuildPropertyToTypeCache()
        {
            var map = new Dictionary<PropertyDefinitionHandle, TypeDefinitionHandle>();
            foreach (var typeDefHandle in m_reader.TypeDefinitions)
            {
                var typeDef = m_reader.GetTypeDefinition(typeDefHandle);
                foreach (var ph in typeDef.GetProperties())
                {
                    map[ph] = typeDefHandle;
                }
            }
            m_propertyToType = map;
        }

        private void BuildEventToTypeCache()
        {
            var map = new Dictionary<EventDefinitionHandle, TypeDefinitionHandle>();
            foreach (var typeDefHandle in m_reader.TypeDefinitions)
            {
                var typeDef = m_reader.GetTypeDefinition(typeDefHandle);
                foreach (var eh in typeDef.GetEvents())
                {
                    map[eh] = typeDefHandle;
                }
            }
            m_eventToType = map;
        }


        private void BuildParamToMethodCache()
        {
            var map = new Dictionary<ParameterHandle, MethodDefinitionHandle>();
            foreach (var typeDefHandle in m_reader.TypeDefinitions)
            {
                var typeDef = m_reader.GetTypeDefinition(typeDefHandle);
                foreach (var methodHandle in typeDef.GetMethods())
                {
                    var methodDef = m_reader.GetMethodDefinition(methodHandle);
                    foreach (var ph in methodDef.GetParameters())
                    {
                        map[ph] = methodHandle;
                    }
                }
            }
            m_paramToMethod = map;
        }

        // Get the number of generic parameters for the given handle.
        internal int CountGenericParams(EntityHandle handle)
        {
            switch (handle.Kind)
            {
                case HandleKind.TypeDefinition:
                    return m_reader.GetTypeDefinition((TypeDefinitionHandle)handle).GetGenericParameters().Count;
                case HandleKind.MethodDefinition:
                    return m_reader.GetMethodDefinition((MethodDefinitionHandle)handle).GetGenericParameters().Count;
                default:
                    return 0;
            }
        }

        /// <summary>
        /// Get the tokens of the generic parameters in the type or method. 
        /// </summary>
        // Get the generic parameter handles for a type or method definition.
        internal IEnumerable<GenericParameterHandle> GetGenericParameterHandles(EntityHandle typeOrMethodHandle)
        {
            switch (typeOrMethodHandle.Kind)
            {
                case HandleKind.TypeDefinition:
                    foreach (var gp in m_reader.GetTypeDefinition((TypeDefinitionHandle)typeOrMethodHandle).GetGenericParameters())
                        yield return gp;
                    break;
                case HandleKind.MethodDefinition:
                    foreach (var gp in m_reader.GetMethodDefinition((MethodDefinitionHandle)typeOrMethodHandle).GetGenericParameters())
                        yield return gp;
                    break;
            }
        }

        //Get the constraint types of the generic type parameter.
        // Get the constraint types of a generic type parameter.
        internal IEnumerable<Type> GetConstraintTypes(GenericParameterHandle gpHandle)
        {
            var gp = m_reader.GetGenericParameter(gpHandle);
            foreach (var constraintHandle in gp.GetConstraints())
            {
                var constraint = m_reader.GetGenericParameterConstraint(constraintHandle);
                yield return ResolveTypeTokenInternal(constraint.Type, null);
            }
        }

        //Get the owner type or method and the name of the generic paramter.
        //One of the ownerType and ownerMethod must be 0.
        // Get the owner type or method and the name of the generic parameter.
        internal void GetGenericParameterProps(
            GenericParameterHandle gpHandle,
            out TypeDefinitionHandle ownerType,
            out MethodDefinitionHandle ownerMethod,
            out string name,
            out System.Reflection.GenericParameterAttributes attributes,
            out int genIndex)
        {
            var gp = m_reader.GetGenericParameter(gpHandle);
            name = m_reader.GetString(gp.Name);
            attributes = gp.Attributes;
            genIndex = gp.Index;
            var parent = gp.Parent;
            if (parent.Kind == HandleKind.MethodDefinition)
            {
                ownerMethod = (MethodDefinitionHandle)parent;
                ownerType = default;
            }
            else
            {
                ownerType = (TypeDefinitionHandle)parent;
                ownerMethod = default;
            }
        }

        //Get the interfaces the type implements directly.
        internal IEnumerable<Type> GetInterfacesOnType(Type type)
        {
            Debug.Assert(type.Module == this, "GetInterfacesOnType() called on wrong token resolver");

            //If type is a type variable, get the interfaces it implements from its type constraints.
            //e.g 
            //class Foo<T> where T:IFoo
            //IFoo will be in the result when getting interfaces for the type variable T.
            if (type.IsGenericParameter)
            {
                // MetadataToken returns the full ECMA-335 token in the form:
                //     (high byte = table ID, lower 3 bytes = row number)
                // MetadataTokens.GenericParameterHandle() expects just the row number (lower 24 bits),
                // so mask to strip the table prefix.
                foreach (Type c in GetConstraintTypes(MetadataTokens.GenericParameterHandle(type.MetadataToken & 0x00FFFFFF)))
                {
                    if (c.IsInterface)
                    {
                        yield return c;
                    }
                }
            }
            else
            {
                foreach (var implHandle in EnumerateInterfaceImplsOnType(type))
                {
                    yield return GetInterfaceTypeFromInterfaceImpl(type, implHandle);
                }
            }
        }

        internal IEnumerable<InterfaceImplementationHandle> EnumerateInterfaceImplsOnType(Type type)
        {
            var typeDefHandle = MetadataTokens.TypeDefinitionHandle(type.MetadataToken);
            var typeDef = m_reader.GetTypeDefinition(typeDefHandle);
            foreach (var implHandle in typeDef.GetInterfaceImplementations())
            {
                yield return implHandle;
            }
        }

        internal Type GetInterfaceTypeFromInterfaceImpl(Type type, InterfaceImplementationHandle implHandle)
        {
            var impl = m_reader.GetInterfaceImplementation(implHandle);
            Type result = ResolveTypeTokenInternal(impl.Interface, new GenericContext(type.GetGenericArguments(), null));
            return result;
        }

        static public Type GetInterfaceHelper(Type[] interfaces, string name, bool ignoreCase)
        {
            if (name == null)
            {
                throw new ArgumentNullException("name");
            }

            bool match = false;
            Type result = null;

            foreach (Type i in interfaces)
            {
                match = Utility.Compare(name, i.FullName, ignoreCase);
                

                if (match)
                {
                    if (result != null)
                    {
                        throw new AmbiguousMatchException();
                    }
                    else
                    {
                        result = i;
                    }
                }
            }

            return result;
        }

        #region Scope-wide Enumeration
        // Get all the TypeDefinitions in this scope
        // This does not give back instantiated generics (TypeSpecs).
        // Signature is consistent with reflection.  See code:GetTypes for reflection version that returns
        // an array.
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1024:UsePropertiesWhereAppropriate")]
        public IEnumerable<Type> GetTypeList()
        {
            // Fastest enumeration would probably be to get the table size and then zip through that
            // creating tokens.
            foreach (var typeDefHandle in GetTypeDefHandles())
            {
                Type result = this.ResolveTypeDef(typeDefHandle);
                yield return result;
            }
        }

        // Get all the TypeDefinition tokens in this scope
        private IEnumerable<TypeDefinitionHandle> GetTypeDefHandles()
        {
            foreach (var typeDefHandle in m_reader.TypeDefinitions)
            {
                yield return typeDefHandle;
            }
        }
        #endregion

        //CheckBindingFlagsInMethod is used to check if the input Binding flags contains any
        //flag that is not supported by LMR. This method is used by GetXXXOnType().
        private static void CheckBindingFlagsInMethod(BindingFlags flags, string methodName)
        {
            // FlattenHierarchy requires base-class resolution.
            const BindingFlags ok = BindingFlags.DeclaredOnly |
                                    BindingFlags.Instance |
                                    BindingFlags.Static |
                                    BindingFlags.Public |
                                    BindingFlags.NonPublic |
                                    BindingFlags.FlattenHierarchy |
                                    BindingFlags.IgnoreCase |
                // flags required by TargetFrameworkProvider which provides invocation ability
                                    BindingFlags.CreateInstance |
                                    BindingFlags.GetField |
                                    BindingFlags.GetProperty |
                                    BindingFlags.InvokeMethod |
                                    BindingFlags.SetField |
                                    BindingFlags.SetProperty |
                                    BindingFlags.NonPublic |
                                    BindingFlags.ExactBinding;
            if ((flags | ok) != ok)
                throw new NotSupportedException(string.Format(CultureInfo.InvariantCulture,
                    Resources.MethodIsUsingUnsupportedBindingFlags, methodName, flags));
        }


        /// <summary>
        /// Checks if property is static and/or public.
        /// Property is static if one of its set/get accessors is static.
        /// Property is public if one of its set/get accessors is public.
        /// </summary>
        private static void CheckIsStaticAndIsPublicOnProperty(PropertyInfo propertyInfo, ref bool isStatic, ref bool isPublic)
        {
            bool nonPublic = true;
            MethodInfo getter = propertyInfo.GetGetMethod(nonPublic);
            CheckIsStaticAndIsPublic(getter, ref isStatic, ref isPublic);

            MethodInfo setter = propertyInfo.GetSetMethod(nonPublic);
            CheckIsStaticAndIsPublic(setter, ref isStatic, ref isPublic);
        }

        /// <summary>
        /// Checks if event is static and/or public.
        /// Event is static if one of its add/remove/raise accessors is static.
        /// Event is public if one of its add/remove/raise accessors is public.
        /// </summary>
        private static void CheckIsStaticAndIsPublicOnEvent(EventInfo eventInfo, ref bool isStatic, ref bool isPublic)
        {
            bool nonPublic = true;
            MethodInfo addMethod = eventInfo.GetAddMethod(nonPublic);
            CheckIsStaticAndIsPublic(addMethod, ref isStatic, ref isPublic);

            MethodInfo removeMethod = eventInfo.GetRemoveMethod(nonPublic);
            CheckIsStaticAndIsPublic(removeMethod, ref isStatic, ref isPublic);

            MethodInfo raiseMethod = eventInfo.GetRaiseMethod(nonPublic);
            CheckIsStaticAndIsPublic(raiseMethod, ref isStatic, ref isPublic);
        }

        /// <summary>
        /// Check if a Method is static or public. Used for properties and events. They are considered static/public
        /// if any of accessor methods are static/public.
        /// </summary>
        private static void CheckIsStaticAndIsPublic(MethodInfo methodInfo, ref bool isStatic, ref bool isPublic)
        {
            // If there is no particular accessor method, state is unchanged.  
            if (methodInfo == null)
            {
                return;
            }

            if (methodInfo.IsStatic)
            {
                isStatic = true;
            }

            if (methodInfo.IsPublic)
            {
                isPublic = true;
            }
        }


        // Assembly creation will set the assembly backpointer on contained modules.
        Assembly m_assembly;
        internal void SetContainingAssembly(Assembly assembly)
        {
            Debug.Assert(m_assembly == null); // only set once.
            m_assembly = assembly;
        }

        #region Module Members

        // Backpointer to the assembly that this module is contained in.
        // If we open just the .NetModule, then we won't have a containing assembly. ILDasm can do this.
        public override Assembly Assembly
        {
            get
            {
                return m_assembly;
            }
        }

        Dictionary<String , Type> _typeCache = new Dictionary<String, Type>();

        override public Type GetType(string className, bool throwOnError, bool ignoreCase)
        {
            if (ignoreCase)
            {
                // Metadata FindTypeDefByName() doesn't support case-insensitive lookup, so we'll need to
                // roll this ourselves.
                throw new NotImplementedException(Resources.CaseInsensitiveTypeLookupNotImplemented);
            }

            Type type;
            if( _typeCache.TryGetValue(className, out type) )
            {
                return type;
            }

            Func<AssemblyName, Assembly> assemblyResolverCallback = delegate(AssemblyName assemblyName)
            {
                Debug.Assert(assemblyName != null);
                return AssemblyResolver.ResolveAssembly(assemblyName);
            };

            Func<Assembly, string, bool, Type> typeResolver = delegate(Assembly assembly, string simpleTypeName, bool ignoreCaseInCallback)
            {
                bool throwOnErrorInCallback = false;
                if (assembly != null)
                {
                    Type t = assembly.GetType(simpleTypeName, throwOnErrorInCallback, ignoreCaseInCallback);
                    _typeCache[className]=t;
                    return t;
                }
                else
                {
                    // need to plumb through a no-throw option.
                    var tk = FindTypeDefByName((Type)null, simpleTypeName, false);

                    if (tk.IsNil)
                    {
                        _typeCache[className]=null;
                        return null;
                    }

                    Type t = this.ResolveTypeDef(tk);
                    _typeCache[className]=t;
                    return t;
                }
            };

            return Type.GetType(className, assemblyResolverCallback, typeResolver, throwOnError);
        }


        // Reflection version which returns an array. See code:GetTypeList() for enum version.
        override public Type[] GetTypes()
        {
            List<Type> l = new List<Type>(GetTypeList());
            return l.ToArray();
        }

        override public Type[] FindTypes(TypeFilter filter, object filterCriteria)
        {
            List<Type> l = new List<Type>();
            foreach (Type t in GetTypeList())
            {
                if (filter(t, filterCriteria))
                {
                    l.Add(t);
                }
            }
            return l.ToArray();
        }

        private const BindingFlags DefaultLookup = BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public;

        public override FieldInfo GetField(string name, BindingFlags bindingAttr)
        {
            if (name == null) throw new ArgumentNullException("name");
            FieldInfo[] fs = GetFields(bindingAttr);
            foreach (FieldInfo f in fs)
            {
                if (f.Name.Equals(name))
                {
                    return f;
                }
            }
            return null;
        }
        public override FieldInfo[] GetFields(BindingFlags bindingFlags)
        {
            CheckBindingFlagsInMethod(bindingFlags, "GetFields");

            // Get global fields (fields on the <Module> type)
            List<FieldInfo> result = new List<FieldInfo>();
            var moduleTypeHandle = MetadataTokens.TypeDefinitionHandle(this.MetadataToken);
            // Module-level fields are on the first TypeDef (<Module>)
            foreach (var typeDefHandle in m_reader.TypeDefinitions)
            {
                var typeDef = m_reader.GetTypeDefinition(typeDefHandle);
                foreach (var fieldHandle in typeDef.GetFields())
                {
                    FieldInfo fieldInfo = this.Factory.CreateField(this, fieldHandle, null, null);
                    if (Utility.IsBindingFlagsMatching(fieldInfo, false, bindingFlags))
                    {
                        result.Add(fieldInfo);
                    }
                }
                break; // Only first type (<Module>) has global fields
            }
            return result.ToArray();
        }

        protected override MethodInfo GetMethodImpl(string name, BindingFlags bindingAttr, Binder binder, CallingConventions callConvention, Type[] types, ParameterModifier[] modifiers)
        {
            CheckBinderAndModifiersforLMR(binder, modifiers);
            MethodInfo[] methods = GetMethods(bindingAttr);
            return FilterMethod(methods, name, bindingAttr, callConvention, types);
        }

        public override MethodInfo[] GetMethods(BindingFlags bindingFlags)
        {
            CheckBindingFlagsInMethod(bindingFlags, "GetMethods");

            List<MethodInfo> result = new List<MethodInfo>();
            // Global methods are on the first TypeDef (<Module>)
            foreach (var typeDefHandle in m_reader.TypeDefinitions)
            {
                var typeDef = m_reader.GetTypeDefinition(typeDefHandle);
                foreach (var methodHandle in typeDef.GetMethods())
                {
                    MethodBase method = ResolveMethodDef(methodHandle);
                    if (Utility.IsBindingFlagsMatching(method, false, bindingFlags))
                    {
                        MethodInfo methodInfo = method as MethodInfo;
                        if (methodInfo != null)
                        {
                            result.Add(methodInfo);
                        }
                    }
                }
                break; // Only first type (<Module>)
            }
            return result.ToArray();
        }

        // Get the metadata token for this module. This is probably only interesting in multi-module assemblies.
        public override int MetadataToken
        {
            get
            {
                // Module token is always 0x00000001
                return 0x00000001;
            }
        }

        public override bool IsResource()
        {
            return false;
        }

        #endregion

        public override Type ResolveType(int metadataToken, Type[] genericTypeArguments, Type[] genericMethodArguments)
        {
            // This handles TypeDef,Ref,Specs.
            // Note that the generic context is only used for a TypeSpec. So if we pass in a TypeDef for List<T>, we
            // get back the open generic type.
            var t = ResolveTypeTokenInternal(MetadataTokens.EntityHandle(metadataToken), new GenericContext(genericTypeArguments, genericMethodArguments));

            // Reflection's behavior will validate the type that it hands back. 
            // Just resolve it to force validation to mimic reflection semantics.
            // But still return the proxy so that clients can get at it.
            Helpers.EnsureResolve(t);

            return t;
        }

        public override FieldInfo ResolveField(int metadataToken, Type[] genericTypeArguments, Type[] genericMethodArguments)
        {
            // This handles FieldDef, MemberRef, 
            // Just as with code:ResolveType, the generic args are used to resolve specs, but not used for
            // FieldDefs. Callers must resolve the type and then call GetField() on the resolved generic type.
            return this.ResolveFieldTokenInternal(MetadataTokens.EntityHandle(metadataToken), new GenericContext(genericTypeArguments, genericMethodArguments));
        }

        public override MethodBase ResolveMethod(int metadataToken, Type[] genericTypeArguments, Type[] genericMethodArguments)
        {
            // Don't use GetGenericMethod(), that has the wrong semantics around generics. It will apply generic args to a MethodDef.
            // Whereas ResolveMethod() should only apply the generic args to a ref / spec.
            return this.ResolveMethodTokenInternal(MetadataTokens.EntityHandle(metadataToken), new GenericContext(genericTypeArguments, genericMethodArguments));
        }

        public override MemberInfo ResolveMember(int metadataToken, Type[] genericTypeArguments, Type[] genericMethodArguments)
        {
            throw new NotImplementedException();
        }

        public override byte[] ResolveSignature(int metadataToken)
        {
            throw new NotImplementedException();
        }

        private const BindingFlags MembersDeclaredOnTypeOnly = BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance | BindingFlags.Static | BindingFlags.DeclaredOnly;

        /// <summary>
        /// Determine if this module is the symbol module, as decided by the TypeUniverse.
        /// </summary>
        /// <returns>Return true iff this is the system module (mscorlib). </returns>
        /// <remarks>This is needed if the caller wants to compare cached token or name values for
        /// builtin types.</remarks>
        internal bool IsSystemModule()
        {
            var u = this.AssemblyResolver;
            return u.GetSystemAssembly().Equals(this.Assembly);

        }

        /// <summary>
        /// Determine if the module is a windows runtime metadata module
        /// </summary>
        /// <param name="module">The module to determine if it is a windows runtime metadata module</param>
        /// <returns>True if the module is a windows runtime metadata module</returns>
        static internal bool IsWindowsRuntime(Module module)
        {
            return module.Assembly.GetName().ContentType == AssemblyContentType.WindowsRuntime;
        }        

        // Hang this on module so that we have access to a shared readonly mapping.
        internal TypeCode GetTypeCode(Type type)
        {
            // TypeCode of an enum is that of its underlying type.
            // Check this before checking for system assembly since user-defined Enums shouldn't be
            // returning Object.
            if (type.IsEnum)
            {
                type = MetadataOnlyModule.GetUnderlyingType(type);
                return Type.GetTypeCode(type);
            }

            // If we're not in the system assembly, then typecode is just Object.
            if (!IsSystemModule())
            {
                return TypeCode.Object;
            }


            var typeDefHandle = MetadataTokens.TypeDefinitionHandle(type.MetadataToken);
            // Derived class should have taken care of this first.
            


            if (m_typeCodeMapping == null)
            {
                m_typeCodeMapping = CreateTypeCodeMapping();
            }

            // Lookup well known types
            for (int i = 0; i < m_typeCodeMapping.Length; i++)
            {
                if (typeDefHandle == m_typeCodeMapping[i])
                    return (TypeCode)i;
            }

            // If if it's not in the well-known list, just assume object.
            return TypeCode.Object;
        }


        /// <summary>
        /// Return a mapping for code:m_typeCodeMapping. See that field for exact semantics of this array.
        /// This must be called from the assembly's module.
        /// </summary>
        private TypeDefinitionHandle[] CreateTypeCodeMapping()
        {
            // This must be called from the system assembly. All type name lookups are system types in the
            // system assembly.
            Debug.Assert(this.IsSystemModule());

            return new TypeDefinitionHandle[] {
                default,  // TypeCode.Empty
                LookupTypeDefHandle("System.Object"), // 1
                LookupTypeDefHandle("System.DBNull"),
                LookupTypeDefHandle("System.Boolean"),
                LookupTypeDefHandle("System.Char"),
                LookupTypeDefHandle("System.SByte"),
                LookupTypeDefHandle("System.Byte"),
                LookupTypeDefHandle("System.Int16"),
                LookupTypeDefHandle("System.UInt16"),
                LookupTypeDefHandle("System.Int32"),
                LookupTypeDefHandle("System.UInt32"),
                LookupTypeDefHandle("System.Int64"),
                LookupTypeDefHandle("System.UInt64"),
                LookupTypeDefHandle("System.Single"),
                LookupTypeDefHandle("System.Double"),
                LookupTypeDefHandle("System.Decimal"),
                LookupTypeDefHandle("System.DateTime"), // 17 == TypeCode.DateTime
                default, // skipped
                LookupTypeDefHandle("System.String"), // 18 == TypeCode.String
            };
        }

        #region IDisposable Members

#pragma warning disable 0414
        private bool disposed = false;
#pragma warning restore 0414

        /// <summary>
        /// Dispose this module. This should be called in the context of disposing the parent assembly.
        /// This will release the unmanaged metadata pointers this module owns.
        /// Caller is responsible for thread safey here and to not dispose while another thread is using.
        /// Caller should not use after this has been diposed.
        /// </summary>
        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
            disposed = true;
        }

        // The bulk of the clean-up code is implemented in Dispose(bool)
        protected virtual void Dispose(bool disposing)
        {
            if (disposing)
            {
                // Dispose the PEReader which owns the file stream.
                // MetadataReader has no resources to dispose (it piggybacks on PEReader).
                if (m_peReader != null)
                {
                    m_peReader.Dispose();
                }
            }
        }


        #endregion


        #region IModule2 members

        /// <summary>
        /// Gets number of rows in a metadata table.
        /// </summary>
        public int RowCount(MetadataTable metadataTableIndex)
        {
            return m_reader.GetTableRowCount((TableIndex)(int)metadataTableIndex);
        }


        #endregion // IModule2 members


        public override void GetPEKind(out System.Reflection.PortableExecutableKinds peKind, out System.Reflection.ImageFileMachine machine)
        {
            // Get PE kind from the PE headers via PEReader
            var peHeaders = m_peReader.PEHeaders;
            peKind = 0;
            if (peHeaders.CorHeader != null)
            {
                if ((peHeaders.CorHeader.Flags & CorFlags.ILOnly) != 0)
                {
                    peKind |= System.Reflection.PortableExecutableKinds.ILOnly;
                }

                if ((peHeaders.CorHeader.Flags & CorFlags.Requires32Bit) != 0)
                {
                    peKind |= System.Reflection.PortableExecutableKinds.Required32Bit;
                }
            }

            if (peHeaders.PEHeader != null && peHeaders.PEHeader.Magic == PEMagic.PE32Plus)
            {
                peKind |= System.Reflection.PortableExecutableKinds.PE32Plus;
            }
            
            machine = (System.Reflection.ImageFileMachine)peHeaders.CoffHeader.Machine;
        }

        public override int MDStreamVersion
        {
            get
            {
                // The Metadata version is from the '#~ stream' structure in the metadata blob. See II 24.2.6 for details.
                // It should be (minor | (major << 16)).
                // This is 0 for resources, but a MOModule doesn't represent resources.
                throw new NotImplementedException();
            }
        }

        public string GetRuntimeVersion()
        {
            return m_reader.MetadataVersion;
        }

    } // end class MetadataOnlyModule
}
