/**
 *  @file   larpandora/LArPandoraEventBuilding/Slice.h
 *
 *  @brief  header for the lar pandora slice class
 */

#ifndef LAR_PANDORA_NEUTRINO_ID_BASE_TOOL_H
#define LAR_PANDORA_NEUTRINO_ID_BASE_TOOL_H 1

#include "art/Framework/Principal/Event.h"

#include "larpandora/LArPandoraEventBuilding/Slice.h"
#include "larpandora/LArPandoraInterface/LArPandoraHelper.h"

namespace lar_pandora
{

/**
 *  @brief  Abstract base class for a neutrino ID tool
 */
class NeutrinoIdBaseTool
{
public:
    virtual ~NeutrinoIdBaseTool() noexcept = default;

    /**
     *  @brief  The tools interface function. Here the derived tool will classify the input slices
     *
     *  @param  slices the input vector of slices to classify
     *  @param  evt the art event
     */
    virtual void ClassifySlices(SliceVector &slices, const art::Event &evt) = 0;
    
    class SliceMetadata
    {
    public:
        SliceMetadata();

        float m_purity;
        float m_completeness;
        float m_nHits;
        bool  m_isMostComplete;
    };
    typedef std::vector<SliceMetadata> SliceMetadataVector;

protected:
    void GetSliceMetadata(const SliceVector &slices, const art::Event &evt, SliceMetadataVector &sliceMetadata, int &interactionType, float &nuEnergy) const;

private:
    
    typedef std::unordered_map<art::Ptr<recob::Hit>, bool> HitToBoolMap;

    art::Ptr<simb::MCTruth> GetBeamNeutrinoMCTruth(const art::Event &evt, const std::string &truthLabel) const;
    void CollectNeutrinoMCParticles(const art::Event &evt, const std::string &truthLabel, const std::string &mcParticleLabel, const art::Ptr<simb::MCTruth> &beamNuMCTruth, MCParticleVector &mcParticles) const;
    void GetHitOrigins(const art::Event &evt, const std::string &hitLabel, const std::string &backtrackLabel, const MCParticleVector &mcParticles, HitVector &hits, HitToBoolMap &hitToIsNuInducedMap) const;
    unsigned int CountNeutrinoHits(const HitVector &hits, const HitToBoolMap &hitToIsNuInducedMap) const;
    void GetPFParticleToHitsMap(const art::Event &evt, const std::string &pandoraLabel, PFParticlesToHits &pfParticleToHitsMap) const;
    void GetReconstructedHitsInSlice(const Slice &slice, const PFParticlesToHits &pfParticleToHitsMap, HitVector &hits) const;
    void CollectHits(const PFParticleVector &pfParticles, const PFParticlesToHits &pfParticleToHitsMap, HitVector &hits) const;
    void GetSliceMetadata(const SliceVector &slices, const PFParticlesToHits &pfParticleToHitsMap, const HitToBoolMap &hitToIsNuInducedMap, const unsigned int nNuHits, SliceMetadataVector &sliceMetadata) const;
};

} // namespace lar_pandora

#endif // #ifndef LAR_PANDORA_NEUTRINO_ID_BASE_TOOL_H
