// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using Microsoft.VisualStudio.TestTools.UnitTesting;
using System;
using System.Collections.Generic;
using System.Linq;
using Win8Xaml.CompilerProxies;

namespace UnitTests
{
    [TestClass]
    public class ConditionalsTests
    {
        TestHelper _testHelper;
        CompilerDomRootToken DomTree { get; set; }

        [TestInitialize]
        public void TestInitialize()
        {
            _testHelper = new TestHelper();
        }

        private XamlClassCodeInfo GetClassCodeInfo(string xaml, string additionalNamespaceDeclarations)
        {
            string fullXaml = string.Format(
                @"<Page x:Class='LibManagedDll.ConditionalsTestsClass'
                xmlns:sys='using:System'
                xmlns:dll='using:LibManagedDll'
                xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                xmlns:x ='http://schemas.microsoft.com/winfx/2006/xaml'
                {1}>{0}</Page>",
                xaml,
                additionalNamespaceDeclarations);
            DomTree = _testHelper.LoadXamlDom(fullXaml, SchemaMode.ManagedRuntime | SchemaMode.LoadUserDll);
            var classCodeInfo = _testHelper.HarvestClassCodeInfo(".", DomTree, false, false);
            var fileCodeInfo = _testHelper.HarvestFileCodeInfo(".", false, classCodeInfo, DomTree);
            return classCodeInfo;
        }

        private void TestParseConditionalNamespaceExpectErrors(string additionalNamespaces, string xaml, string [] errorCodes = null, string [] errorMessages = null)
        {
            var classCodeInfo = GetClassCodeInfo(xaml, additionalNamespaces);
            var validator = _testHelper.ValidateXamlDom(DomTree, false);
            var errors = DomTree.Schema.SchemaErrors.Concat(validator.Errors);
            Assert.IsNotNull(errors, string.Format("Expecting error(s)"));
            if (errorCodes != null)
            {
                Assert.AreEqual(errorCodes.Length, errors.Count(), string.Format("Expecting {0} error", errorCodes.Length));
            }
            int currentError = 0;
            foreach (var error in errors)
            {
                if (errorCodes != null && errorCodes[currentError] != null)
                {
                    Assert.AreEqual(errorCodes[currentError], error.ErrorCode, "Expecting a different error code");
                }
                if (errorMessages != null && errorMessages[currentError] != null)
                {
                    Assert.AreEqual(errorMessages[currentError], error.Message, "Expecting a different error message");
                }
                currentError++;
            }
        }

        private void TestParseConditionalNamespaceExpectSuccess(string additionalNamespaces, string xaml)
        {
            var classCodeInfo = GetClassCodeInfo(xaml, additionalNamespaces);
            var validator = _testHelper.ValidateXamlDom(DomTree, false);
            var errors = DomTree.Schema.SchemaErrors.Concat(validator.Errors);
            Assert.AreEqual(0, errors.Count(), string.Format("Expected success, received {0} errors", errors.Count()));
        }
        
        [TestMethod]
        public void Conditionals_ParseConditionalNamespace_Success()
        {
            // ApiContractPresent/NotPresent - 3 params
            TestParseConditionalNamespaceExpectSuccess(
                "xmlns:rs1='http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsApiContractPresent(Windows.Foundation.UniversalApiContract,3,0)'",
                "<rs1:TextBlock Text='Foo'/>"
                );
            TestParseConditionalNamespaceExpectSuccess("xmlns:rs1='http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsApiContractNotPresent(\"Windows.Foundation.UniversalApiContract\",3,0)'",
                "<rs1:TextBlock Text='Foo'/>"
                );
            // ApiContractPresent/NotPresent - 2 params
            TestParseConditionalNamespaceExpectSuccess("xmlns:rs1='http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsApiContractPresent(Windows.Foundation.UniversalApiContract,3)'",
                "<rs1:TextBlock Text='Foo'/>"
                );
            TestParseConditionalNamespaceExpectSuccess("xmlns:rs1='http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsApiContractNotPresent(\"Windows.Foundation.UniversalApiContract\",3)'",
                "<rs1:TextBlock Text='Foo'/>"
                );
            // IsPropertyPresent/NotPresent
            TestParseConditionalNamespaceExpectSuccess("xmlns:rs1='http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsPropertyPresent(\"Foo\", \"Bar\")'",
                "<rs1:TextBlock Text='Foo'/>"
                );
            TestParseConditionalNamespaceExpectSuccess("xmlns:rs1='http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsPropertyNotPresent(\"Foo\", \"Bar\")'",
                "<rs1:TextBlock Text='Foo'/>"
                );
            // IsTypePresent/NotPresent
            TestParseConditionalNamespaceExpectSuccess("xmlns:rs1='http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsTypePresent(\"Foo\")'",
                "<rs1:TextBlock Text='Foo'/>"
                );
            TestParseConditionalNamespaceExpectSuccess("xmlns:rs1='http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsTypeNotPresent(\"Foo\")'",
                "<rs1:TextBlock Text='Foo'/>"
                );

            // Custom predicates. Since 7027415b09 ("Custom Predicate Support for XAML", PR 14429148)
            // a function name that is not one of the six built-ins above is no longer an error -
            // ApiInformation's constructor treats it as a user-supplied predicate
            // (ApiInformation.cs, IsCustomPredicate). These two therefore parse cleanly; they used to
            // be expected failures in _SchemaErrors and _SyntaxErrors respectively.
            TestParseConditionalNamespaceExpectSuccess("xmlns:rs1='http://schemas.microsoft.com/winfx/2006/xaml/presentation?FooBar(Windows.Foundation.UniversalApiContract,3,0)'",
                "<rs1:TextBlock Text='Foo'/>"
                );
            // Note this is also a casing mismatch of IsApiContractPresent ("API" vs "Api"), which is
            // itself enough to make it a custom predicate - see _CaseMissmatchErrors.
            TestParseConditionalNamespaceExpectSuccess("xmlns:rs1='http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsAPIContractPresent(,,)'",
                "<rs1:TextBlock Text='Foo'/>"
                );
        }

