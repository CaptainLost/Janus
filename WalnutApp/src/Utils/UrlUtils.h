#pragma once

#include <cctype>
#include <cstdio>
#include <string>
#include <string_view>

namespace UrlUtils
{
	inline std::string StripProtocol(const std::string& url)
	{
		auto pos = url.find("://");
		return pos != std::string::npos ? url.substr(pos + 3) : url;
	}

	inline std::string EnsureProtocol(const std::string& url)
	{
		if (url.find("://") != std::string::npos)
		{
			return url;
		}

		return "https://" + url;
	}

	inline std::string GetHost(const std::string& url)
	{
		std::string noProto = StripProtocol(url);

		size_t slashPos = noProto.find('/');
		if (slashPos != std::string::npos)
		{
			return noProto.substr(0, slashPos);
		}

		size_t queryPos = noProto.find('?');
		if (queryPos != std::string::npos)
		{
			return noProto.substr(0, queryPos);
		}

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
		return url.starts_with("http://") || url.starts_with("https://");
	}

	inline bool IsHttps(const std::string& url)
	{
		return url.starts_with("https://");
	}

	inline std::string UrlEncodeQuery(const std::string& query)
	{
		std::string result;

		for (unsigned char c : query)
		{
			if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
			{
				result += static_cast<char>(c);
			}
			else if (c == ' ')
			{
				result += '+';
			}
			else
			{
				char buf[4];
				snprintf(buf, sizeof(buf), "%%%02X", c);
				result += buf;
			}
		}

		return result;
	}

	inline std::string GetDomainDisplayName(const std::string& url)
	{
		std::string name = UrlUtils::GetHost(url);

		if (name.size() > 4 && name.substr(0, 4) == "www.")
		{
			name = name.substr(4);
		}

		size_t dotPosition = name.find('.');
		if (dotPosition != std::string::npos)
		{
			name = name.substr(0, dotPosition);
		}

		if (!name.empty())
		{
			name[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(name[0])));
		}

		return name;
	}

	inline bool IsSameDomain(const std::string& hostA, const std::string& hostB)
	{
		if (hostA == hostB)
		{
			return true;
		}

		auto isSubdomainOf = [](const std::string& sub, const std::string& domain) {
			if (sub.size() <= domain.size() + 1)
			{
				return false;
			}
			return sub[sub.size() - domain.size() - 1] == '.'
			    && sub.ends_with(domain);
		};

		return isSubdomainOf(hostA, hostB) || isSubdomainOf(hostB, hostA);
	}
}
