// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using Debug=Microsoft.UI.Xaml.Markup.Compiler.Lmr.Internal.Debug;
using System.Reflection.Adds;

using System.Reflection;  

namespace Microsoft.UI.Xaml.Markup.Compiler.Lmr
{
    /// <summary>
    /// Describes criteria used to filter methods. 
    /// This filter will find methods that have right "shape" i.e. that share same:
    ///     a) name
    ///     b) generic or not; if generic share the same number of generic arguments
    ///     c) number of parameters
    ///     d) calling convention
    /// </summary>
    internal class MethodFilter
    {
        public MethodFilter(string name, int genericParameterCount, int parameterCount, CorCallingConvention callingConvention)
        {
            Debug.Assert(!String.IsNullOrEmpty(name), "method name can't be null or empty");
            Debug.Assert(genericParameterCount >= 0, "genericParameterCount must be non-negative");
            Debug.Assert(parameterCount >= 0, "parameterCount must be non-negative");

            this.Name = name;
            this.GenericParameterCount = genericParameterCount;
            this.ParameterCount = parameterCount;
            this.CallingConvention = callingConvention;
        }

        /// <summary>
        /// Name of a method
        /// </summary>
        public string Name { get; set; }

        /// <summary>
        /// Number of generic parameters that method has.
        /// </summary>
        public int GenericParameterCount { get; set; }

        /// <summary>
        /// Number of arguments that method has.
        /// </summary>
        public int ParameterCount { get; set; }

        /// <summary>
        /// Calling convention that method uses.
        /// </summary>
        public CorCallingConvention CallingConvention { get; set; }
    }

    /// <summary>
    /// Compares method signatures (calling convention, parameter types, etc) to find matches. 
    /// </summary>
    /// <remarks>Comparing signatures is safer and more flexible than converting signature blobs, 
    /// which requires complete knowledge of signature blob structure, including all corner cases
    /// and accross all metadata versions.</remarks>
    internal static class SignatureComparer
    {
        /// <summary>
        /// Filters methods based on passed filter.
        /// </summary>
        /// <returns>Subset of methods that have requested "shape".</returns>
        public static IEnumerable<MethodBase> FilterMethods(MethodFilter filter, MethodInfo[] allMethods)
        {
            List<MethodBase> result = new List<MethodBase>();

            CallingConventions callConvention = SignatureUtil.GetReflectionCallingConvention(filter.CallingConvention);

            // We could cache results of method.IsGenericMethod and method.GetParameters()
            // since we might need them again. 
            foreach (MethodInfo method in allMethods)
            {
                if (method.Name.Equals(filter.Name, StringComparison.Ordinal) &&
                    SignatureUtil.IsCallingConventionMatch(method, callConvention) &&
                    SignatureUtil.IsGenericParametersCountMatch(method, filter.GenericParameterCount) &&
                    (method.GetParameters().Length == filter.ParameterCount))
                {
                    result.Add(method);
                }
            }

            return result;
        }

        /// <summary>
        /// Filters constructors based on passed filter.
        /// </summary>
        /// <returns>Subset of constructors that have requested "shape".</returns>
        public static IEnumerable<MethodBase> FilterConstructors(MethodFilter filter, ConstructorInfo[] allConstructors)
        {
            List<MethodBase> result = new List<MethodBase>();

            CallingConventions callConvention = SignatureUtil.GetReflectionCallingConvention(filter.CallingConvention);

            // We could cache results of constructor.GetParameters()
            // since we might need them again. 
            foreach (ConstructorInfo constructor in allConstructors)
            {
                if (constructor.Name.Equals(filter.Name, StringComparison.Ordinal) &&
                    SignatureUtil.IsCallingConventionMatch(constructor, callConvention) &&
                    (constructor.GetParameters().Length == filter.ParameterCount))
                {
                    result.Add(constructor);
                }
            }

            return result;
        }

