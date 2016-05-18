#include <StrategyB2.h>

StrategyB2::StrategyB2() :
  m_TheoCPnLHistFolder(""),
  m_TheoPosFolder(""),
  m_B2_HasEnabledMinCommissionCheck(true),
  m_B2_WhetherToRetain(false),
  m_B2_ActionTimeBefCloseInSec(180),
  m_MoveNextBestGroupUpIfNoSignal(true),
  m_MoveNextBestStkInGrpUpIfNoSignal(true),
  // m_ES_SPY_Together_SPYidx(-1),
  // m_SPY_Px(NAN),
  // m_NoOfSgndESCtrtReqd(0),
  StrategyBase()
{
  UnsetConvenienceVarb();
  m_iNumOfParam = 25;
}

StrategyB2::~StrategyB2()
{
}


bool StrategyB2::GetEstimate(const GetEstParam & gep)
{

  return GetEstimate(
    gep.trainingperiod,
    gep.beta_1,
    gep.beta_2,
    gep.beta_3,
    gep.beta_4,
    *gep.v_hist_trfpx,
    *gep.v_hist_close,
    gep.pdAvgSquaredError,
    gep.pdAvgPnL,
    gep.pdSharpe,
    gep.pdFinalEstimate,
    gep.ws,
    gep.curbInSmpReturn,
    gep.hypothesis
    );
}


double StrategyB2::CalcPredictionTaylor(const vector<double> & v_hist_trfpx, const int idx, const double beta_1, const double beta_2, const double beta_3, const double beta_4, const int hypothesis)
{
  if (hypothesis == B2_HYPOTHESIS_TAYLOR1)
  {
    if (v_hist_trfpx.size()-1 < idx) return NAN;

    double dTaylor1 = (v_hist_trfpx[idx] > v_hist_trfpx[idx-1] ? beta_1 : beta_2) * (v_hist_trfpx[idx] - v_hist_trfpx[idx-1]);
    double dTaylor2 = (v_hist_trfpx[idx] > v_hist_trfpx[idx-1] ? beta_3 : beta_4) * (v_hist_trfpx[idx] - 2 * v_hist_trfpx[idx-1] + v_hist_trfpx[idx-2]);
    return v_hist_trfpx[idx] + dTaylor1 + dTaylor2;
  }
  else if (hypothesis == B2_HYPOTHESIS_TAYLOR2)
  {
    if (v_hist_trfpx.size()-1 < idx) return NAN;

    double dTaylor1 = (v_hist_trfpx[idx] > v_hist_trfpx[idx-1] ? beta_1 : beta_2) * (v_hist_trfpx[idx] - v_hist_trfpx[idx-2]);
    double dTaylor2 = (v_hist_trfpx[idx] > v_hist_trfpx[idx-1] ? beta_3 : beta_4) * (v_hist_trfpx[idx] - 2 * v_hist_trfpx[idx-1] + v_hist_trfpx[idx-2]);
    return v_hist_trfpx[idx] + dTaylor1 + dTaylor2;
  }
}

double StrategyB2::CalcPredictionSMA(const vector<double> & v_hist_trfpx, const int idx, const double beta_1, const double beta_2, const double beta_3, const double beta_4)
{
  int iPrd1 = round(beta_3);
  int iPrd2 = round(beta_4);
  if (v_hist_trfpx.size()-1 < idx) return NAN;
  if (idx+1 < iPrd1) return NAN;
  if (idx+1 < iPrd2) return NAN;
  if (abs(beta_1) < NIR_EPSILON && abs(beta_2) < NIR_EPSILON) return NAN;

  double dSum1 = 0;
  double dSum2 = 0;
  for (int i = 0; i < iPrd1; ++i) dSum1 += v_hist_trfpx[idx-i];
  for (int i = 0; i < iPrd2; ++i) dSum2 += v_hist_trfpx[idx-i];
  double dAvg1 = dSum1 / iPrd1;
  double dAvg2 = dSum2 / iPrd2;

  const double dCurPx = v_hist_trfpx[idx];
  int iIndiVar1 = (dCurPx > dAvg1 ? 1 : 0);
  int iIndiVar2 = (dCurPx > dAvg2 ? 1 : 0);

  if (abs(beta_1) > NIR_EPSILON && abs(beta_2) > NIR_EPSILON)
    return beta_1 * (double) iIndiVar1 + beta_2 * (double) iIndiVar2;
  else if (abs(beta_1) > NIR_EPSILON)
    return beta_1 * (double) iIndiVar1;
  else if (abs(beta_2) > NIR_EPSILON)
    return beta_2 * (double) iIndiVar2;
  else
    return NAN;
}


bool StrategyB2::GetEstimate(
  const double trainingperiod,
  const double beta_1,
  const double beta_2,
  const double beta_3,
  const double beta_4,
  const vector<double> & v_hist_trfpx,
  const vector<double> & v_hist_close,
  double * pdAvgSquaredError,
  double * pdAvgPnL,
  double * pdSharpe,
  double * pdFinalEstimate,
  WEIGHTING_SCHEME ws,
  const double curbInSmpReturn,
  const int hypothesis
  )
{
  // m_Logger->Write(Logger::DEBUG,"StrategyB2: %s::%s (%d) GetEstimate beta_1 %f beta_2 %f beta_3 %f beta_4 %f",
  //                 __FILE__,__FUNCTION__,__LINE__, beta_1,beta_2,beta_3,beta_4);
  if (v_hist_trfpx.size() != v_hist_close.size())
  {
    m_Logger->Write(Logger::DEBUG,"StrategyB2: %s::%s (%d) GetEstimate returns false.",__FILE__,__FUNCTION__,__LINE__);
    return false;
  }
  if (v_hist_trfpx.size() < trainingperiod)
  {
    m_Logger->Write(Logger::DEBUG,"StrategyB2: %s::%s (%d) GetEstimate returns false.",__FILE__,__FUNCTION__,__LINE__);
    return false;
  }
  if (v_hist_trfpx.size() < 2)
  {
    m_Logger->Write(Logger::DEBUG,"StrategyB2: %s::%s (%d) GetEstimate returns false.",__FILE__,__FUNCTION__,__LINE__);
    return false;
  }

  boost::accumulators::accumulator_set<double, boost::accumulators::features<boost::accumulators::tag::mean, boost::accumulators::tag::variance>, double> acc;
  double dPnL = 0;
  double dTotalWeight = 0;
  double dAvgSquaredError = 0;

  for (unsigned int i = 2; i < v_hist_trfpx.size()-1; ++i)
  {
    double dWeight = 0;
    if (ws == WS_LINEAR)
    {
      dWeight = max((double)i-(double)v_hist_trfpx.size()+(double)trainingperiod,(double)0);
      if (dWeight <= NIR_EPSILON) continue;
    }
    else if (ws == WS_UNIFORM)
    {
      if (i < (double)v_hist_trfpx.size()-(double)trainingperiod) continue;
      dWeight = 1;
    }

    dTotalWeight += dWeight;

    double dPrediction = NAN;
    switch(hypothesis)
    {
      case B2_HYPOTHESIS_TAYLOR1:
      case B2_HYPOTHESIS_TAYLOR2:
        {
          dPrediction = CalcPredictionTaylor(v_hist_trfpx, i, beta_1, beta_2, beta_3, beta_4, hypothesis);
          break;
        }
      case B2_HYPOTHESIS_SMA:
        {
          if (CalcPredictionSMA(v_hist_trfpx, i, beta_1, beta_2, beta_3, beta_4) >= 0)
            dPrediction = v_hist_trfpx[i] * 1.01;
          else
            dPrediction = v_hist_trfpx[i] * 0.99;
          break;
        }
      default:
        {
          break;
        }
    }

    if (STool::IsValidPriceOrVol(dPrediction))
    {
      if (pdAvgSquaredError) dAvgSquaredError += dWeight * pow(dPrediction-v_hist_trfpx[i+1],2);
      if (pdAvgPnL || pdSharpe)
      {
        if (curbInSmpReturn>0)  dPnL = (dPrediction > v_hist_trfpx[i] ? 1 : -1) * max(min((v_hist_close[i+1] / v_hist_close[i]) -1, curbInSmpReturn),-curbInSmpReturn);
        else                    dPnL = (dPrediction > v_hist_trfpx[i] ? 1 : -1) * ((v_hist_close[i+1] / v_hist_close[i]) -1);

        acc(dPnL, boost::accumulators::weight = dWeight);
      }
    }
  }

  if (pdAvgSquaredError) *pdAvgSquaredError = dAvgSquaredError / dTotalWeight;

  //--------------------------------------------------
  // Output PnL Sharpe
  //--------------------------------------------------
  if (pdAvgPnL)
  {
    *pdAvgPnL = boost::accumulators::mean(acc);
  }
  if (pdSharpe)
  {
    *pdSharpe = boost::accumulators::mean(acc) / sqrt(boost::accumulators::variance(acc));
  }
  //--------------------------------------------------
  // Final estimate
  //--------------------------------------------------
  if (pdFinalEstimate)
  {
    int i = v_hist_trfpx.size()-1;
    switch(hypothesis)
    {
      case B2_HYPOTHESIS_TAYLOR1:
      case B2_HYPOTHESIS_TAYLOR2:
        {
          *pdFinalEstimate = CalcPredictionTaylor(v_hist_trfpx, i, beta_1, beta_2, beta_3, beta_4, hypothesis) - v_hist_trfpx[i];
          break;
        }
      case B2_HYPOTHESIS_SMA:
        {
          *pdFinalEstimate = CalcPredictionSMA(v_hist_trfpx, i, beta_1, beta_2, beta_3, beta_4);
          break;
        }
      default:
        {

          break;
        }
    }
  }

  return true;
}


bool StrategyB2::TrainUpParamTaylor(
  const string & symbol,
  const YYYYMMDDHHMMSS & ymdhms,
  const double trainingperiod1,
  const double trainingperiod2,
  const double trainingperiod3,
  const double trainingperiod4,
  double & beta_1,
  double & beta_2,
  double & beta_3,
  double & beta_4,
  const vector<double> & v_hist_trfpx,
  const vector<double> & v_hist_close,
  const CountingFunction & countingfunc,
  const TRAINMODE tm,
  const WEIGHTING_SCHEME ws,
  map<double,vector<double> > & mapBestParamSetBeta1Beta3_hypo1,
  map<double,vector<double> > & mapBestParamSetBeta2Beta4_hypo1,
  map<double,vector<double> > & mapBestParamSetBeta1Beta3_hypo2,
  map<double,vector<double> > & mapBestParamSetBeta2Beta4_hypo2,
  const double curbInSmpReturn
)
{
  if (
    trainingperiod1 == 0 &&
    trainingperiod2 == 0 &&
    trainingperiod3 == 0 &&
    trainingperiod3 == 0
    )
  {
    m_Logger->Write(Logger::DEBUG,"StrategyB2: %s::%s (%d) %s TrainUpParamTaylor exits: Training periods are 0.",__FILE__,__FUNCTION__,__LINE__,symbol.c_str());
    return true;
  }

  if (
    trainingperiod1 > v_hist_trfpx.size() ||
    trainingperiod2 > v_hist_trfpx.size() ||
    trainingperiod3 > v_hist_trfpx.size() ||
    trainingperiod4 > v_hist_trfpx.size() ||
    trainingperiod1 > countingfunc.Size() ||
    trainingperiod2 > countingfunc.Size() ||
    trainingperiod3 > countingfunc.Size() ||
    trainingperiod4 > countingfunc.Size()
    )
  {
    m_Logger->Write(Logger::DEBUG,"StrategyB1: %s::%s (%d) %s TrainUpParamTaylor exits: Not enough historical data for the training periods. v_hist_trfpx.size() %d",
                    __FILE__,__FUNCTION__,__LINE__, symbol.c_str(), v_hist_trfpx.size());
    return false;
  }

  if (
    trainingperiod1 <= 0 ||
    trainingperiod2 <= 0 ||
    trainingperiod3 <= 0 ||
    trainingperiod4 <= 0
    )
  {
    m_Logger->Write(Logger::DEBUG,"StrategyB2: %s::%s (%d) %s TrainUpParamTaylor exits: Not all training periods are > 0.",__FILE__,__FUNCTION__,__LINE__, symbol.c_str());
    return false;
  }


  int iTaylorIdx = B2_HYPOTHESIS_TAYLOR1;
  while (iTaylorIdx <= B2_HYPOTHESIS_TAYLOR2)
  {
    map<double,vector<double> > * mapBestParamSetBeta1Beta3 = NULL;
    map<double,vector<double> > * mapBestParamSetBeta2Beta4 = NULL;

    if (iTaylorIdx == B2_HYPOTHESIS_TAYLOR1)
    {
      mapBestParamSetBeta1Beta3 = &mapBestParamSetBeta1Beta3_hypo1;
      mapBestParamSetBeta2Beta4 = &mapBestParamSetBeta2Beta4_hypo1;
    }
    else if (iTaylorIdx == B2_HYPOTHESIS_TAYLOR2)
    {
      mapBestParamSetBeta1Beta3 = &mapBestParamSetBeta1Beta3_hypo2;
      mapBestParamSetBeta2Beta4 = &mapBestParamSetBeta2Beta4_hypo2;
    }

    //--------------------------------------------------
    // 0 = rising, 1 = falling
    //--------------------------------------------------
    for (unsigned int rising_or_falling = 0; rising_or_falling < 2; ++rising_or_falling)
    {
      vector<double> dPrelimAvgPnL1;
      vector<double> dPrelimAvgPnL3;
      vector<double> dPrelimAvgPnL4;
      vector<double> dPrelimSharpe1;
      vector<double> dPrelimSquaredError1;

      if (rising_or_falling == 0)
        mapBestParamSetBeta1Beta3->clear();
      else if (rising_or_falling == 1)
        mapBestParamSetBeta2Beta4->clear();


      //--------------------------------------------------
      double dTrainingPeriodToUse = 0;
      if (rising_or_falling == 0)
      {
        if (countingfunc.CountLastNElements(trainingperiod2) < trainingperiod1)
        {
          //--------------------------------------------------
          // The number of rising days in the whole trainingperiod2 was less than trainingperiod1, so just take the entire period
          //--------------------------------------------------
          dTrainingPeriodToUse = trainingperiod2;
        }
        else
        {
          dTrainingPeriodToUse = countingfunc.HowManyElemFromTailToAccum(trainingperiod1);
        }
      }
      else if (rising_or_falling == 1)
      {
        if (trainingperiod2 - countingfunc.CountLastNElements(trainingperiod2) < trainingperiod1)
        {
          //--------------------------------------------------
          // The number of falling days in the whole trainingperiod2 was less than trainingperiod1, so just take the entire period
          //--------------------------------------------------
          dTrainingPeriodToUse = trainingperiod2;
        }
        else
        {
          dTrainingPeriodToUse = countingfunc.HowManyElemFromTailToAccumOpp(trainingperiod1);
        }
      }
      m_Logger->Write(Logger::INFO,"StrategyB2: %s %s Looping rising / falling regime = %d. Using training period %f.",
                      ymdhms.ToStr().c_str(),
                      symbol.c_str(),
                      rising_or_falling,dTrainingPeriodToUse);


      //--------------------------------------------------
      double dFirTermLowerBound = 0;
      double dFirTermUpperBound = 0;
      double dSecTermLowerBound = 0;
      double dSecTermUpperBound = 0;
      double dFirTermAdj        = 0;
      double dSecTermAdj        = 0;

      if (rising_or_falling == 0)
      {
        dFirTermLowerBound = m_b1_low;
        dFirTermUpperBound = m_b1_high;
        dFirTermAdj        = m_b1_adj;
        dSecTermLowerBound = m_b3_low;
        dSecTermUpperBound = m_b3_high;
        dSecTermAdj        = m_b3_adj;
      }
      else if (rising_or_falling == 1)
      {
        dFirTermLowerBound = m_b2_low;
        dFirTermUpperBound = m_b2_high;
        dFirTermAdj        = m_b2_adj;
        dSecTermLowerBound = m_b4_low;
        dSecTermUpperBound = m_b4_high;
        dSecTermAdj        = m_b4_adj;
      }

      for (double bFir = dFirTermLowerBound; bFir <= dFirTermUpperBound; bFir += dFirTermAdj)
      {
        dPrelimAvgPnL1      .clear();
        dPrelimAvgPnL3      .clear();
        dPrelimAvgPnL4      .clear();
        dPrelimSharpe1      .clear();
        dPrelimSquaredError1.clear();

        for (double bSec = dSecTermLowerBound; bSec <= dSecTermUpperBound; bSec += dSecTermAdj)
        {
          dPrelimAvgPnL1      .push_back(NAN);
          dPrelimAvgPnL3      .push_back(NAN);
          dPrelimAvgPnL4      .push_back(NAN);
          dPrelimSharpe1      .push_back(NAN);
          dPrelimSquaredError1.push_back(NAN);
        }

        //--------------------------------------------------
        int iCnt = 0;
        boost::thread_group thdgrp;

        for (double bSec = dSecTermLowerBound; bSec <= dSecTermUpperBound; bSec += dSecTermAdj)
        {

          GetEstParam gep1;
          GetEstParam gep3;
          GetEstParam gep4;

          if (rising_or_falling == 0)
          {
            gep1.beta_1          =   bFir;
            gep1.beta_3          =   bSec;
            gep3.beta_1          =   bFir;
            gep3.beta_3          =   bSec;
            gep4.beta_1          =   bFir;
            gep4.beta_3          =   bSec;
            gep1.beta_2          =   0;
            gep1.beta_4          =   0;
            gep3.beta_2          =   0;
            gep3.beta_4          =   0;
            gep4.beta_2          =   0;
            gep4.beta_4          =   0;
          }
          else if (rising_or_falling == 1)
          {
            gep1.beta_1          =   0;
            gep1.beta_3          =   0;
            gep3.beta_1          =   0;
            gep3.beta_3          =   0;
            gep4.beta_1          =   0;
            gep4.beta_3          =   0;
            gep1.beta_2          =   bFir;
            gep1.beta_4          =   bSec;
            gep3.beta_2          =   bFir;
            gep3.beta_4          =   bSec;
            gep4.beta_2          =   bFir;
            gep4.beta_4          =   bSec;
          }

          gep1.trainingperiod    =   dTrainingPeriodToUse;
          gep1.v_hist_trfpx      =   &v_hist_trfpx;
          gep1.v_hist_close      =   &v_hist_close;
          gep1.pdAvgSquaredError =   &dPrelimSquaredError1[iCnt];
          gep1.pdAvgPnL          =   &dPrelimAvgPnL1      [iCnt];
          gep1.pdSharpe          =   &dPrelimSharpe1      [iCnt];
          gep1.pdFinalEstimate   =   NULL;
          gep1.ws                =   ws;
          gep1.curbInSmpReturn   =   curbInSmpReturn;
          gep1.hypothesis        =   iTaylorIdx;

          gep3.trainingperiod    =   trainingperiod3;
          gep3.v_hist_trfpx      =   &v_hist_trfpx;
          gep3.v_hist_close      =   &v_hist_close;
          gep3.pdAvgSquaredError =   NULL;
          gep3.pdAvgPnL          =   &dPrelimAvgPnL3[iCnt];
          gep3.pdSharpe          =   NULL;
          gep3.pdFinalEstimate   =   NULL;
          gep3.ws                =   ws;
          gep3.curbInSmpReturn   =   curbInSmpReturn;
          gep3.hypothesis        =   iTaylorIdx;

          gep4.trainingperiod    =   trainingperiod4;
          gep4.v_hist_trfpx      =   &v_hist_trfpx;
          gep4.v_hist_close      =   &v_hist_close;
          gep4.pdAvgSquaredError =   NULL;
          gep4.pdAvgPnL          =   &dPrelimAvgPnL4[iCnt];
          gep4.pdSharpe          =   NULL;
          gep4.pdFinalEstimate   =   NULL;
          gep4.ws                =   ws;
          gep4.curbInSmpReturn   =   curbInSmpReturn;
          gep4.hypothesis        =   iTaylorIdx;

          iCnt++;

          thdgrp.add_thread(new boost::thread(boost::bind(&StrategyB2::GetEstimate, this, gep1)));
          thdgrp.add_thread(new boost::thread(boost::bind(&StrategyB2::GetEstimate, this, gep3)));
          thdgrp.add_thread(new boost::thread(boost::bind(&StrategyB2::GetEstimate, this, gep4)));
        }

        thdgrp.join_all();

        //--------------------------------------------------
        iCnt = 0;
        for (double bSec = dSecTermLowerBound; bSec <= dSecTermUpperBound; bSec += dSecTermAdj)
        {
          if (
            !std::isnan(dPrelimAvgPnL3[iCnt]) &&
            dPrelimAvgPnL3[iCnt] > 0
            &&
            !std::isnan(dPrelimAvgPnL4[iCnt]) &&
            dPrelimAvgPnL4[iCnt] > 0
            )
          {
            vector<double> vTmp;
            vTmp.push_back(bFir);
            vTmp.push_back(bSec);
            if (rising_or_falling == 0)
            {
              if      (tm == TM_MINSQERR)  (*mapBestParamSetBeta1Beta3)[dPrelimSquaredError1[iCnt]] = vTmp;
              else if (tm == TM_MAXPROFIT) (*mapBestParamSetBeta1Beta3)[dPrelimAvgPnL1      [iCnt]] = vTmp;
              else if (tm == TM_MAXSHARPE) (*mapBestParamSetBeta1Beta3)[dPrelimSharpe1      [iCnt]] = vTmp;
            }
            else if (rising_or_falling == 1)
            {
              if      (tm == TM_MINSQERR)  (*mapBestParamSetBeta2Beta4)[dPrelimSquaredError1[iCnt]] = vTmp;
              else if (tm == TM_MAXPROFIT) (*mapBestParamSetBeta2Beta4)[dPrelimAvgPnL1      [iCnt]] = vTmp;
              else if (tm == TM_MAXSHARPE) (*mapBestParamSetBeta2Beta4)[dPrelimSharpe1      [iCnt]] = vTmp;
            }
          }
          if (rising_or_falling == 0)
          {
            m_Logger->Write(Logger::DEBUG,"StrategyB2: %s %s Looped parameters: beta_1 = %f, beta_2 = %f, beta_3 = %f, beta_4 = %f, dPrelimSquaredError = %f. dPrelimAvgPnL = %f. dPrelimSharpe = %f. dPrelimAvgPnL3 = %f. dPrelimAvgPnL4 = %f.",
                            ymdhms.ToStr().c_str(),symbol.c_str(),bFir,(double)0,bSec,(double)0,dPrelimSquaredError1[iCnt],dPrelimAvgPnL1[iCnt],dPrelimSharpe1[iCnt],dPrelimAvgPnL3[iCnt],dPrelimAvgPnL4[iCnt]);
          }
          else if (rising_or_falling == 1)
          {
            m_Logger->Write(Logger::DEBUG,"StrategyB2: %s %s Looped parameters: beta_1 = %f, beta_2 = %f, beta_3 = %f, beta_4 = %f, dPrelimSquaredError = %f. dPrelimAvgPnL = %f. dPrelimSharpe = %f. dPrelimAvgPnL3 = %f. dPrelimAvgPnL4 = %f.",
                            ymdhms.ToStr().c_str(),symbol.c_str(),(double)0,bFir,(double)0,bSec,dPrelimSquaredError1[iCnt],dPrelimAvgPnL1[iCnt],dPrelimSharpe1[iCnt],dPrelimAvgPnL3[iCnt],dPrelimAvgPnL4[iCnt]);
          }

          iCnt++;
        }
      }
    }


    if (!mapBestParamSetBeta1Beta3->empty())
    {
      map<double,vector<double> >::iterator it_prelim;
      if (tm == TM_MINSQERR)
      {
        it_prelim = mapBestParamSetBeta1Beta3->begin();
      }
      else if (tm == TM_MAXPROFIT)
      {
        it_prelim = mapBestParamSetBeta1Beta3->end();
        --it_prelim;
      }
      else if (tm == TM_MAXSHARPE)
      {
        it_prelim = mapBestParamSetBeta1Beta3->end();
        --it_prelim;
      }
      beta_1  = it_prelim->second[0];
      beta_3  = it_prelim->second[1];
      m_Logger->Write(Logger::INFO,"StrategyB2: %s Updated to the best parameters after looping: beta_1 = %f, beta_3 = %f.",ymdhms.ToStr().c_str(),beta_1,beta_3);
      m_Logger->Write(Logger::INFO,"StrategyB2: %s Num of best parameters in mapBestParamSetBeta1Beta3: %d",ymdhms.ToStr().c_str(),mapBestParamSetBeta1Beta3->size());
    }
    else
    {
      m_Logger->Write(Logger::INFO,"StrategyB2: %s Parameters remain unchanged: beta_1 = %f, beta_3 = %f.",
                      ymdhms.ToStr().c_str(),beta_1,beta_3);
      m_Logger->Write(Logger::INFO,"StrategyB2: %s Num of best parameters in mapBestParamSetBeta1Beta3: %d",ymdhms.ToStr().c_str(),mapBestParamSetBeta1Beta3->size());
    }


    if (!mapBestParamSetBeta2Beta4->empty())
    {
      map<double,vector<double> >::iterator it_prelim;
      if (tm == TM_MINSQERR)
      {
        it_prelim = mapBestParamSetBeta2Beta4->begin();
      }
      else if (tm == TM_MAXPROFIT)
      {
        it_prelim = mapBestParamSetBeta2Beta4->end();
        --it_prelim;
      }
      else if (tm == TM_MAXSHARPE)
      {
        it_prelim = mapBestParamSetBeta2Beta4->end();
        --it_prelim;
      }
      beta_2  = it_prelim->second[0];
      beta_4  = it_prelim->second[1];
      m_Logger->Write(Logger::INFO,"StrategyB2: %s Updated to the best parameters after looping: beta_2 = %f, beta_4 = %f.",ymdhms.ToStr().c_str(),beta_2,beta_4);
      m_Logger->Write(Logger::INFO,"StrategyB2: %s Num of best parameters in mapBestParamSetBeta2Beta4: %d",ymdhms.ToStr().c_str(),mapBestParamSetBeta2Beta4->size());
    }
    else
    {
      m_Logger->Write(Logger::INFO,"StrategyB2: %s Parameters remain unchanged: beta_2 = %f, beta_4 = %f.",
                      ymdhms.ToStr().c_str(),beta_2,beta_4);
      m_Logger->Write(Logger::INFO,"StrategyB2: %s Num of best parameters in mapBestParamSetBeta2Beta4: %d",ymdhms.ToStr().c_str(),mapBestParamSetBeta2Beta4->size());
    }

    iTaylorIdx++;
  }

  return true;
}

