#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"

#include "TTree.h"

namespace pandorafnalworkshop
{

/**
 *  @brief Simple template analyzer for use in the Pandora FNAL 2018 workshop
 */
class WorkshopAnalyzerComplete : public art::EDAnalyzer
{
public:
    explicit WorkshopAnalyzerComplete(fhicl::ParameterSet const &parameterSet);

    WorkshopAnalyzerComplete(WorkshopAnalyzerComplete const &) = delete;
    WorkshopAnalyzerComplete(WorkshopAnalyzerComplete &&) = delete;
    WorkshopAnalyzerComplete & operator = (WorkshopAnalyzerComplete const &) = delete;
    WorkshopAnalyzerComplete & operator = (WorkshopAnalyzerComplete &&) = delete;

    void analyze(art::Event const &event) override;

private:

    std::string  m_pandoraLabel;           ///< The input label for the pandora producer
    std::string  m_trackLabel;             ///< The input label for the track producer

    TTree       *m_tree;                   ///< The output tree

    // Variables for use in branches
    int          m_nTracks;                ///< The number of neutrino induced tracks
    int          m_nPFParticles;           ///< The number of neutrino induced particles
};

DEFINE_ART_MODULE(WorkshopAnalyzerComplete)

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

WorkshopAnalyzerComplete::WorkshopAnalyzerComplete(fhicl::ParameterSet const &parameterSet) :
    EDAnalyzer(parameterSet),
    m_pandoraLabel(parameterSet.get<std::string>("PandoraLabel")),
    m_trackLabel(parameterSet.get<std::string>("TrackLabel")),
    m_nTracks(-std::numeric_limits<int>::max()),
    m_nPFParticles(-std::numeric_limits<int>::max())
{

    // Set up the output tree
    art::ServiceHandle<art::TFileService> fileService;
    m_tree = fileService->make<TTree>("workshop", "Pandora FNAL workshop example analysis");

    // Add the branches
    m_tree->Branch("nTracks", &m_nTracks, "nTracks/I");
    m_tree->Branch("nPFParticles", &m_nPFParticles, "nPFParticles/I");
}

// -----------------------------------------------------------------------------------------------------------------------------------------

void WorkshopAnalyzerComplete::analyze(art::Event const &event)
{
    // Get the PFParticle collection
    art::Handle< std::vector<recob::PFParticle> > pfParticleHandle;
    event.getByLabel(m_pandoraLabel, pfParticleHandle); 

    // Get mapping from ID to PFParticle
    std::unordered_map<size_t, art::Ptr<recob::PFParticle> > pfParticleIdMap;
    for (unsigned int i = 0; i < pfParticleHandle->size(); ++i)
    {
        const art::Ptr<recob::PFParticle> pfParticle(pfParticleHandle, i);

        if (!pfParticleIdMap.emplace(pfParticle->Self(), pfParticle).second)
            throw cet::exception("WorkshopAnalyzerComplete") << "Repeated PFParticles in the input collection!" << std::endl;
    }

    // Find the daughters of the neutrino PFParticle
    std::vector<art::Ptr<recob::PFParticle> > finalStatePFParticles;
    for (unsigned int i = 0; i < pfParticleHandle->size(); ++i)
    {
        const art::Ptr<recob::PFParticle> pfParticle(pfParticleHandle, i);
        
        // If the particle is primary, it doesn't have a parent
        if (pfParticle->IsPrimary())
            continue;

        // Find the parent particle
        const auto parentIterator = pfParticleIdMap.find(pfParticle->Parent());
        if (parentIterator == pfParticleIdMap.end())
            throw cet::exception("WorkshopAnalyzerComplete") << "PFParticle has parent that's not in the input collection" << std::endl;
        
        // Check if the parent is primary
        if (!parentIterator->second->IsPrimary())
            continue;
        
        // Check if the parent has a neutrino PDG code
        const int parentPDG = std::abs(parentIterator->second->PdgCode());
        if (parentPDG != pandora::NU_E && parentPDG != pandora::NU_MU && parentPDG != pandora::NU_TAU)
            continue;

        // If we get to here, then the PFParticle's parent is the reconstructed neutrino! So it's a reconstructed final state
        finalStatePFParticles.push_back(pfParticle);
    }

    // Set the number of neutrino induced PFParticles
    m_nPFParticles = finalStatePFParticles.size();

    // Count how many PFParticles have an associated track
    m_nTracks = 0;
    art::FindManyP<recob::Track> pfPartToTrackAssoc(pfParticleHandle, event, m_trackLabel);
    for (const auto &pfParticle : finalStatePFParticles)
    {
        const auto associatedTracks = pfPartToTrackAssoc.at(pfParticle.key());

        if (associatedTracks.empty())
            continue;

        if (associatedTracks.size() > 1)
            throw cet::exception("WorkshopAnalyzerComplete") << "PFParticle has multiple associated tracks!" << std::endl;

        // Count this track!
        m_nTracks++;
    }

    // Fill the tree
    m_tree->Fill();
}

} // namespace pandorafnalworkshop 
