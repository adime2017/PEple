#include "PEWarrior.h"
#include <time.h>

PEWarrior::PEWarrior(char* filePath)
{
	this->MyFile->open(filePath, ios::binary | ios::in | ios::out);
	strcpy(this->filepath, filePath);
	cout << "File Path: " << this->filepath << endl;
	if (!this->MyFile->is_open())
	{
		cout << "Cannot open the file.\n";
	}
	else
	{
		cout << "DOS PART----------------------------------------------------" << endl;
		getDOSHeader();
		cout << "------------------------------------------------------------\n" << endl;
		cout << "PE FILE HEADER PART-----------------------------------------" << endl;
		getPEFileHeader();
		cout << "------------------------------------------------------------\n" << endl;
		cout << "PE OPTIONAL HEADER PART ------------------------------------" << endl;
		getPEOptionHeader();
		cout << "------------------------------------------------------------\n" << endl;
		getSectionHeader();
		BYTE shellCode[] = { 0x6A, 0x00, 0x6A, 0x00, 0x6A, 0x00, 0x6A, 0x00, 0xE8, 0x7A, 0x09, 0x8C, 0x76 };
		//setDllCharcateristic(6, 1);
		//injectCode32(0x76CC0C30);
		//DWORD FOA = RVAToFOA(0x00404018);
		//cout << "FOA: " << hex << FOA << endl;
		//setSectionCharacteristic(sectionTables.numberOfSections - 1, 29, 1);
		DWORD injectPointer = findInjectableSection(30);
		cout << "Injectable Section: " << hex << injectPointer << endl;
		DWORD injectPointerInMem = FOAToRVA(injectPointer);
		cout << "Inject Point in Mem: " << hex << injectPointerInMem;
		
	}
}


bool PEWarrior::checkPE()
{
	
	if (!this->MyFile->is_open())
	{
		cout << "The file is nnot open yet";
		return false;
	}
	// copy 2048 bytes to buffer
	char* binaryBuffer = new char[2048];
	this->MyFile->read(binaryBuffer, 2048);

	//First 2 bytes are MZ significant
	char MZHeader[3];
	strncpy(MZHeader, binaryBuffer, 2);
	MZHeader[2] = '\0';
	int isMZ = strcmp(MZHeader, "MZ");
	if (isMZ == 0)
	{
		cout << "this is MZ file\n";
	}

	// The Position of PE Symbol is in the 60 bytes of the file
	char PESybolPosition[1];

	//Check if the file is PE file, 3Ch = 60
	strncpy(PESybolPosition, binaryBuffer + 60, 1);
	/*printf("%d \n", (unsigned char)PESybolPosition[0]);*/
	char PESymbol[3];
	strncpy(PESymbol, binaryBuffer + (unsigned char)PESybolPosition[0], 2);
	PESymbol[2] = '\0';
	int isPE = strcmp(PESymbol, "PE");
	if (isPE == 0)
	{
		cout << "this is PE file\n";
	}

	// Give the PE Symboal Address to class member.
	this->pESignatureAddress = (unsigned char)PESybolPosition[0];

	//cout << PESymbol << endl;

	delete[] binaryBuffer;

	if (isMZ && isPE)
	{
		return true;
	}
	else
	{
		return false;
	}
	
}

int PEWarrior::RVAToFOA(DWORD rva)
{
	DWORD rvaOffsetBase = rva - peOptionalHeader.baseAddress;
	//If RVA in the header reagion, just return it.
	if (rvaOffsetBase < peOptionalHeader.sizeOfHeaders)
	{
		return rvaOffsetBase;
	}
	
	// check whether RVA is in the each section
	for (int i = 0; i < sectionTables.numberOfSections; i++)
	{
		DWORD currentSectionSize;
		// get the bigger size between file size and memory size.
		if (sectionTables.tableArray[i].Misc.VirtualSize > sectionTables.tableArray[i].SizeOfRawData)
		{
			currentSectionSize = sectionTables.tableArray[i].Misc.VirtualSize;
		}
		else {
			currentSectionSize = sectionTables.tableArray[i].SizeOfRawData;
		}

		// Check whether the RVAOffsetBase is in the section
		if ((rvaOffsetBase >= sectionTables.tableArray[i].VirtualAddress) && (rvaOffsetBase < (sectionTables.tableArray[i].VirtualAddress + currentSectionSize)))
		{
			
			DWORD sectionOffset = rvaOffsetBase - sectionTables.tableArray[i].VirtualAddress;
			DWORD FOA = sectionTables.tableArray[i].PointerToRawData + sectionOffset;
			return FOA;
		}
	}

	return -1;
}

