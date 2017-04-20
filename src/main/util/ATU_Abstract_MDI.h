#ifndef ATU_ABSTRACT_MDI_H_
#define ATU_ABSTRACT_MDI_H_

#include <sstream>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include "SDateTime.h"

using namespace std;
using namespace boost;

typedef struct ATU_MDI_subscription_struct {
  string m_timestamp;
  string m_market;
  string m_instrument;
  string m_begindate;
  string m_enddate;
  string m_subscription_attribute;
  ATU_MDI_subscription_struct():
    m_timestamp(""),
    m_market(""),
    m_instrument(""),
    m_begindate(""),
    m_enddate(""),
    m_subscription_attribute(""){}
} ATU_MDI_subscription_struct;

typedef struct ATU_MDI_unsubscription_struct {
  string m_timestamp;
  string m_market;
  string m_instrument;
  ATU_MDI_unsubscription_struct():
    m_timestamp(""),
    m_market(""),
    m_instrument(""){}
} ATU_MDI_unsubscription_struct;

typedef struct ATU_MDI_acknowledgement_struct {
  string m_timestamp;
  string m_status;
  string m_errormsg;
  ATU_MDI_acknowledgement_struct():
    m_timestamp(""),
    m_status(""),
    m_errormsg(""){}
} ATU_MDI_acknowledgement_struct;


typedef struct ATU_MDI_marketfeed_struct {
  string m_timestamp;
  string m_instrument;
  double m_traded_price;
  double m_traded_volume;
  double m_bid_price_1;
  double m_bid_volume_1;
  double m_bid_price_2;
  double m_bid_volume_2;
  double m_bid_price_3;
  double m_bid_volume_3;
  double m_bid_price_4;
  double m_bid_volume_4;
  double m_bid_price_5;
  double m_bid_volume_5;
  double m_ask_price_1;
  double m_ask_volume_1;
  double m_ask_price_2;
  double m_ask_volume_2;
  double m_ask_price_3;
  double m_ask_volume_3;
  double m_ask_price_4;
  double m_ask_volume_4;
  double m_ask_price_5;
  double m_ask_volume_5;
  ATU_MDI_marketfeed_struct():
    m_timestamp(""),
    m_instrument(""),
    m_traded_price(0),
    m_traded_volume(0),
    m_bid_price_1(0),
    m_bid_volume_1(0),
    m_bid_price_2(0),
    m_bid_volume_2(0),
    m_bid_price_3(0),
    m_bid_volume_3(0),
    m_bid_price_4(0),
    m_bid_volume_4(0),
    m_bid_price_5(0),
    m_bid_volume_5(0),
    m_ask_price_1(0),
    m_ask_volume_1(0),
    m_ask_price_2(0),
    m_ask_volume_2(0),
    m_ask_price_3(0),
    m_ask_volume_3(0),
    m_ask_price_4(0),
    m_ask_volume_4(0),
    m_ask_price_5(0),
    m_ask_volume_5(0){}

  static string ToString(const ATU_MDI_marketfeed_struct & mfs)
  {
    std::ostringstream buffer; 
    buffer << mfs.m_timestamp     << ",";
    buffer << mfs.m_instrument      << ",";
    buffer << mfs.m_traded_price  << ",";
    buffer << mfs.m_traded_volume << ",B,";
    buffer << mfs.m_bid_price_1   << ",";
    buffer << mfs.m_bid_volume_1  << ",";
    buffer << mfs.m_bid_price_2   << ",";
    buffer << mfs.m_bid_volume_2  << ",";
    buffer << mfs.m_bid_price_3   << ",";
    buffer << mfs.m_bid_volume_3  << ",";
    buffer << mfs.m_bid_price_4   << ",";
    buffer << mfs.m_bid_volume_4  << ",";
    buffer << mfs.m_bid_price_5   << ",";
    buffer << mfs.m_bid_volume_5  << ",A,";
    buffer << mfs.m_ask_price_1   << ",";
    buffer << mfs.m_ask_volume_1  << ",";
    buffer << mfs.m_ask_price_2   << ",";
    buffer << mfs.m_ask_volume_2  << ",";
    buffer << mfs.m_ask_price_3   << ",";
    buffer << mfs.m_ask_volume_3  << ",";
    buffer << mfs.m_ask_price_4   << ",";
    buffer << mfs.m_ask_volume_4  << ",";
    buffer << mfs.m_ask_price_5   << ",";
    buffer << mfs.m_ask_volume_5        ;
    return buffer.str();
  }
  static bool ParseMDIString(const string & csv, ATU_MDI_marketfeed_struct & mfs)
  {
    vector<string> vMDI;
    boost::split(vMDI, csv, boost::is_any_of(","));
    if (vMDI.size() != 26) return false;

    mfs.m_timestamp     =                             vMDI[ 0] ;
    mfs.m_instrument    =                             vMDI[ 1] ;
    mfs.m_traded_price  = boost::lexical_cast<double>(vMDI[ 2]);
    mfs.m_traded_volume = boost::lexical_cast<double>(vMDI[ 3]);
    mfs.m_bid_price_1   = boost::lexical_cast<double>(vMDI[ 5]);
    mfs.m_bid_volume_1  = boost::lexical_cast<double>(vMDI[ 6]);
    mfs.m_bid_price_2   = boost::lexical_cast<double>(vMDI[ 7]);
    mfs.m_bid_volume_2  = boost::lexical_cast<double>(vMDI[ 8]);
    mfs.m_bid_price_3   = boost::lexical_cast<double>(vMDI[ 9]);
    mfs.m_bid_volume_3  = boost::lexical_cast<double>(vMDI[10]);
    mfs.m_bid_price_4   = boost::lexical_cast<double>(vMDI[11]);
    mfs.m_bid_volume_4  = boost::lexical_cast<double>(vMDI[12]);
    mfs.m_bid_price_5   = boost::lexical_cast<double>(vMDI[13]);
    mfs.m_bid_volume_5  = boost::lexical_cast<double>(vMDI[14]);
    mfs.m_ask_price_1   = boost::lexical_cast<double>(vMDI[16]);
    mfs.m_ask_volume_1  = boost::lexical_cast<double>(vMDI[17]);
    mfs.m_ask_price_2   = boost::lexical_cast<double>(vMDI[18]);
    mfs.m_ask_volume_2  = boost::lexical_cast<double>(vMDI[19]);
    mfs.m_ask_price_3   = boost::lexical_cast<double>(vMDI[20]);
    mfs.m_ask_volume_3  = boost::lexical_cast<double>(vMDI[21]);
    mfs.m_ask_price_4   = boost::lexical_cast<double>(vMDI[22]);
    mfs.m_ask_volume_4  = boost::lexical_cast<double>(vMDI[23]);
    mfs.m_ask_price_5   = boost::lexical_cast<double>(vMDI[24]);
    mfs.m_ask_volume_5  = boost::lexical_cast<double>(vMDI[25]);

    return true;
  }
} ATU_MDI_marketfeed_struct;

