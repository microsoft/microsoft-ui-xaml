// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using Debug=Microsoft.UI.Xaml.Markup.Compiler.Lmr.Internal.Debug;
using System.Collections.Generic;
using System.Reflection.Adds;
using System.Reflection.Metadata;
using System.Reflection.Metadata.Ecma335;
using System.Text;

using System.Reflection;  

namespace Microsoft.UI.Xaml.Markup.Compiler.Lmr
{
    /// <summary>
    /// Represent a LMR implementation of System.Reflection.MethodInfo. 
    /// This is for MethodInfos backed by real metadata with real a MethodDef token. 
    /// Use a different derived class for fabricated methodInfos.
    /// </summary>
    internal class MetadataOnlyMethodInfo : MethodInfo 
    {
        // !!! IMPORTANT !!!
        //
        // If any new field is added make sure that copy constructor is updated!
        //
        // !!!!!!!!!!!!!!!!!

        #region Fields
        //The token of our function
        readonly private MethodDefinitionHandle m_methodDef;

        // The name of the method (not including class name)
        private string m_name;

        // The type that this method is defined on. 
        // This can be used to get the class name and assemble a full-name.
        private Type m_tOwner;

        // This is important for signature resolution.
        // We'll also eventually need the parameter types in order to have accurate signature comparisons.
        private MethodSignatureDescriptor m_descriptor;

        private ParameterInfo m_returnParameter;

        // Cached value of method body.
        private MethodBody m_methodBody;

        private MethodAttributes m_attrs;

        // m_typeArgs & m_methodArgs are just used to save initial context before 
        // an object is fully initialized. After an object is fully initialized,
        // m_context contains valid context and should be used instead of other two.
        readonly private Type[] m_typeArgs;
        readonly private Type[] m_methodArgs;
        private GenericContext m_context;

        readonly private MetadataOnlyModule m_resolver;
        private TypeDefinitionHandle m_declaringTypeDef;
        private BlobReader m_sigBlob;

        // Used for lazy initialization. 
        private bool m_fullyInitialized;

        #endregion // Fields

        /// <summary>
        /// Helper method to create a MethodInfo or a Constructor
        /// </summary>
        /// <param name="resolver">module resolution scope</param>
        /// <param name="methodDef">metadata token for methodDef of method body</param>
        /// <param name="context">generic arguments</param>
        internal static MethodBase Create(MetadataOnlyModule resolver, MethodDefinitionHandle methodDef, GenericContext context)
        {            
            var typeArgs = Type.EmptyTypes;
            var methodArgs = Type.EmptyTypes;
            if (context != null)
            {
                typeArgs = context.TypeArgs;
                methodArgs = context.MethodArgs;
            }

            var m = resolver.Factory.CreateMethodOrConstructor(resolver, methodDef, typeArgs, methodArgs);

            return m;
        }

        /// <summary>
        /// Copy constructor for a MethodInfo. This allows a derived class to easily override a MethodInfo
        /// without having to wrap every property.
        /// </summary>
        public MetadataOnlyMethodInfo(MetadataOnlyMethodInfo method)
        {
            m_resolver = method.m_resolver;
            m_methodDef = method.m_methodDef;
            m_tOwner = method.m_tOwner;

            m_descriptor = method.m_descriptor;

            m_name = method.m_name;
            m_attrs = method.m_attrs;
            m_returnParameter = method.m_returnParameter;

            m_methodBody = method.m_methodBody;
            m_declaringTypeDef = method.m_declaringTypeDef;
            m_sigBlob = method.m_sigBlob;

            m_typeArgs = method.m_typeArgs;
            m_methodArgs = method.m_methodArgs;
            m_context = method.m_context;

            m_fullyInitialized = method.m_fullyInitialized;
        }