bool StrategyB2::TrainUpParamSMA(
  const string & symbol,
  const YYYYMMDDHHMMSS & ymdhms,
  const double trainingperiod1,
  const double trainingperiod2,
  const double trainingperiod3,
  const double trainingperiod4,
  double & beta_1,
  double & beta_2,
  double & beta_3,
  double & beta_4,
  const vector<double> & v_hist_trfpx,
  const vector<double> & v_hist_close,
  const CountingFunction & countingfunc,
  const TRAINMODE tm,
  const WEIGHTING_SCHEME ws,
  map<double,vector<double> > & mapBestParamSetBeta1Beta3,
  map<double,vector<double> > & mapBestParamSetBeta2Beta4,
  const double curbInSmpReturn
  )
{
  if (
    trainingperiod1 == 0 &&
    trainingperiod2 == 0 &&
    trainingperiod3 == 0 &&
    trainingperiod3 == 0
    )
  {
    m_Logger->Write(Logger::DEBUG,"StrategyB2: %s::%s (%d) %s TrainUpParamSMA exits: Training periods are 0.",__FILE__,__FUNCTION__,__LINE__, symbol.c_str());
    return true;
  }

  if (
    trainingperiod1 > v_hist_trfpx.size() ||
    trainingperiod2 > v_hist_trfpx.size() ||
    trainingperiod3 > v_hist_trfpx.size() ||
    trainingperiod4 > v_hist_trfpx.size() ||
    trainingperiod1 > countingfunc.Size() ||
    trainingperiod2 > countingfunc.Size() ||
    trainingperiod3 > countingfunc.Size() ||
    trainingperiod4 > countingfunc.Size()
    )
  {
    m_Logger->Write(Logger::DEBUG,"StrategyB1: %s::%s (%d) %s TrainUpParamSMA exits: Not enough historical data for the training periods. v_hist_trfpx.size() %d",
                    __FILE__,__FUNCTION__,__LINE__, symbol.c_str(), v_hist_trfpx.size());
    return false;
  }

  if (
    trainingperiod1 <= 0 ||
    trainingperiod2 <= 0 ||
    trainingperiod3 <= 0 ||
    trainingperiod4 <= 0
    )
  {
    m_Logger->Write(Logger::DEBUG,"StrategyB2: %s::%s (%d) %s TrainUpParamSMA exits: Not all training periods are > 0.",__FILE__,__FUNCTION__,__LINE__, symbol.c_str());
    return false;
  }

  vector<double> vvPrelimAvgPnLThd;
  vector<double> vvPrelimSharpeThd;
  vector<double> vvPrelimSquaredErrorThd;

  mapBestParamSetBeta1Beta3.clear();
  mapBestParamSetBeta2Beta4.clear();

  //--------------------------------------------------
  double dTrainingPeriodToUse = trainingperiod2;
  m_Logger->Write(Logger::INFO,"StrategyB2: %s Using training period %f.", ymdhms.ToStr().c_str(),dTrainingPeriodToUse);

  //--------------------------------------------------
  double dCoefSMA1LowerBound = m_b1_low;
  double dCoefSMA1UpperBound = m_b1_high;
  double dCoefSMA1Adj        = m_b1_adj;
  double dCoefSMA2LowerBound = m_b2_low;
  double dCoefSMA2UpperBound = m_b2_high;
  double dCoefSMA2Adj        = m_b2_adj;
  int    iPrdSMA1LowerBound  = m_b3_low;
  int    iPrdSMA1UpperBound  = m_b3_high;
  int    iPrdSMA1Adj         = m_b3_adj;
  int    iPrdSMA2LowerBound  = m_b4_low;
  int    iPrdSMA2UpperBound  = m_b4_high;
  int    iPrdSMA2Adj         = m_b4_adj;

  for (double dCoef1 = dCoefSMA1LowerBound; dCoef1 <= dCoefSMA1UpperBound; dCoef1 += dCoefSMA1Adj)
  {

    for (double dCoef2 = dCoefSMA2LowerBound; dCoef2 <= dCoefSMA2UpperBound; dCoef2 += dCoefSMA2Adj)
    {

      for (int iPrd1 = iPrdSMA1LowerBound; iPrd1 <= iPrdSMA1UpperBound; iPrd1 += iPrdSMA1Adj)
      {

        vvPrelimAvgPnLThd.clear();
        vvPrelimSharpeThd.clear();
        vvPrelimSquaredErrorThd.clear();

        for (int iPrd2 = iPrdSMA2LowerBound; iPrd2 <= iPrdSMA2UpperBound; iPrd2 += iPrdSMA2Adj)
        {
          vvPrelimAvgPnLThd.push_back(NAN);
          vvPrelimSharpeThd.push_back(NAN);
          vvPrelimSquaredErrorThd.push_back(NAN);
        }

        //--------------------------------------------------
        int iCnt = 0;
        boost::thread_group thdgrp;

        for (int iPrd2 = iPrdSMA2LowerBound; iPrd2 <= iPrdSMA2UpperBound; iPrd2 += iPrdSMA2Adj)
        {
          GetEstParam gep;

          gep.beta_1            =   dCoef1;
          gep.beta_2            =   dCoef2;
          gep.beta_3            =   iPrd1;
          gep.beta_4            =   iPrd2;

          gep.trainingperiod    =   dTrainingPeriodToUse;
          gep.v_hist_trfpx      =   &v_hist_trfpx;
          gep.v_hist_close      =   &v_hist_close;
          gep.pdAvgSquaredError =   &vvPrelimSquaredErrorThd[iCnt];
          gep.pdAvgPnL          =   &vvPrelimAvgPnLThd      [iCnt];
          gep.pdSharpe          =   &vvPrelimSharpeThd      [iCnt];
          gep.pdFinalEstimate   =   NULL;
          gep.ws                =   ws;
          gep.curbInSmpReturn   =   curbInSmpReturn;

          iCnt++;

          thdgrp.add_thread(new boost::thread(boost::bind(&StrategyB2::GetEstimate, this, gep)));
        }

        thdgrp.join_all();

        //--------------------------------------------------
        iCnt = 0;
        for (int iPrd2 = iPrdSMA2LowerBound; iPrd2 <= iPrdSMA2UpperBound; iPrd2 += iPrdSMA2Adj)
        {
          vector<double> vTmp1;
          vector<double> vTmp2;
          vTmp1.push_back(dCoef1);
          vTmp1.push_back(iPrd1);
          vTmp2.push_back(dCoef2);
          vTmp2.push_back(iPrd2);
          if      (tm == TM_MINSQERR)  { mapBestParamSetBeta1Beta3[vvPrelimSquaredErrorThd[iCnt]] = vTmp1; mapBestParamSetBeta2Beta4[vvPrelimSquaredErrorThd[iCnt]] = vTmp2; }
          else if (tm == TM_MAXPROFIT) { mapBestParamSetBeta1Beta3[vvPrelimAvgPnLThd      [iCnt]] = vTmp1; mapBestParamSetBeta2Beta4[vvPrelimAvgPnLThd      [iCnt]] = vTmp2; }
          else if (tm == TM_MAXSHARPE) { mapBestParamSetBeta1Beta3[vvPrelimSharpeThd      [iCnt]] = vTmp1; mapBestParamSetBeta2Beta4[vvPrelimSharpeThd      [iCnt]] = vTmp2; }

          m_Logger->Write(Logger::DEBUG,"StrategyB2: %s %s Looped parameters: beta_1 = %f, beta_2 = %f, beta_3 = %d, beta_4 = %d, vvPrelimSquaredErrorThd %f vvPrelimAvgPnLThd %f vvPrelimSharpeThd %f",
                          ymdhms.ToStr().c_str(),
                          symbol.c_str(),
                          dCoef1,dCoef2,iPrd1,iPrd2,
                          vvPrelimSquaredErrorThd[iCnt],
                          vvPrelimAvgPnLThd[iCnt],
                          vvPrelimSharpeThd[iCnt]
                         );

          iCnt++;
        }
      }
    }
  }

  if (!mapBestParamSetBeta1Beta3.empty())
  {
    map<double,vector<double> >::iterator it_prelim;
    if (tm == TM_MINSQERR)
    {
      it_prelim = mapBestParamSetBeta1Beta3.begin();
    }
    else if (tm == TM_MAXPROFIT)
    {
      it_prelim = mapBestParamSetBeta1Beta3.end();
      --it_prelim;
    }
    else if (tm == TM_MAXSHARPE)
    {
      it_prelim = mapBestParamSetBeta1Beta3.end();
      --it_prelim;
    }
    beta_1  = it_prelim->second[0];
    beta_3  = it_prelim->second[1];
    m_Logger->Write(Logger::INFO,"StrategyB2: %s Updated to the best parameters after looping: beta_1 = %f, beta_3 = %f.", ymdhms.ToStr().c_str(),beta_1,beta_3);
    m_Logger->Write(Logger::INFO,"StrategyB2: %s Num of best parameters in mapBestParamSetBeta1Beta3: %d", ymdhms.ToStr().c_str(),mapBestParamSetBeta1Beta3.size());
  }
  else
  {
    m_Logger->Write(Logger::INFO,"StrategyB2: %s Parameters remain unchanged: beta_1 = %f, beta_3 = %f.", ymdhms.ToStr().c_str(),beta_1,beta_3);
    m_Logger->Write(Logger::INFO,"StrategyB2: %s Num of best parameters in mapBestParamSetBeta1Beta3: %d", ymdhms.ToStr().c_str(),mapBestParamSetBeta1Beta3.size());
  }

  if (!mapBestParamSetBeta2Beta4.empty())
  {
    map<double,vector<double> >::iterator it_prelim;
    if (tm == TM_MINSQERR)
    {
      it_prelim = mapBestParamSetBeta2Beta4.begin();
    }
    else if (tm == TM_MAXPROFIT)
    {
      it_prelim = mapBestParamSetBeta2Beta4.end();
      --it_prelim;
    }
    else if (tm == TM_MAXSHARPE)
    {
      it_prelim = mapBestParamSetBeta2Beta4.end();
      --it_prelim;
    }
    beta_2  = it_prelim->second[0];
    beta_4  = it_prelim->second[1];
    m_Logger->Write(Logger::INFO,"StrategyB2: %s Updated to the best parameters after looping: beta_2 = %f, beta_4 = %f.", ymdhms.ToStr().c_str(),beta_2,beta_4);
    m_Logger->Write(Logger::INFO,"StrategyB2: %s Num of best parameters in mapBestParamSetBeta2Beta4: %d", ymdhms.ToStr().c_str(),mapBestParamSetBeta2Beta4.size());
  }
  else
  {
    m_Logger->Write(Logger::INFO,"StrategyB2: %s Parameters remain unchanged: beta_2 = %f, beta_4 = %f.", ymdhms.ToStr().c_str(),beta_2,beta_4);
    m_Logger->Write(Logger::INFO,"StrategyB2: %s Num of best parameters in mapBestParamSetBeta2Beta4: %d", ymdhms.ToStr().c_str(),mapBestParamSetBeta2Beta4.size());
  }


  return true;
}

void StrategyB2::SetParamBetaRange(
  const double b1_low,
  const double b1_high,
  const double b1_adj,
  const double b2_low,
  const double b2_high,
  const double b2_adj,
  const double b3_low,
  const double b3_high,
  const double b3_adj,
  const double b4_low,
  const double b4_high,
  const double b4_adj
  )
{
  m_b1_low  = b1_low;
  m_b1_high = b1_high;
  m_b1_adj  = b1_adj;
  m_b2_low  = b2_low;
  m_b2_high = b2_high;
  m_b2_adj  = b2_adj;
  m_b3_low  = b3_low;
  m_b3_high = b3_high;
  m_b3_adj  = b3_adj;
  m_b4_low  = b4_low;
  m_b4_high = b4_high;
  m_b4_adj  = b4_adj;
  return;
}




//--------------------------------------------------


