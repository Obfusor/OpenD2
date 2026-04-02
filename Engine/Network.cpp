#include "../Shared/D2Packets.hpp"
#include "Diablo2.hpp"
#include "Logging.hpp"
#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
typedef SOCKET NetSocket;
#define NET_INVALID INVALID_SOCKET
#define NET_CLOSE closesocket
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
typedef int NetSocket;
#define NET_INVALID (-1)
#define NET_CLOSE close
#endif

#define MAX_PACKET_SIZE 512
#define MAX_CLIENTS 8

namespace Network
{
	static bool bInitialized = false;
	static NetSocket gServerSocket = NET_INVALID;
	static NetSocket gClientSocket = NET_INVALID;
	static NetSocket gClientConnections[MAX_CLIENTS];
	static int gnMaxPlayers = 1;
	static int gnConnectedClients = 0;

	void Init()
	{
#ifdef _WIN32
		WSADATA wsaData;
		int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (result != 0)
		{
			Log::Print(PRIORITY_MESSAGE, "Network: WSAStartup failed: %d", result);
			return;
		}
#endif
		for (int i = 0; i < MAX_CLIENTS; i++)
			gClientConnections[i] = NET_INVALID;

		bInitialized = true;
		Log::Print(PRIORITY_MESSAGE, "Network: Initialized (platform sockets)");
	}

	void SetMaxPlayerCount(DWORD dwNewPlayerCount)
	{
		gnMaxPlayers = (int)dwNewPlayerCount;
		if (gnMaxPlayers > MAX_CLIENTS)
			gnMaxPlayers = MAX_CLIENTS;
	}

	bool ConnectToServer(char *szServerAddress, DWORD dwPort)
	{
		if (!bInitialized)
			return false;

		struct addrinfo hints, *result = nullptr;
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;

		char portStr[16];
		snprintf(portStr, sizeof(portStr), "%lu", (unsigned long)dwPort);

		int status = getaddrinfo(szServerAddress, portStr, &hints, &result);
		if (status != 0 || !result)
		{
			Log::Print(PRIORITY_MESSAGE, "Network: getaddrinfo failed for %s:%lu", szServerAddress, dwPort);
			return false;
		}

		gClientSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
		if (gClientSocket == NET_INVALID)
		{
			freeaddrinfo(result);
			return false;
		}

		if (connect(gClientSocket, result->ai_addr, (int)result->ai_addrlen) != 0)
		{
			Log::Print(PRIORITY_MESSAGE, "Network: Connect failed to %s:%lu", szServerAddress, dwPort);
			NET_CLOSE(gClientSocket);
			gClientSocket = NET_INVALID;
			freeaddrinfo(result);
			return false;
		}

		freeaddrinfo(result);
		Log::Print(PRIORITY_MESSAGE, "Network: Connected to %s:%lu", szServerAddress, dwPort);
		return true;
	}

	void DisconnectFromServer()
	{
		if (gClientSocket != NET_INVALID)
		{
			NET_CLOSE(gClientSocket);
			gClientSocket = NET_INVALID;
		}
	}

	void StartListen(DWORD dwPort)
	{
		if (!bInitialized)
			return;

		gServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (gServerSocket == NET_INVALID)
		{
			Log::Print(PRIORITY_MESSAGE, "Network: Failed to create server socket");
			return;
		}

		// Allow address reuse
		int opt = 1;
		setsockopt(gServerSocket, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt));

		struct sockaddr_in addr;
		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = INADDR_ANY;
		addr.sin_port = htons((unsigned short)dwPort);

		if (bind(gServerSocket, (struct sockaddr *)&addr, sizeof(addr)) != 0)
		{
			Log::Print(PRIORITY_MESSAGE, "Network: Bind failed on port %lu", dwPort);
			NET_CLOSE(gServerSocket);
			gServerSocket = NET_INVALID;
			return;
		}

		if (listen(gServerSocket, gnMaxPlayers) != 0)
		{
			Log::Print(PRIORITY_MESSAGE, "Network: Listen failed");
			NET_CLOSE(gServerSocket);
			gServerSocket = NET_INVALID;
			return;
		}

		// Set non-blocking
#ifdef _WIN32
		u_long mode = 1;
		ioctlsocket(gServerSocket, FIONBIO, &mode);
#else
		fcntl(gServerSocket, F_SETFL, O_NONBLOCK);
#endif

