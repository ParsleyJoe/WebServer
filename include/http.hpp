#pragma once

#include <unordered_map>
#include <vector>
#include <string>
#include <ranges>
#include <assert.h>

static constexpr std::string_view DEFAULT_MIME = "application/octet-stream";
static const std::unordered_map<std::string, std::string> mimeTypes{
	{".html", "text/html"},
	{".css",  "text/css"},
	{".js",   "text/javascript"},
	{".png",  "image/png"},
	{".jpg",  "image/jpeg"},
	{".jpeg", "image/jpeg"},
	{".gif",  "image/gif"},
	{".svg",  "image/svg+xml"},
	{".txt",  "text/plain"},
	{".ico", "image/x-icon"}
};

static std::string getContentType(const std::filesystem::path& path) {
	auto extension = path.extension().string();

	if (auto it = mimeTypes.find(extension); it != mimeTypes.end()) {
		return it->second;
	}

	return std::string(DEFAULT_MIME);
}

enum class Parse {
	FAILED, INVALID_REQUEST, SUCCESS
};

class HttpResponse {
public:
	std::vector<char> serialize() {
		if (m_statusCode == 0) {
			std::println(std::cerr, "Failed To Serialize HttpResponse, Response Not Initialized");
		}
		//std::string responseStr = "HTTP/1.1 200 OK\r\n"
		//	"Content-Type: " + std::string(getContentType(std::filesystem::path(finalPath))) + "\r\n"
		//	"Content-Length: " + std::to_string(responseBuf.size()) + "\r\n\r\n";

		std::string header = m_httpVersion + " " + std::to_string(m_statusCode) + " " + m_statusText + "\r\n";
		for (auto& p : m_headers) {
			header = header + p.first + ": " + p.second + "\r\n";
		}
		header = header + "\r\n";
			
		std::vector<char> result{};
		result.reserve(header.size() + m_body.size());
		result.insert(result.end(), header.begin(), header.end());
		result.insert(result.end(), m_body.begin(), m_body.end());

		return result;
	}


	HttpResponse(const char* version, int statusCode, const char* statusText, const char* contentType, std::vector<char>& body, int contentLength)
		: m_valid(true)
		, m_httpVersion(version)
		, m_statusCode(statusCode)
		, m_statusText(statusText)
		, m_body(body)
	{
		m_headers["Content-Type"] = contentType;
		m_headers["Content-Length"] = std::to_string(contentLength);
	}
	HttpResponse()
		: m_valid(false)
		, m_statusCode(500)
	{
	}

	bool isValid() const {
		return m_valid;
	}
private:
	bool m_valid{};
	std::string m_httpVersion{};
	int m_statusCode{};
	std::string m_statusText{};
	std::unordered_map<std::string, std::string> m_headers{};
	std::vector<char> m_body{};
};

class HttpRequest {
public:
	HttpResponse getResponse(std::string_view request) {

		if (this->parseHttpRequest(request) != Parse::SUCCESS) {
			std::println("Couldn't Get a Response From Request: {}", request);
			return HttpResponse{};
		}
		constexpr std::string_view webRoot = "E:/RonitCodeStuff/GameDev/Server_CPP/web/";
		std::string finalPath{ m_path };
		if (finalPath == "/") {
			finalPath = "index.html";
		}
		finalPath = std::string(webRoot) + finalPath;

		std::ifstream file(finalPath, std::ios::binary);
		if (!file.is_open()) {
			std::println("ERROR: The Server asked for a file which Does not exist");
			std::vector<char> sent{};
			HttpResponse noFileResponse{ m_version.c_str(), 404, "BAD", std::string(DEFAULT_MIME).c_str(), sent, 1};
			return noFileResponse;
		}
		std::vector<char> fileRead {
			std::istreambuf_iterator<char>(file),
			std::istreambuf_iterator<char>()
		};
		std::string contentType = getContentType(std::filesystem::path(finalPath));
		HttpResponse response{ m_version.c_str(), 200, "OK", contentType.c_str(), fileRead, static_cast<int>(fileRead.size())};

		return response;
	}

private:
	Parse parseHttpRequest(std::string_view request) {
		auto lineEnd = request.find("\r\n");
		std::string_view failedParseErrorMsg = "Failed to Parse HTTP Request";
		if (lineEnd == std::string_view::npos)
		{
			std::println(std::cerr, "{}", failedParseErrorMsg);
			return Parse::INVALID_REQUEST;
		}
		std::string_view firstLine = request.substr(0, lineEnd);

		auto firstSpace = firstLine.find(' ');
		if (firstSpace == std::string_view::npos) {
			std::println(std::cerr, "{}", failedParseErrorMsg);
			return Parse::INVALID_REQUEST;
		}

		auto secondSpace = firstLine.find(' ', firstSpace + 1);
		if (firstSpace == std::string_view::npos || secondSpace == std::string_view::npos) {
			std::println(std::cerr, "{}", failedParseErrorMsg);
			return Parse::INVALID_REQUEST;
		}

		m_method = firstLine.substr(0, firstSpace);
		m_path = firstLine.substr(firstSpace + 1, secondSpace - firstSpace - 1);
		m_version = firstLine.substr(secondSpace + 1);
		std::println("Method: {}, Path: {}, Version: {}", m_method, m_path, m_version);

		
		// HEADERS
		std::string_view headerString = request.substr(lineEnd);
		headerString.remove_prefix(2);
		auto currIndex = 0;
		while (currIndex != std::string::npos && currIndex < headerString.size()) {
			auto nextLine = headerString.find("\r\n", currIndex);
			if (nextLine == std::string_view::npos || nextLine == currIndex) {
				break;
			}

			std::string_view line = headerString.substr(currIndex, nextLine - currIndex);
			auto colon = line.find(":");
			if (colon == std::string_view::npos) {
				break;
			}

			std::string key{ line.substr(0, colon) };
			std::string value{ line.substr(colon + 1) };

			// Remove whitespace
			if (!value.empty() && value.front() == ' ') {
				value.erase(0, 1);
			}
			m_headers.emplace(key, value);

			currIndex = nextLine + 2;

		}

		std::println("Map Created:");
		for (auto& pair : m_headers) {
			std::println("Key: {} Value: {}", pair.first, pair.second);
		}

		// BODY
		auto bodyStart = request.find("\r\n\r\n");
		if (bodyStart == std::string_view::npos) {
			// FIXME
			// Failed to get body
		}
		int contentLength = std::stoi(m_headers.find("Content-Length")->second);
		bodyStart += 4;
		auto body = request.substr(bodyStart);
		m_body.assign(body.begin(), body.end());
		
		if (m_body.size() < contentLength) {
			std::println(std::cerr, "ERROR: Request body received is incomplete");
		}

		return Parse::SUCCESS;
	}

	std::string m_method;
	std::string m_path;
	std::string m_version;
	std::unordered_map<std::string, std::string> m_headers;
	std::vector<char> m_body;
};
