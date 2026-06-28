 How to install — fastest path (winget, one command)

 Run in a normal PowerShell (you have winget on Win 11). This installs the Build Tools with all
 four C++ components in one shot:

 winget install --id Microsoft.VisualStudio.2022.BuildTools --override "--quiet --wait --add
 Microsoft.VisualStudio.Workload.VCTools --add Microsoft.VisualStudio.Component.VC.CMake.Project --add
 Microsoft.VisualStudio.Component.VC.Llvm.Clang --add Microsoft.VisualStudio.Component.VC.Llvm.ClangToolset --includeRecommended"

 What each --add does:
 - Microsoft.VisualStudio.Workload.VCTools + --includeRecommended → MSVC compiler and the
 Windows SDK (item #1 above).
 - Microsoft.VisualStudio.Component.VC.CMake.Project → CMake and Ninja (item #3).
 - Microsoft.VisualStudio.Component.VC.Llvm.Clang + ...ClangToolset → clang-cl (item #2).

 ▎ If you already have Visual Studio 2022 (not just Build Tools) installed, you can instead open the
 ▎ Visual Studio Installer → Modify and tick the same workload + three components by name:
 ▎ "Desktop development with C++", "C++ CMake tools for Windows", "C++ Clang tools for Windows".

 How to install — VSCode extensions

 code --install-extension ms-vscode.cmake-tools
 code --install-extension llvm-vs-code-extensions.vscode-clangd
 code --install-extension ms-vscode.cpptools

 On first activation, the clangd extension will offer to download the clangd language-server
 binary — accept it. Configure clangd as the IntelliSense provider and disable the Microsoft C/C++
 extension's IntelliSense (C_Cpp.intelliSenseEngine: "disabled") so the two don't fight — but keep
 ms-vscode.cpptools installed, because its cppvsdbg debugger is what gives you the MSVC debugging
 experience on our clang-cl builds. (The .vscode/settings.json we create in step 8 sets this up.)

 Important: the MSVC environment must be active when building

 clang-cl, the Windows SDK headers, and the libs are only found when the MSVC environment variables
 (INCLUDE/LIB/PATH) are set. Two ways this gets handled:
 - In VSCode (normal path): CMake Tools scans for "kits", finds a Clang ... (clang-cl) /
 Visual Studio ... amd64 kit, and automatically sources the VS environment when you configure.
 Just pick the clang-cl kit from the status bar. No manual setup.
 - From a terminal (manual builds / verification): use the "Developer PowerShell for VS 2022"
 (Start menu) rather than a plain PowerShell — it has the environment pre-loaded.

 Verify (run in a Developer PowerShell for VS 2022)

 clang-cl --version     # Clang (expect a recent LLVM, e.g. 18+)
 cmake --version        # >= 3.24
 ninja --version        # any
 git --version

 All four resolving = prerequisites are good. (In a plain PowerShell, clang-cl/cmake/ninja may
 not be on PATH — that's expected; use the Developer PowerShell or let VSCode CMake Tools handle it.)