        /// <summary>
        /// Determines if method's parameters match corresponding parameter descriptors.
        /// </summary>
        /// <remarks>
        /// Method parameters come from open version of declaring type since that's what's described
        /// in a signature blob. For non-generic types, this is just direct list of method's parameters.
        /// </remarks>
        internal static bool IsParametersTypeMatch(MethodBase templateMethod, TypeSignatureDescriptor[] parameters)        
        {
            Debug.Assert(templateMethod != null, "templateMethod should not be null.");
            Debug.Assert(parameters != null, "parameters array should not be null.");

            ParameterInfo[] templateMethodParameters = templateMethod.GetParameters();
            int numParams = templateMethodParameters.Length;
            Debug.Assert(numParams == parameters.Length, "Incorrect method filtering.");

            for (int i = 0; i < numParams; i++)
            {
                // There are three possible cases here:
                //  1) Method parameter was not generic => make sure templateType is the exact same type.
                //  2) Method parameter was generic that was instantiated => make sure templateType has matching kind and index.
                //  3) Method parameter is open generic variable (e.g. "T") => there are two sub-cases:
                //      a) both sides are type variables based on typeDef or methodDef => make sure they are the exact same.
                //      b) descriptor contains type variable based on memberRef (i.e. signature blob contained variable's 
                //      index only - like MVar!!0) => we know this kind of type in descriptor is represented by LMR type
                //      MetadataOnlyTypeVariableRef so make sure to use its .Equals implementation since it knows how to compare
                //      itself to typeDef or methodDef based type variables. 

                if (!parameters[i].Type.Equals(templateMethodParameters[i].ParameterType))
                {
                    return false;
                }
            }

            return true;
        }