        /// <summary>
        /// Internal constructor for creating a MethodInfo. If method is a constructor, it must get wrapped in
        /// a ConstructorInfo object before being handed out.
        /// </summary>
        public MetadataOnlyMethodInfo(MetadataOnlyModule resolver, MethodDefinitionHandle methodDef, Type[] typeArgs, Type[] methodArgs)
        {
            Debug.Assert(resolver != null);
            m_resolver = resolver;

            m_methodDef = methodDef;

            m_typeArgs = typeArgs;
            m_methodArgs = methodArgs;

            // Just get some core method properties. If more information about the method is needed
            // we'll get it from metadata on demand.
            resolver.GetMethodAttrs(methodDef, out m_declaringTypeDef, out m_attrs);
        }

        private void InitializeName()
        {
            if (string.IsNullOrEmpty(m_name))
            {
                m_name = m_resolver.GetMethodName(m_methodDef);
            }
        }

        /// <summary>
        /// Get all the non-core information about the method from metadata. This is relatively expensive
        /// so we perform it only when needed. 
        /// </summary>
        /// <remarks>We have to use lock here to avoid race in updating m_tOwner field.</remarks>
        private void Initialize()
        {
            // Makes sure m_tOwner and m_context is assinged to only once. If Initialize is called on multiple
            // threads it will still be fine since it will just assign the same value on each thread. 

            //
            // Part 1: fetch all the results into locals. 
            //

            // Get information about declaring type and its type arguments (if any).
            //
            Type ownerType = null;
            Type[] typeArgs = null;
            if (!m_declaringTypeDef.IsNil)
            {                
                GetOwnerTypeAndTypeArgs(out ownerType, out typeArgs);                
            }
            else
            {
                // If the method is a global method in a module, the declaringTypeDef is invalid.
                // In that case, don't resolve the token and just construct type arguments
                // part of final context for a global method.
                typeArgs = m_typeArgs;
            }

            // Get information about method arguments (if any).
            //
            Type[] methodArgs = GetGenericMethodArgs();
            var tempContext = new GenericContext(typeArgs, methodArgs);

            m_sigBlob = m_resolver.GetMethodSignature(m_methodDef);
            var methodDef = m_resolver.RawReader.GetMethodDefinition(m_methodDef);
            MethodSignatureDescriptor descr = SignatureUtil.FromSrmSignature(
                methodDef.DecodeSignature(m_resolver.TypeProvider, tempContext));


            //
            // Part 2: Commit all the fields at once. This avoids the object being in a partially initialized state.
            //

            m_tOwner = ownerType;
            m_context = tempContext;
            m_descriptor = descr;

            // Check the calling convention. Since this is a method def, the sig should refer to an uninstantiated
            // method if it is generic, so it shouldn't be GenericInst.
            Debug.Assert(m_descriptor.CallingConvention != CorCallingConvention.GenericInst);

            m_fullyInitialized = true;

        }

        /// <summary>
        /// Retreives information about containing type and its context from metadata.
        /// </summary>
        private void GetOwnerTypeAndTypeArgs(out Type ownerType, out Type[] typeArgs)
        {
            Type type = m_resolver.ResolveTypeDef(m_declaringTypeDef);
            GenericContext context = new GenericContext(m_typeArgs, m_methodArgs);

            // If type is generic and we didn't get type arguments in constructor read them
            // from metadata. 
            if (type.IsGenericType && GenericContext.IsNullOrEmptyTypeArgs(context))
            {
                // Update the generic context with the owner type's generic arguments
                context = new GenericContext(type.GetGenericArguments(), m_methodArgs);
            }

            // Propogate generic type arguments to declaring type.
            type = m_resolver.GetGenericType((EntityHandle)m_declaringTypeDef, context);

            ownerType = type;
            typeArgs = context.TypeArgs;
        }

