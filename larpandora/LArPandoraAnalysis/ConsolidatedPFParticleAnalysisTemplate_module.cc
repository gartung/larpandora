/**
 *  @file   larpandora/LArPandoraAnalysis/ConsolidatedPFParticleAnalysisTemplate_module.cc
 *
 *  @brief  A template analysis module for using the Pandora consolidated output
 */

#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Core/EDAnalyzer.h"

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
    std::string m_pandoraLabel;         ///< The label for the pandora producer
    std::string m_trackLabel;           ///< The label for the track producer from PFParticles
    std::string m_showerLabel;          ///< The label for the shower producer from PFParticles
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
#include "canvas/Persistency/Common/FindManyP.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

#include "lardataobj/RecoBase/PFParticle.h"
#include "lardataobj/RecoBase/Track.h"
#include "lardataobj/RecoBase/Shower.h"

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
    m_trackLabel = pset.get<std::string>("TrackLabel");
    m_showerLabel = pset.get<std::string>("ShowerLabel");
}

//------------------------------------------------------------------------------------------------------------------------------------------

void ConsolidatedPFParticleAnalysisTemplate::analyze(const art::Event &evt)
{
    // Collect the PFParticles from the event
    art::Handle< std::vector<recob::PFParticle> > pfParticleHandle;
    evt.getByLabel(m_pandoraLabel, pfParticleHandle);
   
    if (!pfParticleHandle.isValid())
    {
        mf::LogDebug("LArPandora") << "  Failed to find the PFParticles." << std::endl;
        return;
    }

    // Produce a map of the PFParticle IDs for fast navigation through the hierarchy
    std::map< size_t, art::Ptr<recob::PFParticle> > pfParticleMap;
    for (unsigned int i = 0; i < pfParticleHandle->size(); ++i)
    {
        const art::Ptr<recob::PFParticle> pParticle(pfParticleHandle, i);
        pfParticleMap.insert(std::map< size_t, art::Ptr<recob::PFParticle> >::value_type(pParticle->Self(), pParticle));
    }
    
    // Produce two PFParticle vectors containing final-state particles:
    // 1. Particles identified as cosmic-rays - recontructed under cosmic-hypothesis
    // 2. Daughters of the neutrino PFParticle - reconstructed under the neutrino hypothesis
    std::vector< art::Ptr<recob::PFParticle> > crParticles;
    std::vector< art::Ptr<recob::PFParticle> > nuParticles;
    
    for (unsigned int i = 0; i < pfParticleHandle->size(); ++i)
    {
        const art::Ptr<recob::PFParticle> pParticle(pfParticleHandle, i);

        // Only look for primary particles
        if (!pParticle->IsPrimary()) continue;

        // Check if this particle is identified as the neutrino
        // ATTN. We are filling nuParticles under the assumption that there is only one reconstructed neutrino identified per event.
        //       If this is not the case please modify accordingly
        const int pdg(pParticle->PdgCode());
        const bool isNeutrino(std::abs(pdg) == pandora::NU_E || std::abs(pdg) == pandora::NU_MU || std::abs(pdg) == pandora::NU_TAU);

        // All non-neutrino primary particles are reconstructed under the cosmic hypothesis
        if (!isNeutrino)
        {
            crParticles.push_back(pParticle);
            continue;
        }

        // Add the daughters of the neutrino PFParticle to the nuPFParticles vector
        for (const size_t daughterId : pParticle->Daughters())
        {
            if (pfParticleMap.find(daughterId) == pfParticleMap.end())
                throw cet::exception("LArPandora") << "  Invalid PFParticle collection!";

            nuParticles.push_back(pfParticleMap.at(daughterId));
        }
    }

    // Use as required!
    // -----------------------------
    //   What follows is an example showing how to access the reconstructed neutrino final-state tracks and showers
    
    // Get the associations between PFParticles and tracks/showers from the event
    art::FindManyP< recob::Track > pfPartToTrackAssoc(pfParticleHandle, evt, m_trackLabel);
    art::FindManyP< recob::Shower > pfPartToShowerAssoc(pfParticleHandle, evt, m_showerLabel);
   
    // These are the vectors to hold the tracks and showers for the final-states of the reconstructed neutrino
    std::vector< art::Ptr<recob::Track> > tracks;
    std::vector< art::Ptr<recob::Shower> > showers;

    // For each PFParticle, find associated tracks and showers
    for (const art::Ptr<recob::PFParticle> &pParticle : nuParticles)
    {
        const std::vector< art::Ptr<recob::Track> > associatedTracks(pfPartToTrackAssoc.at(pParticle.key()));
        const std::vector< art::Ptr<recob::Shower> > associatedShowers(pfPartToShowerAssoc.at(pParticle.key()));
        const unsigned int nTracks(associatedTracks.size());
        const unsigned int nShowers(associatedShowers.size());

        // Check if the PFParticle has no associated tracks or showers
        if (nTracks == 0 && nShowers == 0)
        {
            mf::LogDebug("LArPandora") << "  No tracks or showers were associated to PFParticle " << pParticle->Self() << std::endl;
            continue;
        }

        // Check if there is an associated track
        if (nTracks == 1 && nShowers == 0)
        {
            tracks.push_back(associatedTracks.front());
            continue;
        }

        // Check if there is an associated shower
        if (nTracks == 0 && nShowers == 1)
        {
            showers.push_back(associatedShowers.front());
            continue;
        }

        throw cet::exception("LArPandora") << "  There were " << nTracks << " tracks and " << nShowers << " showers associated with PFParticle " << pParticle->Self();
    }

    // Print a summary of the consolidated event
    std::cout << "Consolidated event summary:" << std::endl;
    std::cout << "  - Number of primary cosmic-ray PFParticles   : " << crParticles.size() << std::endl;
    std::cout << "  - Number of neutrino final-state PFParticles : " << nuParticles.size() << std::endl;
    std::cout << "    ... of which are track-like   : " << tracks.size() << std::endl;
    std::cout << "    ... of which are showers-like : " << showers.size() << std::endl;
}

} //namespace lar_pandora
