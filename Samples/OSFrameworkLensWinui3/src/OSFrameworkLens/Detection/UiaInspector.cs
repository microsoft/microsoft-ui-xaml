using System;
using System.Runtime.InteropServices;

namespace OSFrameworkLens.Detection;

/// <summary>Talks to UIA via direct COM interop — only the vtable slots we use are declared.</summary>
internal sealed class UiaInspector : IDisposable
{
    // CLSID_CUIAutomation (UIAutomationCore.h).
    private static readonly Guid CLSID_CUIAutomation = new("ff48dba4-60ef-4201-aa87-54103eef594e");

    // UIA_PropertyIds (UIAutomationCore.h) — only the ones surfaced in the UI.
    private const int UIA_FrameworkIdPropertyId          = 30024;
    private const int UIA_LocalizedControlTypePropertyId = 30003;
    private const int UIA_NamePropertyId                 = 30005;
    private const int UIA_AutomationIdPropertyId         = 30011;
    private const int UIA_ClassNamePropertyId            = 30012;

    private const int S_OK = 0; // winerror.h

    private IUIAutomation? _uia;

    public UiaInspector()
    {
        var type = Type.GetTypeFromCLSID(CLSID_CUIAutomation)
                   ?? throw new InvalidOperationException("CUIAutomation CLSID not registered");
        _uia = (IUIAutomation)Activator.CreateInstance(type)!;
    }

    public sealed record UiaSnapshot(
        string FrameworkId,
        string ControlType,
        string Name,
        string AutomationId,
        string ClassName);

    public UiaSnapshot? FromPoint(int screenX, int screenY)
    {
        if (_uia is null) return null;
        try
        {
            int hr = _uia.ElementFromPoint(new POINT { X = screenX, Y = screenY }, out var element);
            if (hr != S_OK || element is null) return null;
            try
            {
                return new UiaSnapshot(
                    FrameworkId:  GetStringProperty(element, UIA_FrameworkIdPropertyId),
                    ControlType:  GetStringProperty(element, UIA_LocalizedControlTypePropertyId),
                    Name:         GetStringProperty(element, UIA_NamePropertyId),
                    AutomationId: GetStringProperty(element, UIA_AutomationIdPropertyId),
                    ClassName:    GetStringProperty(element, UIA_ClassNamePropertyId));
            }
            finally
            {
                Marshal.FinalReleaseComObject(element);
            }
        }
        catch (COMException)
        {
            return null; // target exiting / access denied — retry on next tick
        }
        catch (InvalidCastException)
        {
            return null; // unexpected COM shape — retry on next tick
        }
    }

    private static string GetStringProperty(IUIAutomationElement el, int propId)
    {
        try
        {
            if (el.GetCurrentPropertyValue(propId, out object? v) != S_OK || v is null) return string.Empty;
            return v as string ?? string.Empty;
        }
        catch (COMException)
        {
            return string.Empty; // best-effort property read
        }
    }

    public void Dispose()
    {
        if (_uia is not null)
        {
            Marshal.FinalReleaseComObject(_uia);
            _uia = null;
        }
    }

    [StructLayout(LayoutKind.Sequential)]
    private struct POINT { public int X; public int Y; }

    // Minimal IUIAutomation slice — only the entries we use have real signatures;
    // earlier slots are placeholders so the offset of ElementFromPoint is correct.
    [ComImport]
    [Guid("30cbe57d-d9d0-452a-ab13-7ac5ac4825ee")]
    [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    private interface IUIAutomation
    {
        [PreserveSig] int CompareElements(IntPtr a, IntPtr b, out int areSame);
        [PreserveSig] int CompareRuntimeIds(IntPtr a, IntPtr b, out int areSame);
        [PreserveSig] int GetRootElement(out IUIAutomationElement root);
        [PreserveSig] int ElementFromHandle(IntPtr hwnd, out IUIAutomationElement element);
        [PreserveSig] int ElementFromPoint(POINT pt, out IUIAutomationElement? element);
    }

    [ComImport]
    [Guid("d22108aa-8ac5-49a5-837b-37bbb3d7591e")]
    [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    private interface IUIAutomationElement
    {
        [PreserveSig] int SetFocus();
        [PreserveSig] int GetRuntimeId(out IntPtr runtimeId);
        [PreserveSig] int FindFirst(int scope, IntPtr condition, out IUIAutomationElement found);
        [PreserveSig] int FindAll(int scope, IntPtr condition, out IntPtr found);
        [PreserveSig] int FindFirstBuildCache(int scope, IntPtr cond, IntPtr req, out IUIAutomationElement found);
        [PreserveSig] int FindAllBuildCache(int scope, IntPtr cond, IntPtr req, out IntPtr found);
        [PreserveSig] int BuildUpdatedCache(IntPtr req, out IUIAutomationElement updated);
        [PreserveSig] int GetCurrentPropertyValue(int propertyId, [MarshalAs(UnmanagedType.Struct)] out object? value);
    }
}
