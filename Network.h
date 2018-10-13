#pragma once

#include <string>
#include <vector>

namespace Network
{
	enum class EConnection
	{
		CONNECTION_FAILED,
		INVALID_CREDENTIALS,
		NO_PERMISSION,
		INVALID_HWID,
		FILE_NOT_FOUND,
		AUTHENTICATED,
		NUM_RESPONSES
	};

	void HTTPRequest(const std::string uri, const std::string& fields, std::string& response);
}