
#include "clang/Frontend/CompilerInstance.h"
#include "clang/StaticAnalyzer/Frontend/FrontendActions.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Tooling/CompilationDatabase.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/raw_ostream.h"

#include <memory>
#include <string>
#include <unistd.h>

#include "config.h"
#include "SimpleStreamChecker.h"


using namespace llvm;


static cl::OptionCategory streamcheckerCategory{"runstreamchecker options"};

static cl::opt<std::string> databasePath{"p",
  cl::desc{"Path to compilation database"},
  cl::Optional,
  cl::cat{streamcheckerCategory}};

static cl::opt<std::string> directCompiler{cl::Positional,
  cl::desc{"[-- <compiler>"},
  cl::cat{streamcheckerCategory},
  cl::init("")};

static cl::list<std::string> directArgv{cl::ConsumeAfter,
  cl::desc{"<compiler arguments>...]"},
  cl::cat{streamcheckerCategory}};


class CommandLineCompilationDatabase : public clang::tooling::CompilationDatabase {
private:
  clang::tooling::CompileCommand compileCommand;
  std::string sourceFile;

public:
  CommandLineCompilationDatabase(llvm::StringRef sourceFile,
                                 std::vector<std::string> commandLine)
    : compileCommand(".", sourceFile, std::move(commandLine), "dummy.o"),
      sourceFile{sourceFile}
      { }

  std::vector<clang::tooling::CompileCommand>
  getCompileCommands(llvm::StringRef filePath) const override {
    if (filePath == sourceFile) {
      return {compileCommand};
    }
    return {};
  }

  std::vector<std::string>
  getAllFiles() const override {
    return {sourceFile};
  }

  std::vector<clang::tooling::CompileCommand>
  getAllCompileCommands() const override {
    return {compileCommand};
  }
};


std::unique_ptr<clang::tooling::CompilationDatabase>
createDBFromCommandLine(llvm::StringRef compiler,
                        llvm::ArrayRef<std::string> commandLine,
                        std::string &errors) {
  auto source = std::find(commandLine.begin(), commandLine.end(), "-c");
  if (source == commandLine.end() || ++source == commandLine.end()) {
    errors = "Command line must contain '-c <source file>'";
    return {};
  }
  llvm::SmallString<128> absolutePath(*source);
  llvm::sys::fs::make_absolute(absolutePath);

  std::vector<std::string> args;
  if (compiler.endswith("++")) {
    args.push_back("c++");
  } else {
    args.push_back("cc");
  }

  args.insert(args.end(), commandLine.begin(), commandLine.end());
  return std::make_unique<CommandLineCompilationDatabase>(absolutePath,
                                                          std::move(args));
}


static std::unique_ptr<clang::tooling::CompilationDatabase>
getCompilationDatabase(std::string &errors) {
  using Database = clang::tooling::CompilationDatabase;
  if (!directCompiler.empty()) {
    return createDBFromCommandLine(directCompiler, directArgv, errors);
  } else if (!databasePath.empty()) {
    return Database::autoDetectFromDirectory(databasePath, errors);
  } else {
    char buffer[256];
    if (!getcwd(buffer, 256)) {
      llvm::report_fatal_error("Unable to get current working directory.");
    }
    return Database::autoDetectFromDirectory(buffer, errors);
  }
}


class FilteringDiagConsumer : public clang::DiagnosticConsumer {
public:
  FilteringDiagConsumer(std::unique_ptr<clang::DiagnosticConsumer>&& diags)
    : diags{std::move(diags)}
      { }
  void
  HandleDiagnostic(clang::DiagnosticsEngine::Level diagLevel,
                   const clang::Diagnostic &info) override {
    if (diagLevel != clang::DiagnosticsEngine::Level::Fatal) {
      // Instead of printing out results, they can be collected here.
      diags->HandleDiagnostic(diagLevel, info);
    }
  }
private:
  std::unique_ptr<clang::DiagnosticConsumer> diags;
};


