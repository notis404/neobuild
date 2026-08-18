
// Basic types
typedef unsigned char neobuild_uInt8;
typedef neobuild_uInt8 neobuild_uByte;
typedef neobuild_uByte neobuild_uBool8; 
typedef unsigned short neobuild_uInt16;
typedef neobuild_uInt16 neobuild_uChar16;
typedef unsigned int neobuild_uInt32;
typedef unsigned __int64  neobuild_uInt64;
typedef int neobuild_sInt32;
typedef __int64 neobuild_sInt64;
typedef float neobuild_float32;
typedef double neobuild_float64;
#define NULL 0
#define FALSE 0
#define TRUE 1

// std functions
void* malloc(neobuild_uInt64 size);
void* memcpy(void* destination, const void* source, neobuild_uInt64 size);
neobuild_sInt32 memcmp(const void* left, const void* right, neobuild_uInt64 count);
void* memset(void* dest, neobuild_uByte value, neobuild_uInt64 n);

//======================================= start neostr.h ======================================================
// neostr consists of 3 parts, sequential in memory
// [payload][metadata][string data]
// payload: user-supplied, can be a struct of any size
// metadata: contains information about the payload, such as size and the most recent checksums of the payload struct and the metadata struct
// string data: the memory allocated for the string data

// Users hold onto a pointer to the string data so the address appears like a normal string
// Base memory address can be retrieved by retrieving the address to the payload 
// Macros are used to access the members of different payload types by name
// Certain macro functions expect a certain kind of member variable to exist to function correctly (length, capacity, etc)
// Comments above these macro functions will specify which member names are required. 
// The actual text of the member name can be configured with the member name macros (NEOSTR_MEMBERNAME_LENGTH, etc)  

#if !defined NEOSTR_MEMBERNAME_LENGTH
#define NEOSTR_MEMBERNAME_LENGTH length
#endif

#if !defined NEOSTR_MEMBERNAME_CAPACITY
#define NEOSTR_MEMBERNAME_CAPACITY capacity
#endif

// Basic types
typedef unsigned char neostr_uInt8;
typedef neostr_uInt8 neostr_uByte;
typedef neostr_uByte neostr_uBool8; 
typedef unsigned short neostr_uInt16;
typedef neostr_uInt16 neostr_uChar16;
typedef unsigned int neostr_uInt32;
typedef unsigned __int64  neostr_uInt64;
typedef int neostr_sInt32;
typedef __int64 neostr_sInt64;
typedef float neostr_float32;
typedef double neostr_float64;

// Some simple payload structs supplied for ease of use
typedef struct neostr_fixed_payload64
{
  neostr_uInt64 length;
}neostr_fixed_payload64;

typedef struct neostr_dynamic_payload64
{
  neostr_uInt64 capacity;
  neostr_uInt64 length;
}neostr_dynamic_payload64;

typedef struct neostr_metadata
{
  neostr_uInt64 payloadSize;
  neostr_uInt32 payloadCRC;
  neostr_uInt32 metadataCRC;
}neostr_metadata;
#define NEOSTR_METADATA_WITHOUT_CRC_SIZE sizeof(neostr_metadata) - sizeof(neostr_uInt32)

#pragma section(".rdata$neostr_lit_a", read)
#pragma section(".rdata$neostr_lit_data", read)
#pragma section(".rdata$neostr_lit_z", read)

__declspec(allocate(".rdata$neostr_lit_a")) const char neostr_literal_data_start = 0;
__declspec(allocate(".rdata$neostr_lit_z")) const char neostr_literal_data_end = 0;

extern const char neostr_literal_data_start;
extern const char neostr_literal_data_end;

// Creates a literal neostr in .rdata. CRC is not used to verify validity of literals. Instead, literals are validated by confirming they are 
// in the correct data section
#define neostr_Literal(literalVariableName, cStringLiteral)                                   \
__declspec(allocate(".rdata$neostr_lit_data")) static const struct                                  \
{                                                                                             \
  neostr_fixed_payload64 payload;                                                             \
  neostr_metadata metadata;                                                                   \
  char data[sizeof(cStringLiteral)];                                                          \
} literalVariableName##_memory_##__LINE__ =                                                   \
  {                                                                                           \
    {sizeof(cStringLiteral)},                                                                 \
    {sizeof(neostr_fixed_payload64), 0, 0},                                                   \
    cStringLiteral                                                                            \
  };                                                                                          \
static const char* const literalVariableName = literalVariableName##_memory_##__LINE__.data;

#define neostr_Metadata(string) ((neostr_metadata*)(string - sizeof(neostr_metadata)))
#define neostr_Payload(string, payloadType) ((payloadType*)((neostr_uByte*)neostr_Metadata(string) - neostr_Metadata(string)->payloadSize))
#define neostr_IsValid(string) \
  (string && (neostr_IsMetadataValid(string) && neostr_IsPayloadValid(string)) || neostr_IsLiteral(string))
  
#define neostr_IsPayloadValid(string) \
  (neostr_Metadata(string)->payloadCRC == neostr_CRC32((neostr_uByte*)neostr_Payload(string, void), neostr_Metadata(string)->payloadSize))

#define neostr_IsMetadataValid(string) \
  (neostr_Metadata(string)->metadataCRC == neostr_CRC32((neostr_uByte*)neostr_Metadata(string), NEOSTR_METADATA_WITHOUT_CRC_SIZE))

#define neostr_IsLiteral(string) \
  (string < &neostr_literal_data_end && string > &neostr_literal_data_start)

// NOTE: must reset CRC for payload first because the payload CRC is part of the metadata CRC data stream
#define neostr_ResetCRC(string) \
  (neostr_ResetPayloadCRC(string), neostr_ResetMetadataCRC(string))

#define neostr_ResetMetadataCRC(string) \
  (neostr_Metadata(string)->metadataCRC = neostr_CRC32((neostr_uByte*)neostr_Metadata(string), NEOSTR_METADATA_WITHOUT_CRC_SIZE))

#define neostr_ResetPayloadCRC(string) \
  (neostr_Metadata(string)->payloadCRC = neostr_CRC32((neostr_uByte*)neostr_Payload(string, void), neostr_Metadata(string)->payloadSize))

#define neostr_HeaderSize(payloadType) (sizeof(payloadType) + sizeof(neostr_metadata))
#define neostr_LayoutHeader(stringMemory, payloadType)                                                    \
  (memset(stringMemory, 0, sizeof(payloadType))),                                                         \
  (memset(stringMemory + sizeof(payloadType), 0, sizeof(neostr_metadata))),                               \
  (stringMemory = (void*)((neostr_uByte*)stringMemory + sizeof(payloadType) + sizeof(neostr_metadata))),  \
  (neostr_Metadata(stringMemory)->payloadSize = sizeof(payloadType)),                                     \
  (neostr_ResetCRC(stringMemory))
#define neostr_SetPayloadMember(string, payloadType, memberName, newMemberValue)  \
  ((neostr_Payload(string, payloadType)->memberName = newMemberValue), (neostr_ResetCRC(string)))
#define neostr_SetPayloadMember_Checked(string, payloadType, memberName, newMemberValue)  \
  (neostr_IsValid(string) ? neostr_SetPayloadMember(string, payloadType, memberName, newMemberValue) : 0)
#define neostr_GetPayloadMember(string, payloadType, memberName) \
  (neostr_Payload(string, payloadType)->memberName)
#define neostr_GetPayloadMember_Checked(string, payloadType, memberName) \
  (neostr_IsValid(string) ? neostr_GetPayloadMember(string, payloadType, memberName) : 0)
#define neostr_GetLength(string, payloadType) \
  neostr_GetPayloadMember(string, payloadType, NEOSTR_MEMBERNAME_LENGTH)
#define neostr_GetLength_Checked(string, payloadType) \
  neostr_GetPayloadMember_Checked(string, payloadType, NEOSTR_MEMBERNAME_LENGTH)
#define neostr_GetLiteralLength(string) \
  (neostr_GetPayloadMember(string, neostr_fixed_payload64, NEOSTR_MEMBERNAME_LENGTH))
#define neostr_GetLiteralLength_Checked(string) \
  (neostr_IsLiteral(string) ? neostr_GetLiteralLength(string) : 0)
#define neostr_SetLength(string, payloadType, newLength) \
  neostr_SetPayloadMember(string, payloadType, NEOSTR_MEMBERNAME_LENGTH, newLength)
#define neostr_SetLength_Checked(string, payloadType, newLength) \
  neostr_SetPayloadMember_Checked(string, payloadType, NEOSTR_MEMBERNAME_LENGTH, newLength)
#define neostr_GetCapacity(string, payloadType) \
  neostr_GetPayloadMember(string, payloadType, NEOSTR_MEMBERNAME_CAPACITY)
#define neostr_GetCapacity_Checked(string, payloadType) \
  neostr_GetPayloadMember_Checked(string, payloadType, NEOSTR_MEMBERNAME_CAPACITY)
#define neostr_SetCapacity(string, payloadType, newCapacity) \
  neostr_SetPayloadMember(string, payloadType, NEOSTR_MEMBERNAME_CAPACITY, newCapacity)
#define neostr_SetCapacity_Checked(string, payloadType, newCapacity) \
  neostr_SetPayloadMember_Checked(string, payloadType, NEOSTR_MEMBERNAME_CAPACITY, newCapacity)
#define neostr_IncrementCapacity(string, payloadType, incrementSize) \
  neostr_SetPayloadMember(string, payloadType, NEOSTR_MEMBERNAME_CAPACITY, neostr_GetPayloadMember(string, payloadType, NEOSTR_MEMBERNAME_CAPACITY) + incrementSize)
#define neostr_IncrementCapacity_Checked(string, payloadType, incrementSize) \
  neostr_IsValid(string) ? neostr_IncrementCapacity(string, payloadType, incrementSize) : 0

// Requires payload members NEOSTR_MEMBERNAME_LENGTH, NEOSTR_MEMBERNAME_CAPACITY 
#define neostr_Push(string, payloadType, data, dataSize) \
  (neostr_GetLength(string, payloadType) + dataSize <= neostr_GetCapacity(string, payloadType)) ?  \
    (memcpy(string + neostr_GetLength(string, payloadType), data, dataSize)),                     \
    (neostr_Payload(string, payloadType)->NEOSTR_MEMBERNAME_LENGTH += dataSize),                  \
    (neostr_ResetCRC(string)) : 0
#define neostr_Push_Checked(string, payloadType, data, dataSize) \
  neostr_IsValid(string) ? neostr_Push(string, payloadType, data, dataSize) : 0


// Requires payload members NEOSTR_MEMBERNAME_LENGTH
#define neostr_SetString(stringAddress, payloadType, stringData, stringSize)  \
    (memcpy(stringAddress, stringData, stringSize)),                          \
    (neostr_SetLength(stringAddress, neostr_fixed_payload64, stringSize)),    \
    (neostr_ResetCRC(stringAddress))
#define neostr_SetString_Checked(stringAddress, payloadType, stringData, stringSize)  \
  neostr_IsValid(stringAddress) ?                                                     \
    neostr_SetString(stringAddress, payloadType, stringData, stringSize) : 0

