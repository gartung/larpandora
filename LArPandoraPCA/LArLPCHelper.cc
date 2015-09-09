/**
 *  @file   LArContent/src/LArHelpers/LArClusterHelper.cc
 *
 *  @brief  Implementation of the cluster helper class.
 *
 *  $Log: $
 */

#include "LArHelpers/LArClusterHelper.h"
#include "LArHelpers/LArPfoHelper.h"
#include "LArLPCHelper.h"
#include "Objects/Cluster.h"

#include "LACE/LpcHitCollection.hh"
#include "LACE/LpcHitCollection.hh"
#include "LACE/LpcHit.hh"
#include "LACE/LpcEvent.hh"
#include "LACE/LpcCurve.hh"
#include "LACE/LpcPoint.hh"

using namespace pandora;

namespace lar_content
{

LArLPCHelper::LArLPCHelper(){
  fProcess=new LpcProcess(NULL);
}

void LArLPCHelper::RunLPC(const Cluster* pCluster,std::vector<pandora::TrackState>& trackPoints)
{
    LpcHitCollection* theHits = new LpcHitCollection(3);
    
    CaloHitList hitList;
    pCluster->GetOrderedCaloHitList().GetCaloHitList(hitList);
    
    int hitCount=0;
    for(CaloHitList::const_iterator hitIter=hitList.begin();hitIter!=hitList.end();++hitIter)
    {
      const CartesianVector& hitpos=(*hitIter)->GetPositionVector();
      LpcHit* aHit=new LpcHit(hitCount++, hitpos.GetX(), hitpos.GetY(), hitpos.GetZ(), (*hitIter)->GetInputEnergy());
      theHits->addHit(aHit);
    }

    LpcEvent* theEvent = new LpcEvent(0, theHits);

    fProcess->reconstruct(theEvent);

    LpcCurve* theCurve=theEvent->getCurve();

    std::vector<LpcPoint> thePoints=theCurve->getLpcPoints();
    Eigen::MatrixXd eigenVectors=theCurve->getEigenVectors();

    trackPoints.clear();

    int pointCount=0;
    for(auto pt=thePoints.begin();pt!=thePoints.end();++pt){
      Eigen::VectorXd coords=pt->getCoords();
      if(abs(coords[0])==0&&abs(coords[1])==0&&abs(coords[2])==0) break;
      trackPoints.push_back(pandora::TrackState(coords[0],coords[1],coords[2],
						eigenVectors(pointCount,0),eigenVectors(pointCount,1),eigenVectors(pointCount,2)));
      ++pointCount;
    }
}

void LArLPCHelper::RunLPC(const ParticleFlowObject* pPfo,std::vector<pandora::TrackState>& trackPoints)
{
    //Get 3D cluster associated with this PFO
    ClusterList clusterList;
    LArPfoHelper::GetClusters(pPfo, TPC_3D, clusterList);
    
    if (clusterList.empty())
      return;

    RunLPC(*clusterList.begin(),trackPoints);
}

} // namespace lar_content
