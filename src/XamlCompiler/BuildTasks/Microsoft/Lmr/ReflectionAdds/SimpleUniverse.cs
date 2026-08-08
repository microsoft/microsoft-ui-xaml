// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

using System.Collections.Generic;
using System.Reflection.Metadata;
using Microsoft.UI.Xaml.Markup.Compiler.Lmr.Internal;
using System.Globalization;

namespace System.Reflection.Adds
{
    using System.Reflection;
    using Type = System.Type;

 

    /// <summary>
    /// A basic universe implementation that is agnostic to any particular type provider implementation. 
    /// This provides services like:
    /// - default implementations of universe methods 
    /// - tracking a list of loaded assemblies
    /// - picking the system assembly
    /// - default resolution policy, plus a resolution callback for further customization.
    /// </summary>
    internal class SimpleUniverse : IMutableTypeUniverse, IDisposable
    {
        // Mapping to cache types for GetBuiltInType, GetTypeXFromName
        Dictionary<string, Type> m_hash = new Dictionary<string, Type>();

        // List of loaded assemblies. We need this so that we know what to unload.
        // This can also be used to search for resolving assembly refs.
        private List<Assembly> m_loadedAssemblies = new List<Assembly>();

        // The system assembly (for implementing ITU.GetSystemAssembly()), or null if no
        // system assembly is identified yet.
        Assembly m_systemAssembly;


        // Determine if the assembly is in the loaded list.
        private bool IsAssemblyInList(Assembly assembly)
        {
            // Don't use IndexOf since that does ObjectReference identity.
            foreach (Assembly a in m_loadedAssemblies)
            {
                if (a.Equals(assembly))
                    return true;
            }
            return false;
        }


        #region New public APIs
        /// <summary>
        /// Get list of assemblies that have been explicitly loaded into this universe.
        /// Clients can use this to detect if an assembly is already loaded to avoid double-loading it. 
        /// This can also be used to search the entire set of types in the universe.
        /// </summary>
        public IEnumerable<Assembly> Assemblies
        {
            get { return this.m_loadedAssemblies; }
        }
                
        /// <summary>
        /// Register the assembly as part of this universe. This is agnostic to the assembly implementation.
        /// </summary>
        /// <param name="assembly">an assembly in this universe. The assembly should implement IAssembly2
        /// and already be associated with this universe.</param>
        /// <remarks></remarks>
        public void AddAssembly(Assembly assembly)
        {
            // Impl of IMutableTypeUniverse

            var ia2 = (IAssembly2)assembly;
            Debug.Assert(ia2.TypeUniverse == this);

            this.m_loadedAssemblies.Add(assembly);
        }

        /// <summary>
        /// Explicitly set the system assembly.  This will be used by ITypeUniverse.GetSystemAssembly(). 
        /// </summary>
        /// <param name="systemAssembly">The system assembly. This should already be in this universe.</param>
        /// <remarks>If this isn't called ITU.GetsystemAssembly() will use heuristics to select 
        /// the most likely assembly.</remarks>
        public void SetSystemAssembly(Assembly systemAssembly)
        {
            if (systemAssembly == null)
            {
                throw new ArgumentNullException("systemAssembly");
            }
            Debug.Assert(IsAssemblyInList(systemAssembly), "System assembly should already have been added to this universe.");
            m_systemAssembly = systemAssembly;
        }

        /// <summary>
        /// Invoke the delegate when an assembly name doesn't resolve to within the loaded set.
        /// The delegate implementation can then add the assembly (or even multiple assemblies) into the universe
        /// and return the assembly it resolves to. 
        /// 
        /// This is only fired if the default implementation of ResolveAssembly can't match the assembly name
        /// to within the loaded set. Override ResolveAssembly to get ealier notification.
        /// </summary>
        public event EventHandler<ResolveAssemblyNameEventArgs> OnResolveEvent;



        #endregion

        #region ITypeUniverse Members
        // Impl  ITyepUniverse
        public virtual Type GetBuiltInType(CorElementType elementType)
        {
            // We could use a more efficient mapping by hashing on elementType, since that's an integer.
            string s = ElementTypeUtility.GetNameForPrimitive(elementType);
            return GetTypeXFromName(s);
        }
        
        // Impl ITypeUniverse
        public virtual Type GetTypeXFromName(string fullName)
        {
            // Cache the name lookup. This has been a big perf win, especially for lookup up builtin types.
            Type val;
            if (!m_hash.TryGetValue(fullName, out val))
            {
                val = this.GetSystemAssembly().GetType(fullName, true, false);
                m_hash[fullName] = val;
            }
            return val;

        }

        // Impl ITypeUniverse.
        public virtual Assembly GetSystemAssembly()
        {
            if (m_systemAssembly == null)
            {
                var a = FindSystemAssembly();
                if (a != null)
                {
                    SetSystemAssembly(a); // this validates the results of Find
                }
            }

            if (m_systemAssembly == null)
            {
                throw new UnresolvedAssemblyException(
                    string.Format(CultureInfo.InvariantCulture, Resources.CannotDetermineSystemAssembly));                
                
            }

            return m_systemAssembly;
        }

