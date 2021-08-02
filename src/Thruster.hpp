/*
Copyright (c) 2019-2021 Jacob Allen
See LICENSE file at root of project for license
*/

// This is the non-Box Pushing Game related code for ThrusterEngine.
// Adaptable to other projects, currently its a mess and this is a very old version of the file

#ifdef __PSP__
#include <pspkernel.h>
#include <pspdebug.h>
#include <pspdisplay.h>
#include "./common/callback.h"
#endif

#include "ThrusterConfig.h"
#include "ThrusterWindowing.h"

#include <chipmunk/chipmunk.h>

#include <iostream>
#include <string>
#include <fstream>
#include <map>
#include <vector>
#include <time.h>
#include <functional>

#include <memory>

#include "TFile.h"

#ifndef EMSCRIPTEN
#include "TMessageBox.hpp"
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#if COMPILE_WITH_IMGUI
#include "../src/imgui/imgui.h"
#include "../src/imgui/imgui_sdl.h"
#include "../src/imgui/imgui_impl_sdl.h"
#endif

#define GLM_SWIZZLE 
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <unordered_map>

using namespace std;
using namespace glm;

namespace Thruster
{
	SDL_RWops* TFileToSDL_RW(TFile& file)
	{
		return SDL_RWFromMem(file.Data, file.Size);
	}
	class TPak {
	public:
		void AddFile(TFile toAdd, string name)
		{
			PackedFile newFile;
			newFile.file = toAdd;
			newFile.name = name;
			myFiles.push_back(newFile);
		}

		TFile& GetFile(string name)
		{
			for (auto& current : myFiles)
			{
				if (current.name == name)
				{
					return current.file;
				}
			}
		}

		void Free()
		{
			for (auto& myFile : myFiles)
			{
				myFile.file.Destroy();
			}
			myFiles.clear();
		}

		void UseFile(string file)
		{
			Free();
			queried = QueryTPak(file);
			currentTPakFile = file;
		}

		void ReadFull()
		{
			ifstream fileStream;
			fileStream.open(currentTPakFile, ios::binary | ios::in | ios::ate);

			if (fileStream.is_open())
			{
				unsigned long long size = fileStream.tellg();
				int numItems = queried.fileInfos.size();

				for (int i = 0; i < numItems; i++)
				{
					unsigned long long myLength;
					if (i + 1 == numItems)
					{
						unsigned long long last = size;
						myLength = last - queried.fileInfos[i].offset;
					}
					else
					{
						myLength = queried.fileInfos[i + 1].offset - queried.fileInfos[i].offset;
					}
					PackedFile toPush;
					toPush.file.Size = myLength;
					toPush.file.Data = new char[myLength];
					fileStream.seekg(queried.dataOffset + queried.fileInfos[i].offset, ios::beg);
					fileStream.read(toPush.file.Data, toPush.file.Size);
					toPush.name = queried.fileInfos[i].name;

					myFiles.push_back(toPush);
				}

				fileStream.close();
			}
			else
			{
				printf("Failure Loading TPak: %s", currentTPakFile.c_str());
			}
		}

		void WriteFull(string path)
		{
			ofstream ofileStream;
			ofileStream.open(path, ios::binary | ios::out);

			if (ofileStream.is_open()) {
				unsigned long long currentOffset = 0;
				for (auto& myFile : myFiles)
				{
					ofileStream << myFile.name << endl;
					ofileStream << "\t" << currentOffset << endl;
					currentOffset += myFile.file.Size;
				}
				ofileStream << "\n\n";
				for (auto& myFile : myFiles)
				{
					ofileStream.write(myFile.file.Data, myFile.file.Size);
				}
				ofileStream.close();
			}
			else
			{
				printf("Failure Writing TPak: %s", path.c_str());
			}
		}
	private:
		struct TFileInfo {
			std::string name;
			unsigned long long offset;
		};

		struct TFileInfoQuery {
			unsigned long long dataOffset;
			vector<TFileInfo> fileInfos;
		};

		struct PackedFile {
			TFile file;
			std::string name;
		};

		std::string currentTPakFile;

		vector<PackedFile> myFiles;

		TFileInfoQuery queried;

		TFileInfoQuery QueryTPak(std::string pathToQuery)
		{
			ifstream fileStream(pathToQuery);

			if (fileStream.is_open())
			{
				string data;
				char c;
				bool isLastNewLine = false;
				bool stop = false;
				while (!stop)
				{
					fileStream.get(c);
					if (c == '\n' && isLastNewLine)
					{
						stop = true;
					}
					if (c == '\n')
					{
						isLastNewLine = true;
					}
					else
					{
						isLastNewLine = false;
					}
					data += c;
				}
				fileStream.close();

				unsigned long long dataOffset = data.size();

				data.pop_back();
				data.pop_back();

				string buf = "";

				vector<TFileInfo> inf;

				vector<string> raw;

				for (unsigned long long i = 0; i < data.size(); i++)
				{
					buf += data[i];
					if (data[i] == '\n')
					{
						buf.pop_back();
						raw.push_back(buf);
						buf = "";
					}
					if (data[i] == '\t')
					{
						buf.pop_back();
					}
				}
				raw.push_back(buf);

				for (int i = 0; i < raw.size(); i += 2)
				{
					TFileInfo info;
					info.name = raw[i];
					info.offset = std::stoull(raw[i + 1]);
					inf.push_back(info);
				}

				TFileInfoQuery result;
				result.dataOffset = dataOffset + 1;
				result.fileInfos = inf;

				return result;
			}
			else
			{
				printf("Failure Loading TPak: %s", pathToQuery.c_str());
			}
		}
	};

	class TLoadManager {
	public:
		void SetLoadLocationToPath(string path)
		{
			loadFromPak = false;
			pathToLoadFrom = path;
			if (pathToLoadFrom.back() != '/' || pathToLoadFrom.back() != '\\')
			{
				pathToLoadFrom.push_back('/');
			}
		}
		void SetLoadLocationToTPak(string path)
		{
			loadFromPak = true;
			pathToLoadFrom = path;

			gameAssets.UseFile(path);

			gameAssets.ReadFull();
		}
		TFile& RequestAsset(string name)
		{
			if (loadFromPak)
			{
				return gameAssets.GetFile(name);
			}
			else
			{
				TFile loaded = LoadTFileFromLocalPath(pathToLoadFrom + name);
				return loaded;
			}
		}
	private:
		bool loadFromPak = false;
		string pathToLoadFrom = "";
		TPak gameAssets;
	} TAssetManager;

	inline void initRandom()
	{
		srand(time(NULL));
	}

	inline int randomInt(int from, int to)
	{
		return rand() % (to - from) + from;
	}

	cpSpace* physSpace;

	typedef TFLOAT_DATATYPE tfloat;

#define PI (4*atan(1))

#define degToRad(n) (n * PI / 180)

