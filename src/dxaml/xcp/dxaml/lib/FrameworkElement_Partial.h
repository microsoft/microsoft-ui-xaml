// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//
//      Provides a framework of common APIs for objects that participate in
//      layout. FrameworkElement also defines APIs related to data binding,
//      object tree, and object lifetime feature areas.

#pragma once

#include "FrameworkElement.g.h"
#include "JoltClasses.h"

namespace DirectUI
{
    typedef CFrameworkEventSource<
        IDataContextChangedEventSource,
        IDataContextChangedHandler,
        DependencyObject,
        const DataContextChangedParams> DataContextChangedEventSource;

    class BindingExpression;

    PARTIAL_CLASS(FrameworkElement)
    {
    protected:

        FrameworkElement():
            m_pDataContextChangedSource(NULL),
            m_fIsDataContextBound(FALSE),
            m_isStyleSetFromItemsControl(FALSE),
            m_bHasOpenToolTip(FALSE),
            m_bListenForUnloaded(FALSE),
            m_fIsAutomationPeerFactorySet(FALSE),
            m_pChainedGoToElementStateSourceControls(nullptr)
        {}

        ~FrameworkElement() override;

    public:
        // Raise the LayoutUpdated event
        static _Check_return_ HRESULT OnLayoutUpdated(
            _In_opt_ IInspectable* pSender,
            _In_ IInspectable* pArgs);

        IFACEMETHOD(get_Parent)(_Outptr_ xaml::IDependencyObject** pValue) override;

        IFACEMETHOD(OnCreateAutomationPeer)(_Outptr_ xaml_automation_peers::IAutomationPeer** returnValue) override;

        _Check_return_ HRESULT HasParent(_Out_ BOOLEAN* pHasParent);
        _Check_return_ HRESULT TryGetParent(_Outptr_ xaml::IDependencyObject** pValue);

        // Public methods not exposed in the interface
        _Check_return_ HRESULT GetDataContextChangedSource(_Outptr_ IDataContextChangedEventSource **ppSource);

        _Check_return_ HRESULT GetEffectiveDataContext(_Outptr_ IInspectable** ppValue);

        _Check_return_ HRESULT OnTreeParentUpdated(_In_opt_ CDependencyObject *pNewParent, _In_opt_ BOOLEAN isParentAlive) override;

        // FrameworkElement is its own inheritance context, no need to do anything
        _Check_return_ HRESULT OnInheritanceContextChanged() override { return S_OK; }

        // Gets a reference to the template parent of this element. This
        // property is not relevant if the element was not created through a
        // template.
        _Check_return_ HRESULT get_TemplatedParent(
            _Outptr_ DependencyObject** ppTemplatedParent);

        IFACEMETHOD(add_LayoutUpdated)(
            _In_ wf::IEventHandler<IInspectable*>* pHandler,
            _Out_ EventRegistrationToken* ptToken) override;
        IFACEMETHOD(remove_LayoutUpdated)(
            _In_ EventRegistrationToken tToken) override;

        IFACEMETHOD(RaiseLayoutUpdated)(
            _In_ IInspectable* pSender,
            _In_ IInspectable* pArgs);

        // Occurs when either the ActualHeight or the ActualWidth properties
        // change value on a FrameworkElement.
        IFACEMETHOD(add_SizeChanged)(
            _In_ xaml::ISizeChangedEventHandler* pHandler,
            _Out_ EventRegistrationToken* ptToken) override;
        IFACEMETHOD(remove_SizeChanged)(
            _In_ EventRegistrationToken tToken) override;

        // Raise the SizeChanged event
        _Check_return_ HRESULT OnSizeChanged(
            _In_ IInspectable* pSender,
            _In_ xaml::ISizeChangedEventArgs* pArgs);

        _Check_return_ HRESULT InvalidateViewportImpl();

        virtual _Check_return_ HRESULT FindNameInPage(
            _In_ HSTRING strElementName,
            _In_ bool fIsCalledFromUserControl,
            _Outptr_ IInspectable **ppObj);

        virtual _Check_return_ HRESULT GetPlainText(_Out_ HSTRING* strPlainText);

        virtual _Check_return_ HRESULT HasFocus(_Out_ BOOLEAN *pbHasFocus);

        static _Check_return_ HRESULT OnApplyTemplateFromCore(
            _In_ CFrameworkElement* nativeTarget);

        static _Check_return_ HRESULT MeasureOverrideFromCore(
            _In_ CFrameworkElement* nativeTarget,
            _In_ XFLOAT inWidth,
            _In_ XFLOAT inHeight,
            _Out_ XFLOAT* outWidth,
            _Out_ XFLOAT* outHeight);

        static _Check_return_ HRESULT ArrangeOverrideFromCore(
            _In_ CFrameworkElement* nativeTarget,
            _In_ XFLOAT inWidth,
            _In_ XFLOAT inHeight,
            _Out_ XFLOAT* outWidth,
            _Out_ XFLOAT* outHeight);

        static _Check_return_ HRESULT GetLogicalParentForAPCore(
            _In_ CDependencyObject* nativeTarget,
            _Outptr_ CDependencyObject** ppLogicalParentForAP);

        static _Check_return_ HRESULT GetStringFromObject(
            _In_ IInspectable* pObject,
            _Out_ HSTRING* returnValue);


        virtual _Check_return_ HRESULT OnAncestorDataContextChanged(_In_ const DataContextChangedParams& args);

        _Check_return_ HRESULT GetValueFromStyle(
            _In_ const CDependencyProperty* pDP,
            _Out_opt_ IInspectable** ppValue,
            _Out_ bool* pbGotValue);

        _Check_return_ HRESULT GetBindingExpression(_In_ const CDependencyProperty* pProperty, _Outptr_ xaml_data::IBindingExpression** returnValue);
        _Check_return_ HRESULT SetBinding(_In_ const CDependencyProperty* pProperty, _In_ xaml_data::IBindingBase* binding);

        _Check_return_ HRESULT GetInheritanceParent(_Outptr_ DependencyObject **ppParent);

        // Check whether this FrameworkElement has a Style set by an ItemsControl. This typically happens when the user provides an explicit container
        // in XAML, but does not set a local style for the container.
        BOOLEAN GetIsStyleSetFromItemsControl();

        // Set a flag showing whether this FrameworkElement had a style set by an ItemsControl.
        void SetIsStyleSetFromItemsControl(BOOLEAN value);

        // Clips the given point (which is relative to the specified FrameworkElement)
        // to the bounds of this FrameworkElement.
        _Check_return_ HRESULT ClipPointToElementBounds (
            _In_ FrameworkElement* pElementPointRelativeTo,
            _Inout_ wf::Point* pPoint);

        _Check_return_ HRESULT SetHasOpenToolTip(_In_ bool bHasOpenToolTip);

        XCP_FORCEINLINE void SetIsDataContextBound(_In_ BOOLEAN bIsDataContextBound)
        {
            VERIFYHR(MarkHasState());
            m_fIsDataContextBound = bIsDataContextBound;
        }

        XCP_FORCEINLINE BOOLEAN IsDataContextBound()
        {
            return m_fIsDataContextBound;
        }

        // Used by VisualStateManager to allow the FrameworkElement to react to calls to
        // VisualStateManager.GoToState on its templated parent (or itself, if this is a UserControl).
        // Don't call this method directly - use InvokeGoToElementStateWithControl.
        virtual _Check_return_ HRESULT GoToElementStateCoreImpl(_In_ HSTRING stateName, _In_ BOOLEAN useTransitions, _Out_ BOOLEAN* returnValue);

        // Our default implementation of GoToElementStateCoreImpl needs to know the control that GoToState is
        // being called on, so it can eventually raise events on the VSGs. However, this control isn't part of the
        // public API (by design). This method allows us to hack in knowledge of this control by acting as a
        // wrapper around GoToElementStateCoreImpl calls. Sadly we need this behavior so delvelopers can call base
        // in GoToElementStateCoreImpl and have it work as expected.
        _Check_return_ HRESULT InvokeGoToElementStateWithControl(
            _In_ xaml_controls::IControl* pControl,
            _In_ HSTRING stateName,
            _In_ BOOLEAN bUseTransitions,
            _Out_ BOOLEAN* retval);

        // Scrollviewer looks at its scrollbarvisibility settings and will substitute infinite
        // if those are set to visible or auto. This is an old design that is hard to change
        // However, certain features such as the new modern virtualizingpanels do not appreciate this.
        // They lack the extra communication that scrolldata presents to IScrollInfo implementors, so
        // in those cases we wish to go with a newer more modern approach of actually trusting layout to
        // pass in the correct values. They still need infinity in the virtualizing direction though,
        // because otherwise a bigger than passed in available size will be clipped.
        virtual BOOLEAN WantsScrollViewerToObscureAvailableSizeBasedOnScrollBarVisibility(_In_ xaml_controls::Orientation orientation)
        {
            return true;    // by default, use the old behavior
        }

        // Returns True when either the Width, MinWidth or MaxWidth property was set to a non-default value.
        bool IsWidthSpecified();
        // Returns True when either the Height, MinHeight or MaxHeight property was set to a non-default value.
        bool IsHeightSpecified();

        // Customized properties.
        _Check_return_ HRESULT get_BaseUriImpl(_Outptr_ wf::IUriRuntimeClass** pValue);

        // Customized methods.
        _Check_return_ HRESULT MeasureOverrideImpl(_In_ wf::Size availableSize, _Out_ wf::Size* returnValue);
        _Check_return_ HRESULT ArrangeOverrideImpl(_In_ wf::Size finalSize, _Out_ wf::Size* returnValue);
        _Check_return_ HRESULT IsViewportImpl(_Out_ BOOLEAN* returnValue);
        _Check_return_ HRESULT FindNameImpl(_In_ HSTRING name, _Outptr_ IInspectable** returnValue);
        _Check_return_ HRESULT SetBindingImpl(_In_ xaml::IDependencyProperty* dp, _In_ xaml_data::IBindingBase* binding);
        _Check_return_ HRESULT GetBindingExpressionImpl(_In_ xaml::IDependencyProperty* dp, _Outptr_ xaml_data::IBindingExpression** returnValue);

        static _Check_return_ HRESULT PropagateDataContextChange(_In_ CFrameworkElement* pChildCore);

        _Check_return_ HRESULT get_AutomationPeerFactoryIndex(_Out_ INT* pValue);
        _Check_return_ HRESULT put_AutomationPeerFactoryIndex(_In_ INT value);

        _Check_return_ HRESULT get_FocusVisualPrimaryBrushImpl(_Outptr_result_maybenull_ xaml_media::IBrush** ppValue);
        _Check_return_ HRESULT put_FocusVisualPrimaryBrushImpl(_In_opt_ xaml_media::IBrush* pValue);

        _Check_return_ HRESULT get_FocusVisualSecondaryBrushImpl(_Outptr_result_maybenull_ xaml_media::IBrush** ppValue);
        _Check_return_ HRESULT put_FocusVisualSecondaryBrushImpl(_In_opt_ xaml_media::IBrush* pValue);

        _Check_return_ HRESULT get_IsLoadedImpl(_Out_ BOOLEAN* pValue);
    protected:
        _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args) override;