void StrategyB2::ReadParam()
{
  //--------------------------------------------------
  // Parameters
  //--------------------------------------------------
  m_TheoCPnLHistFolder = m_SysCfg->B2_TheoCPnLHistFolder(m_StyID);
  m_TheoPosFolder      = m_SysCfg->B2_TheoPosFolder     (m_StyID);
  m_PersistTheoPosCPnL = m_SysCfg->B2_PersistTheoPosCPnL(m_StyID);

  //--------------------------------------------------
  m_RotationMode = m_SysCfg->B2_HasEnabledRotationMode(m_StyID);
  m_Logger->Write(Logger::INFO,"Strategy %s: m_RotationMode %d", GetStrategyName(m_StyID).c_str(),  m_RotationMode);
  m_RotationModeTradeHighestReturn = m_SysCfg->B2_RotationModeTradeHighestReturn(m_StyID);
  m_Logger->Write(Logger::INFO,"Strategy %s: m_RotationModeTradeHighestReturn %s", GetStrategyName(m_StyID).c_str(), (m_RotationModeTradeHighestReturn?"true":"false"));
  m_RotationModeUseVolyAdjdReturn = m_SysCfg->B2_RotationModeUseVolyAdjdReturn(m_StyID);
  m_Logger->Write(Logger::INFO,"Strategy %s: m_RotationModeUseVolyAdjdReturn %s", GetStrategyName(m_StyID).c_str(), (m_RotationModeUseVolyAdjdReturn?"true":"false"));
  m_RotationModeUseNDayReturn = m_SysCfg->B2_RotationModeUseNDayReturn(m_StyID);
  m_Logger->Write(Logger::INFO,"Strategy %s: m_RotationModeUseNDayReturn %d", GetStrategyName(m_StyID).c_str(), m_RotationModeUseNDayReturn);

  m_LongOnlyWhenClosePriceBelowAvgPrice = m_SysCfg->B2_LongOnlyWhenClosePriceBelowAvgPrice(m_StyID);
  if (m_LongOnlyWhenClosePriceBelowAvgPrice.IsSome())
    m_Logger->Write(Logger::INFO,"Strategy %s: m_LongOnlyWhenClosePriceBelowAvgPrice %f", GetStrategyName(m_StyID).c_str(), m_LongOnlyWhenClosePriceBelowAvgPrice.Get());
  else
    m_Logger->Write(Logger::INFO,"Strategy %s: m_LongOnlyWhenClosePriceBelowAvgPrice: Nil", GetStrategyName(m_StyID).c_str());

  m_ShortOnlyWhenClosePriceAboveAvgPrice = m_SysCfg->B2_ShortOnlyWhenClosePriceAboveAvgPrice(m_StyID);
  if (m_ShortOnlyWhenClosePriceAboveAvgPrice.IsSome())
    m_Logger->Write(Logger::INFO,"Strategy %s: m_ShortOnlyWhenClosePriceAboveAvgPrice %f", GetStrategyName(m_StyID).c_str(), m_ShortOnlyWhenClosePriceAboveAvgPrice.Get());
  else
    m_Logger->Write(Logger::INFO,"Strategy %s: m_ShortOnlyWhenClosePriceAboveAvgPrice: Nil", GetStrategyName(m_StyID).c_str());

  m_LongOnlyWhenAvgPriceReturnAbove = m_SysCfg->B2_LongOnlyWhenAvgPriceReturnAbove(m_StyID);
  if (m_LongOnlyWhenAvgPriceReturnAbove.IsSome())
    m_Logger->Write(Logger::INFO,"Strategy %s: m_LongOnlyWhenAvgPriceReturnAbove %f", GetStrategyName(m_StyID).c_str(), m_LongOnlyWhenAvgPriceReturnAbove.Get());
  else
    m_Logger->Write(Logger::INFO,"Strategy %s: m_LongOnlyWhenAvgPriceReturnAbove: Nil", GetStrategyName(m_StyID).c_str());

  m_LongOnlyWhenGrpAvgReturnAbove = m_SysCfg->B2_LongOnlyWhenGrpAvgReturnAbove(m_StyID);
  if (m_LongOnlyWhenGrpAvgReturnAbove.IsSome())
    m_Logger->Write(Logger::INFO,"Strategy %s: m_LongOnlyWhenGrpAvgReturnAbove %f", GetStrategyName(m_StyID).c_str(), m_LongOnlyWhenGrpAvgReturnAbove.Get());
  else
    m_Logger->Write(Logger::INFO,"Strategy %s: m_LongOnlyWhenGrpAvgReturnAbove: Nil", GetStrategyName(m_StyID).c_str());

  m_ShortOnlyWhenAvgPriceReturnBelow = m_SysCfg->B2_ShortOnlyWhenAvgPriceReturnBelow(m_StyID);
  if (m_ShortOnlyWhenAvgPriceReturnBelow.IsSome())
    m_Logger->Write(Logger::INFO,"Strategy %s: m_ShortOnlyWhenAvgPriceReturnBelow %f", GetStrategyName(m_StyID).c_str(), m_ShortOnlyWhenAvgPriceReturnBelow.Get());
  else
    m_Logger->Write(Logger::INFO,"Strategy %s: m_ShortOnlyWhenAvgPriceReturnBelow: Nil", GetStrategyName(m_StyID).c_str());

  m_Logger->Write(Logger::INFO,"Strategy %s: B2_CommissionRateThreshold %f", GetStrategyName(m_StyID).c_str(), m_SysCfg->B2_CommissionRateThreshold(m_StyID));

  if (m_RotationMode != 0)
  {
    m_ChooseBestNRotationGroup = m_SysCfg->Get_B2_ChooseBestNRotationGroup(m_StyID);
    m_Logger->Write(Logger::INFO,"Strategy %s: m_ChooseBestNRotationGroup %d", GetStrategyName(m_StyID).c_str(),  m_ChooseBestNRotationGroup);

    m_AvgAggSgndNotnlThresh = m_SysCfg->Get_B2_AvgAggSgndNotnlThresh(m_StyID);
    m_Logger->Write(Logger::INFO,"Strategy %s: m_AvgAggSgndNotnlThresh %f", GetStrategyName(m_StyID).c_str(),  m_AvgAggSgndNotnlThresh);

    m_MoveNextBestGroupUpIfNoSignal = m_SysCfg->Get_B2_MoveNextBestGroupUpIfNoSignal(m_StyID);
    m_Logger->Write(Logger::INFO,"Strategy %s: m_MoveNextBestGroupUpIfNoSignal %s", GetStrategyName(m_StyID).c_str(),  (m_MoveNextBestGroupUpIfNoSignal?"true":"false"));

    m_MoveNextBestStkInGrpUpIfNoSignal = m_SysCfg->Get_B2_MoveNextBestStkInGrpUpIfNoSignal(m_StyID);
    m_Logger->Write(Logger::INFO,"Strategy %s: m_MoveNextBestStkInGrpUpIfNoSignal %s", GetStrategyName(m_StyID).c_str(),  (m_MoveNextBestStkInGrpUpIfNoSignal?"true":"false"));

    m_RotationGroup = m_SysCfg->Get_B2_RotationGroup(m_StyID);
    FForEach (m_RotationGroup,[&](const int gid) { m_Logger->Write(Logger::INFO,"Strategy %s: m_RotationGroup %d", GetStrategyName(m_StyID).c_str(), gid); });

    m_vRoleOfSym = m_SysCfg->Get_B2_RoleOfSym(m_StyID);
    FForEach (m_vRoleOfSym,[&](const int i) { m_Logger->Write(Logger::INFO,"Strategy %s: m_vRoleOfSym %d", GetStrategyName(m_StyID).c_str(), i); });
    for (int i = 0; i < m_RotationGroup.back()+1; ++i) m_vGroupRepresentative.push_back(Option<string>());
    for (int i = 0; i < m_TradedSymbols.size(); ++i)
    {
      m_mapRoleOfSym[m_TradedSymbols[i]] = m_vRoleOfSym[i];
      if (m_vRoleOfSym[i] == 2) m_vGroupRepresentative[m_RotationGroup[i]] = Option<string>(m_TradedSymbols[i]);
    }

    for(map<string,int>::iterator it = m_mapRoleOfSym.begin(); it != m_mapRoleOfSym.end(); ++it)
    {
      m_Logger->Write(Logger::INFO,"Strategy %s: m_mapRoleOfSym %s %d",
                      GetStrategyName(m_StyID).c_str(),
                      it->first.c_str(),
                      it->second);
    }
    for (int i = 0; i < m_vGroupRepresentative.size(); ++i)
    {
      m_Logger->Write(Logger::INFO,"Strategy %s: Group %d m_vGroupRepresentative %s",
                      GetStrategyName(m_StyID).c_str(),
                      i,
                      m_vGroupRepresentative[i].GetOrElse("---").c_str());
    }
  }
  else
  {
    m_Logger->Write(Logger::INFO,"Strategy %s: Any m_ChooseBestNRotationGroup settings are ignored", GetStrategyName(m_StyID).c_str());
    m_Logger->Write(Logger::INFO,"Strategy %s: Any m_AvgAggSgndNotnlThresh settings are ignored", GetStrategyName(m_StyID).c_str());
    m_Logger->Write(Logger::INFO,"Strategy %s: Any m_RotationGroup settings are ignored", GetStrategyName(m_StyID).c_str());
    m_Logger->Write(Logger::INFO,"Strategy %s: Any m_vRoleOfSym settings are ignored", GetStrategyName(m_StyID).c_str());
    m_Logger->Write(Logger::INFO,"Strategy %s: Any m_MoveNextBestGroupUpIfNoSignal settings are ignored", GetStrategyName(m_StyID).c_str());
    m_Logger->Write(Logger::INFO,"Strategy %s: Any m_MoveNextBestStkInGrpUpIfNoSignal settings are ignored", GetStrategyName(m_StyID).c_str());
  }


  if (!m_RotationGroup.empty())
  {
    const int iLastRotGrp = m_RotationGroup.back();

    m_AllAvbSymForRollingBasket .insert(m_AllAvbSymForRollingBasket .begin(), iLastRotGrp+1, map<YYYYMMDD,set<string> >        () );
    m_SymRankedByRollingReturn  .insert(m_SymRankedByRollingReturn  .begin(), iLastRotGrp+1, map<YYYYMMDD,vector<TupRetSym> >  () );
    m_SymAndRollingReturn       .insert(m_SymAndRollingReturn       .begin(), iLastRotGrp+1, map<YYYYMMDD,map<string,double> > () );
    m_MaintainPosWithinGrp      .insert(m_MaintainPosWithinGrp      .begin(), iLastRotGrp+1, map<YYYYMMDD,set<string> >        () );
  }
  //--------------------------------------------------

  m_B2_HasEnabledMinCommissionCheck = m_SysCfg->B2_HasEnabledMinCommissionCheck(m_StyID);
  m_B2_WhetherToRetain              = m_SysCfg->GetWhetherToRetrain(m_StyID);
  m_B2_TrainingFreq                 = m_SysCfg->GetTrainingFreq(m_StyID);
  m_B2_ActionTimeBefCloseInSec      = m_SysCfg->Get_B2_ActionTimeBefCloseInSec(m_StyID);
  m_B2_FilterSMAPeriod              = m_SysCfg->Get_B2_FilterSMAPeriod(m_StyID);

  m_Logger->Write(Logger::INFO,"Strategy %s: m_B2_HasEnabledMinCommissionCheck %s", GetStrategyName(m_StyID).c_str(), (m_B2_HasEnabledMinCommissionCheck?"true":"false"));
  m_Logger->Write(Logger::INFO,"Strategy %s: m_B2_WhetherToRetain              %s", GetStrategyName(m_StyID).c_str(), (m_B2_WhetherToRetain             ?"true":"false"));
  m_Logger->Write(Logger::INFO,"Strategy %s: m_B2_TrainingFreq                 %d", GetStrategyName(m_StyID).c_str(),  m_B2_TrainingFreq                                );
  m_Logger->Write(Logger::INFO,"Strategy %s: m_B2_ActionTimeBefCloseInSec      %d", GetStrategyName(m_StyID).c_str(),  m_B2_ActionTimeBefCloseInSec                     );

  FForEach(m_B2_FilterSMAPeriod,[&](const int iSma) { m_Logger->Write(Logger::INFO,"Strategy %s: m_B2_FilterSMAPeriod %d", GetStrategyName(m_StyID).c_str(), iSma); });

  // //--------------------------------------------------
  // // Special ES + SPY mode
  // //--------------------------------------------------
  // int iSPYIdx  = -1;
  // int iESc1Idx = -1;
  // for (unsigned int i = 0; i < m_TradedSymbols.size(); ++i)
  // {
  //   if      (m_TradedSymbols[i] == ES_CONT_FUT_1) iESc1Idx = i;
  //   else if (m_TradedSymbols[i] == "SPY"        ) iSPYIdx  = i;
  // }
  // if (iSPYIdx >= 0 && iESc1Idx >= 0) m_ES_SPY_Together_SPYidx = iSPYIdx;
  // //--------------------------------------------------

  m_beta_1           .insert(m_beta_1           .begin(), m_TradedSymbols.size(),0);
  m_beta_2           .insert(m_beta_2           .begin(), m_TradedSymbols.size(),0);
  m_beta_3           .insert(m_beta_3           .begin(), m_TradedSymbols.size(),0);
  m_beta_4           .insert(m_beta_4           .begin(), m_TradedSymbols.size(),0);

  m_NotionalAmt      .insert(m_NotionalAmt      .begin(), m_TradedSymbols.size(),0);
  m_PriceMode        .insert(m_PriceMode        .begin(), m_TradedSymbols.size(),0);
  m_CurbInSmpRtn     .insert(m_CurbInSmpRtn     .begin(), m_TradedSymbols.size(),0);
  m_AllowTrdDir      .insert(m_AllowTrdDir      .begin(), m_TradedSymbols.size(),0);
  m_KSBound          .insert(m_KSBound          .begin(), m_TradedSymbols.size(),0);
  m_KSRange          .insert(m_KSRange          .begin(), m_TradedSymbols.size(),0);
  m_MinAnnVolPsnSz   .insert(m_MinAnnVolPsnSz   .begin(), m_TradedSymbols.size(),0);
  m_TrainingPeriod1  .insert(m_TrainingPeriod1  .begin(), m_TradedSymbols.size(),0);
  m_TrainingPeriod2  .insert(m_TrainingPeriod2  .begin(), m_TradedSymbols.size(),0);
  m_TrainingPeriod3  .insert(m_TrainingPeriod3  .begin(), m_TradedSymbols.size(),0);
  m_TrainingPeriod4  .insert(m_TrainingPeriod4  .begin(), m_TradedSymbols.size(),0);
  m_TrainingPeriodMax.insert(m_TrainingPeriodMax.begin(), m_TradedSymbols.size(),0);
  m_beta_1_start     .insert(m_beta_1_start     .begin(), m_TradedSymbols.size(),0);
  m_beta_1_end       .insert(m_beta_1_end       .begin(), m_TradedSymbols.size(),0);
  m_beta_1_incremt   .insert(m_beta_1_incremt   .begin(), m_TradedSymbols.size(),0);
  m_beta_2_start     .insert(m_beta_2_start     .begin(), m_TradedSymbols.size(),0);
  m_beta_2_end       .insert(m_beta_2_end       .begin(), m_TradedSymbols.size(),0);
  m_beta_2_incremt   .insert(m_beta_2_incremt   .begin(), m_TradedSymbols.size(),0);
  m_beta_3_start     .insert(m_beta_3_start     .begin(), m_TradedSymbols.size(),0);
  m_beta_3_end       .insert(m_beta_3_end       .begin(), m_TradedSymbols.size(),0);
  m_beta_3_incremt   .insert(m_beta_3_incremt   .begin(), m_TradedSymbols.size(),0);
  m_beta_4_start     .insert(m_beta_4_start     .begin(), m_TradedSymbols.size(),0);
  m_beta_4_end       .insert(m_beta_4_end       .begin(), m_TradedSymbols.size(),0);
  m_beta_4_incremt   .insert(m_beta_4_incremt   .begin(), m_TradedSymbols.size(),0);
  m_PropOfBestParam  .insert(m_PropOfBestParam  .begin(), m_TradedSymbols.size(),0);
  m_WeightScheme     .insert(m_WeightScheme     .begin(), m_TradedSymbols.size(),WS_UNIFORM);


  for (unsigned int i = 0; i < m_TradedSymbols.size(); ++i)
  {
    m_NotionalAmt    [i] = m_ParamVector[i*m_iNumOfParam + 0];
    m_PriceMode      [i] = m_ParamVector[i*m_iNumOfParam + 1];
    m_CurbInSmpRtn   [i] = m_ParamVector[i*m_iNumOfParam + 2];
    m_AllowTrdDir    [i] = m_ParamVector[i*m_iNumOfParam + 3];
    m_KSBound        [i] = m_ParamVector[i*m_iNumOfParam + 4];
    m_KSRange        [i] = m_ParamVector[i*m_iNumOfParam + 5];
    m_MinAnnVolPsnSz [i] = m_ParamVector[i*m_iNumOfParam + 6];
    m_TrainingPeriod1[i] = m_ParamVector[i*m_iNumOfParam + 7];
    m_TrainingPeriod2[i] = m_ParamVector[i*m_iNumOfParam + 8];
    m_TrainingPeriod3[i] = m_ParamVector[i*m_iNumOfParam + 9];
    m_TrainingPeriod4[i] = m_ParamVector[i*m_iNumOfParam +10];
    m_beta_1_start   [i] = m_ParamVector[i*m_iNumOfParam +11];
    m_beta_1_end     [i] = m_ParamVector[i*m_iNumOfParam +12];
    m_beta_1_incremt [i] = m_ParamVector[i*m_iNumOfParam +13];
    m_beta_2_start   [i] = m_ParamVector[i*m_iNumOfParam +14];
    m_beta_2_end     [i] = m_ParamVector[i*m_iNumOfParam +15];
    m_beta_2_incremt [i] = m_ParamVector[i*m_iNumOfParam +16];
    m_beta_3_start   [i] = m_ParamVector[i*m_iNumOfParam +17];
    m_beta_3_end     [i] = m_ParamVector[i*m_iNumOfParam +18];
    m_beta_3_incremt [i] = m_ParamVector[i*m_iNumOfParam +19];
    m_beta_4_start   [i] = m_ParamVector[i*m_iNumOfParam +20];
    m_beta_4_end     [i] = m_ParamVector[i*m_iNumOfParam +21];
    m_beta_4_incremt [i] = m_ParamVector[i*m_iNumOfParam +22];
    m_PropOfBestParam[i] = m_ParamVector[i*m_iNumOfParam +23];
    m_WeightScheme   [i] =(m_ParamVector[i*m_iNumOfParam +24] == 1 ? WS_UNIFORM : WS_LINEAR);

    m_Logger->Write(Logger::INFO,"Strategy %s: %s m_NotionalAmt      %f", GetStrategyName(m_StyID).c_str(), m_TradedSymbols[i].c_str(), m_NotionalAmt    [i]);
    m_Logger->Write(Logger::INFO,"Strategy %s: %s m_PriceMode        %d", GetStrategyName(m_StyID).c_str(), m_TradedSymbols[i].c_str(), m_PriceMode      [i]);
    m_Logger->Write(Logger::INFO,"Strategy %s: %s m_CurbInSmpRtn     %f", GetStrategyName(m_StyID).c_str(), m_TradedSymbols[i].c_str(), m_CurbInSmpRtn   [i]);
    m_Logger->Write(Logger::INFO,"Strategy %s: %s m_AllowTrdDir      %d", GetStrategyName(m_StyID).c_str(), m_TradedSymbols[i].c_str(), m_AllowTrdDir    [i]);
    m_Logger->Write(Logger::INFO,"Strategy %s: %s m_KSBound          %f", GetStrategyName(m_StyID).c_str(), m_TradedSymbols[i].c_str(), m_KSBound        [i]);
    m_Logger->Write(Logger::INFO,"Strategy %s: %s m_KSRange          %f", GetStrategyName(m_StyID).c_str(), m_TradedSymbols[i].c_str(), m_KSRange        [i]);
    m_Logger->Write(Logger::INFO,"Strategy %s: %s m_MinAnnVolPsnSz   %f", GetStrategyName(m_StyID).c_str(), m_TradedSymbols[i].c_str(), m_MinAnnVolPsnSz [i]);
    m_Logger->Write(Logger::INFO,"Strategy %s: %s m_TrainingPeriod1  %f", GetStrategyName(m_StyID).c_str(), m_TradedSymbols[i].c_str(), m_TrainingPeriod1[i]);
    m_Logger->Write(Logger::INFO,"Strategy %s: %s m_TrainingPeriod2  %f", GetStrategyName(m_StyID).c_str(), m_TradedSymbols[i].c_str(), m_TrainingPeriod2[i]);
    m_Logger->Write(Logger::INFO,"Strategy %s: %s m_TrainingPeriod3  %f", GetStrategyName(m_StyID).c_str(), m_TradedSymbols[i].c_str(), m_TrainingPeriod3[i]);
    m_Logger->Write(Logger::INFO,"Strategy %s: %s m_TrainingPeriod4  %f", GetStrategyName(m_StyID).c_str(), m_TradedSymbols[i].c_str(), m_TrainingPeriod4[i]);
    m_Logger->Write(Logger::INFO,"Strategy %s: %s m_beta_1_start     %f", GetStrategyName(m_StyID).c_str(), m_TradedSymbols[i].c_str(), m_beta_1_start   [i]);
    m_Logger->Write(Logger::INFO,"Strategy %s: %s m_beta_1_end       %f", GetStrategyName(m_StyID).c_str(), m_TradedSymbols[i].c_str(), m_beta_1_end     [i]);
    m_Logger->Write(Logger::INFO,"Strategy %s: %s m_beta_1_incremt   %f", GetStrategyName(m_StyID).c_str(), m_TradedSymbols[i].c_str(), m_beta_1_incremt [i]);
    m_Logger->Write(Logger::INFO,"Strategy %s: %s m_beta_2_start     %f", GetStrategyName(m_StyID).c_str(), m_TradedSymbols[i].c_str(), m_beta_2_start   [i]);
    m_Logger->Write(Logger::INFO,"Strategy %s: %s m_beta_2_end       %f", GetStrategyName(m_StyID).c_str(), m_TradedSymbols[i].c_str(), m_beta_2_end     [i]);
    m_Logger->Write(Logger::INFO,"Strategy %s: %s m_beta_2_incremt   %f", GetStrategyName(m_StyID).c_str(), m_TradedSymbols[i].c_str(), m_beta_2_incremt [i]);
    m_Logger->Write(Logger::INFO,"Strategy %s: %s m_beta_3_start     %f", GetStrategyName(m_StyID).c_str(), m_TradedSymbols[i].c_str(), m_beta_3_start   [i]);
    m_Logger->Write(Logger::INFO,"Strategy %s: %s m_beta_3_end       %f", GetStrategyName(m_StyID).c_str(), m_TradedSymbols[i].c_str(), m_beta_3_end     [i]);
    m_Logger->Write(Logger::INFO,"Strategy %s: %s m_beta_3_incremt   %f", GetStrategyName(m_StyID).c_str(), m_TradedSymbols[i].c_str(), m_beta_3_incremt [i]);
    m_Logger->Write(Logger::INFO,"Strategy %s: %s m_beta_4_start     %f", GetStrategyName(m_StyID).c_str(), m_TradedSymbols[i].c_str(), m_beta_4_start   [i]);
    m_Logger->Write(Logger::INFO,"Strategy %s: %s m_beta_4_end       %f", GetStrategyName(m_StyID).c_str(), m_TradedSymbols[i].c_str(), m_beta_4_end     [i]);
    m_Logger->Write(Logger::INFO,"Strategy %s: %s m_beta_4_incremt   %f", GetStrategyName(m_StyID).c_str(), m_TradedSymbols[i].c_str(), m_beta_4_incremt [i]);
    m_Logger->Write(Logger::INFO,"Strategy %s: %s m_PropOfBestParam  %f", GetStrategyName(m_StyID).c_str(), m_TradedSymbols[i].c_str(), m_PropOfBestParam[i]);
    m_Logger->Write(Logger::INFO,"Strategy %s: %s m_WeightScheme     %d", GetStrategyName(m_StyID).c_str(), m_TradedSymbols[i].c_str(), m_WeightScheme   [i]);
  }

  //--------------------------------------------------

  FForEach (m_TradedSymbols,[&](const string & sym) {
    m_PTask_PrintStyActionTimeToken[sym] = m_PTask_PrintStyActionTime.GetNewTokenAndSetIntervalInSec(60);
  });
  m_PTask_PrintAllAvbSymToken = m_PTask_PrintAllAvbSym.GetNewTokenAndSetIntervalInSec(300);

  if (!m_B2_FilterSMAPeriod.empty())
  {
    if (m_B2_FilterSMAPeriod.size() == 2)
    {
      if (m_B2_FilterSMAPeriod[0] > 0)
      {
        for (unsigned int i = 0; i < m_TradedSymbols.size(); ++i)
          m_v_SMA_short.push_back(Sma<double>(m_B2_FilterSMAPeriod[0],true));
      }
      if (m_B2_FilterSMAPeriod[1] > 0)
      {
        for (unsigned int i = 0; i < m_TradedSymbols.size(); ++i)
          m_v_SMA_long.push_back(Sma<double>(m_B2_FilterSMAPeriod[1],true));
      }
    }
    else if (m_B2_FilterSMAPeriod.size() == 1)
    {
      if (m_B2_FilterSMAPeriod[0] > 0)
      {
        for (unsigned int i = 0; i < m_TradedSymbols.size(); ++i)
          m_v_SMA_long.push_back(Sma<double>(m_B2_FilterSMAPeriod[0],true));
      }
    }
  }

  for (int i = 0; i < m_TradedSymbols.size(); ++i)
  {
    m_TrainingPeriodMax[i] = m_TrainingPeriod1[i];
    if (m_TrainingPeriod2[i] > m_TrainingPeriodMax[i]) m_TrainingPeriodMax[i] = m_TrainingPeriod2[i];
    if (m_TrainingPeriod3[i] > m_TrainingPeriodMax[i]) m_TrainingPeriodMax[i] = m_TrainingPeriod3[i];
    if (m_TrainingPeriod4[i] > m_TrainingPeriodMax[i]) m_TrainingPeriodMax[i] = m_TrainingPeriod4[i];
  }

}

void StrategyB2::StartOfDayInit()
{
  for (unsigned int i = 0; i < m_TradedSymbols.size(); ++i)
  {
    m_map_DoneEODTrade[m_TradedSymbols[i]] = false;
  }

  // m_SPY_Px             = NAN;
  // m_NoOfSgndESCtrtReqd = 0;
  m_StkPicks.clear();
}

void StrategyB2::EndOfDayCleanUp()
{
  StartOfDayInit();
}

void StrategyB2::SetConvenienceVarb(const int iTradSym)
{

  {
    //--------------------------------------------------
    map<string,vector<double> >::iterator it1 = m_map_HistoricalAvgPx.find(m_TradedSymbols[iTradSym]);
    if (it1 == m_map_HistoricalAvgPx.end())
    {
      m_map_HistoricalAvgPx[m_TradedSymbols[iTradSym]] = vector<double>();
      it1 = m_map_HistoricalAvgPx.find(m_TradedSymbols[iTradSym]);
    }
    m_HistoricalAvgPx = &(it1->second);

    map<string,vector<double> >::iterator it2 = m_map_HistoricalClose.find(m_TradedSymbols[iTradSym]);
    if (it2 == m_map_HistoricalClose.end())
    {
      m_map_HistoricalClose[m_TradedSymbols[iTradSym]] = vector<double>();
      it2 = m_map_HistoricalClose.find(m_TradedSymbols[iTradSym]);
    }
    m_HistoricalClose = &(it2->second);
    //--------------------------------------------------
  }
  {
    //--------------------------------------------------
    map<string,vector<double> >::iterator it1 = m_map_HistoricalAvgPxRtn.find(m_TradedSymbols[iTradSym]);
    if (it1 == m_map_HistoricalAvgPxRtn.end())
    {
      m_map_HistoricalAvgPxRtn[m_TradedSymbols[iTradSym]] = vector<double>();
      it1 = m_map_HistoricalAvgPxRtn.find(m_TradedSymbols[iTradSym]);
    }
    m_HistoricalAvgPxRtn = &(it1->second);

    map<string,vector<double> >::iterator it2 = m_map_HistoricalCloseRtn.find(m_TradedSymbols[iTradSym]);
    if (it2 == m_map_HistoricalCloseRtn.end())
    {
      m_map_HistoricalCloseRtn[m_TradedSymbols[iTradSym]] = vector<double>();
      it2 = m_map_HistoricalCloseRtn.find(m_TradedSymbols[iTradSym]);
    }
    m_HistoricalCloseRtn = &(it2->second);
    //--------------------------------------------------
  }

  map<string,CountingFunction>::iterator it4a = m_map_HistoricalNumOfRisingDaysCountAvgPx.find(m_TradedSymbols[iTradSym]);
  if (it4a == m_map_HistoricalNumOfRisingDaysCountAvgPx.end())
  {
    m_map_HistoricalNumOfRisingDaysCountAvgPx[m_TradedSymbols[iTradSym]] = CountingFunction();
    it4a = m_map_HistoricalNumOfRisingDaysCountAvgPx.find(m_TradedSymbols[iTradSym]);
  }
  m_HistoricalNumNumOfRisingDaysCountAvgPx = &(it4a->second);

  map<string,CountingFunction>::iterator it4b = m_map_HistoricalNumOfRisingDaysCountClose.find(m_TradedSymbols[iTradSym]);
  if (it4b == m_map_HistoricalNumOfRisingDaysCountClose.end())
  {
    m_map_HistoricalNumOfRisingDaysCountClose[m_TradedSymbols[iTradSym]] = CountingFunction();
    it4b = m_map_HistoricalNumOfRisingDaysCountClose.find(m_TradedSymbols[iTradSym]);
  }
  m_HistoricalNumNumOfRisingDaysCountClose = &(it4b->second);

  //--------------------------------------------------

  map<string,map<double,vector<double> > >::iterator it5a1 = m_map_BestParamSetBeta1Beta3AvgPx_hypo1.find(m_TradedSymbols[iTradSym]);
  if (it5a1 == m_map_BestParamSetBeta1Beta3AvgPx_hypo1.end())
  {
    m_map_BestParamSetBeta1Beta3AvgPx_hypo1[m_TradedSymbols[iTradSym]] = map<double,vector<double> >();
    it5a1 = m_map_BestParamSetBeta1Beta3AvgPx_hypo1.find(m_TradedSymbols[iTradSym]);
  }
  m_p_map_BestParamSetBeta1Beta3AvgPx_hypo1 = &(it5a1->second);

  map<string,map<double,vector<double> > >::iterator it5a2 = m_map_BestParamSetBeta1Beta3AvgPx_hypo2.find(m_TradedSymbols[iTradSym]);
  if (it5a2 == m_map_BestParamSetBeta1Beta3AvgPx_hypo2.end())
  {
    m_map_BestParamSetBeta1Beta3AvgPx_hypo2[m_TradedSymbols[iTradSym]] = map<double,vector<double> >();
    it5a2 = m_map_BestParamSetBeta1Beta3AvgPx_hypo2.find(m_TradedSymbols[iTradSym]);
  }
  m_p_map_BestParamSetBeta1Beta3AvgPx_hypo2 = &(it5a2->second);

  map<string,map<double,vector<double> > >::iterator it6a1 = m_map_BestParamSetBeta2Beta4AvgPx_hypo1.find(m_TradedSymbols[iTradSym]);
  if (it6a1 == m_map_BestParamSetBeta2Beta4AvgPx_hypo1.end())
  {
    m_map_BestParamSetBeta2Beta4AvgPx_hypo1[m_TradedSymbols[iTradSym]] = map<double,vector<double> >();
    it6a1 = m_map_BestParamSetBeta2Beta4AvgPx_hypo1.find(m_TradedSymbols[iTradSym]);
  }
  m_p_map_BestParamSetBeta2Beta4AvgPx_hypo1 = &(it6a1->second);

  map<string,map<double,vector<double> > >::iterator it6a2 = m_map_BestParamSetBeta2Beta4AvgPx_hypo2.find(m_TradedSymbols[iTradSym]);
  if (it6a2 == m_map_BestParamSetBeta2Beta4AvgPx_hypo2.end())
  {
    m_map_BestParamSetBeta2Beta4AvgPx_hypo2[m_TradedSymbols[iTradSym]] = map<double,vector<double> >();
    it6a2 = m_map_BestParamSetBeta2Beta4AvgPx_hypo2.find(m_TradedSymbols[iTradSym]);
  }
  m_p_map_BestParamSetBeta2Beta4AvgPx_hypo2 = &(it6a2->second);

  map<string,map<double,vector<double> > >::iterator it5_1 = m_map_BestParamSetBeta1Beta3Close_hypo1.find(m_TradedSymbols[iTradSym]);
  if (it5_1 == m_map_BestParamSetBeta1Beta3Close_hypo1.end())
  {
    m_map_BestParamSetBeta1Beta3Close_hypo1[m_TradedSymbols[iTradSym]] = map<double,vector<double> >();
    it5_1 = m_map_BestParamSetBeta1Beta3Close_hypo1.find(m_TradedSymbols[iTradSym]);
  }
  m_p_map_BestParamSetBeta1Beta3Close_hypo1 = &(it5_1->second);

  map<string,map<double,vector<double> > >::iterator it5_2 = m_map_BestParamSetBeta1Beta3Close_hypo2.find(m_TradedSymbols[iTradSym]);
  if (it5_2 == m_map_BestParamSetBeta1Beta3Close_hypo2.end())
  {
    m_map_BestParamSetBeta1Beta3Close_hypo2[m_TradedSymbols[iTradSym]] = map<double,vector<double> >();
    it5_2 = m_map_BestParamSetBeta1Beta3Close_hypo2.find(m_TradedSymbols[iTradSym]);
  }
  m_p_map_BestParamSetBeta1Beta3Close_hypo2 = &(it5_2->second);

  map<string,map<double,vector<double> > >::iterator it6b_1 = m_map_BestParamSetBeta2Beta4Close_hypo1.find(m_TradedSymbols[iTradSym]);
  if (it6b_1 == m_map_BestParamSetBeta2Beta4Close_hypo1.end())
  {
    m_map_BestParamSetBeta2Beta4Close_hypo1[m_TradedSymbols[iTradSym]] = map<double,vector<double> >();
    it6b_1 = m_map_BestParamSetBeta2Beta4Close_hypo1.find(m_TradedSymbols[iTradSym]);
  }
  m_p_map_BestParamSetBeta2Beta4Close_hypo1 = &(it6b_1->second);

  map<string,map<double,vector<double> > >::iterator it6b_2 = m_map_BestParamSetBeta2Beta4Close_hypo2.find(m_TradedSymbols[iTradSym]);
  if (it6b_2 == m_map_BestParamSetBeta2Beta4Close_hypo2.end())
  {
    m_map_BestParamSetBeta2Beta4Close_hypo2[m_TradedSymbols[iTradSym]] = map<double,vector<double> >();
    it6b_2 = m_map_BestParamSetBeta2Beta4Close_hypo2.find(m_TradedSymbols[iTradSym]);
  }
  m_p_map_BestParamSetBeta2Beta4Close_hypo2 = &(it6b_2->second);

  //--------------------------------------------------
  map<string,bool>::iterator it7 = m_map_DoneEODTrade.find(m_TradedSymbols[iTradSym]);
  if (it7 == m_map_DoneEODTrade.end())
  {
    m_map_DoneEODTrade[m_TradedSymbols[iTradSym]] = false;
    it7 = m_map_DoneEODTrade.find(m_TradedSymbols[iTradSym]);
  }
  m_DoneEODTrade = &(it7->second);

  map<string,bool>::iterator it8a = m_map_bTrainRetAvgPx.find(m_TradedSymbols[iTradSym]);
  if (it8a == m_map_bTrainRetAvgPx.end())
  {
    m_map_bTrainRetAvgPx[m_TradedSymbols[iTradSym]] = false;
    it8a = m_map_bTrainRetAvgPx.find(m_TradedSymbols[iTradSym]);
  }
  m_bTrainRetAvgPx = &(it8a->second);

  map<string,bool>::iterator it8b = m_map_bTrainRetClose.find(m_TradedSymbols[iTradSym]);
  if (it8b == m_map_bTrainRetClose.end())
  {
    m_map_bTrainRetClose[m_TradedSymbols[iTradSym]] = false;
    it8b = m_map_bTrainRetClose.find(m_TradedSymbols[iTradSym]);
  }
  m_bTrainRetClose = &(it8b->second);

  map<string,bool>::iterator it9a = m_map_bRisingRegimeAvgPx.find(m_TradedSymbols[iTradSym]);
  if (it9a == m_map_bRisingRegimeAvgPx.end())
  {
    m_map_bRisingRegimeAvgPx[m_TradedSymbols[iTradSym]] = false;
    it9a = m_map_bRisingRegimeAvgPx.find(m_TradedSymbols[iTradSym]);
  }
  m_bRisingRegimeAvgPx = &(it9a->second);

  map<string,bool>::iterator it9b = m_map_bRisingRegimeClose.find(m_TradedSymbols[iTradSym]);
  if (it9b == m_map_bRisingRegimeClose.end())
  {
    m_map_bRisingRegimeClose[m_TradedSymbols[iTradSym]] = false;
    it9b = m_map_bRisingRegimeClose.find(m_TradedSymbols[iTradSym]);
  }
  m_bRisingRegimeClose = &(it9b->second);
}