	int runningStatus = TWin_Good;

	class TVec2D {
	public:
		struct {
			tfloat x = 0;
			tfloat y = 0;
			tfloat w = 1;
			tfloat h = 1;
			tfloat r = 90;
		};
		void incWithDeltaX(tfloat incX)
		{
			x += incX * deltaTime;
		}
		void incWithDeltaY(tfloat incY)
		{
			y += incY * deltaTime;
		}
		void incWithDeltaW(tfloat incW)
		{
			w += incW * deltaTime;
		}
		void incWithDeltaH(tfloat incH)
		{
			h += incH * deltaTime;
		}
		void incWithDeltaR(tfloat incR)
		{
			r += incR * deltaTime;
		}
		SDL_Rect toSDLRect()
		{
			return { (int)x, (int)y, (int)w, (int)h };
		}
		cpVect toCpVect()
		{
			return cpv(x, y);
		}
	};

	bool operator==(const TVec2D& comp1, const TVec2D& comp2)
	{
		return comp1.x == comp2.x && comp1.y == comp2.y && comp1.w == comp2.w && comp1.h == comp2.h && comp1.r == comp2.r;
	}

	bool operator>(const TVec2D& comp1, const TVec2D& comp2)
	{
		return comp1.x > comp2.x && comp1.y > comp2.y && comp1.w > comp2.w && comp1.h > comp2.h && comp1.r > comp2.r;
	}

	bool operator<(const TVec2D& comp1, const TVec2D& comp2)
	{
		return comp1.x < comp2.x&& comp1.y < comp2.y&& comp1.w < comp2.w&& comp1.h < comp2.h&& comp1.r < comp2.r;
	}

	bool operator>=(const TVec2D& comp1, const TVec2D& comp2)
	{
		return comp1.x >= comp2.x && comp1.y >= comp2.y && comp1.w >= comp2.w && comp1.h >= comp2.h && comp1.r >= comp2.r;
	}

	bool operator<=(const TVec2D& comp1, const TVec2D& comp2)
	{
		return comp1.x <= comp2.x && comp1.y <= comp2.y && comp1.w <= comp2.w && comp1.h <= comp2.h && comp1.r <= comp2.r;
	}

	TVec2D operator+(const TVec2D& op1, const TVec2D& op2)
	{
		return { op1.x + op2.x, op1.y + op2.y, op1.w + op2.w, op1.h + op2.h, op1.r + op2.r };
	}

	TVec2D operator-(const TVec2D& op1, const TVec2D& op2)
	{
		return { op1.x - op2.x, op1.y - op2.y, op1.w - op2.w, op1.h - op2.h, op1.r - op2.r };
	}

	std::ostream& operator<<(std::ostream& out, const TVec2D& op1)
	{
		return out << "{" << op1.x << ", " << op1.y << ", " << op1.w << ", " << op1.h << ", " << op1.r << "}";
	}

	struct TGameWindowInfo {
		TVec2D transform;
		bool isWindowMain = true;
	}
	GameWindowInfo;

	class TTexture {
	public:
#if COMPILE_WITH_IMGUI
		operator ImTextureID()
		{
			return (void*)(intptr_t)texture;
		}
#endif
		void loadImage(string path)
		{
			TFile& requestedFile = TAssetManager.RequestAsset(path);
			bool flip;
#if GRAPHICS_IMPLEMENTATION == G_IMPL_SDL2
			flip = false;
#else
			flip = true;
#endif
			stbi_set_flip_vertically_on_load(flip);
			unsigned char* raw = stbi_load_from_memory((stbi_uc*)requestedFile.Data, requestedFile.Size, &width, &height, &numChannels, 4);
#if GRAPHICS_IMPLEMENTATION == G_IMPL_SDL2
			SDL_Surface* loaded = SDL_CreateRGBSurfaceWithFormatFrom((void*)raw, width, height, 32, 4*width, SDL_PIXELFORMAT_RGBA32);
			loadRaw(loaded);
#endif
#if GRAPHICS_IMPLEMENTATION == G_IMPL_OPENGL3
			glGenTextures(1, &texture);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, texture);


			if (raw)
			{
				if (numChannels == 3)
				{
					format = GL_RGB;
				}
				if (numChannels == 4)
				{
					format = GL_RGBA;
				}
			}
			else
			{
				string errMessage = "Unable to load TTexture " + path;
				ThrusterNativeDialogs::showMessageBox(errMessage.c_str(), "TTexture Warning", ThrusterNativeDialogs::Style::Warning);
			}

			glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, raw);
			glGenerateMipmap(GL_TEXTURE_2D);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			float borderColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
			glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
			//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			stbi_image_free(raw);
#endif

		}
		//	void loadText(TText text)
		//	{
		//#if GRAPHICS_IMPLEMENTATION == G_IMPL_SDL2
		//		loadRaw(text.textSurface);
		//#endif
		//	}
#if GRAPHICS_IMPLEMENTATION == G_IMPL_SDL2
		void loadRaw(SDL_Surface* surface)
		{
			//SDL_Surface* imageSurface = SDL_ConvertSurface(surface, screenSurface->format, 0);
			texture = SDL_CreateTextureFromSurface(renderer, surface);

			SDL_FreeSurface(surface);

		}
#endif
#if GRAPHICS_IMPLEMENTATION == G_IMPL_OPENGL3
		/*void loadRaw(unsigned char* raw)
		{

		}*/
#endif
		void destroy()
		{
#if GRAPHICS_IMPLEMENTATION == G_IMPL_SDL2
			SDL_DestroyTexture(texture);
#endif
		}
		int width;
		int height;
		int numChannels;
#if GRAPHICS_IMPLEMENTATION == G_IMPL_SDL2
		SDL_Texture* texture;
#endif
#if GRAPHICS_IMPLEMENTATION == G_IMPL_OPENGL3
		GLenum format;
		unsigned int texture;
#endif
	private:
#if GRAPHICS_IMPLEMENTATION == G_IMPL_OPENGL3
#endif
	};

	class TShader {
	public:
		void AttachShader(string path, uint32_t type)
		{
#if GRAPHICS_IMPLEMENTATION == G_IMPL_OPENGL3
			unsigned int currentShader = LoadShader(path, type);
			shaders.push_back(currentShader);
#endif
		}
		void Create()
		{
#if GRAPHICS_IMPLEMENTATION == G_IMPL_OPENGL3
			ShaderProgram = glCreateProgram();

			for (auto currentShader : shaders)
			{
				glAttachShader(ShaderProgram, currentShader);
			}

			glLinkProgram(ShaderProgram);

			for (auto currentShader : shaders)
			{
				glDeleteShader(currentShader);
			}

			int successful;
			char info[512];
			glGetProgramiv(ShaderProgram, GL_LINK_STATUS, &successful);
			if (!successful)
			{
				glGetProgramInfoLog(ShaderProgram, 512, NULL, info);
				ThrusterNativeDialogs::showMessageBox(info, "Shader Linking Error", ThrusterNativeDialogs::Style::Warning);
			}
#endif
		}
		inline void Use()
		{
#if GRAPHICS_IMPLEMENTATION == G_IMPL_OPENGL3
			glUseProgram(ShaderProgram);
#endif
		}
		void ReadyTextures()
		{
#if GRAPHICS_IMPLEMENTATION == G_IMPL_OPENGL3
			Use();
			for (int i = 0; i <= knownTextures.size(); i++)
			{
				glActiveTexture(GL_TEXTURE0 + i); // Texture unit i
				glBindTexture(GL_TEXTURE_2D, idToTTex[i].texture);
				//cout << "Texture " << i << endl;
			}
#endif
		}
		void SetUniformFloat(string uniformName, float& toSet)
		{
#if GRAPHICS_IMPLEMENTATION == G_IMPL_OPENGL3
			Use();
			int UniformLocation = glGetUniformLocation(ShaderProgram, uniformName.c_str());
			glUniform1f(UniformLocation, toSet);
#endif
		}
		void SetUniformInt(string uniformName, int& toSet)
		{
#if GRAPHICS_IMPLEMENTATION == G_IMPL_OPENGL3
			Use();
			int UniformLocation = glGetUniformLocation(ShaderProgram, uniformName.c_str());
			glUniform1i(UniformLocation, toSet);
#endif
		}
		void SetUniformBool(string uniformName, bool& toSet)
		{
#if GRAPHICS_IMPLEMENTATION == G_IMPL_OPENGL3
			Use();
			int UniformLocation = glGetUniformLocation(ShaderProgram, uniformName.c_str());
			glUniform1i(UniformLocation, toSet);
#endif
		}
		void SetUniformVec4(string uniformName, vec4& toSet)
		{
#if GRAPHICS_IMPLEMENTATION == G_IMPL_OPENGL3
			Use();
			int UniformLocation = glGetUniformLocation(ShaderProgram, uniformName.c_str());
			glUniform4fv(UniformLocation, 1, glm::value_ptr(toSet));
#endif
		}
		void SetUniformVec2(string uniformName, vec2& toSet)
		{
#if GRAPHICS_IMPLEMENTATION == G_IMPL_OPENGL3
			Use();
			int UniformLocation = glGetUniformLocation(ShaderProgram, uniformName.c_str());
			glUniform2fv(UniformLocation, 1, glm::value_ptr(toSet));
#endif
		}
		void SetUniformMat4(string uniformName, glm::mat4& toSet)
		{
#if GRAPHICS_IMPLEMENTATION == G_IMPL_OPENGL3
			Use();
			int UniformLocation = glGetUniformLocation(ShaderProgram, uniformName.c_str());
			glUniformMatrix4fv(UniformLocation, 1, GL_FALSE, glm::value_ptr(toSet));
#endif
		}

		void SetUniformTTexture(string uniformName, TTexture& texture)
		{
#if GRAPHICS_IMPLEMENTATION == G_IMPL_OPENGL3
			Use();
			int UniformLocation = glGetUniformLocation(ShaderProgram, uniformName.c_str());

			if (std::find(knownTextures.begin(), knownTextures.end(), uniformName) == knownTextures.end()) // If this is a new texture
			{
				knownTextures.push_back(uniformName);
				idToTTex[currentTexture] = texture;
				nameToId[uniformName] = currentTexture;

				currentTexture++;
			}

			idToTTex[nameToId[uniformName]] = texture;
			glUniform1i(UniformLocation, nameToId[uniformName]);
#endif
		}

	private:
#if GRAPHICS_IMPLEMENTATION == G_IMPL_OPENGL3
		map<int, TTexture> idToTTex;
		map<string, int> nameToId;

		vector<string> knownTextures;

		int currentTexture = 0;

		unsigned int ShaderProgram;
		vector<unsigned int> shaders;
		unsigned int LoadShader(string path, GLenum shaderType)
		{
			unsigned int Shader;

			string codeString = TFileToString(TAssetManager.RequestAsset(path)).c_str();

			const char* shaderSource = codeString.c_str();

			/*ifstream codeFile(path);

			string shaderCodeString((istreambuf_iterator<char>(codeFile)), istreambuf_iterator<char>());

			const char* shaderSource = shaderCodeString.c_str();

			codeFile.close();*/

			Shader = glCreateShader(shaderType);
			glShaderSource(Shader, 1, &shaderSource, NULL);
			glCompileShader(Shader);

			int successful;
			char info[512];
			glGetShaderiv(Shader, GL_COMPILE_STATUS, &successful);
			if (!successful)
			{
				glGetShaderInfoLog(Shader, 512, NULL, info);
				string title = "Shader Compilation Error: " + path;
				ThrusterNativeDialogs::showMessageBox(info, title.c_str(), ThrusterNativeDialogs::Style::Warning);
			}

			return Shader;
		}
#endif
	};

	TShader grabMainShader()
	{
		TShader tempMain;
#if GRAPHICS_IMPLEMENTATION == G_IMPL_OPENGL3
		tempMain.AttachShader("MainVertex.vert", GL_VERTEX_SHADER);
		tempMain.AttachShader("MainFrag.frag", GL_FRAGMENT_SHADER);
#endif

		return tempMain;
	}

	TShader MainShader;


