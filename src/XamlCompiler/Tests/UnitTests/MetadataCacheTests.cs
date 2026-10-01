// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections;
using System.IO;
using System.Reflection;
using System.Reflection.Emit;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Win8Xaml.CompilerProxies;

namespace UnitTests
{
    [TestClass]
    public class MetadataCacheTests
    {
        string _directory;
        XamlTypeUniverse _universe;
        Assembly _runtimeLookupAssembly;

        [TestInitialize]
        public void Initialize()
        {
            _directory = Path.Combine(Path.GetTempPath(), "XamlCompilerMetadataTests", Guid.NewGuid().ToString("N"));
            Directory.CreateDirectory(_directory);
            _universe = new XamlTypeUniverse(false);
            _universe.LoadAssemblyFromFile(typeof(object).Assembly.Location);
        }

        [TestCleanup]
        public void Cleanup()
        {
            if (_universe != null)
            {
                ((IDisposable)_universe.Instance).Dispose();
            }
            if (_directory != null && Directory.Exists(_directory))
            {
                Directory.Delete(_directory, true);
            }
        }

        [TestMethod]
        public void MetadataCache_QualifiedAndUnqualifiedTypeLookup()
        {
            string path = CreateLookupAssembly();
            Module module = _universe.LoadAssemblyFromFile(path).ManifestModule;

            // Enumerate definitions rather than using the name index under test.
            Type first = FindDefinedType(module, "CacheTests.First.Shared");
            Type second = FindDefinedType(module, "CacheTests.Second.Shared");
            Type global = FindDefinedType(module, "GlobalType");
            for (int i = 0; i < 2; i++)
            {
                AssertTypeDefinition(module, null, first.FullName, first);
                AssertTypeDefinition(module, null, second.FullName, second);
                AssertTypeDefinition(module, null, "Shared", first);
                AssertTypeDefinition(module, null, "GlobalType", global);
                Assert.IsTrue(IsNil(FindTypeDefinition(module, null, "CacheTests.First.shared", false)));
            }
        }

        [TestMethod]
        public void MetadataCache_NestedAndGenericTypeLookup()
        {
            Module module = _universe.LoadAssemblyFromFile(CreateLookupAssembly()).ManifestModule;
            Type outer = FindDefinedType(module, "CacheTests.Outer");
            Type nested = FindDefinedType(module, "CacheTests.Outer+OnlyNested");
            Type generic = FindDefinedType(module, "CacheTests.Generic`1");
            Type nestedGeneric = FindDefinedType(module, "CacheTests.Outer+Nested`1");

            for (int i = 0; i < 2; i++)
            {
                AssertTypeDefinition(module, null, generic.FullName, generic);
                AssertTypeDefinition(module, outer, "OnlyNested", nested);
                AssertTypeDefinition(module, outer, "Nested`1", nestedGeneric);
                Assert.IsTrue(IsNil(FindTypeDefinition(module, null, "OnlyNested", false)));
                Assert.IsTrue(IsNil(FindTypeDefinition(module, null, "CacheTests.OnlyNested", false)));
                Assert.IsTrue(IsNil(FindTypeDefinition(module, outer, "Shared", false)));
            }
        }

        [TestMethod]
        public void MetadataCache_MissingTypePreservesThrowBehavior()
        {
            Module module = _universe.LoadAssemblyFromFile(CreateLookupAssembly()).ManifestModule;
            string message = null;
            for (int i = 0; i < 2; i++)
            {
                Assert.IsTrue(IsNil(FindTypeDefinition(module, null, "CacheTests.Missing", false)));
                var exception = ExpectInvocationException<TypeLoadException>(
                    () => FindTypeDefinition(module, null, "CacheTests.Missing", true));
                StringAssert.Contains(exception.Message, "CacheTests.Missing");
                if (message != null)
                {
                    Assert.AreEqual(message, exception.Message);
                }
                message = exception.Message;
                Type existing = FindDefinedType(module, "CacheTests.First.Shared");
                AssertTypeDefinition(module, null, existing.FullName, existing);
            }
        }

