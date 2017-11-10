#include "torque.h"
#include <Windows.h>
#include <Psapi.h>

//Global variables

//Start of the Blockland.exe module in memory
static DWORD ImageBase;

//Length of the Blockland.exe module
static DWORD ImageSize;

//StringTable pointer
static DWORD StringTable;

//Global Variable dictionary pointer
static DWORD GlobalVars;



//Extern engine function definitions

PrintfFn Printf;
EvalFn Eval;



//Engine function declarations

//Con::lookupNamespace
DECL_BL_FUNC(DWORD*, , LookupNamespace, const char* ns)

//StringTable::insert
DECL_BL_FUNC(const char*, __thiscall, StringTableInsert, DWORD stringTablePtr, const char* val, const bool caseSensitive)

//NameSpace::addCommand overloads
DECL_BL_FUNC(void, __thiscall, AddStringCommand, DWORD* ns, const char* name, StringCallback cb, const char* usage, int minArgs, int maxArgs)
DECL_BL_FUNC(void, __thiscall, AddIntCommand, DWORD* ns, const char* name, IntCallback    cb, const char* usage, int minArgs, int maxArgs)
DECL_BL_FUNC(void, __thiscall, AddFloatCommand, DWORD* ns, const char* name, FloatCallback  cb, const char* usage, int minArgs, int maxArgs)
DECL_BL_FUNC(void, __thiscall, AddVoidCommand, DWORD* ns, const char* name, VoidCallback   cb, const char* usage, int minArgs, int maxArgs)
DECL_BL_FUNC(void, __thiscall, AddBoolCommand, DWORD* ns, const char* name, BoolCallback   cb, const char* usage, int minArgs, int maxArgs)

//Exposing variables to torquescript
DECL_BL_FUNC(void, __thiscall, AddVariable, DWORD dictionaryPtr, const char* name, int type, void* data)



//Signature scanner

//Set the module start and length
void InitScanner(char* moduleName)
{
	//Find the module
	HMODULE module = GetModuleHandleA(moduleName);

	if (module)
	{
		//Retrieve information about the module
		MODULEINFO info;
		GetModuleInformation(GetCurrentProcess(), module, &info, sizeof(MODULEINFO));

		//Store relevant information
		ImageBase = (DWORD)info.lpBaseOfDll;
		ImageSize = info.SizeOfImage;
	}
}

//Compare data at two locations for equality
bool CompareData(PBYTE data, PBYTE pattern, char* mask)
{
	//Iterate over the data, pattern and mask in parallel
	for (; *mask; ++data, ++pattern, ++mask)
	{
		//And check for equality at each unmasked byte
		if (*mask == 'x' && *data != *pattern)
			return false;
	}

	return (*mask) == NULL;
}

//Find a pattern in memory
DWORD FindPattern(DWORD imageBase, DWORD imageSize, PBYTE pattern, char* mask)
{
	//Iterate over the image
	for (DWORD i = imageBase; i < imageBase + imageSize; i++)
	{
		//And check for matching pattern at every byte
		if (CompareData((PBYTE)i, pattern, mask))
			return i;
	}

	return 0;
}

//Scan the module for a pattern
DWORD ScanFunc(char* pattern, char* mask)
{
	//Just search for the pattern in the module
	return FindPattern(ImageBase, ImageSize - strlen(mask), (PBYTE)pattern, mask);
}



//Byte patcher

//Change a byte at a specific location in memory
void PatchByte(BYTE* location, BYTE value)
{
	//Remove protection
	DWORD oldProtection;
	VirtualProtect(location, 1, PAGE_EXECUTE_READWRITE, &oldProtection);

	//Change value
	*location = value;

	//Restore protection
	VirtualProtect(location, 1, oldProtection, &oldProtection);
}

//Change a dword at a specific location in memory
void PatchDword(DWORD* location, DWORD value)
{
	//Remove protection
	DWORD oldProtection;
	VirtualProtect(location, 4, PAGE_EXECUTE_READWRITE, &oldProtection);

	//Change value
	*location = value;

	//Restore protection
	VirtualProtect(location, 4, oldProtection, &oldProtection);
}



//Console functions and variable

void ConsoleFunction(const char* nameSpace, const char* name, StringCallback callBack, const char* usage, int minArgs, int maxArgs)
{
	AddStringCommand(LookupNamespace(nameSpace), StringTableInsert(StringTable, name, false), callBack, usage, minArgs, maxArgs);
}

void ConsoleFunction(const char* nameSpace, const char* name, IntCallback callBack, const char* usage, int minArgs, int maxArgs)
{
	AddIntCommand(LookupNamespace(nameSpace), StringTableInsert(StringTable, name, false), callBack, usage, minArgs, maxArgs);
}

void ConsoleFunction(const char* nameSpace, const char* name, FloatCallback callBack, const char* usage, int minArgs, int maxArgs)
{
	AddFloatCommand(LookupNamespace(nameSpace), StringTableInsert(StringTable, name, false), callBack, usage, minArgs, maxArgs);
}

void ConsoleFunction(const char* nameSpace, const char* name, VoidCallback callBack, const char* usage, int minArgs, int maxArgs)
{
	AddVoidCommand(LookupNamespace(nameSpace), StringTableInsert(StringTable, name, false), callBack, usage, minArgs, maxArgs);
}

void ConsoleFunction(const char* nameSpace, const char* name, BoolCallback callBack, const char* usage, int minArgs, int maxArgs)
{
	AddBoolCommand(LookupNamespace(nameSpace), StringTableInsert(StringTable, name, false), callBack, usage, minArgs, maxArgs);
}

