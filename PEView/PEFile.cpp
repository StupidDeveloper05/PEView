#define _CRT_SECURE_NO_WARNINGS
#include "PEFile.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <algorithm>

PEFile::PEFile(const std::string& filePath)
{
	m_peName = filePath.substr(filePath.find_last_of('\\')+1);

	std::ifstream file(filePath, std::ios::binary);
	if (!file.is_open()) {
		std::cerr << "파일을 열 수 없습니다." << std::endl;
		return;
	}
	if (!readDosHeader(file)) {
		std::cerr << "DOS header를 읽는데 실패하였습니다." << std::endl;
		return;
	}
	if (m_dosHeader.e_magic != IMAGE_DOS_SIGNATURE) {
		std::cerr << "유효하지 않은 DOS header 입니다." << std::endl;
		return;
	}
	if (!readDosStub(file)) {
		std::cerr << "DOS Stub을 읽는데 실패하였습니다." << std::endl;
		return;
	}
	if (!readNtHeaders(file)) {
		std::cerr << "NT Headers를 읽는데 실패하였습니다." << std::endl;
		return;
	}
	if (m_ntHeaders.Signature != IMAGE_NT_SIGNATURE) {
		std::cerr << "유효하지 않은 NT header 입니다." << std::endl;
		return;
	}
	if (m_ntHeaders.OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
		readNtHeaderx86(file);
		x86 = true;
	}	
	if (!readSectionHeaders(file)) {
		std::cerr << "Section Header를 읽는데 실패하였습니다." << std::endl;
		return;
	}
	if (!readDirectory(file)) {
		std::cerr << "Directory를 읽는데 실패하였습니다." << std::endl;
		return;
	}

	GetMachineString(m_ntHeaders.FileHeader.Machine);
	if (x86)
		GetSubSystemString(m_ntHeaderx86.OptionalHeader.Subsystem);
	else
		GetSubSystemString(m_ntHeaders.OptionalHeader.Subsystem);

	// initialize command list
	commands["DOS"] = 1;
	commands["STUB"] = 1;
	commands["NT"] = 1;
	commands["SH"] = 1;
	commands["IDT"] = 1;
	commands["INT"] = 1;
	commands["IAT"] = 1;
	commands["IED"] = 1;
	commands["EAT"] = 1;
	commands["ENT"] = 1;
	commands["EOT"] = 1;
	commands["EXIT"] = 1;
	commands["CLS"] = 1;
	commands["HELP"] = 1;

	file.close();
	loaded = true;
}

