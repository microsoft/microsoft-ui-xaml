# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License. See LICENSE in the project root for license information.
#
# Builds the WinUI/XAML clang-tidy check module (clangTidyXamlModule).
#
# This requires an LLVM/Clang source + build tree: configure it as an
# out-of-tree clang-tidy module (or drop it under
# llvm-project/clang-tools-extra/clang-tidy/xaml and add_subdirectory it).
# See tools/clang-tidy/README.md for exact build steps. It is NOT built by the
# WinUI product build; it is a developer/CI lint gate.

set(LLVM_LINK_COMPONENTS support)

add_clang_library(clangTidyXamlModule STATIC
  PeerComPtrFieldCheck.cpp
  XamlChecksModule.cpp

  LINK_LIBS
  clangTidy
  clangTidyUtils

  DEPENDS
  ClangSACheckers
  )

clang_target_link_libraries(clangTidyXamlModule
  PRIVATE
  clangAST
  clangASTMatchers
  clangBasic
  clangLex
  clangSema
  )
