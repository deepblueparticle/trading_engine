///WARNING INCOMPLETED IMPLEMENTATION
#include "ATU_CTP_OTI.h"
#include "Toolbox.h"
#include <boost/lexical_cast.hpp>

using namespace std;
TThostFtdcInstrumentIDType INSTRUMENT_ID = "IF1405";
//TThostFtdcDirectionType DIRECTION = THOST_FTDC_D_Sell;
TThostFtdcDirectionType DIRECTION = THOST_FTDC_D_Buy;
TThostFtdcPriceType LIMIT_PRICE = 2000;

namespace atu
{
void printCThostFtdcOrderField(CThostFtdcOrderField *pOrder){
	cout<<"BrokerID("<<pOrder->BrokerID<<"); ";
	cout<<"InvestorID("<<pOrder->InvestorID<<"); ";
	cout<<"InstrumentID("<<pOrder->InstrumentID<<"); ";
	cout<<"OrderRef("<<pOrder->OrderRef<<"); ";
	cout<<"UserID("<<pOrder->UserID<<"); ";
	cout<<"OrderPriceType("<<pOrder->OrderPriceType<<"); ";
	cout<<"Direction("<<pOrder->Direction<<"); ";
	cout<<"CombOffsetFlag("<<pOrder->CombOffsetFlag<<"); ";
	cout<<"CombHedgeFlag("<<pOrder->CombHedgeFlag<<"); ";
	cout<<"LimitPrice("<<pOrder->LimitPrice<<"); ";
	cout<<"VolumeTotalOriginal("<<pOrder->VolumeTotalOriginal<<"); ";
	cout<<"TimeCondition("<<pOrder->TimeCondition<<"); ";
	cout<<"GTDDate("<<pOrder->GTDDate<<"); ";
	cout<<"VolumeCondition("<<pOrder->VolumeCondition<<"); ";
	cout<<"MinVolume("<<pOrder->MinVolume<<"); ";
	cout<<"ContingentCondition("<<pOrder->ContingentCondition<<"); ";
	cout<<"StopPrice("<<pOrder->StopPrice<<"); ";
	cout<<"ForceCloseReason("<<pOrder->ForceCloseReason<<"); ";
	cout<<"IsAutoSuspend("<<pOrder->IsAutoSuspend<<"); ";
	cout<<"BusinessUnit("<<pOrder->BusinessUnit<<"); ";
	cout<<"RequestID("<<pOrder->RequestID<<"); ";
	cout<<"OrderLocalID("<<pOrder->OrderLocalID<<"); ";
	cout<<"ExchangeID("<<pOrder->ExchangeID<<"); ";
	cout<<"ParticipantID("<<pOrder->ParticipantID<<"); ";
	cout<<"ClientID("<<pOrder->ClientID<<"); ";
	cout<<"ExchangeInstID("<<pOrder->ExchangeInstID<<"); ";
	cout<<"TraderID("<<pOrder->TraderID<<"); ";
	cout<<"InstallID("<<pOrder->InstallID<<"); ";
	cout<<"OrderSubmitStatus("<<pOrder->OrderSubmitStatus<<"); ";
	cout<<"NotifySequence("<<pOrder->NotifySequence<<"); ";
	cout<<"TradingDay("<<pOrder->TradingDay<<"); ";
	cout<<"SettlementID("<<pOrder->SettlementID<<"); ";
	cout<<"OrderSysID("<<pOrder->OrderSysID<<"); ";
	cout<<"OrderSource("<<pOrder->OrderSource<<"); ";
	cout<<"OrderStatus("<<pOrder->OrderStatus<<"); ";
	cout<<"OrderType("<<pOrder->OrderType<<"); ";
	cout<<"VolumeTraded("<<pOrder->VolumeTraded<<"); ";
	cout<<"VolumeTotal("<<pOrder->VolumeTotal<<"); ";
	cout<<"InsertDate("<<pOrder->InsertDate<<"); ";
	cout<<"InsertTime("<<pOrder->InsertTime<<"); ";
	cout<<"ActiveTime("<<pOrder->ActiveTime<<"); ";
	cout<<"SuspendTime("<<pOrder->SuspendTime<<"); ";
	cout<<"UpdateTime("<<pOrder->UpdateTime<<"); ";
	cout<<"CancelTime("<<pOrder->CancelTime<<"); ";
	cout<<"ActiveTraderID("<<pOrder->ActiveTraderID<<"); ";
	cout<<"ClearingPartID("<<pOrder->ClearingPartID<<"); ";
	cout<<"SequenceNo("<<pOrder->SequenceNo<<"); ";
	cout<<"FrontID("<<pOrder->FrontID<<"); ";
	cout<<"SessionID("<<pOrder->SessionID<<"); ";
	cout<<"UserProductInfo("<<pOrder->UserProductInfo<<"); ";
	cout<<"StatusMsg("<<pOrder->StatusMsg<<"); ";
	cout<<"UserForceClose("<<pOrder->UserForceClose<<"); ";
	cout<<"ActiveUserID("<<pOrder->ActiveUserID<<"); ";
	cout<<"BrokerOrderSeq("<<pOrder->BrokerOrderSeq<<"); ";
	cout<<"RelativeOrderSysID("<<pOrder->RelativeOrderSysID<<"); ";
	cout<<"ZCETotalTradedVolume("<<pOrder->ZCETotalTradedVolume<<"); ";
	cout<<"IsSwapOrder("<<pOrder->IsSwapOrder<<"); ";
	cout<<endl;
	cout<<endl;
}


ATU_CTP_OTI::ATU_CTP_OTI() :ATU_Abstract_OTI()
{
  //--------------------------------------------------
  // Sunny added - [start]
  //--------------------------------------------------
  cout << "CTP OTI: SVN revision no. (CTP OTI): "          << string(__SVN_REV__)       << endl;
  cout << "CTP OTI: SVN revision no. (Entire workspace): " << string(__SVN_WKSPC_REV__) << endl;
  //--------------------------------------------------
  // Sunny added - [end]
  //--------------------------------------------------

//	ATU_Abstract_OTI::ATU_Abstract_OTI();
	m_orderRefCount=0;
	m_lastOrderTimestamp="";
	m_iRequestID = 0;
}
void ATU_CTP_OTI::detach() {
	m_init_and_run_thread=new boost::thread(boost::bind(&ATU_CTP_OTI::init,this));
}
void ATU_CTP_OTI::setConnectString(string connectstr) {
	vector<string> fields;
	addLog(__LOGSOURCE__,ATU_logfeed_struct::DEBUG,"Connect string is %s","s",connectstr.c_str());
	Toolbox::split(fields,connectstr,"@");
	strcpy(m_server_address,fields[0].c_str());
	strcpy(m_broker_id,fields[1].c_str());
	strcpy(m_investor_id,fields[2].c_str());
	strcpy(m_password,fields[3].c_str());
	addLog(__LOGSOURCE__,ATU_logfeed_struct::INFO,"Connect info: %s %s %s %s %s","sssss",m_server_address,m_broker_id,m_investor_id,m_investor_id,m_password);

}
void ATU_CTP_OTI::init() {
//TODO:
	//should load it from config settings
	m_ctp_lib_handle=dlopen("libthosttraderapi.so",RTLD_NOW);
	if (m_ctp_lib_handle!=NULL) {
		addLog(__LOGSOURCE__,ATU_logfeed_struct::INFO,"ctp tradeapi loaded!","");
	} else {
		addLog(__LOGSOURCE__,ATU_logfeed_struct::EMERGENCY,"ctp tradeapi NOT loaded!","");
	}
	typedef CThostFtdcTraderApi* (*CreateFtdcTradeApiPtr)();
	CreateFtdcTradeApiPtr CreateFtdcTradeApi= (CreateFtdcTradeApiPtr)dlsym(m_ctp_lib_handle,"_ZN19CThostFtdcTraderApi19CreateFtdcTraderApiEPKc");

	m_pUserApi=CreateFtdcTradeApi();
//TODO:
	//should load it from config settings
	//#TradingURLAddr=tcp://180.166.165.179:41205
	//#MdURLAddr=tcp://180.166.165.179:41213
	//#BrokerID=1013
	//#UserID=00000062
	//#UserPwd=834591
	//TradingURLAddr=tcp://222.66.235.70:21205
	//MdURLAddr=tcp://222.66.235.70:21213
	//BrokerID=66666
	//UserID=8001034655
	//UserPwd=123456
//	strcpy(m_server_address,"tcp://180.166.165.179:41205");
//	strcpy(m_broker_id,"1013");
//	strcpy(m_investor_id,"00000062");
//	strcpy(m_password,"834591");
	m_pUserApi->RegisterSpi((CThostFtdcTraderSpi*) this);
	m_pUserApi->SubscribePublicTopic(THOST_TERT_QUICK);
	m_pUserApi->SubscribePrivateTopic(THOST_TERT_QUICK);
	m_pUserApi->RegisterFront(m_server_address);
	m_pUserApi->Init();
	if (m_pUserApi!=NULL) {
		m_pUserApi->Join();
	}

}
ATU_CTP_OTI::~ATU_CTP_OTI() {
	trader_api_release();
	if (m_ctp_lib_handle!=NULL) {
		dlclose(m_ctp_lib_handle);
	}
}
bool ATU_CTP_OTI::on_notify_tradefeed(ATU_OTI_tradefeed_struct &s) {
	return true;
}
bool ATU_CTP_OTI::on_notify_errorfeed(ATU_ErrorMsgStruct *ems) {
	return true;
}
bool ATU_CTP_OTI::on_notify_orderfeed(ATU_OTI_orderfeed_struct &s) {
	return true;
}
//process incoming signal feed and dispatch for all the order in the list if necessary
bool ATU_CTP_OTI::on_process_signalfeed(ATU_OTI_signalfeed_struct &s) {
//here we determine what order to create or delete
	if (s.m_order_action.compare("insert")==0) {
		//--------------------------------------------------
		// Sunny added - [start]
		//--------------------------------------------------
		if (s.m_order_id != "" &&
			s.m_feedcode != "")
		{
			map<string,string>::iterator it = m_orderid_feedcode.find(s.m_order_id);

			if (it == m_orderid_feedcode.end())
			{
				m_orderid_feedcode[s.m_order_id] = s.m_feedcode;
			}
		}
		//--------------------------------------------------
		// Sunny added - [end]
		//--------------------------------------------------
		CThostFtdcInputOrderField req;
		memset(&req, 0, sizeof(req));
		strcpy(req.BrokerID, m_broker_id);
		strcpy(req.InvestorID, m_investor_id);
		strcpy(req.InstrumentID, s.m_feedcode.c_str());
		string orderref=createOrderRef(s.m_order_id);
		strcpy(req.OrderRef, orderref.c_str());

		req.ContingentCondition = THOST_FTDC_CC_Immediately;
		//Limit order or Market order or stop order
		if(s.m_order_type.compare("limit_order")==0){
			req.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
			req.LimitPrice = s.m_price;
			//TODO: limit order with expiration time
		}
		else if(s.m_order_type.compare("market_order")==0){
			req.OrderPriceType = THOST_FTDC_OPT_AnyPrice;
			req.LimitPrice = 0;
		}
		else if(s.m_order_type.compare("stop_order")==0){
			char _attr[100],_temp[20];
			strcpy(_attr,s.m_order_attributes.c_str());
			double _stop_price; int _stop_condition; char _orderType[15];
			for(int i = 0 ; i < strlen(_attr) ; i++) if (_attr[i] == ';' || _attr[i] == '=') _attr[i] = ' ';
			sscanf(_attr,"%s%lf%s%d%s%s",_temp,&_stop_price,_temp,&_stop_condition,_temp,_orderType);
			if(strcmp(_orderType,"market_order") == 0){
				req.OrderPriceType = THOST_FTDC_OPT_AnyPrice;
				req.LimitPrice = 0;
			}
			else {
				req.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
				req.LimitPrice = s.m_price;
			}
			req.StopPrice = _stop_price;
			addLog(__LOGSOURCE__,ATU_logfeed_struct::DEBUG,"Stop condition: %f %d %s","fds",_stop_price,_stop_condition,_orderType);
			switch(_stop_condition){
			case 1:	{req.ContingentCondition = THOST_FTDC_CC_BidPriceGreaterEqualStopPrice; break;}
			case 2:	{req.ContingentCondition = THOST_FTDC_CC_BidPriceLesserEqualStopPrice; break;}
			case 3:	{req.ContingentCondition = THOST_FTDC_CC_AskPriceGreaterEqualStopPrice; break;}
			case 4:	{req.ContingentCondition = THOST_FTDC_CC_AskPriceLesserEqualStopPrice; break;}
			case 5:	{req.ContingentCondition = THOST_FTDC_CC_LastPriceGreaterEqualStopPrice; break;}
			case 6:	{req.ContingentCondition = THOST_FTDC_CC_LastPriceLesserEqualStopPrice; break;}
			}
		}

		//Buy or Sell
		if (s.m_buy_or_sell==1) {
			req.Direction = THOST_FTDC_D_Buy;
		} else {
			req.Direction = THOST_FTDC_D_Sell;
		}
		req.UserForceClose = 0;

		//Open or Close
		if (s.m_open_or_close.compare("open") == 0){
			req.CombOffsetFlag[0] = THOST_FTDC_OF_Open;
		}
		else if(s.m_open_or_close.compare("close") == 0){
//			req.CombOffsetFlag[0] = THOST_FTDC_OF_Close;
			req.CombOffsetFlag[0] = THOST_FTDC_OF_CloseToday;
			//some exchange use CloseToday some use Close
//never use Force Close or it cant be cancel
//			req.UserForceClose = 1;

		}

		req.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;

		req.VolumeTotalOriginal = s.m_qty;

		//Order validity
		if(s.m_order_validity.compare("today") == 0){
			req.TimeCondition = THOST_FTDC_TC_GFD;//today
			req.VolumeCondition = THOST_FTDC_VC_AV;//any volume
		}
		else if(s.m_order_validity.compare("fill_and_kill") == 0){
			req.TimeCondition = THOST_FTDC_TC_IOC;//immediately
			req.VolumeCondition = THOST_FTDC_VC_AV;//any volume
		}
		else if(s.m_order_validity.compare("fill_or_kill") == 0){
			req.TimeCondition = THOST_FTDC_TC_IOC;//immediately
			req.VolumeCondition = THOST_FTDC_VC_CV;//all volume
		}

		//MinVoume is valid when req.VolumeCondition = THOST_FTDC_VC_MV //minimum volume
		req.MinVolume = 1;

		req.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
		req.IsAutoSuspend = 0;
		boost::unique_lock<boost::recursive_mutex> lock(m_requestIDMutex);
		int iResult = m_pUserApi->ReqOrderInsert(&req, ++m_iRequestID);
		addLog(__LOGSOURCE__,ATU_logfeed_struct::INFO,"Order Insert to CTP order id=%s","s",s.m_order_id.c_str());
		addLog(__LOGSOURCE__,ATU_logfeed_struct::INFO,"ReqOrderInsert result: %s","s",((iResult == 0) ? "OK" : "Fail"));
		//this should generate errorfeed or orderfeed as notification of error
	}
  else if (s.m_order_action.compare("delete")==0) {
		CThostFtdcInputOrderActionField req;
		memset(&req, 0, sizeof(req));
		strcpy(req.BrokerID, m_broker_id);
		strcpy(req.InvestorID, m_investor_id);
		strcpy(req.OrderRef, getOrderRef(s.m_order_id).c_str());
		req.FrontID = m_front_id;
		req.SessionID = m_session_id;
		req.ActionFlag = THOST_FTDC_AF_Delete;
		//--------------------------------------------------
		// Sunny added - [start]
		//--------------------------------------------------
		if (s.m_feedcode == "" &&
				m_orderid_feedcode.find(s.m_order_id) != m_orderid_feedcode.end())
			s.m_feedcode = m_orderid_feedcode[s.m_order_id];
		//--------------------------------------------------
		// Sunny added - [end]
		//--------------------------------------------------
		strcpy(req.InstrumentID, s.m_feedcode.c_str());
		addLog(__LOGSOURCE__,ATU_logfeed_struct::INFO,"Order Deleted from CTP order id=%s","s",s.m_order_id.c_str());
		boost::unique_lock<boost::recursive_mutex> lock(m_requestIDMutex);
		int iResult = m_pUserApi->ReqOrderAction(&req, ++m_iRequestID);
		addLog(__LOGSOURCE__,ATU_logfeed_struct::INFO,"ReqOrderAction result: %s","s",((iResult == 0) ? "OK" : "Fail"));
		//this should generate errorfeed or orderfeed as notification of error
	}

	return true;
}

bool ATU_CTP_OTI::on_process_portfolio_get_working_orders(ATU_OTI_portfolio_get_working_orders_struct &s) {
	CThostFtdcQryOrderField req;
	memset(&req, 0, sizeof(req));
	strcpy(req.BrokerID, m_broker_id);
	strcpy(req.InvestorID, m_investor_id);
	boost::unique_lock<boost::recursive_mutex> lock(m_requestIDMutex);
	if (m_pUserApi == NULL) sleep(2); // The purpose of adding sleep is that the CTP initialization may take more time to finish.
	int iResult = m_pUserApi->ReqQryOrder(&req, ++m_iRequestID);
	addLog(__LOGSOURCE__,ATU_logfeed_struct::INFO,"ReqQryOrder result: %s","s",((iResult == 0) ? "OK" : "Fail"));
	return true;
}
bool ATU_CTP_OTI::process_portfolio_get_working_orders(ATU_OTI_portfolio_get_working_orders_struct &s) {
	return true;
}
bool ATU_CTP_OTI::process_portfolio_get_trade_history(ATU_OTI_portfolio_get_trade_history_struct &s)
{
	return true;
}
bool ATU_CTP_OTI::on_process_portfolio_get_trade_history(ATU_OTI_portfolio_get_trade_history_struct &s)
{
	CThostFtdcQryTradeField req;
	memset(&req, 0, sizeof(req));
	strcpy(req.BrokerID, m_broker_id);
	strcpy(req.InvestorID, m_investor_id);
	boost::unique_lock<boost::recursive_mutex> lock(m_requestIDMutex);
	if (m_pUserApi == NULL) sleep(2); // The purpose of adding sleep is that the CTP initialization may take more time to finish.
	int iResult = m_pUserApi->ReqQryTrade(&req, ++m_iRequestID);
	addLog(__LOGSOURCE__,ATU_logfeed_struct::INFO,"ReqQryTrade result: %s","s",((iResult == 0) ? "OK" : "Fail"));
	return true;
}
/*
//process orderfeed from lower layer and handle it accordingly
bool ATU_CTP_OTI::notify_orderfeed(ATU_OTI_orderfeed_struct &s)
{
	if (m_notify_orderfeed_call_back_func!=NULL) {
		return (*m_notify_orderfeed_call_back_func)(s);
	} else {
		return false;
	}
}
//process tradefeed from lower layer and handle it accordingly
bool ATU_CTP_OTI::notify_tradefeed(ATU_OTI_tradefeed_struct &s)
{
	if (m_notify_tradefeed_call_back_func!=NULL) {
		return (*m_notify_tradefeed_call_back_func)(s);
	} else {
		return false;
	}
}
bool ATU_CTP_OTI::notify_errorfeed(ATU_ErrorMsgStruct *ems)
{
	if (m_notify_errorfeed_call_back_func!=NULL) {
		return (*m_notify_errorfeed_call_back_func)(ems);
	} else {
		return false;
	}
}
*/


void ATU_CTP_OTI::trader_api_release() {
	addLog(__LOGSOURCE__,ATU_logfeed_struct::INFO,"Trader api is released","");
	if (m_pUserApi!=NULL) {
		m_pUserApi->RegisterSpi(NULL);
		m_pUserApi->Release();
		m_pUserApi = NULL;
	}
}

void ATU_CTP_OTI::OnFrontConnected() {
	addLog(__LOGSOURCE__,ATU_logfeed_struct::INFO,"Front is connected","");
	ReqUserLogin();
}

void ATU_CTP_OTI::ReqUserLogin() {
	CThostFtdcReqUserLoginField req;
	memset(&req, 0, sizeof(req));
	strcpy(req.BrokerID, m_broker_id);
	strcpy(req.UserID, m_investor_id);
	strcpy(req.Password, m_password);
	boost::unique_lock<boost::recursive_mutex> lock(m_requestIDMutex);
	int iResult = m_pUserApi->ReqUserLogin(&req, ++m_iRequestID);
	addLog(__LOGSOURCE__,ATU_logfeed_struct::INFO,"UserLogin result: %s","s",((iResult == 0) ? "OK" : "Fail"));
}

void ATU_CTP_OTI::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
	addLog(__LOGSOURCE__,ATU_logfeed_struct::INFO,"Call OnRspUserLogin","");
	if (bIsLast && !IsErrorRspInfo(pRspInfo)) {
		m_front_id = pRspUserLogin->FrontID;
		m_session_id = pRspUserLogin->SessionID;
//		int iNextOrderRef = atoi(pRspUserLogin->MaxOrderRef);
//		iNextOrderRef++;
//		sprintf(ORDER_REF, "%d", iNextOrderRef); // overflow if > 13 char
		cerr << "pRspUserLogin->MaxOrderRef = " << pRspUserLogin->MaxOrderRef << endl;
		cerr << "--->>> GetTradingDay() = " << m_pUserApi->GetTradingDay() << endl;
		//ReqSettlementInfoConfirm();
	}
}


