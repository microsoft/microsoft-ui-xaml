// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Helper class for navigating and debugging the DirectComposition tree.
//      Similar to VisualTreeHelper for the XAML visual tree.

#pragma once

#ifndef DCOMPTREEHELPER_H
#define DCOMPTREEHELPER_H

class HWCompNode;
class HWCompTreeNode;
class CUIElement;
class CCoreServices;

//------------------------------------------------------------------------------
//
//  DCompTreeHelper
//
//  A helper class for navigating and inspecting the DirectComposition (DComp)
//  tree. Similar to how VisualTreeHelper works for the XAML visual tree.
//
//  Usage from debugger (Immediate Window):
//      DCompTreeHelper::DumpMainTree(pCoreServices)
//
//------------------------------------------------------------------------------
class DCompTreeHelper
{
public:
    // =========================================================================
    // Tree Navigation
    // =========================================================================
    
    // Get the parent comp node of the specified node.
    static HWCompTreeNode* GetParent(_In_ HWCompNode* pCompNode);
    
    // Get a child comp node at the specified index.
    static HWCompNode* GetChild(_In_ HWCompTreeNode* pCompNode, _In_ int index);
    
    // Get the number of children for a comp tree node.
    static int GetChildrenCount(_In_ HWCompTreeNode* pCompNode);

    // =========================================================================
    // XAML <-> DComp Bridge
    // =========================================================================
    
    // Get the UIElement that owns this comp node.
    static CUIElement* GetUIElement(_In_ HWCompTreeNode* pCompNode);
    
    // Get the comp node for a given UIElement.
    static HWCompTreeNode* GetCompNode(_In_ CUIElement* pUIElement);

    // =========================================================================
    // Debug / Diagnostic Methods
    // =========================================================================
    
    // Dump the entire DComp tree for the main visual tree.
    // Output goes to VS Debug Output window (OutputDebugString).
    // Call from Immediate Window: DCompTreeHelper::DumpMainTree(pCore)
    static void DumpMainTree(_In_ CCoreServices* pCore);

    // Zero-argument convenience overload - grabs the current core itself.
    // Easiest to call from the Immediate Window: DCompTreeHelper::DumpMainTree()
    static void DumpMainTree();
    
    // Dump the DComp tree starting from a specific comp node.
    static void DumpTree(_In_ HWCompTreeNode* pRootCompNode, _In_ int maxDepth = 50);
    
    // Dump the DComp tree for a specific UIElement.
    static void DumpTreeForElement(_In_ CUIElement* pUIElement, _In_ int maxDepth = 50);

    // =========================================================================
    // Auto Snapshot (throttled full-tree dump)
    // =========================================================================

    // Dumps the full indented tree from the given root, but only if enough time
    // has elapsed since the last dump (throttled). Designed to be called once per
    // frame from the comp tree root update, giving a readable periodic snapshot
    // without flooding the output. No debugger or hotkey needed - just watch the
    // debug output (e.g. DebugView).
    static void MaybeDumpTreeSnapshot(_In_ HWCompTreeNode* pRootCompNode);

    // Enable/disable the automatic throttled snapshot. On by default in DBG.
    static void EnableAutoSnapshot(_In_ bool enable);
    static bool IsAutoSnapshotEnabled();

    // =========================================================================
    // Live Logging (Poor Man's Live Visual Tree)
    // =========================================================================

    // Enable/disable live logging of comp tree changes to the VS Output window.
    // Toggle from the Immediate Window: DCompTreeHelper::EnableLiveLogging(true)
    static void EnableLiveLogging(_In_ bool enable);

    // Returns true if live logging is currently enabled.
    static bool IsLiveLoggingEnabled();

    // Log a comp node being added to the tree (called from HWCompTreeNode::InsertChild).
    static void LogCompNodeInserted(_In_ HWCompNode* pParent, _In_ HWCompNode* pChild);

    // Log a comp node being removed from the tree (called from HWCompTreeNode::RemoveChild).
    static void LogCompNodeRemoved(_In_ HWCompNode* pParent, _In_ HWCompNode* pChild);

private:
    // Internal recursive helper (indented text format)
    static void DumpTreeRecursive(
        _In_ HWCompNode* pCompNode,
        _In_ int currentDepth,
        _In_ int maxDepth,
        _In_ int indentLevel);

    // Internal recursive helper (XML format - emits nested <CompNode> tags)
    static void DumpTreeRecursiveXml(
        _In_ HWCompNode* pCompNode,
        _In_ int currentDepth,
        _In_ int maxDepth,
        _In_ int indentLevel);

    // Get description string for a comp node
    static void GetNodeDescription(
        _In_ HWCompNode* pCompNode,
        _Out_writes_(bufferSize) WCHAR* buffer,
        _In_ size_t bufferSize);
};

#endif // DCOMPTREEHELPER_H