DWORD PEWarrior::FOAToRVA(DWORD foa)
{
	if (foa < peOptionalHeader.sizeOfHeaders)
	{
		return foa;
	}
	
	for (int i = 0; i < sectionTables.numberOfSections; i++)
	{
		if ((foa >= sectionTables.tableArray[i].PointerToRawData) && (foa < (sectionTables.tableArray[i].PointerToRawData + sectionTables.tableArray[i].SizeOfRawData)))
		{
			DWORD RAVInTheSection = foa - sectionTables.tableArray[i].PointerToRawData;
			return sectionTables.tableArray[i].VirtualAddress + RAVInTheSection;
		}
	}
}

PEWarrior::~PEWarrior()
{
	MyFile->close();
}

int PEWarrior::getHex(char* address, int size)
{
	int hex = 0;
	for (int i = 0; i < size; i++)
	{
		hex += (address[i] * pow(16, size - 1 - i));
	}
	return hex;
}

bool PEWarrior::getDOSHeader()
{
	if (!this->MyFile->is_open())
	{
		cout << "Please load a file first.\n";
		return false;
	}
	
	char* DOSHeaderBuffer = new char[dosPart.sizeOfDosHeader];
	MyFile->read(DOSHeaderBuffer, dosPart.sizeOfDosHeader);

	// MZ signature
	dosPart.MZSignature = ((_IMAGE_DOS_HEADER*)DOSHeaderBuffer)->e_magic; 
	// PE signature position
	dosPart.positionOfPESignature = ((_IMAGE_DOS_HEADER*)DOSHeaderBuffer)->e_lfanew;
	cout << "MZ signature: " << hex << ((char*)(&(dosPart.MZSignature)))[0] << ((char*)(&(dosPart.MZSignature)))[1] << endl;
	cout << "PE signature position: " << hex << dosPart.positionOfPESignature << endl;
	delete[] DOSHeaderBuffer;
	return true;
}

bool PEWarrior::getPEFileHeader()
{
	if (dosPart.MZSignature != dosPart.Signature)
	{
		return false;
	}
	
	char* PEFileHeaderBuffer = new char[peFileHeader.sizeOfFileHeader];
	
	peFileHeader.header = (IMAGE_FILE_HEADER*)PEFileHeaderBuffer;
	MyFile->seekg(dosPart.positionOfPESignature);
	// The first 4 Bytes is PE signature
	MyFile->read((char*)&currentPESignature, sizeof(DWORD));
	cout << "PE signature: " << ((char*)(&currentPESignature))[0] << ((char*)(&currentPESignature))[1] << endl;
	MyFile->read(PEFileHeaderBuffer, peFileHeader.sizeOfFileHeader);
	peFileHeader.sizeOfOptionalHeader = ((_IMAGE_FILE_HEADER*)PEFileHeaderBuffer)->SizeOfOptionalHeader;
	peFileHeader.machine = ((_IMAGE_FILE_HEADER*)PEFileHeaderBuffer)->Machine;
	cout << "Size Of Optional Header: " << hex << peFileHeader.sizeOfOptionalHeader << endl;
	if (peFileHeader.machine == 0)
	{
		cout << "This exe run in: Every CPU" << endl;
	}
	else if (peFileHeader.machine == 0x14c) {
		cout << "This exe run in: 32 bit computer" << endl;
	}
	else if (peFileHeader.machine == 0x8664) {
		cout << "This exe run in: 64 bit computer" << endl;
	}
	else {
		cout << "This exe run in: other CPU" << endl;
	}

	peFileHeader.numberOfSection = ((_IMAGE_FILE_HEADER*)PEFileHeaderBuffer)->NumberOfSections;
	cout << "Sections: " << peFileHeader.numberOfSection << endl;
	peFileHeader.timeStamp = ((_IMAGE_FILE_HEADER*)PEFileHeaderBuffer)->TimeDateStamp;
	peFileHeader.characteristics = ((_IMAGE_FILE_HEADER*)PEFileHeaderBuffer)->Characteristics;
	return true;
}

