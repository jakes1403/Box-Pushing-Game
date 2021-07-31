#pragma once

#include <fstream>
#include <SDL2/SDL.h>

namespace Thruster
{
	class TFile {
	public:
		char* Data;
		unsigned long long Size;
		void Destroy()
		{
			delete[] Data;
		}
	};

	TFile LoadTFileFromLocalPath(std::string path);

	void WriteTFileToLocalPath(TFile& file, std::string path);

	std::string TFileToString(TFile file);

	TFile StringToTFile(std::string data);
}