void PEFile::Run()
{
	while (true)
	{
		std::string cmd;

		std::cout << m_peName << ">";
		std::getline(std::cin, cmd);
		std::transform(cmd.begin(), cmd.end(), cmd.begin(), [](unsigned char c) { return std::toupper(c); });

		std::vector<std::string> result;
		std::istringstream iss(cmd);
		std::string word;
		while (iss >> word) {
			result.push_back(word);
		}

		if (result.size() < 1)
			continue;

		if (commands[result[0]] == 1)
		{
			if (result[0] == "DOS")
			{
				if (result.size() == 1)
					printDosHeader();
				else if (result.size() == 2)
				{
					if (result[1] == "-R")
						printRaw(&m_dosHeader, sizeof(IMAGE_DOS_HEADER));
					else if (result[1] == "-B")
						printByte(&m_dosHeader, sizeof(IMAGE_DOS_HEADER));
					else if (result[1] == "-RB")
						printByteAndRaw(&m_dosHeader, sizeof(IMAGE_DOS_HEADER));
					else if (result[1] == "-H")
						printHelp(result[0]);
					else
						std::cout << "\'" << result[1] << "\' 은(는) 올바른 옵션이 아닙니다." <<
						std::endl << "도움말을 보려면 DOS -h를 입력하세요.\n" << std::endl;
				}
				else
				{
					std::cout << "올바른 옵션 형식이 아닙니다." <<
						std::endl << "도움말을 보려면 DOS -h를 입력하세요.\n" << std::endl;
				}
			}
			else if (result[0] == "STUB")
			{
				if (result.size() == 1)
					printByteAndRaw(&m_dosStubByte[0], m_dosStubByte.size());
				else if (result.size() == 2)
				{
					if (result[1] == "-R")
						printRaw(&m_dosStubByte[0], m_dosStubByte.size());
					else if (result[1] == "-B")
						printByte(&m_dosStubByte[0], m_dosStubByte.size());
					else if (result[1] == "-RB")
						printByteAndRaw(&m_dosStubByte[0], m_dosStubByte.size());
					else if (result[1] == "-H")
						printHelp(result[0]);
					else
						std::cout << "\'" << result[1] << "\' 은(는) 올바른 옵션이 아닙니다." <<
						std::endl << "도움말을 보려면 STUB -h를 입력하세요.\n" << std::endl;
				}
				else
				{
					std::cout << "올바른 옵션 형식이 아닙니다." <<
						std::endl << "도움말을 보려면 STUB -h를 입력하세요.\n" << std::endl;
				}
			}
			else if (result[0] == "NT")
			{
				if (result.size() == 1)
					printNtHeaders();
				else if (result.size() == 2)
				{
					if (result[1] == "-R")
						printRaw(x86 ? (void*)&m_ntHeaderx86 : (void*)&m_ntHeaders, x86 ? sizeof(IMAGE_NT_HEADERS32) : sizeof(IMAGE_NT_HEADERS64));
					else if (result[1] == "-B")
						printByte(x86 ? (void*)&m_ntHeaderx86 : (void*)&m_ntHeaders, x86 ? sizeof(IMAGE_NT_HEADERS32) : sizeof(IMAGE_NT_HEADERS64));
					else if (result[1] == "-RB")
						printByteAndRaw(x86 ? (void*)&m_ntHeaderx86 : (void*)&m_ntHeaders, x86 ? sizeof(IMAGE_NT_HEADERS32) : sizeof(IMAGE_NT_HEADERS64));
					else if (result[1] == "-H")
						printHelp(result[0]);
					else if (result[1] == "-FILE")
						printFileHeader();
					else if (result[1] == "-OPT")
						printOptionalHeader();
					else
						std::cout << "\'" << result[1] << "\' 은(는) 올바른 옵션이 아닙니다." <<
						std::endl << "도움말을 보려면 NT -h를 입력하세요.\n" << std::endl;
				}
				else if (result.size() == 3)
				{
					if (result[1] == "-FILE")
					{
						if (result[2] == "-R")
							printRaw(x86 ? (void*)&m_ntHeaderx86.FileHeader : (void*)&m_ntHeaders.FileHeader, sizeof(IMAGE_FILE_HEADER));
						else if (result[2] == "-B")
							printByte(x86 ? (void*)&m_ntHeaderx86.FileHeader : (void*)&m_ntHeaders.FileHeader, sizeof(IMAGE_FILE_HEADER));
						else if (result[2] == "-RB")
							printByteAndRaw(x86 ? (void*)&m_ntHeaderx86.FileHeader : (void*)&m_ntHeaders.FileHeader, sizeof(IMAGE_FILE_HEADER));
						else
							std::cout << "\'" << result[2] << "\' 은(는) 올바른 옵션이 아닙니다." <<
							std::endl << "도움말을 보려면 NT -h를 입력하세요.\n" << std::endl;
					}
					else if (result[1] == "-OPT")
					{
						if (result[2] == "-R")
							printRaw(x86 ? (void*)&m_ntHeaderx86.OptionalHeader : (void*)&m_ntHeaders.OptionalHeader, x86 ? sizeof(IMAGE_OPTIONAL_HEADER32) : sizeof(IMAGE_OPTIONAL_HEADER64));
						else if (result[2] == "-B")
							printByte(x86 ? (void*)&m_ntHeaderx86.OptionalHeader : (void*)&m_ntHeaders.OptionalHeader, x86 ? sizeof(IMAGE_OPTIONAL_HEADER32) : sizeof(IMAGE_OPTIONAL_HEADER64));
						else if (result[2] == "-RB")
							printByteAndRaw(x86 ? (void*)&m_ntHeaderx86.OptionalHeader : (void*)&m_ntHeaders.OptionalHeader, x86 ? sizeof(IMAGE_OPTIONAL_HEADER32) : sizeof(IMAGE_OPTIONAL_HEADER64));
						else
							std::cout << "\'" << result[2] << "\' 은(는) 올바른 옵션이 아닙니다." <<
							std::endl << "도움말을 보려면 NT -h를 입력하세요.\n" << std::endl;
					}
					else
					{
						std::cout << "올바른 옵션 형식이 아닙니다." <<
							std::endl << "도움말을 보려면 NT -h를 입력하세요.\n" << std::endl;
					}
				}
				else
				{
					std::cout << "올바른 옵션 형식이 아닙니다." <<
						std::endl << "도움말을 보려면 NT -h를 입력하세요.\n" << std::endl;
				}
			}
			else if (result[0] == "SH")
			{
				if (result.size() == 1)
				{
					for (int i = 0; i < m_sectionHeaders.size(); ++i)
						std::cout << i << " : " << m_sectionHeaders[i].Name << std::endl;
					std::cout << std::endl;
				}
				else if (result.size() >= 2)
				{
					if (result[1] == "-H")
					{
						if (result.size() == 2)
							printHelp(result[0]);
						else
							std::cout << "올바른 옵션 형식이 아닙니다." <<
							std::endl << "도움말을 보려면 SH -h를 입력하세요.\n" << std::endl;
					}
					else
					{
						if (std::all_of(result[1].begin(), result[1].end(), std::isdigit))
						{
							int idx = std::stoi(result[1]);
							if (idx >= 0 && idx < m_sectionHeaders.size())
							{
								if (result.size() == 3)
								{
									if (result[2] == "-R")
										printRaw(&m_sectionHeaders[idx], sizeof(IMAGE_SECTION_HEADER));
									else if (result[2] == "-B")
										printByte(&m_sectionHeaders[idx], sizeof(IMAGE_SECTION_HEADER));
									else if (result[2] == "-RB")
										printByteAndRaw(&m_sectionHeaders[idx], sizeof(IMAGE_SECTION_HEADER));
									else
										std::cout << "\'" << result[2] << "\' 은(는) 올바른 옵션이 아닙니다." <<
										std::endl << "도움말을 보려면 SH -h를 입력하세요.\n" << std::endl;
								}
								else if (result.size() == 2)
									printSectionHeader(m_sectionHeaders[idx]);
								else
									std::cout << "올바른 옵션 형식이 아닙니다." <<
									std::endl << "도움말을 보려면 SH -h를 입력하세요.\n" << std::endl;
							}
							else
								std::cout << "올바른 범위의 인덱스를 입력하세요." << std::endl << std::endl;
						}
						else
						{
							int idx = 0;
							bool success = false;
							for (auto sec : m_sectionHeaders)
							{
								std::string name = (const char*)sec.Name;
								std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return std::toupper(c); });

								if (result[1] == name)
								{
									if (result.size() == 3)
									{
										if (result[2] == "-R")
											printRaw(&m_sectionHeaders[idx], sizeof(IMAGE_SECTION_HEADER));
										else if (result[2] == "-B")
											printByte(&m_sectionHeaders[idx], sizeof(IMAGE_SECTION_HEADER));
										else if (result[2] == "-RB")
											printByteAndRaw(&m_sectionHeaders[idx], sizeof(IMAGE_SECTION_HEADER));
										else
											std::cout << "\'" << result[2] << "\' 은(는) 올바른 옵션이 아닙니다." <<
											std::endl << "도움말을 보려면 SH -h를 입력하세요.\n" << std::endl;
									}
									else if (result.size() == 2)
										printSectionHeader(sec);
									else
										std::cout << "올바른 옵션 형식이 아닙니다." <<
										std::endl << "도움말을 보려면 SH -h를 입력하세요.\n" << std::endl;
									success = true;
									break;
								}
								idx++;
							}
							if (!success)
								std::cout << "\'" << result[1] << "\' 은(는) 올바른 옵션이 아닙니다." <<
								std::endl << "도움말을 보려면 SH -h를 입력하세요.\n" << std::endl;
						}
					}						
				}
			}
			else if (result[0] == "IDT") 
			{
				if (result.size() == 1)
					printImportDirectoyTable();
				else if (result.size() == 2)
				{
					if (result[1] == "-H")
						printHelp(result[0]);
					else if (result[1] == "-R")
					{
						for (auto iid : m_IIDs)
						{
							std::cout << iid.Name.c_str() << std::endl;
							printRaw(&iid.iid, sizeof(IMAGE_IMPORT_DESCRIPTOR));
						}
					}
					else if (result[1] == "-B")
					{
						for (auto iid : m_IIDs)
						{
							std::cout << iid.Name.c_str() << std::endl;
							printByte(&iid.iid, sizeof(IMAGE_IMPORT_DESCRIPTOR));
						}
					}
					else if (result[1] == "-RB")
					{
						for (auto iid : m_IIDs)
						{
							std::cout << iid.Name.c_str() << std::endl;
							printByteAndRaw(&iid.iid, sizeof(IMAGE_IMPORT_DESCRIPTOR));
						}
					}
					else
						std::cout << "\'" << result[1] << "\' 은(는) 올바른 옵션이 아닙니다." <<
						std::endl << "도움말을 보려면 IDT -h를 입력하세요.\n" << std::endl;
				}
				else
				{
					std::cout << "올바른 옵션 형식이 아닙니다." <<
						std::endl << "도움말을 보려면 INT -h를 입력하세요.\n" << std::endl;
				}
			}
			else if (result[0] == "INT")
			{
				if (result.size() == 1)
					printImportNameTable();
				else if (result.size() == 2)
				{
					if (result[1] == "-H")
						printHelp(result[0]);
					else if (result[1] == "-R")
					{
						if (x86)
							printRaw(m_INTx86.data(), m_INTx86.size(), 0, sizeof(INT_Elementx86), sizeof(DWORD));
						else
							printRaw(m_INT.data(), m_INT.size(), 0, sizeof(INT_Element), sizeof(ULONGLONG));
					}
					else if (result[1] == "-B")
					{
						if (x86)
							printByte(m_INTx86.data(), m_INTx86.size(), 0, sizeof(INT_Elementx86), sizeof(DWORD));
						else
							printByte(m_INT.data(), m_INT.size(), 0, sizeof(INT_Element), sizeof(ULONGLONG));
					}
					else if (result[1] == "-RB")
					{
						if (x86)
							printByteAndRaw(m_INTx86.data(), m_INTx86.size(), 0, sizeof(INT_Elementx86), sizeof(DWORD));
						else
							printByteAndRaw(m_INT.data(), m_INT.size(), 0, sizeof(INT_Element), sizeof(ULONGLONG));
					}
					else
						std::cout << "\'" << result[1] << "\' 은(는) 올바른 옵션이 아닙니다." <<
						std::endl << "도움말을 보려면 INT -h를 입력하세요.\n" << std::endl;
				}
				else
				{
					std::cout << "올바른 옵션 형식이 아닙니다." <<
						std::endl << "도움말을 보려면 INT -h를 입력하세요.\n" << std::endl;
				}
			}
			else if (result[0] == "IAT")
			{
				if (!hasIAT)
				{
					std::cout << "IAT가 INT로 대체 되었습니다.\n" << std::endl;
					continue;
				}
				if (result.size() == 1)
					printImportAddressTable();
				else if (result.size() == 2)
				{
					if (result[1] == "-H")
						printHelp(result[0]);
					else if (result[1] == "-R")
					{
						if (x86)
							printRaw(m_IATx86.data(), m_IATx86.size(), 0, sizeof(DWORD), sizeof(DWORD));
						else
							printRaw(m_IAT.data(), m_IAT.size(), 0, sizeof(ULONGLONG), sizeof(ULONGLONG));
					}
					else if (result[1] == "-B")
					{
						if (x86)
							printByte(m_IATx86.data(), m_IATx86.size(), 0, sizeof(DWORD), sizeof(DWORD));
						else
							printByte(m_IAT.data(), m_IAT.size(), 0, sizeof(ULONGLONG), sizeof(ULONGLONG));
					}
					else if (result[1] == "-RB")
					{
						if (x86)
							printByteAndRaw(m_IATx86.data(), m_IATx86.size(), 0, sizeof(DWORD), sizeof(DWORD));
						else
							printByteAndRaw(m_IAT.data(), m_IAT.size(), 0, sizeof(ULONGLONG), sizeof(ULONGLONG));
					}
					else
						std::cout << "\'" << result[1] << "\' 은(는) 올바른 옵션이 아닙니다." <<
						std::endl << "도움말을 보려면 IAT -h를 입력하세요.\n" << std::endl;
				}
				else
				{
					std::cout << "올바른 옵션 형식이 아닙니다." <<
						std::endl << "도움말을 보려면 IAT -h를 입력하세요.\n" << std::endl;
				}
			}
			else if (result[0] == "IED")
			{
				if (!exportDir)
				{
					std::cout << "Export Directory가 존재하지 않습니다.\n" << std::endl;
					continue;
				}
				if (result.size() == 1)
					printExportDirectory();
				else if (result.size() == 2)
				{
					if (result[1] == "-H")
						printHelp(result[0]);
					else if (result[1] == "-R")
						printRaw(&m_IED, sizeof(IMAGE_EXPORT_DIRECTORY));
					else if (result[1] == "-B")
						printByte(&m_IED, sizeof(IMAGE_EXPORT_DIRECTORY));
					else if (result[1] == "-RB")
						printByteAndRaw(&m_IED, sizeof(IMAGE_EXPORT_DIRECTORY));
					else
						std::cout << "\'" << result[1] << "\' 은(는) 올바른 옵션이 아닙니다." <<
						std::endl << "도움말을 보려면 IED -h를 입력하세요.\n" << std::endl;
				}
				else
				{
					std::cout << "올바른 옵션 형식이 아닙니다." <<
						std::endl << "도움말을 보려면 IED -h를 입력하세요.\n" << std::endl;
				}
			}
			else if (result[0] == "EAT")
			{
				if (!exportDir)
				{
					std::cout << "Export Directory가 존재하지 않습니다.\n" << std::endl;
					continue;
				}
				if (result.size() == 1)
					printExportAddressTable();
				else if (result.size() == 2)
				{
					if (result[1] == "-H")
						printHelp(result[0]);
					else if (result[1] == "-R")
						printRaw(m_ExportTable.data(), m_ExportTable.size(), 8, sizeof(ExportElement), sizeof(DWORD));
					else if (result[1] == "-B")
						printByte(m_ExportTable.data(), m_ExportTable.size(), 8, sizeof(ExportElement), sizeof(DWORD));
					else if (result[1] == "-RB")
						printByteAndRaw(m_ExportTable.data(), m_ExportTable.size(), 8, sizeof(ExportElement), sizeof(DWORD));
					else
						std::cout << "\'" << result[1] << "\' 은(는) 올바른 옵션이 아닙니다." <<
						std::endl << "도움말을 보려면 EAT -h를 입력하세요.\n" << std::endl;
				}
				else
				{
					std::cout << "올바른 옵션 형식이 아닙니다." <<
						std::endl << "도움말을 보려면 EAT -h를 입력하세요.\n" << std::endl;
				}
			}
			else if (result[0] == "ENT")
			{
				if (!exportDir)
				{
					std::cout << "Export Directory가 존재하지 않습니다.\n" << std::endl;
					continue;
				}
				if (result.size() == 1)
					printExportNameTable();
				else if (result.size() == 2)
				{
					if (result[1] == "-H")
						printHelp(result[0]);
					else if (result[1] == "-R")
						printRaw(m_ExportTable.data(), m_ExportTable.size(), 4, sizeof(ExportElement), sizeof(DWORD));
					else if (result[1] == "-B")
						printByte(m_ExportTable.data(), m_ExportTable.size(), 4, sizeof(ExportElement), sizeof(DWORD));
					else if (result[1] == "-RB")
						printByteAndRaw(m_ExportTable.data(), m_ExportTable.size(), 4, sizeof(ExportElement), sizeof(DWORD));
					else
						std::cout << "\'" << result[1] << "\' 은(는) 올바른 옵션이 아닙니다." <<
						std::endl << "도움말을 보려면 ENT -h를 입력하세요.\n" << std::endl;
				}
				else
				{
					std::cout << "올바른 옵션 형식이 아닙니다." <<
						std::endl << "도움말을 보려면 ENT -h를 입력하세요.\n" << std::endl;
				}
			}
			else if (result[0] == "EOT")
			{
				if (!exportDir)
				{
					std::cout << "Export Directory가 존재하지 않습니다.\n" << std::endl;
					continue;
				}
				if (result.size() == 1)
					printExportOrdinalTable();
				else if (result.size() == 2)
				{
					if (result[1] == "-H")
						printHelp(result[0]);
					else if (result[1] == "-R")
						printRaw(m_ExportTable.data(), m_ExportTable.size(), 0, sizeof(ExportElement), sizeof(WORD));
					else if (result[1] == "-B")
						printByte(m_ExportTable.data(), m_ExportTable.size(), 0, sizeof(ExportElement), sizeof(WORD));
					else if (result[1] == "-RB")
						printByteAndRaw(m_ExportTable.data(), m_ExportTable.size(), 0, sizeof(ExportElement), sizeof(WORD));
					else
						std::cout << "\'" << result[1] << "\' 은(는) 올바른 옵션이 아닙니다." <<
						std::endl << "도움말을 보려면 EOT -h를 입력하세요.\n" << std::endl;
				}
				else
				{
					std::cout << "올바른 옵션 형식이 아닙니다." <<
						std::endl << "도움말을 보려면 EOT -h를 입력하세요.\n" << std::endl;
				}
			}
			else if (result[0] == "HELP")
				printHelp("help");
			else if (result[0] == "EXIT")
				break;
			else if (result[0] == "CLS")
				system("cls");
		}
		else
		{
			std::cout << "\'" << cmd << "\'은(는) 올바른 명령어가 아닙니다.\n" << std::endl;
		}

	}
}

