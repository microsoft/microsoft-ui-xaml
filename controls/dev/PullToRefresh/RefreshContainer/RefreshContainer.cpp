// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"

#include "RefreshContainer.h"
#include "RefreshVisualizer.h"
#include "ScrollViewerIRefreshInfoProviderAdapter.h"
#include "RefreshVisualizerEventArgs.h"
#include "RuntimeProfiler.h"
#include "PTRTracing.h"

#include <DoubleUtil.h>

#define MAX_BFS_DEPTH 10
#define DEFAULT_PULL_DIMENSION_SIZE 100

// Change to 'true' to turn on debugging outputs in Output window
bool PTRTrace::s_IsDebugOutputEnabled{ false };
bool PTRTrace::s_IsVerboseDebugOutputEnabled{ false };

// RefreshContainer is the top level visual in a PTR experience. responsible for displaying its content property along with a RefreshVisualizer,
// in the specified location. It also adapts a member of its contents tree to an IRefreshInfoProvider and attaches it to the
// RefreshVisualizer.
//
// Here is the object map of the PTR experience:
//
//                                          RefreshContainer
//                                          |        |     \
//                                          V     *Visual   \
//                                  ___Adapter_    tree*     \
//                                 /     |     \     |        \
//                                /      V      \    |         \
//                               /  Animation ___\___|_______   \
//                              /    Handler \__  \  |       \   \
//                             |        |       |  | |        |   |
//                             |        V       V  V V        V   V
//                             |   Interaction   Scroll       Refresh
//                             |     Tracker     Viewer       Visualizer
//                              \          \       |          /
//                               \       *owner*   |         /
//                                \          \     V        /
//                                 \          \->Refresh   /
//                                  \__________-> Info <-_/
//                                               Provider
RefreshContainer::~RefreshContainer()
{
    PTR_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);
    if (auto refreshVisualizer = m_refreshVisualizer.safe_get())
    {
        refreshVisualizer.SizeChanged(m_refreshVisualizerSizeChangedToken);
    }
}

RefreshContainer::RefreshContainer()
{
    PTR_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    __RP_Marker_ClassById(RuntimeProfiler::ProfId_RefreshContainer);

    SetDefaultStyleKey(this);

    m_refreshInfoProviderAdapter.set(winrt::make<ScrollViewerIRefreshInfoProviderAdapter>(PullDirection(), nullptr));
    m_hasDefaultRefreshInfoProviderAdapter = true;
    OnRefreshInfoProviderAdapterChanged();        
}

void RefreshContainer::OnApplyTemplate()
{
    PTR_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);
    // BEGIN: Populate template children
    winrt::IControlProtected thisAsControlProtected = *this;
    m_root.set(GetTemplateChildT<winrt::Panel>(L"Root", thisAsControlProtected));
    m_refreshVisualizerPresenter.set(GetTemplateChildT<winrt::Panel>(L"RefreshVisualizerPresenter", thisAsControlProtected));
    // END: Populate template children

    if (m_root)
    {
        auto rootVisual = winrt::ElementCompositionPreview::GetElementVisual(m_root.get());
        rootVisual.Clip(rootVisual.Compositor().CreateInsetClip(0.0f, 0.0f, 0.0f, 0.0f));
    }

    m_refreshVisualizer.set(Visualizer());
    if (!m_refreshVisualizer)
    {
        Visualizer(winrt::RefreshVisualizer());
        m_hasDefaultRefreshVisualizer = true;
    }
    else
    {
        OnRefreshVisualizerChangedImpl();
        m_hasDefaultRefreshVisualizer = false;
    }

    m_refreshPullDirection = PullDirection();
    OnPullDirectionChangedImpl();
}

void RefreshContainer::RequestRefresh() 
{
    PTR_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);
    if (m_refreshVisualizer)
    {
        m_refreshVisualizer.get().RequestRefresh();
    }
}

//Privates
void RefreshContainer::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    winrt::IDependencyProperty property = args.Property();
    if (property == s_VisualizerProperty)
    {
        OnRefreshVisualizerChanged(args);
    }
    else if (property == s_PullDirectionProperty)
    {
        OnPullDirectionChanged(args);
    }
}

void RefreshContainer::OnRefreshVisualizerChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    PTR_TRACE_INFO(*this, TRACE_MSG_METH_PTR_PTR, METH_NAME, this, args.OldValue(), args.NewValue());
    if (m_refreshVisualizer && m_refreshVisualizerSizeChangedToken.value != 0)
    {
        m_refreshVisualizer.get().SizeChanged(m_refreshVisualizerSizeChangedToken);
    }

    m_refreshVisualizer.set(Visualizer());
    m_hasDefaultRefreshVisualizer = false;
    OnRefreshVisualizerChangedImpl();
}

void RefreshContainer::OnRefreshVisualizerChangedImpl()
{
    if (m_refreshVisualizerPresenter)
    {
        m_refreshVisualizerPresenter.get().Children().Clear();
        if (m_refreshVisualizer)
        {
            m_refreshVisualizerPresenter.get().Children().Append(m_refreshVisualizer.get());
        }
    }

    if (m_refreshVisualizer)
    {
        m_refreshVisualizer.get().SizeChanged({ this, &RefreshContainer::OnVisualizerSizeChanged });
        m_refreshVisualizer.get().RefreshRequested({ this, &RefreshContainer::OnVisualizerRefreshRequested });
    }
}