#define neostr_Compare_Sized_Checked(leftString, leftLength, rightString, rightLength)  \
  neostr_Compare_Sized(                                                                 \
    leftString ? leftString : "\0",                                                     \
    leftString ? leftLength : 0,                                                        \
    rightString ? rightString : "\0",                                                   \
    rightString ? rightLength : 0)                             
#define neostr_Compare_Checked(leftString, leftPayloadType, rightString, rightPayloadType)                            \
  neostr_Compare_Sized_Checked(                                                                                       \
    leftString,                                                                                                       \
    neostr_IsValid(leftString) ? neostr_GetLength(leftString, leftPayloadType) : neostr_CStringLength(leftString),    \
    rightString,                                                                                                      \
    neostr_IsValid(rightString) ? neostr_GetLength(rightString, rightPayloadType) : neostr_CStringLength(rightString))
#define neostr_Compare(leftString, leftPayloadType, rightString, rightPayloadType)  \
  neostr_Compare_Sized(                                                             \
    leftString,                                                                     \
    neostr_GetLength(leftString, leftPayloadType),                                  \
    rightString,                                                                    \
    neostr_GetLength(rightString, rightPayloadType))


void* malloc(neostr_uInt64 size);
void* memcpy(void* destination, const void* source, neostr_uInt64 size);
neostr_sInt32 memcmp(const void* left, const void* right, neostr_uInt64 count);
void* memset(void* dest, neostr_uByte value, neostr_uInt64 n);
neostr_uInt64 neostr_CStringLength(const char* cString);
neostr_sInt32 neostr_Compare_Sized(const char* left, neostr_uInt64 leftLength, const char* right, neostr_uInt64 rightLength);
neostr_uInt32 neostr_CRC32(neostr_uByte* memory, neostr_uInt64 memorySize);

//======================================= end neostr.h ======================================================


//======================================= start neoplat.h ======================================================

#define NEOPLAT_COMPILER_MSVC 0 
#define NEOPLAT_COMPILER_CLANG 0 
#define NEOPLAT_COMPILER_GCC 0

#define NEOPLAT_OS_WINDOWS 0
#define NEOPLAT_OS_LINUX 0
#define NEOPLAT_OS_APPLE 0


// TODO: context cracking register size if needed

#if defined (__clang__)
#undef NEOPLAT_COMPILER_CLANG
#define NEOPLAT_COMPILER_CLANG 1
#elif defined(__GNUC__) || defined(__GNUG__)
#undef NEOPLAT_COMPILER_GCC
#define NEOPLAT_COMPILER_GCC 1
#elif defined(_MSC_VER)
#undef NEOPLAT_COMPILER_MSVC
#define NEOPLAT_COMPILER_MSVC 1
#endif

#if defined(_WIN32) || defined(_WIN64)
#undef NEOPLAT_OS_WINDOWS
#define NEOPLAT_OS_WINDOWS 1
#elif defined (__linux__)
#undef NEOPLAT_OS_LINUX
#define NEOPLAT_OS_LINUX 1
#elif defined (__APPLE__)
#undef NEOPLAT_OS_APPLE
#define NEOPLAT_OS_APPLE 1
#endif

neostr_uBool8 neoplat_SupportsHardwareCRC();

#if NEOPLAT_COMPILER_MSVC
void __cpuid(neostr_sInt32 cpuInfo[4], neostr_sInt32 function_id);
neostr_uInt64 _mm_crc32_u64(neostr_uInt64 crc, neostr_uInt64 data);
neostr_uInt32 _mm_crc32_u8(neostr_uInt32 crc, neostr_uInt8 data);

#endif

//======================================= end neoplat.h ======================================================

//======================================= start neobuild.h ======================================================
// _Error sets warnings as errors
// TODO: maybe refactor into bitmask 
typedef enum neobuild_warning_level
{
  neobuild_warning_level_NONE = 0,
  neobuild_warning_level_SEVERE,
  neobuild_warning_level_IMPORTANT,
  neobuild_warning_level_VERBOSE,
  neobuild_warning_level_ALL,
  neobuild_warning_level_SEVERE_ERROR,
  neobuild_warning_level_IMPORTANT_ERROR,
  neobuild_warning_level_VERBOSE_ERROR,
  neobuild_warning_level_ALL_ERROR,
  neobuild_warning_level_ERROR_START = neobuild_warning_level_SEVERE_ERROR,
}neobuild_warning_level;

typedef enum neobuild_target_language
{
  neobuild_target_language_INVALID = 0,
  neobuild_target_language_C,
  neobuild_target_language_CPP,
  neobuild_target_language_PERTARGET // Signals to check target language setting per-target
  // TODO: maybe add option to check the file extension per-file
}neobuild_target_language;

typedef struct neobuild_string_view
{
  const char* data;
  neobuild_uInt64 length;
}neobuild_string_view;

typedef struct neobuild_compile_target
{
  neobuild_string_view path;
  neobuild_target_language language;
}neobuild_compile_target;

typedef enum neobuild_optimize_level
{
  neobuild_optimize_level_INVALID = 0,
  neobuild_optimize_level_DISABLED,
  neobuild_optimize_level_SMALL,
  neobuild_optimize_level_FAST
}neobuild_optimize_level;

typedef enum neobuild_binary_type
{
  neobuild_binary_type_INVALID = 0,
  neobuild_binary_type_DYNAMIC_LIBRARY,
  neobuild_binary_type_EXE,
  
  // Windows-specific:
  neobuild_binary_type_WIN32_CONSOLE,
  neobuild_binary_type_WIN32_WINDOW,
}neobuild_binary_type;

typedef struct neobuild_macro_definitions
{
  neobuild_string_view* definitionList;
  neobuild_uInt64 count;
}neobuild_macro_definitions;

typedef struct neobuild_compile_targets
{
  neobuild_compile_target* data;
  neobuild_uInt64 count;
}neobuild_compile_targets;

typedef struct neobuild_linker_inputs
{
  neobuild_string_view* inputFileList;
  neobuild_uInt64 count;
}neobuild_linker_inputs;

typedef struct neobuild_error
{
  neobuild_uInt32 errorCode;
  const char* errorReason;
}neobuild_error;

typedef struct neobuild_command
{
  neobuild_binary_type executableType;

  neobuild_target_language buildLanguage;
  neobuild_compile_targets compileTargets;
  neobuild_linker_inputs linkerInputs;

  // msvc-specific
  neobuild_string_view msvcEnvironment;

  char* command; // do not set manually
  
  // optional
  neobuild_macro_definitions macroDefinitions;
  
  neobuild_string_view binaryFilepath;
  neobuild_string_view intermediateFilepath;
  neobuild_string_view debugSymbolsFilepath;

  const char* customFlags; // TODO
  neobuild_warning_level warningLevel;
  neobuild_optimize_level optimizationLevel;
  neobuild_uBool8 generateDebugSymbols;

  neobuild_error error;
}neobuild_command;

neobuild_uInt64 neobuild_EscapedPathLength(const char* unescapedPath, neobuild_uInt64 unescapedPathLength);
neobuild_uInt64 neobuild_LayoutEscapedPath(const char* unescapedPath, neobuild_uInt64 unescapedPathLength, char* outEscapedPath);

// MSVC:
neobuild_uInt64 neobuild_msvc_FindCompilerPathRequiredMemory(neobuild_error* outError);
void* neobuild_msvc_FindCompilerPath(neobuild_uByte* findCompilerPathMemory, neobuild_error* outError);
neobuild_uInt64 neobuild_msvc_InitializeEnvironmentRequiredMemory(void* compilerPathHandle, neobuild_error* outError);
void* neobuild_msvc_InitializeEnvironment(void* compilerPathHandle, neobuild_uByte* initEnvCommandMemory, neobuild_error* outError);
neobuild_uInt64 neobuild_msvc_EnvironmentBlockSize(void* envBlockHandle, neobuild_error* outError);
neobuild_string_view neobuild_msvc_GetEnvironment(void* envBlockHandle, void* environmentBlockMemory, neobuild_error* outError);

#define neobuild_SetCommandMemory(commandAddress, memoryAddress)      \
  (commandAddress) ? (commandAddress)->command = (memoryAddress), 1 : 0  

#define neobuild_SetMSVCEnvironment(commandAddress, msvcEnvironmentBlock)           \
  (commandAddress) ? (commandAddress)->msvcEnvironment = (msvcEnvironmentBlock), 1 : 0 


//======================================= end neobuild.h ======================================================

#if defined NEOBUILD_IMPLEMENTATION

//======================================= start NEOSTR_IMPLEMENTATION ======================================================
neostr_uInt32 neostr_CRC32(neostr_uByte* memory, neostr_uInt64 memorySize)
{
  neostr_uInt32 crc = 0;
  if (neoplat_SupportsHardwareCRC())
  {
    for (neostr_uInt64 i = 0; i < memorySize;)
    {
      if (memorySize - i > 7)
      {
        crc = (neostr_uInt32)_mm_crc32_u64(crc, (neostr_uInt64)memory[i]); // NOTE: downcasting to uInt32 is safe because function only returns lower bits
        i += 8;
      }
      else
      {
        crc = _mm_crc32_u8(crc, (neostr_uInt8)memory[i]);
        ++i;
      }
    }
  } 
  else
  {
    // TODO: write a software CRC function
  }

  return crc;
}

// Max uInt64 value
#define NEOSTR_CSTRING_MAX_LENGTH ~((neostr_uInt64)0)
neostr_uInt64 neostr_CStringLength(const char* cString)
{
  cString = cString ? cString : "\0";
  neostr_uInt64 length;
  for (length = 0; length < NEOSTR_CSTRING_MAX_LENGTH; ++length)
  {
    if (*cString == '\0')
    {
      break;
    }
    cString++;
  }

  return length;
}

neostr_sInt32 neostr_Compare_Sized(const char* left, neostr_uInt64 leftLength, const char* right, neostr_uInt64 rightLength)
{
  neostr_uInt64 smallerLength = leftLength < rightLength ? leftLength : rightLength;

  neostr_sInt32 textCompare = memcmp(left, right, smallerLength);
  if (textCompare == 0 && leftLength != rightLength)
  {
    // one of the strings is longer
    textCompare = leftLength != smallerLength ? left[smallerLength] : -right[smallerLength];    
  }
  return textCompare;
}
//======================================= end NEOSTR_IMPLEMENTATION ======================================================
//======================================= start NEOPLAT_IMPLEMENTATION ======================================================
#if NEOPLAT_COMPILER_MSVC
neostr_uBool8 neoplat_SupportsHardwareCRC()
{
  neostr_sInt32 info[4];
  __cpuid(info, 1);
  return (info[2] & (1 << 20)) != 0;
}
#endif
//======================================= end NEOPLAT_IMPLEMENTATION ======================================================

//======================================= start NEOBUILD_IMPLEMENTATION ======================================================

// Win32 API START:
typedef struct neobuild_win32_security_attributes {
  neobuild_uInt32 length;
  void* securityDescriptor;
  neobuild_sInt32 inheritHandle;
} neobuild_win32_security_attributes;