bool PEWarrior::getPEOptionHeader()
{
	if (!MyFile->is_open())
	{
		return false;
	}
	
	char* PEoptionalHeaderBuffer = new char[peFileHeader.sizeOfOptionalHeader];
	MyFile->seekg(dosPart.positionOfPESignature + 4 + 20);
	MyFile->read(PEoptionalHeaderBuffer, peFileHeader.sizeOfOptionalHeader);
	peOptionalHeader.setHeader(PEoptionalHeaderBuffer);
	//Firstly, we nned to figure out if the exe 64 bit or 32 bit
	
	if (peFileHeader.machine == 0x14c)
	{
		peOptionalHeader.magic = ((_IMAGE_OPTIONAL_HEADER*)PEoptionalHeaderBuffer)->Magic;
		peOptionalHeader.baseAddress = ((_IMAGE_OPTIONAL_HEADER*)PEoptionalHeaderBuffer)->ImageBase;
		peOptionalHeader.addressOfEntryPoint = ((_IMAGE_OPTIONAL_HEADER*)PEoptionalHeaderBuffer)->AddressOfEntryPoint;
		peOptionalHeader.fileAlignment = ((_IMAGE_OPTIONAL_HEADER*)PEoptionalHeaderBuffer)->FileAlignment;
		peOptionalHeader.memoryAlignment = ((_IMAGE_OPTIONAL_HEADER*)PEoptionalHeaderBuffer)->SectionAlignment;
		peOptionalHeader.sizeOfImage = ((_IMAGE_OPTIONAL_HEADER*)PEoptionalHeaderBuffer)->SizeOfImage;
		peOptionalHeader.sizeOfHeaders = ((_IMAGE_OPTIONAL_HEADER*)PEoptionalHeaderBuffer)->SizeOfHeaders;
		peOptionalHeader.checkSum = ((_IMAGE_OPTIONAL_HEADER*)PEoptionalHeaderBuffer)->CheckSum;
		peOptionalHeader.dllCharacteristic = ((_IMAGE_OPTIONAL_HEADER*)PEoptionalHeaderBuffer)->DllCharacteristics;
	}
	else {
		peOptionalHeader.magic = ((_IMAGE_OPTIONAL_HEADER64*)PEoptionalHeaderBuffer)->Magic;
		peOptionalHeader.baseAddress = ((_IMAGE_OPTIONAL_HEADER64*)PEoptionalHeaderBuffer)->ImageBase;
		peOptionalHeader.addressOfEntryPoint = ((_IMAGE_OPTIONAL_HEADER64*)PEoptionalHeaderBuffer)->AddressOfEntryPoint;
		peOptionalHeader.fileAlignment = ((_IMAGE_OPTIONAL_HEADER64*)PEoptionalHeaderBuffer)->FileAlignment;
		peOptionalHeader.memoryAlignment = ((_IMAGE_OPTIONAL_HEADER64*)PEoptionalHeaderBuffer)->SectionAlignment;
		peOptionalHeader.sizeOfImage = ((_IMAGE_OPTIONAL_HEADER64*)PEoptionalHeaderBuffer)->SizeOfImage;
		peOptionalHeader.sizeOfHeaders = ((_IMAGE_OPTIONAL_HEADER64*)PEoptionalHeaderBuffer)->SizeOfHeaders;
		peOptionalHeader.checkSum = ((_IMAGE_OPTIONAL_HEADER64*)PEoptionalHeaderBuffer)->CheckSum;
		peOptionalHeader.dllCharacteristic = ((_IMAGE_OPTIONAL_HEADER64*)PEoptionalHeaderBuffer)->DllCharacteristics;
	}

	cout << "Magic code: " << hex << peOptionalHeader.magic << endl;
	cout << "Base Address: " << hex << peOptionalHeader.baseAddress << endl;
	cout << "Entry Point: " << hex << peOptionalHeader.addressOfEntryPoint << endl;
	cout << "File Alignment: " << hex << peOptionalHeader.fileAlignment << endl;
	cout << "Memory Alignment: " << hex << peOptionalHeader.memoryAlignment << endl;
	cout << "Occupation Of Memory: " << hex << peOptionalHeader.sizeOfImage << endl;
	cout << "Size Of Headers using Disk Alignment: " << hex << peOptionalHeader.sizeOfHeaders << endl;
	cout << "Check Sum: " << hex << peOptionalHeader.checkSum << "  //System will check this when you modify the system file." << endl;
	cout << "dllCharacteristic: " << hex << peOptionalHeader.dllCharacteristic << endl;
	bitset<16> characteristicBitSet(peOptionalHeader.dllCharacteristic);
	cout << characteristicBitSet << endl;
	
	if (characteristicBitSet[6] == 1)
	{
		characteristicBitSet[6] = 0;
		cout << "This exe has dynamic base address\n";
	}
	if (characteristicBitSet[7] == 1)
	{
		cout << "System will check sum for this exe.\n";
	}

	return true;
}