        IFACEMETHOD(OnDisconnectVisualChildren)() override;

        // DataContext propagation
        // TODO: To avoid problems with missing typeinfo for some of our classes
        // this method is not going to return any errors.
        virtual void NotifyOfDataContextChange(_In_ const DataContextChangedParams& args);

        virtual _Check_return_ HRESULT NotifyBindingExpressions(_In_ const DataContextChangedParams& args);
        virtual _Check_return_ HRESULT PropagateDataContextChanged(_In_ const DataContextChangedParams& args);
        virtual _Check_return_ HRESULT IsDataContextChangeRelevant(_In_ const DataContextChangedParams& args, _Out_ BOOLEAN *pfIsDataContextChangeRelevant);

        void OnReferenceTrackerWalk(_In_ INT walkType) override
        {
            if( m_pDataContextChangedSource != NULL )
            {
                m_pDataContextChangedSource->ReferenceTrackerWalk(static_cast<EReferenceTrackerWalkType>(walkType));
            }

            FrameworkElementGenerated::OnReferenceTrackerWalk(walkType);
        }

        virtual _Check_return_ HRESULT GetLogicalParentForAPProtected(
            _Outptr_ DependencyObject** ppLogicalParentForAP)
        {
            *ppLogicalParentForAP = nullptr;
            RRETURN(S_OK);
        }

    private:

        _Check_return_ HRESULT OnUnloaded(_In_ IInspectable* pSender, _In_ xaml::IRoutedEventArgs* pArgs);

        // static
        static _Check_return_ HRESULT PropagateDataContextChangedInCoreTree(
            _In_ CFrameworkElement *pElementCore,
            _In_ const DataContextChangedParams& args);

        DataContextChangedEventSource *m_pDataContextChangedSource;
        BOOLEAN m_fIsDataContextBound : 1;
        BOOLEAN m_isStyleSetFromItemsControl : 1;
        BOOLEAN m_bHasOpenToolTip : 1;              // Whether an automatic ToolTip is currently opened with this Control as its owner.
        BOOLEAN m_bListenForUnloaded : 1;
        BOOLEAN m_fIsAutomationPeerFactorySet : 1;

        // This field exists to implement the required virtual behavior of GoToElementStateCore. See InvokeGoToElementStateWithControl.
        std::vector<xaml_controls::IControl*>* m_pChainedGoToElementStateSourceControls;
    };
}
