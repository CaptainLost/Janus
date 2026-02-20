#pragma once

#include <string>

namespace UrlUtils {

inline std::string StripProtocol(const std::string& url)
{
	auto pos = url.find("://");

	return pos != std::string::npos ? url.substr(pos + 3) : url;
}

inline std::string GetHost(const std::string& url)
{
	std::string noProto = StripProtocol(url);
	size_t slashPos = noProto.find('/');
	if (slashPos != std::string::npos)
		return noProto.substr(0, slashPos);
	size_t queryPos = noProto.find('?');
	if (queryPos != std::string::npos)
		return noProto.substr(0, queryPos);
	return noProto;
}

inline std::string GetPath(const std::string& url)
{
	std::string noProto = StripProtocol(url);
	size_t slashPos = noProto.find('/');
	return slashPos != std::string::npos ? noProto.substr(slashPos) : "/";
}

inline bool IsHttp(const std::string& url)
{
	return url.size() > 7 &&
		(url.substr(0, 7) == "http://" || url.substr(0, 8) == "https://");
}

inline bool IsSameDomain(const std::string& hostA, const std::string& hostB)
{
	if (hostA == hostB)
		return true;

	auto isSubdomainOf = [](const std::string& sub, const std::string& domain) {
		if (sub.size() <= domain.size() + 1)
			return false;
		return sub[sub.size() - domain.size() - 1] == '.' &&
		       sub.substr(sub.size() - domain.size()) == domain;
	};

	return isSubdomainOf(hostA, hostB) || isSubdomainOf(hostB, hostA);
}

} // namespace UrlUtils