#if GRAPHICS_IMPLEMENTATION == G_IMPL_OPENGL3
	class TModel {
	public:
		void LoadPrimitiveCube()
		{
			Vertices = {
				// X   // Y   // Z    // U  // V
				-0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
				 0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
				 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
				 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
				-0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
				-0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

				-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
				 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
				 0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
				 0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
				-0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
				-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

				-0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
				-0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
				-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
				-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
				-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
				-0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

				 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
				 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
				 0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
				 0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
				 0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
				 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

				-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
				 0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
				 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
				 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
				-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
				-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

				-0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
				 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
				 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
				 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
				-0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
				-0.5f,  0.5f, -0.5f,  0.0f, 1.0f
			};
		}
		void LoadPrimitivePlane()
		{
			Vertices = {
				// X   // Y   // Z    // U  // V
				-0.5f, -0.5f, 0.0f,  0.0f, 0.0f,
				 0.5f, -0.5f, 0.0f,  1.0f, 0.0f,
				 0.5f,  0.5f, 0.0f,  1.0f, 1.0f,
				 0.5f,  0.5f, 0.0f,  1.0f, 1.0f,
				-0.5f,  0.5f, 0.0f,  0.0f, 1.0f,
				-0.5f, -0.5f, 0.0f,  0.0f, 0.0f,

			};
		}
		void Create()
		{
			unsigned int VertexBufferObj;
			glGenBuffers(1, &VertexBufferObj); // Create buffers for VertexBuffer

			//glGenBuffers(1, &ElementBufferObj); // Creat buffers for ElementBuffer

			// Load vertices into VertexBufferObj, is static so using GL_STATIC_DRAW
			glBindBuffer(GL_ARRAY_BUFFER, VertexBufferObj);
			glBufferData(GL_ARRAY_BUFFER, Vertices.size() * sizeof(float), &Vertices[0], GL_STATIC_DRAW);

			unsigned int VertexArrayObject;
			glGenVertexArrays(1, &VertexArrayObject);

			glBindVertexArray(VertexArrayObject);

			glBindBuffer(GL_ARRAY_BUFFER, VertexBufferObj);
			glBufferData(GL_ARRAY_BUFFER, Vertices.size() * sizeof(float), &Vertices[0], GL_STATIC_DRAW);

			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, (sizeof(float) * 5), (void*)0); // Location 0 (vec3) Stride of 5, start at 0
			glEnableVertexAttribArray(0);

			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, (sizeof(float) * 5), (void*)(sizeof(float) * 3)); // Location 1 (vec2) stride of 5, start at 3
			glEnableVertexAttribArray(1);

			//modelMatrix = glm::mat4(1.0f);

			//vertSize = Vertices.size() / 5;

			//cout << vertSize << endl;

		}
		void Draw(TShader& Shader)
		{
			Shader.Use();
			modelMatrix = glm::mat4(1.0f);
			modelMatrix = translate(modelMatrix, Position);
			modelMatrix = rotate(modelMatrix, radians(Direction.x), vec3(1.0f, 0.0f, 0.0f));
			modelMatrix = rotate(modelMatrix, radians(Direction.y), vec3(0.0f, 1.0f, 0.0f));
			modelMatrix = rotate(modelMatrix, radians(Direction.z), vec3(0.0f, 0.0f, 1.0f));
			modelMatrix = scale(modelMatrix, glm::max(vec3(0, 0, 0), Scale));

			Shader.SetUniformMat4("model", modelMatrix);

			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			Shader.ReadyTextures();

			//glBindTexture(GL_TEXTURE_2D, Texture.texture);

			//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ElementBufferObj);
			//glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

			glDrawArrays(GL_TRIANGLES, 0, Vertices.size());
		}
		vec3 Position;
		vec3 Direction;
		vec3 Scale;
	private:
		vector<float> Vertices;
		glm::mat4 modelMatrix;
	};

