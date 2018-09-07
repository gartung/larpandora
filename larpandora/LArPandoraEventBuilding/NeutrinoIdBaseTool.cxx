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

void NeutrinoIdBaseTool::GetSliceMetadata(const SliceVector &slices, const art::Event &evt, SliceMetadataVector &sliceMetadata, int &interactionType, float &nuEnergy) const
{
    // TODO make configurable
    const std::string truthLabel("generator");
    const std::string mcParticleLabel("largeant");
    const std::string hitLabel("gaushit");
    const std::string backtrackLabel("gaushitTruthMatch");
    const std::string pandoraLabel("pandoraPatRec:allOutcomes");
    // ----

    // Find the beam neutrino in MC
    const auto beamNuMCTruth(this->GetBeamNeutrinoMCTruth(evt, truthLabel));
    const auto mcNeutrino(beamNuMCTruth->GetNeutrino());
    interactionType = mcNeutrino.InteractionType();
    nuEnergy = mcNeutrino.Nu().E();
    
    // Collect all MC particles resulting from the beam neutrino
    MCParticleVector mcParticles;
    this->CollectNeutrinoMCParticles(evt, truthLabel, mcParticleLabel, beamNuMCTruth, mcParticles);

    // Get the hits and determine which are neutrino induced
    HitVector hits;
    HitToBoolMap hitToIsNuInducedMap;
    this->GetHitOrigins(evt, hitLabel, backtrackLabel, mcParticles, hits, hitToIsNuInducedMap);
    const unsigned int nNuHits(this->CountNeutrinoHits(hits, hitToIsNuInducedMap));

    // Get the mapping from PFParticle to hits through clusters
    PFParticlesToHits pfParticleToHitsMap;
    this->GetPFParticleToHitsMap(evt, pandoraLabel, pfParticleToHitsMap);

    // Calculate the metadata for each slice
    this->GetSliceMetadata(slices, pfParticleToHitsMap, hitToIsNuInducedMap, nNuHits, sliceMetadata);
}

// -----------------------------------------------------------------------------------------------------------------------------------------

unsigned int NeutrinoIdBaseTool::GetNHitsInSlice(const Slice &slice, const art::Event &evt) const
{
    // TODO make configurable
    const std::string pandoraLabel("pandoraPatRec:allOutcomes");
    // ----
    
    // Get the mapping from PFParticle to hits through clusters
    PFParticlesToHits pfParticleToHitsMap;
    this->GetPFParticleToHitsMap(evt, pandoraLabel, pfParticleToHitsMap);

    HitVector hits;
    this->GetReconstructedHitsInSlice(slice, pfParticleToHitsMap, hits);

    return hits.size();
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

// -----------------------------------------------------------------------------------------------------------------------------------------

void NeutrinoIdBaseTool::GetSliceMetadata(const SliceVector &slices, const PFParticlesToHits &pfParticleToHitsMap, const HitToBoolMap &hitToIsNuInducedMap, const unsigned int nNuHits, SliceMetadataVector &sliceMetadata) const
{
    if (!sliceMetadata.empty())
        throw cet::exception("LArPandora") << " NeutrinoIdBaseTool::GetSliceMetadata - non empty input metadata vector" << std::endl;

    if (slices.empty())
        return;

    unsigned int mostCompleteSliceIndex(0);
    unsigned int maxNuHits(0);

    for (unsigned int sliceIndex = 0; sliceIndex < slices.size(); ++sliceIndex)
    {
        const Slice &slice(slices.at(sliceIndex));
        HitVector hits;
        this->GetReconstructedHitsInSlice(slice, pfParticleToHitsMap, hits);

        const unsigned int nHitsInSlice(hits.size());
        const unsigned int nNuHitsInSlice(this->CountNeutrinoHits(hits, hitToIsNuInducedMap));

        // ATTN possible reproducibility issue here if input slices aren't sorted
        // TODO check this
        if (nNuHitsInSlice > maxNuHits)
        {
            mostCompleteSliceIndex = sliceIndex;
            maxNuHits = nNuHitsInSlice;
        }

        SliceMetadata metadata;
        metadata.m_nHits = nHitsInSlice;
        metadata.m_purity = ((nHitsInSlice == 0) ? -1.f : static_cast<float>(nNuHitsInSlice) / static_cast<float>(nHitsInSlice));
        metadata.m_completeness = ((nNuHits == 0) ? -1.f : static_cast<float>(nNuHitsInSlice) / static_cast<float>(nNuHits));
        metadata.m_isMostComplete = false;

        sliceMetadata.push_back(metadata);
    }

    sliceMetadata.at(mostCompleteSliceIndex).m_isMostComplete = true;
}

// -----------------------------------------------------------------------------------------------------------------------------------------
        
NeutrinoIdBaseTool::SliceMetadata::SliceMetadata() :
    m_purity(-std::numeric_limits<float>::max()),
    m_completeness(-std::numeric_limits<float>::max()),
    m_nHits(std::numeric_limits<unsigned int>::max()),
    m_isMostComplete(false)
{
}

} // namespace lar_pandora