void ATU_CTP_OTI::OnRspSettlementInfoConfirm(
		CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm,
		CThostFtdcRspInfoField *pRspInfo,
		int nRequestID,
		bool bIsLast) {
	addLog(__LOGSOURCE__,ATU_logfeed_struct::INFO,"Call OnRspSettlementInfoConfirm","");
	if (bIsLast && !IsErrorRspInfo(pRspInfo)) {
		//ReqQryInstrument();
	}
}


void ATU_CTP_OTI::OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument,
		CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
	addLog(__LOGSOURCE__,ATU_logfeed_struct::INFO,"Call OnRspQryInstrument","");
	if (bIsLast && !IsErrorRspInfo(pRspInfo)) {
//		ReqQryTradingAccount();
	}
}


void ATU_CTP_OTI::OnRspQryTradingAccount(
		CThostFtdcTradingAccountField *pTradingAccount,
		CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
	addLog(__LOGSOURCE__,ATU_logfeed_struct::INFO,"Balance=%f, Available=%f","ff",pTradingAccount->Balance ,pTradingAccount->Available);
	if (bIsLast && !IsErrorRspInfo(pRspInfo)) {
		//ReqQryInvestorPosition();
	}
}


void ATU_CTP_OTI::OnRspQryInvestorPosition(
		CThostFtdcInvestorPositionField *pInvestorPosition,
		CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
	addLog(__LOGSOURCE__,ATU_logfeed_struct::INFO,"Call OnRspQryInvestorPosition","");
	addLog(__LOGSOURCE__,ATU_logfeed_struct::INFO,"%s %s %s today pos=%d pos=%d open vol=%d openAmt=%f","sssdddf",pInvestorPosition->TradingDay,pInvestorPosition->PosiDirection,pInvestorPosition->InstrumentID,pInvestorPosition->TodayPosition, pInvestorPosition->Position, pInvestorPosition->OpenVolume,pInvestorPosition->OpenAmount);
	if (!IsErrorRspInfo(pRspInfo)) {		//ReqOrderInsert();

	}
}
void ATU_CTP_OTI::OnRspQryInvestorPositionDetail(
		CThostFtdcInvestorPositionDetailField *pInvestorPosition,
		CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
	addLog(__LOGSOURCE__,ATU_logfeed_struct::INFO,"Call OnRspQryInvestorPositionDetail","");
	addLog(__LOGSOURCE__,ATU_logfeed_struct::INFO,"%s %s %s %s TradeID=%s Volume=%d OpenPrice=%f","sssssdf",pInvestorPosition->TradingDay,pInvestorPosition->OpenDate,pInvestorPosition->Direction,pInvestorPosition->InstrumentID, pInvestorPosition->TradeID, pInvestorPosition->Volume, pInvestorPosition->OpenPrice);
	if (!IsErrorRspInfo(pRspInfo)) {		//ReqOrderInsert();

	}
}