        [TestMethod]
        public void MetadataCache_AssemblyQualifiedNamesMatchRuntimeReflection()
        {
            string path = CreateLookupAssembly();
            Assembly runtime = _runtimeLookupAssembly;
            Assembly metadata = _universe.LoadAssemblyFromFile(path);
            foreach (string name in new[] { "GlobalType", "CacheTests.First.Shared", "CacheTests.Outer+OnlyNested", "CacheTests.Generic`1" })
            {
                Type expected = runtime.GetType(name, true);
                Type actual = metadata.GetType(name, true);
                for (int i = 0; i < 2; i++)
                {
                    Assert.AreEqual(expected.AssemblyQualifiedName, actual.AssemblyQualifiedName);
                }
            }

            Type runtimeGeneric = runtime.GetType("CacheTests.Generic`1", true);
            Type metadataGeneric = metadata.GetType("CacheTests.Generic`1", true);
            Type metadataInt = _universe.GetSystemAssembly().GetType("System.Int32", true);
            Assert.AreEqual(runtimeGeneric.MakeGenericType(typeof(int)).AssemblyQualifiedName,
                metadataGeneric.MakeGenericType(metadataInt).AssemblyQualifiedName);
            Assert.IsNull(metadataGeneric.GetGenericArguments()[0].AssemblyQualifiedName);
        }

        [TestMethod]
        public void MetadataCache_AssemblyReferencesAreScopedToTheirModule()
        {
            Type firstBase;
            Type secondBase;
            string firstPath = CreateDependency(new Version(1, 0, 0, 0), out firstBase);
            string secondPath = CreateDependency(new Version(2, 0, 0, 0), out secondBase);
            Assembly first = _universe.LoadAssemblyFromFile(firstPath);
            Assembly second = _universe.LoadAssemblyFromFile(secondPath);
            Module firstModule = _universe.LoadAssemblyFromFile(CreateConsumer("FirstConsumer", firstBase)).ManifestModule;
            Module secondModule = _universe.LoadAssemblyFromFile(CreateConsumer("SecondConsumer", secondBase)).ManifestModule;
            object firstReference = FindAssemblyReference(firstModule, "CacheDependency");
            object secondReference = FindAssemblyReference(secondModule, "CacheDependency");

            Assert.AreEqual(firstReference, secondReference, "The fixture must reuse the same metadata row in different modules.");
            Assert.AreNotEqual(first.FullName, second.FullName);
            for (int i = 0; i < 2; i++)
            {
                Assert.AreSame(first, ResolveAssembly(_universe, firstModule, firstReference));
                Assert.AreSame(second, ResolveAssembly(_universe, secondModule, secondReference));
            }
        }

        [TestMethod]
        public void MetadataCache_MissingAssemblyCanResolveAfterLoading()
        {
            Type baseType;
            string dependencyPath = CreateDependency(new Version(1, 0, 0, 0), out baseType);
            Module module = _universe.LoadAssemblyFromFile(CreateConsumer("MissingConsumer", baseType)).ManifestModule;
            object reference = FindAssemblyReference(module, "CacheDependency");
            string message = null;
            for (int i = 0; i < 2; i++)
            {
                var exception = ExpectInvocationException<Exception>(() => ResolveAssembly(_universe, module, reference));
                Assert.AreEqual("UnresolvedAssemblyException", exception.GetType().Name);
                StringAssert.Contains(exception.Message, "CacheDependency");
                if (message != null)
                {
                    Assert.AreEqual(message, exception.Message);
                }
                message = exception.Message;
            }

            Assembly dependency = _universe.LoadAssemblyFromFile(dependencyPath);
            Assert.AreSame(dependency, ResolveAssembly(_universe, module, reference));
            Assert.AreSame(dependency, ResolveAssembly(_universe, module, reference));
        }

        [TestMethod]
        public void MetadataCache_SeparateUniversesDoNotShareResolvedAssemblies()
        {
            Type baseType;
            string dependencyPath = CreateDependency(new Version(1, 0, 0, 0), out baseType);
            string consumerPath = CreateConsumer("IsolatedConsumer", baseType);
            Assembly first = _universe.LoadAssemblyFromFile(dependencyPath);
            Module firstModule = _universe.LoadAssemblyFromFile(consumerPath).ManifestModule;
            Assert.AreSame(first, ResolveAssembly(_universe, firstModule, FindAssemblyReference(firstModule, "CacheDependency")));

            var otherUniverse = new XamlTypeUniverse(false);
            using ((IDisposable)otherUniverse.Instance)
            {
                Assembly second = otherUniverse.LoadAssemblyFromFile(dependencyPath);
                Module secondModule = otherUniverse.LoadAssemblyFromFile(consumerPath).ManifestModule;
                Assert.AreNotSame(first, second);
                Assert.AreSame(second, ResolveAssembly(otherUniverse, secondModule, FindAssemblyReference(secondModule, "CacheDependency")));
            }
            Assert.AreSame(first, ResolveAssembly(_universe, firstModule, FindAssemblyReference(firstModule, "CacheDependency")));
        }