#endif

	inline TVec2D getScreenSize()
	{
		return GameWindowInfo.transform;
	}

	inline TVec2D getCenterOfScreen()
	{
		TVec2D center;

		TVec2D size = getScreenSize();

		center.x = size.w / 2;
		center.y = size.h / 2;

		return center;
	}

	struct WorldTransform {
		glm::mat4 projection = glm::mat4(1.0f);
		glm::mat4 view = glm::mat4(1.0f);
		float mult;
		TVec2D transform;
	}
	CurrentCameraTransform;

	class TCamera {
	public:
		TVec2D transform;
		vec4 bgColor = vec4(0.0, 0.0, 0.0, 1.0);
		void Create()
		{
#ifndef PLATFORM_NO_FRAMEBUFFER
			glGetIntegerv(GL_FRAMEBUFFER, &prev_Framebuffer);
			glGenFramebuffers(1, &Framebuffer);
			glBindFramebuffer(GL_FRAMEBUFFER, Framebuffer);

			glGenTextures(1, &BufferTexture.texture);

			// "Bind" the newly created texture : all future texture functions will modify this texture
			glBindTexture(GL_TEXTURE_2D, BufferTexture.texture);

			glGenerateMipmap(GL_TEXTURE_2D);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			float borderColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
			glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
			//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			// Set "renderedTexture" as our colour attachement #0
			glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, BufferTexture.texture, 0);

			// Set the list of draw buffers.
			GLenum DrawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
			glDrawBuffers(1, DrawBuffers); // "1" is the size of DrawBuffers

			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
				cout << "Failure init rendering to buffer" << endl;

			glBindFramebuffer(GL_FRAMEBUFFER, prev_Framebuffer);
#endif
		}
		void Destroy()
		{

		}
		void Start(float CanvasWidth, float CanvasHeight, float RenderWidth, float RenderHeight)
		{
			renderSize = vec2(RenderWidth, RenderHeight);
			tfloat screenMultiplier;
			tfloat screenSizePercentageW = RenderWidth / CanvasWidth;
			tfloat screenSizePercentageH = RenderHeight / CanvasHeight;
			if (screenSizePercentageH < screenSizePercentageW)
			{
				screenMultiplier = screenSizePercentageH;
			}
			else
			{
				screenMultiplier = screenSizePercentageW;

			}

			CurrentCameraTransform.mult = screenMultiplier;

			CurrentCameraTransform.transform = transform;

			CurrentCameraTransform.view = translate(mat4(1.0f), vec3(CurrentCameraTransform.transform.x * -1.0f, CurrentCameraTransform.transform.y * -1.0f, 0.0f));

			CurrentCameraTransform.projection = glm::ortho((float)((RenderWidth / 2) * -1), (float)(RenderWidth / 2), (float)((RenderHeight / 2) * -1), (float)(RenderHeight / 2), -100.0f, 100.0f);
			CurrentCameraTransform.projection = glm::scale(CurrentCameraTransform.projection, vec3(screenMultiplier, screenMultiplier, 0.0f));
		}
		void End()
		{

		}
		void StartFrameAsRenderer()
		{
#ifndef PLATFORM_NO_FRAMEBUFFER
			BufferTexture.width = renderSize.x;
			BufferTexture.height = renderSize.y;

			glGetIntegerv(GL_FRAMEBUFFER, &prev_Framebuffer);

			// "Bind" the newly created texture : all future texture functions will modify this texture
			glBindTexture(GL_TEXTURE_2D, BufferTexture.texture);

			// Give an empty image to OpenGL ( the last "0" )
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, BufferTexture.width, BufferTexture.height, 0, GL_RGBA, GL_FLOAT, 0);

			glGenerateMipmap(GL_TEXTURE_2D);

			// Render to our framebuffer
			glBindFramebuffer(GL_FRAMEBUFFER, Framebuffer);
			//glViewport(0, 0, BufferTexture.width, BufferTexture.height); // Render on the whole framebuffer, complete from the lower left corner to the upper right

			//glViewport(0, 0, BufferTexture.width, BufferTexture.height);

			glClearColor(bgColor.r, bgColor.b, bgColor.b, bgColor.a);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#endif
		}
		void EndFrameAsRenderer()
		{
#ifndef PLATFORM_NO_FRAMEBUFFER
			// Render to the screen
			glBindFramebuffer(GL_FRAMEBUFFER, prev_Framebuffer);
			// Render on the whole framebuffer, complete from the lower left corner to the upper right
			//glViewport(0, 0, screenSize.w, screenSize.h);
#endif
		}
		TTexture& GetRenderedFrame()
		{
			return BufferTexture;
		}
		bool UseScaling = true;
	private:
		vec2 renderSize;
		int prev_Framebuffer;
		unsigned int Framebuffer;
		TTexture BufferTexture;
	};

	TCamera Camera;


	typedef vector<TVec2D> TVertexList;

	cpVect* TVertexListToCpVect(TVertexList input)
	{
		cpVect* cpVertexList = (cpVect*)calloc(input.size(), sizeof(cpVect));
		int i = 0;
		for (auto const& vertex : input) {
			cpVertexList[i] = cpv(vertex.x, vertex.y);
			i++;
		}
		return cpVertexList;
	}

	vec2 ConvertNativeToWorldCords(vec2 toConvert)
	{
		TVec2D screenCenter = getCenterOfScreen();

		tfloat screenMultiplier = CurrentCameraTransform.mult;

		return {
			((toConvert.x - screenCenter.x) / screenMultiplier) + CurrentCameraTransform.transform.x,
			(((toConvert.y - screenCenter.y) * -1) / screenMultiplier) + CurrentCameraTransform.transform.y
		};
	}

	TVec2D ConvertWorldToNativeCords(TVec2D toConvert)
	{
		TVec2D screenCenter = getCenterOfScreen();
		TVec2D screenSize = getScreenSize();

		tfloat screenMultiplier = screenSize.h / TWin_ScreenHeight;

		return {
			(((toConvert.x - CurrentCameraTransform.transform.x) * screenMultiplier) + screenCenter.x),
			((((toConvert.y * -1) + CurrentCameraTransform.transform.y) * screenMultiplier) + screenCenter.y),
		};
	}

	inline void initPhysics()
	{
		physSpace = cpSpaceNew(); // Creat physics space
	}

	inline void physicsStep(tfloat timeStep)
	{
		cpSpaceStep(physSpace, timeStep);
	}

	inline void setPhysicsGravity(TVec2D gravity)
	{
		cpSpaceSetGravity(physSpace, gravity.toCpVect());
	}

	class TSprite {
	public:
		void createBlank()
		{
			transform = { 0.0f, 0.0f, 1.0f, 1.0f };
			worldSize.w = spriteTex.width;
			worldSize.h = spriteTex.height;

#if GRAPHICS_IMPLEMENTATION == G_IMPL_OPENGL3
			worldModel.LoadPrimitivePlane();
			//worldModel.Scale = vec3((float)spriteTexture.width, (float)spriteTexture.height, 0.0f);
			worldModel.Scale = vec3(100.0f, 100.0f, 0.0f);
			worldModel.Create();
#endif
		}
		void create(TTexture& spriteTexture, TVec2D worldTransform)
		{
			spriteTex = spriteTexture;
			transform = worldTransform;
			worldSize.w = spriteTex.width;
			worldSize.h = spriteTex.height;
#if GRAPHICS_IMPLEMENTATION == G_IMPL_OPENGL3
			worldModel.LoadPrimitivePlane();
			//worldModel.Scale = vec3((float)spriteTexture.width, (float)spriteTexture.height, 0.0f);
			worldModel.Scale = vec3(100.0f, 100.0f, 0.0f);
			worldModel.Create();
#endif
		}
		TVec2D getWorldSize()
		{
			return { 0, 0, (tfloat)(worldSize.w * transform.w), (tfloat)(worldSize.h * transform.h) };
		}
		void setFrame(int frame)
		{
			currentFrame = frame;
			//spriteTex.SetFrame(frame);
		}
		void draw(TShader& myShader = MainShader)
		{
			if (isHidden)
			{
				return;
			}

			myShader.Use();


			vec2 myCords;

			if (attachedToCamera)
			{
				myCords = vec2(transform.x + CurrentCameraTransform.transform.x, transform.y + CurrentCameraTransform.transform.y);
			}
			else
			{
				myCords = vec2(transform.x, transform.y);
			}

			TVec2D spritePos = {myCords.x, myCords.y, transform.w, transform.h, transform.r};

#if GRAPHICS_IMPLEMENTATION == G_IMPL_OPENGL3

			worldModel.Position = vec3(myCords.x, myCords.y, 0.0f);
			worldModel.Scale = vec3(worldSize.w * transform.w, worldSize.h * transform.h, 1.0f);
			worldModel.Direction.z = ((float)transform.r - 90.0f) * -1.0f;

			float time = (float)SDL_GetTicks() / 1000.0f;

			if (isAnimated) {
				shouldBlit = true;
				blit = vec4(
					(animatedFrameSize.w * currentFrame) / spriteTex.width,
					0,
					animatedFrameSize.w / spriteTex.width,
					animatedFrameSize.h / spriteTex.height
				);
			}

			myShader.SetUniformBool("flipX", flipX);
			myShader.SetUniformBool("flipY", flipY);

			myShader.SetUniformFloat("time", time);

			myShader.SetUniformBool("blittingEnabled", shouldBlit);
			myShader.SetUniformVec4("blit", blit);

			myShader.SetUniformMat4("projection", CurrentCameraTransform.projection);
			myShader.SetUniformMat4("view", CurrentCameraTransform.view);

			myShader.SetUniformTTexture("gameTexture", spriteTex);

			worldModel.Draw(myShader);
#endif

			// Set to use TVec2D ConvertNativeToWorldCords(TVec2D toConvert) later
			// Center of screen is (0,0) +Y goes up
#if GRAPHICS_IMPLEMENTATION == G_IMPL_SDL2
			TVec2D screenCenter = getCenterOfScreen();
			SDL_Rect screenPos = {
				(int)(((((spritePos.x - CurrentCameraTransform.transform.x)) - ((worldSize.w * spritePos.w) / 2)) * CurrentCameraTransform.mult) + screenCenter.x),
				(int)(((((((spritePos.y) * -1) + CurrentCameraTransform.transform.y)) - ((worldSize.h * spritePos.h) / 2)) * CurrentCameraTransform.mult) + screenCenter.y),
				(int)(((worldSize.w * spritePos.w) * CurrentCameraTransform.mult)),
				(int)(((worldSize.h * spritePos.h) * CurrentCameraTransform.mult))
			};
			SDL_Rect renderBlit;
			bool noBlit = false;
			if (isAnimated)
			{
				renderBlit = {
					(int)((animatedFrameSize.w * currentFrame) + (animatedFrameSize.x * currentFrame)),
					(int)animatedFrameSize.y,
					(int)animatedFrameSize.w,
					(int)animatedFrameSize.h
				};
			}
			else
			{
				if (shouldBlit)
				{
					renderBlit = { (int)blit.x, (int)blit.y, (int)blit.z, (int)blit.w };
				}
				else
				{
					noBlit = true;
				}
			}

			if ((spritePos.r != 90) || flipX || flipY)
			{
				SDL_RendererFlip flip = SDL_FLIP_NONE;
				if (flipX && !flipY)
				{
					flip = SDL_FLIP_HORIZONTAL;
				}
				else if (!flipX && flipY)
				{
					flip = SDL_FLIP_VERTICAL;
				}
				else if (flipX && flipY)
				{
					//flip = SDL_FLIP_HORIZONTAL | SDL_FLIP_VERTICAL;
				}
				if (noBlit)
				{
					SDL_RenderCopyEx(renderer, spriteTex.texture, NULL, &screenPos, (tfloat)(spritePos.r - 90), NULL, flip);
				}
				else
				{
					SDL_RenderCopyEx(renderer, spriteTex.texture, &renderBlit, &screenPos, (tfloat)(spritePos.r - 90), NULL, flip);
				}

			}
			else
			{
				if (noBlit)
				{
					SDL_RenderCopy(renderer, spriteTex.texture, NULL, &screenPos);
				}
				else
				{
					SDL_RenderCopy(renderer, spriteTex.texture, &renderBlit, &screenPos);
				}

			}
#endif
		}
		void destroy()
		{

		}
		bool isAnimated = false;
		TVec2D animatedFrameSize; // X and Y are padding, w and h are size
		int currentFrame;
		bool isHidden = false;
		bool attachedToCamera = false;
		TVec2D worldSize;
		TVec2D transform;
		bool shouldBlit;
		vec4 blit;
		TTexture spriteTex;
		bool flipX = false;
		bool flipY = false;
	private:
#if GRAPHICS_IMPLEMENTATION == G_IMPL_OPENGL3
		TModel worldModel;
#endif
	};

	class TObject {
	public:
		void createFromSprite(TSprite& tsprite)
		{
			transform = tsprite.transform;
			sprite = tsprite;
		}
		void createBlank(TVec2D transform)
		{
			transform = transform;
		}
		void setHitBoxToPoly(TVertexList inVertexList)
		{
			usingPolyHitBox = true;
			polyVertexes = inVertexList;
		}
		void enablePhysics(TVec2D inHitBox = { 0, 0, 1, 1 }, bool isStatic = true, tfloat mass = 1, tfloat friction = 1, bool disableRotation = false)
		{
			hitBox = inHitBox;

			physicsEnabled = true;

			cpVect* poly = TVertexListToCpVect(polyVertexes);

			if (isStatic)
			{
				physBody = cpSpaceGetStaticBody(physSpace);
			}
			else
			{
				cpFloat moment;
				if (disableRotation) {
					moment = INFINITY;
				}
				else
				{
					if (usingPolyHitBox)
					{
						moment = cpMomentForPoly(mass, polyVertexes.size(), poly, cpvzero, 0);
					}
					else
					{
						moment = cpMomentForBox(mass, hitBox.w, hitBox.h);
					}
				}
				physBody = cpSpaceAddBody(physSpace, cpBodyNew(mass, moment));
			}

			cpBodySetPosition(physBody, cpv(transform.x, transform.y));

			cpShape* shape;

			if (usingPolyHitBox)
			{
				shape = cpPolyShapeNew(physBody, polyVertexes.size(), poly, cpTransformIdentity, 0);
			}
			else
			{
				shape = cpBoxShapeNew(physBody, hitBox.w, hitBox.h, 0.01);
			}

			physShape = cpSpaceAddShape(physSpace, shape);
			cpShapeSetFriction(physShape, friction);

			//cpShapeSetFilter(physShape, cpShapeFilterNew(1, CP_ALL_CATEGORIES, CP_ALL_CATEGORIES));
		}
		void setPosition(TVec2D pos)
		{
			if (physicsEnabled)
			{
				cpBodySetPosition(physBody, cpv(pos.x, pos.y));
				transform.w = pos.w;
				transform.h = pos.h;
			}
			else
			{
				transform = pos;
			}
		}
		void setFrame(int frame)
		{
			sprite.setFrame(frame);
		}
		void update(TShader& myShader = MainShader)
		{
			if (!dead)
			{
				if (physicsEnabled)
				{
					cpVect pos = cpBodyGetPosition(physBody);

					const int forceRotate = -90;

					tfloat rotation = ((cpBodyGetAngle(physBody) * (180 / PI)) + forceRotate) * -1; //4*atan(1) is pi

					transform = {
						(tfloat)pos.x,
						(tfloat)pos.y,
						transform.w,
						transform.h,
						rotation
					};
				}
				oldTransform = sprite.transform;
				sprite.transform = transform;
				sprite.draw(myShader);
			}

		}
		bool hasMovedSinceLastFrame() // NOT IMPLEMENTED YET
		{
			return false;
		}
		void destroy()
		{
			//dead = true;
			//sprite.hide = true;
			setPosition({ 10000, 10000 });
			cpBodyDestroy(physBody);
			//cpBodyFree(physBody);
		}
		TVec2D hitBox;
		cpBody* physBody;
		cpShape* physShape;
		TSprite sprite;
		TVec2D transform;
	private:
		bool dead = false;
		TVertexList polyVertexes;
		bool usingPolyHitBox = false;
		TVec2D oldTransform;
		bool physicsEnabled = false;

	};

	class TSound {
	public:
		void load(string path)
		{
			TFile& soundFile = TAssetManager.RequestAsset(path);
			sound = Mix_LoadWAV_RW(TFileToSDL_RW(soundFile), 1);
		}
		void play()
		{
			Mix_PlayChannel(-1, sound, 0);
		}
		void playLooped()
		{
			Mix_PlayChannel(-1, sound, -1);
		}
		void destroy()
		{
			Mix_FreeChunk(sound);
		}
		// Add destroy function soon
	private:
		Mix_Chunk* sound;
	};

	TVec2D spriteToHitBox(TSprite sprite)
	{
		return {
			0,
			0,
			sprite.getWorldSize().w,
			sprite.getWorldSize().h
		};
	}

	TVec2D getBlitForTileset(int column, int row, int width, int height)
	{
		TVec2D tsBlit;
		tsBlit.x = tfloat(column * width);
		tsBlit.y = tfloat(row * height);
		tsBlit.w = width;
		tsBlit.h = height;
		return tsBlit;
	}

	struct TiledObject {
		int id;
		int gid;
		tfloat x;
		tfloat y;
		tfloat width;
		tfloat height;
		tfloat rotation;
		string name;
		bool isImage;
	};

	struct TiledObjectGroup {
		int id;
		string name;
		vector<TiledObject> objects;
	};