        /// <summary>
        /// Retreives information about method's generic arguments if there are any.
        /// </summary>
        private Type[] GetGenericMethodArgs()
        {
            Type[] methodArgs = null;

            int genericMethodParamCount = m_resolver.CountGenericParams((EntityHandle)m_methodDef);
            bool methodParametersPresent = (m_methodArgs != null) && (m_methodArgs.Length > 0);
            if (genericMethodParamCount > 0)
            {
                if (!methodParametersPresent)
                {
                    // Constructing fully open generic method.
                    methodArgs = new Type[genericMethodParamCount];
                    int i = 0;
                    foreach (GenericParameterHandle gpHandle in m_resolver.GetGenericParameterHandles((EntityHandle)m_methodDef))
                    {
                        methodArgs[i++] = m_resolver.Factory.CreateTypeVariable(m_resolver, gpHandle);
                    }
                }
                else
                {
                    // Constructing closed, paritialy closed, or open generic method.
                    // Number of parameters must match.
                    if (genericMethodParamCount != m_methodArgs.Length)
                    {
                        throw new ArgumentException(Resources.WrongNumberOfGenericArguments);
                    }
                    else
                    {
                        methodArgs = m_methodArgs;
                    }
                }
            }

            return (methodArgs == null) ? Type.EmptyTypes : methodArgs;
        }

        public override int MetadataToken { get { return MetadataTokens.GetToken(m_methodDef); } }

        internal MetadataOnlyModule Resolver
        {
            get { return m_resolver; }
        }

        public override Module Module
        {
            get { return m_resolver; }
        }

        public override Type ReturnType
        {
            get 
            {
                if (!m_fullyInitialized)
                {
                    Initialize();
                }
                return m_descriptor.ReturnParameter.Type; 
            }
        }

        public override bool Equals(object obj)
        {
            MetadataOnlyMethodInfo mi = obj as MetadataOnlyMethodInfo;

            if (mi == null)
            {
                return false;
            }

            if (!this.DeclaringType.Equals(mi.DeclaringType))
            {
                return false;
            }

            if (!this.IsGenericMethod)
                //the methods are equal if they have the same resolver and same token.
                return mi.GetHashCode() == this.GetHashCode();

            if (!mi.IsGenericMethod)
                return false;

            // now we know that both operands are generic methods

            Type[] lhs = this.GetGenericArguments();
            Type[] rhs = mi.GetGenericArguments();

            if (lhs.Length != rhs.Length)
                return false;

            for (int i = 0; i < lhs.Length; i++)
            {
                if (!lhs[i].Equals(rhs[i]))
                    return false;
            }

            return true;
        }

        public override int GetHashCode()
        {
            return m_resolver.GetHashCode() * 32767 + m_methodDef.GetHashCode();
        }

        //The following shows some sample outputs of MethodInfo::ToString().
        //"Void Foo()"
        //"Int32 Foo(Int32, System.String)"
        //"System.Collections.Generic.List`1[System.Int32] Foo(System.Collections.Generic.List`1[System.String])"
        //"Int32 Foo[T,U](T, Int32)"
        //"Int32 Foo(Int32 ByRef, System.String ByRef)"
        //"Void Foo(System.String, ...)"
        public override string ToString()
        {
            return CommonToString(this);            
        }

        static internal string CommonToString(MethodInfo m)
        {
            StringBuilder sb = StringBuilderPool.Get();
            MetadataOnlyCommonType.TypeSigToString(m.ReturnType, sb);
            sb.Append(" ");
            ConstructMethodString(m, sb);

            string result = sb.ToString();
            StringBuilderPool.Release(ref sb);
            return result;
        }
        
        static private void ConstructMethodString(MethodInfo m, StringBuilder sb)
        {
            sb.Append(m.Name);
            string comma = "";
            if (m.IsGenericMethod)
            {
                sb.Append("[");
                //To the format of MethodName[TArg1, TArg2, ...]
                foreach (var tArg in m.GetGenericArguments())
                {
                    sb.Append(comma);
                    MetadataOnlyCommonType.TypeSigToString(tArg, sb);
                    comma = ",";
                }
                sb.Append("]");
            }
            sb.Append("(");
            ConstructParameters(sb, m.GetParameters(), m.CallingConvention);
            sb.Append(")");
        }

