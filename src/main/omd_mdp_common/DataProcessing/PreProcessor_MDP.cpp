#include "PreProcessor.h"

void PreProcessor_MDP::Run()
{
  for (;;)
  {
    //--------------------------------------------------
    // System shutdown
    //--------------------------------------------------
    if (m_ShrObj->ThreadShouldExit())
    {
      m_Logger->Write(Logger::NOTICE,"PreProcessor: ChannelID:%u. Stopping thread.", m_ChannelID);
      return;
    }

    //--------------------------------------------------
    // Report Health
    //--------------------------------------------------
    m_ThreadHealthMon->ReportThatIAmHealthy(ThreadHealthMonitor::PREPROCESSOR, m_ChannelID);

    //--------------------------------------------------
    unsigned long ulTStamp = 0;
    BYTE * pbPkt = NULL;
    if (!m_RawPktCirBuf->GetReadingPtrTStamp(pbPkt,&ulTStamp))
    {
      m_RawPktCirBuf->WaitForData();
      continue;
    }

    //--------------------------------------------------
    // Output Packet Header info
    //--------------------------------------------------
    MDP_Packet_Header * mph         = (MDP_Packet_Header*)(pbPkt);
    size_t sizeof_MDP_Packet_Header = sizeof(MDP_Packet_Header);

    m_Logger->Write(m_PrintPreProcSeqNoAsInfo ? Logger::INFO : Logger::DEBUG, "PreProcessor: ChannelID:%u. %s (%c): Packet Header: PktSeqNum:  %u", m_ChannelID, (m_McastIdentifier.McastType() == McastIdentifier::REALTIME ? "RT" : "RF"), (m_McastIdentifier.Channel() == McastIdentifier::A ? 'A':'B'), mph->PktSeqNum);
    m_Logger->Write(Logger::DEBUG,"PreProcessor: ChannelID:%u. %s (%c): Packet Header: Send Time:  %s", m_ChannelID, (m_McastIdentifier.McastType() == McastIdentifier::REALTIME ? "RT" : "RF"), (m_McastIdentifier.Channel() == McastIdentifier::A ? 'A':'B'), SDateTime::fromUnixTimeToString(mph->SendingTime, SDateTime::NANOSEC).c_str());

    //--------------------------------------------------
    // Output other Debug info
    //--------------------------------------------------
    m_Logger->Write(Logger::DEBUG,"PreProcessor: %s : %u : m_RawPktCirBuf.Size()          %u", m_McastIdentifier.IP().c_str(), m_McastIdentifier.Port(), m_RawPktCirBuf->Size());
    m_Logger->Write(Logger::DEBUG,"PreProcessor: %s : %u : m_RawPktCirBuf.AllocatedSize() %u", m_McastIdentifier.IP().c_str(), m_McastIdentifier.Port(), m_RawPktCirBuf->AllocatedSize());
    m_Logger->Write(Logger::DEBUG,"PreProcessor: %s : %u : m_RawPktCirBuf.GetPktSize()    %u", m_McastIdentifier.IP().c_str(), m_McastIdentifier.Port(), m_RawPktCirBuf->GetPktSize());

    //--------------------------------------------------
    // Record canned data, with heartbeat
    //--------------------------------------------------
    m_BinaryRecorder.WriteHKExUnixTime(pbPkt);

    //--------------------------------------------------
    // Because CME keeps the sequence number of packets rather than messages,
    // So we defer our message parsing to RealTimeProcessor
    //--------------------------------------------------
    m_MsgCirBuf->PushMsg(pbPkt,mph->PktSeqNum,ulTStamp);
    m_RawPktCirBuf->PopFront();
    m_MsgCirBuf->NotifyConsumer();
  }
}
