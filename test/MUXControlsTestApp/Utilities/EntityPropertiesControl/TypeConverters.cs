// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Reflection;
using Windows.Foundation;
using Windows.UI;
using Windows.UI.Text;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Markup;
using Windows.UI.Xaml.Media;

namespace MUXControlsTestApp.Utilities
{
    public class EnumConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, string language)
        {
            if (targetType.Name == "String" || targetType.Name == "Object")
            {
                if (value == null)
                {
                    return "Null";
                }

                Type enumType = value.GetType();
                bool valueTypeIsInt = false;

                if (!enumType.GetTypeInfo().IsEnum && parameter != null)
                {
                    enumType = parameter as Type;
                    valueTypeIsInt = true;
                }

                if (enumType.GetTypeInfo().IsEnum)
                {
#if !ARM64
                    foreach (FieldInfo fieldInfo in enumType.GetFields(BindingFlags.Public | BindingFlags.Static))
                    {
                        if (fieldInfo.GetValue(null).Equals(value) || (valueTypeIsInt && (int)fieldInfo.GetValue(null) == (int)value))
                            return fieldInfo.Name;
                    }
#endif
                }
            }

            return value;
        }

        public object ConvertBack(object value, Type targetType, object parameter, string language)
        {
            if (value != null)
            {
                string valueAsStr = value as string;
                if (!string.IsNullOrWhiteSpace(valueAsStr))
                {
                    if (valueAsStr == "Null")
                    {
                        return null;
                    }

                    Type enumType = targetType;

                    if (!enumType.GetTypeInfo().IsEnum && parameter != null)
                    {
                        enumType = parameter as Type;
                    }

                    if (enumType.GetTypeInfo().IsEnum)
                    {
#if !ARM64
                        foreach (FieldInfo fieldInfo in enumType.GetFields(BindingFlags.Public | BindingFlags.Static))
                        {
                            if (fieldInfo.Name == valueAsStr)
                                return fieldInfo.GetValue(null);
                        }
#endif
                    }
                }
            }

            return value;
        }
    }

    public class SolidColorBrushConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, string language)
        {
            if (targetType.Name == "String" || targetType.Name == "Object")
            {
                if (value == null)
                    return "Null";

                SolidColorBrush scb = value as SolidColorBrush;
                if (scb != null)
                {
                    string str = scb.Color.ToString();

#if !ARM64
                    foreach (PropertyInfo colorPropertyInfo in typeof(Colors).GetProperties(BindingFlags.DeclaredOnly | BindingFlags.Static | BindingFlags.Public))
                    {
                        if (colorPropertyInfo.GetValue(null).ToString() == str)
                        {
                            return colorPropertyInfo.Name;
                        }
                    }
#endif

                    return str;
                }
            }

            return value;
        }

        public object ConvertBack(object value, Type targetType, object parameter, string language)
        {
            try
            {
                if (value as string == "Null")
                    return null;

                Color c = (Color)XamlBindingHelper.ConvertValue(typeof(Color), value);
                SolidColorBrush scb = new SolidColorBrush(c);
                return scb;
            }
            catch (FormatException)
            {
                return value;
            }
        }
    }

    public class UniversalConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, string language)
        {
            if (value != null && targetType.Name == "String")
            {
                string str = string.Empty;

                if (value.GetType() == typeof(Point))
                {
                    str = ((Point)value).ToString();
                }
                else if (value.GetType() == typeof(Size))
                {
                    str = ((Size)value).ToString();
                }
                else if (value.GetType() == typeof(Thickness))
                {
                    str = ((Thickness)value).ToString();
                }
                else if (value.GetType() == typeof(DateTime))
                {
                    str = string.Format("{0:ddd dd MMMM yyyy}", (DateTime)value);
                }
                else if (value.GetType() == typeof(FontWeight))
                {
                    str = System.Convert.ToString(((FontWeight)value).Weight);
                }
                else if (value.GetType() == typeof(double))
                {
                    str = System.Convert.ToString((double)value);
                }
                else if (value.GetType() == typeof(float))
                {
                    str = System.Convert.ToString((float)value);
                }
                else if (value.GetType() == typeof(Int32))
                {
                    str = System.Convert.ToString((Int32)value);
                }
                else if (value.GetType() == typeof(System.Numerics.Vector2))
                {
                    str = System.Convert.ToString((System.Numerics.Vector2)value);
                }
                else if (value.GetType() == typeof(System.Numerics.Vector3))
                {
                    str = System.Convert.ToString((System.Numerics.Vector3)value);
                }
                else if (value is FontFamily)
                {
                    str = (value as FontFamily).Source;
                }

                return str;
            }

            return value;
        }

        public object ConvertBack(object value, Type targetType, object parameter, string language)
        {
            try
            {
                if (value != null)
                {
                    if (targetType == typeof(object) || targetType == typeof(DateTime))
                    {
                        try
                        {
                            DateTime dt = System.Convert.ToDateTime(value);
                            return dt;
                        }
                        catch
                        {
                        }
                    }

                    if (targetType == typeof(Point))
                    {
                        string valueAsStr = value as string;
                        if (!string.IsNullOrWhiteSpace(valueAsStr))
                        {
                            string[] lengths = valueAsStr.Split(',');
                            if (lengths.Length < 2)
                            {
                                double pt = System.Convert.ToDouble(lengths[0]);
                                return new Point(pt, pt);
                            }
                            else
                                return new Point(System.Convert.ToDouble(lengths[0]), System.Convert.ToDouble(lengths[1]));
                        }
                    }
                    else if (targetType == typeof(Size))
                    {
                        string valueAsStr = value as string;
                        if (!string.IsNullOrWhiteSpace(valueAsStr))
                        {
                            string[] lengths = valueAsStr.Split(',');
                            if (lengths.Length < 2)
                            {
                                double size = System.Convert.ToDouble(lengths[0]);
                                return new Size(size, size);
                            }
                            else
                                return new Size(System.Convert.ToDouble(lengths[0]), System.Convert.ToDouble(lengths[1]));
                        }
                    }
                    else if (targetType == typeof(Thickness))
                    {
                        string valueAsStr = value as string;
                        if (!string.IsNullOrWhiteSpace(valueAsStr))
                        {
                            string[] lengths = valueAsStr.Split(',');
                            if (lengths.Length < 4)
                                return new Thickness(System.Convert.ToDouble(lengths[0]));
                            else
                                return new Thickness(
                                    System.Convert.ToDouble(lengths[0]), System.Convert.ToDouble(lengths[1]), System.Convert.ToDouble(lengths[2]), System.Convert.ToDouble(lengths[3]));
                        }
                    }
                    else if (targetType == typeof(FontWeight))
                    {
                        string valueAsStr = value as string;
                        if (!string.IsNullOrWhiteSpace(valueAsStr))
                        {
                            ushort valueAsUshort = System.Convert.ToUInt16(valueAsStr);
                            FontWeight fontWeight = new FontWeight { Weight = valueAsUshort };
                            return fontWeight;
                        }
                    }
                    else if (targetType == typeof(double))
                    {
                        double d = System.Convert.ToDouble(value);
                        return d;
                    }
                    else if (targetType == typeof(float))
                    {
                        float f = System.Convert.ToSingle(value);
                        return f;
                    }
                    else if (targetType == typeof(Int32))
                    {
                        int i = System.Convert.ToInt32(value);
                        return i;
                    }
                    else if (targetType == typeof(System.Numerics.Vector2))
                    {
                        string valueAsStr = value as string;
                        if (!string.IsNullOrWhiteSpace(valueAsStr))
                        {
                            string[] lengths = valueAsStr.Split(',');
                            if (lengths.Length == 2)
                                return new System.Numerics.Vector2(
                                    System.Convert.ToSingle(lengths[0].TrimStart('<')),
                                    System.Convert.ToSingle(lengths[1].TrimEnd('>')));
                        }
                    }
                    else if (targetType == typeof(System.Numerics.Vector3))
                    {
                        string valueAsStr = value as string;
                        if (!string.IsNullOrWhiteSpace(valueAsStr))
                        {
                            string[] lengths = valueAsStr.Split(',');
                            if (lengths.Length == 3)
                                return new System.Numerics.Vector3(
                                    System.Convert.ToSingle(lengths[0].TrimStart('<')),
                                    System.Convert.ToSingle(lengths[1]),
                                    System.Convert.ToSingle(lengths[2].TrimEnd('>')));
                        }
                    }
                    else if (value is FontFamily)
                    {
                        string valueAsStr = value as string;
                        if (!string.IsNullOrWhiteSpace(valueAsStr))
                        {
                            FontFamily fontFamily = new FontFamily(valueAsStr);
                            return fontFamily;
                        }
                    }
                }

                return value;
            }
            catch (FormatException)
            {
                return value;
            }
        }
    }
}