bool PEFile::readDosHeader(std::ifstream& file)
{
	file.seekg(0, std::ios::beg);
	file.read(reinterpret_cast<char*>(&m_dosHeader), sizeof(IMAGE_DOS_HEADER));
	
	return file.good();
}

bool PEFile::readDosStub(std::ifstream& file)
{
	int size = m_dosHeader.e_lfanew - sizeof(IMAGE_DOS_HEADER);
	m_dosStubByte = std::string(size, '\0');

	file.seekg(sizeof(IMAGE_DOS_HEADER), std::ios::beg);
	file.read(&m_dosStubByte[0], size);

	return file.good();
}

bool PEFile::readNtHeaders(std::ifstream& file)
{
	file.seekg(m_dosHeader.e_lfanew, std::ios::beg);
	file.read(reinterpret_cast<char*>(&m_ntHeaders), sizeof(IMAGE_NT_HEADERS));
	
	return file.good();
}

bool PEFile::readNtHeaderx86(std::ifstream& file)
{
	file.seekg(m_dosHeader.e_lfanew, std::ios::beg);
	file.read(reinterpret_cast<char*>(&m_ntHeaderx86), sizeof(IMAGE_NT_HEADERS32));
	
	return file.good();
}

bool PEFile::readSectionHeaders(std::ifstream& file)
{
	for (int i = 0; i < m_ntHeaders.FileHeader.NumberOfSections; ++i)
	{
		if (x86)
		{
			IMAGE_SECTION_HEADER header;
			file.seekg(m_dosHeader.e_lfanew + sizeof(IMAGE_NT_HEADERS32) + sizeof(IMAGE_SECTION_HEADER) * i, std::ios::beg);
			file.read(reinterpret_cast<char*>(&header), sizeof(IMAGE_SECTION_HEADER));
			m_sectionHeaders.push_back(header);
		}
		else
		{
			IMAGE_SECTION_HEADER header;
			file.seekg(m_dosHeader.e_lfanew + sizeof(IMAGE_NT_HEADERS64) + sizeof(IMAGE_SECTION_HEADER) * i, std::ios::beg);
			file.read(reinterpret_cast<char*>(&header), sizeof(IMAGE_SECTION_HEADER));
			m_sectionHeaders.push_back(header);
		}
	}
	return file.good();
}

bool PEFile::readDirectory(std::ifstream& file)
{
	if (x86)
	{
		for (int i = 0; i < m_ntHeaderx86.OptionalHeader.NumberOfRvaAndSizes; ++i)
		{
			switch (i)
			{
			case 0x0:
				readExportDirectory(file, m_ntHeaderx86.OptionalHeader.DataDirectory[i].VirtualAddress);
				break;
			case 0x1:
				readImportDirectory(file, m_ntHeaderx86.OptionalHeader.DataDirectory[i].VirtualAddress);
				break;
			case 0x2:
				return "RESOURCE Directory";
			case 0x3:
				return "EXCEPTION Directory";
			case 0x4:
				return "SECURITY Directory";
			case 0x5:
				return "BASE RELOCATION Directory";
			case 0x6:
				return "DEBUG Directory";
			case 0x7:
				return "COPYRIGHT Directory";
			case 0x8:
				return "GLOBALPTR Directory";
			case 0x9:
				return "TLS Directory";
			case 0xa:
				return "LOAD CONFIG Directory";
			case 0xb:
				return "BOUND IMPORT Directory";
			case 0xc:
				return "IAT Directory";
			case 0xd:
				return "DELAY IMPORT Directory";
			case 0xe:
				return "COM DESCRIPTOR Directory";
			case 0xf:
				return "Reserved Directory";
			}
		}
	}
	else
	{
		for (int i = 0; i < m_ntHeaders.OptionalHeader.NumberOfRvaAndSizes; ++i)
		{
			switch (i)
			{
			case 0x0:
				readExportDirectory(file, m_ntHeaders.OptionalHeader.DataDirectory[i].VirtualAddress);
				break;
			case 0x1:
				readImportDirectory(file, m_ntHeaders.OptionalHeader.DataDirectory[i].VirtualAddress);
				break;
			case 0x2:
				return "RESOURCE Directory";
			case 0x3:
				return "EXCEPTION Directory";
			case 0x4:
				return "SECURITY Directory";
			case 0x5:
				return "BASE RELOCATION Directory";
			case 0x6:
				return "DEBUG Directory";
			case 0x7:
				return "COPYRIGHT Directory";
			case 0x8:
				return "GLOBALPTR Directory";
			case 0x9:
				return "TLS Directory";
			case 0xa:
				return "LOAD CONFIG Directory";
			case 0xb:
				return "BOUND IMPORT Directory";
			case 0xc:
				return "IAT Directory";
			case 0xd:
				return "DELAY IMPORT Directory";
			case 0xe:
				return "COM DESCRIPTOR Directory";
			case 0xf:
				return "Reserved Directory";
			}
		}
	}

	return file.good();
}

bool PEFile::readExportDirectory(std::ifstream& file, DWORD rva)
{
	if (rva != NULL)
	{
		file.seekg(rva2raw(rva), std::ios::beg);
		file.read(reinterpret_cast<char*>(&m_IED), sizeof(IMAGE_EXPORT_DIRECTORY));

		file.seekg(rva2raw(m_IED.Name), std::ios::beg);
		file >> m_exportName;
		
		for (int i = 0; i < m_IED.NumberOfNames; ++i)
		{
			ExportElement en;

			file.seekg(rva2raw(m_IED.AddressOfNames) + sizeof(DWORD) * i, std::ios::beg);
			file.read(reinterpret_cast<char*>(&en.rva), sizeof(DWORD));

			file.seekg(rva2raw(en.rva), std::ios::beg);
			file >> en.name;

			file.seekg(rva2raw(m_IED.AddressOfNameOrdinals) + sizeof(WORD) * i, std::ios::beg);
			file.read(reinterpret_cast<char*>(&en.ordinal), sizeof(WORD));

			file.seekg(rva2raw(m_IED.AddressOfFunctions) + en.ordinal * sizeof(DWORD), std::ios::beg);
			file.read(reinterpret_cast<char*>(&en.funcAddr), sizeof(DWORD));

			m_ExportTable.push_back(en);
		}

		exportDir = true;
	}
	
	return file.good();
}

bool PEFile::readImportDirectory(std::ifstream& file, DWORD rva)
{
	int i = 0;
	IIDX iidx;
	while (true)
	{
		file.seekg(rva2raw(rva) + sizeof(IMAGE_IMPORT_DESCRIPTOR) * i++, std::ios::beg);
		file.read(reinterpret_cast<char*>(&iidx.iid), sizeof(IMAGE_IMPORT_DESCRIPTOR));
		
		if (!iidx.iid.FirstThunk)
			break;

		file.seekg(rva2raw(iidx.iid.Name), std::ios::beg);
		file >> iidx.Name;
		m_IIDs.push_back(iidx);

		// read INT
		int cnt = 0;
		while (true)
		{
			if (x86)
			{
				if (iidx.iid.OriginalFirstThunk != NULL)
				{
					hasIAT = true;

					INT_Elementx86 element;
					file.seekg(rva2raw(iidx.iid.OriginalFirstThunk) + sizeof(DWORD) * cnt, std::ios::beg);
					file.read(reinterpret_cast<char*>(&element.addr), sizeof(DWORD));

					// IAT
					DWORD iatRva;
					file.seekg(rva2raw(iidx.iid.FirstThunk) + sizeof(DWORD) * cnt, std::ios::beg);
					file.read(reinterpret_cast<char*>(&iatRva), sizeof(DWORD));
					m_IATx86.push_back(iatRva);
					cnt++;

					if (!element.addr)
						break;

					if (element.addr & 0x80000000) // Ordinal
					{
						m_INTx86.push_back(element);
						continue;
					}

					file.seekg(rva2raw(element.addr), std::ios::beg);
					file.read(reinterpret_cast<char*>(&element.Hint), sizeof(WORD));

					file.seekg(rva2raw(element.addr) + sizeof(WORD), std::ios::beg);
					file >> element.Name;

					m_INTx86.push_back(element);
				}
				else
				{
					INT_Elementx86 element;
					file.seekg(rva2raw(iidx.iid.FirstThunk) + sizeof(DWORD) * cnt, std::ios::beg);
					file.read(reinterpret_cast<char*>(&element.addr), sizeof(DWORD));
					cnt++;

					if (!element.addr)
						break;

					if (element.addr & 0x80000000) // Ordinal
					{
						m_INTx86.push_back(element);
						continue;
					}

					file.seekg(rva2raw(element.addr), std::ios::beg);
					file.read(reinterpret_cast<char*>(&element.Hint), sizeof(WORD));

					file.seekg(rva2raw(element.addr) + sizeof(WORD), std::ios::beg);
					file >> element.Name;

					m_INTx86.push_back(element);
				}
			}
			else
			{
				if (iidx.iid.OriginalFirstThunk != NULL)
				{
					hasIAT = true;

					INT_Element element;
					file.seekg(rva2raw(iidx.iid.OriginalFirstThunk) + sizeof(ULONGLONG) * cnt, std::ios::beg);
					file.read(reinterpret_cast<char*>(&element.addr), sizeof(ULONGLONG));

					// IAT
					ULONGLONG iatRva;
					file.seekg(rva2raw(iidx.iid.FirstThunk) + sizeof(ULONGLONG) * cnt, std::ios::beg);
					file.read(reinterpret_cast<char*>(&iatRva), sizeof(ULONGLONG));
					m_IAT.push_back(iatRva);
					cnt++;

					if (!element.addr)
						break;

					if (element.addr & 0x8000000000000000) // Ordinal
					{
						m_INT.push_back(element);
						continue;
					}

					file.seekg(rva2raw(element.addr), std::ios::beg);
					file.read(reinterpret_cast<char*>(&element.Hint), sizeof(WORD));

					file.seekg(rva2raw(element.addr) + sizeof(WORD), std::ios::beg);
					file >> element.Name;

					m_INT.push_back(element);
				}
				else
				{
					INT_Element element;
					file.seekg(rva2raw(iidx.iid.FirstThunk) + sizeof(ULONGLONG) * cnt, std::ios::beg);
					file.read(reinterpret_cast<char*>(&element.addr), sizeof(ULONGLONG));
					cnt++;

					if (!element.addr)
						break;

					if (element.addr & 0x8000000000000000) // Ordinal
					{
						m_INT.push_back(element);
						continue;
					}

					file.seekg(rva2raw(element.addr), std::ios::beg);
					file.read(reinterpret_cast<char*>(&element.Hint), sizeof(WORD));

					file.seekg(rva2raw(element.addr) + sizeof(WORD), std::ios::beg);
					file >> element.Name;

					m_INT.push_back(element);
				}
			}
		}

		if (x86)
		{
			INT_Elementx86 element;
			element.addr = 0;
			element.Hint = 0;
			element.Name = iidx.Name;
			m_INTx86.push_back(element);
		}
		else
		{
			INT_Element element;
			element.addr = 0;
			element.Hint = 0;
			element.Name = iidx.Name;
			m_INT.push_back(element);
		}
	};

	return file.good();
}

