#include <iostream>
#include <print>
#include <sstream>
#include <fstream>
#include <filesystem>

#pragma comment(lib, "ws2_32.lib")
#include <WinSock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#include <unordered_map>

#include "http.hpp"


class NetworkManager {
public:
	bool connect() {
		m_serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (m_serverSocket == INVALID_SOCKET) {
			std::println("socket() failed: {}", WSAGetLastError());
			WSACleanup();
			return false;
		}

		sockaddr_in serverAddress{};
		serverAddress.sin_family = AF_INET;
		serverAddress.sin_addr.S_un.S_addr = INADDR_ANY;
		serverAddress.sin_port = htons(PORT);

		if (bind(m_serverSocket, reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress)) == SOCKET_ERROR) {
			std::println("bind() failed: {}", WSAGetLastError());
			closesocket(m_serverSocket);
			WSACleanup();
			return false;
		}

		if (listen(m_serverSocket, SOMAXCONN) == SOCKET_ERROR) {
			std::println("listen() failed: {}", WSAGetLastError());
			closesocket(m_serverSocket);
			WSACleanup();
			return false;
		}

		std::println("Server listening on port {}", PORT);
		m_isConnected = true;
	}

	void startListening() {
		if (!m_isConnected) {
			std::println(std::cerr, "Couldn't start listening, Not Connected");
			return;
		}

		while (true) {

			// Listen to socket
			SOCKET clientSocket = accept(m_serverSocket, nullptr, nullptr);
			if (clientSocket == INVALID_SOCKET) {
				std::println("accept() failed: {}", WSAGetLastError());
				return;
			}

			std::println("Client connected!");

			// Receive Http request
			char buffer[1024]{};
			int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
			if (bytesReceived > 0) {
				buffer[bytesReceived] = '\0';
				std::println("[Client sent:]\n {}", buffer);
			}

			// Create new response from request
			HttpRequest httpRequest;
			std::string_view request(buffer, bytesReceived);
			HttpResponse response = httpRequest.getResponse(request);
			std::vector<char> responseBytes = response.serialize();
			if (!response.isValid()) {
				// TODO: Handle the error

			}

			// Send Request !![IN A LOOP]!!
			size_t totalSent = 0;
			size_t size = responseBytes.size();
			const char* data = responseBytes.data();
			while (totalSent < size) {

				int sent = send(clientSocket, data + totalSent, static_cast<int>(size - totalSent), 0);

				if (sent == SOCKET_ERROR) {
					// Well frick
					// FIXME
					break;
				}
				totalSent += sent;
			}
			closesocket(clientSocket);
		}
		m_isConnected = false;
	}

	~NetworkManager() {
		closesocket(m_serverSocket);
		WSACleanup();
	}
private:
	const int PORT = 80;
	SOCKET m_serverSocket{};
	bool m_isConnected = false;
};

int main()
{
	WSADATA wsaData{};
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		std::println("WSAStartup failed");
		return -1;
	}

	NetworkManager networkManager{};
	
	networkManager.connect();
	networkManager.startListening();

	return 0;
}