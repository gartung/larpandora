/**
 *  @file   LArContent/src/LArHelpers/LArClusterHelper.cc
 *
 *  @brief  Implementation of the cluster helper class.
 *
 *  $Log: $
 */

#include "LArHelpers/LArClusterHelper.h"
#include "LArHelpers/LArPfoHelper.h"
#include "LArPCAHelper.h"
#include "Objects/Cluster.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include <gsl/gsl_matrix.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_linalg.h>

using namespace pandora;

namespace lar_content
{

void LArPCAHelper::RunPCA(const Cluster* pCluster, std::vector<double>& pEigenvalues,
			  std::vector<CartesianVector>& pEigenvectors)
{
    pEigenvalues.clear();
    pEigenvectors.clear();

    CaloHitList hitList;
    pCluster->GetOrderedCaloHitList().GetCaloHitList(hitList);
    
    double xMean=0.;
    double yMean=0.;
    double zMean=0.;

    for(CaloHitList::const_iterator hitIter=hitList.begin();hitIter!=hitList.end();++hitIter)
    {
      const CartesianVector& hitpos=(*hitIter)->GetPositionVector();
      xMean+=hitpos.GetX();
      yMean+=hitpos.GetY();
      zMean+=hitpos.GetZ();
    }

    int nHits=hitList.size();
    xMean/=nHits;
    yMean/=nHits;
    zMean/=nHits;

    //Build covariance matrix from hit 3D positions
    double xi2  = 0.;
    double xiyi = 0.;
    double xizi = 0.;
    double yi2  = 0.;
    double yizi = 0.;
    double zi2  = 0.;

    for(CaloHitList::const_iterator hitIter=hitList.begin();hitIter!=hitList.end();++hitIter)
    {
      const CartesianVector& hitpos=(*hitIter)->GetPositionVector();
      double x = hitpos.GetX()-xMean;
      double y = hitpos.GetY()-yMean;
      double z = hitpos.GetZ()-zMean;
      
      xi2  += x * x;
      xiyi += x * y;
      xizi += x * z;
      yi2  += y * y;
      yizi += y * z;
      zi2  += z * z;
    }
    
    // Create the actual matrix
    gsl_matrix* sigma = gsl_matrix_alloc(3,3);
    gsl_matrix* V = gsl_matrix_alloc(3,3);
    gsl_vector* work = gsl_vector_alloc(3);
    gsl_vector* S = gsl_vector_alloc(3);

    gsl_matrix_set(sigma,0,0,xi2);
    gsl_matrix_set(sigma,1,0,xiyi);
    gsl_matrix_set(sigma,0,1,xiyi);
    gsl_matrix_set(sigma,2,0,xizi);
    gsl_matrix_set(sigma,0,2,xizi);
    gsl_matrix_set(sigma,1,1,yi2);
    gsl_matrix_set(sigma,1,2,yizi);
    gsl_matrix_set(sigma,2,1,yizi);
    gsl_matrix_set(sigma,2,2,zi2);

    gsl_matrix_scale(sigma,1./(nHits-1.));

    gsl_linalg_SV_decomp (sigma,V,S,work);

    for(int i=0;i<3;++i){
      pEigenvalues.push_back(gsl_vector_get(S,i));
      gsl_matrix_get_col(work,sigma,i);
      pEigenvectors.push_back(CartesianVector(gsl_vector_get(work,0),
					      gsl_vector_get(work,1),
					      gsl_vector_get(work,2)));
    }
}  

void LArPCAHelper::RunPCA(const ParticleFlowObject* pPfo, std::vector<double>& pEigenvalues,
			  std::vector<CartesianVector>& pEigenvectors)
{

  //Get 3D cluster associated with this PFO
  ClusterList clusterList;
  LArPfoHelper::GetClusters(pPfo, TPC_3D, clusterList);
  
  if (clusterList.empty())
    return;

  RunPCA(*clusterList.begin(),pEigenvalues,pEigenvectors);
}

} // namespace lar_content
