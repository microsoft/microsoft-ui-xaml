// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ModernCollectionBasePanel.g.h"

#include <numeric>
#include <iterator>

using namespace DirectUI;

_Check_return_ HRESULT ModernCollectionBasePanel::get_AreHorizontalSnapPointsRegularImpl(_Out_ BOOLEAN* pValue)
{
    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES

    IFCPTR(pValue);
    *pValue = m_snapPointState.AreSnapPointsRegular(xaml_controls::Orientation_Horizontal);

Cleanup:
    RRETURN(S_OK);
}

_Check_return_ HRESULT ModernCollectionBasePanel::get_AreVerticalSnapPointsRegularImpl(_Out_ BOOLEAN* pValue)
{
    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES

    IFCPTR(pValue);
    *pValue = m_snapPointState.AreSnapPointsRegular(xaml_controls::Orientation_Vertical);

Cleanup:
    RRETURN(S_OK);
}

_Check_return_ HRESULT ModernCollectionBasePanel::GetIrregularSnapPointsImpl(
    _In_ xaml_controls::Orientation orientation,
    _In_ xaml_primitives::SnapPointsAlignment alignment,
    _Outptr_ wfc::IVectorView<FLOAT>** returnValue)
{
    HRESULT hr = S_OK;

    IFCPTR(returnValue);
    IFC(m_snapPointState.GetIrregularSnapPoints(orientation, alignment, returnValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ModernCollectionBasePanel::GetRegularSnapPointsImpl(
    _In_ xaml_controls::Orientation orientation,
    _In_ xaml_primitives::SnapPointsAlignment alignment,
    _Out_ FLOAT* pOffset,
    _Out_ FLOAT* pSpacing)
{
    HRESULT hr = S_OK;

    IFCPTR(pOffset);
    IFCPTR(pSpacing);
    IFC(m_snapPointState.GetRegularSnapPoints(orientation, alignment, pOffset, pSpacing));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ModernCollectionBasePanel::UpdateSnapPoints()
{
    HRESULT hr = S_OK;

    xaml_controls::Orientation orientation;
    BOOLEAN hasRegularSnapPoints = FALSE;
    FLOAT nearOffset = 0.0f;
    FLOAT farOffset = 0.0f;
    FLOAT regularSpacing = 0.0f;
    BOOLEAN snapPointsChanged = FALSE;

    IFC(m_spLayoutStrategy->GetVirtualizationDirection(&orientation));

    IFC(m_spLayoutStrategy->GetRegularSnapPoints(&nearOffset, &farOffset, &regularSpacing, &hasRegularSnapPoints));

    if (hasRegularSnapPoints)
    {
        IFC(m_snapPointState.SetRegularSnapPoints(orientation, regularSpacing, nearOffset, farOffset, &snapPointsChanged));
    }
    else
    {
        // Time to loop and tally up the irregular snap points
        std::array<std::vector<FLOAT>, ElementType_Count> rawPoints;
        std::vector<FLOAT> mergedPoints;

        // Use transform to mutate the Rects into FLOATs
        std::transform(begin(m_arrangeRects), end(m_arrangeRects), begin(rawPoints),
            [orientation] (const RectVector& original)
        {
            std::vector<FLOAT> result;
            result.reserve(original.size() + 1);

            // Copy over the element of interest, skipping duplicates
            std::for_each(begin(original), end(original),
                [&result, orientation] (const wf::Rect& rect)
            {
                FLOAT point = orientation == xaml_controls::Orientation_Horizontal ? rect.X : rect.Y;
                if (result.empty() || !DoubleUtil::AreClose(result.back(), point))
                {
                    result.push_back(point);
                }
            });

            // Grab the far edge of the last one
            if (!original.empty())
            {
                if (orientation == xaml_controls::Orientation_Horizontal)
                {
                    if (!DoubleUtil::AreClose(0, original.back().Width))
                    {
                        result.push_back(original.back().X + original.back().Width);
                    }
                }
                else
                {
                    if (!DoubleUtil::AreClose(0, original.back().Height))
                    {
                        result.push_back(original.back().Y + original.back().Height);
                    }
                }
            }

            return result;
        });


        // Since we're getting the starting points plus the end of the last element, we will have an end point for both headers and containers
        // We only want one, so trim off the extra
        if (!rawPoints[xaml_controls::ElementType_GroupHeader].empty() && !rawPoints[xaml_controls::ElementType_ItemContainer].empty())
        {
            if (rawPoints[xaml_controls::ElementType_GroupHeader].back() <= rawPoints[xaml_controls::ElementType_ItemContainer].back())
            {
                rawPoints[xaml_controls::ElementType_GroupHeader].pop_back();
            }
            else
            {
                rawPoints[xaml_controls::ElementType_ItemContainer].pop_back();
            }
        }

        // Merge the two
        mergedPoints.reserve(rawPoints[xaml_controls::ElementType_GroupHeader].size() + rawPoints[xaml_controls::ElementType_ItemContainer].size());
        std::merge(begin(rawPoints[xaml_controls::ElementType_GroupHeader]), end(rawPoints[xaml_controls::ElementType_GroupHeader]),
            begin(rawPoints[xaml_controls::ElementType_ItemContainer]), end(rawPoints[xaml_controls::ElementType_ItemContainer]), back_inserter(mergedPoints));

        IFC(m_snapPointState.SetIrregularSnapPoints(orientation, std::move(mergedPoints), &snapPointsChanged));
    }

    // If the snap points changed, notify listeners
    if (snapPointsChanged)
    {
        if (orientation == xaml_controls::Orientation_Horizontal)
        {
            HorizontalSnapPointsChangedEventSourceType* pEventSource = nullptr;
            ctl::ComPtr<EventArgs> spArgs;

            // Create the args
            IFC(ctl::make<EventArgs>(&spArgs));

            // Raise the event
            IFC(GetHorizontalSnapPointsChangedEventSourceNoRef(&pEventSource));
            IFC(pEventSource->Raise(ctl::iinspectable_cast(this), ctl::as_iinspectable(spArgs.Get())));
        }
        else
        {
            VerticalSnapPointsChangedEventSourceType* pEventSource = nullptr;
            ctl::ComPtr<EventArgs> spArgs;

            // Create the args
            IFC(ctl::make<EventArgs>(&spArgs));

            // Raise the event
            IFC(GetVerticalSnapPointsChangedEventSourceNoRef(&pEventSource));
            IFC(pEventSource->Raise(ctl::iinspectable_cast(this), ctl::as_iinspectable(spArgs.Get())));
        }
    }

Cleanup:
    RRETURN(hr);
}

ModernCollectionBasePanel::SnapPointState::SnapPointState()
    : m_snapPointsSet(FALSE)
    , m_isRegular(FALSE)
    , m_orientation(xaml_controls::Orientation_Horizontal)
    , m_irregularSnapPointKeys()
    , m_regularSpacing(0.0f)
    , m_nearOffset(0.0f)
    , m_farOffset(0.0f)
{
}

_Check_return_ HRESULT ModernCollectionBasePanel::SnapPointState::SetIrregularSnapPoints(
    _In_ xaml_controls::Orientation orientation,
    _Inout_ std::vector<FLOAT>&& keys,
    _Out_ BOOLEAN* pChanged)
{
    HRESULT hr = S_OK;

    *pChanged = !m_snapPointsSet || m_isRegular ||
        (m_orientation != orientation) || (m_irregularSnapPointKeys != keys);

    m_snapPointsSet = TRUE;
    m_orientation = orientation;
    m_isRegular = FALSE;

    m_irregularSnapPointKeys = std::move(keys);

    RRETURN(hr);//RRETURN_REMOVAL
}

_Check_return_ HRESULT ModernCollectionBasePanel::SnapPointState::GetIrregularSnapPoints(
    _In_ xaml_controls::Orientation orientation,
    _In_ xaml_primitives::SnapPointsAlignment alignment,
    _Outptr_ wfc::IVectorView<FLOAT>** returnValue) const
{
    HRESULT hr = S_OK;

    *returnValue = nullptr;

    if (orientation == m_orientation)
    {
        ctl::ComPtr<ValueTypeView<FLOAT>> spSnapPointsVTV;

        if (m_isRegular)
        {
            IFC(E_FAIL);
        }

        // We're assuming that we have both a beginning and end snap point to handle both alignments
        // Any particular alignment request will only return one of the endpoints
        if (m_irregularSnapPointKeys.size() >= 2)
        {
            IFC(ctl::make(&spSnapPointsVTV));

            switch (alignment)
            {
                case xaml_primitives::SnapPointsAlignment_Near:
                    // Grab all the front edges
                    IFC(spSnapPointsVTV->SetView(
                        &(m_irregularSnapPointKeys[0]),
                        static_cast<UINT>(m_irregularSnapPointKeys.size() - 1) ));
                    break;

                case xaml_primitives::SnapPointsAlignment_Center:
                    {
                        // Need to grab the middle of each piece
                        std::vector<FLOAT> tempResult;
                        tempResult.reserve(m_irregularSnapPointKeys.size());

                        // adjacent_difference by default fills the output with the first element, followed
                        // by the difference of each of the adjacent elements. We're going to override
                        // the operation with a lambda to fill with the average of each pair
                        std::adjacent_difference(
                            begin(m_irregularSnapPointKeys), end(m_irregularSnapPointKeys), std::back_inserter(tempResult),
                            [](FLOAT current, FLOAT next) {
                                return (current + next) / 2;
                        });

                        // Hooray! Now we just copy this into the view, skipping the first item because it's just
                        // a copy of the first point
                        IFC(spSnapPointsVTV->SetView(
                            &(tempResult[1]),
                            static_cast<UINT>(tempResult.size() - 1)));
                    }
                    break;

                case xaml_primitives::SnapPointsAlignment_Far:
                    // Grab all the back edges
                    IFC(spSnapPointsVTV->SetView(
                        &(m_irregularSnapPointKeys[1]),
                        static_cast<UINT>(m_irregularSnapPointKeys.size() - 1) ));
                    break;
            }
        }

        *returnValue = spSnapPointsVTV.Detach();
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ModernCollectionBasePanel::SnapPointState::SetRegularSnapPoints(
    _In_ xaml_controls::Orientation orientation,
    _In_ FLOAT spacing,
    _In_ FLOAT nearOffset,
    _In_ FLOAT farOffset,
    _Out_ BOOLEAN* pChanged)
{
    HRESULT hr = S_OK;

    *pChanged = !m_snapPointsSet || !m_isRegular || (m_orientation != orientation) ||
        (m_regularSpacing != spacing) || (m_nearOffset != nearOffset) || (m_farOffset != farOffset);
    m_snapPointsSet = TRUE;
    m_regularSpacing = spacing;
    m_nearOffset = nearOffset;
    m_farOffset = farOffset;
    m_isRegular = TRUE;
    m_orientation = orientation;

    RRETURN(hr);
}

_Check_return_ HRESULT ModernCollectionBasePanel::SnapPointState::GetRegularSnapPoints(
    _In_ xaml_controls::Orientation orientation,
    _In_ xaml_primitives::SnapPointsAlignment alignment,
    _Out_ FLOAT* pOffset,
    _Out_ FLOAT* pSpacing) const
{
    HRESULT hr = S_OK;

    *pOffset = 0.0f;
    *pSpacing = 0.0f;

    if (orientation == m_orientation)
    {
        if (!m_isRegular)
        {
            IFC(E_FAIL);
        }

        switch (alignment)
        {
            case xaml_primitives::SnapPointsAlignment_Near:
                *pOffset = m_nearOffset;
                break;

            case xaml_primitives::SnapPointsAlignment_Center:
                *pOffset = m_nearOffset + (m_regularSpacing / 2);
                break;

            case xaml_primitives::SnapPointsAlignment_Far:
                *pOffset = m_farOffset;
                break;
        }

        *pSpacing = m_regularSpacing;
    }

Cleanup:

    RRETURN(hr);
}