void PEFile::printDosHeader()
{
	printf("%-8s | %-32s | %-32s\n", "Data", "Description", "Value");
	std::cout << std::string(70, '-') << std::endl;
	printf(x86_4byte_desc32, m_dosHeader.e_magic, "Magic number", "IMAGE_DOS_SIGNATURE");
	printf(x86_4byte_desc32, m_dosHeader.e_cblp, "Bytes on last page of file", "\0");
	printf(x86_4byte_desc32, m_dosHeader.e_cp, "Pages in file", "\0");
	printf(x86_4byte_desc32, m_dosHeader.e_crlc, "Relocations", "\0");
	printf(x86_4byte_desc32, m_dosHeader.e_cparhdr, "Size of header in paragraphs", "\0");
	printf(x86_4byte_desc32, m_dosHeader.e_minalloc, "Minimum extra paragraphs needed", "\0");
	printf(x86_4byte_desc32, m_dosHeader.e_maxalloc, "Maximum extra paragraphs needed", "\0");
	printf(x86_4byte_desc32, m_dosHeader.e_ss, "Initial (relative) SS", "\0");
	printf(x86_4byte_desc32, m_dosHeader.e_sp, "Initial SP", "\0");
	printf(x86_4byte_desc32, m_dosHeader.e_csum, "Checksum", "\0");
	printf(x86_4byte_desc32, m_dosHeader.e_ip, "Initial IP", "\0");
	printf(x86_4byte_desc32, m_dosHeader.e_cs, "Initial (relative) CS", "\0");
	printf(x86_4byte_desc32, m_dosHeader.e_lfarlc, "File address of relocation table", "\0");
	printf(x86_4byte_desc32, m_dosHeader.e_ovno, "Overlay number", "\0");
	printf(x86_4byte_desc32, m_dosHeader.e_res[0], "Reserved words", "\0");
	printf(x86_4byte_desc32, m_dosHeader.e_res[1], "Reserved words", "\0");
	printf(x86_4byte_desc32, m_dosHeader.e_res[2], "Reserved words", "\0");
	printf(x86_4byte_desc32, m_dosHeader.e_res[3], "Reserved words", "\0");
	printf(x86_4byte_desc32, m_dosHeader.e_oemid, "OEM identifier", "\0");
	printf(x86_4byte_desc32, m_dosHeader.e_oeminfo, "OEM information", "\0");
	printf(x86_4byte_desc32, m_dosHeader.e_res2[0], "Reserved words", "\0");
	printf(x86_4byte_desc32, m_dosHeader.e_res2[1], "Reserved words", "\0");
	printf(x86_4byte_desc32, m_dosHeader.e_res2[2], "Reserved words", "\0");
	printf(x86_4byte_desc32, m_dosHeader.e_res2[3], "Reserved words", "\0");
	printf(x86_4byte_desc32, m_dosHeader.e_res2[4], "Reserved words", "\0");
	printf(x86_4byte_desc32, m_dosHeader.e_res2[5], "Reserved words", "\0");
	printf(x86_4byte_desc32, m_dosHeader.e_res2[6], "Reserved words", "\0");
	printf(x86_4byte_desc32, m_dosHeader.e_res2[7], "Reserved words", "\0");
	printf(x86_4byte_desc32, m_dosHeader.e_res2[8], "Reserved words", "\0");
	printf(x86_4byte_desc32, m_dosHeader.e_res2[9], "Reserved words", "\0");
	printf(x86_8byte_desc32, m_dosHeader.e_lfanew, "File address of new exe header", "\0");
	printf("\n");
}

void PEFile::printNtHeaders()
{
	printf("%-8s | %-16s | %-24s\n", "Data", "Description", "Value");
	std::cout << std::string(54, '-') << std::endl;
	
	if (x86)
	{
		printf(x86_8byte_desc16, m_ntHeaderx86.Signature, "Signature", "IMAGE_NT_SIGNATURE");
		printf(x86_8str_desc16, "<struct>", "FileHeader", "IMAGE_FILE_HEADER");
		printf(x86_8str_desc16, "<struct>" , "OptionalHeader", "IMAGE_OPTIONAL_HEADER32");
	}
	else
	{
		printf(x86_8byte_desc16, m_ntHeaders.Signature, "Signature", "IMAGE_NT_SIGNATURE");
		printf(x86_8str_desc16, "<struct>", "FileHeader", "IMAGE_FILE_HEADER");
		printf(x86_8str_desc16, "<struct>", "OptionalHeader", "IMAGE_OPTIONAL_HEADER64");
	}
	printf("\n");
}

void PEFile::printFileHeader()
{
	time_t timer = static_cast<time_t>(m_ntHeaders.FileHeader.TimeDateStamp);
	struct tm* t = gmtime(&timer);
	char buffer[80];
	strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", t);

	printf("%-8s | %-24s | %-24s\n", "Data", "Description", "Value");
	std::cout << std::string(62, '-') << std::endl;
	printf(x86_4byte_desc24, m_ntHeaders.FileHeader.Machine, "Machine", m_machineStr.c_str());
	printf(x86_4byte_desc24, m_ntHeaders.FileHeader.NumberOfSections, "Number Of Sections", "\0");
	printf(x86_8byte_desc24, m_ntHeaders.FileHeader.TimeDateStamp, "Time Date Stamp", buffer);
	printf("\n");
}

void PEFile::printOptionalHeader()
{	
	if (x86) 
	{
		printf("%-8s | %-32s | %-32s\n", "Data", "Description", "Value");
		std::cout << std::string(78, '-') << std::endl;
		printf(x86_4byte_desc32_v32, m_ntHeaderx86.OptionalHeader.Magic, "Magic", "IMAGE_NT_OPTIONAL_HDR32_MAGIC");
		printf(x86_2byte_desc32_v32, m_ntHeaderx86.OptionalHeader.MajorLinkerVersion, "Major Linker Version", "\0");
		printf(x86_2byte_desc32_v32, m_ntHeaderx86.OptionalHeader.MinorLinkerVersion, "Minor Linker Version", "\0");
		printf(x86_8byte_desc32_v32, m_ntHeaderx86.OptionalHeader.SizeOfCode, "Size of Code", "\0");
		printf(x86_8byte_desc32_v32, m_ntHeaderx86.OptionalHeader.SizeOfInitializedData, "Size of Initialized Data", "\0");
		printf(x86_8byte_desc32_v32, m_ntHeaderx86.OptionalHeader.SizeOfUninitializedData, "Size of Uninitialized Data", "\0");
		printf(x86_8byte_desc32_v32, m_ntHeaderx86.OptionalHeader.AddressOfEntryPoint, "Address of EntryPoint", "\0");
		printf(x86_8byte_desc32_v32, m_ntHeaderx86.OptionalHeader.BaseOfCode, "Base of Code", "\0");
		printf(x86_8byte_desc32_v32, m_ntHeaderx86.OptionalHeader.BaseOfData, "Base of Data", "\0");
		printf(x86_8byte_desc32_v32, m_ntHeaderx86.OptionalHeader.ImageBase, "Image Base", "\0");
		printf(x86_8byte_desc32_v32, m_ntHeaderx86.OptionalHeader.SectionAlignment, "Section Alignment", "\0");
		printf(x86_8byte_desc32_v32, m_ntHeaderx86.OptionalHeader.FileAlignment, "File Alignment", "\0");
		printf(x86_4byte_desc32_v32, m_ntHeaderx86.OptionalHeader.MajorOperatingSystemVersion, "Major OS Version", "\0");
		printf(x86_4byte_desc32_v32, m_ntHeaderx86.OptionalHeader.MinorOperatingSystemVersion, "Minor OS Version", "\0");
		printf(x86_4byte_desc32_v32, m_ntHeaderx86.OptionalHeader.MajorImageVersion, "Major Image Version", "\0");
		printf(x86_4byte_desc32_v32, m_ntHeaderx86.OptionalHeader.MinorImageVersion, "Minor Image Version", "\0");
		printf(x86_4byte_desc32_v32, m_ntHeaderx86.OptionalHeader.MajorSubsystemVersion, "Major Subsystem Version", "\0");
		printf(x86_4byte_desc32_v32, m_ntHeaderx86.OptionalHeader.MinorSubsystemVersion, "Minor Subsystem Version", "\0");
		printf(x86_8byte_desc32_v32, m_ntHeaderx86.OptionalHeader.Win32VersionValue, "Win32 Version Value", "\0");
		printf(x86_8byte_desc32_v32, m_ntHeaderx86.OptionalHeader.SizeOfImage, "Size of Image", "\0");
		printf(x86_8byte_desc32_v32, m_ntHeaderx86.OptionalHeader.SizeOfHeaders, "Size of Headers", "\0");
		printf(x86_8byte_desc32_v32, m_ntHeaderx86.OptionalHeader.CheckSum, "Checksum", "\0");
		printf(x86_4byte_desc32_v32, m_ntHeaderx86.OptionalHeader.Subsystem, "Subsystem", m_subSystem.c_str());
		printf(x86_4byte_desc32_v32, m_ntHeaderx86.OptionalHeader.DllCharacteristics, "DLL Characteristics", "\0");
		printf(x86_8byte_desc32_v32, m_ntHeaderx86.OptionalHeader.SizeOfStackReserve, "Size of StackReserve", "\0");
		printf(x86_8byte_desc32_v32, m_ntHeaderx86.OptionalHeader.SizeOfStackCommit, "Size of Stack Commit", "\0");
		printf(x86_8byte_desc32_v32, m_ntHeaderx86.OptionalHeader.SizeOfHeapReserve, "Size of Heap Reserve", "\0");
		printf(x86_8byte_desc32_v32, m_ntHeaderx86.OptionalHeader.SizeOfHeapCommit, "Size of Heap Commit", "\0");
		printf(x86_8byte_desc32_v32, m_ntHeaderx86.OptionalHeader.LoaderFlags, "Loader Flags", "\0");
		printf(x86_8byte_desc32_v32, m_ntHeaderx86.OptionalHeader.NumberOfRvaAndSizes, "Number of Data Directory", "\0");
		for (int i = 0; i < m_ntHeaderx86.OptionalHeader.NumberOfRvaAndSizes; ++i)
		{
			std::cout << std::string(78, '-') << std::endl;
			printf(x86_8byte_desc32_v32, m_ntHeaderx86.OptionalHeader.DataDirectory[i].VirtualAddress, "Virtual Address", GetDirectoryName(i).c_str());
			printf(x86_8byte_desc32_v32, m_ntHeaderx86.OptionalHeader.DataDirectory[i].Size, "Size", "\0");
		}
	}
	else
	{
		printf("%-16s | %-32s | %-32s\n", "Data", "Description", "Value");
		std::cout << std::string(86, '-') << std::endl;
		printf(x64_4byte_desc32_v32, m_ntHeaders.OptionalHeader.Magic, "Magic", "IMAGE_NT_OPTIONAL_HDR64_MAGIC");
		printf(x64_2byte_desc32_v32, m_ntHeaders.OptionalHeader.MajorLinkerVersion, "Major Linker Version", "\0");
		printf(x64_2byte_desc32_v32, m_ntHeaders.OptionalHeader.MinorLinkerVersion, "Minor Linker Version", "\0");
		printf(x64_8byte_desc32_v32, m_ntHeaders.OptionalHeader.SizeOfCode, "Size of Code", "\0");
		printf(x64_8byte_desc32_v32, m_ntHeaders.OptionalHeader.SizeOfInitializedData, "Size of Initialized Data", "\0");
		printf(x64_8byte_desc32_v32, m_ntHeaders.OptionalHeader.SizeOfUninitializedData, "Size of Uninitialized Data", "\0");
		printf(x64_8byte_desc32_v32, m_ntHeaders.OptionalHeader.AddressOfEntryPoint, "Address of EntryPoint", "\0");
		printf(x64_8byte_desc32_v32, m_ntHeaders.OptionalHeader.BaseOfCode, "Base of Code", "\0");
		printf(x64_16byte_desc32_v32, m_ntHeaders.OptionalHeader.ImageBase, "Image Base", "\0");
		printf(x64_8byte_desc32_v32, m_ntHeaders.OptionalHeader.SectionAlignment, "Section Alignment", "\0");
		printf(x64_8byte_desc32_v32, m_ntHeaders.OptionalHeader.FileAlignment, "File Alignment", "\0");
		printf(x64_4byte_desc32_v32, m_ntHeaders.OptionalHeader.MajorOperatingSystemVersion, "Major OS Version", "\0");
		printf(x64_4byte_desc32_v32, m_ntHeaders.OptionalHeader.MinorOperatingSystemVersion, "Minor OS Version", "\0");
		printf(x64_4byte_desc32_v32, m_ntHeaders.OptionalHeader.MajorImageVersion, "Major Image Version", "\0");
		printf(x64_4byte_desc32_v32, m_ntHeaders.OptionalHeader.MinorImageVersion, "Minor Image Version", "\0");
		printf(x64_4byte_desc32_v32, m_ntHeaders.OptionalHeader.MajorSubsystemVersion, "Major Subsystem Version", "\0");
		printf(x64_4byte_desc32_v32, m_ntHeaders.OptionalHeader.MinorSubsystemVersion, "Minor Subsystem Version", "\0");
		printf(x64_8byte_desc32_v32, m_ntHeaders.OptionalHeader.Win32VersionValue, "Win32 Version Value", "\0");
		printf(x64_8byte_desc32_v32, m_ntHeaders.OptionalHeader.SizeOfImage, "Size of Image", "\0");
		printf(x64_8byte_desc32_v32, m_ntHeaders.OptionalHeader.SizeOfHeaders, "Size of Headers", "\0");
		printf(x64_8byte_desc32_v32, m_ntHeaders.OptionalHeader.CheckSum, "Checksum", "\0");
		printf(x64_4byte_desc32_v32, m_ntHeaders.OptionalHeader.Subsystem, "Subsystem", m_subSystem.c_str());
		printf(x64_4byte_desc32_v32, m_ntHeaders.OptionalHeader.DllCharacteristics, "DLL Characteristics", "\0");
		printf(x64_16byte_desc32_v32, m_ntHeaders.OptionalHeader.SizeOfStackReserve, "Size of StackReserve", "\0");
		printf(x64_16byte_desc32_v32, m_ntHeaders.OptionalHeader.SizeOfStackCommit, "Size of Stack Commit", "\0");
		printf(x64_16byte_desc32_v32, m_ntHeaders.OptionalHeader.SizeOfHeapReserve, "Size of Heap Reserve", "\0");
		printf(x64_16byte_desc32_v32, m_ntHeaders.OptionalHeader.SizeOfHeapCommit, "Size of Heap Commit", "\0");
		printf(x64_8byte_desc32_v32, m_ntHeaders.OptionalHeader.LoaderFlags, "Loader Flags", "\0");
		printf(x64_8byte_desc32_v32, m_ntHeaders.OptionalHeader.NumberOfRvaAndSizes, "Number of Data Directory", "\0");
		for (int i = 0; i < m_ntHeaders.OptionalHeader.NumberOfRvaAndSizes; ++i)
		{
			std::cout << std::string(86, '-') << std::endl;
			printf(x64_8byte_desc32_v32, m_ntHeaders.OptionalHeader.DataDirectory[i].VirtualAddress, "Virtual Address", GetDirectoryName(i).c_str());
			printf(x64_8byte_desc32_v32, m_ntHeaders.OptionalHeader.DataDirectory[i].Size, "Size", "\0");
		}
	}
	printf("\n");
}