typedef struct neobuild_win32_startup_info {
  neobuild_uInt32  structSizeBytes;
  char*  reserved;
  char*  desktop;
  char*  title;
  neobuild_uInt32  x;
  neobuild_uInt32  y;
  neobuild_uInt32  xSize;
  neobuild_uInt32  ySize;
  neobuild_uInt32  xCountChars;
  neobuild_uInt32  yCountChars;
  neobuild_uInt32  fillAttribute;
  neobuild_uInt32  flags;
  neobuild_uInt16  showWindow;
  neobuild_uInt16  reserved2Size;
  neobuild_uByte*  reserved2;
  void* stdInputHandle;
  void* stdOutputHandle;
  void* stdErrorHandle;
} neobuild_win32_startup_info;

typedef struct neobuild_win32_process_info {
  void* processHandle;
  void* threadHandle;
  neobuild_uInt32  proccessId;
  neobuild_uInt32  threadId;
} neobuild_win32_process_info;

typedef union neobuild_win32_large_int {
  struct {
    neobuild_uInt32 lowPart;
    neobuild_sInt32  highPart;
  } DUMMYSTRUCTNAME;
  struct {
    neobuild_uInt32 lowPart;
    neobuild_sInt32  highPart;
  } u;
  neobuild_sInt64 quadPart;
} neobuild_win32_large_int;

typedef struct neobuild_win32_filetime
{
  neobuild_uInt32 lowDateTime;
  neobuild_uInt32 highDateTime;
} neobuild_win32_filetime;

typedef enum neobuild_win32_file_info_level 
{
  neobuild_win32_getFileExInfoStandard,
  neobuild_win32_getFileExMaxInfoLevel
} neobuild_win32_file_info_level;

typedef struct neobuild_win32_file_attribute_data 
{
  neobuild_uInt32    dwFileAttributes;
  neobuild_win32_filetime ftCreationTime;
  neobuild_win32_filetime ftLastAccessTime;
  neobuild_win32_filetime ftLastWriteTime;
  neobuild_uInt32    nFileSizeHigh;
  neobuild_uInt32    nFileSizeLow;
} neobuild_win32_file_attribute_data;

typedef struct neobuild_win32_overlapped_info {
  neobuild_uInt64 reserved;
  neobuild_uInt64 reservedHighBits;
  union {
    struct {
      neobuild_uInt32 fileOffset;
      neobuild_uInt32 fileOffsetHigh;
    } offsetStruct;
    void* reservedPointer;
  } overlapInfo;
  void* reservedHandle;
} neobuild_win32_overlapped_info;

#define NEOBUILD_WIN32_FLAG_USESTDHANDLES 0x00000100
#define NEOBUILD_WIN32_ACCESS_GENERIC_WRITE 0x40000000
#define NEOBUILD_WIN32_ACCESS_GENERIC_READ 0x80000000
#define NEOBUILD_WIN32_FILE_SHARE_READ 0x00000001
#define NEOBUILD_WIN32_FILE_SHARE_WRITE 0x00000002
#define NEOBUILD_WIN32_FILE_CREATE_ALWAYS 0x02
#define NEOBUILD_WIN32_FILE_OPEN_EXISTING 0x03
#define NEOBUILD_WIN32_FILE_ATTRIBUTE_NORMAL 0x80
#define NEOBUILD_WIN32_FILE_ATTRIBUTE_HIDDEN 0x2
#define NEOBUILD_WIN32_FILE_ATTRIBUTE_TEMPORARY 0x100
#define NEOBUILD_WIN32_FILE_FLAG_DELETE_ON_CLOSE 0x04000000
#define NEOBUILD_WIN32_CREATE_DETACHED_PROCESS 0x00000008
#define NEOBUILD_WIN32_FILE_BEGIN 0x0
#define NEOBUILD_WIN32_INVALID_HANDLE_VALUE ((void*)(neobuild_sInt64)-1)
#define NEOBUILD_WIN32_ERROR_ACCESS_DENIED 0x5
#define NEOBUILD_WIN32_WAIT_INFINTE 0xFFFFFFFF
#define NEOBUILD_WIN32_STD_INPUT_HANDLE ((neobuild_uInt32)-10)
#define NEOBUILD_WIN32_STD_OUTPUT_HANDLE ((neobuild_uInt32)-11)
#define NEOBUILD_WIN32_STD_ERROR_HANDLE ((neobuild_uInt32)-12)
#define NEOBUILD_WIN32_INSUFFICIENT_BUFFER_SIZE_ERROR 0x7A 

neobuild_uInt32 GetLastError();
neobuild_sInt32 CreateProcessA(const char* applicationName, char* commandLine, neobuild_win32_security_attributes* processAttributes, neobuild_win32_security_attributes* threadAttributes, neobuild_sInt32 inheritHandles,
                          neobuild_uInt32 creationFlags, void* environmentBlock, const char* currentDirectory, neobuild_win32_startup_info* startupInfo, neobuild_win32_process_info* processInformation);
neobuild_uBool8 GetExitCodeProcess(void*  processHandle, neobuild_uInt32* outExitCode);
void ExitProcess(neobuild_uInt32 exitCode);
neobuild_uInt32 WaitForSingleObject(void* handle, neobuild_uInt32 milliseconds);
void* GetStdHandle(neobuild_uInt32 stdHandle);
neobuild_uInt32 GetFinalPathNameByHandleA(void* fileHandle, char* outFilePath, neobuild_uInt32 filePathLength, neobuild_uInt32 flags);
void* GetModuleHandleA(const char* moduleName);
void* GetProcAddress(void* moduleHandle, const char* functionName);
void* GetCurrentProcess();
void* CreateFileA(const char* fileName, neobuild_uInt32 desiredAccess, neobuild_uInt32 shareMode, neobuild_win32_security_attributes* securityAttributes, neobuild_uInt32 creationDisposition, 
                  neobuild_uInt32 flagsAndAttributes, void* templateFile);
neobuild_sInt32 ReadFile(void* fileHandle, void* buffer, neobuild_uInt32 numberOfBytesToRead, neobuild_uInt32* numberOfBytesRead, neobuild_win32_overlapped_info* overlapped);
neobuild_uBool8 WriteFile(void* fileHandle, void* buffer, neobuild_uInt32 nNumberOfBytesToWrite, neobuild_uInt32* numberOfBytesWritten, neobuild_win32_overlapped_info* overlapped);
neobuild_uInt32 SetFilePointer(void* fileHandle, neobuild_sInt32 distanceToMove, neobuild_sInt32* distanceToMoveHigh, neobuild_uInt32 moveMethod);
neobuild_uBool8 SetEndOfFile(void* fileHandle);
neobuild_uBool8 MoveFileA(const char* existingFileName, const char* newFileName);
neobuild_uBool8 DeleteFileA(const char* fileName);
neobuild_uBool8 GetFileSizeEx(void* file, neobuild_win32_large_int* fileSize);
neobuild_uBool8 GetFileAttributesExA(const char* fileName, neobuild_win32_file_info_level infoLevelID, void* outFileInfo);
neobuild_uBool8 CreateDirectoryA(const char* directoryName, neobuild_win32_security_attributes* securityAttributes);
neobuild_uBool8 GetFileTime(void* file, neobuild_win32_filetime* creationTime, neobuild_win32_filetime* lastAccessTime, neobuild_win32_filetime* lastWriteTime);
neobuild_sInt32 CompareFileTime(const neobuild_win32_filetime* fileTime1, const neobuild_win32_filetime* fileTime2);
neobuild_uBool8 CloseHandle(void* object);
neobuild_uBool8 SetCurrentDirectoryA(const char* path);
neobuild_uInt32 GetCurrentDirectoryA(neobuild_uInt32 directoryPathLength, char* outDirectoryPath);
neobuild_uInt32 GetEnvironmentVariableA(const char* name, char* buffer, neobuild_uInt32 size);
neobuild_uBool8 SetEnvironmentVariableA(const char* name, const char* value);
neobuild_sInt32 _get_pgmptr(char** outExecutableFilepath);
// Win32 API END

// MSVC Environment Constants:

neostr_Literal(appDataEnvironmentVariable, "LOCALAPPDATA");
neostr_Literal(neobuildAppDirectoryRelativePath, "\\neobuild");
neostr_Literal(programFilesx86EnvVar, "ProgramFiles(x86)");
neostr_Literal(vswhereCommand, "\\Microsoft Visual Studio\\Installer\\vswhere.exe\" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath");
neostr_Literal(msvcToolchainCommandPrefix, "cmd.exe /c \"");
neostr_Literal(msvcToolchainSetupRelativePath, "\\VC\\Auxiliary\\Build\\vcvarsall.bat");
neostr_Literal(msvcToolchainCommandSuffix, "\" x64 >NUL && set");
neostr_Literal(msvcEnvFilename, "msvc.env");
neostr_Literal(foundToolchainTempFilename, "vswhere_out.tmp");
neostr_Literal(pathString1, "PATH=");
neostr_Literal(pathString2, "Path=");
neostr_Literal(pathString3, "path=");

// Error Reason Constants:
neostr_Literal(failedToGetEnvironmentVariable, "Failed to retrieve environment variable. Windows error code: %u\n");
neostr_Literal(failedToCreateAppDirectory, "Failed to create local appdata directory. Windows error code: %u\n");
neostr_Literal(vswhereProcessFailed, "vswhere.exe process failed with error code: %u\n");
neostr_Literal(win32CreateProcessFailed, "Trying to create new process failed with Windows error code: %u\n");
neostr_Literal(win32FailedToCreateFile, "Unable to create file. Windows error code: %u\n");
neostr_Literal(win32GetFileSizeFailed, "Failed to get the size of the file. Windows error code: %u\n");
neostr_Literal(win32WriteFileFailed, "Failed to write file. Win32 error code: %u\n");
neostr_Literal(win32ReadFileFailed, "Failed to read from file. Win32 error code: %u\n");
neostr_Literal(win32OpenFileFailed, "Failed to open file. Win32 error code: %u\n");
neostr_Literal(initMSVCEnvFailed, "MSVC environment initialization process failed with exit code: %u\n");
neostr_Literal(invalidEnvBlockParameterFailureReason, "The parameter \"outMSVCEnv\" expects a block of allocated memory with a size greater than or equal to the size returned by neobuild_msvc_EnvironmentBlockSize\n");
neostr_Literal(invalidBuildCommandParameterFailureReason, "The parameter \"buildCommand\" expects a valid pointer to a neobuild_command\n");
neostr_Literal(buildProcessFailReason, "Build process failed with error code: %u\n");
neostr_Literal(win32RenameFailed, "Failed to rename file. Windows system error code: %u\n");
neostr_Literal(failedToRetrieveExecutablePath, "Failed to retrieve current executable filepath from _get_pgmptr. System error code: %u\n");

neobuild_uInt64 neobuild_EscapedPathLength(const char* unescapedPath, neobuild_uInt64 unescapedPathLength)
{
  neobuild_uInt64 backslashCount = 0;
  for (neobuild_uInt64 i = 0; i < unescapedPathLength; ++i)
  {
    if (unescapedPath[i] == '\\')
    {
      backslashCount++; 
    }
  }
  return unescapedPathLength + backslashCount;
}