bool StrategyB2::SkipSubseqProcessingForSymbol(const int iTradSym, string & sComment)
{
  sComment = "m_DoneEODTrade = " + string(*m_DoneEODTrade?"true":"false");

  HHMMSS hmsStart = m_Exchg->GetTimeNSecBefClose(m_TradedSymbols[iTradSym],m_ymdhms_SysTime_HKT.GetYYYYMMDD(),m_B2_ActionTimeBefCloseInSec);
  HHMMSS hmsEnd   = m_Exchg->GetTimeNSecBefClose(m_TradedSymbols[iTradSym],m_ymdhms_SysTime_HKT.GetYYYYMMDD(),0);

  if (m_PTask_PrintStyActionTime.CheckIfItIsTimeToWakeUp(m_PTask_PrintStyActionTimeToken[m_TradedSymbols[iTradSym]],m_ymdhms_SysTime_HKT))
  {
    m_Logger->Write(Logger::INFO,"Strategy %s: Sym=%s Current time %s Action time between %s and %s",
                    GetStrategyName(m_StyID).c_str(),
                    m_TradedSymbols[iTradSym].c_str(),
                    m_ymdhms_SysTime_HKT.GetHHMMSS().ToStr_().c_str(),
                    hmsStart.ToStr_().c_str(),
                    hmsEnd.ToStr_().c_str()
                   );
  }

  if (*m_DoneEODTrade) return true;
  else if
    (
      m_ymdhms_SysTime_HKT.GetHHMMSS() < hmsStart ||
      m_ymdhms_SysTime_HKT.GetHHMMSS() > hmsEnd
    ) return true;
  else return false;
}



void StrategyB2::InitialWarmUp(const int iTradSym)
{
  //--------------------------------------------------
  // TheoCPnLHist and TheoPos
  //--------------------------------------------------
  m_TheoCPnLHist.clear();
  for (unsigned int i = 0; i < m_TradedSymbols.size(); ++i)
  {
    m_TheoCPnLHist.push_back(CPnLHist());
    m_TheoAcct.push_back(Acct());

    if (m_PersistTheoPosCPnL)
    {
      if (!m_TheoCPnLHist.back().LoadFromFile(m_TheoCPnLHistFolder+"/"+m_TradedSymbols[iTradSym]))
      {
        sleep(1);
        exit(1);
      }

      if (!m_TheoAcct.back().LoadFromFile(m_TheoPosFolder+"/"+m_TradedSymbols[iTradSym]))
      {
        sleep(1);
        exit(1);
      }
    }
  }
  //--------------------------------------------------

  vector<YYYYMMDD> vYMD;
  vector<double>   vOpen;
  vector<double>   vHigh;
  vector<double>   vLow;
  vector<double>   vClose;
  vector<long>     vVol;

  YMD ymdStart(m_p_ymdhms_SysTime_Local->GetYYYYMMDD());
  ymdStart.SubtractDay(m_TrainingPeriodMax[iTradSym]*2); // times 2 since here we are subtracting calendar days

  //TODO MarketData::Instance()
  MarketData::Instance()->GetSuppD1BarOHLCVInDateRange(m_TradedSymbols[iTradSym], YYYYMMDD(ymdStart), m_p_ymdhms_SysTime_Local->GetYYYYMMDD(), vYMD, vOpen, vHigh, vLow, vClose, vVol);

  if (!vYMD.empty() && (m_p_ymdhms_SysTime_Local->GetYYYYMMDD() > vYMD.back() && m_p_ymdhms_SysTime_Local->GetYYYYMMDD() - vYMD.back() > 5))
  {
    m_Logger->Write(Logger::ERROR,"Strategy %s: Sym=%s Supplementary day bar may be out-of-date. Current date: %s. Last supp day bar date: %s.",
                    GetStrategyName(m_StyID).c_str(), m_TradedSymbols[iTradSym].c_str(), m_p_ymdhms_SysTime_Local->GetYYYYMMDD().ToStr_().c_str(), vYMD.back().ToStr_().c_str());
    // sleep(1);
    // exit(1);
  }

  if (vYMD.empty()) return;
  for (unsigned int i = 0; i < vYMD.size(); ++i)
  {
    m_Logger->Write(Logger::INFO,"Strategy %s: Sym=%s Read from day bar file: %s %s High = %f Low = %f Close = %f.",
                    GetStrategyName(m_StyID).c_str(), m_TradedSymbols[iTradSym].c_str(), vYMD[i].ToStr_().c_str(),m_TradedSymbols[iTradSym].c_str(), vHigh[i],vLow[i],vClose[i]);

    //--------------------------------------------------
    // skip today
    //--------------------------------------------------
    if (m_p_ymdhms_SysTime_Local->GetYYYYMMDD() == vYMD[i])
    {
      m_Logger->Write(Logger::INFO,"Strategy %s: Sym=%s Skipping day bar %s because it's the same as today.", GetStrategyName(m_StyID).c_str(), m_TradedSymbols[iTradSym].c_str(), vYMD[i].ToStr_().c_str());
      continue;
    }

    double dAvgPx = (vHigh[i] + vLow[i] + vClose[i]) / 3;
    if (!std::isnan(dAvgPx))
    {
      if (!m_HistoricalAvgPx->empty())   m_HistoricalNumNumOfRisingDaysCountAvgPx->Add(dAvgPx    >= m_HistoricalAvgPx->back());
      if (!m_HistoricalClose->empty())   m_HistoricalNumNumOfRisingDaysCountClose->Add(vClose[i] >= m_HistoricalClose->back());

      m_HistoricalAvgPx->push_back(dAvgPx);
      m_HistoricalClose->push_back(vClose[i]);

      //--------------------------------------------------
      {
        YYYYMMDD ymd = vYMD[i];
        map<YYYYMMDD,set<string> >::iterator it = m_AllSymWithUpdateData.find(ymd);
        if (it == m_AllSymWithUpdateData.end())
        {
          m_AllSymWithUpdateData[ymd] = set<string>();
          it = m_AllSymWithUpdateData.find(ymd);
        }
        it->second.insert(m_TradedSymbols[iTradSym]);
      }
      //--------------------------------------------------

      if (!m_B2_FilterSMAPeriod.empty())
      {
        if (m_B2_FilterSMAPeriod.size() == 2)
        {
          if (m_B2_FilterSMAPeriod[0] > 0) m_v_SMA_short[iTradSym].Add(dAvgPx);
          if (m_B2_FilterSMAPeriod[1] > 0) m_v_SMA_long[iTradSym].Add(dAvgPx);
        }
        else if (m_B2_FilterSMAPeriod.size() == 1)
        {
          if (m_B2_FilterSMAPeriod[0] > 0) m_v_SMA_long[iTradSym].Add(dAvgPx);
        }
      }

      if (m_HistoricalAvgPx->size()>1)
      {
        double d = m_HistoricalAvgPx->back() / *(m_HistoricalAvgPx->end()-2);
        if (!std::isnan(d)) m_HistoricalAvgPxRtn->push_back(d);
      }
      if (m_HistoricalClose->size()>1)
      {
        double d = m_HistoricalClose->back() / *(m_HistoricalClose->end()-2);
        if (!std::isnan(d)) m_HistoricalCloseRtn->push_back(d);
      }

    }
  }

  if (m_B2_TrainingFreq == TradingStrategyConfig::OnceAtInitWarmup)
  {
    DoTraining(iTradSym);
  }

}

void StrategyB2::UpdateInternalDataTrng(const int iTradSym)
{
  return;
}

void StrategyB2::UpdateInternalData(const int iTradSym)
{

  //--------------------------------------------------
  // For rotation
  //--------------------------------------------------
  if (m_RotationMode != 0)
  {
    //--------------------------------------------------
    if (m_AllAvbSymForRollingBasket[m_RotationGroup[iTradSym]].find(m_p_ymdhms_SysTime_Local->GetYYYYMMDD()) == m_AllAvbSymForRollingBasket[m_RotationGroup[iTradSym]].end())
    {
      m_AllAvbSymForRollingBasket[m_RotationGroup[iTradSym]][m_p_ymdhms_SysTime_Local->GetYYYYMMDD()] = set<string>();
    }
    //--------------------------------------------------
    // Only add those symbols with enough backtest data
    //--------------------------------------------------
    if (m_map_HistoricalAvgPx[m_TradedSymbols[iTradSym]].size() >= m_TrainingPeriodMax[iTradSym])
    {
      m_AllAvbSymForRollingBasket[m_RotationGroup[iTradSym]][m_p_ymdhms_SysTime_Local->GetYYYYMMDD()].insert(m_TradedSymbols[iTradSym]);
    }
    //--------------------------------------------------

    {
      YYYYMMDD ymd = m_p_ymdhms_SysTime_Local->GetYYYYMMDD();
      map<YYYYMMDD,set<string> >::iterator it = m_AllAvbSym.find(ymd);
      if (it == m_AllAvbSym.end())
      {
        m_AllAvbSym[ymd] = set<string>();
        it = m_AllAvbSym.find(ymd);
      }
      //--------------------------------------------------
      // strategy base will only call UpdateInternalData() for that symbol only if there is price update for that symbol
      //--------------------------------------------------
      if (m_HistoricalAvgPx->size() >= m_TrainingPeriodMax[iTradSym])
        it->second.insert(m_TradedSymbols[iTradSym]);

      if (m_PTask_PrintAllAvbSym.CheckIfItIsTimeToWakeUp(m_PTask_PrintAllAvbSymToken,m_ymdhms_SysTime_HKT))
        FForEach(it->second,[&](const string & sym) { m_Logger->Write(Logger::INFO,"Strategy %s: m_AllAvbSym %s", GetStrategyName(m_StyID).c_str(), sym.c_str()); });
    }
  }
  //--------------------------------------------------


  //--------------------------------------------------
  // because GetTimeNSecBefClose() gives HKT
  //--------------------------------------------------
  HHMMSS hmsStart = m_Exchg->GetTimeNSecBefClose(m_TradedSymbols[iTradSym],m_ymdhms_SysTime_HKT.GetYYYYMMDD(),m_B2_ActionTimeBefCloseInSec);
  HHMMSS hmsEnd   = m_Exchg->GetTimeNSecBefClose(m_TradedSymbols[iTradSym],m_ymdhms_SysTime_HKT.GetYYYYMMDD(),0);

  m_Logger->Write(Logger::DEBUG,"Strategy %s: Sym=%s will only update internal data when time (currently %s) is between %s and %s",
                  GetStrategyName(m_StyID).c_str(),
                  m_TradedSymbols[iTradSym].c_str(),
                  m_ymdhms_SysTime_HKT.GetHHMMSS().ToStr_().c_str(),
                  hmsStart.ToStr_().c_str(),
                  hmsEnd.ToStr_().c_str()
                 );

  if (
    m_AllSymWithUpdateData[m_p_ymdhms_SysTime_Local->GetYYYYMMDD()].find(m_TradedSymbols[iTradSym]) ==
    m_AllSymWithUpdateData[m_p_ymdhms_SysTime_Local->GetYYYYMMDD()].end() &&
    m_ymdhms_SysTime_HKT.GetHHMMSS() >= hmsStart &&
    m_ymdhms_SysTime_HKT.GetHHMMSS() <= hmsEnd
    )
  {
    double dHigh  = TechIndicators::Instance()->GetDayHigh(m_TradedSymbols[iTradSym]);
    double dLow   = TechIndicators::Instance()->GetDayLow(m_TradedSymbols[iTradSym]);
    double dClose = TechIndicators::Instance()->GetDayClose(m_TradedSymbols[iTradSym]);
    if (!std::isnan(dHigh) && !std::isnan(dLow) && !std::isnan(dClose) && abs(m_ymdhms_SysTime_HKT - m_ymdhms_midquote) < 10)
    {
      m_Logger->Write(Logger::INFO,"Strategy %s: %s Sym=%s Getting aggregated daily high low close: High = %f Low = %f Close = %f MidQuote = %f.",
                      GetStrategyName(m_StyID).c_str(), m_p_ymdhms_SysTime_Local->ToStr().c_str(),m_TradedSymbols[iTradSym].c_str(), dHigh, dLow, dClose, m_SymMidQuote);
      double dAvgPx = (dHigh+dLow+m_SymMidQuote)/(double)3;

      if (!m_HistoricalAvgPx->empty()) m_HistoricalNumNumOfRisingDaysCountAvgPx->Add(dAvgPx >= m_HistoricalAvgPx->back());
      if (!m_HistoricalClose->empty()) m_HistoricalNumNumOfRisingDaysCountClose->Add(dClose >= m_HistoricalClose->back());

      m_HistoricalAvgPx->push_back(dAvgPx);
      m_HistoricalClose->push_back(m_SymMidQuote);

      //--------------------------------------------------
      {
        YYYYMMDD ymd = m_p_ymdhms_SysTime_Local->GetYYYYMMDD();
        map<YYYYMMDD,set<string> >::iterator it = m_AllSymWithUpdateData.find(ymd);
        if (it == m_AllSymWithUpdateData.end())
        {
          m_AllSymWithUpdateData[ymd] = set<string>();
          it = m_AllSymWithUpdateData.find(ymd);
        }
        it->second.insert(m_TradedSymbols[iTradSym]);
      }
      //--------------------------------------------------

      if (!m_B2_FilterSMAPeriod.empty())
      {
        if (m_B2_FilterSMAPeriod.size() == 2)
        {
          if (m_B2_FilterSMAPeriod[0] > 0) m_v_SMA_short[iTradSym].Add(dAvgPx);
          if (m_B2_FilterSMAPeriod[1] > 0) m_v_SMA_long[iTradSym].Add(dAvgPx);
        }
        else if (m_B2_FilterSMAPeriod.size() == 1)
        {
          if (m_B2_FilterSMAPeriod[0] > 0) m_v_SMA_long[iTradSym].Add(dAvgPx);

        }
      }

      if (m_HistoricalAvgPx->size()>1)
      {
        double d = m_HistoricalAvgPx->back() / *(m_HistoricalAvgPx->end()-2);
        m_HistoricalAvgPxRtn->push_back(d);
      }
      if (m_HistoricalClose->size()>1)
      {
        double d = m_HistoricalClose->back() / *(m_HistoricalClose->end()-2);
        m_HistoricalCloseRtn->push_back(d);
      }

    }
  }

  m_dGKYZVal = TechIndicators::Instance()->GetGKYZValue(m_TradedSymbols[iTradSym]);
}



void StrategyB2::DoTraining(const int iTradSym)
{
  if (B2_SKIPMACHLEARNING) return;

  // SetParamAdjAmt(SML_ADJ_BETA12, BIG_ADJ_BETA12, SML_ADJ_BETA34, BIG_ADJ_BETA34);
  SetParamBetaRange(
    m_beta_1_start[iTradSym],
    m_beta_1_end[iTradSym],
    m_beta_1_incremt[iTradSym],
    m_beta_2_start[iTradSym],
    m_beta_2_end[iTradSym],
    m_beta_2_incremt[iTradSym],
    m_beta_3_start[iTradSym],
    m_beta_3_end[iTradSym],
    m_beta_3_incremt[iTradSym],
    m_beta_4_start[iTradSym],
    m_beta_4_end[iTradSym],
    m_beta_4_incremt[iTradSym]);

  switch(B2_HYPOTHESIS)
  {
    case B2_HYPOTHESIS_TAYLOR:
      {
        *m_bTrainRetAvgPx = TrainUpParamTaylor(
          m_TradedSymbols[iTradSym],
          *m_p_ymdhms_SysTime_Local,
          m_TrainingPeriod1[iTradSym],
          m_TrainingPeriod2[iTradSym],
          m_TrainingPeriod3[iTradSym],
          m_TrainingPeriod4[iTradSym],
          m_beta_1[iTradSym],
          m_beta_2[iTradSym],
          m_beta_3[iTradSym],
          m_beta_4[iTradSym],
          (*m_HistoricalAvgPx),
          (*m_HistoricalClose),
          (*m_HistoricalNumNumOfRisingDaysCountAvgPx),
          StrategyB2::TM_MAXSHARPE,
          m_WeightScheme[iTradSym],
          (*m_p_map_BestParamSetBeta1Beta3AvgPx_hypo1),
          (*m_p_map_BestParamSetBeta2Beta4AvgPx_hypo1),
          (*m_p_map_BestParamSetBeta1Beta3AvgPx_hypo2),
          (*m_p_map_BestParamSetBeta2Beta4AvgPx_hypo2),
          m_CurbInSmpRtn[iTradSym]
            );
          break;
      }

    case B2_HYPOTHESIS_SMA:
      {
        *m_bTrainRetAvgPx = TrainUpParamSMA(
          m_TradedSymbols[iTradSym],
          *m_p_ymdhms_SysTime_Local,
          m_TrainingPeriod1[iTradSym],
          m_TrainingPeriod2[iTradSym],
          m_TrainingPeriod3[iTradSym],
          m_TrainingPeriod4[iTradSym],
          m_beta_1[iTradSym],
          m_beta_2[iTradSym],
          m_beta_3[iTradSym],
          m_beta_4[iTradSym],
          (*m_HistoricalAvgPx),
          (*m_HistoricalClose),
          (*m_HistoricalNumNumOfRisingDaysCountAvgPx),
          StrategyB2::TM_MAXSHARPE,
          m_WeightScheme[iTradSym],
          (*m_p_map_BestParamSetBeta1Beta3AvgPx_hypo1),
          (*m_p_map_BestParamSetBeta2Beta4AvgPx_hypo1),
          m_CurbInSmpRtn[iTradSym]
          );
        break;
      }

    default:
      {

        break;
      }
  }

  m_map_ymdhms_LastTrainTime[B2_METH_AVGPX][m_TradedSymbols[iTradSym]] = *m_p_ymdhms_SysTime_Local;
  m_Logger->Write(Logger::INFO,"Strategy %s: %s %s (%d) Sym=%s AvgPx TrainUpParam() returned %s. m_p_map_BestParamSetBeta1Beta3AvgPx_hypo1.size() = %d m_p_map_BestParamSetBeta2Beta4AvgPx_hypo1.size() = %d m_p_map_BestParamSetBeta1Beta3AvgPx_hypo2.size() = %d m_p_map_BestParamSetBeta2Beta4AvgPx_hypo2.size() = %d",
                  GetStrategyName(m_StyID).c_str(),
                  m_p_ymdhms_SysTime_Local->ToStr().c_str(),
                  __FUNCTION__,__LINE__,
                  m_TradedSymbols[iTradSym].c_str(),(*m_bTrainRetAvgPx?"True":"False"),
                  (*m_p_map_BestParamSetBeta1Beta3AvgPx_hypo1).size(),
                  (*m_p_map_BestParamSetBeta2Beta4AvgPx_hypo1).size(),
                  (*m_p_map_BestParamSetBeta1Beta3AvgPx_hypo2).size(),
                  (*m_p_map_BestParamSetBeta2Beta4AvgPx_hypo2).size()
                 );

  if (m_PriceMode[iTradSym] == B2_PRICEMETH_AVGPXCLOSE)
  {
    // SetParamAdjAmt(SML_ADJ_BETA12, BIG_ADJ_BETA12, SML_ADJ_BETA34, BIG_ADJ_BETA34);
    SetParamBetaRange(
      m_beta_1_start[iTradSym],
      m_beta_1_end[iTradSym],
      m_beta_1_incremt[iTradSym],
      m_beta_2_start[iTradSym],
      m_beta_2_end[iTradSym],
      m_beta_2_incremt[iTradSym],
      m_beta_3_start[iTradSym],
      m_beta_3_end[iTradSym],
      m_beta_3_incremt[iTradSym],
      m_beta_4_start[iTradSym],
      m_beta_4_end[iTradSym],
      m_beta_4_incremt[iTradSym]);

    switch(B2_HYPOTHESIS)
    {
      case B2_HYPOTHESIS_TAYLOR:
        {
          *m_bTrainRetClose = TrainUpParamTaylor(
            m_TradedSymbols[iTradSym],
            *m_p_ymdhms_SysTime_Local,
            m_TrainingPeriod1[iTradSym],
            m_TrainingPeriod2[iTradSym],
            m_TrainingPeriod3[iTradSym],
            m_TrainingPeriod4[iTradSym],
            m_beta_1[iTradSym],
            m_beta_2[iTradSym],
            m_beta_3[iTradSym],
            m_beta_4[iTradSym],
            (*m_HistoricalClose),
            (*m_HistoricalClose),
            (*m_HistoricalNumNumOfRisingDaysCountClose),
            StrategyB2::TM_MAXSHARPE,
            m_WeightScheme[iTradSym],
            (*m_p_map_BestParamSetBeta1Beta3Close_hypo1),
            (*m_p_map_BestParamSetBeta2Beta4Close_hypo1),
            (*m_p_map_BestParamSetBeta1Beta3Close_hypo2),
            (*m_p_map_BestParamSetBeta2Beta4Close_hypo2),
            m_CurbInSmpRtn[iTradSym]
              );

            break;
        }
      case B2_HYPOTHESIS_SMA:
        {
          *m_bTrainRetClose = TrainUpParamSMA(
            m_TradedSymbols[iTradSym],
            *m_p_ymdhms_SysTime_Local,
            m_TrainingPeriod1[iTradSym],
            m_TrainingPeriod2[iTradSym],
            m_TrainingPeriod3[iTradSym],
            m_TrainingPeriod4[iTradSym],
            m_beta_1[iTradSym],
            m_beta_2[iTradSym],
            m_beta_3[iTradSym],
            m_beta_4[iTradSym],
            (*m_HistoricalClose),
            (*m_HistoricalClose),
            (*m_HistoricalNumNumOfRisingDaysCountClose),
            StrategyB2::TM_MAXSHARPE,
            m_WeightScheme[iTradSym],
            (*m_p_map_BestParamSetBeta1Beta3Close_hypo1),
            (*m_p_map_BestParamSetBeta2Beta4Close_hypo1),
            m_CurbInSmpRtn[iTradSym]
            );

          break;
        }
      default:
        {

          break;
        }
    }

    m_map_ymdhms_LastTrainTime[B2_METH_CLOSE][m_TradedSymbols[iTradSym]] = *m_p_ymdhms_SysTime_Local;
    m_Logger->Write(Logger::INFO,"Strategy %s: %s %s (%d) Sym=%s Close TrainUpParam() returned %s. m_p_map_BestParamSetBeta1Beta3Close_hypo1.size() = %d m_p_map_BestParamSetBeta2Beta4Close_hypo1.size() = %d m_p_map_BestParamSetBeta1Beta3Close_hypo2.size() = %d m_p_map_BestParamSetBeta2Beta4Close_hypo2.size() = %d",
                    GetStrategyName(m_StyID).c_str(),
                    m_p_ymdhms_SysTime_Local->ToStr().c_str(),
                    __FUNCTION__,__LINE__,
                    m_TradedSymbols[iTradSym].c_str(),(*m_bTrainRetClose?"True":"False"),
                    (*m_p_map_BestParamSetBeta1Beta3Close_hypo1).size(),
                    (*m_p_map_BestParamSetBeta2Beta4Close_hypo1).size(),
                    (*m_p_map_BestParamSetBeta1Beta3Close_hypo2).size(),
                    (*m_p_map_BestParamSetBeta2Beta4Close_hypo2).size()
                   );
  }

}


