/**
 *  @file   LArContent/include/LArHelpers/LArPCAHelper.h
 *
 *  @brief  Header file for the PCA helper class.
 *
 *  $Log: $
 */
#ifndef LAR_PCA_HELPER_H
#define LAR_PCA_HELPER_H 1

#include "Objects/ParticleFlowObject.h"
#include "Objects/CartesianVector.h"

namespace lar_content
{

/**
 *  @brief  LArPCAHelper class
 */
class LArPCAHelper
{
public:

   /**
     *  @brief  Perform PCA on a cluster
     *
     *  @param  pCluster input cluster
     *  @param  pEigenvalues output PCA eigenvalues  
     *  @param  pEigenvectors output PCA eigenvectors
     */
  static void RunPCA(const pandora::Cluster* pCluster, std::vector<double>& pEigenvalues,
		     std::vector<pandora::CartesianVector>& pEigenvectors);

  
   /**
     *  @brief  Perform PCA on a PFO
     *
     *  @param  pPfo input PFO
     *  @param  pEigenvalues output PCA eigenvalues  
     *  @param  pEigenvectors output PCA eigenvectors
     */
  static void RunPCA(const pandora::ParticleFlowObject* pPfo, std::vector<double>& pEigenvalues,
		     std::vector<pandora::CartesianVector>& pEigenvectors);
};

} // namespace lar_content

#endif // #ifndef LAR_PCA_HELPER_H