        /// <summary>
        /// Compares signatures of all methods with the specified name on a given type to the passed
        /// signature (calling convention & parameter types) until a match is found. 
        /// </summary>
        /// <param name="methodName">Method name to look for. Could be constructor (.ctor or .cctor).</param>
        /// <param name="typeToInspect">Type whose methods should be inspected.</param>
        /// <param name="expectedSignature">Descriptor for expected method signature.</param>
        /// <returns>Returns method that matches specified signature or null if there is no match.</returns>
        /// <remarks>
        /// Method signature descriptor knows which parameters came from context (i.e. were instantiated) and which were
        /// not generic (i.e. were directly encoded). We have to make sure that overload we pick not only matches based
        /// on types of fully closed method's parameters but also that it has correct template from which it was
        /// instantiated. Note that multiple different open methods can end up with exact same parameters when they 
        /// are closed.
        /// 
        /// For example, given class C: 
        /// 
        /// class C<T> { int Foo(T i) {...} }
        /// 
        /// and call c.Foo(1) that is encoded in metadata like: 
        /// 
        /// MemberRef #4 (0a00001e)
        /// -------------------------------------------------------
        ///    Member: (0a00001f) Foo: 
        ///    CallCnvntn: [DEFAULT]
        ///    hasThis 
        ///    ReturnType: I4
        ///    3 Arguments
        ///        Argument #1:  Var!0
        /// 
        /// We can see that argument #1 is encoded as type variable and we know it must come from context.
        /// This matches above Foo definition since it's first parameter is indeed a type variable.
        /// 
        /// If class C was changed to something like:
        /// 
        /// class C<T> { int Foo(int i) {...} }
        /// 
        /// And if reference was not changed, there would be no match because all available methods require
        /// there are no parameters that should come from context.
        /// </remarks>        
        public static MethodBase FindMatchingMethod(
            string methodName,
            Type typeToInspect,
            MethodSignatureDescriptor expectedSignature,
            GenericContext context)
        {
            Debug.Assert(!String.IsNullOrEmpty(methodName), "methodName can't be null or empty");
            Debug.Assert(typeToInspect != null, "typeToInspect can't be null");
            Debug.Assert(expectedSignature != null, "expectedSignature can't be null");
            Debug.Assert(context != null, "context can't be null");

            bool isConstructor = methodName.Equals(".ctor", StringComparison.Ordinal) || methodName.Equals(".cctor", StringComparison.Ordinal);

            // If generic parameters are present it means we need to find generic method.
            int genericParameterCount = expectedSignature.GenericParameterCount;

            IEnumerable<MethodBase> candidateMethods = null;
            MethodFilter filter = new MethodFilter(
                methodName,
                genericParameterCount,
                expectedSignature.Parameters.Length,
                expectedSignature.CallingConvention);

            if (isConstructor)
            {
                Debug.Assert(genericParameterCount == 0, "constructor can't be generic method");
                candidateMethods = FilterConstructors(filter, typeToInspect.GetConstructors(MembersDeclaredOnTypeOnly));
            }
            else
            {
                // Get subset of methods that satisfies our filter.
                candidateMethods = FilterMethods(filter, typeToInspect.GetMethods(MembersDeclaredOnTypeOnly));
            }

            MethodBase result = null;            
            bool matchFound = false;

            // In order to find the right match we have to consider three components:
            //      1) candidate methods: pre-filtered methods on the target class. If there is a match one of
            //         these methods will be returned to the caller. candidateMethods could be fully closed, 
            //         fully opened, or anything in between depending on the context they were created in.
            //
            //      2) method signature descriptor: representation of method's signature as extracted from its
            //         signature blob. This contains more information then what candidateMethods have. E.g. it
            //         describes if type was encoded directly or taken out of generic context. Since multiple
            //         closed candidate methods could have exactly same signature we need to disambiguate between
            //         them by understanding open version they came from. That's why we need 3rd component:
            //
            //      3) template methods: if our target class is generic class, each candidate method has coresponding
            //         template method that it came from (when generic arguments were replaced with "real" types). If
            //         target class is not generic we can consider template methods to be same as candidate methods.
            //
            // When we have these three components we have enough information to find the right match even in cases
            // when there are seemingly ambigous overloads present. 

            foreach (MethodBase method in candidateMethods)
            {
                // Fully instantiate any generic parameters on a candidate method (if any) since that's
                // what we are expected to return if match is found. 
                //
                MethodBase candidateMethod = method;
                bool isGenericMethod = false;
                if ((genericParameterCount > 0) && (context.MethodArgs.Length > 0))
                {
                    candidateMethod = (method as MethodInfo).MakeGenericMethod(context.MethodArgs);
                    isGenericMethod = true;
                }

                MethodBase templateMethod = null;
                if (typeToInspect.IsGenericType)
                {
                    // Get generic template from which this closed method was instantiated. 
                    // This will "erase" any generic parameter instantiations.
                    templateMethod = GetTemplateMethod(typeToInspect, candidateMethod.MetadataToken);
                }
                else if (isGenericMethod)
                {
                    // If this is generic method on non-generic class then we already
                    // have our template. 
                    templateMethod = method;
                }
                else
                {
                    // If this is non-generic method on non-generic class then template is
                    // same as candidateMethod (i.e. there is no template).
                    templateMethod = candidateMethod;
                }

                if (!isConstructor)
                {
                    // Check return type against expected signature.
                    if (!expectedSignature.ReturnParameter.Type.Equals((templateMethod as MethodInfo).ReturnType))
                    {
                        continue;
                    }
                }


                // Check all method parameters against expected signature.
                if (!IsParametersTypeMatch(templateMethod, expectedSignature.Parameters))
                {
                    continue;
                }

                if (!matchFound)
                {
                    result = candidateMethod;
                    matchFound = true;
                }
                else
                {
                    throw new AmbiguousMatchException();
                }
            }

            return result;
        }

        /// <summary>
        /// Gets "template" version of a method on a generic class.
        /// </summary>
        /// <param name="typeToInspect">Generic class we are inspecting.</param>
        /// <param name="methodToken">Token of a method on a class.</param>
        /// <returns>
        /// Open version of a method represented by method token. 
        /// </returns>
        /// <remarks>
        /// Example: assume we have class Foo<T> { void m(T t) {...} } and create closed 
        /// version of it: class Foo<int>. If we look at all the methods on closed version
        /// we'll get MethodInfo for: void m(int t). 
        /// 
        /// We can use this method to get back from m(int t) to it's "template" i.e. an
        /// open method this closed version was created from. 
        /// 
        /// Unfortunately, there is no direct way to do this e.g. something like MethodInfo.GetGenericMethodDefinition 
        /// for non-generic methods that are defined on generic classes.
        /// That's why we have to use this indirect way for accomplishing it.
        /// </remarks>
        private static MethodBase GetTemplateMethod(Type typeToInspect, int methodToken)
        {
            return typeToInspect.GetGenericTypeDefinition().Module.ResolveMethod(methodToken);
        }

        private const BindingFlags MembersDeclaredOnTypeOnly = BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance | BindingFlags.Static | BindingFlags.DeclaredOnly;
    }
}