        public virtual Assembly GetSystemRuntimeAssembly()
        {
            return null;
        }
                
        /// <summary>
        /// Search amongst the loaded assemblies for one that may be the system assembly
        /// </summary>
        /// <returns>the system assembly, or null if none found.</returns>
        /// <remarks>This can't invoke assembly resolve events. It's tempting to define the system assembly as
        /// ResolveAssembly("mscorlib"), but 
        /// 1) that chooses arbitrary policy for the assembly name (eg, "mscorlib"), 
        /// 2) that just punts the problem back to the host. </remarks>
        protected Assembly FindSystemAssembly()
        {
            // Avoid hard-coding the assembly name (eg, "mscorlib")  into our search. 
            
            // We use the system-assembly to lookup certain builtin types, such as System.Object.
            // The system assembly is the only one with no assembly references. 
            foreach (var a in this.m_loadedAssemblies)
            {
                int countRefs = a.GetReferencedAssemblies().Length;
                if (countRefs == 0)
                {
                    return a;
                }
            }
            return null;          
        }

        // Impl ITU
        public virtual Assembly ResolveAssembly(AssemblyName name)
        {
            return ResolveAssembly(name, true);
        }

        // Impl ITU
        public virtual Assembly ResolveAssembly(AssemblyName name, bool throwOnError)
        {
            // first try to resolve amongst our own set
            var assembly = this.TryResolveAssembly(name);
            if (assembly != null)
                return assembly;

            // Invoke a user resolution callback. 
            if (OnResolveEvent != null)
            {
                ResolveAssemblyNameEventArgs args = new ResolveAssemblyNameEventArgs(name);
                OnResolveEvent(this, args);
                assembly = args.Target;
                if (assembly != null)
                {
                    // Caller should have added assembly to set
                    Debug.Assert(IsAssemblyInList(assembly), "OnResolveEvent delegate did not call AddAssembly() on result");
                }
            }


            if (assembly == null)
            {
                if(throwOnError) 
                {
                    throw new UnresolvedAssemblyException(string.Format(
                        CultureInfo.InvariantCulture, Resources.UniverseCannotResolveAssembly, name));
                }
                else 
                {
                    return null;
                }
            }

            IAssembly2 ia2 = assembly as IAssembly2;
            if (ia2 == null)
            {
                // Ia2 provides the backpointer to the universe.
                throw new InvalidOperationException(Resources.ResolverMustResolveToValidAssembly);
            }
            if (ia2.TypeUniverse != this)
            {
                throw new InvalidOperationException(Resources.ResolvedAssemblyMustBeWithinSameUniverse);
            }

            return assembly;            
        }
   


        // Impl ITU
        // If a class wants to customize on these resolution arguments, it can subclass and 
        // override this method. 
        // If a caller needs to handle loader contexts, then it will need to override this method
        // so that it knows which context the resolution is occurring in.
        public virtual Assembly ResolveAssembly(Module scope, AssemblyReferenceHandle assemblyRefHandle)
        {
            // Provide a default implementation that forwards to the name-based overload.
            IModule2 im2 = (IModule2)scope;
            var name = im2.GetAssemblyNameFromAssemblyRef(assemblyRefHandle);
            return this.ResolveAssembly(name);
        }

        // Impl ITU        
        public virtual Module ResolveModule(Assembly containingAssembly, string moduleName)
        {
            // Modules have affinity to their containing Assembly.
            // So we'd need to know the assembly specific implementation to properly resolve
            // this module. 
            // This is also related to Assembly.LoadModule(string, byte[]), which is not implemented in LMR.

            // Punt on module resolution. Derived classes can provide this.
            throw new NotImplementedException();
        }

        // Impl ITU        
        public virtual bool WouldResolveToAssembly(AssemblyName name, Assembly assembly)
        {
            // Both AssemblyName.ReferenceMatchesDefinition and AssemblyName.ToString() 
            // are very slow operations. 
            // Some experimentaiton showed caching here to avoid those operations can give a 50% speed increase
            // of resolving in some scenarios. 
            // name.ToString() is a bad hash key because it's slow. The assembly name object may even have
            // identity (LMR assembly caches it since it's slow to compute) 
            // Use ReferenceMatchesDefinition since incoming name may be a partial match. For example,
            // it may not have the version or public key information. An incomplete name should match
            // against a more complete name.
            var defName = assembly.GetName();
            return System.Reflection.AssemblyName.ReferenceMatchesDefinition(name, defName);
        }