bool PEWarrior::getSectionHeader()
{
	if (!MyFile->is_open())
	{
		return false;
	}

	sectionTables.setNumberOfSections(peFileHeader.numberOfSection);

	MyFile->seekg(dosPart.positionOfPESignature + 4 + sizeof(_IMAGE_FILE_HEADER) + peFileHeader.sizeOfOptionalHeader);
	
	_IMAGE_SECTION_HEADER* tablesAddress = new _IMAGE_SECTION_HEADER[peFileHeader.numberOfSection];
	MyFile->read((char*)tablesAddress, sizeof(_IMAGE_SECTION_HEADER) * peFileHeader.numberOfSection);

	sectionTables.setArrayStartAddress((_IMAGE_SECTION_HEADER*)tablesAddress);
	for (int i = 0; i < sectionTables.numberOfSections; i++)
	{
		bitset<32> charac(sectionTables.tableArray[i].Characteristics);
		cout << "------------------------------------------------------------" << endl;
		cout << "Section: " << sectionTables.tableArray[i].Name << endl;
		cout << "Entry in Memory: " << sectionTables.tableArray[i].VirtualAddress << endl;
		cout << "True Size in Memory: " << sectionTables.tableArray[i].Misc.VirtualSize << endl;
		cout << "Entry in File: " << sectionTables.tableArray[i].PointerToRawData << endl;
		cout << "Alignemnt Size in the file: " << sectionTables.tableArray[i].SizeOfRawData << endl;
		cout << "Characristic: " << charac << endl;
		cout << "------------------------------------------------------------" << endl;
	}
	return false;
}

void PEWarrior::printTime(WORD timeStamp)
{
	time_t stamp = timeStamp;
	tm* structuredTime;
	structuredTime = gmtime(&stamp);

	char timeString[256];
	strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", structuredTime);
}