void StrategyB2::PerformTrainingJustBeforeTrading(const int iTradSym)
{
  //--------------------------------------------------
  // if don't want to miss the last data point (which is today) in our training, should do the training here.
  // if want to leave enough time for many symbols to train, train after initial warmup
  //--------------------------------------------------
  if (m_B2_TrainingFreq == TradingStrategyConfig::OnceAtInitWarmup)
  {
    m_Logger->Write(Logger::INFO,"Strategy %s: %s %s (%d) Sym=%s m_B2_TrainingFreq = OnceAtInitWarmup",
                    GetStrategyName(m_StyID).c_str(),
                    m_p_ymdhms_SysTime_Local->ToStr().c_str(),
                    __FUNCTION__,__LINE__,
                    m_TradedSymbols[iTradSym].c_str());
    m_Logger->Write(Logger::INFO,"Strategy %s: %s %s (%d) Sym=%s AvgPx TrainUpParam() returned %s. m_p_map_BestParamSetBeta1Beta3AvgPx_hypo1.size() = %d m_p_map_BestParamSetBeta2Beta4AvgPx_hypo1.size() = %d m_p_map_BestParamSetBeta1Beta3AvgPx_hypo2.size() = %d m_p_map_BestParamSetBeta2Beta4AvgPx_hypo2.size() = %d LastTrainTime = %s",
                    GetStrategyName(m_StyID).c_str(),
                    m_p_ymdhms_SysTime_Local->ToStr().c_str(),
                    __FUNCTION__,__LINE__,
                    m_TradedSymbols[iTradSym].c_str(),
                    (*m_bTrainRetAvgPx?"True":"False"),
                    (*m_p_map_BestParamSetBeta1Beta3AvgPx_hypo1).size(),
                    (*m_p_map_BestParamSetBeta2Beta4AvgPx_hypo1).size(),
                    (*m_p_map_BestParamSetBeta1Beta3AvgPx_hypo2).size(),
                    (*m_p_map_BestParamSetBeta2Beta4AvgPx_hypo2).size(),
                    m_map_ymdhms_LastTrainTime[B2_METH_AVGPX][m_TradedSymbols[iTradSym]].ToStr().c_str()
                   );
    m_Logger->Write(Logger::INFO,"Strategy %s: %s %s (%d) Sym=%s Close TrainUpParam() returned %s. m_p_map_BestParamSetBeta1Beta3Close_hypo1.size() = %d m_p_map_BestParamSetBeta2Beta4Close_hypo1.size() = %d m_p_map_BestParamSetBeta1Beta3Close_hypo2.size() = %d m_p_map_BestParamSetBeta2Beta4Close_hypo2.size() = %d LastTrainTime = %s",
                    GetStrategyName(m_StyID).c_str(),
                    m_p_ymdhms_SysTime_Local->ToStr().c_str(),
                    __FUNCTION__,__LINE__,
                    m_TradedSymbols[iTradSym].c_str(),
                    (*m_bTrainRetClose?"True":"False"),
                    (*m_p_map_BestParamSetBeta1Beta3Close_hypo1).size(),
                    (*m_p_map_BestParamSetBeta2Beta4Close_hypo1).size(),
                    (*m_p_map_BestParamSetBeta1Beta3Close_hypo2).size(),
                    (*m_p_map_BestParamSetBeta2Beta4Close_hypo2).size(),
                    m_map_ymdhms_LastTrainTime[B2_METH_AVGPX][m_TradedSymbols[iTradSym]].ToStr().c_str()
                   );
    return;
  }


  // //--------------------------------------------------
  // // If Special ES + SPY mode, no need to train for ES
  // //--------------------------------------------------
  // if (m_ES_SPY_Together_SPYidx >= 0
  //     &&
  //     m_TradedSymbols[iTradSym] == ES_CONT_FUT_1)
  // {
  //   m_Logger->Write(Logger::INFO,"Strategy %s: %s %s Special ES+SPY mode is on. No need to train for %s",
  //                   GetStrategyName(m_StyID).c_str(),
  //                   m_p_ymdhms_SysTime_Local->ToStr().c_str(),
  //                   m_TradedSymbols[iTradSym].c_str(),
  //                   ES_CONT_FUT_1
  //                  );
  //   return;
  // }

  //--------------------------------------------------
  // Train up our parameters
  //--------------------------------------------------
  if (
    m_B2_TrainingFreq == TradingStrategyConfig::Daily
    ||
    (
      !m_map_ymdhms_LastTrainTime[B2_METH_AVGPX][m_TradedSymbols[iTradSym]].IsValid()
      ||
      ((*m_bTrainRetAvgPx) && m_p_ymdhms_SysTime_Local->GetYYYYMMDD() - m_map_ymdhms_LastTrainTime[B2_METH_AVGPX][m_TradedSymbols[iTradSym]].GetYYYYMMDD() > (int)ceil(m_TrainingPeriod1[iTradSym]/(double)10))
      ||
      (!(*m_bTrainRetAvgPx) && m_p_ymdhms_SysTime_Local->GetYYYYMMDD() > m_map_ymdhms_LastTrainTime[B2_METH_AVGPX][m_TradedSymbols[iTradSym]].GetYYYYMMDD())
      ||
      (B2_HYPOTHESIS == B2_HYPOTHESIS_TAYLOR
       && m_p_map_BestParamSetBeta1Beta3AvgPx_hypo1->empty() && m_p_map_BestParamSetBeta2Beta4AvgPx_hypo1->empty()
       && m_p_map_BestParamSetBeta1Beta3AvgPx_hypo2->empty() && m_p_map_BestParamSetBeta2Beta4AvgPx_hypo2->empty()
      )
      ||
      (B2_HYPOTHESIS == B2_HYPOTHESIS_SMA
       && m_p_map_BestParamSetBeta1Beta3AvgPx_hypo1->empty() && m_p_map_BestParamSetBeta2Beta4AvgPx_hypo1->empty())
    )
    )
  {
    DoTraining(iTradSym);
  }
  else
  {
    m_Logger->Write(Logger::DEBUG,"Strategy %s: %s %s (%d) Sym=%s AvgPx TrainUpParam() returned %s. m_p_map_BestParamSetBeta1Beta3AvgPx_hypo1.size() = %d m_p_map_BestParamSetBeta2Beta4AvgPx_hypo1.size() = %d m_p_map_BestParamSetBeta1Beta3AvgPx_hypo2.size() = %d m_p_map_BestParamSetBeta2Beta4AvgPx_hypo2.size() = %d LastTrainTime = %s",
                    GetStrategyName(m_StyID).c_str(),
                    m_p_ymdhms_SysTime_Local->ToStr().c_str(),
                    __FUNCTION__,__LINE__,
                    m_TradedSymbols[iTradSym].c_str(),
                    (*m_bTrainRetAvgPx?"True":"False"),
                    (*m_p_map_BestParamSetBeta1Beta3AvgPx_hypo1).size(),
                    (*m_p_map_BestParamSetBeta2Beta4AvgPx_hypo1).size(),
                    (*m_p_map_BestParamSetBeta1Beta3AvgPx_hypo2).size(),
                    (*m_p_map_BestParamSetBeta2Beta4AvgPx_hypo2).size(),
                    m_map_ymdhms_LastTrainTime[B2_METH_AVGPX][m_TradedSymbols[iTradSym]].ToStr().c_str()
                   );
  }

  if (m_PriceMode[iTradSym] == B2_PRICEMETH_AVGPXCLOSE)
  {
    if (
      m_B2_TrainingFreq == TradingStrategyConfig::Daily
      ||
      (
        !m_map_ymdhms_LastTrainTime[B2_METH_CLOSE][m_TradedSymbols[iTradSym]].IsValid()
        ||
        ((*m_bTrainRetClose) && m_p_ymdhms_SysTime_Local->GetYYYYMMDD() - m_map_ymdhms_LastTrainTime[B2_METH_CLOSE][m_TradedSymbols[iTradSym]].GetYYYYMMDD() > (int)ceil(m_TrainingPeriod1[iTradSym]/(double)10))
        ||
        (!(*m_bTrainRetClose) && m_p_ymdhms_SysTime_Local->GetYYYYMMDD() > m_map_ymdhms_LastTrainTime[B2_METH_CLOSE][m_TradedSymbols[iTradSym]].GetYYYYMMDD())
        ||
        (B2_HYPOTHESIS == B2_HYPOTHESIS_TAYLOR
         && m_p_map_BestParamSetBeta1Beta3AvgPx_hypo1->empty() && m_p_map_BestParamSetBeta2Beta4AvgPx_hypo1->empty()
         && m_p_map_BestParamSetBeta1Beta3AvgPx_hypo2->empty() && m_p_map_BestParamSetBeta2Beta4AvgPx_hypo2->empty()
        )
        ||
        (B2_HYPOTHESIS == B2_HYPOTHESIS_SMA
         && m_p_map_BestParamSetBeta1Beta3AvgPx_hypo1->empty() && m_p_map_BestParamSetBeta2Beta4AvgPx_hypo1->empty())
      )
      )
    {
      DoTraining(iTradSym);
    }
    else
    {
      m_Logger->Write(Logger::DEBUG,"Strategy %s: %s %s (%d) Sym=%s Close TrainUpParam() returned %s. m_p_map_BestParamSetBeta1Beta3Close_hypo1.size() = %d m_p_map_BestParamSetBeta2Beta4Close_hypo1.size() = %d m_p_map_BestParamSetBeta1Beta3Close_hypo2.size() = %d m_p_map_BestParamSetBeta2Beta4Close_hypo2.size() = %d LastTrainTime = %s",
                      GetStrategyName(m_StyID).c_str(),
                      m_p_ymdhms_SysTime_Local->ToStr().c_str(),
                      __FUNCTION__,__LINE__,
                      m_TradedSymbols[iTradSym].c_str(),
                      (*m_bTrainRetClose?"True":"False"),
                      (*m_p_map_BestParamSetBeta1Beta3Close_hypo1).size(),
                      (*m_p_map_BestParamSetBeta2Beta4Close_hypo1).size(),
                      (*m_p_map_BestParamSetBeta1Beta3Close_hypo2).size(),
                      (*m_p_map_BestParamSetBeta2Beta4Close_hypo2).size(),
                      m_map_ymdhms_LastTrainTime[B2_METH_AVGPX][m_TradedSymbols[iTradSym]].ToStr().c_str()
                     );
    }
  }


}


void StrategyB2::EndOfDayTrainingForEachTimeBucket(const int iTradSym,const map<HHMMSS,double> & map_HistDataInTimeBucket)
{
}




void StrategyB2::DetermineRegime(const int iTradSym)
{
  *m_bRisingRegimeAvgPx = false;
  *m_bRisingRegimeClose = false;

  if (m_HistoricalAvgPx->size() < 2) return;
  if (m_HistoricalClose->size() < 2) return;
  if (m_HistoricalAvgPx->back() >= *(m_HistoricalAvgPx->end()-2)) *m_bRisingRegimeAvgPx = true;
  if (m_HistoricalClose->back() >= *(m_HistoricalClose->end()-2)) *m_bRisingRegimeClose = true;

  m_Logger->Write(Logger::INFO,"Strategy %s: %s Sym=%s m_bRisingRegimeAvgPx = %s m_bRisingRegimeClose = %s",
                  GetStrategyName(m_StyID).c_str(), m_p_ymdhms_SysTime_Local->ToStr().c_str(),m_TradedSymbols[iTradSym].c_str(),(*m_bRisingRegimeAvgPx?"True":"False"),(*m_bRisingRegimeClose?"True":"False"));

}