        static private void ConstructParameters(StringBuilder sb, ParameterInfo[] parameters, CallingConventions callingConvention)
        {
            Type[] parameterTypes = new Type[parameters.Length];

            for (int i = 0; i < parameters.Length; i++)
                parameterTypes[i] = parameters[i].ParameterType;

            ConstructParameters(sb, parameterTypes, callingConvention);
        }

        static private void ConstructParameters(StringBuilder sb, Type[] parameters, CallingConventions callingConvention)
        {
            string comma = "";

            for (int i = 0; i < parameters.Length; i++)
            {
                Type t = parameters[i];

                sb.Append(comma);
                MetadataOnlyCommonType.TypeSigToString(t, sb);
                if (t.IsByRef)
                {
                    sb.Length--;
                    sb.Append(" ByRef");
                }

                comma = ", ";
            }

            if ((callingConvention & CallingConventions.VarArgs) == CallingConventions.VarArgs)
            {
                sb.Append(comma);
                sb.Append("...");
            }
        }

        // Get the Type that this is declared in.
        public override Type DeclaringType
        {
            get
            {
                if (!m_fullyInitialized)
                {
                    Initialize();
                }
                return m_tOwner;
            }
        }

        // Name of just the method (not type).
        public override string Name
        {
            get { InitializeName(); return m_name; }
        }

        public override object[] GetCustomAttributes(bool inherit)
        {
            throw new NotSupportedException();
        }

        public override object[] GetCustomAttributes(Type attributeType, bool inherit)
        {
            throw new NotSupportedException();
        }

        public override bool IsDefined(Type attributeType, bool inherit)
        {
            throw new NotSupportedException();
        }

        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1065:DoNotRaiseExceptionsInUnexpectedLocations")]
        public override Type ReflectedType
        {
            get { throw new NotSupportedException(); }
        }

        /// <summary>
        /// Get parameters, not including this arg and get return parameter 
        /// (if it's present).
        /// </summary>
        public override ParameterInfo[] GetParameters()
        {
            if (!m_fullyInitialized)
            {
                Initialize();
            }

            // We know the number of parameters based from when we loaded the signature.
            int numParams = m_descriptor.Parameters.Length;
            ParameterInfo[] parameters = new ParameterInfo[numParams];

            Type[] paramTypes = new Type[numParams];
            for (int i = 0; i < numParams; i++)
            {
                paramTypes[i] = m_descriptor.Parameters[i].Type;
            }

            // Get parameters from metadata using SRM
            var methodDef = m_resolver.RawReader.GetMethodDefinition(m_methodDef);
            var paramHandles = methodDef.GetParameters();
            ParameterInfo returnParameter = null;

            if (paramHandles.Count == 0)
            {
                // No parameter metadata - use fake params from signature
                for (int i = 0; i < numParams; i++)
                {
                    parameters[i] = this.Resolver.Policy.GetFakeParameterInfo(this, paramTypes[i], i);
                }
                return parameters;
            }

            foreach (var paramHandle in paramHandles)
            {
                var param = m_resolver.RawReader.GetParameter(paramHandle);
                int sequence = param.SequenceNumber;

                if (sequence == 0)
                {
                    // Return parameter has sequence = 0
                    returnParameter = new MetadataOnlyParameterInfo(
                        m_resolver,
                        paramHandle,
                        this.ReturnType,
                        m_descriptor.ReturnParameter.CustomModifiers);
                }
                else
                {
                    var idx = sequence - 1;
                    Debug.Assert(idx < numParams, "Parameter sequence number exceeds method signature parameter count");
                    if (idx < numParams)
                    {
                        parameters[idx] = new MetadataOnlyParameterInfo(
                            m_resolver,
                            paramHandle,
                            paramTypes[idx],
                            m_descriptor.Parameters[idx].CustomModifiers);
                    }
                }
            }

            if (returnParameter == null)
            {
                returnParameter = this.Resolver.Policy.GetFakeParameterInfo(this, this.ReturnType, -1);
            }
            m_returnParameter = returnParameter;

            // Fill in any missing parameters with fakes
            for (int i = 0; i < numParams; i++)
            {
                if (parameters[i] == null)
                {
                    parameters[i] = this.Resolver.Policy.GetFakeParameterInfo(this, paramTypes[i], i);
                }
            }

            return parameters;
        }