void ATU_CTP_OTI::OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder,
		CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
	addLog(__LOGSOURCE__,ATU_logfeed_struct::INFO,"Call OnRspOrderInsert","");
	IsErrorRspInfo(pRspInfo);
	ATU_OTI_orderfeed_struct of;
	string timestamp=Toolbox::timeStamp_full();
	of.m_timestamp = timestamp;
	of.m_market = "CFFEX";
	of.m_feedcode = string(pInputOrder->InstrumentID);
	of.m_order_id = getOrderID(string(pInputOrder->OrderRef));
	of.m_price = pInputOrder->LimitPrice;
	of.m_qty = pInputOrder->VolumeTotalOriginal;
	of.m_islast=0;
	//TODO: need to determine if it is open or close
	if(pInputOrder->CombOffsetFlag[0] == THOST_FTDC_OF_Open){
		of.m_open_or_close = "open";
	}
	else if(pInputOrder->CombOffsetFlag[0] == THOST_FTDC_OF_Close){
		of.m_open_or_close = "close";
	}
	else if(pInputOrder->CombOffsetFlag[0] == THOST_FTDC_OF_CloseToday){
		of.m_open_or_close = "close";
	}

	if (pInputOrder->Direction == THOST_FTDC_D_Buy) {
		of.m_buy_or_sell = 1;
	}
	if (pInputOrder->Direction == THOST_FTDC_D_Sell) {
		of.m_buy_or_sell = 2;
	}

	//Wrong order with qty_fileed = 0
	of.m_qty_filled = 0;

	//Wrong order will be deleted immediately
	of.m_deleted = 1;

	//OnRtnOder receive correct order message, OnRspOrderInsert receive wrong order message
	of.m_order_status = 1;
	of.m_error_description = string(pRspInfo->ErrorMsg);

	//real-time update
	of.m_source = 0;

	//TODO:stop order
	if(pInputOrder->OrderPriceType == THOST_FTDC_OPT_AnyPrice) {
		of.m_order_type = "market_order";
	}
	else if(pInputOrder->OrderPriceType == THOST_FTDC_OPT_LimitPrice){
		of.m_order_type = "limit_order";
	}

	of.m_order_validity = "N/A";

	notify_orderfeed(of);
}


