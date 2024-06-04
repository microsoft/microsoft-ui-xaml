// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace RichTextServices
{
    class TextDrawingContext;
    class TextSource;

    //---------------------------------------------------------------------------
    //
    //  ObjectRun
    //
    //  ObjectRun is a sublass of TextRun that indicates an embedded
    //  UIElement and exposes APIs to access element metrics.
    //
    //---------------------------------------------------------------------------
    class ObjectRun : public TextRun
    {
    public:

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      ObjectRun::HasFixedSize
        //
        //  Synopsis:
        //      Flag indicates whether text object has fixed size regardless of where
        //      it is placed within a line.
        //
        //---------------------------------------------------------------------------
        virtual bool HasFixedSize() const = 0;

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      ObjectRun::Format
        //
        //  Synopsis:
        //      Gets text object measurement metrics that will fit within the specified
        //      remaining width of the paragraph.
        //
        //---------------------------------------------------------------------------
        virtual
        Result::Enum Format(
            _In_ TextSource *pTextSource,
                // The text source for the content.
            _In_ XFLOAT paragraphWidth,
                // Remaining paragraph width.
            _In_ const XPOINTF &currentPosition,
                // Current pen position.
            _Out_ ObjectRunMetrics *pMetrics
                // Pointer to metrics, to be filled in by this method.
            ) = 0;

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      ObjectRun::ComputeBoundingBox
        //
        //  Synopsis:
        //      Computes bounding box of text object.
        //
        //---------------------------------------------------------------------------
        virtual
        Result::Enum ComputeBoundingBox(
            _In_ bool rightToLeft,
                // True if the object is drawn from right to left.
            _In_ bool sideways,
                // True if the object is drawn on its side, i.e. side parallel to line's baseline.
            _Out_ XRECTF *pBounds
                // Pointer to bounding rectangle, to be filled in by this method.
            ) = 0;

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      ObjectRun::Arrange
        //
        //  Synopsis:
        //      Arranges the embedded object within its host container.
        //
        //---------------------------------------------------------------------------
        virtual
        Result::Enum Arrange(
            _In_ const XPOINTF &position
                // A point value that represents the position of the object within its host container.
            ) = 0;

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      ObjectRun::GetProperties
        //
        //  Synopsis:
        //      Returns text run properties associated with this object.
        //
        //---------------------------------------------------------------------------
        TextRunProperties *GetProperties() const;

    protected:

        // Initializes a new instance of ObjectRun class.
        ObjectRun(_In_ XUINT32 characterIndex, _In_ TextRunProperties *pProperties);

        // Release resources such as m_pProperties.
        ~ObjectRun() override;

    private:

        TextRunProperties *m_pProperties;
    };
}
