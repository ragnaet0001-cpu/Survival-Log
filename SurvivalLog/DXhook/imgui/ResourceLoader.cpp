
#include "ResourceLoader.h"
#include "../Utils/utils.h"
#include "../Utils/Logger.h"

std::string ResourceLoader::Load(const char* name, const char* type)
{
	LPBYTE pData = nullptr;
	DWORD size = 0;
	if (!LoadEx(name, type, pData, size))
	{
		LOG_DEBUG("Failed to load resource %s", name);
		return {};
	}

	return std::string(reinterpret_cast<char*>(pData), size);
}

std::string ResourceLoader::Load(int resID, const char* type)
{
	return ResourceLoader::Load(MAKEINTRESOURCEA(resID), type);
}
//资源加载
bool ResourceLoader::LoadEx(const char* name, const char* type, LPBYTE& pDest, DWORD& size)
{
	if (s_Handle == nullptr)
		return false;

	HRSRC hResource = FindResourceA(s_Handle, name, type);
	if (hResource) {
		HGLOBAL hGlob = LoadResource(s_Handle, hResource);
		if (hGlob) {
			size = SizeofResource(s_Handle, hResource);
			pDest = static_cast<LPBYTE>(LockResource(hGlob));
			if (size > 0 && pDest)
				return true;
		}
	}
	return false;
}
//路径加载
//bool ResourceLoader::LoadEx(const char* name, const char* type, LPBYTE& pDest, DWORD& size)
//{
//    std::ifstream file(name, std::ios::binary | std::ios::ate);
//    if (!file.is_open()) {
//        return false; 
//    }
//    size = static_cast<DWORD>(file.tellg());
//    file.seekg(0, std::ios::beg); 
//
//    pDest = new BYTE[size];
//    if (!pDest) {
//        return false; 
//    }
//    file.read(reinterpret_cast<char*>(pDest), size);
//    file.close(); 
//    return file.good(); 
//}

bool ResourceLoader::LoadEx(int resId, const char* type, LPBYTE& pDest, DWORD& size)
{
	return ResourceLoader::LoadEx(MAKEINTRESOURCEA(resId), type, pDest, size);
}

void ResourceLoader::SetModuleHandle(HMODULE handle)
{
	s_Handle = handle;
}