void PEWarrior::setDllCharcateristic(int position, int value)
{
	if (!MyFile->is_open())
	{
		return;
	}
	if ((position < 0) || (position > 15))
	{
		return;
	}
	if (value < 0 || value > 1)
	{
		return;
	}
	void* optionalHeaderAddress = peOptionalHeader.getHeaeder();

	WORD dllChracteristic;
	if (peFileHeader.machine == 0x14c)
	{
		dllChracteristic = ((_IMAGE_OPTIONAL_HEADER*)optionalHeaderAddress)->DllCharacteristics;
	}
	else
	{
		dllChracteristic = ((_IMAGE_OPTIONAL_HEADER64*)optionalHeaderAddress)->DllCharacteristics;
	}

	bitset<16> characteristicBits(dllChracteristic);
	characteristicBits[position] = value;

	((_IMAGE_OPTIONAL_HEADER64*)optionalHeaderAddress)->DllCharacteristics = (WORD)characteristicBits.to_ulong();
	MyFile->seekp(dosPart.positionOfPESignature + 4 + sizeof(_IMAGE_FILE_HEADER));
	MyFile->write((char*)optionalHeaderAddress, peFileHeader.sizeOfOptionalHeader);
}

void PEWarrior::setSectionCharacteristic(int indexOfTable, int indexOfBit, int value)
{
	if (indexOfTable >= sectionTables.numberOfSections)
	{
		return;
	}
	if (indexOfBit >= 32)
	{
		return;
	}
	if (value != 0 && value != 1)
	{
		return;
	}
	_IMAGE_SECTION_HEADER* sectionHeader = sectionTables.tableArray + indexOfTable;
	bitset<32> character(sectionHeader->Characteristics);
	//cout << "Section " << indexOfTable << " Charac is: " << character << endl;
	character[indexOfBit] = value;
	sectionHeader->Characteristics = (DWORD)character.to_ulong();
	MyFile->seekp(dosPart.positionOfPESignature + 4 + sizeof(_IMAGE_FILE_HEADER) + peFileHeader.sizeOfOptionalHeader + sizeof(_IMAGE_SECTION_HEADER) * (indexOfTable));
	MyFile->write((char*)sectionHeader, sizeof(_IMAGE_SECTION_HEADER));
}

void PEWarrior::extendLastSection(int size)
{
	//First seek to the end of the file
	
	MyFile->close();
	MyFile->open(filepath, ios::out | ios::app | ios::binary);
	char* newMemory = new char[size];
	memset(newMemory, 0, size);
	MyFile->write(newMemory, size);
	MyFile->close();
	MyFile->open(filepath, ios::in | ios::out | ios::binary);

	// Change the size of rawdata/virtula size
	int finalSize;
	if (sectionTables.tableArray[sectionTables.numberOfSections - 1].SizeOfRawData >= sectionTables.tableArray[sectionTables.numberOfSections - 1].Misc.VirtualSize)
	{
		finalSize = sectionTables.tableArray[sectionTables.numberOfSections - 1].SizeOfRawData + size;
	}
	else
	{
		// alignment of VirtualSize:
		DWORD virtualSizeAfterAlignment = peOptionalHeader.memoryAlignment * ((sectionTables.tableArray[sectionTables.numberOfSections - 1].Misc.VirtualSize / peOptionalHeader.memoryAlignment) + 1);
		finalSize = virtualSizeAfterAlignment + size;
	}

	//Change the size to final size
	sectionTables.tableArray[sectionTables.numberOfSections - 1].SizeOfRawData = finalSize;
	sectionTables.tableArray[sectionTables.numberOfSections - 1].Misc.VirtualSize = finalSize;
	// Write to file
	MyFile->seekp(dosPart.positionOfPESignature + 4 + sizeof(_IMAGE_FILE_HEADER) + peFileHeader.sizeOfOptionalHeader);
	MyFile->write((char*)(sectionTables.tableArray), sizeof(_IMAGE_SECTION_HEADER) * sectionTables.numberOfSections);

	//Change the size of Image Size

	
	DWORD previousSize = peOptionalHeader.sizeOfImage;
	DWORD previousSizeAfterAlignment;
	//align the image size
	if ((previousSize % peOptionalHeader.memoryAlignment) == 0)
	{
		previousSizeAfterAlignment = previousSize;
	}
	else
	{
		previousSizeAfterAlignment = peOptionalHeader.memoryAlignment * ((previousSize / peOptionalHeader.memoryAlignment) + 1);
	}
	DWORD finalImageSize = previousSizeAfterAlignment + size;
	if (peFileHeader.machine == 0x14)
	{
		((_IMAGE_OPTIONAL_HEADER*)peOptionalHeader.getHeaeder())->SizeOfImage = finalImageSize;
	}
	else {
		((_IMAGE_OPTIONAL_HEADER64*)peOptionalHeader.getHeaeder())->SizeOfImage = finalImageSize;
	}

	//Write to file:
	MyFile->seekp(dosPart.positionOfPESignature + 4 + sizeof(_IMAGE_FILE_HEADER));
	MyFile->write((char*)(peOptionalHeader.getHeaeder()), peFileHeader.sizeOfOptionalHeader);
	delete[] newMemory;
}

