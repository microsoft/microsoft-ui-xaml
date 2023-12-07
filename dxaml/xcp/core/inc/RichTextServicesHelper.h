// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace RichTextServices
{
    class TextLineBreak;
    class TextRunProperties;
    class ElementReference;

    static const HRESULT INTERNAL_ERROR = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_XAML, -57L);
}

class CFontContext;

//---------------------------------------------------------------------------
//
//  RichTextServices Helper Class
//
//  Static helper functions to translate between RichTextServices
//  and Silverlight data structures.
//
//---------------------------------------------------------------------------
class RichTextServicesHelper
{
public:

    //------------------------------------------------------------------------
    //  Summary:
    //      Maps known RichTextServices::Result::Enum to the equivalent HRESULT.
    //
    //      Usage: IFC(MapTxErr(<RichTextServicesCall>));
    //
    //             Alternatively, use the predefined IFCTEXT macro,
    //             IFCTEXT(<RichTextServicesCall>);
    //------------------------------------------------------------------------
    static
    _Check_return_ HRESULT MapTxErr(
        _In_ RichTextServices::Result::Enum txhr
        );

    //------------------------------------------------------------------------
    //  Summary:
    //      Maps xcp FlowDirection to RichTextServices FlowDirection
    //------------------------------------------------------------------------
    static
    _Check_return_ RichTextServices::FlowDirection::Enum XCPFlowDirectionToRTS(
        _In_ DirectUI::FlowDirection nFlowDirection
    );

};
