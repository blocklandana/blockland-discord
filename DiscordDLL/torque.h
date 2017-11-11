#pragma once
#include <Windows.h>

//Macros

//Declare engine function
#define DECL_BL_FUNC(ReturnType, Convention, Name, ...) \
	typedef ReturnType (Convention*Name##Fn)(__VA_ARGS__); \
	static Name##Fn Name;

//Declare exported engine function (for use in other files)
#define DECL_BL_FUNC_EXTERN(ReturnType, Convention, Name, ...) \
	typedef ReturnType (Convention*Name##Fn)(__VA_ARGS__); \
	extern Name##Fn Name;

//Search for an engine function in blockland
#define SCAN_BL_FUNC(Target, Pattern, Mask) \
	Target = (Target##Fn)ScanFunc(Pattern, Mask); \
	if(Target == NULL) \
		Printf("PlayerCollisionToggle | Failed to find function "#Target"!");



//Engine function declarations

//Con::printf
DECL_BL_FUNC_EXTERN(void, , Printf, const char* Format, ...)

//Execute torquescript code in global scope
DECL_BL_FUNC_EXTERN(const char*, , Eval, const char* str, bool echo, const char* fileName)

//Callback types for exposing methods to torquescript
typedef const char* (*StringCallback)(DWORD* obj, int argc, const char* argv[]);
typedef int(*IntCallback)(DWORD* obj, int argc, const char* argv[]);
typedef float(*FloatCallback)(DWORD* obj, int argc, const char* argv[]);
typedef void(*VoidCallback)(DWORD* obj, int argc, const char* argv[]);
typedef bool(*BoolCallback)(DWORD* obj, int argc, const char* argv[]);



//Public functions

//Scan the module for a pattern
DWORD ScanFunc(char* pattern, char* mask);

//Change a byte at a specific location in memory
void PatchByte(BYTE* location, BYTE value);

//Change a dword at a specific location in memory
void PatchDword(DWORD* location, DWORD value);

//Register a torquescript function. The function must look like this:
//Type Func(DWORD* obj, int argc, const char* argv[])
void ConsoleFunction(const char* nameSpace, const char* name, StringCallback callBack, const char* usage, int minArgs, int maxArgs);
void ConsoleFunction(const char* nameSpace, const char* name, IntCallback    callBack, const char* usage, int minArgs, int maxArgs);
void ConsoleFunction(const char* nameSpace, const char* name, FloatCallback  callBack, const char* usage, int minArgs, int maxArgs);
void ConsoleFunction(const char* nameSpace, const char* name, VoidCallback   callBack, const char* usage, int minArgs, int maxArgs);
void ConsoleFunction(const char* nameSpace, const char* name, BoolCallback   callBack, const char* usage, int minArgs, int maxArgs);

//Expose a variable to torquescript
void ConsoleVariable(const char* name, char*  data);
void ConsoleVariable(const char* name, int*   data);
void ConsoleVariable(const char* name, float* data);
void ConsoleVariable(const char* name, bool*  data);

//Initialize the Torque Interface
bool InitTorque();