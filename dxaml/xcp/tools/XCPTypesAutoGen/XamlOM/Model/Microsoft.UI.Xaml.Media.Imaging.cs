// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using XamlOM;

namespace Microsoft.UI.Xaml.Media.Imaging
{
    [CodeGen(partial: true)]
    [NativeName("CBitmapSource")]
    [Guids(ClassGuid = "7993a96a-3e53-4e8f-9e0d-2913a5ef38fe")]
    public abstract class BitmapSource
     : Microsoft.UI.Xaml.Media.ImageSource
    {

        [PropertyKind(PropertyKind.DependencyPropertyOnly)]
        [NativeMethod("CBitmapSource", "PixelWidth")]
        [NativeStorageType(ValueType.valueSigned)]
        [ReadOnly]
        public Windows.Foundation.Int32 PixelWidth
        {
            get;
            private set;
        }

        [PropertyKind(PropertyKind.DependencyPropertyOnly)]
        [NativeMethod("CBitmapSource", "PixelHeight")]
        [NativeStorageType(ValueType.valueSigned)]
        [ReadOnly]
        public Windows.Foundation.Int32 PixelHeight
        {
            get;
            private set;
        }

        protected BitmapSource() { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public void SetSource(Windows.Storage.Streams.IRandomAccessStream streamSource)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public Windows.Foundation.IAsyncAction SetSourceAsync(Windows.Storage.Streams.IRandomAccessStream streamSource)
        {
            return default(Windows.Foundation.IAsyncAction);
        }
    }

    [CodeGen(partial: true)]
    [NativeName("CWriteableBitmap")]
    [Guids(ClassGuid = "1a238ad6-83f6-4882-b4b0-c0892d263181")]
    public sealed class WriteableBitmap
     : Microsoft.UI.Xaml.Media.Imaging.BitmapSource
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public Windows.Storage.Streams.IBuffer PixelBuffer
        {
            get;
            private set;
        }

        internal WriteableBitmap() { }

        [FactoryMethodName("CreateInstanceWithDimensions")]
        public WriteableBitmap(Windows.Foundation.Int32 pixelWidth, Windows.Foundation.Int32 pixelHeight) { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void Invalidate()
        {
        }
    }

    [CodeGen(partial: true)]
    [PartialFactory]
    [NativeName("CSurfaceImageSource")]
    [Guids(ClassGuid = "e37c8104-2001-42ac-8996-45febec92b9e")]
    public class SurfaceImageSource
     : Microsoft.UI.Xaml.Media.ImageSource
    {
        internal SurfaceImageSource() { }

        [FactoryMethodName("CreateInstanceWithDimensions")]
        public SurfaceImageSource(Windows.Foundation.Int32 pixelWidth, Windows.Foundation.Int32 pixelHeight) { }

        [FactoryMethodName("CreateInstanceWithDimensionsAndOpacity")]
        public SurfaceImageSource(Windows.Foundation.Int32 pixelWidth, Windows.Foundation.Int32 pixelHeight, Windows.Foundation.Boolean isOpaque) { }
    }

    [CodeGen(partial: true)]
    [NativeName("CVirtualSurfaceImageSource")]
    [Guids(ClassGuid = "5b02b1f2-fd33-441f-90f7-db0a76abccf9")]
    public sealed class VirtualSurfaceImageSource
     : Microsoft.UI.Xaml.Media.Imaging.SurfaceImageSource
    {
        internal VirtualSurfaceImageSource() { }

        [FactoryMethodName("CreateInstanceWithDimensions")]
        public VirtualSurfaceImageSource(Windows.Foundation.Int32 pixelWidth, Windows.Foundation.Int32 pixelHeight)
         : base(pixelWidth, pixelHeight) { }

        [FactoryMethodName("CreateInstanceWithDimensionsAndOpacity")]
        public VirtualSurfaceImageSource(Windows.Foundation.Int32 pixelWidth, Windows.Foundation.Int32 pixelHeight, Windows.Foundation.Boolean isOpaque)
         : base(pixelWidth, pixelHeight, isOpaque) { }
    }

    [CodeGen(partial: true)]
    [NativeName("CRenderTargetBitmap")]
    [Guids(ClassGuid = "b3432d35-9853-4d4d-b19a-e8acd3c46693")]
    public sealed class RenderTargetBitmap
     : Microsoft.UI.Xaml.Media.ImageSource
    {

        [PropertyKind(PropertyKind.DependencyPropertyOnly)]
        [NativeMethod("CRenderTargetBitmap", "PixelWidth")]
        [NativeStorageType(ValueType.valueSigned)]
        [ReadOnly]
        public Windows.Foundation.Int32 PixelWidth
        {
            get;
            private set;
        }

        [PropertyKind(PropertyKind.DependencyPropertyOnly)]
        [NativeMethod("CRenderTargetBitmap", "PixelHeight")]
        [NativeStorageType(ValueType.valueSigned)]
        [ReadOnly]
        public Windows.Foundation.Int32 PixelHeight
        {
            get;
            private set;
        }

        public RenderTargetBitmap() { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [DXamlOverloadName("RenderAsync")]
        public Windows.Foundation.IAsyncAction RenderAsync([Optional] Microsoft.UI.Xaml.UIElement element)
        {
            return default(Windows.Foundation.IAsyncAction);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [DXamlOverloadName("RenderAsync")]
        public Windows.Foundation.IAsyncAction RenderToSizeAsync(
            [Optional] Microsoft.UI.Xaml.UIElement element,
            Windows.Foundation.Int32 scaledWidth,
            Windows.Foundation.Int32 scaledHeight)
        {
            return default(Windows.Foundation.IAsyncAction);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public Windows.Foundation.IAsyncOperation<Windows.Storage.Streams.IBuffer> GetPixelsAsync()
        {
            return default(Windows.Foundation.IAsyncOperation<Windows.Storage.Streams.IBuffer>);
        }
    }

    [Platform(typeof(PrivateApiContract), 1)]
    public interface IXamlRenderingBackgroundTaskStaticsPrivate
    {
        [AllowCrossThreadAccess]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void EnableStandaloneHosting();

        [AllowCrossThreadAccess]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        Microsoft.UI.Xaml.ResourceDictionary Resources
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void SetScalePercentage(uint percentage);
    }

    [DXamlIdlGroup("coretypes2")]
    [CodeGen(partial: true)]
    [FrameworkTypePattern]
    [TypeTable(IsExcludedFromCore = true)]
    [Implements(typeof(Microsoft.UI.Xaml.Media.Imaging.IXamlRenderingBackgroundTaskStaticsPrivate), IsStaticInterface = true)]
    [ThreadingModel(ThreadingModel.Both)]
    [Guids(ClassGuid = "bb0cf59e-1ed3-4815-827e-af00c0185b1b")]
    public abstract class XamlRenderingBackgroundTask
     : Windows.Foundation.Object
    {
        public XamlRenderingBackgroundTask() { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        protected virtual void OnRun(Windows.ApplicationModel.Background.IBackgroundTaskInstance taskInstance)
        {
        }
    }

    [CodeGen(partial: true)]
    [Guids(ClassGuid = "f9c9e87b-9ff5-47b3-b83d-f4248628a819")]
    [NativeName("CBitmapImage")]
    public sealed class BitmapImage
     : Microsoft.UI.Xaml.Media.Imaging.BitmapSource
    {
        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_nCreateOptions")]
        public Microsoft.UI.Xaml.Media.Imaging.BitmapCreateOptions CreateOptions
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueString)]
        [OffsetFieldName("m_strSource")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Uri UriSource
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueSigned)]
        [OffsetFieldName("m_decodePixelWidth")]
        public Windows.Foundation.Int32 DecodePixelWidth
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueSigned)]
        [OffsetFieldName("m_decodePixelHeight")]
        public Windows.Foundation.Int32 DecodePixelHeight
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_decodePixelType")]
        public Microsoft.UI.Xaml.Media.Imaging.DecodePixelType DecodePixelType
        {
            get;
            set;
        }

        [ReadOnly]
        public bool IsAnimatedBitmap
        {
            get;
            internal set;
        }

        [ReadOnly]
        public bool IsPlaying
        {
            get;
            internal set;
        }

        public bool AutoPlay
        {
            get;
            set;
        }

        public void Play()
        {
        }

        public void Stop()
        {
        }

        [EventFlags(UseEventManager = true)]
        public event Microsoft.UI.Xaml.Media.Imaging.DownloadProgressEventHandler DownloadProgress;

        [EventFlags(UseEventManager = true)]
        public event Microsoft.UI.Xaml.RoutedEventHandler ImageOpened;

        [EventFlags(UseEventManager = true)]
        public event Microsoft.UI.Xaml.ExceptionRoutedEventHandler ImageFailed;

        public BitmapImage() { }

        [FactoryMethodName("CreateInstanceWithUriSource")]
        public BitmapImage(Windows.Foundation.Uri uriSource) { }
    }


