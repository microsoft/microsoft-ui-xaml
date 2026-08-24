// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Microsoft.UI.Xaml;

namespace BindTestbedModel
{
    public sealed class NullablePropertiesButton : Microsoft.UI.Xaml.Controls.Button
    {

        public double? NullableDoubleDP
        {
            get { return (double?)GetValue(NullableDoubleDPProperty); }
            set
            {
                SetValue(NullableDoubleDPProperty, value);
                UpdateContent();
            }
        }
        #region NullableDoubleDP DP
        private const string NullableDoubleDPName = "NullableDoublePropertyDP";
        private static readonly DependencyProperty _NullableDoubleDPProperty =
            DependencyProperty.Register(NullableDoubleDPName, typeof(double?), typeof(DataModel), new PropertyMetadata((double?)(0.0)));
        public static DependencyProperty NullableDoubleDPProperty { get { return _NullableDoubleDPProperty; } }
        #endregion

        private bool? _nullableBool;
        public bool? NullableBool
        {
            get
            {
                return _nullableBool;
            }
            set
            {
                _nullableBool = value;
                UpdateContent();
            }
        }

        private TShirtSize? _nullableEnum;
        public TShirtSize? NullableEnum
        {
            get
            {
                return _nullableEnum;
            }
            set
            {
                _nullableEnum = value;
                UpdateContent();
            }
        }

        private Windows.UI.Color? _nullableColor;
        public Windows.UI.Color? NullableColor
        {
            get
            {
                return _nullableColor;
            }
            set
            {
                _nullableColor = value;
                UpdateContent();
            }
        }

        public void UpdateContent()
        {
            Content = $"NullableDouble: {NullableDoubleDP.ToString()}, NullableBool: {NullableBool.ToString()}, NullableEnum: {NullableEnum.ToString()}, NullableColor: {NullableColor.ToString()}";
        }
    }
}