void RefreshContainer::OnPullDirectionChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    PTR_TRACE_INFO(*this, TRACE_MSG_METH_INT_INT, METH_NAME, this, args.OldValue(), args.NewValue());
    m_refreshPullDirection = PullDirection();
    OnPullDirectionChangedImpl();
}

void RefreshContainer::OnPullDirectionChangedImpl()
{
    if (m_refreshVisualizerPresenter)
    {
        switch (m_refreshPullDirection)
        {
        case winrt::RefreshPullDirection::TopToBottom:
            m_refreshVisualizerPresenter.get().VerticalAlignment(winrt::VerticalAlignment::Top);
            m_refreshVisualizerPresenter.get().HorizontalAlignment(winrt::HorizontalAlignment::Stretch);
            if (m_hasDefaultRefreshVisualizer)
            {
                m_refreshVisualizer.get().as<winrt::IRefreshVisualizerPrivate>().SetInternalPullDirection(winrt::RefreshPullDirection::TopToBottom);
                m_refreshVisualizer.get().Height(DEFAULT_PULL_DIMENSION_SIZE);
                m_refreshVisualizer.get().Width(DoubleUtil::NaN);
            }
            break;
        case winrt::RefreshPullDirection::LeftToRight:
            m_refreshVisualizerPresenter.get().VerticalAlignment(winrt::VerticalAlignment::Stretch);
            m_refreshVisualizerPresenter.get().HorizontalAlignment(winrt::HorizontalAlignment::Left);
            if (m_hasDefaultRefreshVisualizer)
            {
                m_refreshVisualizer.get().as<winrt::IRefreshVisualizerPrivate>().SetInternalPullDirection(winrt::RefreshPullDirection::LeftToRight);
                m_refreshVisualizer.get().Height(DoubleUtil::NaN);
                m_refreshVisualizer.get().Width(DEFAULT_PULL_DIMENSION_SIZE);
            }
            break;
        case winrt::RefreshPullDirection::RightToLeft:
            m_refreshVisualizerPresenter.get().VerticalAlignment(winrt::VerticalAlignment::Stretch);
            m_refreshVisualizerPresenter.get().HorizontalAlignment(winrt::HorizontalAlignment::Right);
            if (m_hasDefaultRefreshVisualizer)
            {
                m_refreshVisualizer.get().as<winrt::IRefreshVisualizerPrivate>().SetInternalPullDirection(winrt::RefreshPullDirection::RightToLeft);
                m_refreshVisualizer.get().Height(DoubleUtil::NaN);
                m_refreshVisualizer.get().Width(DEFAULT_PULL_DIMENSION_SIZE);
            }
            break;
        case winrt::RefreshPullDirection::BottomToTop:
            m_refreshVisualizerPresenter.get().VerticalAlignment(winrt::VerticalAlignment::Bottom);
            m_refreshVisualizerPresenter.get().HorizontalAlignment(winrt::HorizontalAlignment::Stretch);
            if (m_hasDefaultRefreshVisualizer)
            {
                m_refreshVisualizer.get().as<winrt::IRefreshVisualizerPrivate>().SetInternalPullDirection(winrt::RefreshPullDirection::BottomToTop);
                m_refreshVisualizer.get().Height(DEFAULT_PULL_DIMENSION_SIZE);
                m_refreshVisualizer.get().Width(DoubleUtil::NaN);
            }
            break;
        default:
            MUX_ASSERT(false);
        }    
        
        //If we have changed the PullDirection but the Adapter wont be updated by the size changed handler.
        if (m_hasDefaultRefreshInfoProviderAdapter &&
            m_hasDefaultRefreshVisualizer &&
            m_refreshVisualizer.get().ActualHeight() == DEFAULT_PULL_DIMENSION_SIZE &&
            m_refreshVisualizer.get().ActualWidth() == DEFAULT_PULL_DIMENSION_SIZE)
        {
            m_refreshInfoProviderAdapter.set(winrt::make<ScrollViewerIRefreshInfoProviderAdapter>(PullDirection(), nullptr));
            OnRefreshInfoProviderAdapterChanged();
        }

    }
}

void RefreshContainer::OnRefreshInfoProviderAdapterChanged()
{
    if (m_root)
    {
        winrt::IRefreshInfoProvider firstChildAsInfoProvider = m_root.get().Children().GetAt(0).try_as<winrt::IRefreshInfoProvider>();
        if (firstChildAsInfoProvider)
        {
            m_refreshVisualizer.get().as<winrt::IRefreshVisualizerPrivate>().InfoProvider(firstChildAsInfoProvider);
        }
        else
        {
            winrt::IRefreshInfoProvider adaptFromTreeResult = nullptr;
            if (m_refreshInfoProviderAdapter)
            {
                adaptFromTreeResult = m_refreshInfoProviderAdapter.get().AdaptFromTree(m_root.get(), m_refreshVisualizer.get().RenderSize());
                if (adaptFromTreeResult)
                {
                    m_refreshVisualizer.get().as<winrt::IRefreshVisualizerPrivate>().InfoProvider(adaptFromTreeResult);
                    m_refreshInfoProviderAdapter.get().SetAnimations(m_refreshVisualizer.get());
                }
            }
            if (!adaptFromTreeResult)
            {
                m_refreshVisualizer.get().as<winrt::IRefreshVisualizerPrivate>().InfoProvider(SearchTreeForIRefreshInfoProvider());
            }
        }
    }
}