void PEWarrior::addASection(DWORD size)
{
	DWORD lastSectionFOA = dosPart.positionOfPESignature + 4 + sizeof(_IMAGE_FILE_HEADER) + peFileHeader.sizeOfOptionalHeader + (sectionTables.numberOfSections * sizeof(_IMAGE_SECTION_HEADER));
	//add a section in fileheader
	(peFileHeader.header)->NumberOfSections = peFileHeader.numberOfSection + 1;
	MyFile->seekp(dosPart.positionOfPESignature + 4);
	MyFile->write((char*)peFileHeader.header, sizeof(_IMAGE_FILE_HEADER));

	//construct a section header
	_IMAGE_SECTION_HEADER* newSectionHeader = new _IMAGE_SECTION_HEADER;
	memset(newSectionHeader, 0, sizeof(_IMAGE_SECTION_HEADER));

	newSectionHeader->Characteristics = 0x60000020;
	(newSectionHeader->Name)[0] = '.';
	(newSectionHeader->Name)[1] = 'k';
	(newSectionHeader->Name)[2] = 'i';
	(newSectionHeader->Name)[3] = 's';
	(newSectionHeader->Name)[4] = 's';
	//DWORD sizeAftermMemoryAlignment = ((size / peOptionalHeader.memoryAlignment) + 1) * peOptionalHeader.memoryAlignment;
	(newSectionHeader->Misc).VirtualSize = size;
	DWORD pointerToNewRawData = sectionTables.tableArray[sectionTables.numberOfSections - 1].PointerToRawData + sectionTables.tableArray[sectionTables.numberOfSections - 1].SizeOfRawData;
	newSectionHeader->PointerToRawData = pointerToNewRawData;
	DWORD newVirtualAddress = sectionTables.tableArray[sectionTables.numberOfSections - 1].VirtualAddress + (peOptionalHeader.memoryAlignment * ((sectionTables.tableArray[sectionTables.numberOfSections - 1].Misc.VirtualSize / peOptionalHeader.memoryAlignment) + 1));
	newSectionHeader->VirtualAddress = newVirtualAddress;
	//newSectionHeader->SizeOfRawData = ((size / peOptionalHeader.fileAlignment) + 1) * peOptionalHeader.fileAlignment;
	newSectionHeader->SizeOfRawData = size;

	// Write the new table to file
	MyFile->seekp(lastSectionFOA);
	MyFile->write((char*)newSectionHeader, sizeof(_IMAGE_SECTION_HEADER));

	// Modify the ImageSize
	if (peFileHeader.machine == 0x14c)
	{
		((_IMAGE_OPTIONAL_HEADER*)peOptionalHeader.getHeaeder())->SizeOfImage = peOptionalHeader.sizeOfImage + ((size / peOptionalHeader.memoryAlignment) + 1) * peOptionalHeader.memoryAlignment;
	}
	else {
		((_IMAGE_OPTIONAL_HEADER64*)peOptionalHeader.getHeaeder())->SizeOfImage = peOptionalHeader.sizeOfImage + ((size / peOptionalHeader.memoryAlignment) + 1) * peOptionalHeader.memoryAlignment;
	}

	MyFile->seekp(dosPart.positionOfPESignature + 4 + sizeof(_IMAGE_FILE_HEADER));
	MyFile->write((char*)(peOptionalHeader.getHeaeder()), peFileHeader.sizeOfOptionalHeader);

	// Append the new space for new section
	MyFile->close();
	MyFile->open(filepath, ios::app | ios::out | ios::binary);
	BYTE* newMemory = new BYTE[size];
	memset(newMemory, 0, size);
	MyFile->write((char*)newMemory, size);
	MyFile->close();
	MyFile->open(filepath, ios::in | ios::out | ios::binary);

	delete[] newMemory;
	delete newSectionHeader;
}

