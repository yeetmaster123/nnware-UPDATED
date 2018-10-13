#pragma once

#include <string>

enum PostfieldType
{
	NORMAL = 0,
	STUBS = 1,
};

class ForumInterface
{
public:
	static ForumInterface* Get();

	void SetCredentials(std::string s_user, std::string s_pass);
	bool HasCredentials();
	std::string GetUser();
	std::string GetPass();
	uint8_t GetPartialXorKey();
	std::string MakeWebRequest(std::string uri, std::string request_path, std::string postfields, std::string body);
	std::string GeneratePostFields(PostfieldType Posts = PostfieldType::NORMAL);
	void GetKeyAndIV(std::string& key, std::string& iv);
	void BanUser(std::string reason);
protected:
	std::string xor_user, xor_pass;
	ForumInterface();
	~ForumInterface();

	void operator=(const ForumInterface&) = delete;
};

