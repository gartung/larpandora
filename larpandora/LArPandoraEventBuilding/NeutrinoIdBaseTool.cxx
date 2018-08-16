/**
 *  @file   larpandora/LArPandoraEventBuilding/NeutrinoIdBaseTool.cxx
 *
 *  @brief  implementation of neutrino id base tool
 */

#include "nusimdata/SimulationBase/MCTruth.h"
#include "lardataobj/RecoBase/Hit.h"
#include "lardataobj/RecoBase/Cluster.h"
#include "lardataobj/RecoBase/PFParticle.h"

#include "canvas/Persistency/Common/FindManyP.h"

#include "Pandora/PdgTable.h"

#include "larpandora/LArPandoraEventBuilding/NeutrinoIdBaseTool.h"
#include "larpandora/LArPandoraInterface/LArPandoraHelper.h"

namespace lar_pandora
{

void NeutrinoIdBaseTool::GetSliceMetadata(SliceVector &slices, const art::Event &evt) const
{
    std::cout << "===== GETTING SLICE METADATA ==============================================================================" << std::endl;

    // TODO make configurable
    const std::string truthLabel("generator");
    const std::string mcParticleLabel("largeant");
    const std::string hitLabel("gaushit");
    const std::string backtrackLabel("gaushitTruthMatch");
    const std::string pandoraLabel("pandoraPatRec:allOutcomes");
    // ----

    const auto beamNuMCTruth(this->GetBeamNeutrinoMCTruth(evt, truthLabel));
    std::cout << "Found beam neutrino" << std::endl;
    std::cout << "  Interaction type : " << beamNuMCTruth->GetNeutrino().InteractionType() << std::endl;
    std::cout << "  Energy           : " << beamNuMCTruth->GetNeutrino().Nu().E() << std::endl;
    
    MCParticleVector mcParticles;
    this->CollectNeutrinoMCParticles(evt, truthLabel, mcParticleLabel, beamNuMCTruth, mcParticles);
    std::cout << "Found " << mcParticles.size() << " neutrino induced MCParticles!" << std::endl;

    HitVector hits;
    HitToBoolMap hitToIsNuInducedMap;
    this->GetHitOrigins(evt, hitLabel, backtrackLabel, mcParticles, hits, hitToIsNuInducedMap);
    std::cout << "Found " << hits.size() << " hits" << std::endl;

    const unsigned int nNuHits(this->CountNeutrinoHits(hits, hitToIsNuInducedMap));
    std::cout << "  Of which " << nNuHits << " are neutrino induced" << std::endl;

    PFParticlesToHits pfParticleToHitsMap;
    this->GetPFParticleToHitsMap(evt, pandoraLabel, pfParticleToHitsMap);

    for (unsigned int sliceIndex = 0; sliceIndex < slices.size(); ++sliceIndex)
    {
        const Slice &slice(slices.at(sliceIndex));
        HitVector hits;
        this->GetReconstructedHitsInSlice(slice, pfParticleToHitsMap, hits);

        const unsigned int nHitsInSlice(hits.size());
        const unsigned int nNuHitsInSlice(this->CountNeutrinoHits(hits, hitToIsNuInducedMap));

        std::cout << "Slice " << sliceIndex << std::endl;
        std::cout << "  nHits        : " << nHitsInSlice << std::endl;
        std::cout << "  nNuHits      : " << nNuHitsInSlice << std::endl;
        std::cout << "  purity       : " << ((nHitsInSlice == 0) ? -1.f : static_cast<float>(nNuHitsInSlice) / static_cast<float>(nHitsInSlice)) << std::endl;
        std::cout << "  completeness : " << ((nNuHits == 0) ? -1.f : static_cast<float>(nNuHitsInSlice) / static_cast<float>(nNuHits)) << std::endl;
    }

    std::cout << "===========================================================================================================" << std::endl;
}

// -----------------------------------------------------------------------------------------------------------------------------------------

art::Ptr<simb::MCTruth> NeutrinoIdBaseTool::GetBeamNeutrinoMCTruth(const art::Event &evt, const std::string &truthLabel) const
{
    art::Handle< std::vector<simb::MCTruth> > mcTruthHandle;
    evt.getByLabel(truthLabel, mcTruthHandle);
    
    if (!mcTruthHandle.isValid())
        throw cet::exception("LArPandora") << " NeutrinoIdBaseTool::GetBeamNeutrinoMCTruth - invalid MCTruth handle" << std::endl;

    bool foundNeutrino(false);
    art::Ptr<simb::MCTruth> beamNuMCTruth;   
    for (unsigned int i = 0; i < mcTruthHandle->size(); ++i)
    {
        const art::Ptr<simb::MCTruth> mcTruth(mcTruthHandle, i);
        
        if (mcTruth->Origin() != simb::kBeamNeutrino)
            continue;

        if (foundNeutrino)
            throw cet::exception("LArPandora") << " NeutrinoIdBaseTool::GetBeamNeutrinoMCTruth - found multiple beam neutrino MCTruth blocks" << std::endl;

        beamNuMCTruth = mcTruth;
        foundNeutrino = true;
    }

    if (!foundNeutrino)
        throw cet::exception("LArPandora") << " NeutrinoIdBaseTool::GetBeamNeutrinoMCTruth - found no beam neutrino MCTruth blocks" << std::endl;

    return beamNuMCTruth;
}

// -----------------------------------------------------------------------------------------------------------------------------------------

void NeutrinoIdBaseTool::CollectNeutrinoMCParticles(const art::Event &evt, const std::string &truthLabel, const std::string &mcParticleLabel, const art::Ptr<simb::MCTruth> &beamNuMCTruth, MCParticleVector &mcParticles) const
{
    art::Handle< std::vector<simb::MCTruth> > mcTruthHandle;
    evt.getByLabel(truthLabel, mcTruthHandle);
    
    if (!mcTruthHandle.isValid())
        throw cet::exception("LArPandora") << " NeutrinoIdBaseTool::CollectNeutrinoMCParticles - invalid MCTruth handle" << std::endl;

    art::FindManyP<simb::MCParticle> truthToMCParticleAssns(mcTruthHandle, evt, mcParticleLabel);
    mcParticles = truthToMCParticleAssns.at(beamNuMCTruth.key()); // ATTN will throw if association from beamNuMCTruth doesn't exist. We want this!
}

// -----------------------------------------------------------------------------------------------------------------------------------------

void NeutrinoIdBaseTool::GetHitOrigins(const art::Event &evt, const std::string &hitLabel, const std::string &backtrackLabel, const MCParticleVector &mcParticles, HitVector &hits, HitToBoolMap &hitToIsNuInducedMap) const
{
    // Collect the hits from the event
    art::Handle< std::vector<recob::Hit> > hitHandle;
    evt.getByLabel(hitLabel, hitHandle);

    if (!hitHandle.isValid())
        throw cet::exception("LArPandora") << " NeutrinoIdBaseTool::GetHitOrigins - invalid hit handle" << std::endl;

    art::FindManyP<simb::MCParticle> hitToMCParticleAssns(hitHandle, evt, backtrackLabel);

    for (unsigned int i = 0; i < hitHandle->size(); ++i)
    {
        const art::Ptr<recob::Hit> hit(hitHandle, i);
        hits.push_back(hit);

        const auto &particles(hitToMCParticleAssns.at(hit.key()));

        bool foundNuParticle(false);
        for (const auto &part : particles)
        {
            if (std::find(mcParticles.begin(), mcParticles.end(), part) == mcParticles.end())
                continue;

            foundNuParticle = true;
            break;
        }
        
        if (!hitToIsNuInducedMap.insert(HitToBoolMap::value_type(hit, foundNuParticle)).second)
            throw cet::exception("LArPandora") << " NeutrinoIdBaseTool::GetHitOrigins - repeated hits in input collection" << std::endl;
    }
}

// -----------------------------------------------------------------------------------------------------------------------------------------

unsigned int NeutrinoIdBaseTool::CountNeutrinoHits(const HitVector &hits, const HitToBoolMap &hitToIsNuInducedMap) const
{
    unsigned int nNuHits(0);
    for (const auto &hit : hits)
    {
        const auto it(hitToIsNuInducedMap.find(hit));

        if (it == hitToIsNuInducedMap.end())
            throw cet::exception("LArPandora") << " NeutrinoIdBaseTool::CountNeutrinoHits - can't find hit in hitToIsNuInducedMap" << std::endl;

        nNuHits += it->second ? 1 : 0;
    }

    return nNuHits;
}

// -----------------------------------------------------------------------------------------------------------------------------------------

void NeutrinoIdBaseTool::GetPFParticleToHitsMap(const art::Event &evt, const std::string &pandoraLabel, PFParticlesToHits &pfParticleToHitsMap) const
{
    art::Handle< std::vector<recob::PFParticle> > pfParticleHandle;
    evt.getByLabel(pandoraLabel, pfParticleHandle);
    
    if (!pfParticleHandle.isValid())
        throw cet::exception("LArPandora") << " NeutrinoIdBaseTool::GetPFParticleToHitsMap - invalid PFParticle handle" << std::endl;
    
    art::Handle< std::vector<recob::Cluster> > clusterHandle;
    evt.getByLabel(pandoraLabel, clusterHandle);
    
    if (!clusterHandle.isValid())
        throw cet::exception("LArPandora") << " NeutrinoIdBaseTool::GetPFParticleToHitsMap - invalid cluster handle" << std::endl;

    art::FindManyP<recob::Cluster> pfParticleToClusterAssns(pfParticleHandle, evt, pandoraLabel);
    art::FindManyP<recob::Hit> clusterToHitAssns(clusterHandle, evt, pandoraLabel);

    for (unsigned int iPart = 0; iPart < pfParticleHandle->size(); ++iPart)
    {
        const art::Ptr<recob::PFParticle> part(pfParticleHandle, iPart);
        HitVector hits;

        for (const auto &cluster : pfParticleToClusterAssns.at(part.key()))
        {
            for (const auto &hit : clusterToHitAssns.at(cluster.key()))
            {
                if (std::find(hits.begin(), hits.end(), hit) != hits.end())
                    throw cet::exception("LArPandora") << " NeutrinoIdBaseTool::GetPFParticleToHitsMap - double counted hits!" << std::endl;

                hits.push_back(hit);
            }
        }
        
        if (!pfParticleToHitsMap.insert(PFParticlesToHits::value_type(part, hits)).second)
            throw cet::exception("LArPandora") << " NeutrinoIdBaseTool::GetPFParticleToHitsMap - repeated input PFParticles" << std::endl;
    }
}

// -----------------------------------------------------------------------------------------------------------------------------------------

void NeutrinoIdBaseTool::GetReconstructedHitsInSlice(const Slice &slice, const PFParticlesToHits &pfParticleToHitsMap, HitVector &hits) const
{
    this->CollectHits(slice.GetNeutrinoHypothesis(), pfParticleToHitsMap, hits);
    this->CollectHits(slice.GetCosmicRayHypothesis(), pfParticleToHitsMap, hits);
}

// -----------------------------------------------------------------------------------------------------------------------------------------
    
void NeutrinoIdBaseTool::CollectHits(const PFParticleVector &pfParticles, const PFParticlesToHits &pfParticleToHitsMap, HitVector &hits) const
{
    for (const auto &part : pfParticles)
    {
        const auto it(pfParticleToHitsMap.find(part));
        if (it == pfParticleToHitsMap.end())
            throw cet::exception("LArPandora") << " NeutrinoIdBaseTool::CollectHits - can't find any hits associated to input PFParticle" << std::endl;

        for (const auto &hit : it->second)
        {
            if (std::find(hits.begin(), hits.end(), hit) == hits.end())
                hits.push_back(hit);
        }
    }
}

} // namespace lar_pandora