		Log::Print(PRIORITY_MESSAGE, "Network: Listening on port %lu", dwPort);
	}

	void StopListening()
	{
		// Close all client connections
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if (gClientConnections[i] != NET_INVALID)
			{
				NET_CLOSE(gClientConnections[i]);
				gClientConnections[i] = NET_INVALID;
			}
		}
		gnConnectedClients = 0;

		if (gServerSocket != NET_INVALID)
		{
			NET_CLOSE(gServerSocket);
			gServerSocket = NET_INVALID;
		}
	}

	void SendServerPacket(int nClientMask, D2Packet *pPacket)
	{
		if (!bInitialized || !pPacket)
			return;

		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if ((nClientMask & (1 << i)) && gClientConnections[i] != NET_INVALID)
			{
				send(gClientConnections[i], (const char *)pPacket, sizeof(D2Packet), 0);
			}
		}
	}

	void SendClientPacket(D2Packet *pPacket)
	{
		if (!bInitialized || !pPacket || gClientSocket == NET_INVALID)
			return;

		send(gClientSocket, (const char *)pPacket, sizeof(D2Packet), 0);
	}

	DWORD ReadServerPackets(DWORD dwTimeout)
	{
		if (!bInitialized || gServerSocket == NET_INVALID)
			return dwTimeout;

		fd_set readSet;
		struct timeval tv;
		tv.tv_sec = 0;
		tv.tv_usec = dwTimeout * 1000;

		FD_ZERO(&readSet);
		FD_SET(gServerSocket, &readSet);

		int maxFd = (int)gServerSocket;
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if (gClientConnections[i] != NET_INVALID)
			{
				FD_SET(gClientConnections[i], &readSet);
				if ((int)gClientConnections[i] > maxFd)
					maxFd = (int)gClientConnections[i];
			}
		}

		int ready = select(maxFd + 1, &readSet, nullptr, nullptr, &tv);
		if (ready <= 0)
			return 0;

		// Check for new connections
		if (FD_ISSET(gServerSocket, &readSet))
		{
			NetSocket newClient = accept(gServerSocket, nullptr, nullptr);
			if (newClient != NET_INVALID && gnConnectedClients < gnMaxPlayers)
			{
				for (int i = 0; i < MAX_CLIENTS; i++)
				{
					if (gClientConnections[i] == NET_INVALID)
					{
						gClientConnections[i] = newClient;
						gnConnectedClients++;

#ifdef _WIN32
						u_long mode = 1;
						ioctlsocket(newClient, FIONBIO, &mode);
#else
						fcntl(newClient, F_SETFL, O_NONBLOCK);
#endif
						break;
					}
				}
			}
			else if (newClient != NET_INVALID)
			{
				NET_CLOSE(newClient);
			}
		}

		// Read from connected clients
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if (gClientConnections[i] != NET_INVALID && FD_ISSET(gClientConnections[i], &readSet))
			{
				D2Packet packet;
				int bytesRead = recv(gClientConnections[i], (char *)&packet, sizeof(D2Packet), 0);
				if (bytesRead <= 0)
				{
					NET_CLOSE(gClientConnections[i]);
					gClientConnections[i] = NET_INVALID;
					gnConnectedClients--;
				}
				else
				{
					ServerProcessPacket(&packet);
				}
			}
		}

		return 0;
	}

	DWORD ReadClientPackets(DWORD dwTimeout)
	{
		if (!bInitialized || gClientSocket == NET_INVALID)
			return dwTimeout;

		fd_set readSet;
		struct timeval tv;
		tv.tv_sec = 0;
		tv.tv_usec = dwTimeout * 1000;

		FD_ZERO(&readSet);
		FD_SET(gClientSocket, &readSet);

		int ready = select((int)gClientSocket + 1, &readSet, nullptr, nullptr, &tv);
		if (ready <= 0)
			return 0;

		if (FD_ISSET(gClientSocket, &readSet))
		{
			D2Packet packet;
			int bytesRead = recv(gClientSocket, (char *)&packet, sizeof(D2Packet), 0);
			if (bytesRead <= 0)
			{
				NET_CLOSE(gClientSocket);
				gClientSocket = NET_INVALID;
			}
			else
			{
				ClientProcessPacket(&packet);
			}
		}

		return 0;
	}

	void Shutdown()
	{
		if (!bInitialized)
			return;

		DisconnectFromServer();
		StopListening();

#ifdef _WIN32
		WSACleanup();
#endif
		bInitialized = false;
		Log::Print(PRIORITY_MESSAGE, "Network: Shutdown");
	}
}
