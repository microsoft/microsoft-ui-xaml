// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// Forward declarations for external headers.  Please use in header files instead of declaring manually.

#include <abi/xaml_abi.h>

XAML_ABI_NAMESPACE_BEGIN
namespace Windows {
namespace Graphics {
    struct DisplayAdapterId;
    struct PointInt32;
    struct RectInt32;
    struct SizeInt32;

namespace DirectX {
    enum DirectXAlphaMode : int;
    enum DirectXPixelFormat : int;
} // DirectX

namespace Display {
    class AdvancedColorInfo;
    class BrightnessOverride;
    class BrightnessOverrideSettings;
    class ColorOverrideSettings;
    class DisplayEnhancementOverride;
    class DisplayEnhancementOverrideCapabilities;
    class DisplayEnhancementOverrideCapabilitiesChangedEventArgs;
    class DisplayInformation;
    enum HdrMetadataFormat : int;
    enum ResolutionScale : int;
    interface IAdvancedColorInfo;
    interface IBrightnessOverride;
    interface IBrightnessOverrideSettings;
    interface IBrightnessOverrideSettingsStatics;
    interface IBrightnessOverrideStatics;
    interface IColorOverrideSettings;
    interface IColorOverrideSettingsStatics;
    interface IDisplayEnhancementOverride;
    interface IDisplayEnhancementOverrideCapabilities;
    interface IDisplayEnhancementOverrideCapabilitiesChangedEventArgs;
    interface IDisplayEnhancementOverrideStatics;
    interface IDisplayInformation;
    interface IDisplayInformation2;
    interface IDisplayInformation3;
    interface IDisplayInformation4;
    interface IDisplayInformation5;
    interface IDisplayInformationStatics;
    interface IDisplayPropertiesEventHandler;
    interface IDisplayPropertiesStatics;
    struct NitRange;
} // Display

namespace Effects {
    interface IGraphicsEffect;
    interface IGraphicsEffectD2D1Interop;
    interface IGraphicsEffectSource;
} // Effects

namespace Imaging {
    class BitmapBuffer;
    class BitmapCodecInformation;
    class BitmapDecoder;
    class BitmapEncoder;
    class BitmapFrame;
    class BitmapProperties;
    class BitmapPropertiesView;
    class BitmapPropertySet;
    class BitmapTransform;
    class BitmapTypedValue;
    class ImageStream;
    class PixelDataProvider;
    class SoftwareBitmap;
    enum BitmapAlphaMode : int;
    enum BitmapBufferAccessMode : int;
    enum BitmapFlip : int;
    enum BitmapInterpolationMode : int;
    enum BitmapPixelFormat : int;
    enum BitmapRotation : int;
    enum ColorManagementMode : int;
    enum ExifOrientationMode : int;
    enum JpegSubsamplingMode : int;
    enum PngFilterMode : int;
    enum TiffCompressionMode : int;
    interface IBitmapBuffer;
    interface IBitmapCodecInformation;
    interface IBitmapDecoder;
    interface IBitmapDecoderStatics;
    interface IBitmapDecoderStatics2;
    interface IBitmapEncoder;
    interface IBitmapEncoderStatics;
    interface IBitmapEncoderStatics2;
    interface IBitmapEncoderWithSoftwareBitmap;
    interface IBitmapFrame;
    interface IBitmapFrameWithSoftwareBitmap;
    interface IBitmapProperties;
    interface IBitmapPropertiesView;
    interface IBitmapTransform;
    interface IBitmapTypedValue;
    interface IBitmapTypedValueFactory;
    interface IPixelDataProvider;
    interface ISoftwareBitmap;
    interface ISoftwareBitmapFactory;
    interface ISoftwareBitmapStatics;
    struct BitmapBounds;
    struct BitmapPlaneDescription;
    struct BitmapSize;
} // Imaging

namespace Printing {
    class PrintManager;
    class PrintPageInfo;
    class PrintPageRange;
    class PrintPageRangeOptions;
    class PrintTask;
    class PrintTaskCompletedEventArgs;
    class PrintTaskOptions;
    class PrintTaskProgressingEventArgs;
    class PrintTaskRequest;
    class PrintTaskRequestedDeferral;
    class PrintTaskRequestedEventArgs;
    class PrintTaskSourceRequestedArgs;
    class PrintTaskSourceRequestedDeferral;
    interface IPrintDocumentSource;
    interface IPrintManager;
    interface IPrintManagerStatic;
    interface IPrintManagerStatic2;
    interface IPrintPageInfo;
    interface IPrintPageRange;
    interface IPrintPageRangeFactory;
    interface IPrintPageRangeOptions;
    interface IPrintTask;
    interface IPrintTask2;
    interface IPrintTaskCompletedEventArgs;
    interface IPrintTaskOptions;
    interface IPrintTaskOptions2;
    interface IPrintTaskOptionsCore;
    interface IPrintTaskOptionsCoreProperties;
    interface IPrintTaskOptionsCoreUIConfiguration;
    interface IPrintTaskProgressingEventArgs;
    interface IPrintTaskRequest;
    interface IPrintTaskRequestedDeferral;
    interface IPrintTaskRequestedEventArgs;
    interface IPrintTaskSourceRequestedArgs;
    interface IPrintTaskSourceRequestedDeferral;
    interface IPrintTaskSourceRequestedHandler;
    interface IPrintTaskTargetDeviceSupport;
    interface IStandardPrintTaskOptionsStatic;
    interface IStandardPrintTaskOptionsStatic2;
    interface IStandardPrintTaskOptionsStatic3;
    struct PrintPageDescription;
} // Printing

} // Graphics
} // Windows
XAML_ABI_NAMESPACE_END
