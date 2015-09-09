/**
 *  @file   LArContent/include/LArHelpers/LArLPCHelper.h
 *
 *  @brief  Header file for the LPC (local principal curves) helper class.
 *
 *  $Log: $
 */
#ifndef LAR_LPC_HELPER_H
#define LAR_LPC_HELPER_H 1

#include "Objects/ParticleFlowObject.h"
#include "Objects/CartesianVector.h"

#include "LACE/LpcProcess.hh"

namespace lar_content
{

/**
 *  @brief  LArLPCHelper class
 */
class LArLPCHelper
{
public:
  
  LArLPCHelper();

   /**
     *  @brief  Run LPC algorithm on a cluster
     *
     *  @param  pPfo input PFO
     *  @param  trackPoints output LPC track points
     */
  void RunLPC(const pandora::Cluster* pCluster,std::vector<pandora::TrackState>& trackPoints);

   /**
     *  @brief  Run LPC algorithm on a PFO
     *
     *  @param  pPfo input PFO
     *  @param  trackPoints output LPC track points
     */
  void RunLPC(const pandora::ParticleFlowObject* pPfo,std::vector<pandora::TrackState>& trackPoints);

private:

  LpcProcess* fProcess;
};

} // namespace lar_content

#endif // #ifndef LAR_LPC_HELPER_H
