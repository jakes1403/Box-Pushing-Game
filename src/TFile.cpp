#include "TFile.h"
#include <cstdint>
#include <vector>
#include <string>

using namespace std;

namespace Thruster
{
	TFile LoadTFileFromLocalPath(string path)
	{
		TFile file;

		ifstream fileStream;
		fileStream.open(path, ios::binary | ios::in | ios::ate);

		if (fileStream.is_open())
		{
			file.Size = fileStream.tellg();
			file.Data = new char[file.Size];
			fileStream.seekg(0, ios::beg);
			fileStream.read(file.Data, file.Size);
			fileStream.close();
		}
		else
		{
			printf("Failure Loading TFile: %s", path.c_str());
		}

		return file;
	}

	void WriteTFileToLocalPath(TFile& file, string path)
	{
		ofstream ofileStream;
		ofileStream.open(path, ios::binary | ios::out);

		if (ofileStream.is_open()) {
			ofileStream.write(file.Data, file.Size);
			ofileStream.close();
		}
		else
		{
			printf("Failure Writing TFile: %s", path.c_str());
		}
	}

	string TFileToString(TFile file)
	{
		string asString = "";
		for (int i = 0; i < file.Size; i++)
		{
			asString += file.Data[i];
		}
		return asString;
	}

	TFile StringToTFile(string data)
	{
		TFile file;
		file.Size = data.length();
		file.Data = new char[file.Size];

		for (int i = 0; i < data.length(); i++)
		{
			file.Data[i] = data[i];
		}

		return file;
	}

	
}