void StrategyB2::PreTradePreparation(const int iTradSym)
{

  //--------------------------------------------------
  if (m_AllSymWithUpdateData[m_p_ymdhms_SysTime_Local->GetYYYYMMDD()].find(m_TradedSymbols[iTradSym]) ==
      m_AllSymWithUpdateData[m_p_ymdhms_SysTime_Local->GetYYYYMMDD()].end())
  {
    m_Logger->Write(Logger::INFO,"Strategy %s: %s Sym=%s Guard in PreTradePreparation(): Internal data not updated yet.",
                    GetStrategyName(m_StyID).c_str(),
                    m_p_ymdhms_SysTime_Local->ToStr().c_str(),
                    m_TradedSymbols[iTradSym].c_str()
                   );
    return;
  }

  //--------------------------------------------------
  m_Logger->Write(Logger::INFO,"Strategy %s: %s Sym=%s Using parameters: m_beta_1 = %f, m_beta_2 = %f, m_beta_3 = %f, m_beta_4 = %f.",
                  GetStrategyName(m_StyID).c_str(),
                  m_p_ymdhms_SysTime_Local->ToStr().c_str(),
                  m_TradedSymbols[iTradSym].c_str(),m_beta_1[iTradSym],m_beta_2[iTradSym],m_beta_3[iTradSym],m_beta_4[iTradSym]);

  //--------------------------------------------------
  // Training combinations
  //--------------------------------------------------
  switch(B2_HYPOTHESIS)
  {
    case B2_HYPOTHESIS_TAYLOR:
      {
        if (*m_bRisingRegimeAvgPx)
        {
          m_lNumOfTrngCombnAvgPx =
            ((m_beta_1_end[iTradSym] - m_beta_1_start[iTradSym]) / m_beta_1_incremt[iTradSym] + 1) *
            ((m_beta_3_end[iTradSym] - m_beta_3_start[iTradSym]) / m_beta_3_incremt[iTradSym] + 1);
        }
        else
        {
          m_lNumOfTrngCombnAvgPx =
            ((m_beta_2_end[iTradSym] - m_beta_2_start[iTradSym]) / m_beta_2_incremt[iTradSym] + 1) *
            ((m_beta_4_end[iTradSym] - m_beta_4_start[iTradSym]) / m_beta_4_incremt[iTradSym] + 1);
        }

        if (*m_bRisingRegimeClose)
        {
          m_lNumOfTrngCombnClose =
            ((m_beta_1_end[iTradSym] - m_beta_1_start[iTradSym]) / m_beta_1_incremt[iTradSym] + 1) *
            ((m_beta_3_end[iTradSym] - m_beta_3_start[iTradSym]) / m_beta_3_incremt[iTradSym] + 1);
        }
        else
        {
          m_lNumOfTrngCombnClose =
            ((m_beta_2_end[iTradSym] - m_beta_2_start[iTradSym]) / m_beta_2_incremt[iTradSym] + 1) *
            ((m_beta_4_end[iTradSym] - m_beta_4_start[iTradSym]) / m_beta_4_incremt[iTradSym] + 1);
        }

        break;
      }
    case B2_HYPOTHESIS_SMA:
      {
        m_lNumOfTrngCombnAvgPx =
          ((m_beta_1_end[iTradSym] - m_beta_1_start[iTradSym]) / m_beta_1_incremt[iTradSym] + 1) *
          ((m_beta_2_end[iTradSym] - m_beta_2_start[iTradSym]) / m_beta_2_incremt[iTradSym] + 1) *
          ((m_beta_3_end[iTradSym] - m_beta_3_start[iTradSym]) / m_beta_3_incremt[iTradSym] + 1) *
          ((m_beta_4_end[iTradSym] - m_beta_4_start[iTradSym]) / m_beta_4_incremt[iTradSym] + 1);
        m_lNumOfTrngCombnClose = m_lNumOfTrngCombnAvgPx;

        break;
      }
    default:
      {
        m_lNumOfTrngCombnAvgPx = 0;
        m_lNumOfTrngCombnClose = 0;

        break;
      }
  }

  //--------------------------------------------------
  // Get our predictor
  //--------------------------------------------------
  m_dStrengthCountAvgPx = 0;
  m_dStrengthCountClose = 0;
  m_map_TotSharpeOfMethod.clear();

  if (!B2_SKIPMACHLEARNING)
  { 

    switch(B2_HYPOTHESIS)
    {
      case B2_HYPOTHESIS_TAYLOR:
        {
          int iTaylorIdx = B2_HYPOTHESIS_TAYLOR1;

          while (iTaylorIdx <= B2_HYPOTHESIS_TAYLOR2)
          {
            map<double,vector<double> > * m_p_map_BestParamSetBeta1Beta3AvgPx = NULL;
            map<double,vector<double> > * m_p_map_BestParamSetBeta2Beta4AvgPx = NULL;
            map<double,vector<double> > * m_p_map_BestParamSetBeta1Beta3Close = NULL;
            map<double,vector<double> > * m_p_map_BestParamSetBeta2Beta4Close = NULL;

            if (iTaylorIdx == B2_HYPOTHESIS_TAYLOR1)
            {
              m_p_map_BestParamSetBeta1Beta3AvgPx = m_p_map_BestParamSetBeta1Beta3AvgPx_hypo1;
              m_p_map_BestParamSetBeta2Beta4AvgPx = m_p_map_BestParamSetBeta2Beta4AvgPx_hypo1;
              m_p_map_BestParamSetBeta1Beta3Close = m_p_map_BestParamSetBeta1Beta3Close_hypo1;
              m_p_map_BestParamSetBeta2Beta4Close = m_p_map_BestParamSetBeta2Beta4Close_hypo1;
            }
            else if (iTaylorIdx == B2_HYPOTHESIS_TAYLOR2)
            {
              m_p_map_BestParamSetBeta1Beta3AvgPx = m_p_map_BestParamSetBeta1Beta3AvgPx_hypo2;
              m_p_map_BestParamSetBeta2Beta4AvgPx = m_p_map_BestParamSetBeta2Beta4AvgPx_hypo2;
              m_p_map_BestParamSetBeta1Beta3Close = m_p_map_BestParamSetBeta1Beta3Close_hypo2;
              m_p_map_BestParamSetBeta2Beta4Close = m_p_map_BestParamSetBeta2Beta4Close_hypo2;
            }


            for (unsigned int iAorC = 0; iAorC < 2; ++iAorC)
            {
              double & dStrengthCount = (iAorC == 0 ? m_dStrengthCountAvgPx : m_dStrengthCountClose);
              bool & m_bTrainRet = (iAorC == 0 ? *m_bTrainRetAvgPx : *m_bTrainRetClose);
              bool & bRisingRegime = (iAorC == 0 ? *m_bRisingRegimeAvgPx : *m_bRisingRegimeClose);
              map<double,vector<double> > & m_p_map_BestParamSetBeta1Beta3 = (iAorC == 0 ? *m_p_map_BestParamSetBeta1Beta3AvgPx : *m_p_map_BestParamSetBeta1Beta3Close);
              map<double,vector<double> > & m_p_map_BestParamSetBeta2Beta4 = (iAorC == 0 ? *m_p_map_BestParamSetBeta2Beta4AvgPx : *m_p_map_BestParamSetBeta2Beta4Close);
              vector<double> * m_Historical = (iAorC == 0 ? m_HistoricalAvgPx : m_HistoricalClose);
              long & lNumOfTrngCombn = (iAorC == 0 ? m_lNumOfTrngCombnAvgPx : m_lNumOfTrngCombnClose);

              //--------------------------------------------------
              m_map_TotSharpeOfMethod[iAorC] = 0;
              //--------------------------------------------------

              //--------------------------------------------------
              if (m_bTrainRet)
              {
                double b1 = 0;
                double b2 = 0;
                double b3 = 0;
                double b4 = 0;

                if ( (bRisingRegime && !m_p_map_BestParamSetBeta1Beta3.empty()) ||
                    (!bRisingRegime && !m_p_map_BestParamSetBeta2Beta4.empty())   )
                {
                  int iCnt = 0;
                  map<double,vector<double> >::iterator it_rising  = m_p_map_BestParamSetBeta1Beta3.end();
                  map<double,vector<double> >::iterator it_falling = m_p_map_BestParamSetBeta2Beta4.end();

                  if (bRisingRegime)
                    it_rising--;
                  else
                    it_falling--;

                  while (
                    (bRisingRegime  && it_rising  != m_p_map_BestParamSetBeta1Beta3.begin()) ||
                    (!bRisingRegime && it_falling != m_p_map_BestParamSetBeta2Beta4.begin())
                    )
                  {

                    double dScale = 10000;
                    if (bRisingRegime)
                    {
                      b1 = round(it_rising->second[0]*dScale)/dScale;
                      b3 = round(it_rising->second[1]*dScale)/dScale;
                    }
                    else
                    {
                      b2 = round(it_falling->second[0]*dScale)/dScale;
                      b4 = round(it_falling->second[1]*dScale)/dScale;
                    }

                    double dEstimate     = 0;
                    double dAvgPnL       = 0;
                    double dSharpe       = 0;
                    double dSquaredError = 0;

                    if (GetEstimate(m_TrainingPeriod1[iTradSym],b1,b2,b3,b4,*m_Historical,*m_HistoricalClose,&dSquaredError,&dAvgPnL,&dSharpe,&dEstimate,m_WeightScheme[iTradSym],false,iTaylorIdx))
                    {
                      if (dSharpe > 0)
                      {
                        if (dEstimate > 0)
                        {
                          dStrengthCount += 1;
                        }
                        else if (dEstimate < 0)
                        {
                          dStrengthCount -= 1;
                        }
                      }

                      m_map_TotSharpeOfMethod[iAorC] += dSharpe;

                      m_Logger->Write(Logger::INFO,"Strategy %s: %s Sym=%s %s Selected from the set of best parameters: m_beta_1 = %f, m_beta_2 = %f, m_beta_3 = %f, m_beta_4 = %f. dSquaredError = %f. dAvgPnL = %f. dSharpe = %f.",
                                      GetStrategyName(m_StyID).c_str(),
                                      m_p_ymdhms_SysTime_Local->ToStr().c_str(),
                                      m_TradedSymbols[iTradSym].c_str(),
                                      (iAorC == 0 ? "AvgPx" : "Close"),
                                      b1,b2,b3,b4,dSquaredError,dAvgPnL,dSharpe);
                    }
                    iCnt++;
                    if ((iCnt+1) >= m_PropOfBestParam[iTradSym] / (double)100 * (double)lNumOfTrngCombn) break;
                    if ( bRisingRegime && iCnt >= m_p_map_BestParamSetBeta1Beta3.size()) break;
                    if (!bRisingRegime && iCnt >= m_p_map_BestParamSetBeta2Beta4.size()) break;
                    if (bRisingRegime)
                      it_rising--;
                    else
                      it_falling--;
                  }
                  m_Logger->Write(Logger::INFO,"Strategy %s: %s Sym=%s %s Selected a set of %d parameters in total.",
                                  GetStrategyName(m_StyID).c_str(),
                                  m_p_ymdhms_SysTime_Local->ToStr().c_str(),
                                  m_TradedSymbols[iTradSym].c_str(),
                                  (iAorC == 0 ? "AvgPx" : "Close"),
                                  iCnt);
                }
                else
                {
                  m_Logger->Write(Logger::INFO,"Strategy %s: Sym=%s %s %s m_p_map_BestParamSet empty.",
                                  GetStrategyName(m_StyID).c_str(), m_TradedSymbols[iTradSym].c_str(), m_p_ymdhms_SysTime_Local->ToStr().c_str(), (iAorC == 0 ? "AvgPx" : "Close"));
                }

              }
              else
              {
                m_Logger->Write(Logger::INFO,"Strategy %s: %s Sym=%s %s (%d) %s m_bTrainRet is false.",
                                GetStrategyName(m_StyID).c_str(),
                                m_p_ymdhms_SysTime_Local->ToStr().c_str(),
                                m_TradedSymbols[iTradSym].c_str(),
                                __FUNCTION__,
                                __LINE__,
                                (iAorC == 0 ? "AvgPx" : "Close"));
              }
              m_Logger->Write(Logger::INFO,"Strategy %s: %s Sym=%s %s dStrengthCount = %f lNumOfTrngCombn = %d",
                              GetStrategyName(m_StyID).c_str(),
                              m_p_ymdhms_SysTime_Local->ToStr().c_str(),m_TradedSymbols[iTradSym].c_str(),
                              (iAorC == 0 ? "AvgPx" : "Close"),
                              dStrengthCount,
                              lNumOfTrngCombn);
            }

            iTaylorIdx++;
          }

          break;
        }
      case B2_HYPOTHESIS_SMA:
        {
          for (unsigned int iAorC = 0; iAorC < 2; ++iAorC)
          {
            double & dStrengthCount = (iAorC == 0 ? m_dStrengthCountAvgPx : m_dStrengthCountClose);
            bool & m_bTrainRet = (iAorC == 0 ? *m_bTrainRetAvgPx : *m_bTrainRetClose);
            map<double,vector<double> > & m_p_map_BestParamSetBeta1Beta3 = (iAorC == 0 ? *m_p_map_BestParamSetBeta1Beta3AvgPx_hypo1 : *m_p_map_BestParamSetBeta1Beta3Close_hypo1);
            map<double,vector<double> > & m_p_map_BestParamSetBeta2Beta4 = (iAorC == 0 ? *m_p_map_BestParamSetBeta2Beta4AvgPx_hypo1 : *m_p_map_BestParamSetBeta2Beta4Close_hypo1);
            vector<double> * m_Historical = (iAorC == 0 ? m_HistoricalAvgPx : m_HistoricalClose);
            long & lNumOfTrngCombn = (iAorC == 0 ? m_lNumOfTrngCombnAvgPx : m_lNumOfTrngCombnClose);

            //--------------------------------------------------
            m_map_TotSharpeOfMethod[iAorC] = 0;
            //--------------------------------------------------

            //--------------------------------------------------
            if (m_bTrainRet)
            {
              double b1 = 0;
              double b2 = 0;
              double b3 = 0;
              double b4 = 0;

              if (
                !m_p_map_BestParamSetBeta1Beta3.empty()
                &&
                !m_p_map_BestParamSetBeta2Beta4.empty()
                )
              {
                int iCnt = 0;
                map<double,vector<double> >::iterator it_SMA1  = m_p_map_BestParamSetBeta1Beta3.end();
                map<double,vector<double> >::iterator it_SMA2  = m_p_map_BestParamSetBeta2Beta4.end();

                it_SMA1--;
                it_SMA2--;

                while (
                  it_SMA1 != m_p_map_BestParamSetBeta1Beta3.begin()
                  &&
                  it_SMA2 != m_p_map_BestParamSetBeta2Beta4.begin()
                  )
                {

                  double dScale = 10000;
                  b1 = round(it_SMA1->second[0]*dScale)/dScale;
                  b3 = round(it_SMA1->second[1]*dScale)/dScale;
                  b2 = round(it_SMA2->second[0]*dScale)/dScale;
                  b4 = round(it_SMA2->second[1]*dScale)/dScale;

                  double dEstimate     = 0;
                  double dAvgPnL       = 0;
                  double dSharpe       = 0;
                  double dSquaredError = 0;

                  if (GetEstimate(m_TrainingPeriod1[iTradSym],b1,b2,b3,b4,*m_Historical,*m_HistoricalClose,&dSquaredError,&dAvgPnL,&dSharpe,&dEstimate,m_WeightScheme[iTradSym],false,B2_HYPOTHESIS_SMA))
                  {
                    if (dSharpe > 0)
                    {
                      if (dEstimate > 0)
                      {
                        dStrengthCount += 1;
                      }
                      else if (dEstimate < 0)
                      {
                        dStrengthCount -= 1;
                      }
                    }

                    m_map_TotSharpeOfMethod[iAorC] += dSharpe;

                    m_Logger->Write(Logger::INFO,"Strategy %s: %s Sym=%s %s Selected from the set of best parameters: m_beta_1 = %f, m_beta_2 = %f, m_beta_3 = %f, m_beta_4 = %f. dSquaredError = %f. dAvgPnL = %f. dSharpe = %f.",
                                    GetStrategyName(m_StyID).c_str(),
                                    m_p_ymdhms_SysTime_Local->ToStr().c_str(),
                                    m_TradedSymbols[iTradSym].c_str(),
                                    (iAorC == 0 ? "AvgPx" : "Close"),
                                    b1,b2,b3,b4,dSquaredError,dAvgPnL,dSharpe);
                  }
                  iCnt++;
                  if ((iCnt+1) >= m_PropOfBestParam[iTradSym] / (double)100 * (double)lNumOfTrngCombn) break;
                  if (iCnt >= m_p_map_BestParamSetBeta1Beta3.size()) break;
                  if (iCnt >= m_p_map_BestParamSetBeta2Beta4.size()) break;
                  it_SMA1--;
                  it_SMA2--;
                }
                m_Logger->Write(Logger::INFO,"Strategy %s: %s Sym=%s %s Selected a set of %d parameters in total.",
                                GetStrategyName(m_StyID).c_str(),
                                m_p_ymdhms_SysTime_Local->ToStr().c_str(),
                                m_TradedSymbols[iTradSym].c_str(),
                                (iAorC == 0 ? "AvgPx" : "Close"),
                                iCnt);
              }
              else
              {
                m_Logger->Write(Logger::INFO,"Strategy %s: Sym=%s %s %s m_p_map_BestParamSet empty.",
                                GetStrategyName(m_StyID).c_str(), m_TradedSymbols[iTradSym].c_str(), m_p_ymdhms_SysTime_Local->ToStr().c_str(), (iAorC == 0 ? "AvgPx" : "Close"));
              }

            }
            else
            {
              m_Logger->Write(Logger::INFO,"Strategy %s: %s Sym=%s %s (%d) %s m_bTrainRet is false.",
                              GetStrategyName(m_StyID).c_str(),
                              m_p_ymdhms_SysTime_Local->ToStr().c_str(),
                              m_TradedSymbols[iTradSym].c_str(),
                              __FUNCTION__,
                              __LINE__,
                              (iAorC == 0 ? "AvgPx" : "Close"));
            }
            m_Logger->Write(Logger::INFO,"Strategy %s: %s Sym=%s %s dStrengthCount = %f lNumOfTrngCombn = %d",
                            GetStrategyName(m_StyID).c_str(),
                            m_p_ymdhms_SysTime_Local->ToStr().c_str(),m_TradedSymbols[iTradSym].c_str(),
                            (iAorC == 0 ? "AvgPx" : "Close"),
                            dStrengthCount,
                            lNumOfTrngCombn);
          }

          break;
        }


      default:
        {

          break;
        }
    }

  }

  //--------------------------------------------------
  // Calculate actual position size
  //--------------------------------------------------
  m_dAggUnsignedQty = 0;
  m_dAggSignedQty = 0;
  if (m_map_dq_GKYZ.find(m_TradedSymbols[iTradSym]) == m_map_dq_GKYZ.end())
  {
    m_map_dq_GKYZ[m_TradedSymbols[iTradSym]] = deque<double>();
  }

  if (!std::isnan(m_dGKYZVal))
  {
    m_map_dq_GKYZ[m_TradedSymbols[iTradSym]].push_back(m_dGKYZVal);
  }

  m_dAggUnsignedQty = m_NotionalAmt[iTradSym] / m_SymMidQuote;
  m_Logger->Write(Logger::INFO,"Strategy %s: %s Sym=%s GKYZ.size() %d GKYZ %f m_dAggUnsignedQty (if not perform any volatility adj) %f.",
                  GetStrategyName(m_StyID).c_str(),
                  m_p_ymdhms_SysTime_Local->ToStr().c_str(),
                  m_TradedSymbols[iTradSym].c_str(),
                  m_map_dq_GKYZ[m_TradedSymbols[iTradSym]].size(),
                  m_dGKYZVal,
                  m_dAggUnsignedQty);

  if (!(m_map_dq_GKYZ[m_TradedSymbols[iTradSym]].empty() || std::isnan(m_dGKYZVal) || m_MinAnnVolPsnSz[iTradSym] <= 0))
  {
    m_dAggUnsignedQty = m_NotionalAmt[iTradSym] / m_SymMidQuote * min(m_MinAnnVolPsnSz[iTradSym] / m_dGKYZVal,(double)1);
    m_Logger->Write(Logger::INFO,"Strategy %s: %s Sym=%s m_dAggUnsignedQty (after volatility adj) %f GKYZ %f.", GetStrategyName(m_StyID).c_str(), m_p_ymdhms_SysTime_Local->ToStr().c_str(),m_TradedSymbols[iTradSym].c_str(),m_dAggUnsignedQty,m_dGKYZVal);
  }

  //--------------------------------------------------
  // Figure out the proportion to use between AvgPx and Close
  //--------------------------------------------------
  if (m_PriceMode[iTradSym] == B2_PRICEMETH_AVGPXCLOSE)
  {
    double dAvgPxMethWeight = 0;
    double dCloseMethWeight = 0;
    double dTotalSharpe     = max(m_map_TotSharpeOfMethod[0],(double)0)+max(m_map_TotSharpeOfMethod[1],(double)0);

    if (dTotalSharpe > 0)
    {
      if (m_map_TotSharpeOfMethod[0] > m_map_TotSharpeOfMethod[1])
      {
        dAvgPxMethWeight = max(m_map_TotSharpeOfMethod[0],(double)0) / dTotalSharpe;
        dCloseMethWeight = 1-dAvgPxMethWeight;
      }
      else
      {
        dCloseMethWeight = max(m_map_TotSharpeOfMethod[1],(double)0) / dTotalSharpe;
        dAvgPxMethWeight = 1-dCloseMethWeight;
      }
    }

    m_Logger->Write(Logger::INFO,"Strategy %s: %s Sym=%s dAvgPxMethWeight = %f dCloseMethWeight = %f",
                    GetStrategyName(m_StyID).c_str(), m_p_ymdhms_SysTime_Local->ToStr().c_str(),m_TradedSymbols[iTradSym].c_str(),dAvgPxMethWeight,dCloseMethWeight);

    if (m_TrainingPeriod1[iTradSym] != 0 || m_TrainingPeriod2[iTradSym] != 0 || m_TrainingPeriod3[iTradSym] != 0 || m_TrainingPeriod4[iTradSym] != 0)
    {
      m_dAggSignedQty =
        dAvgPxMethWeight * round(m_dAggUnsignedQty * m_dStrengthCountAvgPx / (m_PropOfBestParam[iTradSym] / (double)100 * (double)m_lNumOfTrngCombnAvgPx))
        +
        dCloseMethWeight * round(m_dAggUnsignedQty * m_dStrengthCountClose / (m_PropOfBestParam[iTradSym] / (double)100 * (double)m_lNumOfTrngCombnClose))
        ;
    }
    else
    {
      m_dAggSignedQty =
        dAvgPxMethWeight * round(m_dAggUnsignedQty) * (m_dStrengthCountAvgPx >= 0 ? 1 : -1)
        +
        dCloseMethWeight * round(m_dAggUnsignedQty) * (m_dStrengthCountClose >= 0 ? 1 : -1)
        ;
    }
  }
  else
  {
    m_dAggSignedQty =
      round(m_dAggUnsignedQty * m_dStrengthCountAvgPx / (m_PropOfBestParam[iTradSym] / (double)100 * (double)m_lNumOfTrngCombnAvgPx));
  }

  //--------------------------------------------------
  if (B2_SKIPMACHLEARNING) m_dAggSignedQty = round(m_NotionalAmt[iTradSym] / m_SymMidQuote);
  //--------------------------------------------------

  //--------------------------------------------------
  // Taylor has 2 hypothesis
  //--------------------------------------------------
  if (B2_HYPOTHESIS == B2_HYPOTHESIS_TAYLOR) m_dAggSignedQty = m_dAggSignedQty / 2.0;
  //--------------------------------------------------

  //--------------------------------------------------
  // KolmogorovSmirnov
  //--------------------------------------------------
  if (m_KSBound[iTradSym] > 0 &&
      m_KSRange[iTradSym] > 0 &&
      m_HistoricalAvgPxRtn->size() >= m_TrainingPeriodMax[iTradSym] &&
      //--------------------------------------------------
      // only adjust size when price has gone down
      //--------------------------------------------------
      m_HistoricalAvgPx->size() > B2_KS_PRICE_FILTER_NDAYS+2
      //--------------------------------------------------
     )
  {
    vector<double> vdPeriod1;
    vector<double> vdPeriod2;
    vdPeriod1.insert(vdPeriod1.begin(),m_HistoricalAvgPxRtn->end()-m_TrainingPeriod1[iTradSym],m_HistoricalAvgPxRtn->end()-m_TrainingPeriod1[iTradSym]/2);
    vdPeriod2.insert(vdPeriod2.begin(),m_HistoricalAvgPxRtn->end()-m_TrainingPeriod1[iTradSym]/2,m_HistoricalAvgPxRtn->end());

    double ks = KolmogorovSmirnov::TwoSmpKSStat(vdPeriod1,vdPeriod2,20);
    m_Logger->Write(Logger::INFO,"Strategy %s: %s Sym=%s vdPeriod1.size() = %d vdPeriod2.size() = %d KolmogorovSmirnov = %f",
                    GetStrategyName(m_StyID).c_str(), m_p_ymdhms_SysTime_Local->ToStr().c_str(),m_TradedSymbols[iTradSym].c_str(),
                    vdPeriod1.size(), vdPeriod2.size(), ks);

    m_Logger->Write(Logger::INFO,"Strategy %s: %s Sym=%s m_dAggSignedQty (before KolmogorovSmirnov adjustment)  %f",
                    GetStrategyName(m_StyID).c_str(), m_p_ymdhms_SysTime_Local->ToStr().c_str(),m_TradedSymbols[iTradSym].c_str(), m_dAggSignedQty);


    if (m_HistoricalAvgPx->back() < *(m_HistoricalAvgPx->end() - B2_KS_PRICE_FILTER_NDAYS - 1))
      m_dAggSignedQty = m_dAggSignedQty * (1 - B2_KS_PRICE_FILTER_FALL_ADJFACTOR) + m_dAggSignedQty * (B2_KS_PRICE_FILTER_FALL_ADJFACTOR) * (min(max(m_KSBound[iTradSym]-ks,(double)0),m_KSRange[iTradSym])) / m_KSRange[iTradSym];
    else
      m_dAggSignedQty = m_dAggSignedQty * (1 - B2_KS_PRICE_FILTER_RISE_ADJFACTOR) + m_dAggSignedQty * (B2_KS_PRICE_FILTER_RISE_ADJFACTOR) * (min(max(m_KSBound[iTradSym]-ks,(double)0),m_KSRange[iTradSym])) / m_KSRange[iTradSym];

    m_Logger->Write(Logger::INFO,"Strategy %s: %s Sym=%s m_dAggSignedQty (after KolmogorovSmirnov adjustment)  %f",
                    GetStrategyName(m_StyID).c_str(), m_p_ymdhms_SysTime_Local->ToStr().c_str(),m_TradedSymbols[iTradSym].c_str(), m_dAggSignedQty);
  }
  //--------------------------------------------------

  //--------------------------------------------------
  // Adhoc filter: Filter away stocks with a declining trend
  //--------------------------------------------------
  if (!m_B2_FilterSMAPeriod.empty())
  {
    //--------------------------------------------------
    // short SMA long SMA
    //      1       -1      trade
    //     -1       -1      not trade
    //     -1        1      trade
    //      1        1      trade
    //--------------------------------------------------
    if (m_RotationMode == 1
        &&
        m_B2_FilterSMAPeriod.size() == 2
        &&
        m_v_SMA_short[iTradSym].Ready()
        &&
        m_v_SMA_long[iTradSym].Ready()
        &&
        m_HistoricalAvgPx->back() < m_v_SMA_short[iTradSym].Value()
        &&
        m_HistoricalAvgPx->back() < m_v_SMA_long[iTradSym].Value())
    {
      m_Logger->Write(Logger::INFO,"Strategy %s: %s Sym=%s m_dAggSignedQty is set to zero because of the SMA filters. m_dAggSignedQty (bef adj) = %f Avg Px = %f SMA = %f SMA = %f",
                      GetStrategyName(m_StyID).c_str(), m_p_ymdhms_SysTime_Local->ToStr().c_str(),m_TradedSymbols[iTradSym].c_str(),
                      m_dAggSignedQty, m_HistoricalAvgPx->back(), m_v_SMA_short[iTradSym].Value(), m_v_SMA_long[iTradSym].Value());
      m_dAggSignedQty = 0;
    }
    else if (m_RotationMode == 1
             &&
             m_B2_FilterSMAPeriod.size() == 1
             &&
             m_v_SMA_short[iTradSym].Ready()
             &&
             m_v_SMA_long[iTradSym].Ready()
             &&
             m_HistoricalAvgPx->back() < m_v_SMA_long[iTradSym].Value())
    {
      m_Logger->Write(Logger::INFO,"Strategy %s: %s Sym=%s m_dAggSignedQty is set to zero because of the SMA filters. m_dAggSignedQty (bef adj) = %f Avg Px = %f SMA = %f",
                      GetStrategyName(m_StyID).c_str(), m_p_ymdhms_SysTime_Local->ToStr().c_str(),m_TradedSymbols[iTradSym].c_str(),
                      m_dAggSignedQty, m_HistoricalAvgPx->back(), m_v_SMA_long[iTradSym].Value());
      m_dAggSignedQty = 0;
    }
    else if (m_RotationMode == -1
             &&
             m_B2_FilterSMAPeriod.size() == 2
             &&
             m_v_SMA_short[iTradSym].Ready()
             &&
             m_v_SMA_long[iTradSym].Ready()
             &&
             m_HistoricalAvgPx->back() > m_v_SMA_short[iTradSym].Value()
             &&
             m_HistoricalAvgPx->back() > m_v_SMA_long[iTradSym].Value())
    {
      m_Logger->Write(Logger::INFO,"Strategy %s: %s Sym=%s m_dAggSignedQty is set to zero because of the SMA filters. m_dAggSignedQty (bef adj) = %f Avg Px = %f SMA = %f SMA = %f",
                      GetStrategyName(m_StyID).c_str(), m_p_ymdhms_SysTime_Local->ToStr().c_str(),m_TradedSymbols[iTradSym].c_str(),
                      m_dAggSignedQty, m_HistoricalAvgPx->back(), m_v_SMA_short[iTradSym].Value(), m_v_SMA_long[iTradSym].Value());
      m_dAggSignedQty = 0;
    }
    else if (m_RotationMode == -1
             &&
             m_B2_FilterSMAPeriod.size() == 1
             &&
             m_v_SMA_long[iTradSym].Ready()
             &&
             m_HistoricalAvgPx->back() > m_v_SMA_long[iTradSym].Value())
    {
      m_Logger->Write(Logger::INFO,"Strategy %s: %s Sym=%s m_dAggSignedQty is set to zero because of the SMA filters. m_dAggSignedQty (bef adj) = %f Avg Px = %f SMA = %f",
                      GetStrategyName(m_StyID).c_str(), m_p_ymdhms_SysTime_Local->ToStr().c_str(),m_TradedSymbols[iTradSym].c_str(),
                      m_dAggSignedQty, m_HistoricalAvgPx->back(), m_v_SMA_long[iTradSym].Value());
      m_dAggSignedQty = 0;
    }
    else if (m_B2_FilterSMAPeriod.size() == 2 && m_v_SMA_long[iTradSym].Ready())
    {
      m_Logger->Write(Logger::INFO,"Strategy %s: %s Sym=%s m_dAggSignedQty = %f Avg Px = %f SMA = %f SMA = %f",
                      GetStrategyName(m_StyID).c_str(), m_p_ymdhms_SysTime_Local->ToStr().c_str(),m_TradedSymbols[iTradSym].c_str(),
                      m_dAggSignedQty, m_HistoricalAvgPx->back(), m_v_SMA_short[iTradSym].Value(), m_v_SMA_long[iTradSym].Value());
    }
    else if (m_B2_FilterSMAPeriod.size() == 1 && m_v_SMA_long[iTradSym].Ready())
    {
      m_Logger->Write(Logger::INFO,"Strategy %s: %s Sym=%s m_dAggSignedQty = %f Avg Px = %f SMA = %f",
                      GetStrategyName(m_StyID).c_str(), m_p_ymdhms_SysTime_Local->ToStr().c_str(),m_TradedSymbols[iTradSym].c_str(),
                      m_dAggSignedQty, m_HistoricalAvgPx->back(), m_v_SMA_long[iTradSym].Value());
    }
  }
  //--------------------------------------------------

  //--------------------------------------------------
  // Allow Trade Direction
  //--------------------------------------------------
  if      (m_AllowTrdDir[iTradSym] ==  1) m_dAggSignedQty = max(m_dAggSignedQty,(double)0);
  else if (m_AllowTrdDir[iTradSym] == -1) m_dAggSignedQty = min(m_dAggSignedQty,(double)0);


  //--------------------------------------------------
  // Close price relative to average price
  //--------------------------------------------------
  if (m_LongOnlyWhenClosePriceBelowAvgPrice.IsSome()
      &&
      STool::IsValidPriceOrVol(m_SymMidQuote) && !m_HistoricalAvgPx->empty())
  {
    double dClosePxAgstAvgPx = m_SymMidQuote / m_HistoricalAvgPx->back() - 1;
    if (dClosePxAgstAvgPx > m_LongOnlyWhenClosePriceBelowAvgPrice.Get())
    {
      m_Logger->Write(Logger::INFO,"Strategy %s: %s Sym=%s m_dAggSignedQty is set to min(%f, 0). dClosePxAgstAvgPx %f",
                      GetStrategyName(m_StyID).c_str(), m_p_ymdhms_SysTime_Local->ToStr().c_str(), m_TradedSymbols[iTradSym].c_str(),
                      m_dAggSignedQty, dClosePxAgstAvgPx);
      m_dAggSignedQty = min(m_dAggSignedQty,0.0);
    }
  }
  if (m_ShortOnlyWhenClosePriceAboveAvgPrice.IsSome()
      &&
      STool::IsValidPriceOrVol(m_SymMidQuote) && !m_HistoricalAvgPx->empty())
  {
    double dClosePxAgstAvgPx = m_SymMidQuote / m_HistoricalAvgPx->back() - 1;
    if (dClosePxAgstAvgPx < m_ShortOnlyWhenClosePriceAboveAvgPrice.Get())
    {
      m_Logger->Write(Logger::INFO,"Strategy %s: %s Sym=%s m_dAggSignedQty is set to max(%f, 0). dClosePxAgstAvgPx %f",
                      GetStrategyName(m_StyID).c_str(), m_p_ymdhms_SysTime_Local->ToStr().c_str(), m_TradedSymbols[iTradSym].c_str(),
                      m_dAggSignedQty, dClosePxAgstAvgPx);
      m_dAggSignedQty = max(m_dAggSignedQty,0.0);
    }
  }

  //--------------------------------------------------
  // Average price return
  //--------------------------------------------------
  if (m_LongOnlyWhenAvgPriceReturnAbove.IsSome()
      &&
      m_HistoricalAvgPx->size() >= 2)
  {
    double dAvgPxRtn = m_HistoricalAvgPx->back() / *(m_HistoricalAvgPx->end()-2) - 1;
    if (dAvgPxRtn < m_LongOnlyWhenAvgPriceReturnAbove.Get())
    {
      m_Logger->Write(Logger::INFO,"Strategy %s: %s Sym=%s m_dAggSignedQty is set to min(%f, 0). dAvgPxRtn %f",
                      GetStrategyName(m_StyID).c_str(), m_p_ymdhms_SysTime_Local->ToStr().c_str(), m_TradedSymbols[iTradSym].c_str(),
                      m_dAggSignedQty, dAvgPxRtn);
      m_dAggSignedQty = min(m_dAggSignedQty,0.0);
    }
  }
  if (m_ShortOnlyWhenAvgPriceReturnBelow.IsSome()
      &&
      m_HistoricalAvgPx->size() >= 2)
  {
    double dAvgPxRtn = m_HistoricalAvgPx->back() / *(m_HistoricalAvgPx->end()-2) - 1;
    if (dAvgPxRtn > m_ShortOnlyWhenAvgPriceReturnBelow.Get())
    {
      m_Logger->Write(Logger::INFO,"Strategy %s: %s Sym=%s m_dAggSignedQty is set to max(%f, 0). dAvgPxRtn %f",
                      GetStrategyName(m_StyID).c_str(), m_p_ymdhms_SysTime_Local->ToStr().c_str(), m_TradedSymbols[iTradSym].c_str(),
                      m_dAggSignedQty, dAvgPxRtn);
      m_dAggSignedQty = max(m_dAggSignedQty,0.0);
    }
  }



  // //--------------------------------------------------
  // // Special ES + SPY mode
  // //--------------------------------------------------
  // if (m_ES_SPY_Together_SPYidx >= 0)
  // {
  //   if (m_TradedSymbols[iTradSym] == ES_CONT_FUT_1)
  //   {
  //
  //     if (!m_map_DoneEODTrade[m_TradedSymbols[m_ES_SPY_Together_SPYidx]])
  //     {
  //       m_Logger->Write(Logger::INFO,"Strategy %s: %s %s Not trading %s yet because SPY is not traded yet.",
  //                       GetStrategyName(m_StyID).c_str(), m_p_ymdhms_SysTime_Local->ToStr().c_str(),m_TradedSymbols[iTradSym].c_str(), ES_CONT_FUT_1);
  //       //--------------------------------------------------
  //       // return to wait for the next loop
  //       //--------------------------------------------------
  //       return;
  //     }
  //
  //     m_dAggSignedQty = m_NoOfSgndESCtrtReqd;
  //
  //     m_Logger->Write(Logger::INFO,"Strategy %s: %s %s m_NoOfSgndESCtrtReqd = %d.",
  //                     GetStrategyName(m_StyID).c_str(), m_p_ymdhms_SysTime_Local->ToStr().c_str(),m_TradedSymbols[iTradSym].c_str(), m_NoOfSgndESCtrtReqd);
  //
  //   }
  //   else if (m_TradedSymbols[iTradSym] == "SPY")
  //   {
  //     m_SPY_Px = m_SymMidQuote;
  //
  //     double dESFM1Px = NAN;
  //
  //     if (!std::isnan(m_SymFM1MidQuote))
  //     {
  //       dESFM1Px = m_SymFM1MidQuote;
  //     }
  //     else
  //     {
  //       //--------------------------------------------------
  //       // Get estimate of its price from day bar
  //       //--------------------------------------------------
  //       vector<YYYYMMDD> vYMD;
  //       vector<double>   vOpen;
  //       vector<double>   vHigh;
  //       vector<double>   vLow;
  //       vector<double>   vClose;
  //       vector<long>     vVol;
  //
  //       MarketData::Instance()->GetSuppD1BarOHLCVInDateRange(ES_CONT_FUT_1, YYYYMMDD(19000101), m_p_ymdhms_SysTime_Local->GetYYYYMMDD(), vYMD, vOpen, vHigh, vLow, vClose, vVol);
  //
  //       if (!vClose.empty()) dESFM1Px = vClose.back();
  //     }
  //
  //
  //     if (!std::isnan(dESFM1Px))
  //     {
  //       double dESMul = m_CME->GetContractMultiplier(ES_CONT_FUT_1);
  //
  //       double dTotalSgndNotional = (m_SPY_Px * m_dAggSignedQty);
  //       double dNotionalAmtPerES  = (dESFM1Px * dESMul);
  //
  //       m_NoOfSgndESCtrtReqd = STool::Sign(m_dAggSignedQty) * (int)(floor(abs(dTotalSgndNotional) / dNotionalAmtPerES));
  //       double dSPY_SgndNotional  = dTotalSgndNotional  -  (dESFM1Px * dESMul * m_NoOfSgndESCtrtReqd);
  //
  //       //--------------------------------------------------
  //       // The new SPY quantity
  //       //--------------------------------------------------
  //       m_dAggSignedQty = dSPY_SgndNotional / m_SPY_Px;
  //
  //       //--------------------------------------------------
  //       m_Logger->Write(Logger::INFO,"Strategy %s: %s %s dTotalSgndNotional = %f dESFM1Px = %f dESMul = %f dNotionalAmtPerES = %f m_NoOfSgndESCtrtReqd = %d SPY m_dAggSignedQty = %f ",
  //                       GetStrategyName(m_StyID).c_str(), m_p_ymdhms_SysTime_Local->ToStr().c_str(),m_TradedSymbols[iTradSym].c_str(),
  //                       dTotalSgndNotional,
  //                       dESFM1Px,
  //                       dESMul,
  //                       dNotionalAmtPerES,
  //                       m_NoOfSgndESCtrtReqd,
  //                       m_dAggSignedQty);
  //     }
  //     else
  //     {
  //       m_Logger->Write(Logger::ERROR,"Strategy %s: %s %s %s price can't be obtained. Falling back to normal SPY only mode.",
  //                       GetStrategyName(m_StyID).c_str(), m_p_ymdhms_SysTime_Local->ToStr().c_str(),m_TradedSymbols[iTradSym].c_str(), ES_CONT_FUT_1);
  //     }
  //
  //   }
  // }
  // //--------------------------------------------------

  //--------------------------------------------------
  // Rotation
  //--------------------------------------------------
  if (m_RotationMode != 0)
  {
    //--------------------------------------------------
    // check whether there is enough historical data
    //--------------------------------------------------
    if (m_HistoricalAvgPx->size() < m_RotationModeUseNDayReturn)
    {
      m_Logger->Write(Logger::INFO,"Strategy %s: Rotation mode: %s Sym=%s Not enough data points to calculate %d-day return. %d",
                      GetStrategyName(m_StyID).c_str(),
                      m_p_ymdhms_SysTime_Local->ToStr().c_str(),
                      m_TradedSymbols[iTradSym].c_str(),
                      m_RotationModeUseNDayReturn,
                      m_HistoricalAvgPx->size());

      m_TargetTradeDir [iTradSym] = TD_NODIR;
      m_TargetAbsPos   [iTradSym] = 0;
      *m_DoneEODTrade = true;

      return;
    }

    //--------------------------------------------------
    // calculate rolling return
    //--------------------------------------------------
    double dRollingReturn = (m_HistoricalAvgPx->back() / *(m_HistoricalAvgPx->end()-1-m_RotationModeUseNDayReturn)) - 1;
    m_Logger->Write(Logger::INFO,"Strategy %s: Rotation mode: %s Sym=%s dRollingReturn (before voly adj) = %f m_HistoricalAvgPx %f %f",
                    GetStrategyName(m_StyID).c_str(),
                    m_p_ymdhms_SysTime_Local->ToStr().c_str(),
                    m_TradedSymbols[iTradSym].c_str(),
                    dRollingReturn,
                    m_HistoricalAvgPx->back(),
                    *(m_HistoricalAvgPx->end()-1-m_RotationModeUseVolyAdjdReturn));
    if (m_RotationModeUseVolyAdjdReturn)
    {
      dRollingReturn = dRollingReturn / TechIndicators::Instance()->GetGKYZValue(m_TradedSymbols[iTradSym]) * 100.0;
    }
    m_Logger->Write(Logger::INFO,"Strategy %s: Rotation mode: %s Sym=%s dRollingReturn (after voly adj) = %f",
                    GetStrategyName(m_StyID).c_str(),
                    m_p_ymdhms_SysTime_Local->ToStr().c_str(),
                    m_TradedSymbols[iTradSym].c_str(),
                    dRollingReturn);

    //--------------------------------------------------
    // sticky group: get the groups whose position we will maintain
    // sticky stock: get the set of symbol whose position we will maintain (within the groups)
    //--------------------------------------------------
    if (
      (
        m_RotationMode == 1
        &&
        GetPrevTheoSgndPos(m_TradedSymbols[iTradSym]) > 0
        &&
        m_dAggSignedQty > 0
      )
      || 
      (
        m_RotationMode == -1
        &&
        GetPrevTheoSgndPos(m_TradedSymbols[iTradSym]) < 0
        &&
        m_dAggSignedQty < 0
      )
      )
    {
      {
        //--------------------------------------------------
        // sticky stock within group
        //--------------------------------------------------
        map<YYYYMMDD,set<string> >::iterator it = m_MaintainPosWithinGrp[m_RotationGroup[iTradSym]].find(m_p_ymdhms_SysTime_Local->GetYYYYMMDD());
        if (it == m_MaintainPosWithinGrp[m_RotationGroup[iTradSym]].end())
        {
          m_MaintainPosWithinGrp[m_RotationGroup[iTradSym]][m_p_ymdhms_SysTime_Local->GetYYYYMMDD()] = set<string>();
          it = m_MaintainPosWithinGrp[m_RotationGroup[iTradSym]].find(m_p_ymdhms_SysTime_Local->GetYYYYMMDD());
        }
        it->second.insert(m_TradedSymbols[iTradSym]);
        m_Logger->Write(Logger::INFO,"Strategy %s: Rotation mode: %s Sym=%s has a position that we will maintain. Previous pos = %f Current (raw) m_dAggSignedQty = %f",
                        GetStrategyName(m_StyID).c_str(),
                        m_p_ymdhms_SysTime_Local->ToStr().c_str(),
                        m_TradedSymbols[iTradSym].c_str(),
                        GetPrevTheoSgndPos(m_TradedSymbols[iTradSym]),
                        m_dAggSignedQty);
      }

      {
        //--------------------------------------------------
        // sticky group
        //--------------------------------------------------
        map<YYYYMMDD,set<int> >::iterator it = m_MaintainGrp.find(m_p_ymdhms_SysTime_Local->GetYYYYMMDD());
        if (it == m_MaintainGrp.end())
        {
          m_MaintainGrp[m_p_ymdhms_SysTime_Local->GetYYYYMMDD()] = set<int>();
          it = m_MaintainGrp.find(m_p_ymdhms_SysTime_Local->GetYYYYMMDD());
        }
        set<int> & setMtnGrp = it->second;
        setMtnGrp.insert(m_RotationGroup[iTradSym]);

        FForEach(setMtnGrp,[&](const int grp) {
          m_Logger->Write(Logger::INFO,"Strategy %s: Rotation mode: %s Sym=%s Sticky Group %d",
                          GetStrategyName(m_StyID).c_str(),
                          m_p_ymdhms_SysTime_Local->ToStr().c_str(),
                          m_TradedSymbols[iTradSym].c_str(),
                          grp);
        });
      }

    }

    //--------------------------------------------------
    // save rolling return and m_dAggSignedQty
    //--------------------------------------------------
    AddNDayRollingReturn(m_RotationGroup[iTradSym],
                         m_p_ymdhms_SysTime_Local->GetYYYYMMDD(),
                         m_TradedSymbols[iTradSym],
                         dRollingReturn);
    AddAggSgndQty(m_p_ymdhms_SysTime_Local->GetYYYYMMDD(),
                  m_TradedSymbols[iTradSym],
                  m_dAggSignedQty);
    AddAggSgndNotnl(m_p_ymdhms_SysTime_Local->GetYYYYMMDD(),
                    m_RotationGroup[iTradSym],
                    m_TradedSymbols[iTradSym],
                    m_SymMidQuote * m_dAggSignedQty);

    //--------------------------------------------------
    // check if all rolling returns are available
    //--------------------------------------------------
    if (!NDayRollingReturnReadyForAllSym(m_RotationGroup[iTradSym],m_p_ymdhms_SysTime_Local->GetYYYYMMDD()))
    {
      m_Logger->Write(Logger::INFO,"Strategy %s: Rotation mode: %s Sym=%s Not all %d-day returns are ready.",
                      GetStrategyName(m_StyID).c_str(),
                      m_p_ymdhms_SysTime_Local->ToStr().c_str(),
                      m_TradedSymbols[iTradSym].c_str(),
                      m_RotationModeUseNDayReturn);
      return;
    }
    m_Logger->Write(Logger::INFO,"Strategy %s: Rotation mode: %s Sym=%s All %d-day returns are ready.",
                    GetStrategyName(m_StyID).c_str(),
                    m_p_ymdhms_SysTime_Local->ToStr().c_str(),
                    m_TradedSymbols[iTradSym].c_str(),
                    m_RotationModeUseNDayReturn);

    //--------------------------------------------------
    // check if all aggregate signed quantity is available
    //--------------------------------------------------
    if (!AggSgndQtyReadyForAllSym(m_p_ymdhms_SysTime_Local->GetYYYYMMDD()))
    {
      m_Logger->Write(Logger::INFO,"Strategy %s: Rotation mode: %s Sym=%s Not all aggregate signed qty are ready.",
                      GetStrategyName(m_StyID).c_str(),
                      m_p_ymdhms_SysTime_Local->ToStr().c_str(),
                      m_TradedSymbols[iTradSym].c_str());
      return;
    }
    m_Logger->Write(Logger::INFO,"Strategy %s: Rotation mode: %s Sym=%s All aggregate signed qty are ready.",
                    GetStrategyName(m_StyID).c_str(),
                    m_p_ymdhms_SysTime_Local->ToStr().c_str(),
                    m_TradedSymbols[iTradSym].c_str());

    //--------------------------------------------------
    // check if we have updated internal data FOR ALL symbols before generating signals
    //--------------------------------------------------
    if (m_AllSymWithUpdateData[m_p_ymdhms_SysTime_Local->GetYYYYMMDD()].size() <
        m_AllAvbSym[m_p_ymdhms_SysTime_Local->GetYYYYMMDD()].size())
    {
      m_Logger->Write(Logger::INFO,"Strategy %s: %s Sym=%s Not all symbols have up-to-date internal data",
                      GetStrategyName(m_StyID).c_str(),
                      m_p_ymdhms_SysTime_Local->ToStr().c_str(),
                      m_TradedSymbols[iTradSym].c_str());
      FForEach(m_AllSymWithUpdateData[m_p_ymdhms_SysTime_Local->GetYYYYMMDD()],[&](const string & sym) {
        m_Logger->Write(Logger::INFO,"Strategy %s: %s Sym=%s With update internal data",
                        GetStrategyName(m_StyID).c_str(),
                        m_p_ymdhms_SysTime_Local->ToStr().c_str(),
                        sym.c_str());
      });
      return;
    }

    //--------------------------------------------------
    // if not performed the rotation logic, do it now, but do it once only
    //--------------------------------------------------
    if (!m_HasPerformedRotationLogic.Contains(m_p_ymdhms_SysTime_Local->GetYYYYMMDD()))
    {
      //--------------------------------------------------
      // check the average return of the groups
      //--------------------------------------------------
      {
        map<YYYYMMDD,vector<double> >::iterator it = m_AvgRtnOfGrp.find(m_p_ymdhms_SysTime_Local->GetYYYYMMDD());
        if (it == m_AvgRtnOfGrp.end())
        {
          vector<double> vGrpRtn;
          vGrpRtn.insert(vGrpRtn.begin(), m_RotationGroup.back()+1, NAN);
          m_AvgRtnOfGrp[m_p_ymdhms_SysTime_Local->GetYYYYMMDD()] = vGrpRtn;
          it = m_AvgRtnOfGrp.find(m_p_ymdhms_SysTime_Local->GetYYYYMMDD());
        }

        vector<double> & vGrpRtn = it->second;

        FForEach(m_RotationGroup,[&](const int grp){
          if (std::isnan(vGrpRtn[grp])) vGrpRtn[grp] = GetAvgRollgRetOfGrp(grp, m_p_ymdhms_SysTime_Local->GetYYYYMMDD());
        });

        m_Logger->Write(Logger::INFO,"Strategy %s: Rotation mode: --- Group Avg Return ---", GetStrategyName(m_StyID).c_str());
        for (int grp = 0; grp < vGrpRtn.size(); ++grp)
        {
          m_Logger->Write(Logger::INFO,"Strategy %s: Rotation mode: %s Group %d Avg Return %f",
                          GetStrategyName(m_StyID).c_str(),
                          m_p_ymdhms_SysTime_Local->ToStr().c_str(),
                          grp, vGrpRtn[grp]);
        }
        m_Logger->Write(Logger::INFO,"Strategy %s: Rotation mode: --- Group Avg Return ---", GetStrategyName(m_StyID).c_str());
      }


      {
        map<YYYYMMDD,vector<Option<string> > >::iterator it = m_RttnGrpWithSgnl.find(m_p_ymdhms_SysTime_Local->GetYYYYMMDD());
        if (it == m_RttnGrpWithSgnl.end())
        {
          vector<Option<string> > vSymWithSgnl;
          vSymWithSgnl.insert(vSymWithSgnl.begin(), m_RotationGroup.back()+1, Option<string>());
          m_RttnGrpWithSgnl[m_p_ymdhms_SysTime_Local->GetYYYYMMDD()] = vSymWithSgnl;
          it = m_RttnGrpWithSgnl.find(m_p_ymdhms_SysTime_Local->GetYYYYMMDD());
        }

        vector<Option<string> > & vSymWithSgnl = it->second;

        for (int iTradSymRttnGrp = 0; iTradSymRttnGrp < m_TradedSymbols.size(); ++iTradSymRttnGrp)
        {
          //--------------------------------------------------
          // pick stocks for trading in each group
          //--------------------------------------------------
          const set<string> & setMtnPsn = m_MaintainPosWithinGrp[m_RotationGroup[iTradSymRttnGrp]][m_p_ymdhms_SysTime_Local->GetYYYYMMDD()];

          //--------------------------------------------------
          // those maintained stocks have precedence over others
          //--------------------------------------------------
          if (setMtnPsn.find(m_TradedSymbols[iTradSymRttnGrp]) != setMtnPsn.end())
          {
            vSymWithSgnl[m_RotationGroup[iTradSymRttnGrp]] = Option<string>(m_TradedSymbols[iTradSymRttnGrp]);
          }
        }

        for (int iRttnGrp = m_RotationGroup.front(); iRttnGrp < m_RotationGroup.back()+1; ++iRttnGrp)
        {
          int iRank = 1;
          while (
            vSymWithSgnl[iRttnGrp].IsNone()
            &&
            iRank <=
            m_AllAvbSymForRollingBasket[iRttnGrp][m_p_ymdhms_SysTime_Local->GetYYYYMMDD()].size()
            )
          {
            ASC_DESC ad;
            if (m_RotationMode == 1)
              ad = AD_DESCENDING;
            else if (m_RotationMode == -1)
              ad = AD_ASCENDING;

            Option<string> o_sym = GetSymInGrpRankByRet(iRttnGrp,
                                                        m_p_ymdhms_SysTime_Local->GetYYYYMMDD(),
                                                        iRank,
                                                        ad);

            if (o_sym.IsSome())
            {
              double dAggSgndQty = m_SymAggSgndQty[m_p_ymdhms_SysTime_Local->GetYYYYMMDD()][o_sym.GetOrElse("")];

              if (
                (
                  (m_RotationMode == 1 && dAggSgndQty > 0)
                  ||
                  (m_RotationMode == -1 && dAggSgndQty < 0)
                )
                &&
                (m_mapRoleOfSym[o_sym.Get()] > 0) // not indicator symbol
                )
              {
                vSymWithSgnl[iRttnGrp] = o_sym;
              }
            }
            iRank++;

            //--------------------------------------------------
            // if the option isn't set, we don't move the next best stock forward for trading
            //--------------------------------------------------
            if (!m_MoveNextBestStkInGrpUpIfNoSignal && iRank > B2_ROTATION_PICKTOPSYM) break;
            //--------------------------------------------------

            m_Logger->Write(Logger::INFO,"Strategy %s: Rotation mode: Rotation group %d Rank %d m_AllAvbSymForRollingBasket.size() %d vSymWithSgnl %s",
                            GetStrategyName(m_StyID).c_str(),
                            iRttnGrp,
                            iRank,
                            m_AllAvbSymForRollingBasket[iRttnGrp][m_p_ymdhms_SysTime_Local->GetYYYYMMDD()].size(),
                            vSymWithSgnl[iRttnGrp].GetOrElse("___").c_str()
                           );
          }

          FForEach(m_AllAvbSymForRollingBasket[iRttnGrp][m_p_ymdhms_SysTime_Local->GetYYYYMMDD()], [&](const string & s) {
            m_Logger->Write(Logger::INFO,"Strategy %s: Rotation mode: Available symbol for the group %d: %s", GetStrategyName(m_StyID).c_str(), iRttnGrp, s.c_str());
          });

        }

        m_Logger->Write(Logger::INFO,"Strategy %s: Rotation mode: --- Symbol with signal ---", GetStrategyName(m_StyID).c_str());
        for (int grp = 0; grp < vSymWithSgnl.size(); ++grp)
        {
          m_Logger->Write(Logger::INFO,"Strategy %s: Rotation mode: %s Group %d Symbol with signal %s",
                          GetStrategyName(m_StyID).c_str(),
                          m_p_ymdhms_SysTime_Local->ToStr().c_str(),
                          grp, vSymWithSgnl[grp].GetOrElse("___").c_str());
        }
        m_Logger->Write(Logger::INFO,"Strategy %s: Rotation mode: --- Symbol with signal ---", GetStrategyName(m_StyID).c_str());

        for (int iRttnGrp = m_RotationGroup.front(); iRttnGrp < m_RotationGroup.back()+1; ++iRttnGrp)
        {
          double dAvgSgndNotnlOfRttnGrp = GetAvgAggSgndNotnlOfGrp(iRttnGrp,m_p_ymdhms_SysTime_Local->GetYYYYMMDD());
          m_Logger->Write(Logger::INFO,"Strategy %s: Rotation mode: %s Group %d Avg Sgnd Notional of Grp %f",
                          GetStrategyName(m_StyID).c_str(),
                          m_p_ymdhms_SysTime_Local->ToStr().c_str(),
                          iRttnGrp, dAvgSgndNotnlOfRttnGrp);

          if (
            (m_RotationMode == 1 && dAvgSgndNotnlOfRttnGrp < m_AvgAggSgndNotnlThresh)
            ||
            (m_RotationMode == -1 && dAvgSgndNotnlOfRttnGrp > m_AvgAggSgndNotnlThresh)
            )
          {
            //--------------------------------------------------
            // check the average signed notional of each group,
            // if it is smaller than the threshold, then remove all the symbols from the group
            //--------------------------------------------------
            vSymWithSgnl[iRttnGrp] = Option<string>();
          }
          //--------------------------------------------------
          // check if the group has any representative
          //--------------------------------------------------
          else if (m_vGroupRepresentative[iRttnGrp].IsSome())
          {
            vSymWithSgnl[iRttnGrp] = m_vGroupRepresentative[iRttnGrp];
            m_Logger->Write(Logger::INFO,"Strategy %s: Rotation mode: %s Group %d Setting symbol with signal with the representative group %s",
                            GetStrategyName(m_StyID).c_str(),
                            m_p_ymdhms_SysTime_Local->ToStr().c_str(),
                            iRttnGrp,
                            m_vGroupRepresentative[iRttnGrp].Get().c_str());
          }

        }
        //--------------------------------------------------

      }

      //--------------------------------------------------
      // sort the groups (with stock positions) by their returns
      //--------------------------------------------------
      {
        map<YYYYMMDD,vector<Option<string> > >::iterator it1 = m_RttnGrpWithSgnl.find(m_p_ymdhms_SysTime_Local->GetYYYYMMDD());
        map<YYYYMMDD,vector<double> >::iterator it2 = m_AvgRtnOfGrp.find(m_p_ymdhms_SysTime_Local->GetYYYYMMDD());

        vector<Option<string> > & vSymWithSgnl = it1->second;
        vector<double> & vGrpRtn = it2->second;

        map<YYYYMMDD,vector<TupRetSym> >::iterator it = m_GrpRtnAndLeadingSym.find(m_p_ymdhms_SysTime_Local->GetYYYYMMDD());
        if (it == m_GrpRtnAndLeadingSym.end())
        {
          vector<TupRetSym> mGrpRtnAndLeadSym;
          m_GrpRtnAndLeadingSym[m_p_ymdhms_SysTime_Local->GetYYYYMMDD()] = mGrpRtnAndLeadSym;
          it  = m_GrpRtnAndLeadingSym.find(m_p_ymdhms_SysTime_Local->GetYYYYMMDD());
        }

        vector<TupRetSym> & mGrpRtnAndLeadSym = it->second;

        for (int grp = m_RotationGroup.front(); grp < m_RotationGroup.back()+1; ++grp)
        {
          // if (vSymWithSgnl[grp].IsNone()) continue;
          if (std::isnan(vGrpRtn[grp])) continue;
          mGrpRtnAndLeadSym.push_back(TupRetSym(vGrpRtn[grp],vSymWithSgnl[grp].GetOrElse(""),grp));
        }

        //--------------------------------------------------
        // pick only the top groups with signals
        //--------------------------------------------------
        if (!mGrpRtnAndLeadSym.empty())
        {

          m_Logger->Write(Logger::INFO,"Strategy %s: Rotation mode: --- mGrpRtnAndLeadSym (before sorting) ---", GetStrategyName(m_StyID).c_str());

          int grp = 0;
          FForEach(mGrpRtnAndLeadSym,[&](const TupRetSym & tup) {
            m_Logger->Write(Logger::INFO,"Strategy %s: Rotation mode: mGrpRtnAndLeadSym: Group %d %f %s (Grp %d)",
                            GetStrategyName(m_StyID).c_str(),
                            grp,
                            tup.m_return,
                            tup.m_symbol().c_str(),
                            tup.m_rttngrp);
            grp++;
          });

          m_Logger->Write(Logger::INFO,"Strategy %s: Rotation mode: --- mGrpRtnAndLeadSym (sorted) ---", GetStrategyName(m_StyID).c_str());

          std::sort(mGrpRtnAndLeadSym.begin(), mGrpRtnAndLeadSym.end());

          FForEach(mGrpRtnAndLeadSym,[&](const TupRetSym & tup) {
            m_Logger->Write(Logger::INFO,"Strategy %s: Rotation mode: (ascending order) mGrpRtnAndLeadSym: %f %s (Grp %d)",
                            GetStrategyName(m_StyID).c_str(),
                            tup.m_return,
                            tup.m_symbol().c_str(),
                            tup.m_rttngrp);
          });

          if (m_RotationModeTradeHighestReturn) FReverse(mGrpRtnAndLeadSym);

          FForEach(mGrpRtnAndLeadSym,[&](const TupRetSym & tup) {
            m_Logger->Write(Logger::INFO,"Strategy %s: Rotation mode: (according to rotation scheme) mGrpRtnAndLeadSym: %f %s (Grp %d)",
                            GetStrategyName(m_StyID).c_str(),
                            tup.m_return,
                            tup.m_symbol().c_str(),
                            tup.m_rttngrp);
          });
          m_Logger->Write(Logger::INFO,"Strategy %s: Rotation mode: --- mGrpRtnAndLeadSym ---", GetStrategyName(m_StyID).c_str());


          {
            //--------------------------------------------------
            // sticky group: here we move sticky groups to the front
            //--------------------------------------------------
            vector<TupRetSym> newGrpRtnAndLeadSym;
            map<YYYYMMDD,set<int> >::iterator it = m_MaintainGrp.find(m_p_ymdhms_SysTime_Local->GetYYYYMMDD());
            if (it != m_MaintainGrp.end())
            {
              const set<int> & setMtnGrp = it->second;

              //--------------------------------------------------
              // filter those sticky groups
              //--------------------------------------------------
              FForEach(mGrpRtnAndLeadSym,[&](const TupRetSym & tup) {
                if (setMtnGrp.find(tup.m_rttngrp) != setMtnGrp.end())
                  newGrpRtnAndLeadSym.push_back(tup);
              });
              //--------------------------------------------------
              // filter those non-sticky groups
              //--------------------------------------------------
              FForEach(mGrpRtnAndLeadSym,[&](const TupRetSym & tup) {
                if (setMtnGrp.find(tup.m_rttngrp) == setMtnGrp.end())
                  newGrpRtnAndLeadSym.push_back(tup);
              });

              mGrpRtnAndLeadSym = newGrpRtnAndLeadSym;
            }

            FForEach(mGrpRtnAndLeadSym,[&](const TupRetSym & tup) {
              m_Logger->Write(Logger::INFO,"Strategy %s: Rotation mode: (after moving sticky group to front) mGrpRtnAndLeadSym: %f %s (Grp %d)",
                              GetStrategyName(m_StyID).c_str(),
                              tup.m_return,
                              tup.m_symbol().c_str(),
                              tup.m_rttngrp);
            });
            m_Logger->Write(Logger::INFO,"Strategy %s: Rotation mode: --- mGrpRtnAndLeadSym ---", GetStrategyName(m_StyID).c_str());

          }
          //--------------------------------------------------


          int iCnt = 0;
          while (true)
          {
            if (!m_MoveNextBestGroupUpIfNoSignal)
            {
              if (iCnt >= m_ChooseBestNRotationGroup) break;
            }
            else
            {
              if (m_StkPicks.size() >= m_ChooseBestNRotationGroup) break;
            }

            //--------------------------------------------------
            // if there is a choosen symbol in the group
            // and
            // if the group return is at least positive
            // then pick the stock
            //--------------------------------------------------
            if (mGrpRtnAndLeadSym[iCnt].m_symbol() != "")
            {
              if (m_LongOnlyWhenGrpAvgReturnAbove.IsNone())
              {
                m_StkPicks.insert(mGrpRtnAndLeadSym[iCnt].m_symbol());
              }
              else
              {
                if (mGrpRtnAndLeadSym[iCnt].m_return > m_LongOnlyWhenGrpAvgReturnAbove.Get())
                {
                  m_StkPicks.insert(mGrpRtnAndLeadSym[iCnt].m_symbol());
                }
                else
                {
                  m_Logger->Write(Logger::INFO,"Strategy %s: Rotation mode: %s Stock %s is not picked because group return is smaller than threshold %f",
                                  GetStrategyName(m_StyID).c_str(),
                                  m_p_ymdhms_SysTime_Local->ToStr().c_str(),
                                  mGrpRtnAndLeadSym[iCnt].m_symbol().c_str(),
                                  m_LongOnlyWhenGrpAvgReturnAbove.Get());
                }
              }
            }

            iCnt++;
            if (iCnt >= mGrpRtnAndLeadSym.size()) break;
          }
        }

        m_Logger->Write(Logger::INFO,"Strategy %s: Rotation mode: --- Stock pick ---", GetStrategyName(m_StyID).c_str());
        for (set<string>::iterator it = m_StkPicks.begin(); it != m_StkPicks.end(); ++it)
        {
          m_Logger->Write(Logger::INFO,"Strategy %s: Rotation mode: %s Stock pick %s",
                          GetStrategyName(m_StyID).c_str(),
                          m_p_ymdhms_SysTime_Local->ToStr().c_str(),
                          it->c_str());
        }
        m_Logger->Write(Logger::INFO,"Strategy %s: Rotation mode: --- Stock pick ---", GetStrategyName(m_StyID).c_str());
      }

      //--------------------------------------------------
      // mark done rotation logic
      //--------------------------------------------------
      m_HasPerformedRotationLogic.Add(m_p_ymdhms_SysTime_Local->GetYYYYMMDD());

    }

    //--------------------------------------------------
    // if not in the correct direction, set quantity to zero
    //--------------------------------------------------
    if (m_RotationMode == 1
        &&
        m_dAggSignedQty < 0)
    {
      m_dAggSignedQty = 0;
    }
    else if (m_RotationMode == -1
             &&
             m_dAggSignedQty > 0)
    {
      m_dAggSignedQty = 0;
    }

    //--------------------------------------------------
    // set 0 to trade position if this is not the stock that we should keep
    //--------------------------------------------------
    if (m_StkPicks.find(m_TradedSymbols[iTradSym]) == m_StkPicks.end())
    {
      m_Logger->Write(Logger::INFO,"Strategy %s: Rotation mode: %s Sym=%s is not the chosen one. Set position to 0.",
                      GetStrategyName(m_StyID).c_str(),
                      m_p_ymdhms_SysTime_Local->ToStr().c_str(),
                      m_TradedSymbols[iTradSym].c_str());

      m_TargetTradeDir [iTradSym] = TD_NODIR;
      m_TargetAbsPos   [iTradSym] = 0;
      *m_DoneEODTrade = true;

      return;
    }

    //--------------------------------------------------
    // if this is the representative symbol, set it with the average notional amount
    //--------------------------------------------------
    if (m_vGroupRepresentative[m_RotationGroup[iTradSym]].IsSome())
    {
      const string sRepreSym = m_vGroupRepresentative[m_RotationGroup[iTradSym]].Get();
      if (m_TradedSymbols[iTradSym] == sRepreSym)
      {
        m_Logger->Write(Logger::INFO,"Strategy %s: Rotation mode: %s Sym=%s m_dAggSignedQty (before representative adjustment) %f",
                        GetStrategyName(m_StyID).c_str(),
                        m_p_ymdhms_SysTime_Local->ToStr().c_str(),
                        m_TradedSymbols[iTradSym].c_str(),
                        m_dAggSignedQty);

        m_dAggSignedQty = (double)(m_RotationMode > 0 ? 1 : -1) *
          abs(GetAvgAggSgndNotnlOfGrp(m_RotationGroup[iTradSym],
                                      m_p_ymdhms_SysTime_Local->GetYYYYMMDD())) / m_SymMidQuote;

        m_Logger->Write(Logger::INFO,"Strategy %s: Rotation mode: %s Sym=%s m_dAggSignedQty (after representative adjustment) %f",
                        GetStrategyName(m_StyID).c_str(),
                        m_p_ymdhms_SysTime_Local->ToStr().c_str(),
                        m_TradedSymbols[iTradSym].c_str(),
                        m_dAggSignedQty);
      }
    }

  }

  //--------------------------------------------------
  // Round down to lot size if HK stocks
  //--------------------------------------------------
  if (m_StyID == STY_B2_HK && GetStyDomicileMkt() == SDM_HK)
  {
    m_Logger->Write(Logger::INFO,"Strategy %s: %s Sym=%s Before lot size round down: m_dAggSignedQty = %f",
                    GetStrategyName(m_StyID).c_str(),
                    m_p_ymdhms_SysTime_Local->ToStr().c_str(),
                    m_TradedSymbols[iTradSym].c_str(),
                    m_dAggSignedQty);
    m_dAggSignedQty = m_SysCfg->RoundDownLotSize(m_TradedSymbols[iTradSym],(long)m_dAggSignedQty);
    m_Logger->Write(Logger::INFO,"Strategy %s: %s Sym=%s After lot size round down: m_dAggSignedQty = %f",
                    GetStrategyName(m_StyID).c_str(),
                    m_p_ymdhms_SysTime_Local->ToStr().c_str(),
                    m_TradedSymbols[iTradSym].c_str(),
                    m_dAggSignedQty);
  }

  //--------------------------------------------------
  // Position size after considering the effect of commission
  //--------------------------------------------------
  if (m_B2_HasEnabledMinCommissionCheck)
  {

    if (
      //--------------------------------------------------
      // if position is to be set to zero, do it! ignore commission impact
      //--------------------------------------------------
      abs(m_dAggSignedQty) > NIR_EPSILON
      &&
      CommissionCalculator::CalcCommission(
        CommissionCalculator::IB,
        CommissionCalculator::USSTK,
        CommissionCalculator::SPOT,
        m_SymMidQuote,
        m_dAggSignedQty) / (m_SymMidQuote * abs(m_dAggSignedQty)) > m_SysCfg->B2_CommissionRateThreshold(m_StyID)
      )
    {
      //--------------------------------------------------
      // looking one step ahead - when we actually have to close the remaining position
      // the target quantity is too small, just make the quantity zero.
      // not worth keeping a tiny position here
      //--------------------------------------------------
      m_Logger->Write(Logger::INFO,"Strategy %s: %s Sym=%s m_dAggSignedQty will be adjusted to zero after considering commission impact. m_SymMidQuote = %f m_dAggSignedQty = %f",
                      GetStrategyName(m_StyID).c_str(),
                      m_p_ymdhms_SysTime_Local->ToStr().c_str(),
                      m_TradedSymbols[iTradSym].c_str(),
                      m_SymMidQuote,
                      m_dAggSignedQty);
      m_dAggSignedQty = 0;
    }

  }

  double dAbsDeltaQty = abs(m_dAggSignedQty - GetPrevTheoSgndPos(m_TradedSymbols[iTradSym]));
  if (
    !m_B2_HasEnabledMinCommissionCheck
    ||
    //--------------------------------------------------
    // if position is to be set to zero, do it! ignore commission impact
    //--------------------------------------------------
    abs(m_dAggSignedQty) < NIR_EPSILON
    ||
    (
      m_B2_HasEnabledMinCommissionCheck
      &&
      CommissionCalculator::CalcCommission(
        CommissionCalculator::IB,
        CommissionCalculator::USSTK,
        CommissionCalculator::SPOT,
        m_SymMidQuote,
        dAbsDeltaQty) / (m_SymMidQuote * dAbsDeltaQty) <= m_SysCfg->B2_CommissionRateThreshold(m_StyID)
    )
    )
  {
    if (!(*m_DoneEODTrade))
    {
      m_TargetTradeDir [iTradSym] = (m_dAggSignedQty > 0 ? TD_LONG : TD_SHORT);
      m_TargetAbsPos   [iTradSym] = round(abs(m_dAggSignedQty));
    }
  }
  else
  {
    m_Logger->Write(Logger::INFO,"Strategy %s: Rotation mode: %s Sym=%s Trade wasn't initiated because the commission expense rate is higher than threshold. m_SymMidQuote = %f m_dAggSignedQty = %f",
                    GetStrategyName(m_StyID).c_str(),
                    m_p_ymdhms_SysTime_Local->ToStr().c_str(),
                    m_TradedSymbols[iTradSym].c_str(),
                    m_SymMidQuote,
                    m_dAggSignedQty);
  }


  *m_DoneEODTrade = true;
  //--------------------------------------------------


}

