//===--- XamlChecksModule.cpp - clang-tidy --------------------------------===//
//
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license
// information.
//
//===----------------------------------------------------------------------===//

#include "PeerComPtrFieldCheck.h"
#include "clang-tidy/ClangTidyModule.h"
#include "clang-tidy/ClangTidyModuleRegistry.h"

namespace clang {
namespace tidy {
namespace xaml {

/// A module aggregating WinUI/XAML-specific checks.
class XamlChecksModule : public ClangTidyModule {
public:
  void addCheckFactories(ClangTidyCheckFactories &CheckFactories) override {
    CheckFactories.registerCheck<PeerComPtrFieldCheck>(
        "xaml-peer-comptr-field");
  }
};

// Register the module using this statically initialized variable.
static ClangTidyModuleRegistry::Add<XamlChecksModule>
    X("xaml-module", "Adds WinUI/XAML peer-lifetime lint checks.");

} // namespace xaml

// This anchor is used to force the linker to link in the generated object file
// and thus register the XamlChecksModule.
volatile int XamlChecksModuleAnchorSource = 0;

} // namespace tidy
} // namespace clang
