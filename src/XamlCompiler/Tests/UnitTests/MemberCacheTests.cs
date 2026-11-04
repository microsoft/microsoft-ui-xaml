// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Reflection.Emit;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Win8Xaml.CompilerProxies;

namespace UnitTests
{
    [TestClass]
    public class MemberCacheTests
    {
        public class Base
        {
            public virtual string Value { get; set; }
            public int Hidden { get; set; }
            protected int ProtectedValue { get; set; }
            private int PrivateValue { get; set; }
            public static int StaticValue { get; set; }
            public virtual string Method(int value) { return null; }
            public void Overload(int value) { }
            public void Overload(string value) { }
            protected void ProtectedMethod() { }
            private void PrivateMethod() { }
            public static void StaticMethod() { }
        }

        public class Derived : Base
        {
            public override string Value { get; set; }
            public new string Hidden { get; set; }
            public int ReadOnly { get { return 1; } }
            public int MixedAccess { get; private set; }
            public string this[int index] { get { return null; } }
            public string this[string index] { get { return null; } }
            public override string Method(int value) { return null; }
            public new void Overload(int value) { }
            public T GenericMethod<T>(T value) { return value; }
        }

        public class Generic<T> : Base
        {
            public T Item { get; set; }
            public T Echo(T value) { return value; }
            public U Convert<U>(T value, U other) { return other; }
        }

        public interface IFixture
        {
            int Item { get; }
            void Method();
        }

        public struct Value
        {
            public int Item { get; set; }
            public void Method() { }
        }

        private XamlTypeUniverse _universe;
        private Assembly _fixture;
        private string _directory;
        private const BindingFlags PublicInstance = BindingFlags.Instance | BindingFlags.Public;

