#define NEOBUILD_IMPLEMENTATION
#include "neobuild\neobuild.h"
#undef NEOBUILD_IMPLEMENTATION

// std functions
void* malloc(size_t size);
void free(void* data);

int main(int argc, char* argv[])
{
  neobuild_error error = {0};
  void* compilerPathHandle = neobuild_msvc_FindCompilerPath(malloc(neobuild_msvc_FindCompilerPathRequiredMemory()), &error);
  void* msvcEnvHandle = neobuild_msvc_InitializeEnvironment(compilerPathHandle, malloc(neobuild_msvc_InitializeEnvironmentRequiredMemory(compilerPathHandle, &error)), &error);
  neobuild_msvc_environment msvcEnvironment = neobuild_msvc_GetEnvironment(msvcEnvHandle, malloc(neobuild_msvc_EnvironmentBlockSize(msvcEnvHandle, &error)), &error);

  // Check if the build tool source file is newer than the executable and recompile if it is
  neobuild_command recompileCommand = {0};
  neobuild_SetMSVCEnvironment(&recompileCommand, msvcEnvironment);
  neobuild_SetCommandMemory(&recompileCommand, malloc(neobuild_RecompileCommandSize(argc, argv, &recompileCommand)));
  neobuild_Recompile(argc, argv, &recompileCommand);
 
  neobuild_command buildCommand = {0};
  buildCommand.executableType = neobuild_binary_type_WIN32_CONSOLE;
  buildCommand.buildLanguage = neobuild_target_language_C;
  neobuild_compile_target helloWorldSrcFile = { "C:\\neo\\proj\\neobuild\\examples\\helloworld\\src\\helloworld.c", neobuild_target_language_C };
  buildCommand.compileTargets.data = &helloWorldSrcFile;
  buildCommand.compileTargets.count = 1;
  buildCommand.binaryFilepath.filepath = "C:\\neo\\proj\\neobuild\\examples\\helloworld\\binaries\\helloworld.exe";
  buildCommand.binaryFilepath.length = neostr_Length(buildCommand.binaryFilepath.filepath);
  buildCommand.intermediateFilepath.filepath = "C:\\neo\\proj\\neobuild\\examples\\helloworld\\binaries\\";
  buildCommand.intermediateFilepath.length = neostr_Length(buildCommand.intermediateFilepath.filepath);
  buildCommand.debugSymbolsFilepath.filepath = "C:\\neo\\proj\\neobuild\\examples\\helloworld\\binaries\\";
  buildCommand.debugSymbolsFilepath.length = neostr_Length(buildCommand.debugSymbolsFilepath.filepath);
  buildCommand.optimizationLevel = neobuild_optimize_level_DISABLED;
  buildCommand.warningLevel = neobuild_warning_level_ALL_ERROR;
  buildCommand.generateDebugSymbols = TRUE;

  neobuild_SetMSVCEnvironment(&buildCommand, msvcEnvironment);
  neobuild_SetCommandMemory(&buildCommand, malloc(neobuild_RequiredCommandMemory(&buildCommand)));
  neobuild_ParseAndRunCommand(&buildCommand); 

  return 0;
}