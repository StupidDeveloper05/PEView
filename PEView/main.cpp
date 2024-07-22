#include "PEFile.h"

int main(int argc, char* argv[])
{
	if (argc != 2)
		return 0;

	std::string filePath = argv[1];
	PEFile pe(filePath);
	if (!pe.loaded)
		return 1;

	pe.Run();

	return 0;
}