void ATU_CTP_OTI::OnRspOrderAction(
		CThostFtdcInputOrderActionField *pInputOrderAction,
		CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {

	//Do not believe order information attached in the response with Error.
	addLog(__LOGSOURCE__,ATU_logfeed_struct::INFO,"Call OnRspOrderAction","");
	IsErrorRspInfo(pRspInfo);
	ATU_OTI_orderfeed_struct of;
		of.m_islast=0;
		of.m_timestamp = Toolbox::timeStamp_full();
		of.m_market = "CFFEX";
		of.m_feedcode = string(pInputOrderAction->InstrumentID);
		of.m_order_id = getOrderID(string(pInputOrderAction->OrderRef));
		of.m_price = pInputOrderAction->LimitPrice;
		//cancel order failed, pInputOrderAction don't provide quantity
		of.m_qty = 0;
		//cancel order failed, pInputOrderAction don't provide open or close information
		of.m_open_or_close = "open";
		//cancel order failed, pInputOrderAction don't provide buy or sell information
		of.m_buy_or_sell = 1;

		//Wrong order with qty_fileed = 0
		of.m_qty_filled = 0;

		//cancel order got error
		of.m_deleted = 1; //changed to mark as deleted

		//OnRtnOder receive correct order message, OnRspOrderAction receive wrong order message
		of.m_order_status = 1;
		of.m_error_description = string(pRspInfo->ErrorMsg);

		//real-time update
		of.m_source = 0;

		//cancel order failed, pInputOrderAction don't provide order type information
		of.m_order_type = "N/A";

		of.m_order_validity = "N/A";
		notify_orderfeed(of);
}

void ATU_CTP_OTI::OnRtnOrder(CThostFtdcOrderField *pOrder) {
	addLog(__LOGSOURCE__,ATU_logfeed_struct::INFO,"Call OnRtnOrder","");
	//printCThostFtdcOrderField(pOrder);
	ATU_OTI_orderfeed_struct of;

	of.m_timestamp = Toolbox::timeStamp_full();
	of.m_market = "CFFEX";
	of.m_feedcode = string(pOrder->InstrumentID);
	of.m_order_id = getOrderID(string(pOrder->OrderRef));
	of.m_price = pOrder->LimitPrice;
	of.m_qty = pOrder->VolumeTotalOriginal;
	//TODO: need to determine if it is open or close
	of.m_open_or_close="NA";
	if(pOrder->CombOffsetFlag[0] == THOST_FTDC_OF_Open){
		of.m_open_or_close = "open";
	}
	else if(pOrder->CombOffsetFlag[0] == THOST_FTDC_OF_Close){
		of.m_open_or_close = "close";
	}
	else if(pOrder->CombOffsetFlag[0] == THOST_FTDC_OF_CloseToday){
		of.m_open_or_close = "close";
	}

	if (pOrder->Direction == THOST_FTDC_D_Buy) {
		of.m_buy_or_sell = 1;
	}
	if (pOrder->Direction == THOST_FTDC_D_Sell) {
		of.m_buy_or_sell = 2;
	}

	of.m_qty_filled = pOrder->VolumeTraded;

	if(pOrder->OrderStatus != THOST_FTDC_OST_Canceled) of.m_deleted = 0;
	else  of.m_deleted = 1;
	if (of.m_qty==of.m_qty_filled) {
		of.m_deleted=1;
	}
	//OnRtnOder receive correct order message, OnRspOrderInsert receive wrong order message
	of.m_order_status = 0;
	of.m_error_description = "";
    if(pOrder->OrderSubmitStatus == THOST_FTDC_OSS_InsertRejected ) {
    	of.m_order_status = 1;
    	of.m_error_description = "Insert Order Rejected";
    }
    if(pOrder->OrderSubmitStatus == THOST_FTDC_OSS_CancelRejected ) {
    	of.m_order_status = 1;
    	of.m_error_description = "Cancel Order Rejected";
    }
    if(pOrder->OrderSubmitStatus == THOST_FTDC_OSS_ModifyRejected ) {
    	of.m_order_status = 1;
    	of.m_error_description = "Modify Order Rejected";
    }


	of.m_islast=0;
	//real-time update
	of.m_source = 0;

	//TODO:stop order
	if(pOrder->OrderPriceType == THOST_FTDC_OPT_AnyPrice) {
		of.m_order_type = "market_order";
	}
	else if(pOrder->OrderPriceType == THOST_FTDC_OPT_LimitPrice){
		of.m_order_type = "limit_order";
	}

	of.m_order_validity = "N/A";

	addLog(__LOGSOURCE__,ATU_logfeed_struct::DEBUG,"StatusMsg:%s","s",pOrder->StatusMsg);

	notify_orderfeed(of);

}

void ATU_CTP_OTI::OnRtnTrade(CThostFtdcTradeField *pTrade) {
	addLog(__LOGSOURCE__,ATU_logfeed_struct::INFO,"Call OnRtnTrade","");
//	cout << pTrade->InstrumentID << "," << pTrade->Direction << "," << pTrade->Price << "," << pTrade->Volume << "," << pTrade->TraderID << "," << pTrade->OrderLocalID << "," << pTrade->OrderRef << endl;

	ATU_OTI_tradefeed_struct tf;

	tf.m_timestamp=Toolbox::timeStamp_full();
	tf.m_market="CFFEX";
	tf.m_feedcode=string(pTrade->InstrumentID);
	tf.m_order_id=getOrderID(string(pTrade->OrderRef));
	tf.m_price=pTrade->Price;
	tf.m_qty=pTrade->Volume;
	tf.m_islast=0;
	if(pTrade->OffsetFlag == THOST_FTDC_OF_Open){
		tf.m_open_or_close="open";
	}
	else if(pTrade->OffsetFlag == THOST_FTDC_OF_Close){
		tf.m_open_or_close="close";
	}
	else if(pTrade->OffsetFlag == THOST_FTDC_OF_CloseToday){
		tf.m_open_or_close="close";
	}

	if(pTrade->Direction == THOST_FTDC_D_Buy){
		tf.m_buy_or_sell=1;
	}
	else if(pTrade->Direction == THOST_FTDC_D_Sell){
		tf.m_buy_or_sell=2;
	}

	if (pTrade->Direction==THOST_FTDC_D_Sell) {
		tf.m_buy_or_sell=2;
	}
	tf.m_trade_id=pTrade->TradeID;
	boost::trim(tf.m_trade_id);
	tf.m_portfolio_name="N/A";

	char _tradeTime_c[9],_tradeTime_l[9];
	strcpy(_tradeTime_c, pTrade->TradeTime);
	_tradeTime_l[0] = _tradeTime_c[0],_tradeTime_l[1] = _tradeTime_c[1],_tradeTime_l[2] = _tradeTime_c[3];
	_tradeTime_l[3] = _tradeTime_c[5],_tradeTime_l[4] = _tradeTime_c[7],_tradeTime_l[5] = _tradeTime_c[8];
	_tradeTime_l[6] = 0;
	string _tradeDate = pTrade->TradeDate, _tradeTime = _tradeTime_l;
	tf.m_trade_timestamp=_tradeDate+"_"+_tradeTime_l;
	tf.m_source=0;
	notify_tradefeed(tf);
}

void ATU_CTP_OTI::OnFrontDisconnected(int nReason) {
	addLog(__LOGSOURCE__,ATU_logfeed_struct::INFO,"Call OnFrontDisconnected: Reason = %d","d",nReason);
}

void ATU_CTP_OTI::OnHeartBeatWarning(int nTimeLapse) {
	addLog(__LOGSOURCE__,ATU_logfeed_struct::INFO,"Call OnHeartBeatWarning. nTimerLapse=%d","d",nTimeLapse);
}

void ATU_CTP_OTI::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
	addLog(__LOGSOURCE__,ATU_logfeed_struct::INFO,"Call OnRspError","");
	IsErrorRspInfo(pRspInfo);
//TODO:: should call notify_errorfeed
//TODO:: or should call orderfeed with error_status marked..
}