class StaticAnalyzerAction : public clang::ento::AnalysisAction {
protected:
  std::unique_ptr<clang::ASTConsumer>
  CreateASTConsumer(clang::CompilerInstance &ci,
                    llvm::StringRef inFile) override {
    auto& diags = ci.getDiagnostics();
    diags.setClient(new FilteringDiagConsumer{diags.takeClient()});
    auto analyzerOpts = ci.getAnalyzerOpts();
    analyzerOpts->CheckersAndPackages.clear();
    analyzerOpts->CheckersAndPackages.push_back({CHECKER_PLUGIN_NAME, true});
    auto &plugins = ci.getFrontendOpts().Plugins;
    plugins.push_back(getPluginPath());
    return clang::ento::AnalysisAction::CreateASTConsumer(ci, inFile);
  }

private:
  std::string
  getPluginPath() const {
#ifdef CMAKE_INSTALL_PREFIX
    constexpr const char* path = CMAKE_INSTALL_PREFIX "/lib/" PLUGIN_LIBRARY;
#elif defined(CMAKE_TEMP_LIBRARY_PATH)
    constexpr const char* path = CMAKE_TEMP_LIBRARY_PATH "/" PLUGIN_LIBRARY;
#else
    constexpr char* path = nullptr;
#endif
    if (!path || !llvm::sys::fs::exists(path)) {
      llvm::report_fatal_error("Plugin runner compiled without a working plugin path.");
    }
    return path;
  }
};


static void
processFile(clang::tooling::CompilationDatabase const &database,
            std::string& file) {
  clang::tooling::ClangTool tool{database, file};
  tool.appendArgumentsAdjuster(clang::tooling::getClangStripOutputAdjuster());

  const char* SYSTEM_INCLUDES =
    "-I" LLVM_LIBRARY_DIRS "/clang/11.0.0/include/";
  clang::tooling::CommandLineArguments include{SYSTEM_INCLUDES};
  auto includeInserter = clang::tooling::getInsertArgumentAdjuster(include,
    clang::tooling::ArgumentInsertPosition::BEGIN);
  tool.appendArgumentsAdjuster(includeInserter);

  auto frontendFactory =
    clang::tooling::newFrontendActionFactory<StaticAnalyzerAction>();
  tool.run(frontendFactory.get());
}


static void
processDatabase(clang::tooling::CompilationDatabase const &database) {
  auto count = 0u;
  auto files = database.getAllFiles();
  llvm::outs() << "Number of files: " << files.size() << "\n";

  for (auto &file : files) {
    llvm::outs() << count << ") File: " << file << "\n";
    processFile(database, file);
    ++count;
  }
}


void
warnAboutDebugBuild(llvm::StringRef programName) {
  const unsigned COLUMNS = 80;
  const char SEPARATOR = '*';

  llvm::outs().changeColor(llvm::raw_ostream::Colors::YELLOW, true);
  for (unsigned i = 0; i < COLUMNS; ++i) {
    llvm::outs().write(SEPARATOR);
  }

  llvm::outs().changeColor(llvm::raw_ostream::Colors::RED, true);
  llvm::outs() << "\nWARNING: ";
  llvm::outs().resetColor();
  llvm::outs() << programName << " appears to have been built in debug mode.\n"
               << "Your analysis may take longer than normal.\n";

  llvm::outs().changeColor(llvm::raw_ostream::Colors::YELLOW, true);
  for (unsigned i = 0; i < COLUMNS; ++i) {
    llvm::outs().write(SEPARATOR);
  }
  llvm::outs().resetColor();
  llvm::outs() << "\n\n";
 }


int
main(int argc, char const **argv) {
  sys::PrintStackTraceOnErrorSignal(argv[0]);
  llvm::PrettyStackTraceProgram X(argc, argv);
  llvm_shutdown_obj shutdown;

  cl::HideUnrelatedOptions(streamcheckerCategory);
  cl::ParseCommandLineOptions(argc, argv);

#if !defined(NDEBUG) || defined(LLVM_ENABLE_ASSERTIONS)
  warnAboutDebugBuild(argv[0]);
#endif

  std::string error;
  auto compilationDB = getCompilationDatabase(error);
  if (!compilationDB) {
    llvm::errs() << "Error while trying to load a compilation database:\n"
                 << error << "\n";
    return -1;
  }

  processDatabase(*compilationDB);

  return 0;
}