DWORD PEWarrior::findInjectableSection(int size)
{

	for (int i = 0; i < peFileHeader.numberOfSection; i++)
	{
		if (sectionTables.tableArray[i].Misc.VirtualSize < sectionTables.tableArray[i].SizeOfRawData)
		{
			if (sectionTables.tableArray[i].SizeOfRawData - sectionTables.tableArray[i].Misc.VirtualSize >= size)
			{
				return (sectionTables.tableArray[i].PointerToRawData + sectionTables.tableArray[i].Misc.VirtualSize);
			}
		}
	}
}

void PEWarrior::injectMessageBoxA32(DWORD funAddress) // Only support 32 bit exe
{

	//find where can inject code
	DWORD injectableFOA = findInjectableSection(50);
	DWORD injectableRAV = FOAToRVA(injectableFOA);
	MyFile->seekp(injectableFOA);
	//Inject Code Here:

	BYTE code[13];
	code[0] = 0x6A;
	code[1] = 0x00;
	code[2] = 0x6A;
	code[3] = 0x00;
	code[4] = 0x6A;
	code[5] = 0x00;
	code[6] = 0x6A;
	code[7] = 0x00;
	code[8] = 0xe8;

	DWORD callDestination = funAddress - (peOptionalHeader.baseAddress + injectableRAV + 8) - 5;

	for (int i = 0; i < 4; i++)
	{
		code[9 + i] = ((char*)(&callDestination))[i];
	}

	for (int i = 0; i < sizeof(code); i++)
	{
		MyFile->write((char*)(code + i), 1);
	}

	//Inject jump back to the previous entry point code:
	DWORD jumpInstructionAddress = peOptionalHeader.baseAddress + injectableRAV + sizeof(code);
	DWORD memoryEntryAddress = peOptionalHeader.baseAddress + peOptionalHeader.addressOfEntryPoint;
	DWORD jumpDestination = memoryEntryAddress - jumpInstructionAddress - 5;

	BYTE jumpBack[5];
	memset(&jumpBack, 0, 5);
	jumpBack[0] = 0xE9;
	for (int i = 0; i < 4; i++)
	{
		jumpBack[1+i] = ((BYTE*)&jumpDestination)[i];
	}

	MyFile->write((char*)jumpBack, 5);

	//Change the entry point to new entry point

	_IMAGE_OPTIONAL_HEADER* optionalHeader = (_IMAGE_OPTIONAL_HEADER*)(peOptionalHeader.getHeaeder());
	optionalHeader->AddressOfEntryPoint = injectableRAV;
	MyFile->seekp(dosPart.positionOfPESignature + 4 + sizeof(_IMAGE_FILE_HEADER));
	MyFile->write((char*)optionalHeader, peFileHeader.sizeOfOptionalHeader);
	// The reanson +1 is that the address of the instruction is the address of the firt byte.
	

}