neobuild_uInt64 neobuild_LayoutEscapedPath(const char* unescapedPath, neobuild_uInt64 unescapedPathLength, char* outEscapedPath)
{
  neobuild_uInt64 backslashCount = 0;
  neobuild_uInt64 prevBackslashIndex = 0;
  for (neobuild_uInt64 i = 0; i < unescapedPathLength; ++i)
  {
    if (unescapedPath[i] == '\\')
    {
      // prevBackslashIndex == number of characters from unescapedPath that have already been copied
      // backslashCount == number of additional backslashes that have already been added to outEscapedPath
      char* copyLocation = outEscapedPath + prevBackslashIndex + backslashCount;
      neobuild_uInt64 copyLength = i - prevBackslashIndex;
      memcpy(copyLocation, unescapedPath + prevBackslashIndex, copyLength);
      memcpy(copyLocation + copyLength, "\\", 1);
      prevBackslashIndex = i;
      ++backslashCount;
    }
  }

  // Copy from the final backslash to the end of the path
  char* copyLocation = outEscapedPath + prevBackslashIndex + backslashCount;
  memcpy(copyLocation, unescapedPath + prevBackslashIndex, unescapedPathLength - prevBackslashIndex);
  return unescapedPathLength + backslashCount;
}

neobuild_uInt64 neobuild_msvc_GetAppDirectory(char* outAppDirectory, neobuild_error* outError)
{
  neobuild_error error = {0};
  outError = outError ? outError : &error;
  *outError = error;

  neobuild_uInt64 requiredMemory = 0;
#if NEOPLAT_COMPILER_MSVC
  neobuild_uInt64 environmentVariableLength = GetEnvironmentVariableA(appDataEnvironmentVariable, NULL, 0);
  environmentVariableLength -= 1; // decrementing to write over env var null-terminator
  neobuild_uInt64 appDataPathLength = neostr_GetLiteralLength(neobuildAppDirectoryRelativePath);

  requiredMemory += environmentVariableLength;
  requiredMemory += appDataPathLength;
  if (outAppDirectory)
  {
    GetEnvironmentVariableA(appDataEnvironmentVariable, outAppDirectory, environmentVariableLength + 1);
    memcpy(outAppDirectory + environmentVariableLength, neobuildAppDirectoryRelativePath, appDataPathLength);

    // Create the %LOCALAPPDIR%\neobuild directory if it doesn't already exist
    neobuild_win32_file_attribute_data dirAttributeData = {0};
    if (!GetFileAttributesExA(outAppDirectory, neobuild_win32_getFileExInfoStandard, &dirAttributeData))
    {
      // NOTE: this only works in a single call because all env files are currently saved only one subfolder below LOCALAPPDIR
      neobuild_win32_security_attributes attributes = {0};
      attributes.length = sizeof(attributes);
      if (!CreateDirectoryA(outAppDirectory, &attributes))
      {
        outError->errorCode = GetLastError();
        outError->errorReason = failedToCreateAppDirectory;
      }
    }
  }
#endif  
  return outError->errorCode == 0 ? requiredMemory : 0;
}

neobuild_uInt64 neobuild_msvc_AppDirectoryRequiredMemory(neobuild_error* outError)
{
  return neobuild_msvc_GetAppDirectory(NULL, outError);
}

// TODO: Add options for selecting a specific MSVC toolchain version rather than just latest
neobuild_uInt64 neobuild_msvc_FindCompilerPathRequiredMemory(neobuild_error* outError)
{
  neobuild_error error = {0};
  outError = outError ? outError : &error;
  *outError = error;

  neobuild_uInt64 requiredMemory = 0;
#if NEOPLAT_COMPILER_MSVC
  neobuild_uInt64 findToolchainCommandSize = GetEnvironmentVariableA(programFilesx86EnvVar, NULL, 0) + neostr_GetLiteralLength(vswhereCommand) + 2; // null-terminator + open quotation mark
  neobuild_uInt64 maxAppDirPathSize = neobuild_msvc_AppDirectoryRequiredMemory(outError);
  maxAppDirPathSize += neostr_GetLiteralLength(msvcEnvFilename) > neostr_GetLiteralLength(foundToolchainTempFilename) ?
                    neostr_GetLiteralLength(msvcEnvFilename) : neostr_GetLiteralLength(foundToolchainTempFilename);
  maxAppDirPathSize += 1;
  requiredMemory = maxAppDirPathSize > findToolchainCommandSize ? maxAppDirPathSize : findToolchainCommandSize; 
#endif
  return requiredMemory;
}

void* neobuild_msvc_FindCompilerPath(neobuild_uByte* findCompilerPathMemory, neobuild_error* outError)
{
  neobuild_error error = {0};
  outError = outError ? outError : &error;
  *outError = error;

  void* vswhereOutputHandle = NULL;
  char* appDirectory = findCompilerPathMemory;
#if NEOPLAT_COMPILER_MSVC
  if (neobuild_msvc_GetAppDirectory(appDirectory, outError))
  {
    neobuild_uInt64 appDirectoryLength = neobuild_msvc_AppDirectoryRequiredMemory(outError);
    appDirectory[appDirectoryLength - 1] = '\\';
    
    memcpy(appDirectory + appDirectoryLength, msvcEnvFilename, neostr_GetLiteralLength(msvcEnvFilename));
    appDirectoryLength += neostr_GetLiteralLength(msvcEnvFilename);

    memcpy(appDirectory + appDirectoryLength, "\0", 1);
    appDirectoryLength += 1;

    neobuild_win32_file_attribute_data fileAttributeData = {0};
    if (!GetFileAttributesExA(appDirectory, neobuild_win32_getFileExInfoStandard, &fileAttributeData))
    {
      while (appDirectory[appDirectoryLength - 1] != '\\')
      {
        appDirectoryLength--;
      }

      memcpy(appDirectory + appDirectoryLength, foundToolchainTempFilename, neostr_GetLiteralLength(foundToolchainTempFilename));
      appDirectoryLength += neostr_GetLiteralLength(foundToolchainTempFilename);

      memcpy(appDirectory + appDirectoryLength, "\0", 1);
      appDirectoryLength += 1;

      neobuild_win32_security_attributes attributes = {0};
      attributes.length = sizeof(attributes);
      attributes.securityDescriptor = NULL;
      attributes.inheritHandle = TRUE;
      vswhereOutputHandle = 
        CreateFileA(appDirectory, NEOBUILD_WIN32_ACCESS_GENERIC_WRITE | NEOBUILD_WIN32_ACCESS_GENERIC_READ,
                    NEOBUILD_WIN32_FILE_SHARE_WRITE | NEOBUILD_WIN32_FILE_SHARE_READ, &attributes, NEOBUILD_WIN32_FILE_CREATE_ALWAYS,
                    NEOBUILD_WIN32_FILE_ATTRIBUTE_TEMPORARY | NEOBUILD_WIN32_FILE_ATTRIBUTE_HIDDEN | NEOBUILD_WIN32_FILE_FLAG_DELETE_ON_CLOSE, NULL);
      if (vswhereOutputHandle)
      {
        // Build the vswhere command
        char* findCompilerCommand = appDirectory; // overwriting appdata path now
        memcpy(findCompilerCommand, "\"", 1); // vswhere.exe path needs to be surrounded by quotes since this is going through the CMD shell
        neobuild_uInt64 findCompilerCommandLength = 1;

        neobuild_uInt32 programFilesx86PathLength = GetEnvironmentVariableA(programFilesx86EnvVar, NULL, 0);
        GetEnvironmentVariableA(programFilesx86EnvVar, findCompilerCommand + findCompilerCommandLength, programFilesx86PathLength);
        findCompilerCommandLength += --programFilesx86PathLength; // writing over env var null-terminator

        memcpy(findCompilerCommand + findCompilerCommandLength, vswhereCommand, neostr_GetLiteralLength(vswhereCommand));
        findCompilerCommandLength += neostr_GetLiteralLength(vswhereCommand);

        memcpy(findCompilerCommand + findCompilerCommandLength, "\0", 1);
        findCompilerCommandLength += 1;
       

        neobuild_win32_process_info vswhereProcess = {0};
        neobuild_win32_startup_info vswhereStartupInfo = {0};
        vswhereStartupInfo.structSizeBytes = sizeof(vswhereStartupInfo);
        vswhereStartupInfo.flags = NEOBUILD_WIN32_FLAG_USESTDHANDLES;
        vswhereStartupInfo.stdInputHandle = GetStdHandle(NEOBUILD_WIN32_STD_INPUT_HANDLE);
        vswhereStartupInfo.stdOutputHandle = vswhereOutputHandle;
        vswhereStartupInfo.stdErrorHandle = GetStdHandle(NEOBUILD_WIN32_STD_ERROR_HANDLE);

        if (CreateProcessA(0, findCompilerCommand, 0, 0, TRUE, 0, NULL, NULL, &vswhereStartupInfo, &vswhereProcess))
        {
          WaitForSingleObject(vswhereProcess.processHandle, NEOBUILD_WIN32_WAIT_INFINTE);
          
          GetExitCodeProcess(vswhereProcess.processHandle, &outError->errorCode);
          outError->errorReason = outError->errorCode != 0 ? vswhereProcessFailed : NULL;

          CloseHandle(vswhereProcess.processHandle);
          CloseHandle(vswhereProcess.threadHandle);
        }
        else
        {
          outError->errorCode = GetLastError();
          outError->errorReason = win32CreateProcessFailed;
        }

        // If finding the toolchain failed, close the file handle
        if (outError->errorCode != 0)
        {
          CloseHandle(vswhereOutputHandle);
          vswhereOutputHandle = NULL;
        } 
      }
      else
      {
        outError->errorCode = GetLastError();
        outError->errorReason = win32FailedToCreateFile;
      }
    }
  }
#endif
  
  return vswhereOutputHandle;
}

neobuild_uInt64 neobuild_msvc_InitializeEnvironmentRequiredMemory(void* compilerPathHandle, neobuild_error* outError)
{
  neobuild_error error = {0};
  outError = outError ? outError : &error;
  *outError = error;

  neobuild_uInt64 requiredMemory = 0;
#if NEOPLAT_COMPILER_MSVC
  neobuild_win32_large_int fileSize = {0};
  if (GetFileSizeEx(compilerPathHandle, &fileSize))
  {
    requiredMemory = neostr_GetLiteralLength(msvcToolchainCommandPrefix) + fileSize.quadPart + 
                            neostr_GetLiteralLength(msvcToolchainSetupRelativePath) + neostr_GetLiteralLength(msvcToolchainCommandSuffix) + 1; 
  }
  else if (compilerPathHandle) // It is valid for compilerPathHandle parameter to be null but if it wasn't then log the error
  {
    outError->errorCode = GetLastError();
    outError->errorReason = win32GetFileSizeFailed;
  }

  requiredMemory += neobuild_msvc_AppDirectoryRequiredMemory(outError);
  requiredMemory += neostr_GetLiteralLength(msvcEnvFilename);
#endif
  return requiredMemory;
}