    [CodeGen(partial: true)]
    [Guids(ClassGuid = "ac4f05ec-fcf4-4f11-8b65-60bd35b59e5f")]
    [NativeName("CSvgImageSource")]
    [DXamlIdlGroup("coretypes2")]
    public class SvgImageSource
     : Microsoft.UI.Xaml.Media.ImageSource
    {
        [NativeStorageType(ValueType.valueString)]
        [OffsetFieldName("m_strSource")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Uri UriSource
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_rasterizeWidth")]
        public Windows.Foundation.Double RasterizePixelWidth
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_rasterizeHeight")]
        public Windows.Foundation.Double RasterizePixelHeight
        {
            get;
            set;
        }

        [EventFlags(UseEventManager = true)]
        [EventHandlerType(EventHandlerKind.TypedSenderAndArgs)]
        public event Windows.Foundation.TypedEventHandler<SvgImageSource, SvgImageSourceOpenedEventArgs> Opened;

        [EventFlags(UseEventManager = true)]
        public event Windows.Foundation.TypedEventHandler<SvgImageSource, SvgImageSourceFailedEventArgs> OpenFailed;

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public Windows.Foundation.IAsyncOperation<Microsoft.UI.Xaml.Media.Imaging.SvgImageSourceLoadStatus> SetSourceAsync(Windows.Storage.Streams.IRandomAccessStream streamSource)
        {
            return default(Windows.Foundation.IAsyncOperation<Microsoft.UI.Xaml.Media.Imaging.SvgImageSourceLoadStatus>);
        }

        public SvgImageSource() { }

        [FactoryMethodName("CreateInstanceWithUriSource")]
        public SvgImageSource(Windows.Foundation.Uri uriSource) { }
    }

    [CodeGen(partial: true)]
    [NativeName("CSoftwareBitmapSource")]
    [DXamlIdlGroup("coretypes2")]
    [Implements(typeof(Windows.Foundation.IClosable))]
    [Guids(ClassGuid = "3454c48e-a9fc-4b16-af63-c7068b6990c9")]
    public sealed class SoftwareBitmapSource
     : Microsoft.UI.Xaml.Media.ImageSource
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public Windows.Foundation.IAsyncAction SetBitmapAsync([Optional] Windows.Graphics.Imaging.SoftwareBitmap softwareBitmap)
        {
            return default(Windows.Foundation.IAsyncAction);
        }
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CDownloadProgressEventArgs")]
    [Guids(ClassGuid = "228d9bdb-0b18-4a7a-8f7a-8826629ad8fb")]
    public sealed class DownloadProgressEventArgs
     : Microsoft.UI.Xaml.EventArgs
    {
        [CoreType(typeof(Windows.Foundation.Double))]
        [PropertyKind(PropertyKind.DependencyPropertyOnly)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_downloadProgress")]
        [DelegateToCore]
        public Windows.Foundation.Int32 Progress
        {
            get;
            set;
        }

        internal DownloadProgressEventArgs() { }
    }

    [NativeName("BitmapCreateOptions")]
    [EnumFlags(AreValuesFlags = true, HasTypeConverter = true)]
    [NativeComment("Contains flags for controlling bitmap creation options. This is a bit field enumeration.")]
    public enum BitmapCreateOptions
    {
        [NativeValueName("bcNone")]
        None = 0,
        [NativeValueName("bcIgnoreImageCache")]
        IgnoreImageCache = 8,
    }

    [NativeName("DecodePixelType")]
    [EnumFlags(HasTypeConverter = true)]
    public enum DecodePixelType
    {
        [NativeValueName("DecodePixelType_Physical")]
        Physical = 0,
        [NativeValueName("DecodePixelType_Logical")]
        Logical = 1,
    }

    [NativeName("SvgImageSourceLoadStatus")]
    [EnumFlags(HasTypeConverter = true)]
    [NativeComment("This enum indicates whether a SVG image is loaded correctly ")]
    [DXamlIdlGroup("coretypes2")]
    public enum SvgImageSourceLoadStatus
    {
        [NativeValueName("SvgImageSourceLoadStatusSuccess")]
        Success,
        [NativeValueName("SvgImageSourceLoadStatusNetworkError")]
        NetworkError,
        [NativeValueName("SvgImageSourceLoadStatusInvalidFormat")]
        InvalidFormat,
        [NativeValueName("SvgImageSourceLoadStatusOther")]
        Other,
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CSvgImageSourceOpenedEventArgs")]
    [DXamlIdlGroup("coretypes2")]
    [Guids(ClassGuid = "c89fdb2d-7cd5-46c1-abbf-f989ab9f1ba1")]
    public sealed class SvgImageSourceOpenedEventArgs
     : Microsoft.UI.Xaml.EventArgs
    {
        internal SvgImageSourceOpenedEventArgs() { }
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CSvgImageSourceFailedEventArgs")]
    [DXamlIdlGroup("coretypes2")]
    [Guids(ClassGuid = "4eb20bec-e6c4-41c6-b301-6a29da9092f8")]
    public sealed class SvgImageSourceFailedEventArgs
     : Microsoft.UI.Xaml.EventArgs
    {
        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_status")]
        [DelegateToCore]
        public Microsoft.UI.Xaml.Media.Imaging.SvgImageSourceLoadStatus Status
        {
            get;
            internal set;
        }
        internal SvgImageSourceFailedEventArgs() { }
    }

    public delegate void DownloadProgressEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.Media.Imaging.DownloadProgressEventArgs e);
}