bool ATU_CTP_OTI::IsErrorRspInfo(CThostFtdcRspInfoField *pRspInfo) {
	bool bResult = ((pRspInfo) && (pRspInfo->ErrorID != 0));
	if (bResult)
		addLog(__LOGSOURCE__,ATU_logfeed_struct::ERROR,"ErrorID=%d, ErrorMsg=%s","ds",pRspInfo->ErrorID,pRspInfo->ErrorMsg);
	return bResult;
}

///�����ѯ������Ӧ
void ATU_CTP_OTI::OnRspQryOrder(CThostFtdcOrderField *pOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast){

		//executed or canceled

	 	ATU_OTI_orderfeed_struct of;
	 	of.m_source = 1;
		if (bIsLast || pOrder==NULL) {
//			cout << "**************************************************************************************" << endl;
		}
		if (pOrder==NULL) {
			if (bIsLast) {
				of.m_islast=1;
				notify_orderfeed(of);
			}
			return;
		}
		addLog(__LOGSOURCE__,ATU_logfeed_struct::DEBUG,"OrderRef=%s","s",pOrder->OrderRef);
		if(pOrder->OrderStatus == THOST_FTDC_OST_AllTraded || pOrder->OrderStatus == THOST_FTDC_OST_Canceled || pOrder->OrderStatus == THOST_FTDC_OST_PartTradedNotQueueing || pOrder->OrderStatus == THOST_FTDC_OST_NoTradeNotQueueing){
			addLog(__LOGSOURCE__,ATU_logfeed_struct::DEBUG,"Order is executed or canceled","");
			if (bIsLast) {
				of.m_islast=1;
				notify_orderfeed(of);
			}
			return ;
		}

		of.m_timestamp = Toolbox::timeStamp_full();
		of.m_market = "CFFEX";
		of.m_feedcode = string(pOrder->InstrumentID);
		of.m_order_id = getOrderID(string(pOrder->OrderRef));
		of.m_price = pOrder->LimitPrice;
		of.m_qty = pOrder->VolumeTotalOriginal;
		of.m_open_or_close="N/A";
		if(pOrder->CombOffsetFlag[0] == THOST_FTDC_OF_Open){
			of.m_open_or_close = "open";
		}
		else if(pOrder->CombOffsetFlag[0] == THOST_FTDC_OF_Close){
			of.m_open_or_close = "close";
		}
		else if(pOrder->CombOffsetFlag[0] == THOST_FTDC_OF_CloseToday){
			of.m_open_or_close = "close";
		}

		if (pOrder->Direction == THOST_FTDC_D_Buy) {
			of.m_buy_or_sell = 1;
		}
		if (pOrder->Direction == THOST_FTDC_D_Sell) {
			of.m_buy_or_sell = 2;
		}

		of.m_qty_filled = pOrder->VolumeTraded;

		//working order should not be labeled as "deleted"
		of.m_deleted = 0;
		of.m_islast = 0;
		//TODO: Checking Status
		of.m_order_status = 0;
		of.m_error_description = "";

		//message reply
		of.m_source = 1;

		//TODO:stop order
		if(pOrder->OrderPriceType == THOST_FTDC_OPT_AnyPrice) {
			of.m_order_type = "market_order";
		}
		else if(pOrder->OrderPriceType == THOST_FTDC_OPT_LimitPrice){
			of.m_order_type = "limit_order";
		}

		of.m_order_validity = "N/A";
		notify_orderfeed(of);
		if (bIsLast) {
			of.m_islast=1;
			notify_orderfeed(of);
		}

}
///�����ѯ�ɽ���Ӧ
void ATU_CTP_OTI::OnRspQryTrade(CThostFtdcTradeField *pTrade, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast){
		ATU_OTI_tradefeed_struct tf;
		if (pTrade==NULL) {
			tf.m_source=1;
			tf.m_islast=1;
			notify_tradefeed(tf);
			return;
		}
		tf.m_timestamp=Toolbox::timeStamp_full();
		tf.m_market="CFFEX";
		tf.m_feedcode=string(pTrade->InstrumentID);
		tf.m_order_id=getOrderID(string(pTrade->OrderRef));
		tf.m_price=pTrade->Price;
		tf.m_qty=pTrade->Volume;
		tf.m_islast=0;
		tf.m_open_or_close="NA";
		if(pTrade->OffsetFlag == THOST_FTDC_OF_Open){
			tf.m_open_or_close="open";
		}
		else if(pTrade->OffsetFlag == THOST_FTDC_OF_Close){
			tf.m_open_or_close="close";
		}
		else if(pTrade->OffsetFlag == THOST_FTDC_OF_CloseToday){
			tf.m_open_or_close="close";
		}

		if(pTrade->Direction == THOST_FTDC_D_Buy){
			tf.m_buy_or_sell=1;
		}
		else if(pTrade->Direction == THOST_FTDC_D_Sell){
			tf.m_buy_or_sell=2;
		}

		if (pTrade->Direction==THOST_FTDC_D_Sell) {
			tf.m_buy_or_sell=2;
		}
		tf.m_trade_id=pTrade->TradeID;
		boost::trim(tf.m_trade_id);
		tf.m_portfolio_name="N/A";

		char _tradeTime_c[9],_tradeTime_l[9];
		strcpy(_tradeTime_c, pTrade->TradeTime);
		_tradeTime_l[0] = _tradeTime_c[0],_tradeTime_l[1] = _tradeTime_c[1],_tradeTime_l[2] = _tradeTime_c[3];
		_tradeTime_l[3] = _tradeTime_c[5],_tradeTime_l[4] = _tradeTime_c[7],_tradeTime_l[5] = _tradeTime_c[8];
		_tradeTime_l[6] = 0;
		string _tradeDate = pTrade->TradeDate, _tradeTime = _tradeTime_l;
		tf.m_trade_timestamp=_tradeDate+"_"+_tradeTime_l;
		tf.m_source=1;

		notify_tradefeed(tf);
		if (bIsLast) {
			tf.m_islast=1;
			notify_tradefeed(tf);
		}
}