// Returns environment handle
void* neobuild_msvc_InitializeEnvironment(void* compilerPathHandle, neobuild_uByte* initEnvCommandMemory, neobuild_error* outError)
{
  neobuild_error error = {0};
  outError = outError ? outError : &error;
  *outError = error;

  void* envBlockFileHandle = NULL;
  char* msvcEnvFilepath = initEnvCommandMemory;
#if NEOPLAT_COMPILER_MSVC
  if (neobuild_msvc_GetAppDirectory(msvcEnvFilepath, outError))
  {
    neobuild_uInt64 msvcEnvFilepathLength = neobuild_msvc_AppDirectoryRequiredMemory(outError);
    msvcEnvFilepath[msvcEnvFilepathLength - 1] = '\\';
    
    memcpy(msvcEnvFilepath + msvcEnvFilepathLength, msvcEnvFilename, neostr_GetLiteralLength(msvcEnvFilename));
    msvcEnvFilepathLength += neostr_GetLiteralLength(msvcEnvFilename);

    neobuild_win32_file_attribute_data fileAttributeData = {0};
    if (!GetFileAttributesExA(msvcEnvFilepath, neobuild_win32_getFileExInfoStandard, &fileAttributeData))
    {
      char* initEnvCommand = msvcEnvFilepath + msvcEnvFilepathLength;
      
      neobuild_uInt64 commandLength = 0;
      neobuild_uInt64 substringLength = neostr_GetLiteralLength(msvcToolchainCommandPrefix) - 1;
      memcpy(initEnvCommand + commandLength, msvcToolchainCommandPrefix, substringLength);
      commandLength += substringLength;

      neobuild_win32_large_int fileSize = {0};
      SetFilePointer(compilerPathHandle, 0, NULL, NEOBUILD_WIN32_FILE_BEGIN);
      GetFileSizeEx(compilerPathHandle, &fileSize);
      substringLength = fileSize.quadPart;

      neobuild_uInt32 readBytes = 0;
      if (ReadFile(compilerPathHandle, initEnvCommand + commandLength, (neobuild_uInt32)substringLength, &readBytes, NULL))
      {
        // If the find toolchain output file is greater than uint32, copy the rest
        neobuild_uInt32 compilerPathFileSizeUpperBytes = substringLength >> 32;
        if (compilerPathFileSizeUpperBytes == 0 || ReadFile(compilerPathHandle, initEnvCommand + commandLength + (neobuild_uInt32)substringLength, compilerPathFileSizeUpperBytes, &readBytes, NULL))
        {
          CloseHandle(compilerPathHandle); // This deletes the find toolchain output file
          commandLength += substringLength - 2; // Path returned by vswhere ends with "\r\n"

          substringLength = neostr_GetLiteralLength(msvcToolchainSetupRelativePath) - 1;
          memcpy(initEnvCommand + commandLength, msvcToolchainSetupRelativePath, substringLength);
          commandLength += substringLength;

          substringLength = neostr_GetLiteralLength(msvcToolchainCommandSuffix);
          memcpy(initEnvCommand + commandLength, msvcToolchainCommandSuffix, substringLength);
          commandLength += substringLength;

          neobuild_win32_security_attributes attributes = {0};
          attributes.length = sizeof(attributes);
          attributes.securityDescriptor = NULL;
          attributes.inheritHandle = TRUE;
          envBlockFileHandle = CreateFileA(msvcEnvFilepath, NEOBUILD_WIN32_ACCESS_GENERIC_WRITE | NEOBUILD_WIN32_ACCESS_GENERIC_READ, 
                                  NEOBUILD_WIN32_FILE_SHARE_WRITE | NEOBUILD_WIN32_FILE_SHARE_READ, &attributes,
                                  NEOBUILD_WIN32_FILE_CREATE_ALWAYS, NEOBUILD_WIN32_FILE_ATTRIBUTE_HIDDEN, NULL);
          envBlockFileHandle = envBlockFileHandle != NEOBUILD_WIN32_INVALID_HANDLE_VALUE ? envBlockFileHandle : NULL;
          if (envBlockFileHandle)
          {
            neobuild_uInt32 processExitCode = 0;
            neobuild_uInt32 createProcessError = 0;
            neobuild_win32_process_info initEnvProcess = {0};
            neobuild_win32_startup_info initEnvStartupInfo = {0};
            initEnvStartupInfo.structSizeBytes = sizeof(initEnvStartupInfo);
            initEnvStartupInfo.flags = NEOBUILD_WIN32_FLAG_USESTDHANDLES;
            initEnvStartupInfo.stdInputHandle = GetStdHandle(NEOBUILD_WIN32_STD_INPUT_HANDLE);
            initEnvStartupInfo.stdOutputHandle = envBlockFileHandle;
            initEnvStartupInfo.stdErrorHandle = GetStdHandle(NEOBUILD_WIN32_STD_ERROR_HANDLE);
            if (CreateProcessA(NULL, initEnvCommand, NULL, NULL, TRUE, 0, NULL, NULL, &initEnvStartupInfo, &initEnvProcess))
            {
              WaitForSingleObject(initEnvProcess.processHandle, NEOBUILD_WIN32_WAIT_INFINTE);
              GetExitCodeProcess(initEnvProcess.processHandle, &processExitCode);
              CloseHandle(initEnvProcess.processHandle);
              CloseHandle(initEnvProcess.threadHandle);

              if (processExitCode == 0)
              {
                neobuild_win32_large_int envFileSize = {0};
                if (!GetFileSizeEx(envBlockFileHandle, &envFileSize))
                {
                  outError->errorCode = GetLastError();
                  outError->errorReason = win32GetFileSizeFailed; 
                }
                SetFilePointer(envBlockFileHandle, 0, NULL, NEOBUILD_WIN32_FILE_BEGIN);
                
#define NEOBUILD_CACHELINE_SIZE 64 // TODO: Get the user's processor's actual cache line size
                neobuild_uByte readBuffer[NEOBUILD_CACHELINE_SIZE]; 
                neobuild_uInt32 fileWriteIndex = 0;
                neobuild_uInt64 totalBytesRead = 0;
                while (totalBytesRead < envFileSize.quadPart && outError->errorCode == 0)
                {
                  neobuild_uInt32 bytesRead = 0;
                  neobuild_win32_overlapped_info readWindowInfo = {0};
                  readWindowInfo.overlapInfo.offsetStruct.fileOffset = totalBytesRead;
                  if (ReadFile(envBlockFileHandle, &readBuffer, NEOBUILD_CACHELINE_SIZE, &bytesRead, &readWindowInfo))
                  {
                    neobuild_uInt32 writeFileError = 0;
                    neobuild_uInt32 bufferReadStartIndex = 0;
                    for (neobuild_uInt32 bufferIndex = 0; bufferIndex < bytesRead; ++bufferIndex)
                    {
                      neobuild_uByte currentByte = readBuffer[bufferIndex];
                      if (currentByte == '\r')
                      {
                        readBuffer[bufferIndex] = '\0';
                        neobuild_uInt32 writeSize = bufferIndex - bufferReadStartIndex + 1;
                        neobuild_win32_overlapped_info writeWindowInfo = {0};
                        writeWindowInfo.overlapInfo.offsetStruct.fileOffset = fileWriteIndex;

                        neobuild_uInt32 bytesWritten = 0;
                        if (WriteFile(envBlockFileHandle, &readBuffer[bufferReadStartIndex], writeSize, &bytesWritten, &writeWindowInfo))
                        {
                          fileWriteIndex += writeSize; // Move forward
                          // Move past the subsequent '\n' and set the next read start to the character after '\n'
                          bufferReadStartIndex = ++bufferIndex + 1; 
                        }
                        else
                        {
                          outError->errorCode = GetLastError();
                          outError->errorReason = win32WriteFileFailed;
                          break;
                        }
                      }
                    }

                    // There are remaining bytes in the buffer to be copied before next read
                    if (outError->errorCode == 0 && bufferReadStartIndex < bytesRead) 
                    {
                      neobuild_uInt32 writeSize = bytesRead - bufferReadStartIndex;
                      
                      neobuild_uInt32 bytesWritten = 0;
                      neobuild_win32_overlapped_info writeWindowInfo = {0};
                      writeWindowInfo.overlapInfo.offsetStruct.fileOffset = fileWriteIndex;
                      if (WriteFile(envBlockFileHandle, &readBuffer[bufferReadStartIndex], writeSize, &bytesWritten, &writeWindowInfo))
                      {
                        fileWriteIndex += writeSize; // Move forward
                      }
                      else
                      {
                        outError->errorCode = GetLastError();
                        outError->errorReason = win32WriteFileFailed;
                      }
                    }
                    
                    totalBytesRead += bytesRead;
                  }
                  else
                  {
                    outError->errorCode = GetLastError();
                    outError->errorReason = win32ReadFileFailed;
                  } 
                }

                neobuild_uInt32 bytesWritten = 0;
                neobuild_win32_overlapped_info writeWindowInfo = {0};
                writeWindowInfo.overlapInfo.offsetStruct.fileOffset = fileWriteIndex;
                if (WriteFile(envBlockFileHandle, "\0", 1, &bytesWritten, &writeWindowInfo))
                {
                  SetEndOfFile(envBlockFileHandle);
                  SetFilePointer(envBlockFileHandle, 0, NULL, NEOBUILD_WIN32_FILE_BEGIN);
                }
                else if (outError->errorCode == 0)
                {
                  outError->errorCode = GetLastError();
                  outError->errorReason = win32WriteFileFailed;
                }
              }
              else
              {
                outError->errorCode = processExitCode;
                outError->errorReason = initMSVCEnvFailed;
              }
            }
            else
            {
              outError->errorCode = GetLastError();
              outError->errorReason = win32CreateProcessFailed;
            }
          }
          else
          {
            outError->errorCode = GetLastError();
            outError->errorReason = win32FailedToCreateFile;
          }
        }
        else
        {
          outError->errorCode = GetLastError();
          outError->errorReason = win32ReadFileFailed;
        }
      }
      else
      {
        outError->errorCode = GetLastError();
        outError->errorReason = win32ReadFileFailed;
      }
    }
    else // Environment file has already been created
    {
      neobuild_win32_security_attributes attributes = {0}; 
      attributes.length = sizeof(neobuild_win32_security_attributes);
      attributes.securityDescriptor = NULL;
      attributes.inheritHandle = FALSE;
      envBlockFileHandle = CreateFileA(msvcEnvFilepath, NEOBUILD_WIN32_ACCESS_GENERIC_READ, NEOBUILD_WIN32_FILE_SHARE_READ,
                                        &attributes, NEOBUILD_WIN32_FILE_OPEN_EXISTING, 0, NULL);
      envBlockFileHandle = envBlockFileHandle != NEOBUILD_WIN32_INVALID_HANDLE_VALUE ? envBlockFileHandle : NULL;
      if (!envBlockFileHandle)
      {
        outError->errorCode = GetLastError();
        outError->errorReason = win32OpenFileFailed;
      }
    }
  }
#endif

  return outError->errorCode == 0 ? envBlockFileHandle : NULL;
}

neobuild_uInt64 neobuild_msvc_EnvironmentBlockSize(void* envBlockHandle, neobuild_error* outError)
{
  neobuild_error error = {0};
  outError = outError ? outError : &error;
  *outError = error;

  neobuild_uInt64 envBlockSize = 0;
#if NEOPLAT_COMPILER_MSVC
  neobuild_win32_large_int fileSize = {0};
  if (GetFileSizeEx(envBlockHandle, &fileSize))
  {
    envBlockSize = fileSize.quadPart;   
  }
  else
  {
    outError->errorCode = GetLastError();
    outError->errorReason = win32GetFileSizeFailed;
  }
#endif
  return envBlockSize;
}

