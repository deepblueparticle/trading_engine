//**************************************************
//  Author:      Sunny Yan
//  Created On:  Fri Apr 11 12:04:04 HKT 2014
//  Description: There is 1 data processor for each multicast channel
//               Note that 1 channel consists of 4 multicast streams RT_A RT_B RF_A RF_B,
//               so there are 4 McastReceivers for each of them. But there is only 1 data processor.
//
//
//**************************************************
#ifndef PREPROCESSOR_H_
#define PREPROCESSOR_H_

#include "../Logger/Logger.h"
#include "../SharedObjects.h"
#include "../SystemConfig.h"
#include "../ChannelAggregation/McastIdentifier.h"
#include "../OMD_Global.h"
#include "../Util.h"
#include "../OMD_Message_Headers.h"
#include "../Container/OrderBook.h"
#include "DataProcFunctions.h"
#include <boost/shared_ptr.hpp>
#include <cstdio>
#include <cstring>
#include <set>
#include "../ThreadHealthMonitor.h"

using namespace std;
using namespace boost;

class PreProcessor {
  public:
    PreProcessor(const  McastIdentifier  &);
    virtual             ~PreProcessor();
    void                Run();
  private:
    bool                DealingWithSeqNoGaps(uint32_t);
    shared_ptr<DataProcFunctions>      m_DataProcFunc;

    //Output related
    FILE *                             m_CannedMcastFile;
    bool                               m_bRecordMcast;
    bool                               m_bOutputJson;

    //--------------------------------------------------
    // Special Debug options, to be turned off in production
    //--------------------------------------------------
    bool                               m_PrintPreProcSeqNoAsInfo;

    //Shared objects
    ExpandableCirBuffer *              m_RawPktCirBuf;
    ExpandableCirBuffer4Msg *          m_MsgCirBuf;
    shared_ptr<SharedObjects>          m_ShrObj;
    shared_ptr<SystemConfig>           m_SysCfg;
    shared_ptr<Logger>                 m_Logger;
    shared_ptr<ThreadHealthMonitor>    m_ThreadHealthMon;

    //Others
    const McastIdentifier &            m_McastIdentifier;
    unsigned short                     m_ChannelID;
    uint32_t                           m_LocalLastBaseSeqNo;
    // uint32_t                           m_LastUnadjSeqNo;
    // unsigned short                     m_PreProcessorSleepMillisec;
    char                               m_JsonBuffer[JSON_BUFFER_SIZE];
    char                               m_NameBuffer[256];
    boost::posix_time::ptime           m_ProgramStartTime;
    // uint64_t                           m_PrevPktHdrTime;
    unsigned long                      m_MaxOneTimeAlloc;
    unsigned long                      m_TrashSeqNoGapLargerThan;
    uint32_t                           m_LocalLastAdjSeqNo;
};

#endif /* PREPROCESSOR_H_ */