void StrategyB2::CalcPosSize(const int iTradSym)
{

}

void StrategyB2::UnsetConvenienceVarb()
{

  m_HistoricalAvgPx                         = NULL;
  m_HistoricalClose                         = NULL;
  m_HistoricalNumNumOfRisingDaysCountAvgPx  = NULL;
  m_HistoricalNumNumOfRisingDaysCountClose  = NULL;
  m_p_map_BestParamSetBeta1Beta3AvgPx_hypo1 = NULL;
  m_p_map_BestParamSetBeta2Beta4AvgPx_hypo1 = NULL;
  m_p_map_BestParamSetBeta1Beta3Close_hypo1 = NULL;
  m_p_map_BestParamSetBeta2Beta4Close_hypo1 = NULL;
  m_p_map_BestParamSetBeta1Beta3AvgPx_hypo2 = NULL;
  m_p_map_BestParamSetBeta2Beta4AvgPx_hypo2 = NULL;
  m_p_map_BestParamSetBeta1Beta3Close_hypo2 = NULL;
  m_p_map_BestParamSetBeta2Beta4Close_hypo2 = NULL;
  m_DoneEODTrade                            = NULL;
  m_bTrainRetAvgPx                          = NULL;
  m_bTrainRetClose                          = NULL;
  m_bRisingRegimeAvgPx                      = NULL;
  m_bRisingRegimeClose                      = NULL;

  return;
}