neobuild_string_view neobuild_msvc_GetEnvironment(void* envBlockHandle, void* environmentBlockMemory, neobuild_error* outError)
{
  neobuild_error error = {0};
  outError = outError ? outError : &error;
  *outError = error;

  neobuild_uInt32 bytesRead = 0;
  neobuild_string_view msvcEnvironment = {0};
#if NEOPLAT_COMPILER_MSVC
  if (environmentBlockMemory)
  {
    neobuild_uInt64 environmentBlockSize = neobuild_msvc_EnvironmentBlockSize(envBlockHandle, outError);
    if (ReadFile(envBlockHandle, environmentBlockMemory, (neobuild_uInt32)environmentBlockSize, &bytesRead, 0))
    {
      outError->errorCode = GetLastError();

      neobuild_uInt32 envBlockSizeUpperBytes = environmentBlockSize >> 32;
      if (envBlockSizeUpperBytes == 0 || ReadFile(envBlockHandle, (neobuild_uByte*)environmentBlockMemory + bytesRead, envBlockSizeUpperBytes, &bytesRead, 0))
      {
        msvcEnvironment.data = environmentBlockMemory;
        msvcEnvironment.length = environmentBlockSize;
        CloseHandle(envBlockHandle);
      }
      else
      {
        outError->errorCode = GetLastError();
        outError->errorReason = win32ReadFileFailed;
      }
    }
    else
    {
      outError->errorCode = GetLastError();
      outError->errorReason = win32ReadFileFailed;
    }
  }
  else
  {
    outError->errorCode = 404;
    outError->errorReason = invalidEnvBlockParameterFailureReason;
  }
#endif
  return msvcEnvironment;
}

// Build command literals
neostr_Literal(compileCommand, "cl");
neostr_Literal(dynamicLibraryFlag, "/LD"); 
neostr_Literal(executableFlag, "/MT"); 
neostr_Literal(compilerDebugSymbolFlag, "d");
neostr_Literal(debugInfoFlag, "/Zi");
neostr_Literal(optimizeDisabledFlag, "/Od");
neostr_Literal(optimizeSmallFlag, "/O1");
neostr_Literal(optimizeFastFlag, "/O2");
neostr_Literal(warningLevelNoneFlag, "/W0"); 
neostr_Literal(warningLevelSevereFlag, "/W1"); 
neostr_Literal(warningLevelImportantFlag, "/W2"); 
neostr_Literal(warningLevelVerboseFlag, "/W4"); 
neostr_Literal(warningLevelAllFlag, "/Wall");
neostr_Literal(warningAsErrorFlag, "/WX");
neostr_Literal(outputFilepathFlag, "/Fe:");
neostr_Literal(intermediateFilepathFlag, "/Fo:");
neostr_Literal(debugSymbolsFilepathFlag, "/Fd:");
neostr_Literal(macroFlag, "/D");
neostr_Literal(allCFileFlag, "/TC");
neostr_Literal(allCPPFileFlag, "/TP");
neostr_Literal(cFileFlag, "/Tc");
neostr_Literal(cppFileFlag, "/Tp");
neostr_Literal(linkerFlag, "/link");
neostr_Literal(linkerDebugSymbolFlag, "/DEBUG");
neostr_Literal(debugSymbolsFilenameFlag, "/PDB:");
neostr_Literal(windowsSubsystemFlag, "/SUBSYSTEM:");
neostr_Literal(windowsSubsystemConsoleFlag, "CONSOLE");
neostr_Literal(windowsSubsystemWindowFlag, "WINDOWS");
neostr_Literal(commandQuotationMark, "\"");
neostr_Literal(commandSpace, " ");


// Build command parsing helpers:
#define WriteSingleCommandToBuildCommand(commandName, totalLengthCounter)                                         \
  {                                                                                                               \
    neobuild_uInt64 commandLength = neostr_GetLiteralLength(commandName) - 1;                                                \
    buildCommand->command ? memcpy(buildCommand->command + totalLengthCounter, commandName, commandLength) : 0;   \
    totalLengthCounter += commandLength;                                                                          \
  }
#define WriteEscapedPathToBuildCommand(path, pathLength, totalLengthCounter)                                              \
  {                                                                                                                       \
    neobuild_uInt64 escapedPathLength = neobuild_EscapedPathLength(path, pathLength);                                           \
    buildCommand->command ? neobuild_LayoutEscapedPath(path, pathLength, buildCommand->command + totalLengthCounter) : 0;  \
    totalLengthCounter += escapedPathLength;                                                                              \
  }
#define WriteStringToBuildCommand(string, stringLength, totalLengthCounter)                               \
  {                                                                                                       \
    buildCommand->command ? memcpy(buildCommand->command + totalLengthCounter, string, stringLength) : 0; \
    totalLengthCounter += stringLength;                                                                   \
  }

// Returns the required memory to store the build command string
// If buildCommand->command is pointing to valid memory, this also builds the command string
neobuild_uInt64 neobuild_ParseCommand(neobuild_command* buildCommand)
{
  neobuild_uInt64 outCommandLength = 0;
  if (buildCommand)
  {
    WriteSingleCommandToBuildCommand(compileCommand, outCommandLength); // MSVC: cl
    WriteSingleCommandToBuildCommand(commandSpace, outCommandLength); // Space
    
    // ============= Start Compiler Options =============
    if (buildCommand->executableType == neobuild_binary_type_DYNAMIC_LIBRARY)
    {
      WriteSingleCommandToBuildCommand(dynamicLibraryFlag, outCommandLength); // MSVC: /LD 
    }
    else
    {
      WriteSingleCommandToBuildCommand(executableFlag, outCommandLength); // MSVC: /MT
    }

    if (buildCommand->generateDebugSymbols)
    {
      WriteSingleCommandToBuildCommand(compilerDebugSymbolFlag, outCommandLength); // MSVC: d (/LDd or /MTd)
      WriteSingleCommandToBuildCommand(commandSpace, outCommandLength); // Space
      WriteSingleCommandToBuildCommand(debugInfoFlag, outCommandLength); // MSVC: /Zi 
    }

    WriteSingleCommandToBuildCommand(commandSpace, outCommandLength); // Space
    
    if (buildCommand->optimizationLevel == neobuild_optimize_level_DISABLED)
    {
      WriteSingleCommandToBuildCommand(optimizeDisabledFlag, outCommandLength); // MSVC: /Od
      WriteSingleCommandToBuildCommand(commandSpace, outCommandLength); // Space
    }
    else if (buildCommand->optimizationLevel == neobuild_optimize_level_SMALL)
    {
      WriteSingleCommandToBuildCommand(optimizeSmallFlag, outCommandLength); // MSVC: /O1
      WriteSingleCommandToBuildCommand(commandSpace, outCommandLength); // Space
    }
    else if (buildCommand->optimizationLevel == neobuild_optimize_level_FAST)
    {
      WriteSingleCommandToBuildCommand(optimizeFastFlag, outCommandLength); // MSVC: /O2
      WriteSingleCommandToBuildCommand(commandSpace, outCommandLength); // Space
      
    }

    if (buildCommand->warningLevel == neobuild_warning_level_NONE)
    {
      WriteSingleCommandToBuildCommand(warningLevelNoneFlag, outCommandLength); // MSVC: /W0
      WriteSingleCommandToBuildCommand(commandSpace, outCommandLength); // Space
      
    }
    else if (buildCommand->warningLevel == neobuild_warning_level_SEVERE || 
             buildCommand->warningLevel == neobuild_warning_level_SEVERE_ERROR)
    {
      WriteSingleCommandToBuildCommand(warningLevelSevereFlag, outCommandLength); // MSVC: /W1
      WriteSingleCommandToBuildCommand(commandSpace, outCommandLength); // Space
      
    }
    else if (buildCommand->warningLevel == neobuild_warning_level_IMPORTANT || 
             buildCommand->warningLevel == neobuild_warning_level_IMPORTANT_ERROR)
    {
      WriteSingleCommandToBuildCommand(warningLevelImportantFlag, outCommandLength); // MSVC: /W2
      WriteSingleCommandToBuildCommand(commandSpace, outCommandLength); // Space
    }
    else if (buildCommand->warningLevel == neobuild_warning_level_VERBOSE ||
             buildCommand->warningLevel == neobuild_warning_level_VERBOSE_ERROR)
    {
      WriteSingleCommandToBuildCommand(warningLevelVerboseFlag, outCommandLength); // MSVC: /W4
      WriteSingleCommandToBuildCommand(commandSpace, outCommandLength); // Space
      
    }
    else if (buildCommand->warningLevel == neobuild_warning_level_ALL ||
             buildCommand->warningLevel == neobuild_warning_level_ALL_ERROR)
    {
      WriteSingleCommandToBuildCommand(warningLevelAllFlag, outCommandLength); // MSVC: /Wall
      WriteSingleCommandToBuildCommand(commandSpace, outCommandLength); // Space
    }

    // Treat warnings as errors
    if (buildCommand->warningLevel >= neobuild_warning_level_ERROR_START)
    {
      WriteSingleCommandToBuildCommand(warningAsErrorFlag, outCommandLength); // MSVC: /WX
      WriteSingleCommandToBuildCommand(commandSpace, outCommandLength); // Space
    }

    if (buildCommand->binaryFilepath.data)
    {
      WriteSingleCommandToBuildCommand(outputFilepathFlag, outCommandLength); // MSVC: /Fe:
      WriteSingleCommandToBuildCommand(commandQuotationMark, outCommandLength); // Open quotation mark
      WriteEscapedPathToBuildCommand(buildCommand->binaryFilepath.data, buildCommand->binaryFilepath.length, outCommandLength); // Output filepath
      WriteSingleCommandToBuildCommand(commandQuotationMark, outCommandLength); // Close quotation mark
      WriteSingleCommandToBuildCommand(commandSpace, outCommandLength); // Space
    }

    if (buildCommand->intermediateFilepath.data)
    {
      WriteSingleCommandToBuildCommand(intermediateFilepathFlag, outCommandLength); // MSVC: /Fo:
      WriteSingleCommandToBuildCommand(commandQuotationMark, outCommandLength); // Open quotation mark
      WriteEscapedPathToBuildCommand(buildCommand->intermediateFilepath.data, buildCommand->intermediateFilepath.length, outCommandLength); // Intermediate filepath
      WriteSingleCommandToBuildCommand(commandQuotationMark, outCommandLength); // Close quotation mark
      WriteSingleCommandToBuildCommand(commandSpace, outCommandLength); // Space
    }

    if (buildCommand->debugSymbolsFilepath.data)
    {
      WriteSingleCommandToBuildCommand(debugSymbolsFilepathFlag, outCommandLength); // MSVC: /Fd:
      WriteSingleCommandToBuildCommand(commandQuotationMark, outCommandLength); // Open quotation mark
      WriteEscapedPathToBuildCommand(buildCommand->debugSymbolsFilepath.data, buildCommand->debugSymbolsFilepath.length, outCommandLength); // Debug Symbols Filepath
      WriteSingleCommandToBuildCommand(commandQuotationMark, outCommandLength); // Close quotation mark
      WriteSingleCommandToBuildCommand(commandSpace, outCommandLength); // Space
    }

    // TODO: Custom flags
    for (neobuild_uInt64 i = 0; i < buildCommand->macroDefinitions.count; ++i)
    {
      neobuild_string_view* macroDefinition = (neobuild_string_view*)&buildCommand->macroDefinitions.definitionList[i];
      WriteSingleCommandToBuildCommand(macroFlag, outCommandLength); // MSVC: /D
      WriteStringToBuildCommand(macroDefinition->data, macroDefinition->length, outCommandLength); // Macro definition
      WriteSingleCommandToBuildCommand(commandSpace, outCommandLength); // Space
    }

    neobuild_uBool8 addLanguagePerFile = TRUE;
    if (buildCommand->buildLanguage == neobuild_target_language_C)
    {
      WriteSingleCommandToBuildCommand(allCFileFlag, outCommandLength); // MSVC: /TC
      WriteSingleCommandToBuildCommand(commandSpace, outCommandLength); // Space
      addLanguagePerFile = FALSE;
    }
    else if (buildCommand->buildLanguage == neobuild_target_language_CPP)
    {
      WriteSingleCommandToBuildCommand(allCPPFileFlag, outCommandLength); // MSVC: /TP
      WriteSingleCommandToBuildCommand(commandSpace, outCommandLength); // Space
      addLanguagePerFile = FALSE;
    }

    for (neobuild_uInt64 i = 0; i < buildCommand->compileTargets.count; ++i)
    {
      neobuild_compile_target* target = (neobuild_compile_target*)&buildCommand->compileTargets.data[i];
      
      // if neccessary, add a flag per-file
      if (addLanguagePerFile)
      {
        if (target->language == neobuild_target_language_C)
        {
          WriteSingleCommandToBuildCommand(cFileFlag, outCommandLength); // MSVC: /Tc
          WriteSingleCommandToBuildCommand(commandSpace, outCommandLength); // Space
        }
        else if (target->language == neobuild_target_language_CPP)
        {
          WriteSingleCommandToBuildCommand(cppFileFlag, outCommandLength); // MSVC: /Tp
          WriteSingleCommandToBuildCommand(commandSpace, outCommandLength); // Space
        }
      }

      WriteSingleCommandToBuildCommand(commandQuotationMark, outCommandLength); // Open quotation mark  
      WriteEscapedPathToBuildCommand(target->path.data, target->path.length, outCommandLength); // Compilation target filepath
      WriteSingleCommandToBuildCommand(commandQuotationMark, outCommandLength); // Close quotation mark  
      WriteSingleCommandToBuildCommand(commandSpace, outCommandLength); // Space
    }
    // ============= End Compiler Options =============

    WriteSingleCommandToBuildCommand(linkerFlag, outCommandLength); // MSVC: /link
    WriteSingleCommandToBuildCommand(commandSpace, outCommandLength); // Space
  
    // ============= Start Linker Options =============

    if (buildCommand->generateDebugSymbols)
    {
      WriteSingleCommandToBuildCommand(linkerDebugSymbolFlag, outCommandLength); // MSVC: /DEBUG
      WriteSingleCommandToBuildCommand(commandSpace, outCommandLength); // Space
      if (buildCommand->debugSymbolsFilepath.data)
      {
        neobuild_uBool8 hasFilename = FALSE;
        neobuild_uInt64 finalBackslashIndex = buildCommand->debugSymbolsFilepath.length; 
        while (buildCommand->debugSymbolsFilepath.data[finalBackslashIndex] != '\\')
        {
          hasFilename |= buildCommand->debugSymbolsFilepath.data[finalBackslashIndex] == '.';
          --finalBackslashIndex;
        }

        if (hasFilename)
        {
          neobuild_uInt64 filenameLength = buildCommand->debugSymbolsFilepath.length - finalBackslashIndex - 1;
          WriteSingleCommandToBuildCommand(debugSymbolsFilenameFlag, outCommandLength); // MSVC: /PDB:
          WriteStringToBuildCommand(&buildCommand->debugSymbolsFilepath.data[finalBackslashIndex + 1], filenameLength, outCommandLength);
          WriteSingleCommandToBuildCommand(commandSpace, outCommandLength); // Space
        }
      }
    }

    // TODO: use context to determine if windows logic should be run 
    // Win32 Subsystem
    WriteSingleCommandToBuildCommand(windowsSubsystemFlag, outCommandLength); // MSVC: /SUBSYSTEM:
    if (buildCommand->executableType == neobuild_binary_type_WIN32_CONSOLE)
    {
      WriteSingleCommandToBuildCommand(windowsSubsystemConsoleFlag, outCommandLength); // MSVC: CONSOLE
      WriteSingleCommandToBuildCommand(commandSpace, outCommandLength); // Space
    }
    else if (buildCommand->executableType == neobuild_binary_type_WIN32_WINDOW)
    {
      WriteSingleCommandToBuildCommand(windowsSubsystemWindowFlag, outCommandLength); // MSVC: WINDOWS
      WriteSingleCommandToBuildCommand(commandSpace, outCommandLength); // Space
    }

    // Linker input files
    for (neobuild_uInt64 i = 0; i < buildCommand->linkerInputs.count; ++i)
    {
      neobuild_string_view* inputFile = (neobuild_string_view*)&buildCommand->linkerInputs.inputFileList[i];    
      WriteSingleCommandToBuildCommand(commandQuotationMark, outCommandLength); // Open quotation mark  
      WriteEscapedPathToBuildCommand(inputFile->data, inputFile->length, outCommandLength); // Linker input filepath
      WriteSingleCommandToBuildCommand(commandQuotationMark, outCommandLength); // Close quotation mark  
      WriteSingleCommandToBuildCommand(commandSpace, outCommandLength); // Space
    }

    // ============= End Linker Options =============

    WriteStringToBuildCommand("\0", 1, outCommandLength); // Null-terminator
  }
  else
  {
    buildCommand->error.errorReason = invalidBuildCommandParameterFailureReason;
    buildCommand->error.errorCode = 404;
  }

  return outCommandLength;
}

