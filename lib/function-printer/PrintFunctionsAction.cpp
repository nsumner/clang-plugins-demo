

#include "clang/AST/AST.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include <clang/Frontend/FrontendPluginRegistry.h>

#include "PrintFunctionsAction.h"

using namespace printfunctions;

static clang::FrontendPluginRegistry::Add<PrintFunctionsAction>
  X("function-printer-demo", "Print the names of functions inside the file.");


class FunctionNameVisitor :
  public clang::RecursiveASTVisitor<FunctionNameVisitor> {
public:
  bool
  VisitFunctionDecl(clang::FunctionDecl *f) {
    llvm::outs() << "Function "
                 << (f->hasBody() ? "Def" : "Decl")
                 << " "
                 << f->getNameInfo().getName()
                 << "\n";
    return true;
  }
};


void
PrintFunctionsAction::EndSourceFileAction() {
  auto &ci      = getCompilerInstance();
  auto &context = ci.getASTContext();

  auto &input = getCurrentInput();
  llvm::StringRef fileName = input.getFile();
  llvm::outs() << "Filename in Action: " << fileName << "\n";

  auto *unit = context.getTranslationUnitDecl();
  FunctionNameVisitor visitor;
  visitor.TraverseDecl(unit);

  clang::ASTFrontendAction::EndSourceFileAction();
}

