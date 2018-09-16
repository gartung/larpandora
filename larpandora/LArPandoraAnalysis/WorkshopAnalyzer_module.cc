#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"

#include "TTree.h"

namespace pandorafnalworkshop
{

/**
 *  @brief Simple template analyzer for use in the Pandora FNAL 2018 workshop
 */
class WorkshopAnalyzer : public art::EDAnalyzer
{
public:
    explicit WorkshopAnalyzer(fhicl::ParameterSet const &parameterSet);

    WorkshopAnalyzer(WorkshopAnalyzer const &) = delete;
    WorkshopAnalyzer(WorkshopAnalyzer &&) = delete;
    WorkshopAnalyzer & operator = (WorkshopAnalyzer const &) = delete;
    WorkshopAnalyzer & operator = (WorkshopAnalyzer &&) = delete;

    void analyze(art::Event const &event) override;

private:

    std::string  m_pandoraLabel;           ///< The input label for the pandora producer
    std::string  m_trackLabel;             ///< The input label for the track producer

    TTree       *m_tree;                   ///< The output tree

    // Variables for use in branches
    int          m_nTracks;                ///< The number of neutrino induced tracks
    int          m_nPFParticles;           ///< The number of neutrino induced particles
};

DEFINE_ART_MODULE(WorkshopAnalyzer)

} // namespace pandorafnalworkshop 

// -----------------------------------------------------------------------------------------------------------------------------------------
// Implementation follows
// -----------------------------------------------------------------------------------------------------------------------------------------

#include "art/Framework/Services/Optional/TFileService.h"

#include "canvas/Persistency/Common/FindManyP.h"

#include "lardataobj/RecoBase/PFParticle.h"
#include "lardataobj/RecoBase/Track.h"

#include "Pandora/PdgTable.h"

namespace pandorafnalworkshop
{

WorkshopAnalyzer::WorkshopAnalyzer(fhicl::ParameterSet const &parameterSet) :
    EDAnalyzer(parameterSet),
    m_pandoraLabel(parameterSet.get<std::string>("PandoraLabel")),
    m_trackLabel(parameterSet.get<std::string>("TrackLabel")),
    m_nTracks(std::numeric_limits<int>::max())
{

    // Set up the output tree
    art::ServiceHandle<art::TFileService> fileService;
    m_tree = fileService->make<TTree>("workshop", "Pandora FNAL workshop example analysis");

    // Add the branch
    m_tree->Branch("nTracks", &m_nTracks, "nTracks/I");
}

// -----------------------------------------------------------------------------------------------------------------------------------------

void WorkshopAnalyzer::analyze(art::Event const &event)
{
    // Get the PFParticle collection

    // Get mapping from ID to PFParticle

    // Find the daughters of the neutrino PFParticle

    // Count how many PFParticles have an associated track

    // Fill the tree
}

} // namespace pandorafnalworkshop 