neobuild_uInt64 neobuild_RequiredCommandMemory(neobuild_command* buildCommand)
{
  // Parsing with null command string only calculates the required size
  buildCommand->command = NULL;
  return neobuild_ParseCommand(buildCommand);
}

neobuild_uBool8 neobuild_RunCommand(neobuild_command* buildCommand)
{
  // Iterate the msvc environment blob to find the path environment variable and set it for the parent process
  // so it can find cl.exe (for the toolchain specified by buildCommand.msvcEnvironment as well)
  if (buildCommand->msvcEnvironment.data)
  {
    char* path = NULL;
    neobuild_uInt64 pathStringLength = neostr_GetLiteralLength(pathString1) - 1;
    
    const char* envVariable = buildCommand->msvcEnvironment.data; 
    for (neobuild_uInt64 envBlockIndex = 0; envBlockIndex < buildCommand->msvcEnvironment.length; ++envBlockIndex)
    {
      // If this is the first variable, or if the previous index was the null-terminator AND the current index is not (thus it is not double null-terminated),
      // the current index is the start of an environment variable
      if (envBlockIndex == 0 || (buildCommand->msvcEnvironment.data[envBlockIndex] != '\0' && buildCommand->msvcEnvironment.data[envBlockIndex - 1] == '\0'))
      {
        envVariable = &buildCommand->msvcEnvironment.data[envBlockIndex];
        // TODO: make this comparison actually case-insensitive
        if (neostr_Compare_Sized(envVariable, pathStringLength, pathString1, pathStringLength) == 0 ||
            neostr_Compare_Sized(envVariable, pathStringLength, pathString2, pathStringLength) == 0 ||
            neostr_Compare_Sized(envVariable, pathStringLength, pathString3, pathStringLength) == 0)
        {
          path = (char*)envVariable; // casting away const so that path can be temporarily edited (and restored) in order to set environment 
        }
      }
    }
    
    if (path)
    {
      path[pathStringLength - 1] = '\0';
      SetEnvironmentVariableA(path, path + pathStringLength);
      path[pathStringLength - 1] = '=';
    }
  }

  neobuild_win32_process_info buildProcess = {0};
  neobuild_win32_startup_info buildStartupInfo = {0};
  buildStartupInfo.structSizeBytes = sizeof(buildStartupInfo);
  if (CreateProcessA(0, buildCommand->command, 0, 0, 0, 0, (char*)buildCommand->msvcEnvironment.data, NULL, &buildStartupInfo, &buildProcess))
  {
    WaitForSingleObject(buildProcess.processHandle, NEOBUILD_WIN32_WAIT_INFINTE);

    GetExitCodeProcess(buildProcess.processHandle, &buildCommand->error.errorCode);
    
    buildCommand->error.errorReason = buildCommand->error.errorCode != 0 ? buildProcessFailReason : NULL;

    CloseHandle(buildProcess.processHandle);
    CloseHandle(buildProcess.threadHandle);
  }
  else
  {
    buildCommand->error.errorReason = win32CreateProcessFailed;
    buildCommand->error.errorCode = GetLastError();
  }

  return buildCommand->error.errorCode == 0;
}

neobuild_uBool8 neobuild_ParseAndRunCommand(neobuild_command* buildCommand)
{
  neobuild_ParseCommand(buildCommand); // Writes build command string into buildCommand->command
  return neobuild_RunCommand(buildCommand);
}

// Recompile constants
neostr_Literal(removeOldBuildFlag, "-removeOldBuild");
neostr_Literal(bootstrapBuildFlag, "-bootstrap");
neostr_Literal(oldBuildExtension, "_old.exe");