void ConsoleVariable(const char* name, int* data)
{
	AddVariable(GlobalVars, name, 4, data);
}

void ConsoleVariable(const char* name, bool* data)
{
	AddVariable(GlobalVars, name, 6, data);
}

void ConsoleVariable(const char* name, float* data)
{
	AddVariable(GlobalVars, name, 8, data);
}

void ConsoleVariable(const char* name, char* data)
{
	AddVariable(GlobalVars, name, 10, data);
}



//Initialization

//Initialize the Torque Interface
bool InitTorque()
{
	//Init the scanner
	InitScanner("Blockland.exe");

	//Printf is required for debug output, so find it first
	Printf = (PrintfFn)ScanFunc("\x8B\x4C\x24\x04\x8D\x44\x24\x08\x50\x6A\x00\x6A\x00\xE8\x00\x00\x00\x00\x83\xC4\x0C\xC3", "xxxxxxxxxxxxxx????xxxx");

	//Do nothing if we don't find it :(
	if (!Printf)
		return false;

	//First find all the functions
	SCAN_BL_FUNC(LookupNamespace, "\x8B\x44\x24\x04\x85\xC0\x75\x05", "xxxxxxxx");
	SCAN_BL_FUNC(StringTableInsert, "\x53\x8B\x5C\x24\x08\x55\x56\x57\x53", "xxxxxxxxx");

	//These are almost identical. Long sigs required
	SCAN_BL_FUNC(AddStringCommand,
		"\x8B\x44\x24\x04\x56\x50\xE8\x00\x00\x00\x00\x8B\xF0\xA1\x00\x00\x00\x00\x40\xB9\x00\x00\x00\x00\xA3"
		"\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x8B\x4C\x24\x10\x8B\x54\x24\x14\x8B\x44\x24\x18\x89\x4E\x18\x8B"
		"\x4C\x24\x0C\x89\x56\x10\x89\x46\x14\xC7\x46\x0C\x01\x00\x00\x00\x89\x4E\x28\x5E\xC2\x14\x00",
		"xxxxxxx????xxx????xx????x????x????xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");

	SCAN_BL_FUNC(AddIntCommand,
		"\x8B\x44\x24\x04\x56\x50\xE8\x00\x00\x00\x00\x8B\xF0\xA1\x00\x00\x00\x00\x40\xB9\x00\x00\x00\x00\xA3"
		"\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x8B\x4C\x24\x10\x8B\x54\x24\x14\x8B\x44\x24\x18\x89\x4E\x18\x8B"
		"\x4C\x24\x0C\x89\x56\x10\x89\x46\x14\xC7\x46\x0C\x02\x00\x00\x00\x89\x4E\x28\x5E\xC2\x14\x00",
		"xxxxxxx????xxx????xx????x????x????xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");

	SCAN_BL_FUNC(AddFloatCommand,
		"\x8B\x44\x24\x04\x56\x50\xE8\x00\x00\x00\x00\x8B\xF0\xA1\x00\x00\x00\x00\x40\xB9\x00\x00\x00\x00\xA3"
		"\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x8B\x4C\x24\x10\x8B\x54\x24\x14\x8B\x44\x24\x18\x89\x4E\x18\x8B"
		"\x4C\x24\x0C\x89\x56\x10\x89\x46\x14\xC7\x46\x0C\x03\x00\x00\x00\x89\x4E\x28\x5E\xC2\x14\x00",
		"xxxxxxx????xxx????xx????x????x????xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");

	SCAN_BL_FUNC(AddVoidCommand,
		"\x8B\x44\x24\x04\x56\x50\xE8\x00\x00\x00\x00\x8B\xF0\xA1\x00\x00\x00\x00\x40\xB9\x00\x00\x00\x00\xA3"
		"\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x8B\x4C\x24\x10\x8B\x54\x24\x14\x8B\x44\x24\x18\x89\x4E\x18\x8B"
		"\x4C\x24\x0C\x89\x56\x10\x89\x46\x14\xC7\x46\x0C\x04\x00\x00\x00\x89\x4E\x28\x5E\xC2\x14\x00",
		"xxxxxxx????xxx????xx????x????x????xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");

	SCAN_BL_FUNC(AddBoolCommand,
		"\x8B\x44\x24\x04\x56\x50\xE8\x00\x00\x00\x00\x8B\xF0\xA1\x00\x00\x00\x00\x40\xB9\x00\x00\x00\x00\xA3"
		"\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x8B\x4C\x24\x10\x8B\x54\x24\x14\x8B\x44\x24\x18\x89\x4E\x18\x8B"
		"\x4C\x24\x0C\x89\x56\x10\x89\x46\x14\xC7\x46\x0C\x05\x00\x00\x00\x89\x4E\x28\x5E\xC2\x14\x00",
		"xxxxxxx????xxx????xx????x????x????xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");

	SCAN_BL_FUNC(AddVariable, "\x8B\x44\x24\x04\x56\x8B\xF1\x80\x38\x24\x74\x1A", "xxxxxxxxxxxx");
	SCAN_BL_FUNC(Eval, "\x8A\x44\x24\x08\x84\xC0\x56\x57\x8B\x7C\x24\x0C", "xxxxxxxxxxxx");

	//The string table is used in LookupNamespace so we can get it's location
	StringTable = *(DWORD*)(*(DWORD*)((DWORD)LookupNamespace + 15));

	//Get the global variable dictionary pointer
	GlobalVars = *(DWORD*)(ScanFunc("\x8B\x44\x24\x0C\x8B\x4C\x24\x04\x50\x6A\x06", "xxxxxxxxxxx") + 13);

	return true;
}