void PEFile::printSectionHeader(IMAGE_SECTION_HEADER& header)
{
	printf("%-12s | %-32s | %-32s\n", "Data", "Description", "Value");
	std::cout << std::string(82, '-') << std::endl;
	
	printf(" ");
	for (int i = 0; i < 4; ++i)
		printf("%02X ", header.Name[i]);
	printf("| %32s | %32s\n", "Name", header.Name);
	printf(" ");
	for (int i = 4; i < 8; ++i)
		printf("%02X ", header.Name[i]);
	printf("| %32s | %32s\n", "", "");
	
	std::cout << std::string(82, '-') << std::endl;
	printf("    %08X | %32s | %32s\n", header.Misc.VirtualSize, "Virtual Size", "\0");
	printf("    %08X | %32s | %32s\n", header.VirtualAddress, "Virtual Address", "\0");
	printf("    %08X | %32s | %32s\n", header.SizeOfRawData, "Size of Raw Data", "\0");
	printf("    %08X | %32s | %32s\n", header.PointerToRawData, "Pointer to Raw Data", "\0");
	printf("    %08X | %32s | %32s\n", header.PointerToRelocations, "Pointer to Relocations", "\0");
	printf("    %08X | %32s | %32s\n", header.PointerToLinenumbers, "Pointer to Line Numbers", "\0");
	printf("        %04X | %32s | %32s\n", header.NumberOfRelocations, "Number of Relocations", "\0");
	printf("        %04X | %32s | %32s\n", header.NumberOfLinenumbers, "Number of Linenumbers", "\0");
	printf("    %08X | %32s | %32s\n", header.Characteristics, "Characteristics", "\0");
	printCharacteristics(header.Characteristics);
	printf("\n");
}

