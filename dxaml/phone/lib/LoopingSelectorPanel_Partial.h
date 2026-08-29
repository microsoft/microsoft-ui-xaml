// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      LoopingSelectorPanel is designed to be used exclusively within a LoopingSelector control as
//      the content of the ScrollViewer. It allows elements to be abritrarily positioned on a Canvas
//      while

#pragma once

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Controls { namespace Primitives
{
    class LoopingSelectorPanel :
        public LoopingSelectorPanelGenerated
    {

    public:
        LoopingSelectorPanel();

        _Check_return_ HRESULT OnPropertyChanged(
            _In_ xaml::IDependencyPropertyChangedEventArgs* pArgs);

        _Check_return_ HRESULT SetOffsetInPixels(_In_ FLOAT offset);
        _Check_return_ HRESULT SetSizeInPixels(_In_ FLOAT size);

    private:
        ~LoopingSelectorPanel();

        _Check_return_ HRESULT InitializeImpl() override;

    public:
         // Implementation of IScrollSnapPointsInfo
        _Check_return_ HRESULT get_AreHorizontalSnapPointsRegularImpl(_Out_ boolean* pValue) override;
        _Check_return_ HRESULT get_AreVerticalSnapPointsRegularImpl(_Out_ boolean* pValue) override;

        _Check_return_ HRESULT GetIrregularSnapPointsImpl(
            _In_ xaml_controls::Orientation orientation,
            _In_ xaml_primitives::SnapPointsAlignment alignment,
            _Outptr_ wfc::IVectorView<FLOAT>** returnValue);

        _Check_return_ HRESULT GetRegularSnapPointsImpl(
            _In_ xaml_controls::Orientation orientation,
            _In_ xaml_primitives::SnapPointsAlignment alignment,
            _Out_ FLOAT* offset,
            _Out_ FLOAT* returnValue);

    private:
        FLOAT _snapPointOffset;
        FLOAT _snapPointSpacing;

        wrl::EventSource<wf::IEventHandler<IInspectable*>> _snapPointsChangedEventSource;

        _Check_return_ HRESULT RaiseSnapPointsChangedEvents();
    };

} } } } } XAML_ABI_NAMESPACE_END
