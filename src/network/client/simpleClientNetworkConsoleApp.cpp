#include "simpleClientNetworkConsoleApp.h"

void SimpleClientNetworkConsoleLogic::onClientNetworkStatred(ClientNetwork * network)
{
}

void SimpleClientNetworkConsoleLogic::onClientNetworkStopped(ClientNetwork * network)
{
}

void SimpleClientNetworkConsoleLogic::onClientNetworkConnectStateChanged(ClientNetwork * network, EConnectState state)
{
}

void SimpleClientNetworkConsoleLogic::onClientNetworkSendPackEnd(ClientNetwork * network, EErrType err_type, int err_code, uint64_t send_pack_id, RecvPack * recv_pack)
{
}

void SimpleClientNetworkConsoleLogic::onClientNetworkRecvPack(ClientNetwork * network, RecvPack * recv_pack)
{
}