        string CreateLookupAssembly()
        {
            return SaveAssembly("CacheLookup", new Version(1, 2, 3, 4), module =>
            {
                _runtimeLookupAssembly = module.Assembly;
                module.DefineType("CacheTests.First.Shared", TypeAttributes.Public).CreateType();
                module.DefineType("CacheTests.Second.Shared", TypeAttributes.Public).CreateType();
                module.DefineType("GlobalType", TypeAttributes.Public).CreateType();
                TypeBuilder outer = module.DefineType("CacheTests.Outer", TypeAttributes.Public);
                outer.DefineNestedType("OnlyNested", TypeAttributes.NestedPublic).CreateType();
                TypeBuilder nestedGeneric = outer.DefineNestedType("Nested`1", TypeAttributes.NestedPublic);
                nestedGeneric.DefineGenericParameters("T");
                nestedGeneric.CreateType();
                outer.CreateType();
                TypeBuilder generic = module.DefineType("CacheTests.Generic`1", TypeAttributes.Public);
                generic.DefineGenericParameters("T");
                generic.CreateType();
            });
        }

        string CreateDependency(Version version, out Type baseType)
        {
            Type createdType = null;
            string path = SaveAssembly("CacheDependency", version, module =>
                createdType = module.DefineType("CacheTests.Base", TypeAttributes.Public).CreateType());
            baseType = createdType;
            return path;
        }

        string CreateConsumer(string name, Type baseType)
        {
            return SaveAssembly(name, new Version(1, 0, 0, 0), module =>
                module.DefineType("CacheTests.Derived", TypeAttributes.Public, baseType).CreateType());
        }

        string SaveAssembly(string name, Version version, Action<ModuleBuilder> defineTypes)
        {
            string fileName = name + "." + version + ".dll";
            var assemblyName = new AssemblyName(name) { Version = version };
            AssemblyBuilder assembly = AppDomain.CurrentDomain.DefineDynamicAssembly(
                assemblyName, AssemblyBuilderAccess.RunAndSave, _directory);
            ModuleBuilder module = assembly.DefineDynamicModule(name, fileName);
            defineTypes(module);
            assembly.Save(fileName);
            return Path.Combine(_directory, fileName);
        }

        static Type FindDefinedType(Module module, string name)
        {
            foreach (Type type in module.GetTypes())
            {
                if (type.FullName == name)
                {
                    return type;
                }
            }
            Assert.Fail("Missing fixture type: " + name);
            return null;
        }

        static object FindTypeDefinition(Module module, Type outer, string name, bool throwOnError)
        {
            MethodInfo method = module.GetType().GetMethod("FindTypeDefByName",
                BindingFlags.Instance | BindingFlags.NonPublic, null,
                new[] { typeof(Type), typeof(string), typeof(bool) }, null);
            Assert.IsNotNull(method);
            return method.Invoke(module, new object[] { outer, name, throwOnError });
        }

        static void AssertTypeDefinition(Module module, Type outer, string name, Type expected)
        {
            object actual = FindTypeDefinition(module, outer, name, true);
            Type tokens = actual.GetType().Assembly.GetType("System.Reflection.Metadata.Ecma335.MetadataTokens", true);
            object expectedHandle = tokens.GetMethod("TypeDefinitionHandle").Invoke(null,
                new object[] { expected.MetadataToken & 0x00ffffff });
            Assert.AreEqual(expectedHandle, actual, name);
        }

        static bool IsNil(object handle)
        {
            return (bool)handle.GetType().GetProperty("IsNil").GetValue(handle, null);
        }

        static object FindAssemblyReference(Module module, string name)
        {
            object reader = module.GetType().GetProperty("RawReader", BindingFlags.Instance | BindingFlags.NonPublic).GetValue(module, null);
            var references = (IEnumerable)reader.GetType().GetProperty("AssemblyReferences").GetValue(reader, null);
            MethodInfo getName = module.GetType().GetMethod("GetAssemblyNameFromAssemblyRef");
            foreach (object reference in references)
            {
                var assemblyName = (AssemblyName)getName.Invoke(module, new[] { reference });
                if (assemblyName.Name == name)
                {
                    return reference;
                }
            }
            Assert.Fail("Missing fixture assembly reference: " + name);
            return null;
        }

        static Assembly ResolveAssembly(XamlTypeUniverse universe, Module module, object reference)
        {
            MethodInfo method = universe.Instance.GetType().GetMethod("ResolveAssembly",
                new[] { typeof(Module), reference.GetType() });
            return (Assembly)method.Invoke(universe.Instance, new[] { (object)module, reference });
        }

        static T ExpectInvocationException<T>(Action action) where T : Exception
        {
            try
            {
                action();
            }
            catch (TargetInvocationException exception)
            {
                Assert.IsInstanceOfType(exception.InnerException, typeof(T));
                return (T)exception.InnerException;
            }
            Assert.Fail("Expected " + typeof(T).Name);
            return null;
        }
    }
}
