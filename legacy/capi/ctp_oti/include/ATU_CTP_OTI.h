#ifndef _ATU_CTP_OTI_H_
#define _ATU_CTP_OTI_H_
//#include "ParsedMarketData.h"
//#include "ATU_TCP_OTI_string_handler.h"
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <map>
#include "ATU_ErrorMsgStruct.h"
#include "ATU_Abstract_OTI.h"
#include "ThostFtdcUserApiDataType.h"
#include "ThostFtdcTraderApi.h"
#include <dlfcn.h>
#include "ATU_Abstract_OTI.h"
#include <boost/thread.hpp>
#include <stdio.h>

using namespace std;
using namespace atu;
class CThostFtdcTraderApi;
class CTraderSpi;

namespace atu
{
class ATU_CTP_OTI : public ATU_Abstract_OTI, public CThostFtdcTraderSpi
{
public:
	ATU_CTP_OTI();
	virtual void init();
	virtual ~ATU_CTP_OTI();
//would then call all the notify_orderfeed for all the order in the list
	virtual bool on_notify_orderfeed(ATU_OTI_orderfeed_struct &s);
//would then call all the notify_tradefeed for all the order in the list
	virtual bool on_notify_tradefeed(ATU_OTI_tradefeed_struct &s);
//process incoming signal feed and dispatch for all the order in the list if necessary
	virtual bool on_process_signalfeed(ATU_OTI_signalfeed_struct &s);
//notify orderfeed to call back function
/*
	virtual bool notify_orderfeed(ATU_OTI_orderfeed_struct &s);
//notify tradefeed to call back function
	virtual bool notify_tradefeed(ATU_OTI_tradefeed_struct &s);
//notify error msg
	virtual bool notify_errorfeed(ATU_ErrorMsgStruct *ems);
*/
//processing of incoming errorfeed
	virtual bool on_notify_errorfeed(ATU_ErrorMsgStruct *ems);

	virtual bool process_portfolio_get_working_orders(ATU_OTI_portfolio_get_working_orders_struct &s);
	virtual bool on_process_portfolio_get_working_orders(ATU_OTI_portfolio_get_working_orders_struct &s);
	virtual bool process_portfolio_get_trade_history(ATU_OTI_portfolio_get_trade_history_struct &s);
	virtual bool on_process_portfolio_get_trade_history(ATU_OTI_portfolio_get_trade_history_struct &s);
	virtual void detach();



	virtual void OnFrontConnected();
	///请求查询报单响应
	virtual void OnRspQryOrder(CThostFtdcOrderField *pOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	///请求查询成交响应
	virtual void OnRspQryTrade(CThostFtdcTradeField *pTrade, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm,CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRspQryInvestorPositionDetail(CThostFtdcInvestorPositionDetailField *pInvestorPosition, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnFrontDisconnected(int nReason);
	virtual void OnHeartBeatWarning(int nTimeLapse);
	virtual void OnRtnOrder(CThostFtdcOrderField *pOrder);
	virtual void OnRtnTrade(CThostFtdcTradeField *pTrade);
//	virtual int trader_api_join();
	virtual void trader_api_release();
	void ReqUserLogin();
	virtual void setConnectString(string connectstr);


private:
	void *m_ctp_lib_handle;
	boost::thread *m_init_and_run_thread;
	CThostFtdcTraderApi *m_pUserApi;
//	CTraderSpi *m_pUserSpi;
	char m_server_address[300];
	TThostFtdcBrokerIDType m_broker_id;
	TThostFtdcInvestorIDType m_investor_id;
	TThostFtdcPasswordType m_password;
	TThostFtdcFrontIDType m_front_id;
	TThostFtdcSessionIDType m_session_id;
	int m_iRequestID;

	bool IsErrorRspInfo(CThostFtdcRspInfoField *pRspInfo);
	bool IsMyOrder(CThostFtdcOrderField *pOrder);
	bool IsTradingOrder(CThostFtdcOrderField *pOrder);
	string createOrderRef(string orderid);
	string getOrderRef(string orderid);
	string getOrderID(string orderref);
	map<string,string> m_order_idToOrderRef;
	map<string,string> m_OrderRefToorder_id;
	int m_orderRefCount;
	string m_lastOrderTimestamp;

	virtual void addLog(string logSource,int logSeverity,char *argsfmt,char *argtype,...);

	//lock
	boost::recursive_mutex m_requestIDMutex;
	boost::recursive_mutex m_orderRefMutex;
	//--------------------------------------------------
	// Sunny added - [start]
	//--------------------------------------------------
	map<string,string> m_orderid_feedcode;
	//--------------------------------------------------
	// Sunny added - [end]
	//--------------------------------------------------
};
}
#endif //_ATU_CTP_OTI_H_
