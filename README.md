These plugins and programs are demonstrations of how to create and use the
plugin infrastructure of clang for analyzing ASTs or integrating with the
clang static analyzer.

The AST plugin prints out the names of functions found when parsing a file.
The static analyzer plugin is the SimpleStreamChecker from the clang code
base (as well as [How to write a checker in 24 hours][0] by Anna Zaks and
Jordan Rose). It identifies issues in handling `FILE`s in C program.
Neither plugin is a contribution here. This project just provides an example
of how to integrate such plugins into a standalone project.

Building with CMake
==============================================
1. Clone the demo repository.

        git clone https://github.com/nsumner/clang-plugins-demo.git

2. Create a new directory for building.

        mkdir build

3. Change into the new directory.

        cd build

4. Run CMake with the path to the LLVM source.

        cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=True \
            -DLLVM_DIR=</path/to/LLVM/build>/lib/cmake/llvm/ \
            ../clang-plugins-demo

5. Run make inside the build directory:

        make

This produces standalone tools called `bin/print-functions` and
`bin/runstreamchecker`. Loadable plugin variants of the analyses are also
produced inside `lib/`.

Note, building with a tool like ninja can be done by adding `-G Ninja` to
the cmake invocation and running ninja instead of make.

Running
==============================================

Both the AST plugins and clang static analyzer plugins can be run via
standalone programs or via extra command line arguments to clang. The
provided standalone variants can operate on individual files or on
compilation databases, but compilation databases are somewhat easier to
work with.

AST Plugins
-------------

To load and run AST plugins dynamically in clang, you can use:

        clang -fplugin=lib/libfunction-printer-plugin.so -c ../test/functions.c

To run the plugin via the standalone program:

        bin/print-functions -- clang -c ../test/functions.c

Note that this will require you to put the paths to all headers in the command
line (using `-I`) or they will not be found. It can be simpler to instead use
a compilation database:

        bin/print-functions -p compile_commands.json

Clang Static Analyzer Plugins
-----------------------------

To load and run a static analyzer plugin dynamically in clang, use:

        clang -fsyntax-only -fplugin=lib/libstreamchecker.so \
          -Xclang -analyze -Xclang -analyzer-checker=demo.streamchecker \
          ../clang-plugins-demo/test/files.c

To run the plugin via the standalone program:

        bin/runstreamchecker -- clang -c ../clang-plugins-demo/test/files.c

Again, missing headers are likely, and using a compilation database is the
preferred and simplest way to work around this issue. Note that clang comes
with scripts that can build a compilation database for an existing project.


Licenses
==========
The code for this project is released under an MIT License (included in the
LICENSE file in the root directory of the project).

The code in SimpleStreamChecker.cpp comes from the clang project and is
released under the University of Illinois/NCSA Open Source License.

[0]: http://llvm.org/devmtg/2012-11/Zaks-Rose-Checker24Hours.pdf

