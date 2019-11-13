/**
 *
 * @file larpandora/LArPandoraAnalysisUtils/LArPandoraClusterUtils.h
 *
 * @brief Utility containing helpful functions for end users to access information about Clusters
 *
 * @author leigh.howard.whitehead@cern.ch
*/

#ifndef LAR_PANDORA_CLUSTER_UTILS_H
#define LAR_PANDORA_CLUSTER_UTILS_H

#include "art/Framework/Principal/Event.h"
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "canvas/Persistency/Common/FindManyP.h"

// Access the type defs defined in the helper
#include "larpandora/LArPandoraInterface/LArPandoraHelper.h"
#include "larpandora/LArPandoraAnalysisUtils/LArPandoraUtilsBase.h"

#include <string>
#include <vector>

namespace lar_pandora
{

/**
 *
 * @brief LArPandoraClusterUtils class
 *
*/
class LArPandoraClusterUtils:LArPandoraUtilsBase
{

public:

    static const std::vector<art::Ptr<recob::Hit>> GetHits(const art::Ptr<recob::Cluster> cluster, art::Event const &evt, const std::string &label);

private:

};

} // namespace lar_pandora


#endif // LAR_PANDORA_CLUSTER_UTILS_H