        [TestInitialize]
        public void Initialize()
        {
            _directory = Path.Combine(Path.GetTempPath(), "XamlMemberCacheTests", Guid.NewGuid().ToString("N"));
            Directory.CreateDirectory(_directory);
            _universe = new XamlTypeUniverse(false);
            _universe.LoadAssemblyFromFile(typeof(object).Assembly.Location);
            _fixture = _universe.LoadAssemblyFromFile(typeof(MemberCacheTests).Assembly.Location);
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

        private Type Metadata(Type type)
        {
            if (type.IsArray)
            {
                return type.GetArrayRank() == 1 ? Metadata(type.GetElementType()).MakeArrayType()
                    : Metadata(type.GetElementType()).MakeArrayType(type.GetArrayRank());
            }
            if (type.IsConstructedGenericType)
            {
                return Metadata(type.GetGenericTypeDefinition()).MakeGenericType(type.GetGenericArguments().Select(Metadata).ToArray());
            }
            return (type.Assembly == typeof(object).Assembly ? _universe.GetSystemAssembly() : _fixture).GetType(type.FullName, true);
        }

        private static string PropertySignature(PropertyInfo property)
        {
            return property.DeclaringType + "|" + property.Name + "|" + property.PropertyType + "|" +
                string.Join(",", property.GetIndexParameters().Select(p => p.ParameterType.ToString())) + "|" +
                property.GetGetMethod(true) + "|" + property.GetSetMethod(true);
        }

        private static string MethodSignature(MethodInfo method)
        {
            return method.DeclaringType + "|" + method.Name + "|" + method.Attributes + "|" + method.ReturnType + "|" +
                string.Join(",", method.GetParameters().Select(p => p.ParameterType.ToString())) + "|" +
                string.Join(",", method.GetGenericArguments().Select(p => p.ToString()));
        }

        private static T[] EnumerateUncached<T>(Type type, string methodName, BindingFlags flags) where T : MemberInfo
        {
            Type moduleType = type.GetType().Assembly.GetType("Microsoft.UI.Xaml.Markup.Compiler.Lmr.MetadataOnlyModule", true);
            MethodInfo method = moduleType.GetMethod(methodName, BindingFlags.Static | BindingFlags.NonPublic);
            Assert.IsNotNull(method);
            return (T[])method.Invoke(null, new object[] { type, flags });
        }

        [TestMethod]
        public void MemberCache_BindingFlagsOrderingAndSignatures()
        {
            foreach (Type runtime in new[] { typeof(Base), typeof(Derived), typeof(Generic<>), typeof(Generic<int>),
                typeof(Generic<string>), typeof(IFixture), typeof(Value), typeof(int[]), typeof(int[,]) })
            {
                Type type = Metadata(runtime);
                for (int bits = 0; bits < 64; bits++)
                {
                    BindingFlags flags = 0;
                    if ((bits & 1) != 0) flags |= BindingFlags.Public;
                    if ((bits & 2) != 0) flags |= BindingFlags.NonPublic;
                    if ((bits & 4) != 0) flags |= BindingFlags.Instance;
                    if ((bits & 8) != 0) flags |= BindingFlags.Static;
                    if ((bits & 16) != 0) flags |= BindingFlags.DeclaredOnly;
                    if ((bits & 32) != 0) flags |= BindingFlags.FlattenHierarchy;
                    string[] properties = EnumerateUncached<PropertyInfo>(type, "GetPropertiesOnType", flags).Select(PropertySignature).ToArray();
                    string[] methods = EnumerateUncached<MethodInfo>(type, "GetMethodsOnType", flags).Select(MethodSignature).ToArray();
                    for (int i = 0; i < 2; i++)
                    {
                        CollectionAssert.AreEqual(properties, type.GetProperties(flags).Select(PropertySignature).ToArray());
                        CollectionAssert.AreEqual(methods, type.GetMethods(flags).Select(MethodSignature).ToArray());
                    }
                }
            }
        }

        [TestMethod]
        public void MemberCache_ReturnedArraysCannotCorruptLaterCalls()
        {
            Type type = Metadata(typeof(Derived));
            PropertyInfo[] properties = type.GetProperties(PublicInstance);
            string[] expectedProperties = properties.Select(PropertySignature).ToArray();
            properties[0] = null;
            Array.Reverse(properties);
            CollectionAssert.AreEqual(expectedProperties, type.GetProperties(PublicInstance).Select(PropertySignature).ToArray());
            MethodInfo[] methods = type.GetMethods(PublicInstance);
            string[] expectedMethods = methods.Select(MethodSignature).ToArray();
            methods[0] = null;
            Array.Reverse(methods);
            CollectionAssert.AreEqual(expectedMethods, type.GetMethods(PublicInstance).Select(MethodSignature).ToArray());
        }

        [TestMethod]
        public void MemberCache_ConstructedGenericTypesRemainSeparate()
        {
            Type intType = Metadata(typeof(Generic<int>));
            Type stringType = Metadata(typeof(Generic<string>));
            for (int i = 0; i < 3; i++)
            {
                Assert.AreEqual("System.Int32", intType.GetProperty("Item", PublicInstance).PropertyType.FullName);
                Assert.AreEqual("System.String", stringType.GetProperty("Item", PublicInstance).PropertyType.FullName);
                Assert.AreEqual("System.Int32", intType.GetMethod("Echo", PublicInstance).ReturnType.FullName);
                Assert.AreEqual("System.String", stringType.GetMethod("Echo", PublicInstance).ReturnType.FullName);
            }
        }

        [TestMethod]
        public void MemberCache_ParameterAndGenericArgumentArraysRemainIsolated()
        {
            Type type = Metadata(typeof(Derived));
            MethodInfo method = type.GetMethod("GenericMethod", PublicInstance);
            string signature = MethodSignature(method);
            method.GetParameters()[0] = null;
            method.GetGenericArguments()[0] = null;
            Assert.AreEqual(signature, MethodSignature(type.GetMethod("GenericMethod", PublicInstance)));
            PropertyInfo indexer = type.GetProperties(PublicInstance).First(p => p.GetIndexParameters().Length != 0);
            string property = PropertySignature(indexer);
            indexer.GetIndexParameters()[0] = null;
            Assert.IsTrue(type.GetProperties(PublicInstance).Select(PropertySignature).Contains(property));
        }

        [TestMethod]
        public void MemberCache_UnsupportedFlagsKeepThrowing()
        {
            Type type = Metadata(typeof(Derived));
            BindingFlags flags = PublicInstance | BindingFlags.OptionalParamBinding;
            for (int i = 0; i < 2; i++)
            {
                ExpectException<NotSupportedException>(() => type.GetProperties(flags));
                ExpectException<NotSupportedException>(() => type.GetMethods(flags));
            }
            Assert.IsTrue(type.GetProperties(PublicInstance).Length != 0);
            Assert.IsTrue(type.GetMethods(PublicInstance).Length != 0);
        }

        [TestMethod]
        public void MemberCache_MetadataUniversesRemainSeparate()
        {
            var other = new XamlTypeUniverse(false);
            try
            {
                other.LoadAssemblyFromFile(typeof(object).Assembly.Location);
                Assembly otherFixture = other.LoadAssemblyFromFile(typeof(MemberCacheTests).Assembly.Location);
                Type first = Metadata(typeof(Derived));
                Type second = otherFixture.GetType(typeof(Derived).FullName, true);
                PropertyInfo property = first.GetProperty("Value", PublicInstance);
                PropertyInfo otherProperty = second.GetProperty("Value", PublicInstance);
                Assert.AreNotSame(property, otherProperty);
                Assert.IsFalse(property.Module.Equals(otherProperty.Module));
                Assert.IsFalse(first.GetMethod("Method", PublicInstance).Module.Equals(second.GetMethod("Method", PublicInstance).Module));
            }
            finally { ((IDisposable)other.Instance).Dispose(); }
        }

        [TestMethod]
        public void MemberCache_FailedEnumerationCanRecover()
        {
            var name = new AssemblyName("MemberDependency");
            var dependency = AppDomain.CurrentDomain.DefineDynamicAssembly(name, AssemblyBuilderAccess.RunAndSave, _directory);
            ModuleBuilder depModule = dependency.DefineDynamicModule(name.Name, name.Name + ".dll");
            Type depType = depModule.DefineType("MemberDependency.Value", TypeAttributes.Public).CreateType();
            dependency.Save(name.Name + ".dll");
            var consumerName = new AssemblyName("MemberConsumer");
            var consumer = AppDomain.CurrentDomain.DefineDynamicAssembly(consumerName, AssemblyBuilderAccess.RunAndSave, _directory);
            ModuleBuilder module = consumer.DefineDynamicModule(consumerName.Name, consumerName.Name + ".dll");
            TypeBuilder builder = module.DefineType("MemberConsumer.Type", TypeAttributes.Public, depType);
            PropertyBuilder property = builder.DefineProperty("Item", PropertyAttributes.None, depType, Type.EmptyTypes);
            MethodBuilder getter = builder.DefineMethod("get_Item", MethodAttributes.Public | MethodAttributes.SpecialName,
                depType, Type.EmptyTypes);
            getter.GetILGenerator().Emit(OpCodes.Ldnull);
            getter.GetILGenerator().Emit(OpCodes.Ret);
            property.SetGetMethod(getter);
            builder.CreateType();
            consumer.Save(consumerName.Name + ".dll");
            Type metadata = _universe.LoadAssemblyFromFile(Path.Combine(_directory, "MemberConsumer.dll")).GetType("MemberConsumer.Type", true);
            for (int i = 0; i < 2; i++)
            {
                Exception exception = ExpectException<Exception>(() => metadata.GetProperties(PublicInstance));
                Assert.AreEqual("UnresolvedAssemblyException", exception.GetType().Name);
                exception = ExpectException<Exception>(() => metadata.GetMethods(PublicInstance));
                Assert.AreEqual("UnresolvedAssemblyException", exception.GetType().Name);
            }
            _universe.LoadAssemblyFromFile(Path.Combine(_directory, "MemberDependency.dll"));
            for (int i = 0; i < 2; i++)
            {
                Assert.AreEqual("MemberDependency.Value", metadata.GetProperties(PublicInstance)[0].PropertyType.FullName);
                Assert.IsTrue(metadata.GetMethods(PublicInstance).Any(m => m.Name == "get_Item"));
            }
        }

        private static T ExpectException<T>(Action action) where T : Exception
        {
            try { action(); }
            catch (T exception) { return exception; }
            Assert.Fail("Expected " + typeof(T).Name);
            return null;
        }
    }
}
