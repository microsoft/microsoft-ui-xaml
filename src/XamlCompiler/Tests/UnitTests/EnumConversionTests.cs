// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Reflection;
using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Win8Xaml.CompilerProxies;

namespace UnitTests
{
    [TestClass]
    public class EnumConversionTests
    {
        public enum SByteEnum : sbyte { Zero, One, Two, Min = sbyte.MinValue, Max = sbyte.MaxValue }
        public enum ByteEnum : byte { Zero, One, Two, Max = byte.MaxValue }
        public enum Int16Enum : short { Zero, One, Two, Min = short.MinValue, Max = short.MaxValue }
        public enum UInt16Enum : ushort { Zero, One, Two, Max = ushort.MaxValue }
        public enum Int32Enum : int { Zero, One, Two, Min = int.MinValue, Max = int.MaxValue }
        public enum UInt32Enum : uint { Zero, One, Two, Max = uint.MaxValue }
        public enum Int64Enum : long { Zero, One, Two, Min = long.MinValue, Max = long.MaxValue }
        public enum UInt64Enum : ulong { Zero, One, Two, SignedMax = long.MaxValue, Max = ulong.MaxValue }
        public enum EmptyEnum : byte { }
        public enum AnotherInt32Enum : int { One = 1 }

        private static readonly Type[] EnumTypes =
        {
            typeof(SByteEnum), typeof(ByteEnum), typeof(Int16Enum), typeof(UInt16Enum),
            typeof(Int32Enum), typeof(UInt32Enum), typeof(Int64Enum), typeof(UInt64Enum)
        };

        private static string GenerateMetadata()
        {
            InstanceCacheManager.ClearCache();
            using (var universe = new XamlTypeUniverse(false))
            {
                var core = universe.LoadAssemblyFromFile(typeof(object).Assembly.Location);
                var winui = universe.LoadAssemblyFromFile(ProxyHelper.WinUIWinmdPath);
                var fixtures = universe.LoadAssemblyFromFile(typeof(EnumConversionTests).Assembly.Location);
                var schema = new DirectUISchemaContext(new[] { core, winui, fixtures }, null, null, null, true);
                var info = new XamlSchemaCodeInfo();
                foreach (Type enumType in EnumTypes.Concat(new[] { typeof(EmptyEnum), typeof(AnotherInt32Enum) }))
                {
                    info.AddType(new DirectUIXamlType(fixtures.GetType(enumType.FullName), schema));
                }
                var project = new XamlProjectInfo
                {
                    IsLibrary = true,
                    RootNamespace = "EnumConversion",
                    ProjectName = "EnumConversion",
                    TargetPlatformMinVersion = new Version(KnownVersions.Latest)
                };
                var generator = new XamlCodeGenerator(Language.Parse("C#"), false, project, info);
                return generator.GenerateTypeInfo(new ClassName("EnumConversion.App")).Single().Contents;
            }
        }
        private static object CompileProvider(string source)
        {
            // Only the metadata contract is needed; these tests do not activate WinUI.
            const string contracts = @"
namespace Microsoft.UI.Xaml.Markup
{
    public interface IXamlType
    {
        string FullName { get; }
        System.Type UnderlyingType { get; }
        bool IsConstructible { get; }
        object CreateFromString(string input);
    }
    public interface IXamlMember { }
    public interface IXamlMetadataProvider
    {
        IXamlType GetXamlType(System.Type type);
        IXamlType GetXamlType(string name);
        XmlnsDefinition[] GetXmlnsDefinitions();
    }
    public struct XmlnsDefinition { }
    public class FullXamlMetadataProviderAttribute : System.Attribute { }
}
namespace Windows.Foundation.Metadata
{
    public class DefaultOverloadAttribute : System.Attribute { }
}";
            var compilation = CSharpCompilation.Create(
                "EnumMetadata_" + Guid.NewGuid().ToString("N"),
                new[] { CSharpSyntaxTree.ParseText(source), CSharpSyntaxTree.ParseText(contracts) },
                new[]
                {
                    MetadataReference.CreateFromFile(typeof(object).Assembly.Location),
                    MetadataReference.CreateFromFile(typeof(System.CodeDom.Compiler.GeneratedCodeAttribute).Assembly.Location),
                    MetadataReference.CreateFromFile(typeof(EnumConversionTests).Assembly.Location)
                },
                new CSharpCompilationOptions(OutputKind.DynamicallyLinkedLibrary));
            using (var stream = new MemoryStream())
            {
                var result = compilation.Emit(stream);
                Assert.IsTrue(result.Success, string.Join(Environment.NewLine, result.Diagnostics));
                var assembly = Assembly.Load(stream.ToArray());
                return Activator.CreateInstance(assembly.GetTypes().Single(type => type.Name == "XamlTypeInfoProvider"), true);
            }
        }

        private static object GetEnumMetadata(object provider, Type enumType)
        {
            return provider.GetType().GetMethod("GetXamlTypeByType").Invoke(provider, new object[] { enumType });
        }

        private static object ConvertInput(object metadata, string input, out Exception error)
        {
            error = null;
            try
            {
                return metadata.GetType().GetMethod("CreateFromString").Invoke(metadata, new object[] { input });
            }
            catch (TargetInvocationException exception)
            {
                error = exception.InnerException;
                return null;
            }
        }

