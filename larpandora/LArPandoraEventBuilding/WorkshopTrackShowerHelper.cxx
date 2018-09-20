#include "larpandora/LArPandoraEventBuilding/WorkshopTrackShowerHelper.h"

#include "larpandora/LArPandoraObjects/PFParticleMetadata.h"

namespace lar_pandora
{

bool WorkshopTrackShowerHelper::IsTrack(const art::Ptr<recob::PFParticle> &pfParticle, const PFParticlesToMetadata &pfParticleToMetadata, const float minTrackScore)
{
    /* TODO remove this block, it's only here so the code will compile while empty */
    (void) pfParticle;
    (void) pfParticleToMetadata;
    (void) minTrackScore;
    return false;
    /* End of block to remove */
}

// -----------------------------------------------------------------------------------------------------------------------------------------

float WorkshopTrackShowerHelper::GetTrackShowerScore(const art::Ptr<recob::PFParticle> &pfParticle, const PFParticlesToMetadata &pfParticleToMetadata)
{
    /* TODO remove this block, it's only here so the code will compile while empty */
    (void) pfParticle;
    (void) pfParticleToMetadata;
    return false;
    /* End of block to remove */
}

} // namespace larpandora
