
#include "Manager/osm.Manager_Impl_XAudio2.h"
#include "osm.Sound_Impl.h"

#if _WIN32
#include <Windows.h>
std::wstring ToWide(const char* pText);
void GetDirectoryName(char* dst, char* src);
#endif

int main(int argc, char **argv)
{
#if _WIN32
	char current_path[MAX_PATH + 1];
	GetDirectoryName(current_path, argv[0]);
	SetCurrentDirectoryA(current_path);
#endif

	osm::Sound* staticSound = nullptr;
	osm::Sound* streamSound = nullptr;

	auto manager = new osm::Manager_Impl_XAudio2();
	manager->Initialize();

	{
		FILE* fp = nullptr;
		fopen_s(&fp, "se1.wav", "rb");
		if (fp == nullptr) return 0;

		fseek(fp, 0, SEEK_END);
		auto size = ftell(fp);
		fseek(fp, 0, SEEK_SET);

		std::vector<uint8_t> data;
		data.resize(size);
		fread(data.data(), size, 1, fp);
		fclose(fp);

		staticSound = manager->CreateSound(data.data(), data.size(), true);
	}

	{
		FILE* fp = nullptr;
		fopen_s(&fp, "bgm1.ogg", "rb");
		if (fp == nullptr) return 0;

		fseek(fp, 0, SEEK_END);
		auto size = ftell(fp);
		fseek(fp, 0, SEEK_SET);

		std::vector<uint8_t> data;
		data.resize(size);
		fread(data.data(), size, 1, fp);
		fclose(fp);

		streamSound = manager->CreateSound(data.data(), data.size(), true);
	}

	auto id1 = manager->Play(streamSound);
	Sleep(1000);
	auto id2 = manager->Play(staticSound);
	
	while (manager->IsPlaying(id1) || manager->IsPlaying(id2))
	{
		Sleep(1);
	}

	manager->Finalize();
	staticSound->Release();
	streamSound->Release();
	delete manager;

	
}



#if _WIN32
static std::wstring ToWide(const char* pText)
{
	int Len = ::MultiByteToWideChar(CP_ACP, 0, pText, -1, NULL, 0);

	wchar_t* pOut = new wchar_t[Len + 1];
	::MultiByteToWideChar(CP_ACP, 0, pText, -1, pOut, Len);
	std::wstring Out(pOut);
	delete[] pOut;

	return Out;
}

void GetDirectoryName(char* dst, char* src)
{
	auto Src = std::string(src);
	int pos = 0;
	int last = 0;
	while (Src.c_str()[pos] != 0)
	{
		dst[pos] = Src.c_str()[pos];

		if (Src.c_str()[pos] == L'\\' || Src.c_str()[pos] == L'/')
		{
			last = pos;
		}

		pos++;
	}

	dst[pos] = 0;
	dst[last] = 0;
}
#endif
