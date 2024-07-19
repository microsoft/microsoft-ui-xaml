// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using Microsoft.UI.Xaml.Markup;
using XamlOM;

namespace Microsoft.UI.Xaml.Controls
{
    [CodeGen(partial: true)]
    [DXamlIdlGroup("Controls2")]
    [Platform(typeof(Microsoft.UI.Xaml.WinUIContract), 1)]
    [Guids(ClassGuid = "88ff135c-c572-4613-81af-714261917d2b")]
    public abstract class IconSource : Microsoft.UI.Xaml.DependencyObject
    {
        internal IconSource() { }
        
        public Microsoft.UI.Xaml.Media.Brush Foreground { get; set; }
        
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public virtual Microsoft.UI.Xaml.Controls.IconElement CreateIconElement()
        {
            return default(Microsoft.UI.Xaml.Controls.IconElement);
        }
        
        protected virtual Microsoft.UI.Xaml.DependencyProperty GetIconElementPropertyCore(Microsoft.UI.Xaml.DependencyProperty iconSourceProperty)
        {
            return default(Microsoft.UI.Xaml.DependencyProperty);
        }
    }
    
    [CodeGen(partial: true)]
    [DXamlIdlGroup("Controls2")]
    [Platform(typeof(Microsoft.UI.Xaml.WinUIContract), 1)]
    [Velocity(ClassGuid = "2976441c-7c3e-4470-ab67-ed8eac236096")]
    public class SymbolIconSource : IconSource
    {
        public SymbolIconSource() { }
        
        public Microsoft.UI.Xaml.Controls.Symbol Symbol { get; set; }
    }
    
    [CodeGen(partial: true)]
    [DXamlIdlGroup("Controls2")]
    [Platform(typeof(Microsoft.UI.Xaml.WinUIContract), 1)]
    [Velocity(ClassGuid = "ad011974-3694-42b5-93a4-a83b5c73120c")]
    public class FontIconSource : IconSource
    {
        public FontIconSource() { }
        
        public Windows.Foundation.String Glyph { get; set; }
        
        public Windows.Foundation.Double FontSize { get; set; }
        
        public Microsoft.UI.Xaml.Media.FontFamily FontFamily { get; set; }
        
        public Windows.UI.Text.FontWeight FontWeight { get; set; }
        
        public Windows.UI.Text.FontStyle FontStyle { get; set; }
        
        public Windows.Foundation.Boolean IsTextScaleFactorEnabled { get; set; }
        
        public Windows.Foundation.Boolean MirroredWhenRightToLeft { get; set; }
    }
    
    [CodeGen(partial: true)]
    [DXamlIdlGroup("Controls2")]
    [Platform(typeof(Microsoft.UI.Xaml.WinUIContract), 1)]
    [Velocity(ClassGuid = "ce64e57d-7cec-4f6c-b812-6d74385d29dc")]
    public class BitmapIconSource : IconSource
    {
        public BitmapIconSource() { }
        
        public Windows.Foundation.Uri UriSource { get; set; }
        
        public Windows.Foundation.Boolean ShowAsMonochrome { get; set; }
    }
    
    [CodeGen(partial: true)]
    [DXamlIdlGroup("Controls2")]
    [Platform(typeof(Microsoft.UI.Xaml.WinUIContract), 1)]
    [Velocity(ClassGuid = "c40d852e-f972-42a5-83b8-cf8417f6ec1c")]
    public class PathIconSource : IconSource
    {
        public PathIconSource() { }
        
        public Microsoft.UI.Xaml.Media.Geometry Data { get; set; }
    }

    [DXamlIdlGroup("Controls2")]
    [ContentProperty("IconSource")]
    [NativeName("CIconSourceElement")]
    [Platform(typeof(Microsoft.UI.Xaml.WinUIContract), 1)]
    [Guids(ClassGuid = "a4a27b61-eb74-4d59-88dd-4898820884a0")]
    public class IconSourceElement : Microsoft.UI.Xaml.Controls.IconElement
    {
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_iconSource")]
        public Microsoft.UI.Xaml.Controls.IconSource IconSource
        {
            get;
            set;
        }

        public IconSourceElement() { }
    }
}
