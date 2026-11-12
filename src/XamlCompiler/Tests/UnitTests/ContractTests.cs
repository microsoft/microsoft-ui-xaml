// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Text;
using System.Text.Json;
using System.Text.Json.Serialization;
using Microsoft.UI.Xaml.Markup.Compiler.MSBuildInterop;
using Microsoft.UI.Xaml.Markup.Compiler.Tasks;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Win8Xaml.CompilerProxies;

namespace UnitTests
{
    /// <summary>
    /// The JSON contract between the CompileXaml task, which serializes it with DataContractJsonSerializer, and the XAML compiler,
    /// which serializes it with the System.Text.Json source generator (see CompilerContract): both must agree on every member.
    /// </summary>
    [TestClass]
    public class ContractTests
    {
        private static readonly Type[] s_rootTypes = { typeof(CompilerInputs), typeof(CompilerOutputs), typeof(List<FileOverlay>) };

        private static Assembly Compiler => Assembly.LoadFrom(ProxyHelper.XamlCompilerPath);

        private static JsonSerializerContext CompilerJsonContext =>
            (JsonSerializerContext)Compiler.GetType("Microsoft.UI.Xaml.Markup.Compiler.Executable.CompilerJsonContext", true)
                .GetProperty("Default", BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Static).GetValue(null);

        // The same contract type, as compiled into the compiler
        private static Type GetCompilerType(Type taskType)
        {
            if (taskType.IsGenericType)
            {
                return taskType.GetGenericTypeDefinition().MakeGenericType(taskType.GetGenericArguments().Select(GetCompilerType).ToArray());
            }
            return taskType.Assembly == typeof(CompilerInputs).Assembly ? Compiler.GetType(taskType.FullName, true) : taskType;
        }

        [TestMethod]
        public void Contract_RoundTripsBetweenTheTaskAndTheCompiler()
        {
            foreach (Type rootType in s_rootTypes)
            {
                int seed = 0;
                object original = CreateNonDefault(rootType, ref seed);

                // Task -> compiler
                string taskJson = WriteWithTask(rootType, original);
                object compilerValue = JsonSerializer.Deserialize(taskJson, GetCompilerType(rootType), CompilerJsonContext);
                AssertEquivalent(original, compilerValue, rootType.Name);

                // Compiler -> task
                string compilerJson = JsonSerializer.Serialize(compilerValue, GetCompilerType(rootType), CompilerJsonContext);
                object taskValue = ReadWithTask(rootType, compilerJson);
                AssertEquivalent(original, taskValue, rootType.Name);
            }
        }

        [TestMethod]
        public void Contract_OnlyUsesMembersBothSerializersAgreeOn()
        {
            HashSet<Type> visited = new HashSet<Type>();
            List<string> violations = new List<string>();
            foreach (Type rootType in s_rootTypes)
            {
                CheckType(rootType, rootType.Name, visited, violations);
            }
            Assert.AreEqual(0, violations.Count, "Members not allowed in the contract (see CompilerContract):" + Environment.NewLine + string.Join(Environment.NewLine, violations));
        }

        [TestMethod]
        public void Contract_CompilerTypesMatchTheTaskTypes()
        {
            foreach (Type type in typeof(CompilerInputs).Assembly.GetTypes().Where(t => t.Namespace == typeof(CompilerInputs).Namespace && t.IsPublic && !t.IsInterface))
            {
                Type compilerType = GetCompilerType(type);
                CollectionAssert.AreEquivalent(
                    type.GetProperties().Select(p => p.Name + ":" + p.PropertyType.Name).ToArray(),
                    compilerType.GetProperties().Select(p => p.Name + ":" + p.PropertyType.Name).ToArray(),
                    type.FullName);
            }
            Assert.AreEqual(CompilerContract.Version, (int)GetCompilerType(typeof(CompilerContract)).GetField("Version").GetValue(null));
        }

        private static void CheckType(Type type, string path, HashSet<Type> visited, List<string> violations)
        {
            if (!IsAllowed(type, out Type elementType))
            {
                violations.Add(path + ": " + type);
                return;
            }
            if (elementType != null)
            {
                CheckType(elementType, path + "[]", visited, violations);
                return;
            }
            if (!IsContractClass(type) || !visited.Add(type))
            {
                return;
            }

            if (type.GetFields(BindingFlags.Public | BindingFlags.Instance).Length > 0)
            {
                violations.Add(path + ": public fields");
            }
            foreach (PropertyInfo property in type.GetProperties(BindingFlags.Public | BindingFlags.Instance))
            {
                string propertyPath = path + "." + property.Name;
                MethodInfo setter = property.GetSetMethod();
                if (property.GetGetMethod() == null || setter == null)
                {
                    violations.Add(propertyPath + ": not a public read/write property");
                    continue;
                }
                if (setter.ReturnParameter.GetRequiredCustomModifiers().Contains(typeof(IsExternalInit)))
                {
                    violations.Add(propertyPath + ": init-only");
                    continue;
                }
                CheckType(property.PropertyType, propertyPath, visited, violations);
            }
        }

