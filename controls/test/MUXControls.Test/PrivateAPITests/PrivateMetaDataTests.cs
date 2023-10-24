using System;
using System.Linq;
using System.Reflection;
using System.Collections.ObjectModel;
using System.Collections.Generic;
using System.Threading;
using System.Resources;
using MUXControls.Test;
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public class PrivateMetaDataTests
    {
        private Assembly WinUIDll;
        [ClassInitialize()]
        public void ClassInit(TestContext context)
        {
            DebugHelper.Debug(context);
            var currentPath = global::System.IO.Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location);
            //Reflection only load is not supported. Using LoadFile instead to inspect the dll's metadata
            WinUIDll = Assembly.LoadFile($"{currentPath}\\PrivateAPITests\\Microsoft.WinUI.dll");
            Log.Comment(currentPath);
            Verify.IsNotNull(WinUIDll);        
        }

        [TestMethod]
        [Description("Validates that there are no types defined in the Microsoft.UI.Private namespace")]
        public void PrivateNamespacesTest()
        {
            var types = from t in WinUIDll.GetTypes()
                        where t.FullName.Contains("Microsoft.UI.Private")
                        select t;

            foreach (Type t in types)
            {
                Log.Comment($"{t.FullName} is a typed defined in the Microsoft.UI.Private namespace and should not exist \n");
            }

            Verify.IsTrue(!types.Any());
        }

        [TestMethod]
        [Description("Validates that there are no types that have the PrivateApiContract attribute on them")]
        public void PrivateAPIContractTest()
        {        
            var types = WinUIDll.GetTypes();
            bool badAttributeEncountered = false;
            foreach (var t in types)
            {
                foreach (var customAttribute in CustomAttributeData
                    .GetCustomAttributes(t)
                    .Where(customAttribute => customAttribute.AttributeType.FullName.Contains("ContractVersionAttribute")))
                {
                    foreach (var argument in customAttribute
                        .ConstructorArguments
                        .Union(customAttribute.NamedArguments.Select(namedArgument => namedArgument.TypedValue))
                        .Where(argument => (argument.Value ?? String.Empty)
                        .ToString()
                        .Contains("PrivateApiContract")))
                    {
                        Log.Comment("Type: {0}", t.FullName);
                        Log.Comment("          Attribute: {0}", customAttribute.AttributeType.FullName);
                        Log.Comment("        Argument                   Type: '{0}'  Value: '{1}'\n", argument.ArgumentType, argument.Value);
                        badAttributeEncountered = true;
                    }
                }
            }
            Verify.IsFalse(badAttributeEncountered);
        }
    }
}