#define ObjectGroup 0
#define TileLayer 1

	void Thruster_Update()
	{
#if GRAPHICS_IMPLEMENTATION == G_IMPL_OPENGL3
		/*MainShader.SetUniformMat4("projection", projection);
		MainShader.SetUniformMat4("view", view);*/
#endif
	}

	static int resizingEventWatcher(void* data, SDL_Event* event) {
		if (event->type == SDL_WINDOWEVENT &&
			event->window.event == SDL_WINDOWEVENT_RESIZED) {
			SDL_Window* win = SDL_GetWindowFromID(event->window.windowID);
			if (win == (SDL_Window*)data) {
				int w, h;
				SDL_GetWindowSize(win, &w, &h);
				if (GameWindowInfo.isWindowMain)
				{
					GameWindowInfo.transform.w = (tfloat)w;
					GameWindowInfo.transform.h = (tfloat)h;
				}
#if GRAPHICS_IMPLEMENTATION == G_IMPL_OPENGL3
				glViewport(0, 0, w, h);
#endif
			}
		}
		return 0;
	}

	string TGameName;

	TCamera bufferCam;

	//bufferCam.Create();

	TSprite drawCam;

	void Thruster_Init(string GameName, int screenWidth, int screenHeight)
	{
#if GRAPHICS_IMPLEMENTATION == G_IMPL_OPENGL3
		TWin_Init(TWin_NoFPSLimit, screenWidth, screenHeight, true, true, true);
#else
		TWin_Init(TWin_UseVSync, screenWidth, screenHeight, true, true, true);
#endif

		TWin_CreateWindow(GameName.c_str());

		GameWindowInfo.transform.w = screenWidth;
		GameWindowInfo.transform.h = screenHeight;

#if GRAPHICS_IMPLEMENTATION == G_IMPL_OPENGL3

		CurrentCameraTransform.projection = glm::ortho((float)((TWin_ScreenWidth / 2) * -1), (float)(TWin_ScreenWidth / 2), (float)((TWin_ScreenHeight / 2) * -1), (float)(TWin_ScreenHeight / 2), -100.0f, 100.0f);

		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);

		MainShader = grabMainShader();

		MainShader.AttachShader("defaultVert.vert", GL_VERTEX_SHADER);
		MainShader.AttachShader("defaultFrag.frag", GL_FRAGMENT_SHADER);

		MainShader.Create();
