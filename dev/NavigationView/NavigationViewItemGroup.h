// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "NavigationViewItemBase.h"
#include "NavigationViewItemGroup.g.h"
#include "NavigationViewItemGroup.properties.h"

class NavigationViewItemGroup :
	public winrt::implementation::NavigationViewItemGroupT<NavigationViewItemGroup, NavigationViewItemBase>,
	public NavigationViewItemGroupProperties
{
public:
	ForwardRefToBaseReferenceTracker(NavigationViewItemBase)

    NavigationViewItemGroup();

	// These functions are ambiguous with NavigationViewItemBase, disambiguate 
	using NavigationViewItemGroupProperties::EnsureProperties;
	using NavigationViewItemGroupProperties::ClearProperties;

	// IFrameworkElementOverrides
	void OnApplyTemplate() override;

	//void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
private:

    void SetNavigationViewItemGroupListPosition(winrt::ListView& listView, NavigationViewListPosition position);

};
