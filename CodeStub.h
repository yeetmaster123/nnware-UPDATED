#pragma once

#include "ResourceRetriever.h"
#include <map>

#define STRING_SECTION XorStr("[Strings]")
#define NETVAR_SECTION XorStr("[Netvars]")

class CodeStub : public ResourceRetriever
{
public:
	CodeStub() : ResourceRetriever() { }
	~CodeStub();

	void Init() { LoadFromForum(); }

	std::string GetString(std::string key);
	ptrdiff_t GetNetvar(std::string key);

private:
	void LoadFromForum();

	std::map<std::string, ptrdiff_t> NetvarMap;
	std::map<std::string, std::string> StringMap;
};