        [TestMethod]
        public void Conditionals_ParseConditionalNamespace_SyntaxErrors()
        {
            // Missing closing )
            TestParseConditionalNamespaceExpectErrors(
                "xmlns:rs1='http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsAPIContractPresent(Windows.Foundation.UniversalApiContract,3,0'",
                "<rs1:TextBlock Text='Foo'/>",
                new [] {
                    "WMC0916",
                    "WMC0001"
                    },
                new [] {
                    "Syntax error at '<EOF>'. while parsing conditional namespace expression 'http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsAPIContractPresent(Windows.Foundation.UniversalApiContract,3,0'",
                    null
                    }
                );
            // Missing opening (
            TestParseConditionalNamespaceExpectErrors(
                "xmlns:rs1='http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsAPIContractPresent Windows.Foundation.UniversalApiContract,3,0)'",
                "<rs1:TextBlock Text='Foo'/>",
                new [] {
                    "WMC0916",
                    "WMC0001"
                    },
                new [] {
                    "Syntax error at '<EOF>'. while parsing conditional namespace expression 'http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsAPIContractPresent Windows.Foundation.UniversalApiContract,3,0)'",
                    null
                    }
                );
            // Other syntax errors(
            // NOTE: 'IsAPIContractPresent(,,)' used to be listed here as a third syntax error. The
            // grammar's function_param rule matches any tokens between the parens
            // (ConditionalNamespace.g4), so an empty argument list has never been a *syntax* error;
            // it was rejected because the function name was unknown. Since 7027415b09 unknown names
            // are legal custom predicates, so it now parses cleanly and has moved to
            // Conditionals_ParseConditionalNamespace_Success.
        }

        [TestMethod]
        public void Conditionals_ParseConditionalNamespace_CaseMissmatchErrors()
        {
            // Missmatched casing of ApiInformation
            TestParseConditionalNamespaceExpectErrors(
                "xmlns:rs1='http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsApiConTractPresent(Windows.Foundation.UniversalApiContract,3)'",
                "<rs1:TextBlock Test='Foo'/>"
                );
            TestParseConditionalNamespaceExpectErrors("xmlns:rs1='http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsApiConTractNotPresent(\"Windows.Foundation.UniversalApiContract\",3)'",
                "<rs1:TextBlock Test='Foo'/>"
                );
            TestParseConditionalNamespaceExpectErrors("xmlns:rs1='http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsProperTyPresent(\"Foo\", \"Bar\")'",
                "<rs1:TextBlock Test='Foo'/>"
                );
            TestParseConditionalNamespaceExpectErrors("xmlns:rs1='http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsProperTyNotPresent(\"Foo\", \"Bar\")'",
                "<rs1:TextBlock Test='Foo'/>"
                );
            TestParseConditionalNamespaceExpectErrors("xmlns:rs1='http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsTypePreSent(\"Foo\")'",
                "<rs1:TextBlock Test='Foo'/>"
                );
            TestParseConditionalNamespaceExpectErrors("xmlns:rs1='http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsTypeNotPreSent(\"Foo\")'",
                "<rs1:TextBlock Test='Foo'/>"
                );
        }

        [TestMethod]
        public void Conditionals_ParseConditionalNamespace_SchemaErrors()
        {
            // NOTE: 'FooBar(...)' used to be listed here as an unrecognised API information method.
            // Since 7027415b09 an unknown name is a legal custom predicate, so it now parses cleanly
            // and has moved to Conditionals_ParseConditionalNamespace_Success. What is still
            // validated is the argument count of the six built-in methods.

            // More params than needed
            TestParseConditionalNamespaceExpectErrors("xmlns:rs1='http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsApiContractPresent(Windows.Foundation.UniversalApiContract,3,0,4)'",
                "<rs1:TextBlock Test='Foo'/>",
                new[] {
                    "WMC0916",
                    "WMC0001"
                    },
                new[] {
                    "Unmatched API information parameters for method 'IsApiContractPresent'. while parsing conditional namespace expression 'http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsApiContractPresent(Windows.Foundation.UniversalApiContract,3,0,4)'",
                    null
                    }
                );
            // Less params than needed
            TestParseConditionalNamespaceExpectErrors(
                "xmlns:rs1='http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsApiContractPresent(Windows.Foundation.UniversalApiContract)'",
                "<rs1:TextBlock Test='Foo'/>",
                new[] {
                    "WMC0916",
                    "WMC0001"
                    },
                new[] {
                    "Unmatched API information parameters for method 'IsApiContractPresent'. while parsing conditional namespace expression 'http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsApiContractPresent(Windows.Foundation.UniversalApiContract)'",
                    null
                    }
                );
        }

        [TestMethod]
        public void Conditionals_DuplicatePropertyWithDifferentNS_Success()
        {
            // Invalid Api Information Method
            TestParseConditionalNamespaceExpectSuccess(
                "xmlns:rs1 = 'http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsApiContractPresent(Windows.Foundation.UniversalApiContract,3,0)' " +
                "xmlns:rs2 = 'http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsApiContractPresent(Windows.Foundation.UniversalApiContract,4,0)'",
                "<TextBlock rs1:Text='Foo' rs2:Text='Bar'/>"
                );
        }
    }
}