        /// <summary>
        /// Resolves a type from a Windows Runtime type name
        /// </summary>
        /// <param name="typeName">The name of the Windows Runtime type to resolve. 
        /// The name can be a simple type name or an assemblyQualifiedTypeName.</param>
        /// <param name="throwOnError">True to throw an exception if the type cannot be found otherwise
        /// false to return null when the type cannot be found</param>
        /// <param name="ignoreCase">true to perform a case-insensitive search for typeName, false to 
        /// perform a case-sensitive search for typeName.</param>
        /// <returns>Returns null if the assembly cannot be found otherwise return an assembly that contains a 
        /// definition for the assemblyQualifiedTypeName</returns>
        public Type ResolveWindowsRuntimeType(string typeName, bool throwOnError, bool ignoreCase)
        {
            string namespaceName;
            int indexOfPeriod;

            Debug.Assert(!TypeNameParser.IsCompoundType(typeName));

            // Look for the type in an assembly with the name of the full namespace name of the type. 
            // If the assembly is not found or the type is not defined in the assembly look in the 
            // parent namespace. Continue the search until the a assembly is found defining the type
            // or the top level namespace has been reached.
            namespaceName = typeName;
            indexOfPeriod = namespaceName.LastIndexOf('.');

            while(0 <= indexOfPeriod) 
            {
                Assembly assembly;
                Type type;

                namespaceName = namespaceName.Remove(indexOfPeriod);
                assembly = ResolveAssembly(new AssemblyName(namespaceName), false);

                if(null != assembly) 
                {
                    bool thowOnErrorWhileResolvingType = false;
                    type = assembly.GetType(typeName, thowOnErrorWhileResolvingType, ignoreCase);

                    if(null != type) 
                    {
                        return type;
                    }
                }

                indexOfPeriod = namespaceName.LastIndexOf('.');
            }

            if(throwOnError) 
            {
                throw new TypeLoadException(string.Format(
                        CultureInfo.InvariantCulture, Resources.WindowsRuntimeTypeNotFound, typeName));
            }
            else 
            {
                return null;
            }
        }

        #endregion


        // Helper method to check if any of the currently loaded assemblies matches the assembly name.
        // Returns null if not found. 
        protected Assembly TryResolveAssembly(AssemblyName name)
        {
            // Look down existing loaded assemblies for a match
            foreach (var s in this.m_loadedAssemblies)
            {
                if (WouldResolveToAssembly(name, s))
                    return s;
            }
            return null;
        }

        #region Dispose
        /// <summary>
        /// Dispose this universe. This will release all native metadata objects within this universes's 
        /// assemblies and modules.  
        /// Do not use this universe after it's disposed.
        /// Caller is responsible for thread-safety and ensuring no other threads are using universe once
        /// the dispose is called.
        /// </summary>
        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        // No finalizer, since there are no direct native resources to free.
        // The bulk of the clean-up code is implemented in Dispose(bool)
        protected virtual void Dispose(bool disposing)
        {
            if (disposing)
            {
                // Free managed resources and call Dispose() on children.
                
                // Walk all assemblies (which will walk modules) disposing to free up native metadata objects.
                if (m_loadedAssemblies != null)
                {
                    foreach (Assembly a in m_loadedAssemblies)
                    {
                        IDisposable d = a as IDisposable;
                        if (d != null)
                        {
                            d.Dispose();
                        }
                    }
                    m_loadedAssemblies = null;
                }
            }

            // No native resources to free.            
        }

        #endregion // Dipose
        
    } // end class

    /// <summary>
    /// Exception thrown when assembly can't resolve. 
    /// </summary>
    /// Having an internal exception class deriving from System.Exception causes the following FxCop violation.
    /// It is ok to suppress it now since the exception type is only internally used and is not supposed to be
    /// caught by external users.
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1064:ExceptionsShouldBePublic")]
    [Serializable]
    internal class UnresolvedAssemblyException : Exception
    {
        public UnresolvedAssemblyException(string message) :
            base(message)
        {
        }
        public UnresolvedAssemblyException() :
            base()
        {
        }
        public UnresolvedAssemblyException(string message, Exception innerException) :
            base(message, innerException)
        {
        }

        protected UnresolvedAssemblyException(System.Runtime.Serialization.SerializationInfo info, System.Runtime.Serialization.StreamingContext context)
            : base(info, context)
        {

        }
    }


    /// <summary>
    /// Assembly resolution callback.
    /// </summary>
    /// <param name="args">an assembly name for that could not be matched to an already loaded assembly. </param>
    /// <returns>The assembly that the name resolves to. </returns>
    [System.Diagnostics.DebuggerDisplay("Resolve for {Name}")]
    internal class ResolveAssemblyNameEventArgs : EventArgs
    {
        // $$$ Pass universe in?
        public ResolveAssemblyNameEventArgs(AssemblyName name)
        {
            Debug.Assert(name != null);
            this.Name = name;
        }

        // AssemblyName to resolve.
        public AssemblyName Name { get; internal set; }

        // Set to null if can't be found.
        public Assembly Target { get; set; }
    }

 
}