#define neobuild_RecompileCommandSize(argc, argv, recompileCommand, outError) neobuild_Recompile(argc, argv, recompileCommand, outError)
#define neobuild_Recompile(argc, argv, recompileCommand, outError) neobuild_ParseRecompileCommand(argc, argv, recompileCommand, __FILE__, outError)
neobuild_uInt64 neobuild_ParseRecompileCommand(int argc, char* argv[], neobuild_command* recompileCommand, const char* recompileSourceFile, neobuild_error* outError)
{
  neobuild_error error = {0};
  outError = outError ? outError : &error;
  *outError = error;

  neobuild_uInt64 outRequiredMemory = 0;
  if (recompileCommand)
  {
    char* executableFilepath = NULL;
    outError->errorCode = _get_pgmptr(&executableFilepath);
    if (outError->errorCode == 0)
    {
      neobuild_uInt64 executableFilepathLength = neostr_CStringLength(executableFilepath);
      neobuild_uInt64 finalBackslashIndex = executableFilepathLength;
      neobuild_uInt64 extensionIndex = executableFilepathLength;
      while (executableFilepath[finalBackslashIndex] != '\\') 
      {
        if (executableFilepath[finalBackslashIndex] == '.')
        {
          extensionIndex = finalBackslashIndex;
        }
        finalBackslashIndex--;
      };
      neobuild_uInt64 exeDirectoryPathLength = finalBackslashIndex + 1;
      neobuild_uInt64 exePathWithoutExtensionLength = extensionIndex; // 1 before the extension seperator 
      
      neobuild_uBool8 bootstrapped = FALSE;
      neobuild_uBool8 removeOldBuild = FALSE;
      for (int i = 1; i < argc; ++i)
      {
        neobuild_uInt64 argumentLength = neostr_CStringLength(argv[i]);
        if (neostr_Compare_Sized(argv[i], argumentLength, bootstrapBuildFlag, neostr_GetLiteralLength(bootstrapBuildFlag)) == 0)
        {
          recompileSourceFile = argv[++i];
          bootstrapped = TRUE;
        }
        else if (neostr_Compare_Sized(argv[i], argumentLength, removeOldBuildFlag, neostr_GetLiteralLength(removeOldBuildFlag)) == 0)
        {
          removeOldBuild = TRUE;
        }
      }

      void* recompileSourceFileHandle = CreateFileA(recompileSourceFile, NEOBUILD_WIN32_ACCESS_GENERIC_READ, NEOBUILD_WIN32_FILE_SHARE_READ, NULL, NEOBUILD_WIN32_FILE_OPEN_EXISTING, NEOBUILD_WIN32_FILE_ATTRIBUTE_NORMAL, NULL);
      neobuild_win32_filetime recompileSourceFileWriteTime = {0};
      GetFileTime(recompileSourceFileHandle, NULL, NULL, &recompileSourceFileWriteTime);
      neobuild_uInt32 filetimeError = GetLastError();

      void* buildExeHandle = CreateFileA(executableFilepath, NEOBUILD_WIN32_ACCESS_GENERIC_READ, NEOBUILD_WIN32_FILE_SHARE_READ, NULL, NEOBUILD_WIN32_FILE_OPEN_EXISTING, NEOBUILD_WIN32_FILE_ATTRIBUTE_NORMAL, NULL);
      neobuild_win32_filetime buildExeWriteTime = {0};
      GetFileTime(buildExeHandle, NULL, NULL, &buildExeWriteTime);
      filetimeError = GetLastError();

      CloseHandle(recompileSourceFileHandle);
      CloseHandle(buildExeHandle);

      // If build source file is newer than this exe, recompile
      if (CompareFileTime(&recompileSourceFileWriteTime, &buildExeWriteTime) > 0 || bootstrapped)
      {
        // Default any of the optional parameters if they are unset
        neobuild_compile_target recompileTarget = {0};
        recompileTarget.path.data = recompileSourceFile;
        recompileTarget.path.length = neostr_CStringLength(recompileSourceFile);
        recompileTarget.language = neobuild_target_language_C;
        recompileCommand->compileTargets.count = recompileCommand->compileTargets.data ? recompileCommand->compileTargets.count : 1;  
        recompileCommand->compileTargets.data =  recompileCommand->compileTargets.data ? recompileCommand->compileTargets.data : &recompileTarget;
        recompileCommand->executableType = recompileCommand->executableType != neobuild_binary_type_INVALID ? recompileCommand->executableType : neobuild_binary_type_WIN32_CONSOLE;
        recompileCommand->buildLanguage = recompileCommand->buildLanguage != neobuild_target_language_INVALID ? recompileCommand->buildLanguage : neobuild_target_language_C;
        recompileCommand->optimizationLevel = recompileCommand->optimizationLevel != neobuild_optimize_level_INVALID ? recompileCommand->optimizationLevel : neobuild_optimize_level_FAST;
        recompileCommand->binaryFilepath.length = recompileCommand->binaryFilepath.data ? recompileCommand->binaryFilepath.length : executableFilepathLength;
        recompileCommand->binaryFilepath.data = recompileCommand->binaryFilepath.data ? recompileCommand->binaryFilepath.data : executableFilepath;
        recompileCommand->intermediateFilepath.length = recompileCommand->intermediateFilepath.data ? recompileCommand->intermediateFilepath.length : exeDirectoryPathLength;
        recompileCommand->intermediateFilepath.data = recompileCommand->intermediateFilepath.data ? recompileCommand->intermediateFilepath.data : executableFilepath;
        recompileCommand->debugSymbolsFilepath.length = recompileCommand->debugSymbolsFilepath.data ? recompileCommand->debugSymbolsFilepath.length : exeDirectoryPathLength; 
        recompileCommand->debugSymbolsFilepath.data = recompileCommand->debugSymbolsFilepath.data ? recompileCommand->debugSymbolsFilepath.data : executableFilepath;
      
        neobuild_uInt64 recompileCommandLength = neobuild_ParseCommand(recompileCommand);

        // If there is valid command memory allocated, run the rebuild
        char* oldBuildPath = NULL;
        neobuild_uInt64 oldBuildExtensionLength = neostr_GetLiteralLength(oldBuildExtension); 
        neobuild_uInt64 oldBuildPathLength = exePathWithoutExtensionLength + oldBuildExtensionLength;
        if (recompileCommand->command)
        {
          neobuild_uInt64 nextCopyIndex = 0;

          // Using the memory after the recompile command to cache the old executable path
          oldBuildPath = recompileCommand->command + recompileCommandLength; 
          memcpy(oldBuildPath, recompileCommand->binaryFilepath.data, exePathWithoutExtensionLength);
          nextCopyIndex += exePathWithoutExtensionLength;

          memcpy(oldBuildPath + nextCopyIndex, oldBuildExtension, oldBuildExtensionLength);
          nextCopyIndex += oldBuildExtensionLength;

          // Rename the running executable to a temp name so rebuild works 
          MoveFileA(recompileCommand->binaryFilepath.data, oldBuildPath);
          neobuild_uInt32 moveFileError = GetLastError();
          if (moveFileError != 0)
          {
            recompileCommand->error.errorCode = moveFileError;
            recompileCommand->error.errorReason = win32RenameFailed;
          }

          if (recompileCommand->error.errorCode == 0)
          {
            neobuild_RunCommand(recompileCommand);
          }
        }

        // Run the recompiled build executable with the passed in arguments 
        if (recompileCommand->error.errorCode == 0)
        {
          neobuild_uInt64 recompiledCommandLineLength = 0;
          char* recompiledCommandLine = recompileCommand->command;
          for (neobuild_uInt32 i = 0; i < (neobuild_uInt32)argc; ++i)
          {
            // Don't pass the bootstrap flag or source file path to the new build
            char* argument = argv[i];
            neobuild_uInt64 argumentLength = neostr_CStringLength(argument);
            if (neostr_Compare_Sized(argument, argumentLength, bootstrapBuildFlag, neostr_GetLiteralLength(bootstrapBuildFlag)) == 0)
            {
              ++i;
              continue; 
            }

            recompileCommand->command ? memcpy(recompiledCommandLine + recompiledCommandLineLength, argument, argumentLength) : 0; 
            recompiledCommandLineLength += argumentLength;
            recompileCommand->command ? memcpy(recompiledCommandLine + recompiledCommandLineLength, " ", 1) : 0; 
            recompiledCommandLineLength += 1;        
          }
          recompileCommand->command ? memcpy(recompiledCommandLine + recompiledCommandLineLength - 1, "\0", 1) : 0; // null-terminator overwrites final space
          
          neobuild_uInt64 removeOldBuildRequiredMemory = recompileCommand->binaryFilepath.length + 1 + neostr_GetLiteralLength(removeOldBuildFlag);
          outRequiredMemory = recompileCommandLength > recompiledCommandLineLength ? recompileCommandLength : recompiledCommandLineLength;
          outRequiredMemory = outRequiredMemory > removeOldBuildRequiredMemory ? outRequiredMemory : removeOldBuildRequiredMemory;
          outRequiredMemory += oldBuildPathLength;
          
          if (recompileCommand->command)
          {  
            neobuild_win32_startup_info newBuildStartupInfo = {0};
            newBuildStartupInfo.structSizeBytes = sizeof(newBuildStartupInfo);
            neobuild_win32_process_info newBuildProcess = {0};
            
            if (CreateProcessA(0, recompiledCommandLine, 0, 0, TRUE, 0, (char*)recompileCommand->msvcEnvironment.data, NULL, &newBuildStartupInfo, &newBuildProcess))
            {
              // Wait for the new build executable to finish running
              WaitForSingleObject(newBuildProcess.processHandle, NEOBUILD_WIN32_WAIT_INFINTE);
              CloseHandle(newBuildProcess.processHandle);
              CloseHandle(newBuildProcess.threadHandle);

              // Rewrite the recompiled executable command line to specify the -removeOldBuild flag
              memcpy(recompiledCommandLine + recompileCommand->binaryFilepath.length, " ", 1);
              memcpy(recompiledCommandLine + recompileCommand->binaryFilepath.length + 1, removeOldBuildFlag, neostr_GetLiteralLength(removeOldBuildFlag));
              memcpy(recompiledCommandLine + removeOldBuildRequiredMemory, "\0", 1);

              // Call the recompiled executable again with the removeOldBuild flag to delete this executable file after it closes
              neobuild_win32_startup_info removeOldBuildStartupInfo = {0};
              removeOldBuildStartupInfo.structSizeBytes = sizeof(removeOldBuildStartupInfo);

              neobuild_win32_process_info removeOldBuildProcess = {0};
              if (CreateProcessA(0, recompiledCommandLine, 0, 0, 0, NEOBUILD_WIN32_CREATE_DETACHED_PROCESS, (char*)recompileCommand->msvcEnvironment.data, 0, &removeOldBuildStartupInfo, &removeOldBuildProcess))
              {
                CloseHandle(removeOldBuildProcess.processHandle);
                CloseHandle(removeOldBuildProcess.threadHandle);
              }
              else
              {
                recompileCommand->error.errorCode = GetLastError();
                recompileCommand->error.errorReason = win32CreateProcessFailed;
              }
              
              ExitProcess(recompileCommand->error.errorCode);
            }
            else
            {
              recompileCommand->error.errorCode = GetLastError();
              recompileCommand->error.errorReason = win32CreateProcessFailed;
            }
            

            // If execution is here, either the recompile failed or starting the recompiled process failed
            // Rename the executable back to it's original name
            if (!MoveFileA(oldBuildPath, recompileCommand->binaryFilepath.data))
            {
              recompileCommand->error.errorCode = GetLastError();
              recompileCommand->error.errorReason = win32RenameFailed;
            }
          }
        }
      }
      else if (removeOldBuild)
      {
        neobuild_uInt64 oldBuildExtensionLength = neostr_GetLiteralLength(oldBuildExtension); 
        neobuild_uInt64 oldBuildPathLength = exePathWithoutExtensionLength + oldBuildExtensionLength;
        outRequiredMemory = oldBuildPathLength;

        if (recompileCommand->command)
        {
          neobuild_uInt64 nextCopyIndex = 0; 
          char* oldBuildPath = recompileCommand->command;
          memcpy(oldBuildPath, executableFilepath, exePathWithoutExtensionLength);
          nextCopyIndex += exePathWithoutExtensionLength;

          memcpy(oldBuildPath + nextCopyIndex, oldBuildExtension, oldBuildExtensionLength);
          nextCopyIndex += oldBuildExtensionLength;
          
          // Try to delete the old executable after it closes
          while (!DeleteFileA(oldBuildPath))
          {
            neobuild_uInt32 deleteFileError = GetLastError();
            if (deleteFileError != NEOBUILD_WIN32_ERROR_ACCESS_DENIED) 
            {
              // If the delete failed for any other reason than the old process is still blocking it, stop trying to avoid infinite loop
              break; 
            }
          }

          // If removeOldBuild is specified, the process should not attempt to build anything and exit now
          ExitProcess(0);
        }
      }
    }
    else
    {
      outError->errorReason = failedToRetrieveExecutablePath;
    }
  }

  return outRequiredMemory;
}

//======================================= end NEOBUILD_IMPLEMENTATION ======================================================


#endif // defined NEOBUILD_IMPLEMENTATION