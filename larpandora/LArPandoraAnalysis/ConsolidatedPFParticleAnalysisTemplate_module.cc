/**
 *  @file   larpandora/LArPandoraAnalysis/ConsolidatedPFParticleAnalysisTemplate_module.cc
 *
 *  @brief  A template analysis module for using the Pandora consolidated output
 */

#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Core/EDAnalyzer.h"

#include "larpandora/LArPandoraInterface/LArPandoraHelper.h"

#include <string>

//------------------------------------------------------------------------------------------------------------------------------------------

namespace lar_pandora
{

/**
 *  @brief  ConsolidatedPFParticleAnalysisTemplate class
 */
class ConsolidatedPFParticleAnalysisTemplate : public art::EDAnalyzer
{
public:
    /**
     *  @brief  Constructor
     *
     *  @param  pset the set of input fhicl parameters
     */
    ConsolidatedPFParticleAnalysisTemplate(fhicl::ParameterSet const &pset);
    
    /**
     *  @brief  Configure memeber variables using FHiCL parameters
     *
     *  @param  pset the set of input fhicl parameters
     */
    void reconfigure(fhicl::ParameterSet const &pset);

    /**
     *  @brief  Analyze an event!
     *
     *  @param  evt the art event to analyze
     */
    void analyze(const art::Event &evt);

private:
    /**
     *  @brief  Get vectors of PFParticles containing primary cosmic ray particles and daughters of the neutrino
     *
     *  @param  pfParticles the input list of all PFParticles
     *  @param  crParticles the output list of CR primary PFParticles
     *  @param  nuParticles the output list of neutrino final state PFParticles
     */
    void GetPFParticleVectors(const PFParticleVector &pfParticles, PFParticleVector &crParticles, PFParticleVector &nuParticles) const;

    std::string m_pandoraLabel;         ///< The label for the pandora producer
};

DEFINE_ART_MODULE(ConsolidatedPFParticleAnalysisTemplate)

} // namespace lar_pandora

//------------------------------------------------------------------------------------------------------------------------------------------
// implementation follows

#include "art/Framework/Principal/Event.h"
#include "fhiclcpp/ParameterSet.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "art/Framework/Services/Optional/TFileService.h"
#include "art/Framework/Services/Optional/TFileDirectory.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

#include "lardataobj/RecoBase/PFParticle.h"
#include "Pandora/PdgTable.h"

#include <iostream>

namespace lar_pandora
{

ConsolidatedPFParticleAnalysisTemplate::ConsolidatedPFParticleAnalysisTemplate(fhicl::ParameterSet const &pset) : art::EDAnalyzer(pset)
{
    this->reconfigure(pset);
}

//------------------------------------------------------------------------------------------------------------------------------------------

void ConsolidatedPFParticleAnalysisTemplate::reconfigure(fhicl::ParameterSet const &pset)
{
    m_pandoraLabel = pset.get<std::string>("PandoraLabel");
}

//------------------------------------------------------------------------------------------------------------------------------------------

void ConsolidatedPFParticleAnalysisTemplate::analyze(const art::Event &evt)
{
    // Collect the PFParticles from the event
    PFParticleVector pfParticles;
    LArPandoraHelper::CollectPFParticles(evt, m_pandoraLabel, pfParticles);
    
    // Produce two PFParticle vectors containing final-state particles:
    // 1. Particles identified as cosmic-rays - recontructed under cosmic-hypothesis
    // 2. Daughters of the neutrino PFParticle - reconstructed under the neutrino hypothesis
    PFParticleVector crParticles;
    PFParticleVector nuParticles;
    this->GetPFParticleVectors(pfParticles, crParticles, nuParticles);

    // Use as required!
}

//------------------------------------------------------------------------------------------------------------------------------------------

void ConsolidatedPFParticleAnalysisTemplate::GetPFParticleVectors(const PFParticleVector &pfParticles, PFParticleVector &crParticles, PFParticleVector &nuParticles) const
{
    // Get the mapping from ID to PFParticles
    PFParticleMap pfParticleIdMap;
    LArPandoraHelper::BuildPFParticleMap(pfParticles, pfParticleIdMap);

    for (const auto &pParticle : pfParticles)
    {
        // Only collect final-state particles
        //     If you need secondaries, ask one of the final-state particles for its ->Daughters()
        if (!LArPandoraHelper::IsFinalState(pfParticleIdMap, pParticle)) continue;

        // Check if the parent of the particle is a neutrino using its PDG code
        const int parentPdg(LArPandoraHelper::GetParentPFParticle(pfParticleIdMap, pParticle)->PdgCode());
        if (std::abs(parentPdg) == pandora::NU_E  ||
            std::abs(parentPdg) == pandora::NU_MU ||
            std::abs(parentPdg) == pandora::NU_TAU )
        {
            nuParticles.push_back(pParticle);
        }
        else
        {
            crParticles.push_back(pParticle);
        }
    }
}

} //namespace lar_pandora
