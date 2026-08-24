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
    public class BindPathParserTests
    {
        TestHelper _testHelper;

        [TestInitialize]
        public void TestInitialize()
        {
            _testHelper = new TestHelper();
        }

        private XamlClassCodeInfo GetClassCodeInfo(string xaml)
        {
            string fullXaml = string.Format(
                @"<Page x:Class='LibManagedDll.BindPathParserClass'
                xmlns:sys='using:System'
                xmlns:dll='using:LibManagedDll'
                xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                xmlns:x ='http://schemas.microsoft.com/winfx/2006/xaml'>{0}</Page>",
                xaml);
            var domTree = _testHelper.LoadXamlDom(fullXaml, SchemaMode.ManagedRuntime | SchemaMode.LoadUserDll);
            var classCodeInfo = _testHelper.HarvestClassCodeInfo(".", domTree, false, false);
            var fileCodeInfo = _testHelper.HarvestFileCodeInfo(".", false, classCodeInfo, domTree);
            return classCodeInfo;
        }

        private IEnumerable<XamlCompileError> ParseBindUniverse(string xaml)
        {
            var classCodeInfo = GetClassCodeInfo(xaml);
            return classCodeInfo.BindUniverses[0].Parse(classCodeInfo);
        }

        private IEnumerable<XamlCompileError> TestParsePathExpectOneError(string xaml, string errorCode, string errorMessage)
        {
            var classCodeInfo = GetClassCodeInfo(xaml);
            var errors = classCodeInfo.BindUniverses[0].Parse(classCodeInfo);
            Assert.IsNotNull(errors, string.Format("Expecting one error"));
            Assert.AreEqual(1, errors.Count(), string.Format("Expecting one error"));
            foreach (var error in errors)
            {
                Assert.AreEqual(errorCode, error.ErrorCode, "Expecting a different error code");
                Assert.AreEqual(errorMessage, error.Message, "Expecting a different error message");
                break; // one iteration loop
            }
            return errors;
        }

        private void TestParsePathExpectSuccess(string xaml)
        {
            var classCodeInfo = GetClassCodeInfo(xaml);
            var errors = classCodeInfo.BindUniverses[0].Parse(classCodeInfo);
            Assert.AreEqual(0, errors.Count(), string.Format("Expected success, received {0} errors", errors.Count()));
        }

        [TestMethod]
        public void ParsePath_TestUnclosedIndexOperatorFailure()
        {
            // a path that opens a [ but doesn't close it
            try
            {
                TestParsePathExpectSuccess("<TextBlock Text='{x:Bind [}'/>");
            }
            catch(Exception e)
            {
                // WMC9997
                Assert.AreEqual(e.InnerException.Message, "'Unexpected token 'None' in rule: 'MarkupExtension ::= '{' TYPENAME (Arguments)? @'}'', in '{x:Bind [}'.' Line number '5' and line position '94'.");
            }
        }

        [TestMethod]
        public void ParsePath_TestIndexNonIntegerFailure()
        {
            // a path that with an index operator that has a non-number index
            TestParsePathExpectOneError("<TextBlock Text='{x:Bind StringProperty[7.3]}'/>",
                "WMC1110", "Invalid binding path 'StringProperty[7.3]' : Syntax error at '.'.");
        }

        [TestMethod]
        public void ParsePath_TestMissingClosingBracket()
        {
            TestParsePathExpectSuccess("<TextBlock Text='{x:Bind StringField}' Height='{Binding StringProperty[7}'/>");
        }

        [TestMethod]
        public void ParsePath_TestMissingOpeningBracket()
        {
            TestParsePathExpectSuccess("<TextBlock Text='{x:Bind StringField}' Height='{Binding StringProperty7]}'/>");
        }

        [TestMethod]
        public void ParsePath_TestIndexNonCollectionFailure()
        {
            // a path that with an index operator that has a non-number index
            TestParsePathExpectOneError("<TextBlock Text='{x:Bind SomeButton[7]}'/>",
                "WMC1110", "Invalid binding path 'SomeButton[7]' : Unexpected array indexer.");
        }

        [TestMethod]
        public void ParsePath_TestDoubleDotFailure()
        {
            // a path that has two dots in a row
            TestParsePathExpectOneError("<TextBlock Text='{x:Bind ..}'/>",
                "WMC1110", "Invalid binding path '..' : Syntax error at '.'.");
        }

        [TestMethod]
        public void ParsePath_TestUnclosedParenthesisFailure()
        {
            // a path that has an open parenthesis ( but not a close
            try
            {
                TestParsePathExpectSuccess("<TextBlock Text='{x:Bind (}'/>");
            }
            catch (Exception e)
            {
                // WMC9997
                Assert.AreEqual(e.InnerException.Message, "'Unexpected token 'None' in rule: 'MarkupExtension ::= '{' TYPENAME (Arguments)? @'}'', in '{x:Bind (}'.' Line number '5' and line position '94'.");
            }
        }

        [TestMethod]
        public void ParsePath_TestUnrecognizedProperty()
        {
            // A well formed cast of a known type but unrecognized property
            // should fail like (x:Int32)DoesNotExist.  Confirm this fails.
            TestParsePathExpectOneError("<TextBlock Text='{x:Bind (x:Int32)DoesNotExist}'/>",
                "WMC1110", "Invalid binding path '(x:Int32)DoesNotExist' : Property 'DoesNotExist' not found on type 'BindPathParserClass'.");
            TestParsePathExpectOneError("<TextBlock Text='{x:Bind DoesNotExist}'/>",
                "WMC1110", "Invalid binding path 'DoesNotExist' : Property 'DoesNotExist' not found on type 'BindPathParserClass'.");
        }

        [TestMethod]
        public void ParsePath_TestRootFieldStep()
        {
            // A straight string without tokens as a path input will try to match
            // a field defined in the rootFieldDefinitions input.  Confirm this
            // results in a RootFieldStep output.
            TestParsePathExpectSuccess("<TextBlock Text='{x:Bind StringField}'/>");
        }

        [TestMethod]
        public void ParsePath_TestIdentifier()
        {
            TestParsePathExpectSuccess("<TextBlock Text='{x:Bind StringProperty}'/>");
        }

        [TestMethod]
        public void ParsePath_TestIdentifierCalledValue()
        {
            TestParsePathExpectSuccess("<TextBlock Text='{x:Bind Value}'/>");
        }

        [TestMethod]
        public void ParsePath_TestStaticIdentifier()
        {
            TestParsePathExpectSuccess("<TextBlock Text='{x:Bind sys:DateTime.Today}'/>");
        }

        [TestMethod]
        public void ParsePath_TestStaticIdentifierAsInstance()
        {
            TestParsePathExpectSuccess("<TextBlock Text='{x:Bind StringPropertyStatic}'/>");
        }

        [TestMethod]
        public void ParsePath_TestPathDotIdentifier()
        {
            TestParsePathExpectSuccess("<TextBlock Text='{x:Bind StringProperty.Length}'/>");
        }

        [TestMethod]
        public void ParsePath_TestCantTwoWayCastStepError()
        {
            TestParsePathExpectOneError("<TextBlock Margin='{x:Bind (Thickness), Mode=TwoWay}' />",
                "WMC1121", "Invalid binding assignment : TwoWay binding is invalid when the binding expression ends with a cast");
        }

        [TestMethod]
        public void ParsePath_TestPathCast()
        {
            TestParsePathExpectSuccess("<CheckBox Padding='{x:Bind (Thickness)}' />");
            TestParsePathExpectOneError("<CheckBox IsChecked='{x:Bind (x:Boolean)}' />",
                "WMC1110", "Invalid binding path '(x:Boolean)' : Unable to cast type 'LibManagedDll.BindPathParserClass' to 'System.Boolean'.  Use a converter or function binding to change the type.");
        }

        [TestMethod]
        public void ParsePath_TestPathCastPath()
        {
            TestParsePathExpectSuccess("<TextBlock Text='{x:Bind (sys:String)SomeButton.Tag}'/>");
        }

        [TestMethod]
        public void ParsePath_TestPathCastPathParen()
        {
            TestParsePathExpectSuccess("<TextBlock Text='{x:Bind ((sys:String)SomeButton.Tag).Length}'/>");
        }

        [TestMethod]
        public void ParsePath_TestCastStartsWithAttachedPropertyError()
        {
            TestParsePathExpectOneError("<CheckBox Grid.Row='{x:Bind (Grid.Row)SomeButton}' />",
                "WMC1110", "Invalid binding path '(Grid.Row)SomeButton' : Starting a cast with an Attached Property is not supported.  Try a syntax like: path.(owner.property).");
        }

        [TestMethod]
        public void ParsePath_TestUnconsumedPathRemainderError()
        {
            TestParsePathExpectOneError("<TextBlock Margin='{x:Bind (Grid)NullObject).Margin}' />",
                "WMC1110", "Invalid binding path '(Grid)NullObject).Margin' : Syntax error at ').Margin'.");
        }

        [TestMethod]
        public void ParsePath_TestPathDotAttached()
        {
            TestParsePathExpectSuccess("<TextBlock Text='{x:Bind StringProperty.(sys:String.Length)}'/>");
            TestParsePathExpectSuccess("<TextBlock Text='{x:Bind SomeButton.(AutomationProperties.Name)}'/>");
        }

        [TestMethod]
        public void ParsePath_TestIndexer()
        {
            TestParsePathExpectSuccess("<TextBlock Text='{x:Bind Rainbow[1].A}'/>");
        }

        [TestMethod]
        public void ParsePath_TestStringIndexer()
        {
            TestParsePathExpectSuccess("<TextBlock Text=\"{x:Bind RainbowAsString['red']}\"/>");
            TestParsePathExpectSuccess("<TextBlock Text='{x:Bind RainbowAsString[\"red\"]}'/>");
        }

        [TestMethod]
        public void ParsePath_FunctionType_OnRoot()
        {
            TestParsePathExpectSuccess(@"<TextBlock Text='{x:Bind GetTipOfTheDay()}'/>");
        }

        [TestMethod]
        public void ParsePath_FunctionType_OnModel()
        {
            TestParsePathExpectSuccess(@"<TextBlock Text='{x:Bind Rainbow[1].ToString()}'/>");
        }

        [TestMethod]
        public void ParsePath_FunctionType_Static()
        {
            TestParsePathExpectSuccess(@"<TextBlock Text='{x:Bind dll:BindPathParserClass.GetTipOfTheDayStatic()}'/>");
        }

        [TestMethod]
        public void ParsePath_FunctionParameterCount_Zero()
        {
            TestParsePathExpectSuccess(@"<TextBlock Text='{x:Bind Rainbow[1].ToString()}'/>");
        }

        [TestMethod]
        public void ParsePath_FunctionParameterCount_OneButGotZero()
        {
            TestParsePathExpectOneError(@"<TextBlock Text='{x:Bind FormatTitle()}'/>",
                "WMC1110", "Invalid binding path 'FormatTitle()' : Missmatched function parameter count.");
        }

        [TestMethod]
        public void ParsePath_FunctionParameterCount_One()
        {
            TestParsePathExpectSuccess("<TextBlock Text=\"{x:Bind FormatTitle('SDE')}\"/>");
        }

        [TestMethod]
        public void ParsePath_FunctionParameterCount_Three()
        {
            TestParsePathExpectSuccess("<TextBlock Text=\"{x:Bind FormatTitleAndLevel(x:Null, -3.5, x:False)}\"/>");
        }

        [TestMethod]
        public void ParsePath_FunctionParameterCount_Arity()
        {
            TestParsePathExpectSuccess("<TextBlock Text=\"{x:Bind ArityTest()}\"/>");
            TestParsePathExpectSuccess("<TextBlock Text=\"{x:Bind ArityTest(1)}\"/>");
            TestParsePathExpectSuccess("<TextBlock Text=\"{x:Bind ArityTest(1, 2)}\"/>");
            TestParsePathExpectSuccess("<TextBlock Text=\"{x:Bind ArityTest(1, 2, 3)}\"/>");
        }

        [TestMethod]
        public void ParsePath_FunctionParameterCount_ArityParamCountMissmatch()
        {
            TestParsePathExpectOneError("<TextBlock Text=\"{x:Bind ArityTest(1, 2, 3, 4)}\"/>", 
                "WMC1110", "Invalid binding path 'ArityTest(1, 2, 3, 4)' : Cannot find an overload that takes 4 parameters.");
        }

        [TestMethod]
        public void ParsePath_FunctionParameterCount_ArityParamTypeMissmatch()
        {
            TestParsePathExpectOneError("<TextBlock Text=\"{x:Bind ArityTest(1, 2, '3')}\"/>",
                "WMC1110", "Invalid binding path 'ArityTest(1, 2, '3')' : Invalid or missmatched parameter at position '3'.");
        }

        [TestMethod]
        public void ParsePath_FunctionParameterType_QuotedString()
        {
            TestParsePathExpectSuccess("<TextBlock Text=\"{x:Bind FormatTitle('SDE')}\"/>");
        }

        [TestMethod]
        public void ParsePath_FunctionParameterType_Integer()
        {
            TestParsePathExpectSuccess("<TextBlock Text='{x:Bind FormatPosition(2)}'/>");
        }

        [TestMethod]
        public void ParsePath_FunctionParameterType_Float()
        {
            TestParsePathExpectSuccess("<TextBlock Text='{x:Bind FormatPositionDouble(-2.1)}'/>");
            TestParsePathExpectSuccess("<TextBlock Text='{x:Bind FormatPositionDouble(0.1)}'/>");
        }

        [TestMethod]
        public void ParsePath_FunctionParameterType_Double()
        {
            TestParsePathExpectSuccess("<TextBlock Text='{x:Bind FormatPositionDouble(-2.1)}'/>");
            TestParsePathExpectSuccess("<TextBlock Text='{x:Bind FormatPositionDouble(0.1)}'/>");
        }

        [TestMethod]
        public void ParsePath_FunctionParameterType_Bool()
        {
            TestParsePathExpectSuccess("<TextBlock Text=\"{x:Bind FormatTitleAndLevel('SDE', -3.5, x:False)}\"/>");
            TestParsePathExpectSuccess("<TextBlock Text=\"{x:Bind FormatTitleAndLevel('SDE', -3.5, x:True)}\"/>");
        }

        [TestMethod]
        public void ParsePath_FunctionParameterType_Null()
        {
            TestParsePathExpectSuccess("<TextBlock Text=\"{x:Bind FormatTitleAndLevel(x:Null, -3.5, x:False)}\"/>");
        }

        [TestMethod]
        public void ParsePath_FunctionParameterType_UpCastShortToInt()
        {
            TestParsePathExpectSuccess("<TextBlock Text='{x:Bind FormatPosition(Short3)}'/>");
        }

        [TestMethod]
        public void ParsePath_FunctionParameterInvalid_BoolInsteadOfDouble()
        {
            TestParsePathExpectOneError("<TextBlock Text=\"{x:Bind FormatTitleAndLevel('SDE', x:False, x:False)}\"/>",
                "WMC1110", "Invalid binding path 'FormatTitleAndLevel('SDE', x:False, x:False)' : Invalid or missmatched parameter at position '2'.");
        }

        [TestMethod]
        public void ParsePath_FunctionParameterInvalid_IntInsteadOfBool()
        {
            TestParsePathExpectOneError("<TextBlock Text=\"{x:Bind FormatTitleAndLevel('SDE', 3.5, 2)}\"/>",
                "WMC1110", "Invalid binding path 'FormatTitleAndLevel('SDE', 3.5, 2)' : Invalid or missmatched parameter at position '3'.");
        }

        [TestMethod]
        public void ParsePath_FunctionParameterInvalid_StringInsteadOfBool()
        {
            TestParsePathExpectOneError("<TextBlock Text=\"{x:Bind FormatTitleAndLevel('SDE', 3.5, 'SDE')}\"/>",
                "WMC1110", "Invalid binding path 'FormatTitleAndLevel('SDE', 3.5, 'SDE')' : Invalid or missmatched parameter at position '3'.");
        }

        [TestMethod]
        public void ParsePath_FunctionParameterInvalid_FloatInsteadOfInt()
        {
            TestParsePathExpectOneError("<TextBlock Text='{x:Bind FormatPosition(3.5)}'/>",
                "WMC1110", "Invalid binding path 'FormatPosition(3.5)' : Invalid or missmatched parameter at position '1'.");
        }

        [TestMethod]
        public void ParsePath_FunctionParameterInvalid_LargeNumber()
        {
            TestParsePathExpectOneError("<TextBlock Text='{x:Bind FormatPosition(555555555555555555555555555555555555555555555555)}'/>",
                "WMC1110", "Invalid binding path 'FormatPosition(555555555555555555555555555555555555555555555555)' : Invalid or missmatched parameter at position '1'.");
        }

        [TestMethod]
        public void ParsePath_FunctionParameterType_InstanceProperty()
        {
            TestParsePathExpectSuccess("<TextBlock Text='{x:Bind FormatTitle(StringProperty)}'/>");
        }

        [TestMethod]
        public void ParsePath_FunctionParameterType_StaticProperty()
        {
            TestParsePathExpectSuccess("<TextBlock Text='{x:Bind FormatTitle(dll:BindPathParserClass.StringPropertyStatic)}'/>");
        }

        [TestMethod]
        public void ParsePath_FunctionSyntax_OnlyAsLeaf()
        {
            // function '.' path
            TestParsePathExpectOneError(@"<TextBlock Text='{x:Bind GetTipOfTheDay().ToString()}'/>",
                "WMC1110", "Invalid binding path 'GetTipOfTheDay().ToString()' : Functions are only supported at the end of the bind path.");

            // path '.' function '.' path
            TestParsePathExpectOneError("<TextBlock Text='{x:Bind InnerClass.StringFunction().Length}' />",
                "WMC1110", "Invalid binding path 'InnerClass.StringFunction().Length' : Functions are only supported at the end of the bind path.");

            // static_type '.' function '.' path
            TestParsePathExpectOneError(@"<TextBlock Text='{x:Bind dll:BindPathParserClass.GetTipOfTheDayStatic().ToString()}'/>",
                "WMC1110", "Invalid binding path 'dll:BindPathParserClass.GetTipOfTheDayStatic().ToString()' : Functions are only supported at the end of the bind path.");
        }

        [TestMethod]
        public void ParsePath_FunctionSyntax_DirectlyOnNamespace()
        {
            TestParsePathExpectOneError(@"<TextBlock Text='{x:Bind dll:GetTipOfTheDay()}'/>",
                "WMC1110", "Invalid binding path 'dll:GetTipOfTheDay()' : Syntax error at '('.");
        }

        [TestMethod]
        public void ParsePath_FunctionSyntax_ParameterNotAnotherFunction()
        {
            TestParsePathExpectOneError(@"<TextBlock Text='{x:Bind FormatTitle(GetTipOfTheDay())}'/>",
                "WMC1110", "Invalid binding path 'FormatTitle(GetTipOfTheDay())' : Functions cannot have other functions as parameters.");
        }

        [TestMethod]
        public void ParsePath_FunctionSyntax_PropertyCalledAsFunction()
        {
            TestParsePathExpectOneError(@"<TextBlock Text='{x:Bind StringProperty()}'/>",
                "WMC1110", "Invalid binding path 'StringProperty()' : Expecting a method.");
        }

        [TestMethod]
        public void ParsePath_FunctionSyntax_VoidFunction()
        {
            TestParsePathExpectOneError(@"<TextBlock Text='{x:Bind VoidFunction()}'/>",
                "WMC1121", "Invalid binding assignment : Function return type 'System.Void' must match binding target type 'System.String'");
        }

        [TestMethod]
        public void ParsePath_FunctionSyntax_IntFunction()
        {
            // This binding is supported now because a .ToString() will be generated
            TestParsePathExpectSuccess(@"<TextBlock Text='{x:Bind IntFunction()}'/>");
        }

        [TestMethod]
        public void ParsePath_FunctionSyntax_FunctionWithRefParam()
        {
            TestParsePathExpectOneError("<TextBlock Text='{x:Bind FunctionWithRefParam(0, 1)}'/>",
                "WMC1110", "Invalid binding path 'FunctionWithRefParam(0, 1)' : Unsuported ref/out parameter at position '2'.");
        }

        [TestMethod]
        public void ParsePath_FunctionSyntax_FunctionWithOutParam()
        {
            TestParsePathExpectOneError("<TextBlock Text='{x:Bind FunctionWithOutParam(0, 1)}'/>",
                "WMC1110", "Invalid binding path 'FunctionWithOutParam(0, 1)' : Unsuported ref/out parameter at position '2'.");
        }

        [TestMethod]
        public void ParsePath_FunctionBindBack_NotATwoWayBinding()
        {
            TestParsePathExpectOneError(@"<TextBox Text='{x:Bind Rainbow[1].ToString(), BindBack=FormatTitle}'/>",
                "WMC1121", "Invalid binding assignment : Unexpected BindBack found");
        }

        [TestMethod]
        public void ParsePath_FunctionBindBack_Missing()
        {
            TestParsePathExpectOneError(@"<TextBox Text='{x:Bind Rainbow[1].ToString(), Mode=TwoWay}'/>",
                "WMC1121", "Invalid binding assignment : BindBack was expected, but not found");
        }

        [TestMethod]
        public void ParsePath_FunctionBindBack_Missing_ButOK()
        {
            // It's ok to not have BindBack on a TwoWay non-function binding
            TestParsePathExpectSuccess(@"<TextBox Text='{x:Bind Rainbow[1], Mode=TwoWay}'/>");
        }

        [TestMethod]
        public void ParsePath_FunctionBindBack_NotAFunction()
        {
            TestParsePathExpectOneError(@"<TextBox Text='{x:Bind Rainbow[1].ToString(), BindBack=Rainbow[1], Mode=TwoWay}'/>",
                "WMC1121", "Invalid binding assignment : BindBack must point to a method");
        }

        [TestMethod]
        public void ParsePath_FunctionBindBack_IncorrectFunctionParametersZero()
        {
            TestParsePathExpectOneError(@"<TextBox Text='{x:Bind Rainbow[1].ToString(), BindBack=dll:BindPathParserClass.GetTipOfTheDay, Mode=TwoWay}'/>",
                "WMC1121", "Invalid binding assignment : BindBack must point to a method that takes one argument of type 'System.String'");
        }

        [TestMethod]
        public void ParsePath_FunctionBindBack_IncorrectFunctionParametersThree()
        {
            TestParsePathExpectOneError(@"<TextBox Text='{x:Bind Rainbow[1].ToString(), BindBack=dll:BindPathParserClass.FormatTitleAndLevel, Mode=TwoWay}'/>",
                "WMC1121", "Invalid binding assignment : BindBack must point to a method that takes one argument of type 'System.String'");
        }

        [TestMethod]
        public void ParsePath_FunctionBindBack_Valid()
        {
            TestParsePathExpectSuccess(@"<TextBox Text='{x:Bind Rainbow[1].ToString(), BindBack=FormatTitle, Mode=TwoWay}'/>");
        }

        [TestMethod]
        public void ParsePath_FunctionBindBack_InvalidPath()
        {
            TestParsePathExpectOneError(@"<TextBox Text='{x:Bind Rainbow[1].ToString(), BindBack=Inva.Lid.Path, Mode=TwoWay}'/>",
                "WMC1110", "Invalid binding path 'Inva.Lid.Path' : Property 'Inva' not found on type 'BindPathParserClass'.");
        }

        [TestMethod]
        public void ParsePath_FunctionWithConverterNotSupported()
        {
            TestParsePathExpectOneError(@"<TextBox Text='{x:Bind Rainbow[1].ToString(), Converter={StaticResource ConvertSomething}}'/>",
                "WMC1121", "Invalid binding assignment : Using a converter with function binding is not supported");
        }

        [TestMethod]
        public void ParsePath_NeedConverter()
        {
            TestParsePathExpectOneError(@"<TextBox Visibility='{x:Bind Rainbow[1]}'/>",
                "WMC1121", "Invalid binding assignment : Cannot directly bind type 'Windows.UI.Color' to 'Microsoft.UI.Xaml.Visibility'. Use a cast, converter or function binding to change the type");
        }

        [TestMethod]
        public void ParsePath_ConverterLanguageWithoutConverter()
        {
            TestParsePathExpectOneError(@"<TextBox Text='{x:Bind Rainbow[1], ConverterLanguage=eng}'/>",
                "WMC1121", "Invalid binding assignment : ConverterLanguage cannot be specified without an actual converter");
        }

        [TestMethod]
        public void ParsePath_ConverterParameterWithoutConverter()
        {
            TestParsePathExpectOneError(@"<TextBox Text='{x:Bind Rainbow[1], ConverterParameter=eng}'/>",
                "WMC1121", "Invalid binding assignment : ConverterParameter cannot be specified without an actual converter");
        }

        [TestMethod]
        public void ParsePath_PathSetTwiceError()
        {
            // It's not valid for the path to be set twice like
            // { x:Bind Username, Path=DataContext} so confirm
            // this results in an exception.
            TestParsePathExpectOneError(@"<TextBlock Text='{x:Bind Username, Path=DataContext}'/>",
                "WMC1110", "Invalid binding path 'DataContext' : The value for Path is set twice.");
        }

        [TestMethod]
        public void ParsePath_FunctionParameterType_ExcapedStrings()
        {
            TestParsePathExpectSuccess("<TextBlock Text=\"{x:Bind FormatTitle('It^'s too big')}\"/>");
            TestParsePathExpectSuccess("<TextBlock Text='{x:Bind FormatTitle(\"Tese are some ^\" (quotes)\")}'/>");
            // TODO: test that the function parameter was properly unescaped
        }

        [TestMethod]
        public void ParsePath_Event_Valid()
        {
            TestParsePathExpectSuccess(@"<Button Click='{x:Bind VoidFunction}'/>");
        }

        [TestMethod]
        public void ParsePath_Event_BindingToPropertyNotSupported()
        {
            TestParsePathExpectOneError(@"<Button Click='{x:Bind StringProperty}'/>",
                 "WMC1121", "Invalid binding assignment : Event 'Click' can only be bound to properties of delegate type RoutedEventHandler");
        }

        [TestMethod]
        public void ParsePath_Event_OverloadedFunctionNotSupported()
        {
            TestParsePathExpectOneError(@"<Button Click='{x:Bind OverloadedFunction}'/>",
                 "WMC1121", "Invalid binding assignment : Events can only be bound to non-overloaded methods");
        }

        [TestMethod]
        public void ParsePath_Event_InvalidMethodSignature()
        {
            TestParsePathExpectOneError(@"<Button Click='{x:Bind FormatTitle}'/>",
                 "WMC1121", "Invalid binding assignment : Invalid signature for event 'Click'. Events can only be bound to methods that match the event signature or are parameterless");
        }

        [TestMethod]
        public void ParsePath_Event_MethodSignatureMissmatch()
        {
            TestParsePathExpectOneError(@"<Button Click='{x:Bind FunctionWithTwoArguments}'/>",
                 "WMC1121", "Invalid binding assignment : Invalid signature for event 'Click'. Parameter 0 of type 'String' is not assignable from type 'Object'");
        }

        [TestMethod]
        public void ParsePath_Property_WithoutGetAccessor()
        {
            TestParsePathExpectOneError(@"<Button Click='{x:Bind PropertyWithNoGetAccessor}'/>",
                 "WMC1110", "Invalid binding path 'PropertyWithNoGetAccessor' : Property 'PropertyWithNoGetAccessor' on type 'BindPathParserClass' does not have a 'get' accessor.");
        }

        [TestMethod]
        public void ParsePath_Property_InstancePropertyAsStaticCall()
        {
            TestParsePathExpectOneError(@"<Button Click='{x:Bind dll:BindPathParserClass.StringProperty}'/>",
                 "WMC1110", "Invalid binding path 'dll:BindPathParserClass.StringProperty' : Expecting 'StringProperty' on type 'BindPathParserClass' to be a static property.");
        }

        [TestMethod]
        public void ParsePath_Property_InstanceFunctionAsStaticCall()
        {
            TestParsePathExpectOneError(@"<Button Click='{x:Bind dll:BindPathParserClass.GetTipOfTheDay()}'/>",
                 "WMC1110", "Invalid binding path 'dll:BindPathParserClass.GetTipOfTheDay()' : Expecting 'GetTipOfTheDay' on type 'BindPathParserClass' to be a static function.");
        }

        [TestMethod]
        public void UpdateSourceTrigger_Valid()
        {
            TestParsePathExpectSuccess(@"<TextBox Text='{x:Bind StringProperty, Mode=TwoWay, UpdateSourceTrigger=Default}'/>");
            TestParsePathExpectSuccess(@"<TextBox Text='{x:Bind StringProperty, Mode=TwoWay, UpdateSourceTrigger=PropertyChanged}'/>");
            TestParsePathExpectSuccess(@"<TextBox Text='{x:Bind StringProperty, Mode=TwoWay, UpdateSourceTrigger=LostFocus}'/>");
        }

        [TestMethod]
        public void UpdateSourceTrigger_InvalidEnum()
        {
            TestParsePathExpectOneError(@"<TextBox Text='{x:Bind StringProperty, Mode=TwoWay, UpdateSourceTrigger=InvalidEnum}'/>",
                "WMC1121", "Invalid binding assignment : Unrecognized value for 'UpdateSourceTrigger'");
        }

        [TestMethod]
        public void UpdateSourceTrigger_ExplicitNotSupported()
        {
            TestParsePathExpectOneError(@"<TextBox Text='{x:Bind StringProperty, Mode=TwoWay, UpdateSourceTrigger=Explicit}'/>",
                "WMC1121", "Invalid binding assignment : 'Explicit' is not a supported value for 'UpdateSourceTrigger'");
        }

        [TestMethod]
        public void UpdateSourceTrigger_UpdateSourceTriggerOnlyOnTwoWay()
        {
            TestParsePathExpectOneError(@"<TextBox Text='{x:Bind StringProperty, UpdateSourceTrigger=Default}'/>",
                "WMC1121", "Invalid binding assignment : 'UpdateSourceTrigger' may only be used with binding 'Mode=TwoWay'");
        }

        [TestMethod]
        public void UpdateSourceTrigger_LostFocusMissing()
        {
             TestParsePathExpectOneError(@"<Grid><Grid.Background><SolidColorBrush Color='{x:Bind StringProperty, Mode=TwoWay, UpdateSourceTrigger=LostFocus}'/></Grid.Background></Grid>",
                "WMC1121", "Invalid binding assignment : 'LostFocus' is only supported with members that expose a 'LostFocus' event");
        }


        [TestMethod]
        public void UpdateSourceTrigger_TwoWayNotADP()
        {
            TestParsePathExpectOneError(@"<TextBox SelectedText='{x:Bind StringProperty, Mode=TwoWay, UpdateSourceTrigger=PropertyChanged}'/>",
                "WMC1121", "Invalid binding assignment : 'PropertyChanged' is only supported with members that are DependencyProperty");
        }

        // <TextBlock Foreground='{x:Bind Background, FallbackValue={StaticResource testKey}}' />
        [TestMethod]
        public void FallbackValue_InvalidValue()
        {
            TestParsePathExpectOneError(@"<Grid><Grid.Resources><x:String x:Key='testKey'>Red</x:String></Grid.Resources> <TextBlock Text='{x:Bind StringProperty, FallbackValue={StaticResource testKey}}' /> </Grid>",
               "WMC1121", "Invalid binding assignment : Only string and {x:Null} are supported for FallbackValue");
        }
    }
}
