#include "larpandora/LArPandoraEventBuilding/WorkshopTrackShowerHelper.h"

namespace lar_pandora
{

bool WorkshopTrackShowerHelper::IsTrack(const recob::PFParticle &pfParticle, const PFParticlesToMetadata &pfParticleToMetadata, const float minTrackScore)
{
    // ATTN the below are temporary so the template will compile whilst empty
    // PLEASE DELETE
    (void) pfParticle;
    (void) pfParticleToMetadata;
    (void) minTrackScore;
    return false;
    // END DELETE
}

// -----------------------------------------------------------------------------------------------------------------------------------------

float WorkshopTrackShowerHelper::GetTrackShowerScore(const recob::PFParticle &pfParticle, const PFParticlesToMetadata &pfParticleToMetadata)
{
    // ATTN the below are temporary so the template will compile whilst empty
    // PLEASE DELETE
    (void) pfParticle;
    (void) pfParticleToMetadata;
    return 0.f;
    // END DELETE
}

} // namespace larpandora
