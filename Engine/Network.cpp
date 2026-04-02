#include "../Shared/D2Packets.hpp"
#include "Diablo2.hpp"
#include "Logging.hpp"

// Networking stubs (Phase 6 will implement platform sockets)
namespace Network
{
	void SendServerPacket(int nClientMask, D2Packet* pPacket) { (void)nClientMask; (void)pPacket; }
	void SendClientPacket(D2Packet* pPacket) { (void)pPacket; }
	void SetMaxPlayerCount(DWORD dwNewPlayerCount) { (void)dwNewPlayerCount; }
	DWORD ReadClientPackets(DWORD dwTimeout) { return dwTimeout; }
	DWORD ReadServerPackets(DWORD dwTimeout) { return dwTimeout; }
	bool ConnectToServer(char* szServerAddress, DWORD dwPort) { (void)szServerAddress; (void)dwPort; return false; }
	void DisconnectFromServer() {}
	void StartListen(DWORD dwPort) { (void)dwPort; }
	void StopListening() {}
	void Init() {}
	void Shutdown() {}
}
