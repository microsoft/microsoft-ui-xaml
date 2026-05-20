// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "DataItem.h"

using namespace Tests::Native::External::Framework;

DataItem::DataItem(Platform::String^ title, Platform::String^ text) { m_title = title; m_buttonText = text; }
Platform::String^ DataItem::ButtonText::get() { return m_buttonText; }
void DataItem::ButtonText::set(Platform::String^ value) { m_buttonText = value; }
Platform::String^ DataItem::Title::get() { return m_title; }
void DataItem::Title::set(Platform::String^ value) { m_title = value; }