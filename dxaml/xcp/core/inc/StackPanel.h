// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CStackPanel final : public CPanel
{
protected:
    CStackPanel(_In_ CCoreServices *pCore) : CPanel( pCore )
    {
        // Jupiter-specific fields
        m_bNotifyHorizontalSnapPointsChanges = FALSE;
        m_bNotifyVerticalSnapPointsChanges = FALSE;
        m_bNotifiedHorizontalSnapPointsChanges = FALSE;
        m_bNotifiedVerticalSnapPointsChanges = FALSE;
        m_bAreSnapPointsKeysHorizontal = FALSE;
        m_pIrregularSnapPointKeys = NULL;
        m_cIrregularSnapPointKeys = -1;
        m_regularSnapPointKey = -1.0;
        m_lowerMarginSnapPointKey = 0.0;
        m_upperMarginSnapPointKey = 0.0;

        m_bAreScrollSnapPointsRegular = false;
        m_orientation = DirectUI::Orientation::Vertical;
    }

   ~CStackPanel() override;

public:
// Creation method
    DECLARE_CREATE(CStackPanel);

// CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CStackPanel>::Index;
    }

// FrameworkElement overrides
protected:
    _Check_return_ HRESULT MeasureOverride(XSIZEF availableSize, XSIZEF& desiredSize) final;
    _Check_return_ HRESULT ArrangeOverride(XSIZEF finalSize, XSIZEF& newFinalSize) final;

// DirectManipulation-specific declarations
public:
    // Used to detect changes in AreScrollSnapPointsRegular property.
    _Check_return_ HRESULT SetValue(_In_ const SetValueParams& args) override;

    // Used to retrieve an array of irregular snap points.
    virtual _Check_return_ HRESULT GetIrregularSnapPoints(
        _In_ bool bHorizontalOrientation,  // True when horizontal snap points are requested.
        _In_ bool bNearAligned,            // True when requested snap points will align to the left/top of the children
        _In_ bool bFarAligned,             // True when requested snap points will align to the right/bottom of the children
        _Outptr_opt_result_buffer_(*pcSnapPoints) XFLOAT** ppSnapPoints,   // Placeholder for irregular snap points
        _Out_ XUINT32* pcSnapPoints);                                  // Placeholder for regular snap points

    // Used to retrieve an offset and interval for regular snap points.
    virtual _Check_return_ HRESULT GetRegularSnapPoints(
        _In_ bool bHorizontalOrientation,  // True when horizontal snap points are requested.
        _In_ bool bNearAligned,            // True when requested snap points will align to the left/top of the children
        _In_ bool bFarAligned,             // True when requested snap points will align to the right/bottom of the children
        _Out_ XFLOAT* pOffset,              // Placeholder for snap points offset
        _Out_ XFLOAT* pInterval);           // Placeholder for snap points interval

    // Determines whether the StackPanel must call NotifySnapPointsChanged
    // when snap points change or not.
    virtual _Check_return_ HRESULT SetSnapPointsChangeNotificationsRequirement(
        _In_ bool bIsForHorizontalSnapPoints,
        _In_ bool bNotifyChanges);

    xref_ptr<CBrush> GetBorderBrush() const final;
    XTHICKNESS GetBorderThickness() const final;
    XTHICKNESS GetPadding() const final;
    XCORNERRADIUS GetCornerRadius() const final;
    DirectUI::BackgroundSizing GetBackgroundSizing() const final;
    float GetSpacing() const;

private:
    // Resets both regular and irregular snap point keys.
    _Check_return_ HRESULT ResetSnapPointKeys();

    // Determines the keys for irregular snap points.
    _Check_return_ HRESULT GetIrregularSnapPointKeys(
        const CUIElementCollectionWrapper& children,
        _Outptr_result_buffer_(*pcSnapPointKeys) XFLOAT** ppSnapPointKeys,
        _Out_ XINT32* pcSnapPointKeys,
        _Out_ XFLOAT* pLowerMarginSnapPointKey,
        _Out_ XFLOAT* pUpperMarginSnapPointKey);

    // Determines the keys for regular snap points.
    _Check_return_ HRESULT GetRegularSnapPointKeys(
        const CUIElementCollectionWrapper& children,
        _Out_ XFLOAT* pSnapPointKey,
        _Out_ XFLOAT* pLowerMarginSnapPointKey,
        _Out_ XFLOAT* pUpperMarginSnapPointKey);

    // Determines the common keys for regular and irregular snap points.
    // Those keys are the left/right margins for a horizontal stackpanel,
    // or the top/bottom margins for a vertical stackpanel.
    _Check_return_ HRESULT GetCommonSnapPointKeys(
        _Out_ XFLOAT* pLowerMarginSnapPointKey,
        _Out_ XFLOAT* pUpperMarginSnapPointKey);

    // Refreshes the m_pIrregularSnapPointKeys/m_cIrregularSnapPointKeys
    // fields based on all children.
    _Check_return_ HRESULT RefreshIrregularSnapPointKeys();

    // Refreshes the m_regularSnapPointKey field based on a single child.
    // Refreshes also the m_lowerMarginSnapPointKey/m_upperMarginSnapPointKey fields based
    // on the current margins.
    _Check_return_ HRESULT RefreshRegularSnapPointKeys();

    // Called to let the peer know that snap points have changed.
    _Check_return_ HRESULT NotifySnapPointsChanges(_In_ bool bIsForHorizontalSnapPoints);

    // Checks if the snap point keys have changed and a notification needs to be raised.
    _Check_return_ HRESULT NotifySnapPointsChanges(const CUIElementCollectionWrapper& children);

    // Called when the AreSnapPointsChanged property changed.
    _Check_return_ HRESULT OnAreScrollSnapPointsRegularChanged();

private:    XFLOAT* m_pIrregularSnapPointKeys;                  // Unique identifiers for irregular snap points (independent of snap point alignment)
private:    XFLOAT m_regularSnapPointKey;                       // Unique identifier for regular snap points (independent of snap point alignment)
private:    XFLOAT m_lowerMarginSnapPointKey;                   // Top/left margin dimension used to compute regular and irregular snap points
private:    XFLOAT m_upperMarginSnapPointKey;                   // Bottom/right margin dimension used to compute regular and irregular snap points
private:    bool m_bNotifyHorizontalSnapPointsChanges : 1;      // True when NotifySnapPointsChanged needs to be called when horizontal snap points change
private:    bool m_bNotifyVerticalSnapPointsChanges : 1;        // True when NotifySnapPointsChanged needs to be called when vertical snap points change
private:    bool m_bNotifiedHorizontalSnapPointsChanges : 1;    // True when NotifySnapPointsChanged was already called once and horizontal snap points have not been accessed since
private:    bool m_bNotifiedVerticalSnapPointsChanges : 1;      // True when NotifySnapPointsChanged was already called once and vertical snap points have not been accessed since
private:    bool m_bAreSnapPointsKeysHorizontal : 1;            // True when the snap point keys are for horizontal snap points
private:    bool : 3;                                           // unused bits
public:     bool m_bAreScrollSnapPointsRegular;                 // Backing field for the AreScrollSnapPointsRegular dependency property.
private:    uint8_t m_cIrregularSnapPointKeys;                  // Number of irregular snap point keys
public:     DirectUI::Orientation m_orientation;



};
