// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using Windows.UI;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;

namespace LibManagedDll
{
    public class AnotherClassForPathing
    {
        public string StringFunction() { return "StringFunction"; }

        public string Format(string value) { return String.Format("Format: {0}", value); }
    }

    public class NamedElementForPathing : FrameworkElement
    {
        public string Value { get; set; }

        public string Format(string value) { return String.Format("Format: {0}", value); }
    }

    public class BindPathParserClass
    {
        public Color[] Rainbow = {
            Colors.Red, Colors.Orange, Colors.Yellow, Colors.Violet, Colors.Black, Colors.Indigo, Colors.Green
        };

        public Dictionary<string, Color> RainbowAsString;

        public double Double3dot0 = 3.0;
        public short Short3 = 3;
        public string StringField;
        public string Value;

        public String StringProperty { get; set; }
        public static String StringPropertyStatic { get; set; }

        public String PropertyWithNoGetAccessor { set { } }

        public string GetTipOfTheDay() { return "Tip of the day"; }
        public static string GetTipOfTheDayStatic() { return "Tip of the day static"; }

        public string FormatPosition(int value) { return String.Format("Position: {0}", value); }
        public string FormatPositionFloat(float value) { return String.Format("Position: {0}", value); }
        public string FormatPositionDouble(double value) { return String.Format("Position: {0}", value); }
        public string FormatTitle(string value) { return String.Format("Title: {0}", value); }
        public string FormatTitleAndLevel(string title, double level, bool isManager)
        {
            string titleString = title ?? "null_title";
            return String.Format("Title: {0}, Level: {1}, IsManager: {2}", titleString, level, isManager);
        }

        public string FunctionWithTwoArguments(string title, double level)
        {
            string titleString = title ?? "null_title";
            return String.Format("Title: {0}, Level: {1}", titleString, level);
        }

        public string FunctionWithOutParam(int one, out int two)
        {
            two = 0;
            return "";
        }

        public string FunctionWithRefParam(int one, ref int two)
        {
            return "";
        }

        public void VoidFunction()
        {
        }

        public void OverloadedFunction()
        {
        }

        public void OverloadedFunction(int i)
        {
        }

        public int IntFunction()
        {
            return 0;
        }

        public string ArityTest() { return "ArityTest()"; }
        public string ArityTest(int a) { return "ArityTest(int)"; }
        public string ArityTest(int a, int b) { return "ArityTest(int, int)"; }
        public string ArityTest(int a, int b, int c) { return "ArityTest(int, int, int)"; }
        public string ArityTest(int a, int b, string c) { return "ArityTest(int, int, string)"; }

        public static explicit operator Thickness(BindPathParserClass instance)
        {
            return ThicknessHelper.FromUniformLength(20);
        }

        public Button SomeButton { get; }

        public object NullObject { get; }

        public AnotherClassForPathing InnerClass { get; }

        public BindPathParserClass()
        {
            foreach (var color in Rainbow)
            {
                RainbowAsString.Add(color.ToString(), color);
            }
        }
    }
}