//?????????????????????????????????????????????????????????????????????????????????????
// How to set template to different type???????????????????????????????????????????????
//?????????????????????????????????????????????????????????????????????????????????????
void PEWarrior::injectMessageBoxA32AtEnd(DWORD funAddress)
{

	BYTE code[13];
	code[0] = 0x6A;
	code[1] = 0x00;
	code[2] = 0x6A;
	code[3] = 0x00;
	code[4] = 0x6A;
	code[5] = 0x00;
	code[6] = 0x6A;
	code[7] = 0x00;
	code[8] = 0xe8;

	DWORD virtualSizeBeforeExtend = sectionTables.tableArray[sectionTables.numberOfSections - 1].Misc.VirtualSize;
	// extend to last section
	extendLastSection(0x200);

	setSectionCharacteristic(sectionTables.numberOfSections - 1, 29, 1);
	// find a place to insert code
	DWORD FOAToInsert = sectionTables.tableArray[sectionTables.numberOfSections - 1].PointerToRawData + virtualSizeBeforeExtend;
	DWORD RVAOfInsertion = FOAToRVA(FOAToInsert);

	DWORD callDestini = funAddress - (peOptionalHeader.baseAddress + RVAOfInsertion + 8) - 5;
	*(DWORD*)(code + 9) = callDestini;
	MyFile->seekp(FOAToInsert);
	MyFile->write((char*)code, sizeof(code));

	BYTE jumpCode[5];
	jumpCode[0] = 0xe9;

	DWORD jumpInsertionFOA = FOAToInsert + sizeof(code);
	DWORD jumpInsertionRVA = FOAToRVA(jumpInsertionFOA);
	DWORD currentEntryPoint = peOptionalHeader.baseAddress + peOptionalHeader.addressOfEntryPoint;
	DWORD jumpDestini = currentEntryPoint - (peOptionalHeader.baseAddress + jumpInsertionRVA) - 5;
	*((DWORD*)(jumpCode + 1)) = jumpDestini;

	// Write jump code:
	MyFile->seekp(jumpInsertionFOA);
	MyFile->write((char*)jumpCode, sizeof(jumpCode));

	// Write new entry point
	if (peFileHeader.machine == 0x14)
	{
		// 32 bit
		((_IMAGE_OPTIONAL_HEADER*)peOptionalHeader.getHeaeder())->AddressOfEntryPoint = RVAOfInsertion;
	}
	else {
		((_IMAGE_OPTIONAL_HEADER64*)peOptionalHeader.getHeaeder())->AddressOfEntryPoint = RVAOfInsertion;
	}
	
	//Write entry point
	MyFile->seekp(dosPart.positionOfPESignature + 4 + sizeof(_IMAGE_FILE_HEADER));
	MyFile->write((char*)(peOptionalHeader.getHeaeder()), peFileHeader.sizeOfOptionalHeader);

}

void PEWarrior::modifyEntryPoint(DWORD newEntryPoint)
{
	if (peFileHeader.machine == 0x14c)
	{
		// 32 bit
		((_IMAGE_OPTIONAL_HEADER*)peOptionalHeader.getHeaeder())->AddressOfEntryPoint = newEntryPoint;
	}
	else {
		// 64 bit
		((_IMAGE_OPTIONAL_HEADER64*)peOptionalHeader.getHeaeder())->AddressOfEntryPoint = newEntryPoint;
	}

	MyFile->seekp(dosPart.positionOfPESignature + 4 + sizeof(_IMAGE_FILE_HEADER));
	MyFile->write((char*)peOptionalHeader.getHeaeder(), peFileHeader.sizeOfOptionalHeader);
}


PEWarrior::SectionTables::~SectionTables()
{
	delete[] tableArray;
}

void PEWarrior::SectionTables::setArrayStartAddress(_IMAGE_SECTION_HEADER* tablesAddress)
{
	this->tableArray = tablesAddress;
}

void PEWarrior::SectionTables::setNumberOfSections(int num)
{
	this->numberOfSections = num;
}


void PEWarrior::PEOptionHeader::setHeader(void* headerAddress)
{
	this->peOptionalHeader = headerAddress;
}

void* PEWarrior::PEOptionHeader::getHeaeder()
{
	return this->peOptionalHeader;
}