typedef struct ATU_MDI_binary_marketfeed_struct {
  uint64_t m_microsec_since_epoch_gmt;
  char     m_instrument[50];
  double   m_traded_price;
  int32_t  m_traded_volume;
  double   m_bid_price_1;
  int32_t  m_bid_volume_1;
  double   m_bid_price_2;
  int32_t  m_bid_volume_2;
  double   m_bid_price_3;
  int32_t  m_bid_volume_3;
  double   m_bid_price_4;
  int32_t  m_bid_volume_4;
  double   m_bid_price_5;
  int32_t  m_bid_volume_5;
  double   m_ask_price_1;
  int32_t  m_ask_volume_1;
  double   m_ask_price_2;
  int32_t  m_ask_volume_2;
  double   m_ask_price_3;
  int32_t  m_ask_volume_3;
  double   m_ask_price_4;
  int32_t  m_ask_volume_4;
  double   m_ask_price_5;
  int32_t  m_ask_volume_5;
  static string ToString(const ATU_MDI_binary_marketfeed_struct & bmfs)
  {
    std::ostringstream buffer; 
    buffer << SDateTime::fromUnixTimeToString(bmfs.m_microsec_since_epoch_gmt, SDateTime::MICROSEC, SDateTime::GMT, SDateTime::HKT) << ",";
    buffer << bmfs.m_instrument    << ",";
    buffer << bmfs.m_traded_price  << ",";
    buffer << bmfs.m_traded_volume << ",B,";
    buffer << bmfs.m_bid_price_1   << ",";
    buffer << bmfs.m_bid_volume_1  << ",";
    buffer << bmfs.m_bid_price_2   << ",";
    buffer << bmfs.m_bid_volume_2  << ",";
    buffer << bmfs.m_bid_price_3   << ",";
    buffer << bmfs.m_bid_volume_3  << ",";
    buffer << bmfs.m_bid_price_4   << ",";
    buffer << bmfs.m_bid_volume_4  << ",";
    buffer << bmfs.m_bid_price_5   << ",";
    buffer << bmfs.m_bid_volume_5  << ",A,";
    buffer << bmfs.m_ask_price_1   << ",";
    buffer << bmfs.m_ask_volume_1  << ",";
    buffer << bmfs.m_ask_price_2   << ",";
    buffer << bmfs.m_ask_volume_2  << ",";
    buffer << bmfs.m_ask_price_3   << ",";
    buffer << bmfs.m_ask_volume_3  << ",";
    buffer << bmfs.m_ask_price_4   << ",";
    buffer << bmfs.m_ask_volume_4  << ",";
    buffer << bmfs.m_ask_price_5   << ",";
    buffer << bmfs.m_ask_volume_5        ;
    return buffer.str();
  }
  static bool ParseMDIString(const string & csv, ATU_MDI_binary_marketfeed_struct & bmfs)
  {
    vector<string> vMDI;
    boost::split(vMDI, csv, boost::is_any_of(","));
    if (vMDI.size() != 26) return false;

    // bmfs.m_timestamp     =                             vMDI[ 0] ;
    // bmfs.m_instrument    =                             vMDI[ 1] ;
    // bmfs.m_traded_price  = boost::lexical_cast<double>(vMDI[ 2]);
    // bmfs.m_traded_volume = boost::lexical_cast<double>(vMDI[ 3]);
    // bmfs.m_bid_price_1   = boost::lexical_cast<double>(vMDI[ 5]);
    // bmfs.m_bid_volume_1  = boost::lexical_cast<double>(vMDI[ 6]);
    // bmfs.m_bid_price_2   = boost::lexical_cast<double>(vMDI[ 7]);
    // bmfs.m_bid_volume_2  = boost::lexical_cast<double>(vMDI[ 8]);
    // bmfs.m_bid_price_3   = boost::lexical_cast<double>(vMDI[ 9]);
    // bmfs.m_bid_volume_3  = boost::lexical_cast<double>(vMDI[10]);
    // bmfs.m_bid_price_4   = boost::lexical_cast<double>(vMDI[11]);
    // bmfs.m_bid_volume_4  = boost::lexical_cast<double>(vMDI[12]);
    // bmfs.m_bid_price_5   = boost::lexical_cast<double>(vMDI[13]);
    // bmfs.m_bid_volume_5  = boost::lexical_cast<double>(vMDI[14]);
    // bmfs.m_ask_price_1   = boost::lexical_cast<double>(vMDI[16]);
    // bmfs.m_ask_volume_1  = boost::lexical_cast<double>(vMDI[17]);
    // bmfs.m_ask_price_2   = boost::lexical_cast<double>(vMDI[18]);
    // bmfs.m_ask_volume_2  = boost::lexical_cast<double>(vMDI[19]);
    // bmfs.m_ask_price_3   = boost::lexical_cast<double>(vMDI[20]);
    // bmfs.m_ask_volume_3  = boost::lexical_cast<double>(vMDI[21]);
    // bmfs.m_ask_price_4   = boost::lexical_cast<double>(vMDI[22]);
    // bmfs.m_ask_volume_4  = boost::lexical_cast<double>(vMDI[23]);
    // bmfs.m_ask_price_5   = boost::lexical_cast<double>(vMDI[24]);
    // bmfs.m_ask_volume_5  = boost::lexical_cast<double>(vMDI[25]);

    return true;
  }
} ATU_MDI_binary_marketfeed_struct;

typedef struct ATU_MDI_ping_struct {
  string m_timestamp;
  ATU_MDI_ping_struct():
    m_timestamp(""){}
} ATU_MDI_ping_struct;

typedef struct ATU_MDI_reset_struct {
  string m_timestamp;
  ATU_MDI_reset_struct():
    m_timestamp(""){}
} ATU_MDI_reset_struct;

typedef struct ATU_MDI_ohlcfeed_struct {
  string m_timestamp;
  string m_market;
  string m_instrument;
  double m_open;
  double m_high;
  double m_low;
  double m_close;
  double m_traded_volume;
  ATU_MDI_ohlcfeed_struct():
    m_timestamp(""),
    m_market(""),
    m_instrument(""),
    m_open(0),
    m_high(0),
    m_low(0),
    m_close(0),
    m_traded_volume(0){}
} ATU_MDI_ohlcfeed_struct;

#endif /* ATU_ABSTRACT_MDI_H_ */
