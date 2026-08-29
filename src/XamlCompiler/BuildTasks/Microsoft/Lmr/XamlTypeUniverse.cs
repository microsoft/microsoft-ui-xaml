// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using Microsoft.UI.Xaml.Markup.Compiler.Lmr;

namespace Microsoft.UI.Xaml.Markup.Compiler.Lmr
{
    internal class XamlTypeUniverse : System.Reflection.Adds.SimpleUniverse
    {
        Dictionary<String, Assembly> _asmNameCache = new Dictionary<string, Assembly>();
        Dictionary<String, Type> _resolvedTypes = new Dictionary<string, Type>();

        // The loader, which handles shared state (factory, dispenser), and provides convenience
        // operators for loading modules.
        Loader _loader;
        Assembly _systemAssembly;
        Assembly _systemRuntimeAssembly;
        Assembly _xamlProxyAssembly;

        public XamlTypeUniverse(bool useManagedProjections)
        {
            _loader = new Loader(this);
            _loader.Factory = new XamlReflectionFactory();
            if (useManagedProjections)
            {
                _loader.ReaderOptions = System.Reflection.Metadata.MetadataReaderOptions.ApplyWindowsRuntimeProjections;
            }
        }

        public string ProjectPath { get; set; }
        public String[] ReferenceAssemblyPaths { get; set; }

        public bool IsSystemAssemblyLoaded { get { return _systemAssembly != null; } }

        private Loader Loader { get { return _loader; } }
        private Assembly SystemAssembly { get { return _systemAssembly; } }

        // Implements ITypeUniverse
        // Provide a default implementation of module resolution which just looks at the file.
        public override Module ResolveModule(Assembly containingAssembly, string moduleName)
        {
            return _loader.ResolveModule(containingAssembly, moduleName);
        }

        public Assembly LoadAssemblyFromFile(string path)
        {
            string fullPath = Path.GetFullPath(path);
            Assembly asm = Loader.ReadAssemblyFromFile(path);
            string asmName = asm.GetName().FullName;

            Assembly loadedAsm = null;
            // We can have identically named assemblies between the locally built assembly and a referenced static lib.
            // If there is a conflict, store the new assembly with a different name.  This assembly won't be retrievable via
            // our ResolveAssembly call, but can still be used to resolve types via the FindType method.
            while (_asmNameCache.TryGetValue(asmName, out loadedAsm))
            {
                if (loadedAsm.Location.Equals(asm.Location))
                {
                    return loadedAsm;
                }
                else
                {
                    asmName = "!" + asmName;
                }
            }

            AddAssembly(asm);
            _asmNameCache.Add(asmName, asm);
            if (asm.GetName().Name.Equals("mscorlib"))
            {
                _systemAssembly = asm;
                SetSystemAssembly(_systemAssembly);
            }

            if (asm.GetName().Name.Equals("System.Runtime"))
            {
                _systemRuntimeAssembly = asm;
            }
            return asm;
        }

        public override Assembly GetSystemRuntimeAssembly()
        {
            return this._systemRuntimeAssembly;
        }

        public override Assembly ResolveAssembly(AssemblyName name, bool throwOnError)
        {
            Assembly asm;

            // Note: this AssemblyName.FullName call accounts for a surprising amount of perf time.  Can't easily cache the result
            // as AssemblyName doesn't override Equals/GetHashCode and we are given different AssemblyName instances for the same actual assemblies.
            // Is there a way of caching this result somehow?
            string fullName = name.FullName;

            if (!_asmNameCache.TryGetValue(fullName, out asm))
            {
                asm = base.ResolveAssembly(name, throwOnError);

                //   base.ResolveAssembly() then went and did something that loaded the assembly.
                // It either went through LoadAssemblyFromFile() and added to the cache,
                // or some other path that didn't add it to the cache.
                //   Check if it is in the cache and add it if it needs to be added.
                // It is NOT EXPECTED that the new asm in the cache be different
                // than the return value from base.
                Assembly asm2;
                if (!_asmNameCache.TryGetValue(fullName, out asm2))
                {
                    _asmNameCache.Add(fullName, asm);
                }
                else
                {
                    System.Diagnostics.Debug.Assert(asm == asm2);
                }
            }
            return asm;
        }

        public override Assembly GetSystemAssembly()
        {
            if (_systemAssembly == null)
            {
                string path = string.Empty;
                if (ReferenceAssemblyPaths != null && ReferenceAssemblyPaths.Length > 0)
                {
                    foreach (String dirPath in ReferenceAssemblyPaths)
                    {
                        path = Path.Combine(dirPath, "mscorlib.dll");
                        if (File.Exists(path))
                        {
                            break;
                        }
                    }
                }
                else
                {
                    // In the C++ case there is no Reference Assembly Path
                    // So use the .NetFramework mscorlib we are running on.
                    // In the managed case don't fall back on this because this mscorlib is not
                    // right for a managed app.
                    // It isn't exactly right in the C++ case either but the differences are mitigated.
                    Assembly runtimeAssembly = typeof(int).Assembly;
                    path = runtimeAssembly.Location;
                }
                try
                {
                    // This takes care of double load checking and
                    // setting the base.SystemAssembly.
                    LoadAssemblyFromFile(path);
                }
                catch (FileNotFoundException fnf)
                {
                    throw new FileNotFoundException(String.Format("MsCorLib.dll not found at '{0}'", path), path, fnf);
                }
            }
            return base.GetSystemAssembly();
        }

        public Assembly GetXamlProxyAssembly()
        {
            if (_xamlProxyAssembly == null)
            {
                string path = Assembly.GetCallingAssembly().Location;
                _xamlProxyAssembly = LoadAssemblyFromFile(path);
            }
            return _xamlProxyAssembly;
        }

        public Type FindType(string typeName)
        {
            Type type;
            if (!_resolvedTypes.TryGetValue(typeName, out type))
            {
                type = GetSystemAssembly().GetType(typeName);
                if (type == null)
                {
                    foreach (Assembly asm in Assemblies)
                    {
                        type = asm.GetType(typeName);
                        if (type != null)
                        {
                            break;
                        }
                    }
                }
                if (type != null)
                {
                    _resolvedTypes.Add(typeName, type);
                }
            }
            return type;
        }
    }


    // This is used to wrap the RUN TIME mscorlib.
    //
    internal class XamlLmrAssemblyProxy : System.Reflection.Adds.AssemblyProxy
    {
        Assembly _assembly;
        AssemblyName _asmName;

        public XamlLmrAssemblyProxy(System.Reflection.Adds.ITypeUniverse universe, Assembly assembly)
            : base(universe)
        {
            _assembly = assembly;
        }

        protected override Assembly GetResolvedAssemblyWorker()
        {
            return _assembly;
        }

        protected override AssemblyName GetNameWithNoResolution()
        {
            if (_asmName == null)
            {
                _asmName = _assembly.GetName();
            }
            return _asmName;
        }

        public override int GetHashCode()
        {
            return GetResolvedAssembly().GetHashCode();
        }

        // Needed to fix Equals.  Compare with the Equals in SimpleUniverse
        public override bool Equals(object obj)
        {
            var proxy = obj as System.Reflection.Adds.AssemblyProxy;
            if (proxy != null)
            {
                obj = proxy.GetResolvedAssembly();
            }
            return GetResolvedAssembly().Equals(obj);
        }
    }
}