        public override ParameterInfo ReturnParameter
        {
            get 
            {
                if (m_returnParameter == null)
                {
                    // Initialize the return parameter by calling GetParameters().
                    // This will also fully initialize this object if it wasn't already.
                    GetParameters();
                }

                //We return a faked parameter if GetParameters() doesn't initialize the field.
                //This happens when the method has no parameters.
                //We do it in this way instead of initializing the field here is for the thread
                //safety to make the non-readonly field be only assigned at one place.
                if (m_returnParameter == null)
                {
                    return this.Resolver.Policy.GetFakeParameterInfo(this, this.ReturnType, -1);
                }
                else
                {
                    return m_returnParameter;
                }
            }
        }

        public override MethodAttributes Attributes
        {
            get { return m_attrs; }
        }

        public override CallingConventions CallingConvention
        {
            get
            {
                if (!m_fullyInitialized)
                {
                    Initialize();
                }

                CorCallingConvention corCallingConvention = m_descriptor.CallingConvention;
                CallingConventions callingConv;
                if ((corCallingConvention & CorCallingConvention.Mask) == CorCallingConvention.VarArg)
                    callingConv = CallingConventions.VarArgs;
                else
                    callingConv = CallingConventions.Standard;

                if ((corCallingConvention & CorCallingConvention.HasThis) != 0)
                    callingConv |= CallingConventions.HasThis;

                if ((corCallingConvention & CorCallingConvention.ExplicitThis) != 0)
                    callingConv |= CallingConventions.ExplicitThis;
                return callingConv;
            }
        }

        public override MemberTypes MemberType
        {
            get { return MemberTypes.Method; }
        }


        #region Generics
        public override bool IsGenericMethodDefinition
        {
            get
            {
                if (!m_fullyInitialized)
                {
                    Initialize();
                }

                // True for Class::Func<T>(), 
                // False for Class::Func(), Class<int>::Func(), Class::Func<int>()
                bool generic = (m_descriptor.CallingConvention & CorCallingConvention.Generic) != 0;

                if (!generic)
                    return false;

                // The methdod signature has type arguments. 

                // We may have a context with type args, but no method args.
                // Eg, Class<int>::Func<T>()
                if (GenericContext.IsNullOrEmptyMethodArgs(m_context))
                {
                    return true;
                }

                // Get completely open version of this method so we can compare it with DeclaringMethod
                // from generic method arguments. They are always created in completely open form.
                MethodInfo openMethod = this.Resolver.Factory.CreateMethodOrConstructor(this.Resolver, m_methodDef, null, null) as MethodInfo;

                // If all the method args in the context are generic type variables (like 'T')
                // and all are declared in this method, it is a generic method definition.
                foreach (Type arg in m_context.MethodArgs)
                {
                    if (!arg.IsGenericParameter)
                    {
                        // This argument is instantiated, so this can't be generic
                        // method definition.
                        return false;
                    }
                    else if (!openMethod.Equals(arg.DeclaringMethod) )
                    {
                        // Note: arg.DeclaringMethod could be null so it's
                        // important to use it on the right hand side of .Equals.
                        return false;
                    }
                }

                return true;
            }
        }

        public override bool IsGenericMethod
        {
            get
            {
                if (!m_fullyInitialized)
                {
                    Initialize();
                }

                return !GenericContext.IsNullOrEmptyMethodArgs(m_context);
            }
        }

