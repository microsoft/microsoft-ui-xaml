# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License. See LICENSE in the project root for license information.

"""Run with: .\\initrun.ps1 python .\\tools\\XamlProfiler\\tests\\test_operation_identities.py

Compile the production operation emitters and identity/containing-text-host resolvers
against small core-object and TraceLogging test doubles. This checks field selection,
pointer adjustment, payload/opcode preservation, and disabled-provider behavior without
starting ETW or requiring a running XAML application. It does not test ETW transport.
"""

import pathlib
import re
import shutil
import subprocess
import tempfile
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[3]


def function_definition(source, signature):
    start = source.index(signature)
    opening = source.index("{", start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


HARNESS = r"""
#include <cstdint>
#include <cstdlib>
#include <initializer_list>
#include <iostream>
#include <map>
#include <sal.h>
#include <string>
#include <type_traits>
#include <variant>

using PCWSTR = const wchar_t*;
using Value = std::variant<uint64_t, int64_t, bool, std::wstring>;
using Fields = std::map<std::string, Value>;
struct Field { std::string name; Value value; };
bool enabled = true;
unsigned writes = 0, casts = 0, parentWalks = 0, checks = 0;
std::string eventName;
Fields fields;

template<typename T> Field MakeField(T value, const char* name)
{
    if constexpr (std::is_same_v<T, PCWSTR>)
        return {name, std::wstring(value)};
    else if constexpr (std::is_same_v<T, bool>)
        return {name, value};
    else if constexpr (std::is_signed_v<T>)
        return {name, static_cast<int64_t>(value)};
    else
        return {name, static_cast<uint64_t>(value)};
}

void Capture(const char* name, std::initializer_list<Field> values)
{
    if (!enabled) return;
    ++writes;
    eventName = name;
    fields.clear();
    for (const auto& value : values)
        if (!fields.emplace(value.name, value.value).second) std::abort();
}

void Require(bool condition)
{
    if (!condition)
    {
        std::cerr << "Failed check " << checks << " for " << eventName << "\n";
        std::exit(1);
    }
    ++checks;
}

#define IMPLEMENT_TELEMETRY_CLASS(...)
#define WINEVENT_LEVEL_VERBOSE 5
#define WINEVENT_OPCODE_START 1
#define WINEVENT_OPCODE_STOP 2
#define TraceLoggingProviderEnabled(...) enabled
#define TraceLoggingValue(value, name) MakeField(value, name)
#define TraceLoggingLevel(value) MakeField(uint64_t(value), "#level")
#define TraceLoggingOpcode(value) MakeField(uint64_t(value), "#opcode")
#define TraceLoggingWrite(provider, name, ...) Capture(name, {__VA_ARGS__})
struct TelemetryBase {};

enum class KnownTypeIndex { Other, TextBlock, RichTextBlock };
struct CDependencyObject
{
    virtual ~CDependencyObject() = default;
    CDependencyObject* parent = nullptr;
    KnownTypeIndex type = KnownTypeIndex::Other;
    CDependencyObject* GetParentInternal(bool) { ++parentWalks; return parent; }
    template<KnownTypeIndex T> bool OfTypeByIndex() { return type == T; }
};
// Deliberately offset core bases: serializing the incoming DO address is not
// sufficient to produce the CUIElement identity, including for controls.
struct Padding { virtual ~Padding() = default; uint64_t padding = 0; };
struct CUIElement : Padding, CDependencyObject {};
struct CFrameworkElement : CUIElement {};
struct ControlPadding { virtual ~ControlPadding() = default; uint64_t padding = 0; };
struct CControl : ControlPadding, CFrameworkElement {};
struct CTextElement : CDependencyObject
{
    CFrameworkElement* GetContainingFrameworkElement();
};
template<typename T> T* do_pointer_cast(CDependencyObject* object)
{
    ++casts;
    return dynamic_cast<T*>(object);
}

// PRODUCTION

uint64_t Address(const void* value) { return reinterpret_cast<uint64_t>(value); }
void Check(const char* name, uint64_t opcode, uint64_t ui, uint64_t object,
           std::initializer_list<Field> payload = {})
{
    Fields expected = {{"#level", uint64_t{5}}};
    if (opcode != 0) expected.emplace("#opcode", opcode);
    if (ui != 0) expected.emplace("cuielement", ui);
    if (object != 0 || ui == 0) expected.emplace("objectpointer", object);
    for (const auto& field : payload) expected.emplace(field.name, field.value);
    Require(eventName == name);
    Require(fields == expected);
}

int main()
{
    CUIElement ui;
    CControl control;
    CFrameworkElement host, otherHost, richHost;
    host.type = KnownTypeIndex::TextBlock;
    richHost.type = KnownTypeIndex::RichTextBlock;
    CDependencyObject collection, object, invalidOwner;
    CTextElement hyperlink, detachedText, richText;
    collection.parent = &host;
    hyperlink.parent = &collection;
    richText.parent = &richHost;
    object.parent = &host; // A generic parent is deliberately not treated as UI ownership.
    Require(Address(static_cast<CDependencyObject*>(&ui)) != Address(&ui));
    Require(Address(static_cast<CUIElement*>(&control)) != Address(&control));

    struct Subject
    {
        CDependencyObject* object;
        uint64_t ui, dependencyObject;
    };
    const Subject subjects[] = {
        {&ui, Address(&ui), 0},
        {&control, Address(static_cast<CUIElement*>(&control)), 0},
        {&hyperlink, Address(static_cast<CUIElement*>(&host)), Address(&hyperlink)},
        {&richText, Address(static_cast<CUIElement*>(&richHost)), Address(&richText)},
        {&detachedText, 0, Address(&detachedText)},
        {&object, 0, Address(&object)},
        {nullptr, 0, 0},
    };
    for (const auto& subject : subjects)
    {
        auto p = subject.object;
        // EMISSIONS
    }

    XamlElementTracing::PrepareContainerStart(&object, &host);
    Check("PrepareContainer", 1, Address(static_cast<CUIElement*>(&host)), Address(&object));
    XamlElementTracing::SetupCCCStart(&object, 17, &host);
    Check("SetupCCC", 1, Address(static_cast<CUIElement*>(&host)), Address(&object),
          {MakeField(int32_t{17}, "ItemIndex")});
    XamlElementTracing::GenerateContainerStop(&object, &host);
    Check("GenerateContainer", 2, Address(static_cast<CUIElement*>(&host)), Address(&object));
    XamlElementTracing::PrepareContainerStop(&control, &otherHost);
    Check("PrepareContainer", 2, Address(static_cast<CUIElement*>(&control)), 0);
    XamlElementTracing::PrepareContainerStart(&object, &invalidOwner);
    Check("PrepareContainer", 1, 0, Address(&object));
    XamlElementTracing::PrepareContainerStop(nullptr, &host);
    Check("PrepareContainer", 2, 0, 0);
    XamlElementTracing::UpdateFocusStart(&hyperlink, &invalidOwner);
    Check("UpdateFocus", 1, Address(static_cast<CUIElement*>(&host)), Address(&hyperlink));
    XamlElementTracing::GenerateContainerStart();
    Require(eventName == "GenerateContainer");
    Require(fields == Fields({{"#level", uint64_t{5}}, {"#opcode", uint64_t{1}}}));

    const auto previousWrites = writes;
    casts = parentWalks = 0;
    enabled = false;
    XamlElementTracing::UpdateFocusStart(&hyperlink);
    XamlElementTracing::GenerateContainerStart();
    Require(writes == previousWrites);
    Require(casts == 0 && parentWalks == 0);
    std::cout << checks << " assertions passed across " << writes << " emissions\n";
}
"""


class OperationIdentityTests(unittest.TestCase):
    def test_production_emitters_and_identity_resolution(self):
        compiler = shutil.which("cl.exe")
        self.assertIsNotNone(compiler, "Run this test through initrun.ps1 to initialize MSVC.")
        header = (ROOT / "dxaml/xcp/components/base/inc/XamlProfilerTracing.h").read_text(encoding="utf-8")
        operations = header[header.index("class XamlElementTracing"):header.rindex("#endif")]
        ui_source = (ROOT / "dxaml/xcp/core/core/elements/uielement.cpp").read_text(encoding="utf-8-sig")
        text_source = (ROOT / "dxaml/xcp/core/text/RichTextArea/TextElement.cpp").read_text(encoding="utf-8-sig")
        production = "\n".join((
            operations,
            function_definition(ui_source, "XamlElementTracing::Identity XamlElementTracing::GetIdentity("),
            function_definition(text_source, "CFrameworkElement *CTextElement::GetContainingFrameworkElement()"),
        ))

        payload_cases = {
            "XYFocusEnteredStart": ("XYFocusEntered", 1, [("PCWSTR{L\"Right\"}", "Direction")]),
            "PlaceElementStart": ("PlaceElement", 1, [("int32_t{-9}", "DataIndex")]),
            "GetBuiltInStyleStop": ("GetBuiltInStyle", 2, [("PCWSTR{L\"Button\"}", "StyleName")]),
            "SetupCCCStart": ("SetupCCC", 1, [("int32_t{17}", "ItemIndex")]),
            "GenerateMCContainer": ("GenerateMCContainer", 0, [("int32_t{11}", "Index")]),
            "VirtualizationIsEnabledByLayout": ("VirtualizationIsEnabledByLayout", 0, [
                ("true", "IsVirtualizationActive"), ("PCWSTR{L\"Items\"}", "ElementName"),
                ("PCWSTR{L\"ListView\"}", "ClassName"), ("PCWSTR{L\"Grid\"}", "ParentClassName")]),
            "HubSectionCount": ("HubSectionCount", 0, [("uint32_t{3}", "SectionCount")]),
            "SendInputScopeToRichEdit": ("SendInputScopeToRichEdit", 0, [("uint32_t{7}", "InputScope")]),
            "SetIhmKeyboardSkin": ("SetIhmKeyboardSkin", 0, [("uint32_t{2}", "KeyboardSkin")]),
            "RealizeTransition": ("RealizeTransition", 0, []),
        }
        for name in ("ShowBegin", "ShowEnd", "HideBegin", "HideEnd", "Reposition"):
            event = "TouchSelectionGripper" + name
            payload_cases[event] = (event, 0, [("true", "IsStartGripper"), ("int32_t{-21}", "X"), ("int32_t{34}", "Y")])
        for name in ("Outer", "Inner", "Content", "Child"):
            event = name + "BoundsStats"
            payload_cases[event] = (event, 0, [("uint32_t{5}", "RecalculatedCount"), ("uint32_t{8}", "ReusedCount")])
        for event in ("FrameNavigated", "FrameNavigating"):
            payload_cases[event] = (event, 0, [("PCWSTR{L\"Page\"}", "Descriptor"), ("uint8_t{2}", "NavigationMode")])

        explicit_methods = set(re.findall(r"static void (\w+)\(", operations))
        self.assertEqual(explicit_methods, set(payload_cases) | {"GenerateContainerStart"})
        activities = re.findall(r"^\s+DEFINE_ELEMENT_ACTIVITY\((\w+)\);", operations, re.MULTILINE)
        self.assertEqual(len(set(activities) | {case[0] for case in payload_cases.values()}), 43)
        emissions = []
        for activity in activities:
            for edge, opcode in (("Start", 1), ("Stop", 2)):
                emissions += [
                    f"XamlElementTracing::{activity}{edge}(p);",
                    f'Check("{activity}", {opcode}, subject.ui, subject.dependencyObject);',
                ]
        for method, (event, opcode, payload) in payload_cases.items():
            args = "".join(", " + value for value, _ in payload)
            expected = ", ".join(f'MakeField({value}, "{name}")' for value, name in payload)
            emissions += [
                f"XamlElementTracing::{method}(p{args});",
                f'Check("{event}", {opcode}, subject.ui, subject.dependencyObject, {{{expected}}});',
            ]
        harness = HARNESS.replace("// PRODUCTION", production).replace("// EMISSIONS", "\n".join(emissions))
        with tempfile.TemporaryDirectory(prefix="xaml-operation-identities-") as directory:
            cpp = pathlib.Path(directory) / "tests.cpp"
            cpp.write_text(harness, encoding="utf-8")
            build = subprocess.run(
                [compiler, "/nologo", "/std:c++17", "/EHsc", "/W4", "/WX", "/utf-8", str(cpp)],
                cwd=directory, text=True, capture_output=True,
            )
            self.assertEqual(build.returncode, 0, build.stdout + build.stderr)
            result = subprocess.run([str(cpp.with_suffix(".exe"))], text=True, capture_output=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            print(result.stdout.strip())


if __name__ == "__main__":
    unittest.main()
