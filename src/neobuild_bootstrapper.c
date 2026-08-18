#define NEOBUILD_IMPLEMENTATION
#include "neobuild.h"
#undef NEOBUILD_IMPLEMENTATION

void* malloc(size_t size);
int main(int argc, char* argv[])
{
  neobuild_error error = {0};
  void* compilerPathHandle = neobuild_msvc_FindCompilerPath(malloc(neobuild_msvc_FindCompilerPathRequiredMemory(&error)), &error);
  void* msvcEnvHandle = neobuild_msvc_InitializeEnvironment(compilerPathHandle, malloc(neobuild_msvc_InitializeEnvironmentRequiredMemory(compilerPathHandle, &error)), &error);
  neobuild_string_view msvcEnvironment = neobuild_msvc_GetEnvironment(msvcEnvHandle, malloc(neobuild_msvc_EnvironmentBlockSize(msvcEnvHandle, &error)), &error);
    
  // Check if the build tool source file is newer than the executable and recompile if it is
  neobuild_command recompileCommand = {0};
  recompileCommand.optimizationLevel = neobuild_optimize_level_DISABLED;
  recompileCommand.generateDebugSymbols = TRUE;
  neobuild_SetMSVCEnvironment(&recompileCommand, msvcEnvironment);
  neobuild_SetCommandMemory(&recompileCommand, malloc(neobuild_RecompileCommandSize(argc, argv, &recompileCommand, &error)));
  neobuild_Recompile(argc, argv, &recompileCommand, &error);

  return 0;
}