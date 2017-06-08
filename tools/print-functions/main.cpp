
#include "clang/Tooling/CompilationDatabase.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/raw_ostream.h"

#include <memory>
#include <string>
#include <unistd.h>

#include "PrintFunctionsAction.h"

using namespace printfunctions;
using namespace llvm;


static cl::OptionCategory printFunctionsCategory{"print-functions options"};

static cl::opt<std::string> databasePath{"p",
  cl::desc{"Path to compilation database"},
  cl::Optional,
  cl::cat{printFunctionsCategory}};

static cl::opt<std::string> directCompiler{cl::Positional,
  cl::desc{"[-- <compiler>"},
  cl::cat{printFunctionsCategory},
  cl::init("")};

static cl::list<std::string> directArgv{cl::ConsumeAfter,
  cl::desc{"<compiler arguments>...]"},
  cl::cat{printFunctionsCategory}};


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


static void
processFile(clang::tooling::CompilationDatabase const &database,
            std::string& file) {
  clang::tooling::ClangTool tool{database, file};
  tool.appendArgumentsAdjuster(clang::tooling::getClangStripOutputAdjuster());
  auto frontendFactory =
    clang::tooling::newFrontendActionFactory<PrintFunctionsAction>();
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

  cl::HideUnrelatedOptions(printFunctionsCategory);
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

