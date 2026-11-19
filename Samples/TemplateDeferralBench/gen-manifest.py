# Regenerates app.manifest with WinRT class registrations for the locally built WinUI package.
# A self-contained app built against the local Microsoft.WindowsAppSDK.WinUI 3.0.0-dev package gets no
# MUX/MUXC registrations in its embedded manifest, so activation (e.g. XamlControlsXamlMetaDataProvider) fails.
# Usage (from repo root): python Samples\TemplateDeferralBench\gen-manifest.py
import os
import xml.etree.ElementTree as ET

here = os.path.dirname(os.path.abspath(__file__))
repo = os.path.abspath(os.path.join(here, '..', '..'))
source = os.path.join(repo, 'packages', 'Microsoft.WindowsAppSDK.WinUI.3.0.0-dev', 'build', 'native', 'LiftedWinRTClassRegistrations.xml')
ns = {'m': 'http://schemas.microsoft.com/appx/manifest/foundation/windows10'}
# The other DLLs in the source file are already registered by the auto-generated WindowsAppSDK.manifest.
dlls = ['Microsoft.UI.Xaml.dll', 'Microsoft.UI.Xaml.Controls.dll', 'Microsoft.UI.Xaml.Controls.Tabular.dll',
        'Microsoft.UI.Xaml.Phone.dll', 'WinUIEdit.dll', 'Microsoft.Web.WebView2.Core.dll']

lines = [
    '<?xml version="1.0" encoding="utf-8"?>',
    '<assembly manifestVersion="1.0" xmlns="urn:schemas-microsoft-com:asm.v1">',
    '  <assemblyIdentity version="1.0.0.0" name="TemplateDeferralBench.app" />',
    '  <application xmlns="urn:schemas-microsoft-com:asm.v3">',
    '    <windowsSettings>',
    '      <dpiAware xmlns="http://schemas.microsoft.com/SMI/2005/WindowsSettings">true/PM</dpiAware>',
    '      <dpiAwareness xmlns="http://schemas.microsoft.com/SMI/2016/WindowsSettings">PerMonitorV2, PerMonitor</dpiAwareness>',
    '    </windowsSettings>',
    '  </application>',
]
for server in ET.parse(source).getroot().iterfind('.//m:InProcessServer', ns):
    if server.findtext('m:Path', namespaces=ns) not in dlls:
        continue
    lines.append(f'  <file name="{server.findtext("m:Path", namespaces=ns)}" xmlns="urn:schemas-microsoft-com:asm.v3">')
    for cls in server.iterfind('m:ActivatableClass', ns):
        lines.append(f'    <activatableClass name="{cls.get("ActivatableClassId")}" threadingModel="{cls.get("ThreadingModel")}" xmlns="urn:schemas-microsoft-com:winrt.v1" />')
    lines.append('  </file>')
lines.append('</assembly>')

with open(os.path.join(here, 'app.manifest'), 'w', encoding='utf-8', newline='\r\n') as f:
    f.write('\n'.join(lines) + '\n')