#endif

		initRandom();

		SDL_AddEventWatch(resizingEventWatcher, TWin_Window);

		drawCam.createBlank();
	}

	class TTimer {
	public:
		void Start(Uint32 expiration = -1)
		{
			isStarted = true;
			timerStartTicks = SDL_GetTicks();
			timerExpiration = expiration;
		}
		Uint32 GetTicks()
		{
			return SDL_GetTicks() - timerStartTicks;
		}
		bool IsExpired()
		{
			isStarted = false;
			return GetTicks() >= timerExpiration;

		}
		bool IsStarted()
		{
			return isStarted;
		}
	private:
		bool isStarted = false;
		Uint32 timerStartTicks;
		Uint32 timerExpiration;

	};


	struct StateContext {
		TTimer Timer;
		bool IsFirstRun = false;
	};

	class TStateManager
	{
	public:
		void ProccessStates()
		{
			if (inState)
			{
				currentState(currentStateContext);
				if (currentStateContext.IsFirstRun == true)
				{
					currentStateContext.IsFirstRun = false;
				}
			}
		}
		void SetState(function<void(StateContext& Context)> state)
		{
			inState = true;
			currentState = state;
			currentStateContext.IsFirstRun = true;
		}
		void ClearState()
		{
			inState = false;
		}
		void SetStateTimed(Uint32 time, function<void(StateContext& Context)> state)
		{
			currentStateContext.Timer.Start(time);
			SetState(state);
		}
		template<class T>
		void SetState(T* target, void (T::* update_func) (StateContext&)) {
			SetState([target, update_func](StateContext& ctx) {
				return (target->*update_func)(ctx);
			});
		}
		template<class T>
		void SetStateTimed(Uint32 time, T* target, void (T::* update_func) (StateContext&)) {
			SetStateTimed(time, [target, update_func](StateContext& ctx) {
				return (target->*update_func)(ctx);
			});
		}
	private:
		bool inState = false;

		function<void(StateContext& Context)> currentState;
		StateContext currentStateContext;
	};

	class TGameObject;

	enum CompType {
		SpriteComp,
		ColiderComp,
		PhysComp
	};

	enum DataTypes {
		String_Type,
		Int_Type,
		Float_Type,
		Double_Type,
		Bool_Type,
		TVertexList_Type,
		PhysShape_Type
	};

	struct ComponentData
	{
		map<string, void*> data;
		map<string, DataTypes> type;
		void SetData(string dataname, DataTypes datatype, void* dataset)
		{
			data[dataname] = dataset;
			type[dataname] = datatype;
		}
	};

	class TComponent
	{
	public:
		CompType Type;

		TComponent(TGameObject* parent) : parent(parent) {}

		virtual void Start() {};

		virtual void Update() {};

		virtual void Destroy() {};

		ComponentData PublicData;

	protected:
		TGameObject* parent;
	};

	class TGameObject
	{
	public:
		string Name = "GameObject";
		void Start()
		{
			for (int i = components.size() - 1; i >= 0; i--)
			{
				components[i]->Start();
			}
			for (int i = Children.size() - 1; i >= 0; i--)
			{
				Children[i]->Start();
			}
		}

		void Update()
		{
			for (int i = components.size() - 1; i >= 0; i--)
			{
				components[i]->Update();
			}
			for (int i = Children.size() - 1; i >= 0; i--)
			{
				Children[i]->Update();
			}
		}

		void Destroy()
		{
			for (int i = components.size() - 1; i >= 0; i--)
			{
				components[i]->Destroy();
				components.clear();
			}
			for (int i = Children.size() - 1; i >= 0; i--)
			{
				Children[i]->Destroy();
				Children.clear();
			}
		}

		template <typename T> shared_ptr<T> AddComponent()
		{
			static_assert(is_base_of<TComponent, T>::value, "Must be of TComponent type");

			for (auto& existing : components) // Do we have a component of this type yet?
			{
				// Prevent adding same type twice
				if (dynamic_pointer_cast<T>(existing))
				{
					return dynamic_pointer_cast<T>(existing);
				}
			}

			shared_ptr<T> newComponent = make_shared<T>(this);

			components.push_back(newComponent);

			return newComponent;
		}

		template <typename T> shared_ptr<T> GetComponent()
		{
			static_assert(is_base_of<TComponent, T>::value, "Must be of TComponent type");

			for (auto& existing : components)
			{
				if (dynamic_pointer_cast<T>(existing))
				{
					return dynamic_pointer_cast<T>(existing);
				}
			}

			return nullptr;
		}

		vec3 position = vec3(0.0f, 0.0f, 0.0f);
		vec3 rotation = vec3(0.0f, 0.0f, 90.0f);
		vec3 scale = vec3(1.0f, 1.0f, 1.0f);

		vector<shared_ptr<TGameObject>> Children;
		vector<shared_ptr<TComponent>> components;
	protected:
		TGameObject* parent;
	};

	class SpriteComponent : public TComponent
	{
	public:
		SpriteComponent(TGameObject* parent) : TComponent(parent)
		{
			PublicData.SetData("FlipX", Bool_Type, (void*)false);
			PublicData.SetData("FlipY", Bool_Type, (void*)false);
			PublicData.SetData("IsHidden", Bool_Type, (void*)false);

			sprite = make_shared<TSprite>();
			TComponent::Type = SpriteComp;
		}

		void Load(TTexture& myTexture)
		{
			sprite->create(myTexture, { 0, 0 });
		}

		void Update() override
		{
			sprite->flipX = PublicData.data["FlipX"];
			sprite->flipY = PublicData.data["FlipY"];
			sprite->isHidden = PublicData.data["IsHidden"];

			sprite->transform = { parent->position.x , parent->position.y, parent->scale.x, parent->scale.y, parent->rotation.z };
			sprite->draw();
		}
		shared_ptr<TSprite> sprite;

	};

	class ColliderComponent : public TComponent
	{
	public:
		ColliderComponent(TGameObject* parent) : TComponent(parent)
		{
			TComponent::Type = ColiderComp;
		}

		void Load(TVertexList& listAdd)
		{
			list = listAdd;
			PublicData.SetData("Vertexes", TVertexList_Type, &list);
		}

	private:
		TVertexList list;
	};

	class PhysicsBodyComponent : public TComponent
	{
	public:
		PhysicsBodyComponent(TGameObject* parent) : TComponent(parent)
		{
			TComponent::Type = PhysComp;
		}

		void Load()
		{
			TVertexList vertexes = *reinterpret_cast<TVertexList*>(parent->GetComponent<ColliderComponent>()->PublicData.data["Vertexes"]);
			cpVect* poly = TVertexListToCpVect(vertexes);
			cpFloat moment = cpMomentForPoly(1.0f, vertexes.size(), poly, cpvzero, 0);

			physBody = cpSpaceAddBody(physSpace, cpBodyNew(1.0f, moment));

			cpBodySetPosition(physBody, cpv(parent->position.x, parent->position.y));

			cpShape* shape = cpPolyShapeNew(physBody, vertexes.size(), poly, cpTransformIdentity, 0);

			physShape = cpSpaceAddShape(physSpace, shape);
		}

		void Update() override
		{
			cpVect pos = cpBodyGetPosition(physBody);

			const int forceRotate = -90;

			tfloat rotation = ((cpBodyGetAngle(physBody) * (180 / PI)) + forceRotate) * -1;

			parent->position = vec3(pos.x, pos.y, parent->position.z);
			parent->rotation.z = rotation;
		}
	private:
		cpBody* physBody;
		cpShape* physShape;
	};

	class TScene
	{
	public:
		TScene()
		{
			SceneRoot = make_shared<TGameObject>();
		}
		virtual void Start() {};
		virtual void PreUpdate() {};
		virtual void Update() {};
		virtual void PostUpdate() {};
		virtual void Destroy() {};
		shared_ptr<TGameObject> SceneRoot;
		TStateManager SceneStateManager;
	};

	class TSceneManager
	{
	public:
		// will init and add scene
		void AddScene(string name, shared_ptr<TScene> scene)
		{
			scenes[name] = scene;
		}
		void UpdateScenes()
		{
			if (currentScene != "")
			{
				scenes[currentScene]->PreUpdate();
			}
			if (currentScene != "")
			{
				scenes[currentScene]->Update();
			}
			if (currentScene != "")
			{
				scenes[currentScene]->SceneRoot->Update();
			}
			if (currentScene != "")
			{
				scenes[currentScene]->SceneStateManager.ProccessStates();
			}
			if (currentScene != "")
			{
				scenes[currentScene]->PostUpdate();
			}
		}
		shared_ptr<TScene> GetCurrentScene()
		{
			return scenes[currentScene];
		}
		void SwitchToScene(string name)
		{
			if (currentScene != "")
			{
				scenes[currentScene]->SceneRoot->Destroy();
			}
			if (currentScene != "")
			{
				scenes[currentScene]->Destroy();
			}
			currentScene = name;
			if (currentScene != "")
			{
				scenes[currentScene]->SceneRoot->Start();
			}
			if (currentScene != "")
			{
				scenes[currentScene]->Start();
			}
		}
	private:
		unordered_map<string, shared_ptr<TScene>> scenes;
		string currentScene = "";
	};

	void RenderCamera(TCamera& RenderCamera, float CanvasWidth, float CanvasHeight, float Renderwidth, float RenderHeight, TShader shader = MainShader)
	{
		bufferCam.Start(CanvasWidth, CanvasHeight, Renderwidth, RenderHeight);

		drawCam.spriteTex = RenderCamera.GetRenderedFrame();

		drawCam.worldSize = { 0, 0, CanvasWidth, CanvasHeight };

		drawCam.draw(shader);

		bufferCam.End();
	}

	class TLightBuffer
	{
	public:
		vec3 color = vec3(0.0, 0.0, 0.0);
		void Init()
		{
			lightCam.Create();
		}
		void StartRender(float canvasWidth, float canvasHeight)
		{
			lightCam.bgColor = vec4(color.r, color.g, color.b, 1.0f);
			lightCam.transform = CurrentCameraTransform.transform;
			lightCam.Start(canvasWidth, canvasHeight, getScreenSize().w, getScreenSize().h);
			lightCam.StartFrameAsRenderer();
		}
		void EndRender()
		{
			lightCam.EndFrameAsRenderer();
			lightCam.End();
		}
		TTexture& GetTexture()
		{
			return lightCam.GetRenderedFrame();
		}
	private:
		TCamera lightCam;
	}
	lightBuff;

	class TLight2D
	{
	public:
		void Create(TVec2D pos, TTexture& cookie)
		{
			transform = pos;
			Cookie = cookie;
			sprite.create(Cookie, pos);

			lightShader = grabMainShader();
#if GRAPHICS_IMPLEMENTATION == G_IMPL_OPENGL3
			lightShader.AttachShader("defaultVert.vert", GL_VERTEX_SHADER);
			lightShader.AttachShader("lightFrag.frag", GL_FRAGMENT_SHADER);
#endif
			lightShader.Create();
		}
		void Draw()
		{
#ifndef PLATFORM_NO_FRAMEBUFFER
			lightShader.Use();
			lightShader.SetUniformVec4("lightColor", color);
			lightShader.SetUniformFloat("brightness", brightness);
			lightShader.SetUniformTTexture("lightBuffer", lightBuff.GetTexture());
			sprite.draw(lightShader);
#endif
		}
		vec4 color = vec4(1.0f, 1.0f, 1.0f, 1.0f);
		float brightness = 1.0f;
		TTexture Cookie;
		TVec2D transform;
		TSprite sprite;
	private:
		TShader lightShader;
	};
}