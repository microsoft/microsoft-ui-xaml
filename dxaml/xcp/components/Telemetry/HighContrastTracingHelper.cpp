// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "uielement.h"
#include "CDependencyObject.h"
#include "TypeTableStructs.h"
#include "SolidColorBrush.h"
#include "XamlTraceLogging.h"
#include <dopointercast.h>
#include "Theme.h"
#include "HighContrastTracingHelper.h"

using namespace Theming;

static bool IsBackgroundProp(KnownPropertyIndex propIndex)
{
    return propIndex == KnownPropertyIndex::Control_Background ||
        propIndex == KnownPropertyIndex::Panel_Background;
}

static bool IsForegroundProp(KnownPropertyIndex propIndex)
{
    return propIndex == KnownPropertyIndex::Control_Foreground;
}

static bool HasBackgroundError(Theme theme, XUINT32 rgbColor)
{
    bool res = false;
    switch (theme)
    {
    case Theme::HighContrastWhite:
        res = (rgbColor & 0xFF000000) != 0 && rgbColor != (XUINT32)0xFFFFFFFF;
        break;
    case Theme::HighContrastBlack:
        res = (rgbColor & 0xFF000000) != 0 && rgbColor != (XUINT32)0xFF000000;
        break;
    default:
        break;
    }

    return res;
}

static bool HasForegroundError(Theme theme, XUINT32 rgbColor)
{
    bool res = false;
    switch (theme)
    {
    case Theme::HighContrastWhite:
        res = (rgbColor & 0xFF000000) != 0 && rgbColor != (XUINT32)0xFF000000 &&
            rgbColor != (XUINT32)0xFF00009F && rgbColor != (XUINT32)0xFF600000 &&
            rgbColor != (XUINT32)0xFFFFFFFF;
        break;
    case Theme::HighContrastBlack:
        res = (rgbColor & 0xFF000000) != 0 && rgbColor != (XUINT32)0xFFFFFFFF &&
            rgbColor != (XUINT32)0xFF3FF23F && rgbColor != (XUINT32)0xFFFFFF00 &&
            rgbColor != (XUINT32)0xFF000000;
        break;
    default:
        break;
    }

    return res;
}

void TraceLoggingHCColor(_In_ CUIElement *element, _In_ Theme theme, _In_ const SetValueParams& args)
{
    bool isBackground = IsBackgroundProp(args.m_pDP->GetIndex());
    bool isForeground = IsForegroundProp(args.m_pDP->GetIndex());

    if (element->ParserOwnsParent() && (isBackground || isForeground) && element->IsVisible() &&
        args.m_value.GetType() == ValueType::valueObject)
    {
        const CSolidColorBrush* pBrush = do_pointer_cast<CSolidColorBrush>(args.m_value);
        XUINT32 rgbColor = pBrush ? pBrush->m_rgb : 0;
        bool hasBackgroundError = isBackground && HasBackgroundError(theme, rgbColor);
        bool hasForegroundError = isForeground && HasForegroundError(theme, rgbColor);

        if (hasBackgroundError || hasForegroundError)
        {
            TraceLoggingWrite(g_hTraceProvider,
                "XAMLHCColor",
                TraceLoggingLevel(WINEVENT_LEVEL_LOG_ALWAYS),
                TraceLoggingValue((int)theme, "Theme"),
                TraceLoggingWideString(element->GetClassName().GetBuffer(), "ClassName"),
                TraceLoggingBool(isBackground, "IsBackground"),
                TraceLoggingValue(rgbColor, "rgbValue")
                );
        }
    }
}