//===--- PeerComPtrFieldCheck.cpp - clang-tidy ----------------------------===//
//
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license
// information.
//
//===----------------------------------------------------------------------===//

#include "PeerComPtrFieldCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/DeclTemplate.h"
#include "clang/AST/Type.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Sema/Sema.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace xaml {

namespace {

// Storage wrappers that are NOT raw, GC-invisible peer storage. A field whose
// outermost template is one of these is fine and must never be flagged:
//   - TrackerPtr / TrackerPtrVector : the tracked (correct) storage.
//   - ctl::WeakRefPtr               : explicit weak, non-owning opt-out.
//   - NonTracked                    : annotated deliberate opt-out wrapper.
bool isSanctionedStorageName(StringRef Name) {
  return Name == "TrackerPtr" || Name == "TrackerPtrVector" ||
         Name == "WeakRefPtr" || Name == "NonTracked";
}

// Raw COM smart pointers that make their pointee invisible to the GC walk.
bool isRawComPtrName(StringRef Name) {
  return Name == "ComPtr";
}

// Returns the sole template type argument of a ComPtr<T>-like specialization,
// or a null QualType if the type is not a single-type-argument specialization.
QualType getSoleTypeArg(const ClassTemplateSpecializationDecl *Spec) {
  const TemplateArgumentList &Args = Spec->getTemplateArgs();
  if (Args.size() < 1)
    return {};
  const TemplateArgument &Arg = Args[0];
  if (Arg.getKind() != TemplateArgument::Type)
    return {};
  return Arg.getAsType();
}

} // namespace

void PeerComPtrFieldCheck::registerMatchers(MatchFinder *Finder) {
  // Any non-static data member whose type is a class template specialization.
  Finder->addMatcher(
      fieldDecl(hasType(classTemplateSpecializationType())).bind("field"),
      this);
}

bool PeerComPtrFieldCheck::isTrackerTarget(
    QualType Pointee, const MatchFinder::MatchResult &Result) {
  ASTContext &Ctx = *Result.Context;

  // Strip pointer / reference / cv so we key on the underlying record, matching
  // is_tracker_target<T> which is specialized on the interface/record type.
  Pointee = Pointee.getCanonicalType();
  if (Pointee->isPointerType())
    Pointee = Pointee->getPointeeType();
  Pointee = Pointee.getUnqualifiedType();

  const CXXRecordDecl *Record = Pointee->getAsCXXRecordDecl();
  if (!Record)
    return false;

  // Look up the primary template `is_tracker_target` in the translation unit.
  // If the generated TrackerTargetTraits header was not included, we cannot
  // classify the type, so we conservatively do NOT flag (avoids false errors).
  IdentifierInfo &II = Ctx.Idents.get("is_tracker_target");
  DeclarationName Name(&II);
  auto Lookup = Ctx.getTranslationUnitDecl()->lookup(Name);
  const ClassTemplateDecl *Trait = nullptr;
  for (const NamedDecl *ND : Lookup) {
    if (const auto *CTD = dyn_cast<ClassTemplateDecl>(ND)) {
      Trait = CTD;
      break;
    }
  }
  if (!Trait)
    return false;

  // Form is_tracker_target<Pointee> and read its ::value. Find an existing
  // specialization for the pointee type; absence means the primary template
  // (false_type) applies, so the type is not a tracker target.
  void *InsertPos = nullptr;
  TemplateArgument Arg(Ctx.getCanonicalType(Pointee));
  llvm::SmallVector<TemplateArgument, 1> ArgList{Arg};
  ClassTemplateSpecializationDecl *Spec =
      const_cast<ClassTemplateDecl *>(Trait)->findSpecialization(ArgList,
                                                                 InsertPos);
  if (!Spec)
    return false; // No specialization => primary (false_type) => not a target.

  // Read the static constexpr `value` member; a generated tracker-target
  // specialization derives from std::true_type, so value == true.
  for (const Decl *D : Spec->decls()) {
    const auto *VD = dyn_cast<VarDecl>(D);
    if (!VD || VD->getName() != "value")
      continue;
    if (const Expr *Init = VD->getAnyInitializer()) {
      bool Value = false;
      if (Init->EvaluateAsBooleanCondition(Value, Ctx))
        return Value;
    }
  }

  // Fall back to base-class inspection: derives from std::true_type ?
  if (Spec->hasDefinition()) {
    for (const CXXBaseSpecifier &Base : Spec->bases()) {
      if (Base.getType().getAsString().find("true_type") != std::string::npos)
        return true;
    }
  }
  return false;
}

void PeerComPtrFieldCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *Field = Result.Nodes.getNodeAs<FieldDecl>("field");
  if (!Field)
    return;

  const Type *T = Field->getType().getCanonicalType().getTypePtr();
  const auto *CTSD = dyn_cast_or_null<ClassTemplateSpecializationDecl>(
      T->getAsCXXRecordDecl());
  if (!CTSD)
    return;

  StringRef TemplateName = CTSD->getSpecializedTemplate()->getName();

  // Correct or explicitly-sanctioned storage: never flag.
  if (isSanctionedStorageName(TemplateName))
    return;

  // We only police raw ComPtr storage here.
  if (!isRawComPtrName(TemplateName))
    return;

  QualType Pointee = getSoleTypeArg(CTSD);
  if (Pointee.isNull())
    return;

  if (!isTrackerTarget(Pointee, Result))
    return;

  diag(Field->getLocation(),
       "peer field %0 stores a reference-tracker target in a raw ComPtr, which "
       "is invisible to the GC walk; use TrackerPtr<T> for tracked storage, or "
       "the explicit ctl::WeakRefPtr / NonTracked<T> opt-out")
      << Field;
}

} // namespace xaml
} // namespace tidy
} // namespace clang
