#pragma once
#include <fstream>
#include <vector>
#include <unordered_map>
#include <Windows.h>

struct IIDX {
	IMAGE_IMPORT_DESCRIPTOR iid;
	std::string Name;
};

struct INT_Element {
	ULONGLONG addr;
	WORD Hint;
	std::string Name;
};

struct INT_Elementx86 {
	DWORD addr;
	WORD Hint;
	std::string Name;
};


/*
| ordinal	2byte	|
| null		2byte	|
| rva		4byte	|
| funcAddr	4byte	|
| null		4byte	|
| name		40byte	|
*/
struct ExportElement {
	WORD ordinal;
	DWORD rva;
	DWORD funcAddr;
	std::string name;
};

class PEFile
{
public:
	PEFile(const std::string& filePath);

public:
	void Run();

private:
	// read data
	bool readDosHeader(std::ifstream& file);
	bool readDosStub(std::ifstream& file);
	bool readNtHeaders(std::ifstream& file);
	bool readNtHeaderx86(std::ifstream& file);
	bool readSectionHeaders(std::ifstream& file);

	bool readDirectory(std::ifstream& file);
	bool readExportDirectory(std::ifstream& file, DWORD rva);
	bool readImportDirectory(std::ifstream& file, DWORD rva);

	// functional
	void printDosHeader();
	void printNtHeaders();
	void printFileHeader();
	void printOptionalHeader();
	void printSectionHeader(IMAGE_SECTION_HEADER& header);
	void printCharacteristics(DWORD characteristics);
	void printImportDirectoyTable();
	void printImportNameTable();
	void printImportAddressTable();
	void printExportDirectory();
	void printExportAddressTable();
	void printExportNameTable();
	void printExportOrdinalTable();

	// Utills
	void printByte(void* data, int size, int first_offset = 0, int interval = 1, int size_of_element = 1);
	void printRaw(void* data, int size, int first_offset = 0, int interval = 1, int size_of_element = 1);
	void printByteAndRaw(void* data, int size, int first_offset = 0, int interval = 1, int size_of_element = 1);
	DWORD rva2raw(DWORD rva);

	void printHelp(const std::string& cmd);

	void GetMachineString(WORD machine);
	void GetSubSystemString(WORD subsys);
	std::string GetDirectoryName(int i);

public:
	bool loaded = false;

private:
	IMAGE_DOS_HEADER m_dosHeader;
	IMAGE_NT_HEADERS m_ntHeaders;
	IMAGE_NT_HEADERS32 m_ntHeaderx86;

	std::vector<IMAGE_SECTION_HEADER> m_sectionHeaders;

	std::unordered_map<std::string, int> commands;

	// import directory data
	std::vector<IIDX> m_IIDs;
	std::vector<INT_Element> m_INT;
	std::vector<INT_Elementx86> m_INTx86;
	std::vector<ULONGLONG> m_IAT;
	std::vector<DWORD> m_IATx86;

	// export directory data
	IMAGE_EXPORT_DIRECTORY m_IED;
	std::vector<ExportElement> m_ExportTable;
	
	// bytes
	std::string m_dosStubByte;

	// strings
	std::string m_peName;
	std::string m_machineStr;
	std::string m_subSystem;
	std::string m_exportName;

	// format string x86
	char x86_2byte_desc32_v32[28] = "      %02X | %-32s | %-32s\n";
	char x86_4byte_desc32_v32[26] = "    %04X | %-32s | %-32s\n";
	char x86_8byte_desc32_v32[22] = "%08X | %-32s | %-32s\n";

	char x86_4byte_desc16[26] = "    %04X | %-16s | %-24s\n";
	char x86_4byte_desc24[26] = "    %04X | %-24s | %-24s\n";
	char x86_4byte_desc32[26] = "    %04X | %-32s | %-24s\n";	

	char x86_8byte_desc16[22] = "%08X | %-16s | %-24s\n";
	char x86_8byte_desc24[22] = "%08X | %-24s | %-24s\n";
	char x86_8byte_desc32[22] = "%08X | %-32s | %-24s\n";	

	char x86_8str_desc16[21] = "%8s | %-16s | %-24s\n";
	char x86_8str_desc24[21] = "%8s | %-24s | %-24s\n";
	char x86_8str_desc32[21] = "%8s | %-32s | %-24s\n";
	char x86_8str_desc16_v32[22] = "%08X | %-16s | %-32s\n";

	// format string x64
	char x64_2byte_desc32_v32[36] = "              %02X | %-32s | %-32s\n";
	char x64_4byte_desc32_v32[34] = "            %04X | %-32s | %-32s\n";
	char x64_8byte_desc32_v32[30] = "        %08X | %-32s | %-32s\n";
	char x64_16byte_desc32_v32[25] = "%016llX | %-32s | %-32s\n";
	char x64_16byte_desc16_v32[25] = "%016llX | %-16s | %-32s\n";

	bool x86 = false;
	bool exportDir = false;
	bool hasIAT = false;
};

