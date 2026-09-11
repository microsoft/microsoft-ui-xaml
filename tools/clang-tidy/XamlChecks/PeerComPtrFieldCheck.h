//===--- PeerComPtrFieldCheck.h - clang-tidy --------------------*- C++ -*-===//
//
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license
// information.
//
//===----------------------------------------------------------------------===//

#pragma once

#include "clang-tidy/ClangTidyCheck.h"

namespace clang {
namespace tidy {
namespace xaml {

/// Flags peer fields that are stored in a raw ComPtr<T> (GC-invisible) when the
/// pointee type T is a reference-tracker target, i.e. `is_tracker_target<T>` is
/// true. Storing a peer this way hides the reference from the reference-tracker
/// GC walk and is the root cause of leaked / dangling managed peers.
///
/// The only sanctioned opt-outs are the explicit, greppable wrappers
/// `ctl::WeakRefPtr` (weak, non-owning) and `NonTracked<T>` (annotated
/// deliberate opt-out). Tracked peers must use `TrackerPtr<T>`.
///
/// For the user-facing documentation see:
/// tools/clang-tidy/README.md
class PeerComPtrFieldCheck : public ClangTidyCheck {
public:
  PeerComPtrFieldCheck(StringRef Name, ClangTidyContext *Context)
      : ClangTidyCheck(Name, Context) {}

  // Only run on C++; the trait is a C++ construct.
  bool isLanguageVersionSupported(const LangOptions &LangOpts) const override {
    return LangOpts.CPlusPlus;
  }

  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;

private:
  // Resolves whether `is_tracker_target<T>::value` is true for the given
  // pointee type, by instantiating/looking up the trait specialization.
  bool isTrackerTarget(QualType Pointee,
                       const ast_matchers::MatchFinder::MatchResult &Result);
};

} // namespace xaml
} // namespace tidy
} // namespace clang