winrt::IRefreshInfoProvider RefreshContainer::SearchTreeForIRefreshInfoProvider()
{
    PTR_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);
    if (m_root)
    {
        winrt::IRefreshInfoProvider rootAsIRIP = m_root.try_as<winrt::IRefreshInfoProvider>();
        int depth = 0;
        if (rootAsIRIP)
        {
            return rootAsIRIP;
        }
        else
        {
            while (depth < MAX_BFS_DEPTH)
            {
                winrt::IRefreshInfoProvider result = SearchTreeForIRefreshInfoProviderRecursiveHelper(m_root.get(), depth);
                if (result)
                {
                    return result;
                    break;
                }
                depth++;
            }
        }
    }

    return nullptr;
}

winrt::IRefreshInfoProvider RefreshContainer::SearchTreeForIRefreshInfoProviderRecursiveHelper(winrt::DependencyObject root, int depth)
{
    PTR_TRACE_INFO(*this, TRACE_MSG_METH_INT, METH_NAME, this, depth);
    const int numChildren = winrt::VisualTreeHelper::GetChildrenCount(root);
    if (depth == 0)
    {
        for (int i = 0; i < numChildren; i++)
        {
            winrt::DependencyObject childObject = winrt::VisualTreeHelper::GetChild(root, i);
            winrt::IRefreshInfoProvider childObjectAsIRIP = childObject.try_as<winrt::IRefreshInfoProvider>();
            if (childObjectAsIRIP)
            {
                return childObjectAsIRIP;
            }
        }
        return nullptr;
    }
    else
    {
        for (int i = 0; i < numChildren; i++)
        {
            winrt::DependencyObject childObject = winrt::VisualTreeHelper::GetChild(root, i);
            winrt::IRefreshInfoProvider recursiveResult = SearchTreeForIRefreshInfoProviderRecursiveHelper(childObject, depth - 1);
            if (recursiveResult)
            {
                return recursiveResult;
            }
        }
        return nullptr;
    }
}

void RefreshContainer::OnVisualizerSizeChanged(const winrt::IInspectable& /*sender*/, const winrt::SizeChangedEventArgs& args)
{
    PTR_TRACE_INFO(*this, TRACE_MSG_METH_FLT_FLT_FLT_FLT, METH_NAME, this, args.PreviousSize().Width, args.PreviousSize().Height, args.NewSize().Width, args.NewSize().Height);
    if (m_hasDefaultRefreshInfoProviderAdapter)
    {
        m_refreshInfoProviderAdapter.set(winrt::make<ScrollViewerIRefreshInfoProviderAdapter>(PullDirection(), nullptr));
        OnRefreshInfoProviderAdapterChanged();
    }
}

void RefreshContainer::OnVisualizerRefreshRequested(const winrt::IInspectable& /*sender*/, const winrt::RefreshRequestedEventArgs& args)
{
    PTR_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);
    m_visualizerRefreshCompletedDeferral.set(args.GetDeferral());
    RaiseRefreshRequested();
}

void RefreshContainer::RaiseRefreshRequested()
{
    PTR_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);
    com_ptr<RefreshContainer> strongThis = get_strong();

    winrt::Deferral instance{ [strongThis]()
        {
            strongThis->CheckThread();
            strongThis->RefreshCompleted();
        }
    };

    auto args = winrt::make_self<RefreshRequestedEventArgs>(instance);

    //This makes sure that everyone registered for this event can get access to the deferral
    //Otherwise someone could complete the deferral before someone else has had a chance to grab it
    args->IncrementDeferralCount();
    m_refreshRequestedEventSource(*this, *args);
    args->DecrementDeferralCount();
}

void RefreshContainer::RefreshCompleted()
{
    PTR_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);
    if (m_visualizerRefreshCompletedDeferral)
    {
        m_visualizerRefreshCompletedDeferral.get().Complete();
    }
}

//Private interface implementations
winrt::IRefreshInfoProviderAdapter RefreshContainer::RefreshInfoProviderAdapter()
{
    PTR_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);
    return m_refreshInfoProviderAdapter.get();
}

void RefreshContainer::RefreshInfoProviderAdapter(winrt::IRefreshInfoProviderAdapter const& value)
{
    PTR_TRACE_INFO(*this, TRACE_MSG_METH_PTR, METH_NAME, this, value);
    m_refreshInfoProviderAdapter.set(value);
    m_hasDefaultRefreshInfoProviderAdapter = false;
    OnRefreshInfoProviderAdapterChanged();
}