void StrategyB2::ParamSanityCheck()
{
  if (m_RotationMode != 0)
  {
    if (
      m_RotationGroup.size() != m_TradedSymbols.size()
      ||
      m_ParamVector.size() / m_iNumOfParam != m_TradedSymbols.size()
      ||
      m_RotationGroup.size() != m_vRoleOfSym.size()
      )
    {
      m_Logger->Write(Logger::ERROR,"Strategy %s: Failed sanity test", GetStrategyName(m_StyID).c_str());
      sleep(1);
      exit(1);
    }
  }
  return;
}

void StrategyB2::LoadOptimalParamsFromTrainingResults()
{
  return;
}

bool StrategyB2::NDayRollingReturnReadyForAllSym(const int gid, const YYYYMMDD & ymd)
{
  map<YYYYMMDD,map<string,double> >::iterator it = m_SymAndRollingReturn[gid].find(ymd);
  if (it == m_SymAndRollingReturn[gid].end())
  {
    m_Logger->Write(Logger::INFO,"Strategy %s: Date %s not found in m_SymAndRollingReturn. gid = %d",
                    GetStrategyName(m_StyID).c_str(), ymd.ToStr().c_str(),
                    gid);
    return false;
  }

  map<string,double> & mapTmp = it->second;

  if (mapTmp.size() < m_AllAvbSymForRollingBasket[gid][ymd].size())
  {
    m_Logger->Write(Logger::INFO,"Strategy %s: gid = %d No of rolling return obtained = %d. Num of available symbols = %d",
                    GetStrategyName(m_StyID).c_str(), gid,
                    mapTmp.size(),
                    m_AllAvbSymForRollingBasket[gid][ymd].size());

    FForEach(mapTmp,[&](const std::pair<string,double> tup) {
      m_Logger->Write(Logger::INFO,"Strategy %s: gid = %d Rolling return obtained for %s",
                      GetStrategyName(m_StyID).c_str(), gid,
                      tup.first.c_str());
    });

    FForEach(m_AllAvbSymForRollingBasket[gid][ymd],[&](const string & s) {
      m_Logger->Write(Logger::INFO,"Strategy %s: gid = %d Available symbols %s",
                      GetStrategyName(m_StyID).c_str(), gid,
                      s.c_str());
    });

    return false;
  }

  return true;
}

bool StrategyB2::AggSgndQtyReadyForAllSym(const YYYYMMDD & ymd)
{
  map<YYYYMMDD,map<string,double> >::iterator it = m_SymAggSgndQty.find(ymd);
  if (it == m_SymAggSgndQty.end())
  {
    m_Logger->Write(Logger::INFO,"Strategy %s: Date %s not found in m_SymAggSgndQty.",
                    GetStrategyName(m_StyID).c_str(), ymd.ToStr().c_str());
    return false;
  }

  map<string,double> & mapTmp = it->second;

  if (mapTmp.size() < m_AllAvbSym[ymd].size())
  {
    m_Logger->Write(Logger::INFO,"Strategy %s: No of aggregate signed qty obtained = %d. Num of available symbols = %d",
                    GetStrategyName(m_StyID).c_str(),
                    mapTmp.size(), m_AllAvbSym[ymd].size());

    FForEach(mapTmp,[&](const std::pair<string,double> tup) {
      m_Logger->Write(Logger::INFO,"Strategy %s: Aggregate signed qty obtained for %s",
                      GetStrategyName(m_StyID).c_str(), tup.first.c_str());
    });

    FForEach(m_AllAvbSym[ymd],[&](const string & s) {
      m_Logger->Write(Logger::INFO,"Strategy %s: Available symbols %s",
                      GetStrategyName(m_StyID).c_str(), s.c_str());
    });

    return false;
  }

  return true;
}

void StrategyB2::AddNDayRollingReturn(const int gid, const YYYYMMDD & ymd, const string & sym, const double ret)
{
  //--------------------------------------------------
  // don't add if NAN
  //--------------------------------------------------
  if (std::isnan(ret)) return;

  if (m_AllAvbSymForRollingBasket[gid][ymd].find(sym) == m_AllAvbSymForRollingBasket[gid][ymd].end()) return;

  map<YYYYMMDD,map<string,double> >::iterator it1 = m_SymAndRollingReturn[gid].find(ymd);
  map<YYYYMMDD,vector<TupRetSym> >::iterator it2 = m_SymRankedByRollingReturn[gid].find(ymd);
  if (it1 == m_SymAndRollingReturn[gid].end())
  {
    m_SymAndRollingReturn[gid][ymd] = map<string,double>();
    it1 = m_SymAndRollingReturn[gid].find(ymd);
    m_SymRankedByRollingReturn[gid][ymd] = vector<TupRetSym>();
    it2 = m_SymRankedByRollingReturn[gid].find(ymd);
  }
  else
  {
    //--------------------------------------------------
    // added already, won't add second time
    //--------------------------------------------------
    if (it1->second.find(sym) != it1->second.end()) return;
  }

  //--------------------------------------------------
  // add now
  //--------------------------------------------------
  it1->second[sym] = ret;
  it2->second.push_back(TupRetSym(ret, sym, gid));

}

void StrategyB2::AddAggSgndQty(const YYYYMMDD & ymd, const string & sym, const double dAggSgndQty)
{
  map<YYYYMMDD,map<string,double> >::iterator it = m_SymAggSgndQty.find(ymd);

  if (it == m_SymAggSgndQty.end())
  {
    m_SymAggSgndQty[ymd] = map<string,double>();
    it = m_SymAggSgndQty.find(ymd);
  }

  (it->second)[sym] = dAggSgndQty;
}

void StrategyB2::AddAggSgndNotnl(const YYYYMMDD & ymd, const int gid, const string & sym, const double dStrengthIndi)
{
  map<YYYYMMDD,map<int,map<string,double> > >::iterator it = m_AggSgndNotnlOfSym.find(ymd);

  if (it == m_AggSgndNotnlOfSym.end())
  {
    m_AggSgndNotnlOfSym[ymd] = map<int,map<string,double> >();
    it = m_AggSgndNotnlOfSym.find(ymd);
  }

  map<int,map<string,double> > & mapTmp = it->second;

  map<int,map<string,double> >::iterator it2 = mapTmp.find(gid);
  if (it2 == mapTmp.end())
  {
    mapTmp[gid] = map<string,double>();
    it2 = mapTmp.find(gid);
  }

  (it2->second)[sym] = dStrengthIndi;
}


Option<string> StrategyB2::GetSymInGrpRankByRet(const int gid, const YYYYMMDD & ymd, const int rank, const ASC_DESC ad)
{
  map<YYYYMMDD,vector<TupRetSym> >::iterator it2 = m_SymRankedByRollingReturn[gid].find(ymd);
  if (it2 == m_SymRankedByRollingReturn[gid].end()) return Option<string>();

  vector<TupRetSym> & vecTmp = it2->second;
  if (vecTmp.empty()) return Option<string>();

  std::sort(vecTmp.begin(), vecTmp.end());

  if (ad == AD_ASCENDING && vecTmp.size() >= rank) return Option<string>(vecTmp[rank-1].m_symbol());
  if (ad == AD_DESCENDING && vecTmp.size() >= rank) return Option<string>(vecTmp[vecTmp.size()-rank].m_symbol());

  return Option<string>();
}

double StrategyB2::GetAvgRollgRetOfGrp(const int gid,const YYYYMMDD & ymd)
{
  map<YYYYMMDD,vector<TupRetSym> >::iterator it2 = m_SymRankedByRollingReturn[gid].find(ymd);
  if (it2 == m_SymRankedByRollingReturn[gid].end()) return NAN;

  const vector<TupRetSym> vt = FFilterNot(it2->second,[](const TupRetSym &tup){ return std::isnan(tup.m_return); });

  int iCnt = vt.size();
  double dTot = FFold(vt,(double)0,[](const double &a, const TupRetSym &b) { return a+b.m_return; } );

  if (iCnt > 0) return dTot / (double)iCnt;
  else return NAN;
}

double StrategyB2::GetAvgAggSgndNotnlOfGrp(const int gid,const YYYYMMDD & ymd)
{
  map<YYYYMMDD,map<int,map<string,double> > >::iterator it = m_AggSgndNotnlOfSym.find(ymd);
  if (it == m_AggSgndNotnlOfSym.end()) return NAN;

  map<int,map<string,double> > & mapTmp = it->second;
  map<int,map<string,double> >::iterator it2 = mapTmp.find(gid);
  if (it2 == mapTmp.end()) return NAN;

  map<string,double> & mapTmp2 = it2->second;

  vector<double> vt;
  FForEach(mapTmp2,[&](const std::pair<string,double> symStren) {
    if (!std::isnan(symStren.second)) vt.push_back(symStren.second);
  });

  int iCnt = vt.size();
  double dTot = FFold(vt,(double)0,[](const double c, const double e) { return c+e; } );

  if (iCnt > 0) return dTot / (double)iCnt;
  else return NAN;
}

