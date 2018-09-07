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
    
    /**
     *  @brief  Class to hold MC metdata about slices
     */ 
    class SliceMetadata
    {
    public:
        /**
         *  @brief  Default constructor
         */
        SliceMetadata();

        float        m_purity;          ///< The fraction of hits in the slice that are neutrino induced
        float        m_completeness;    ///< The fraction of all neutrino induced hits tthat are inteh slice
        unsigned int m_nHits;           ///< The number of hits in the slice
        bool         m_isMostComplete;  ///< If the slice has the highest completeness in the event
    };
    typedef std::vector<SliceMetadata> SliceMetadataVector;

protected:
    /**
     *  @brief  Get MC metadata about each slice
     *
     *  @param  slices the input vector of slices
     *  @param  evt the art event
     *  @param  sliceMetadata the output vector of slice metadata (mapping 1:1 to the slices)
     *  @param  interactionType the output true interaction type code of the MCNeutrino
     *  @param  nuEnergy the output true energy of the neutrino
     */ 
    void GetSliceMetadata(const SliceVector &slices, const art::Event &evt, SliceMetadataVector &sliceMetadata, int &interactionType, float &nuEnergy) const;

    /**
     *  @brief  Get MC metadata about each slice
     *
     *  @param  slice the slice
     *  
     *  @return the number of hits in the slice
     */
    unsigned int GetNHitsInSlice(const Slice &slice, const art::Event &evt) const;

private:
    typedef std::unordered_map<art::Ptr<recob::Hit>, bool> HitToBoolMap;

    /**
     *  @brief  Get the MCTruth block for the simulated beam neutrino
     *
     *  @param  evt the art event
     *  @param  truthLabel the label of the MCTruth producer
     *
     *  @return the MCTruth block for the simulated beam neutrino
     */
    art::Ptr<simb::MCTruth> GetBeamNeutrinoMCTruth(const art::Event &evt, const std::string &truthLabel) const;

    /**
     *  @brief  Collect all MCParticles that come from the beam neutrino interaction
     *
     *  @param  evt the art event
     *  @param  truthLabel the label of the MCTruth producer
     *  @param  mcParticleLabel the label of the MCParticle producer
     *  @param  beamNuMCTruth the MCTruth block for the beam neutrino
     *  @param  mcParticles the output vector of neutrino induced MCParticles
     */
    void CollectNeutrinoMCParticles(const art::Event &evt, const std::string &truthLabel, const std::string &mcParticleLabel, const art::Ptr<simb::MCTruth> &beamNuMCTruth, MCParticleVector &mcParticles) const;

    /**
     *  @brief  For each hit in the event, determine if any of it's charge was deposited by a neutrino induced particle
     *
     *  @param  evt the art event
     *  @param  hitLabel the label of the Hit producer
     *  @param  backtrackLabel the label of the Hit->MCParticle association producer - backtracker
     *  @param  mcParticles the input vector of neutrino induced MCParticles
     *  @param  hits the output vector of all hits
     *  @param  hitToIsNuInducedMap the output mapping from hits to a bool = true if hit is neutrino induced
     */
    void GetHitOrigins(const art::Event &evt, const std::string &hitLabel, const std::string &backtrackLabel, const MCParticleVector &mcParticles, HitVector &hits, HitToBoolMap &hitToIsNuInducedMap) const;
    
    /**
     *  @brief  Count the number of hits in an input vector that are neutrino induced
     *
     *  @param  hits the input vector of hits
     *  @param  hitToIsNuInducedMap the mapping from hits to isNuInduced boolean
     *
     *  @return the number of hits that are neutrino induced
     */
    unsigned int CountNeutrinoHits(const HitVector &hits, const HitToBoolMap &hitToIsNuInducedMap) const;

    /**
     *  @brief  Get the mapping from PFParticles to associated hits (via clusters)
     *
     *  @param  evt the art event
     *  @param  pandoraLabel the label of the PFParticle <-> Cluster and Cluster <-> Hit associations - Pandora pattern recognition
     *  @param  pfParticleToHitsMap the output mapping from PFParticles to associated hits
     */
    void GetPFParticleToHitsMap(const art::Event &evt, const std::string &pandoraLabel, PFParticlesToHits &pfParticleToHitsMap) const;

    /**
     *  @brief  Collect the hits in the slice that have been added to a PFParticle (under either reconstruction hypothesis)
     *
     *  @param  slice the input slice
     *  @param  pfParticleToHitsMap the input mapping from PFParticles to hits
     *  @param  hits the output vector of reconstructed hits in the slice
     */
    void GetReconstructedHitsInSlice(const Slice &slice, const PFParticlesToHits &pfParticleToHitsMap, HitVector &hits) const;

    /**
     *  @brief  Collect the hits in a given vector of PFParticles
     *
     *  @param  pfParticles the input vector of PFParticles
     *  @param  pfParticleToHitsMap the input mapping from PFParticles to hits
     *  @param  hits the output vector of hits
     */
    void CollectHits(const PFParticleVector &pfParticles, const PFParticlesToHits &pfParticleToHitsMap, HitVector &hits) const;

    /**
     *  @brief  Calculate the MC slice metadata
     *
     *  @param  slices the input vector of slices
     *  @param  pfParticleToHitsMap the input mapping from PFParticles to hits
     *  @param  hitToIsNuInducedMap the input mapping from hits to isNuInduced boolean
     *  @param  nNuHits the total number of neutrino induced hits in the event
     *  @param  sliceMetadata the output vector of metadata objects correspoinding 1:1 to the input slices
     */
    void GetSliceMetadata(const SliceVector &slices, const PFParticlesToHits &pfParticleToHitsMap, const HitToBoolMap &hitToIsNuInducedMap, const unsigned int nNuHits, SliceMetadataVector &sliceMetadata) const;
};

} // namespace lar_pandora

#endif // #ifndef LAR_PANDORA_NEUTRINO_ID_BASE_TOOL_H