void PEFile::printCharacteristics(DWORD characteristics)
{
	if (characteristics & IMAGE_SCN_TYPE_NO_PAD)
		printf("    %8s |                         %08X | %32s\n", "", IMAGE_SCN_TYPE_NO_PAD, "IMAGE_SCN_TYPE_NO_PAD");
	if (characteristics & IMAGE_SCN_CNT_CODE)
		printf("    %8s |                         %08X | %32s\n", "", IMAGE_SCN_CNT_CODE, "IMAGE_SCN_CNT_CODE");
	if (characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA)
		printf("    %8s |                         %08X | %32s\n", "", IMAGE_SCN_CNT_INITIALIZED_DATA, "IMAGE_SCN_CNT_INITIALIZED_DATA");
	if (characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA)
		printf("    %8s |                         %08X | %32s\n", "", IMAGE_SCN_CNT_UNINITIALIZED_DATA, "IMAGE_SCN_CNT_UNINITIALIZED_DATA");
	if (characteristics & IMAGE_SCN_LNK_OTHER)
		printf("    %8s |                         %08X | %32s\n", "", IMAGE_SCN_LNK_OTHER, "IMAGE_SCN_LNK_OTHER");
	if (characteristics & IMAGE_SCN_LNK_INFO)
		printf("    %8s |                         %08X | %32s\n", "", IMAGE_SCN_LNK_INFO, "IMAGE_SCN_LNK_INFO");
	if (characteristics & IMAGE_SCN_LNK_REMOVE)
		printf("    %8s |                         %08X | %32s\n", "", IMAGE_SCN_LNK_REMOVE, "IMAGE_SCN_LNK_REMOVE");
	if (characteristics & IMAGE_SCN_LNK_COMDAT)
		printf("    %8s |                         %08X | %32s\n", "", IMAGE_SCN_LNK_COMDAT, "IMAGE_SCN_LNK_COMDAT");
	if (characteristics & IMAGE_SCN_NO_DEFER_SPEC_EXC)
		printf("    %8s |                         %08X | %32s\n", "", IMAGE_SCN_NO_DEFER_SPEC_EXC, "IMAGE_SCN_NO_DEFER_SPEC_EXC");
	if (characteristics & IMAGE_SCN_GPREL)
		printf("    %8s |                         %08X | %32s\n", "", IMAGE_SCN_GPREL, "IMAGE_SCN_GPREL");
	if (characteristics & IMAGE_SCN_MEM_PURGEABLE)
		printf("    %8s |                         %08X | %32s\n", "", IMAGE_SCN_MEM_PURGEABLE, "IMAGE_SCN_MEM_PURGEABLE");
	if (characteristics & IMAGE_SCN_MEM_LOCKED)
		printf("    %8s |                         %08X | %32s\n", "", IMAGE_SCN_MEM_LOCKED, "IMAGE_SCN_MEM_LOCKED");
	if (characteristics & IMAGE_SCN_MEM_PRELOAD)
		printf("    %8s |                         %08X | %32s\n", "", IMAGE_SCN_MEM_PRELOAD, "IMAGE_SCN_MEM_PRELOAD");
	if (characteristics & IMAGE_SCN_ALIGN_1BYTES)
		printf("    %8s |                         %08X | %32s\n", "", IMAGE_SCN_ALIGN_1BYTES, "IMAGE_SCN_ALIGN_1BYTES");
	if (characteristics & IMAGE_SCN_ALIGN_2BYTES)
		printf("    %8s |                         %08X | %32s\n", "", IMAGE_SCN_ALIGN_2BYTES, "IMAGE_SCN_ALIGN_2BYTES");
	if (characteristics & IMAGE_SCN_ALIGN_4BYTES)
		printf("    %8s |                         %08X | %32s\n", "", IMAGE_SCN_ALIGN_4BYTES, "IMAGE_SCN_ALIGN_4BYTES");
	if (characteristics & IMAGE_SCN_ALIGN_8BYTES)
		printf("    %8s |                         %08X | %32s\n", "", IMAGE_SCN_ALIGN_8BYTES, "IMAGE_SCN_ALIGN_8BYTES");
	if (characteristics & IMAGE_SCN_ALIGN_16BYTES)
		printf("    %8s |                         %08X | %32s\n", "", IMAGE_SCN_ALIGN_16BYTES, "IMAGE_SCN_ALIGN_16BYTES");
	if (characteristics & IMAGE_SCN_ALIGN_32BYTES)
		printf("    %8s |                         %08X | %32s\n", "", IMAGE_SCN_ALIGN_32BYTES, "IMAGE_SCN_ALIGN_32BYTES");
	if (characteristics & IMAGE_SCN_ALIGN_64BYTES)
		printf("    %8s |                         %08X | %32s\n", "", IMAGE_SCN_ALIGN_64BYTES, "IMAGE_SCN_ALIGN_64BYTES");
	if (characteristics & IMAGE_SCN_ALIGN_128BYTES)
		printf("    %8s |                         %08X | %32s\n", "", IMAGE_SCN_ALIGN_128BYTES, "IMAGE_SCN_ALIGN_128BYTES");
	if (characteristics & IMAGE_SCN_ALIGN_256BYTES)
		printf("    %8s |                         %08X | %32s\n", "", IMAGE_SCN_ALIGN_256BYTES, "IMAGE_SCN_ALIGN_256BYTES");
	if (characteristics & IMAGE_SCN_ALIGN_512BYTES)
		printf("    %8s |                         %08X | %32s\n", "", IMAGE_SCN_ALIGN_512BYTES, "IMAGE_SCN_ALIGN_512BYTES");
	if (characteristics & IMAGE_SCN_ALIGN_1024BYTES)
		printf("    %8s |                         %08X | %32s\n", "", IMAGE_SCN_ALIGN_1024BYTES, "IMAGE_SCN_ALIGN_1024BYTES");
	if (characteristics & IMAGE_SCN_ALIGN_2048BYTES)
		printf("    %8s |                         %08X | %32s\n", "", IMAGE_SCN_ALIGN_2048BYTES, "IMAGE_SCN_ALIGN_2048BYTES");
	if (characteristics & IMAGE_SCN_ALIGN_4096BYTES)
		printf("    %8s |                         %08X | %32s\n", "", IMAGE_SCN_ALIGN_4096BYTES, "IMAGE_SCN_ALIGN_4096BYTES");
	if (characteristics & IMAGE_SCN_ALIGN_8192BYTES)
		printf("    %8s |                         %08X | %32s\n", "", IMAGE_SCN_ALIGN_8192BYTES, "IMAGE_SCN_ALIGN_8192BYTES");
	if (characteristics & IMAGE_SCN_ALIGN_MASK)
		printf("    %8s |                         %08X | %32s\n", "", IMAGE_SCN_ALIGN_MASK, "IMAGE_SCN_ALIGN_MASK");
	if (characteristics & IMAGE_SCN_LNK_NRELOC_OVFL)
		printf("    %8s |                         %08X | %32s\n", "", IMAGE_SCN_LNK_NRELOC_OVFL, "IMAGE_SCN_LNK_NRELOC_OVFL");
	if (characteristics & IMAGE_SCN_MEM_DISCARDABLE)
		printf("    %8s |                         %08X | %32s\n", "", IMAGE_SCN_MEM_DISCARDABLE, "IMAGE_SCN_MEM_DISCARDABLE");
	if (characteristics & IMAGE_SCN_MEM_NOT_CACHED)
		printf("    %8s |                         %08X | %32s\n", "", IMAGE_SCN_MEM_NOT_CACHED, "IMAGE_SCN_MEM_NOT_CACHED");
	if (characteristics & IMAGE_SCN_MEM_NOT_PAGED)
		printf("    %8s |                         %08X | %32s\n", "", IMAGE_SCN_MEM_NOT_PAGED, "IMAGE_SCN_MEM_NOT_PAGED");
	if (characteristics & IMAGE_SCN_MEM_SHARED)
		printf("    %8s |                         %08X | %32s\n", "", IMAGE_SCN_MEM_SHARED, "IMAGE_SCN_MEM_SHARED");
	if (characteristics & IMAGE_SCN_MEM_EXECUTE)
		printf("    %8s |                         %08X | %32s\n", "", IMAGE_SCN_MEM_EXECUTE, "IMAGE_SCN_MEM_EXECUTE");
	if (characteristics & IMAGE_SCN_MEM_READ)
		printf("    %8s |                         %08X | %32s\n", "", IMAGE_SCN_MEM_READ, "IMAGE_SCN_MEM_READ");
	if (characteristics & IMAGE_SCN_MEM_WRITE)
		printf("    %8s |                         %08X | %32s\n", "", IMAGE_SCN_MEM_WRITE, "IMAGE_SCN_MEM_WRITE");
}

void PEFile::printImportDirectoyTable()
{
	printf("%-8s | %-16s | %-24s\n", "Data", "Description", "Value");
	for (auto iid : m_IIDs)
	{
		std::cout << std::string(54, '-') << std::endl;
		printf(x86_8byte_desc16, iid.iid.OriginalFirstThunk, "RVA to INT", "\0");
		printf(x86_8byte_desc16, iid.iid.TimeDateStamp, "Time Date Stamp", iid.iid.TimeDateStamp ? "Bound" : "Not Bound");
		printf(x86_8byte_desc16, iid.iid.ForwarderChain, "Forwarder Chain", "\0");
		printf(x86_8byte_desc16, iid.iid.Name, "Name", iid.Name.c_str());
		printf(x86_8byte_desc16, iid.iid.FirstThunk, "RVA to IAT", "\0");
	}
	printf("\n");
}

void PEFile::printImportNameTable()
{
	if (x86)
	{
		printf("%-8s | %-16s | %-32s\n", "Data", "Description", "Value");
		std::cout << std::string(64, '-') << std::endl;
		for (auto element : m_INTx86)
		{
			if (element.addr & 0x80000000)
			{
				char ordinal[32];
				sprintf(ordinal, "%04X", element.addr & 0xFFFF);
				printf(x86_8str_desc16_v32, element.addr, "Ordinal", ordinal);
			}
			else if (element.addr == 0 && element.Hint == 0)
			{
				printf(x86_8str_desc16_v32, element.addr, "End of Imports", element.Name.c_str());
				std::cout << std::string(64, '-') << std::endl;
			}
			else
			{
				char text[64];
				sprintf(text, "%04X %s", element.Hint, element.Name.c_str());
				printf(x86_8str_desc16_v32, element.addr, "Hint/Name RVA", text);
			}
		}
	}
	else
	{
		printf("%-16s | %-16s | %-32s\n", "Data", "Description", "Value");
		std::cout << std::string(72, '-') << std::endl;
		for (auto element : m_INT)
		{
			if (element.addr & 0x8000000000000000)
			{
				char ordinal[32];
				sprintf(ordinal, "%04X", element.addr & 0xFFFF);
				printf(x64_16byte_desc16_v32, element.addr, "Ordinal", ordinal);
			}
			else if (element.addr == 0 && element.Hint == 0)
			{
				printf(x64_16byte_desc16_v32, element.addr, "End of Imports", element.Name.c_str());
				std::cout << std::string(72, '-') << std::endl;
			}
			else
			{
				char text[64];
				sprintf(text, "%04X %s", element.Hint, element.Name.c_str());
				printf(x64_16byte_desc16_v32, element.addr, "Hint/Name RVA", text);
			}
		}
	}
	printf("\n");
}

void PEFile::printImportAddressTable()
{
	if (x86)
	{
		printf("%-8s | %-16s | %-32s\n", "Data", "Description", "Value");
		std::cout << std::string(64, '-') << std::endl;
		for (int i = 0; i < m_INTx86.size(); ++i)
		{
			if (m_INTx86[i].addr & 0x80000000)
			{
				char ordinal[32];
				sprintf(ordinal, "%04X", m_INTx86[i].addr & 0xFFFF);
				printf(x86_8str_desc16_v32, m_IATx86[i], "Ordinal", ordinal);
			}
			else if (m_INTx86[i].addr == 0 && m_INTx86[i].Hint == 0)
			{
				printf(x86_8str_desc16_v32, 0, "End of Imports", m_INTx86[i].Name.c_str());
				std::cout << std::string(64, '-') << std::endl;
			}
			else
			{
				char text[64];
				sprintf(text, "%04X %s", m_INTx86[i].Hint, m_INTx86[i].Name.c_str());
				printf(x86_8str_desc16_v32, m_IATx86[i], "Hint/Name RVA", text);
			}
		}
	}
	else
	{
		printf("%-16s | %-16s | %-32s\n", "Data", "Description", "Value");
		std::cout << std::string(72, '-') << std::endl;
		for (int i = 0; i < m_INT.size(); ++i)
		{
			if (m_INT[i].addr & 0x8000000000000000)
			{
				char ordinal[32];
				sprintf(ordinal, "%04X", m_INT[i].addr & 0xFFFF);
				printf(x64_16byte_desc16_v32, m_IAT[i], "Ordinal", ordinal);
			}
			else if (m_INT[i].addr == 0 && m_INT[i].Hint == 0)
			{
				printf(x64_16byte_desc16_v32, 0, "End of Imports", m_INT[i].Name.c_str());
				std::cout << std::string(72, '-') << std::endl;
			}
			else
			{
				char text[64];
				sprintf(text, "%04X %s", m_INT[i].Hint, m_INT[i].Name.c_str());
				printf(x64_16byte_desc16_v32, m_IAT[i], "Hint/Name RVA", text);
			}
		}
	}
	printf("\n");
}

void PEFile::printExportDirectory()
{
	time_t timer = static_cast<time_t>(m_IED.TimeDateStamp);
	struct tm* t = gmtime(&timer);
	char buffer[80];
	strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", t);

	printf("%-8s | %-24s | %-24s\n", "Data", "Description", "Value");
	std::cout << std::string(64, '-') << std::endl;
	printf(x86_8byte_desc24, m_IED.Characteristics, "Characteristics", "\0");
	printf(x86_8byte_desc24, m_IED.TimeDateStamp, "Time Date Stamp", buffer);
	
	char text[16];
	sprintf(text, "%04X", m_IED.MajorVersion);
	printf(x86_8str_desc24, text, "Major Version", "\0");
	sprintf(text, "%04X", m_IED.MinorVersion);
	printf(x86_8str_desc24, text, "Minor Version", "\0");

	printf(x86_8byte_desc24, m_IED.Name, "Name", m_exportName.c_str());
	printf(x86_8byte_desc24, m_IED.Base, "Base", "\0");
	printf(x86_8byte_desc24, m_IED.NumberOfFunctions, "Number of Functions", "\0");
	printf(x86_8byte_desc24, m_IED.NumberOfNames, "Number of Names", "\0");
	printf(x86_8byte_desc24, m_IED.AddressOfFunctions, "Address of Functions", "\0");
	printf(x86_8byte_desc24, m_IED.AddressOfNames, "Address of Names", "\0");
	printf(x86_8byte_desc24, m_IED.AddressOfNameOrdinals, "Address of Name Ordinals", "\0");
	printf("\n");
}