        public override MethodInfo MakeGenericMethod(params Type[] types)
        {
            // Need unresolved generic args to make a generic method.
            // This will also fully initialize this object.
            if (!IsGenericMethodDefinition)
            {
                throw new InvalidOperationException();
            }

            Type[] typeArgs = m_context.TypeArgs;
            Type[] methodArgs = types;
            GenericContext ctx = new GenericContext(typeArgs, methodArgs);

            // Constructors are not generic.
            MethodInfo m = (MethodInfo)MetadataOnlyMethodInfo.Create(this.m_resolver, this.m_methodDef, ctx);

            return m;
        }

        public override Type[] GetGenericArguments()
        {
            // Initialize to get right generic context.
            if (!m_fullyInitialized)
            {
                Initialize();
            }

            return (Type[])m_context.MethodArgs.Clone();
        }

        public override MethodInfo GetGenericMethodDefinition()
        {
            if (!this.IsGenericMethod)
            {
                throw new InvalidOperationException();
            }

            if (this.IsGenericMethodDefinition)
            {
                return this;
            }
            else
            {
                return this.Resolver.Factory.CreateMethodOrConstructor(this.Resolver, m_methodDef, m_context.TypeArgs, null) as MethodInfo;
            }
        }

        public override bool ContainsGenericParameters
        {
            get
            {
                // If containing type has any open type arguments; all its
                // methods are considered to have them too (regardless if they
                // are generic methods or not).
                if (this.DeclaringType.ContainsGenericParameters)
                {
                    return true;
                }

                // If containing type doesn't have any open arguments or
                // is not a generic class; we need to check generic method
                // arguments (if there are any).
                Type[] genericArguments = GetGenericArguments();
                for (int i = 0; i < genericArguments.Length; i++)
                {
                    if (genericArguments[i].ContainsGenericParameters)
                        return true;
                }

                return false;
            }
        }

        #endregion // Generics

        // Returns null if the method does not have any IL code (such as an ecall into the runtime or a pinvoke)
        public override MethodBody GetMethodBody()
        {
            if (m_methodBody == null)
            {
                m_methodBody = MetadataOnlyMethodBody.TryCreate(this);
            }
            return m_methodBody;
        }

        public override System.Reflection.MethodImplAttributes GetMethodImplementationFlags()
        {
            return this.m_resolver.GetMethodImplFlags(this.m_methodDef);
        }

        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1065:DoNotRaiseExceptionsInUnexpectedLocations")]
        public override RuntimeMethodHandle MethodHandle
        {
            get { throw new NotSupportedException(); }
        }

        public override object Invoke(object obj, BindingFlags invokeAttr, Binder binder, object[] parameters, System.Globalization.CultureInfo culture)
        {
            throw new NotSupportedException();
        }

        public override MethodInfo GetBaseDefinition()
        {
            if (!this.IsVirtual || this.IsStatic || this.DeclaringType == null || this.DeclaringType.IsInterface)
                return this;

            //Look for the method with the same name and signature in the parent type of the containing type.
            var baseType = this.DeclaringType.BaseType;
            if (baseType == null)
            {
                return this;
            }
            List<Type> paramTypes = new List<Type>();
            foreach (ParameterInfo p in this.GetParameters())
            {
                paramTypes.Add(p.ParameterType);
            }
            var matchingMethod = baseType.GetMethod
                (
                    this.Name,
                    BindingFlags.FlattenHierarchy | BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic,
                    null, //binder
                    this.CallingConvention,
                    paramTypes.ToArray(),
                    null //parameter modifiers
                );

            if (matchingMethod == null)
            {
                return this;
            }

            return matchingMethod.GetBaseDefinition();
        }

        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1065:DoNotRaiseExceptionsInUnexpectedLocations")]
        public override System.Reflection.ICustomAttributeProvider ReturnTypeCustomAttributes
        {
            get { throw new NotSupportedException(); }
        }

        public override IList<CustomAttributeData> GetCustomAttributesData()
        {
            return this.Resolver.GetCustomAttributeData((EntityHandle)m_methodDef);
        }
    }

} // namespace