        private static bool IsContractClass(Type type) => type.IsClass && type.IsSealed && type.Namespace == typeof(CompilerInputs).Namespace;

        private static bool IsAllowed(Type type, out Type elementType)
        {
            elementType = null;
            if (type == typeof(string) || type == typeof(bool) || type == typeof(int) || type == typeof(long) || type.IsEnum || IsContractClass(type))
            {
                return true;
            }
            if (type.IsArray)
            {
                elementType = type.GetElementType();
                return true;
            }
            if (type.IsGenericType && (type.GetGenericTypeDefinition() == typeof(List<>) || type.GetGenericTypeDefinition() == typeof(IList<>)))
            {
                elementType = type.GetGenericArguments()[0];
                return true;
            }
            return false;
        }

        // A value of the type where every member has a value that isn't the default one
        private static object CreateNonDefault(Type type, ref int seed)
        {
            seed++;
            if (type == typeof(string))
            {
                return "value \"" + seed + "\" C:\\path\\é/\u2603";
            }
            if (type == typeof(int))
            {
                return 1000 + seed;
            }
            if (type == typeof(long))
            {
                return 5_000_000_000L + seed;
            }
            if (type.IsEnum)
            {
                return Enum.GetValues(type).Cast<object>().Last();
            }
            if (type.IsArray)
            {
                Array array = Array.CreateInstance(type.GetElementType(), 2);
                array.SetValue(CreateNonDefault(type.GetElementType(), ref seed), 0);
                array.SetValue(CreateNonDefault(type.GetElementType(), ref seed), 1);
                return array;
            }
            if (type.IsGenericType && (type.GetGenericTypeDefinition() == typeof(List<>) || type.GetGenericTypeDefinition() == typeof(IList<>)))
            {
                Type itemType = type.GetGenericArguments()[0];
                IList list = (IList)Activator.CreateInstance(typeof(List<>).MakeGenericType(itemType));
                list.Add(CreateNonDefault(itemType, ref seed));
                list.Add(CreateNonDefault(itemType, ref seed));
                return list;
            }

            object instance = Activator.CreateInstance(type);
            object defaults = Activator.CreateInstance(type);
            foreach (PropertyInfo property in type.GetProperties(BindingFlags.Public | BindingFlags.Instance).Where(p => p.CanWrite))
            {
                object value = property.PropertyType == typeof(bool) ? !(bool)property.GetValue(defaults) : CreateNonDefault(property.PropertyType, ref seed);
                Assert.AreNotEqual(property.GetValue(defaults), value, type.Name + "." + property.Name);
                property.SetValue(instance, value);
            }
            return instance;
        }

        private static void AssertEquivalent(object expected, object actual, string path)
        {
            if (expected == null || actual == null)
            {
                Assert.AreEqual(expected == null, actual == null, path);
                return;
            }

            Type type = expected.GetType();
            if (type == typeof(string) || type.IsPrimitive)
            {
                Assert.AreEqual(expected, actual, path);
            }
            else if (type.IsEnum)
            {
                Assert.AreEqual(Convert.ToInt64(expected), Convert.ToInt64(actual), path);
                Assert.AreEqual(expected.ToString(), actual.ToString(), path);
            }
            else if (expected is IEnumerable expectedItems)
            {
                object[] expectedArray = expectedItems.Cast<object>().ToArray();
                object[] actualArray = ((IEnumerable)actual).Cast<object>().ToArray();
                Assert.AreEqual(expectedArray.Length, actualArray.Length, path + ".Count");
                for (int i = 0; i < expectedArray.Length; i++)
                {
                    AssertEquivalent(expectedArray[i], actualArray[i], path + "[" + i + "]");
                }
            }
            else
            {
                foreach (PropertyInfo property in type.GetProperties(BindingFlags.Public | BindingFlags.Instance))
                {
                    PropertyInfo actualProperty = actual.GetType().GetProperty(property.Name);
                    Assert.IsNotNull(actualProperty, path + "." + property.Name);
                    AssertEquivalent(property.GetValue(expected), actualProperty.GetValue(actual), path + "." + property.Name);
                }
            }
        }

        private static string WriteWithTask(Type type, object value)
        {
            using (MemoryStream stream = new MemoryStream())
            {
                typeof(CompilerJson).GetMethod(nameof(CompilerJson.Write), new[] { typeof(Stream), Type.MakeGenericMethodParameter(0) })
                    .MakeGenericMethod(type).Invoke(null, new[] { stream, value });
                return Encoding.UTF8.GetString(stream.ToArray());
            }
        }

        private static object ReadWithTask(Type type, string json)
        {
            using (MemoryStream stream = new MemoryStream(Encoding.UTF8.GetBytes(json)))
            {
                return typeof(CompilerJson).GetMethod(nameof(CompilerJson.Read), new[] { typeof(Stream) })
                    .MakeGenericMethod(type).Invoke(null, new object[] { stream });
            }
        }
    }
}
