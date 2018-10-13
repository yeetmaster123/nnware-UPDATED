#pragma once

#include <string>

namespace Crypto
{
	std::string Encrypt(const std::string& strPlainText, const std::string& strKey, const std::string& strInitializationVector);
	std::string Decrypt(const std::string& strPlainText, const std::string& strKey, const std::string& strInitializationVector);
	std::string MD5Hash(const char* szPlainText);
	std::string Replace(std::string a, std::string b, std::string c);
	std::string HashIdentifier(const char* init_vector, const char* str);
	std::string Base64Decode(const std::string& b64str);
	std::string Base64Encode(const std::string& str);
}