void PEFile::printExportAddressTable()
{
	char text[64];
	printf("%-8s | %-16s | %-32s\n", "Data", "Description", "Value");
	std::cout << std::string(64, '-') << std::endl;
	for (auto ee : m_ExportTable)
	{
		sprintf(text, "%04X %s", ee.ordinal, ee.name.c_str());
		printf(x86_8byte_desc16, ee.funcAddr, "Function RVA", text);
	}
	printf("\n");
}

void PEFile::printExportNameTable()
{
	char text[64];
	printf("%-8s | %-24s | %-32s\n", "Data", "Description", "Value");
	std::cout << std::string(64, '-') << std::endl;
	for (auto ee : m_ExportTable)
	{
		sprintf(text, "%04X %s", ee.ordinal, ee.name.c_str());
		printf(x86_8byte_desc24, ee.rva, "Function Name RVA", text);
	}
	printf("\n");
}

void PEFile::printExportOrdinalTable()
{
	char text[64];
	printf("%-8s | %-24s | %-32s\n", "Data", "Description", "Value");
	std::cout << std::string(64, '-') << std::endl;
	for (auto ee : m_ExportTable)
	{
		sprintf(text, "%04X %s", ee.ordinal, ee.name.c_str());
		printf(x86_8byte_desc24, ee.ordinal, "Function Ordinal", text);
	}
	printf("\n");
}

void PEFile::printByte(void* data, int size, int first_offset, int interval, int size_of_element)
{
	for (int i = 1; i < size * size_of_element + 1; ++i)
	{
		int index = first_offset + interval * ((i - 1) / size_of_element) + ((i - 1) % size_of_element);
		unsigned char c = ((char*)data)[index];

		std::cout << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << (int)c << ' ';

		if (i % 8 == 0)
			std::cout << ' ';
		if (i % 16 == 0)
		{
			if (i != size * size_of_element)
				std::cout << std::endl;
		}
	}

	printf("\n\n");
}

void PEFile::printRaw(void* data, int size, int first_offset, int interval, int size_of_element)
{
	for (int i = 1; i < size * size_of_element + 1; ++i)
	{
		int index = first_offset + interval * ((i - 1) / size_of_element) + ((i - 1) % size_of_element);
		char c = ((char*)data)[index];

		if (0x21 > c || 0x7f < c)
			std::cout << '.';
		else
			std::cout << c;

		if (i % 16 == 0)
		{
			if (i != size * size_of_element)
				std::cout << std::endl;
		}
	}
	printf("\n\n");
}

void PEFile::printByteAndRaw(void* data, int size, int first_offset, int interval, int size_of_element)
{
	std::string raw;
	for (int i = 1; i < size * size_of_element + 1; ++i)
	{
		int index = first_offset + interval * ((i - 1) / size_of_element) + ((i - 1) % size_of_element);
		unsigned char c = ((char*)data)[index];

		if (0x21 > (char)c || 0x7f < (char)c)
			raw.append(1, '.');
		else
			raw.append(1, (char)c);

		std::cout << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << (int)c << ' ';

		if (i % 8 == 0)
			std::cout << ' ';
		if (i % 16 == 0)
		{
			std::cout << raw;
			raw.clear();

			if (i != size * size_of_element)
				std::cout << std::endl;
		}
		else if (i == size * size_of_element)
		{
			std::cout << std::string(i % 16 < 8 ? (16 - i % 16) * 3 + 2 : (16 - i % 16) * 3 + 1, ' ') << raw;
		}
	}
	printf("\n\n");
}

DWORD PEFile::rva2raw(DWORD rva)
{
	int idx = -1;
	for (int i = 0; i < m_sectionHeaders.size(); ++i)
	{
		if (m_sectionHeaders[i].VirtualAddress > rva)
			break;
		idx = i;
	}
	if (idx == -1)
		return rva;
	else
		return rva - m_sectionHeaders[idx].VirtualAddress + m_sectionHeaders[idx].PointerToRawData;
}

void PEFile::printHelp(const std::string& cmd)
{
	if (cmd == "help")
	{
		std::cout << "DOS : DOS 헤더에 대한 정보를 표시합니다." << std::endl;
		std::cout << "STUB : DOS STUB 프로그램 정보를 표시합니다." << std::endl;
		std::cout << "NT : NT 헤더에 대한 정보를 표시합니다." << std::endl;
		std::cout << "SH : Section 헤더에 대한 정보를 표시합니다." << std::endl;
		std::cout << "IDT : Import Directory Table에 대한 정보를 표시합니다." << std::endl;
		std::cout << "INT : Import Name Table에 대한 정보를 표시합니다." << std::endl;
		std::cout << "IAT : Import Address Table에 대한 정보를 표시합니다." << std::endl;
		std::cout << "IED : Image Export Directory에 대한 정보를 표시합니다." << std::endl;
		std::cout << "EAT : Export Address Table에 대한 정보를 표시합니다." << std::endl;
		std::cout << "ENT : Export Name Table에 대한 정보를 표시합니다." << std::endl;
		std::cout << "EOT : Export Ordinal Table에 대한 정보를 표시합니다." << std::endl;
		std::cout << "CLS : 화면을 지웁니다." << std::endl;
		std::cout << "EXIT : 프로그램을 종료합니다." << std::endl;
		std::cout << std::endl;
	}
	else if (cmd == "DOS")
	{
		std::cout << "DOS 헤더에 대한 정보를 표시합니다." << std::endl;
		std::cout << "기본 명령어 : DOS 헤더의 각 요소를 나열합니다" << std::endl << std::endl;
		std::cout << "옵션" << std::endl;
		std::cout << "-r : DOS 헤더 영역의 문자열을 표시합니다" << std::endl;
		std::cout << "-b : DOS 헤더 영역의 바이트 값을 표시합니다" << std::endl;
		std::cout << "-rb : DOS 헤더 영역의 바이트 값과 문자열을 동시에 표시합니다" << std::endl;
		std::cout << std::endl;
	}
	else if (cmd == "STUB")
	{
		std::cout << "DOS STUB 프로그램에 대한 정보를 표시합니다." << std::endl;
		std::cout << "기본 명령어 : STUB -rb와 동일합니다." << std::endl << std::endl;
		std::cout << "옵션" << std::endl;
		std::cout << "-r : DOS STUB 프로그램 영역의 문자열을 표시합니다" << std::endl;
		std::cout << "-b : DOS STUB 프로그램 영역의 바이트 값을 표시합니다" << std::endl;
		std::cout << "-rb : DOS STUB 프로그램 영역의 바이트 값과 문자열을 동시에 표시합니다" << std::endl;
		std::cout << std::endl;
	}
	else if (cmd == "NT")
	{
		std::cout << "NT 헤더에 대한 정보를 표시합니다." << std::endl;
		std::cout << "기본 명령어 : NT 헤더의 각 요소를 나열합니다." << std::endl << std::endl;
		std::cout << "옵션" << std::endl;
		std::cout << "-r : NT 헤더 영역의 문자열을 표시합니다." << std::endl;
		std::cout << "-b : NT 헤더 영역의 바이트 값을 표시합니다." << std::endl;
		std::cout << "-rb : NT 헤더 영역의 바이트 값과 문자열을 동시에 표시합니다." << std::endl;
		std::cout << "-file : NT 헤더의 File 헤더 정보를 표시합니다." << std::endl;
		std::cout << "-opt : NT 헤더의 Optional 헤더 정보를 표시합니다." << std::endl << std::endl;
		std::cout << "-file, -opt에 -r, -b, -rb 옵션을 주어 각각에 해당하는 문자열 및 바이트 값을 표시할 수 있습니다." << std::endl;
		std::cout << "ex) NT -file -rb" << std::endl;
		std::cout << std::endl;
	}
	else if (cmd == "SH")
	{
		std::cout << "Section 헤더에 대한 정보를 표시합니다." << std::endl;
		std::cout << "기본 명령어 : 존재하는 Section 헤더들을 나열합니다. (index : name)" << std::endl << std::endl;
		std::cout << "옵션" << std::endl;
		std::cout << "[index] : 해당 인덱스의 Section 헤더 정보를 표시합니다." << std::endl;
		std::cout << "[name] : 해당 이름의 Section 헤더 정보를 표시 합니다." << std::endl;
		std::cout << "-r : 해당 옵션 앞에 쓰인 Section 헤더 영역의 문자열을 표시합니다." << std::endl;
		std::cout << "-b : 해당 옵션 앞에 쓰인 Section 헤더 영역의 바이트 값을 표시합니다." << std::endl;
		std::cout << "-rb : 해당 옵션 앞에 쓰인 Section 헤더 영역의 바이트 값과 문자열을 동시에 표시합니다." << std::endl;
		std::cout << std::endl;
	}
	else if (cmd == "IDT")
	{
		std::cout << "Import Directory Table에 대한 정보를 표시합니다." << std::endl;
		std::cout << "기본 명령어 : Import Directory Table의 각 요소에 대한 정보를 나열합니다." << std::endl << std::endl;
		std::cout << "옵션" << std::endl;
		std::cout << "-r : Import Directory Table 영역의 문자열을 표시합니다." << std::endl;
		std::cout << "-b : Import Directory Table 영역의 바이트 값을 표시합니다" << std::endl;
		std::cout << "-rb : Import Directory Table 영역의 바이트 값과 문자열을 동시에 표시합니다" << std::endl;
		std::cout << std::endl;
	}
	else if (cmd == "INT")
	{
		std::cout << "Import Name Table에 대한 정보를 표시합니다." << std::endl;
		std::cout << "기본 명령어 : Import Name Table의 각 요소를 나열합니다" << std::endl << std::endl;
		std::cout << "옵션" << std::endl;
		std::cout << "-r : Import Name Table 영역의 각 RVA 값의 문자열을 표시합니다" << std::endl;
		std::cout << "-b : Import Name Table 영역의 각 RVA 값의 바이트 값을 표시합니다" << std::endl;
		std::cout << "-rb : Import Name Table 영역의 각 RVA 값의 바이트 값과 문자열을 동시에 표시합니다" << std::endl;
		std::cout << std::endl;
	}
	else if (cmd == "IAT")
	{
		std::cout << "Import Address Table에 대한 정보를 표시합니다." << std::endl;
		std::cout << "기본 명령어 : Import Address Table의 각 요소를 나열합니다" << std::endl << std::endl;
		std::cout << "옵션" << std::endl;
		std::cout << "-r : Import Address Table 영역의 각 RVA 값의 문자열을 표시합니다" << std::endl;
		std::cout << "-b : Import Address Table 영역의 각 RVA 값의 바이트 값을 표시합니다" << std::endl;
		std::cout << "-rb : Import Address Table 영역의 각 RVA 값의 바이트 값과 문자열을 동시에 표시합니다" << std::endl;
		std::cout << std::endl;
	}
	else if (cmd == "IED")
	{
		std::cout << "Image Export Directory에 대한 정보를 표시합니다." << std::endl;
		std::cout << "기본 명령어 : Image Export Directory의 각 요소를 나열합니다" << std::endl << std::endl;
		std::cout << "옵션" << std::endl;
		std::cout << "-r : Image Export Directory 영역의 문자열을 표시합니다" << std::endl;
		std::cout << "-b : Image Export Directory 영역의 바이트 값을 표시합니다" << std::endl;
		std::cout << "-rb : Image Export Directory 영역의 바이트 값과 문자열을 동시에 표시합니다" << std::endl;
		std::cout << std::endl;
	}
	else if (cmd == "EAT")
	{
		std::cout << "Export Address Table에 대한 정보를 표시합니다." << std::endl;
		std::cout << "기본 명령어 : Export Address Table의 각 요소를 나열합니다" << std::endl << std::endl;
		std::cout << "옵션" << std::endl;
		std::cout << "-r : Export Address Table 영역의 각 RVA 값의 문자열을 표시합니다" << std::endl;
		std::cout << "-b : Export Address Table 영역의 각 RVA 값의 바이트 값을 표시합니다" << std::endl;
		std::cout << "-rb : Export Address Table 영역의 각 RVA 값의 바이트 값과 문자열을 동시에 표시합니다" << std::endl;
		std::cout << std::endl;
	}
	else if (cmd == "ENT")
	{
		std::cout << "Export Name Table에 대한 정보를 표시합니다." << std::endl;
		std::cout << "기본 명령어 : Export Name Table의 각 요소를 나열합니다" << std::endl << std::endl;
		std::cout << "옵션" << std::endl;
		std::cout << "-r : Export Name Table 영역의 각 RVA 값의 문자열을 표시합니다" << std::endl;
		std::cout << "-b : Export Name Table 영역의 각 RVA 값의 바이트 값을 표시합니다" << std::endl;
		std::cout << "-rb : Export Name Table 영역의 각 RVA 값의 바이트 값과 문자열을 동시에 표시합니다" << std::endl;
		std::cout << std::endl;
	}
	else if (cmd == "EOT")
	{
		std::cout << "Export Ordinal Table에 대한 정보를 표시합니다." << std::endl;
		std::cout << "기본 명령어 : Export Ordinal Table의 각 요소를 나열합니다" << std::endl << std::endl;
		std::cout << "옵션" << std::endl;
		std::cout << "-r : Export Ordinal Table 영역의 문자열을 표시합니다" << std::endl;
		std::cout << "-b : Export Ordinal Table 영역의 바이트 값을 표시합니다" << std::endl;
		std::cout << "-rb : Export Ordinal Table 영역의 바이트 값과 문자열을 동시에 표시합니다" << std::endl;
		std::cout << std::endl;
	}
}

