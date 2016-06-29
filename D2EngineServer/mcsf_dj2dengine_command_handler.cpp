#include "mcsf_dj2dengine_command_handler.h"
#include "CommunicationMessage.pb.h"
#include <sstream>


MCSF_DJ2DENGINE_BEGIN_NAMESPACE

Mcsf2DEngineCommandHandler::Mcsf2DEngineCommandHandler(int threadsNum)
{
	m_pCommandDispatcher = CommandsDispatcher::GetInstance();
}


Mcsf2DEngineCommandHandler::~Mcsf2DEngineCommandHandler(void)
{
	if (m_pCommandDispatcher)
	{
		m_pCommandDispatcher->Release();
	}
}



int Mcsf2DEngineCommandHandler::HandleCommand(const Mcsf::CommandContext* pContext, std::string* pReplyObject)
{
	*pReplyObject = "0";
	
	Mcsf::CommandContext *cmd = new Mcsf::CommandContext();
	
	cmd->iCommandId = pContext->iCommandId;
	cmd->sReceiver = pContext->sReceiver;
	cmd->sSender = pContext->sSender;
	cmd->sSerializeObject = pContext->sSerializeObject;

	ACE_Message_Block *mb = new ACE_Message_Block(sizeof(cmd));
	memcpy(mb->wr_ptr(), &cmd, sizeof(cmd));
	
	m_pCommandDispatcher->ReceiveMessage(mb);
	return 0;
}

Mcsf2DEngineDataHandler::Mcsf2DEngineDataHandler() 
{

}

Mcsf2DEngineDataHandler::~Mcsf2DEngineDataHandler()
{

}

int Mcsf2DEngineDataHandler::HandleDataTrans( void* pRawData, size_t iLen )
{
	
	return 0;
}

MCSF_DJ2DENGINE_END_NAMESPACE