string ATU_CTP_OTI::createOrderRef(string orderid) {
	string tmpstr=Toolbox::timeStamp_with_underscore();
	vector<string> fieldvec;
	boost::split(fieldvec, tmpstr, boost::is_any_of("_"));
	string currentOrderTimestamp=fieldvec[1];
	boost::unique_lock<boost::recursive_mutex> lock(m_orderRefMutex);
	if (m_lastOrderTimestamp.compare(currentOrderTimestamp)==0) {
		m_orderRefCount++;
	} else {
		m_orderRefCount=0;
		m_lastOrderTimestamp=currentOrderTimestamp;
	}
	char temp_orderId[15];
	sprintf(temp_orderId,"%s%06d",currentOrderTimestamp.c_str(),m_orderRefCount);
//	string orderref=currentOrderTimestamp+"_"+boost::lexical_cast<string>(m_orderRefCount);
	string orderref = string(temp_orderId);
	m_order_idToOrderRef[orderid]=orderref;
	m_OrderRefToorder_id[orderref]=orderid;
	return orderref;
}
string ATU_CTP_OTI::getOrderRef(string orderid) {
	boost::unique_lock<boost::recursive_mutex> lock(m_orderRefMutex);
	if (m_order_idToOrderRef.find(orderid)==m_order_idToOrderRef.end()) {
		return orderid;
	} else {
		return m_order_idToOrderRef[orderid];
	}
}
string ATU_CTP_OTI::getOrderID(string orderref) {
	boost::unique_lock<boost::recursive_mutex> lock(m_orderRefMutex);
	if (m_OrderRefToorder_id.find(orderref)==m_OrderRefToorder_id.end()) {
		return orderref;
	} else {
		return m_OrderRefToorder_id[orderref];
	}
}

void ATU_CTP_OTI::addLog(string logSource,int logSeverity,char *argsfmt,char *argtype,...) {
			va_list listPointer;
			va_start(listPointer,argtype);

			ATU_logfeed_struct *logfeed=copy2logfeed_struct(logSource,
					logSeverity,argsfmt,argtype, listPointer);
			notify_logfeed(logfeed);

		}


}