void PEFile::GetMachineString(WORD machine)
{
	if (machine & IMAGE_FILE_MACHINE_TARGET_HOST)
		m_machineStr = "IMAGE_FILE_MACHINE_TARGET_HOST";
	else if (machine & IMAGE_FILE_MACHINE_I386)
		m_machineStr = "IMAGE_FILE_MACHINE_I386";
	else if (machine & IMAGE_FILE_MACHINE_R3000)
		m_machineStr = "IMAGE_FILE_MACHINE_R3000";
	else if (machine & IMAGE_FILE_MACHINE_R4000)
		m_machineStr = "IMAGE_FILE_MACHINE_R4000";
	else if (machine & IMAGE_FILE_MACHINE_R10000)
		m_machineStr = "IMAGE_FILE_MACHINE_R10000";
	else if (machine & IMAGE_FILE_MACHINE_WCEMIPSV2)
		m_machineStr = "IMAGE_FILE_MACHINE_WCEMIPSV2";
	else if (machine & IMAGE_FILE_MACHINE_ALPHA)
		m_machineStr = "IMAGE_FILE_MACHINE_ALPHA";
	else if (machine & IMAGE_FILE_MACHINE_SH3)
		m_machineStr = "IMAGE_FILE_MACHINE_SH3";
	else if (machine & IMAGE_FILE_MACHINE_SH3DSP)
		m_machineStr = "IMAGE_FILE_MACHINE_SH3DSP";
	else if (machine & IMAGE_FILE_MACHINE_SH3E)
		m_machineStr = "IMAGE_FILE_MACHINE_SH3E";
	else if (machine & IMAGE_FILE_MACHINE_SH4)
		m_machineStr = "IMAGE_FILE_MACHINE_SH4";
	else if (machine & IMAGE_FILE_MACHINE_SH5)
		m_machineStr = "IMAGE_FILE_MACHINE_SH5";
	else if (machine & IMAGE_FILE_MACHINE_ARM)
		m_machineStr = "IMAGE_FILE_MACHINE_ARM";
	else if (machine & IMAGE_FILE_MACHINE_THUMB)
		m_machineStr = "IMAGE_FILE_MACHINE_THUMB";
	else if (machine & IMAGE_FILE_MACHINE_ARMNT)
		m_machineStr = "IMAGE_FILE_MACHINE_ARMNT";
	else if (machine & IMAGE_FILE_MACHINE_AM33)
		m_machineStr = "IMAGE_FILE_MACHINE_AM33";
	else if (machine & IMAGE_FILE_MACHINE_POWERPC)
		m_machineStr = "IMAGE_FILE_MACHINE_POWERPC";
	else if (machine & IMAGE_FILE_MACHINE_POWERPCFP)
		m_machineStr = "IMAGE_FILE_MACHINE_POWERPCFP";
	else if (machine & IMAGE_FILE_MACHINE_IA64)
		m_machineStr = "IMAGE_FILE_MACHINE_IA64";
	else if (machine & IMAGE_FILE_MACHINE_MIPS16)
		m_machineStr = "IMAGE_FILE_MACHINE_MIPS16";
	else if (machine & IMAGE_FILE_MACHINE_ALPHA64)
		m_machineStr = "IMAGE_FILE_MACHINE_ALPHA64";
	else if (machine & IMAGE_FILE_MACHINE_MIPSFPU)
		m_machineStr = "IMAGE_FILE_MACHINE_MIPSFPU";
	else if (machine & IMAGE_FILE_MACHINE_MIPSFPU16)
		m_machineStr = "IMAGE_FILE_MACHINE_MIPSFPU16";
	else if (machine & IMAGE_FILE_MACHINE_AXP64)
		m_machineStr = "IMAGE_FILE_MACHINE_AXP64";
	else if (machine & IMAGE_FILE_MACHINE_TRICORE)
		m_machineStr = "IMAGE_FILE_MACHINE_TRICORE";
	else if (machine & IMAGE_FILE_MACHINE_CEF)
		m_machineStr = "IMAGE_FILE_MACHINE_CEF";
	else if (machine & IMAGE_FILE_MACHINE_EBC)
		m_machineStr = "IMAGE_FILE_MACHINE_EBC";
	else if (machine & IMAGE_FILE_MACHINE_AMD64)
		m_machineStr = "IMAGE_FILE_MACHINE_AMD64";
	else if (machine & IMAGE_FILE_MACHINE_M32R)
		m_machineStr = "IMAGE_FILE_MACHINE_M32R";
	else if (machine & IMAGE_FILE_MACHINE_ARM64)
		m_machineStr = "IMAGE_FILE_MACHINE_ARM64";
	else if (machine & IMAGE_FILE_MACHINE_CEE)
		m_machineStr = "IMAGE_FILE_MACHINE_CEE";
	else if (machine & IMAGE_FILE_MACHINE_UNKNOWN)
		m_machineStr = "IMAGE_FILE_MACHINE_UNKNOWN";
}

void PEFile::GetSubSystemString(WORD subsys)
{
	switch (subsys) {
	case IMAGE_SUBSYSTEM_UNKNOWN:
		m_subSystem = "IMAGE_SUBSYSTEM_UNKNOWN";
		break;
	case IMAGE_SUBSYSTEM_NATIVE:
		m_subSystem = "IMAGE_SUBSYSTEM_NATIVE";
		break;
	case IMAGE_SUBSYSTEM_WINDOWS_GUI:
		m_subSystem = "IMAGE_SUBSYSTEM_WINDOWS_GUI";
		break;
	case IMAGE_SUBSYSTEM_WINDOWS_CUI:
		m_subSystem = "IMAGE_SUBSYSTEM_WINDOWS_CUI";
		break;
	case IMAGE_SUBSYSTEM_OS2_CUI:
		m_subSystem = "IMAGE_SUBSYSTEM_OS2_CUI";
		break;
	case IMAGE_SUBSYSTEM_POSIX_CUI:
		m_subSystem = "IMAGE_SUBSYSTEM_POSIX_CUI";
		break;
	case IMAGE_SUBSYSTEM_NATIVE_WINDOWS:
		m_subSystem = "IMAGE_SUBSYSTEM_NATIVE_WINDOWS";
		break;
	case IMAGE_SUBSYSTEM_WINDOWS_CE_GUI:
		m_subSystem = "IMAGE_SUBSYSTEM_WINDOWS_CE_GUI";
		break;
	case IMAGE_SUBSYSTEM_EFI_APPLICATION:
		m_subSystem = "IMAGE_SUBSYSTEM_EFI_APPLICATION";
		break;
	case IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER:
		m_subSystem = "IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER";
		break;
	case IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER:
		m_subSystem = "IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER";
		break;
	case IMAGE_SUBSYSTEM_EFI_ROM:
		m_subSystem = "IMAGE_SUBSYSTEM_EFI_ROM";
		break;
	case IMAGE_SUBSYSTEM_XBOX:
		m_subSystem = "IMAGE_SUBSYSTEM_XBOX";
		break;
	case IMAGE_SUBSYSTEM_WINDOWS_BOOT_APPLICATION:
		m_subSystem = "IMAGE_SUBSYSTEM_WINDOWS_BOOT_APPLICATION";
		break;
	case IMAGE_SUBSYSTEM_XBOX_CODE_CATALOG:
		m_subSystem = "IMAGE_SUBSYSTEM_XBOX_CODE_CATALOG";
		break;
	}
}

std::string PEFile::GetDirectoryName(int i)
{
	switch (i)
	{
	case 0x0:
		return "EXPORT Directory";
	case 0x1:
		return "IMPORT Directory";
	case 0x2:
		return "RESOURCE Directory";
	case 0x3:
		return "EXCEPTION Directory";
	case 0x4:
		return "SECURITY Directory";
	case 0x5:
		return "BASE RELOCATION Directory";
	case 0x6:
		return "DEBUG Directory";
	case 0x7:
		return "COPYRIGHT Directory";
	case 0x8:
		return "GLOBALPTR Directory";
	case 0x9:
		return "TLS Directory";
	case 0xa:
		return "LOAD CONFIG Directory";
	case 0xb:
		return "BOUND IMPORT Directory";
	case 0xc:
		return "IAT Directory";
	case 0xd:
		return "DELAY IMPORT Directory";
	case 0xe:
		return "COM DESCRIPTOR Directory";
	case 0xf:
		return "Reserved Directory";
	}
}