        [TestMethod]
        public void EnumConversion_GeneratedMetadataPreservesConversionBehavior()
        {
            string source = GenerateMetadata();
            Assert.IsFalse(source.Contains("Convert.ChangeType"));
            Assert.IsFalse(source.Contains("Enum.GetUnderlyingType"));
            foreach (Type enumType in EnumTypes)
            {
                string name = Enum.GetUnderlyingType(enumType).Name;
                StringAssert.Contains(source, "userType.EnumConverter = EnumConverter_" + name + ";");
                StringAssert.Contains(source, "return global::System.Convert.To" + name + "(value);");
            }

            object provider = CompileProvider(source);
            object baseline = CompileProvider(source.Replace("return EnumConverter(value);",
                "return global::System.Convert.ChangeType(value, global::System.Enum.GetUnderlyingType(this.UnderlyingType));"));
            string[] inputs =
            {
                null, "", "Unknown", "One", "oNe", " Max ", "Min", "SignedMax", "One,Two", "One, 2",
                "Max,One", "Min,One", "Unknown,Two", "1,2", "-1,2", "0", "-1", "+1", "~1",
                "-129", "-128", "127", "128", "255", "256", "-32769", "-32768", "32767", "32768",
                "65535", "65536", "-2147483649", "-2147483648", "2147483647", "2147483648",
                "4294967295", "4294967296", "-9223372036854775809", "-9223372036854775808",
                "9223372036854775807", "9223372036854775808", "18446744073709551615",
                "18446744073709551616", "1.0", "1,000", "One,,Two"
            };
            var customCulture = (CultureInfo)CultureInfo.InvariantCulture.Clone();
            customCulture.NumberFormat.NegativeSign = "~";
            CultureInfo originalCulture = CultureInfo.CurrentCulture;
            try
            {
                foreach (CultureInfo culture in new[] { CultureInfo.InvariantCulture, new CultureInfo("tr-TR"), customCulture })
                {
                    CultureInfo.CurrentCulture = culture;
                    foreach (Type enumType in EnumTypes)
                    {
                        object metadata = GetEnumMetadata(provider, enumType);
                        object oldMetadata = GetEnumMetadata(baseline, enumType);
                        foreach (string input in inputs)
                        {
                            string message = enumType.Name + ": " + input + " (" + culture.Name + ")";
                            Exception expectedError, actualError;
                            object expected = ConvertInput(oldMetadata, input, out expectedError);
                            object actual = ConvertInput(metadata, input, out actualError);
                            Assert.AreEqual(expectedError?.GetType(), actualError?.GetType(), message);
                            Assert.AreEqual(expectedError?.Message, actualError?.Message, message);
                            Assert.AreEqual((expectedError as ArgumentException)?.ParamName,
                                (actualError as ArgumentException)?.ParamName, message);
                            Assert.AreEqual(expected, actual, message);
                            if (actualError == null)
                            {
                                Assert.AreEqual(Enum.GetUnderlyingType(enumType), actual.GetType(), message);
                            }
                        }
                    }
                }
                object empty = GetEnumMetadata(provider, typeof(EmptyEnum));
                object oldEmpty = GetEnumMetadata(baseline, typeof(EmptyEnum));
                foreach (string input in new[] { null, "", "0", "One" })
                {
                    Exception expectedError, actualError;
                    Assert.IsNull(ConvertInput(oldEmpty, input, out expectedError));
                    Assert.IsNull(ConvertInput(empty, input, out actualError));
                    Assert.AreEqual(typeof(ArgumentException), actualError.GetType());
                    Assert.AreEqual(expectedError.Message, actualError.Message);
                    Assert.AreEqual(((ArgumentException)expectedError).ParamName, ((ArgumentException)actualError).ParamName);
                }
            }
            finally
            {
                CultureInfo.CurrentCulture = originalCulture;
            }
        }

        public static object CustomConvert(string input) { return 42; }
        public static object OuterCustomConvert(string input) { return 43; }

        [TestMethod]
        public void EnumConversion_CustomAndBoxedConvertersRetainPrecedence()
        {
            object provider = CompileProvider(GenerateMetadata());
            object metadata = GetEnumMetadata(provider, typeof(Int32Enum));
            object boxedMetadata = GetEnumMetadata(provider, typeof(ByteEnum));
            var customMethod = typeof(EnumConversionTests).GetMethod(nameof(CustomConvert));
            foreach (object type in new[] { metadata, boxedMetadata })
            {
                var converter = type.GetType().GetProperty("CreateFromStringMethod");
                converter.SetValue(type, Delegate.CreateDelegate(converter.PropertyType, customMethod));
            }
            Exception error;
            Assert.AreEqual(42, ConvertInput(metadata, null, out error));
            Assert.IsNull(error);

            var outerConverter = metadata.GetType().GetProperty("CreateFromStringMethod");
            outerConverter.SetValue(metadata, Delegate.CreateDelegate(outerConverter.PropertyType,
                typeof(EnumConversionTests).GetMethod(nameof(OuterCustomConvert))));
            metadata.GetType().GetMethod("SetBoxedType").Invoke(metadata, new[] { boxedMetadata });
            var box = metadata.GetType().GetProperty("BoxInstance");
            box.SetValue(metadata, Delegate.CreateDelegate(box.PropertyType, metadata,
                metadata.GetType().GetMethod("BoxType").MakeGenericMethod(typeof(int))));
            Assert.AreEqual(42, ConvertInput(metadata, "18446744073709551616", out error));
            Assert.IsNull(error);
        }
    }
}
