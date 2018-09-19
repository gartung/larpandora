#ifndef WORKSHOP_TRACK_SHOWER_HELPER_H
#define WOKRSHOP_TRACK_SHOWER_HELPER_H

#include "larpandora/LArPandoraInterface/LArPandoraHelper.h"

namespace lar_pandora
{

/**
 *  @brief  Helper class for track vs. shower separation in the Pandora FNAL workshop
 */
class WorkshopTrackShowerHelper
{
public:
    /**
     *  @brief  Determine if a given input particle is a track or a shower by cutting on its score at the given value
     *
     *  @param  pfParticle the input PFParticle
     *  @param  pfParticleToMetadata the input mapping from PFParticles to their metadata object
     *  @param  minTrackScore the minimum track/shower score a particle can have to be considered a track
     *
     *  @return boolean - true for track, false for shower
     */
    static bool IsTrack(const recob::PFParticle &pfParticle, const PFParticlesToMetadata &pfParticleToMetadata, const float minTrackScore);

private:
    /**
     *  @brief  Get the track/shower score for a given input PFParticle
     *
     *  @param  pfParticle the input PFParticle
     *  @param  pfParticleToMetadata the input mapping from PFParticles to their metadata object
     *
     *  @return the track/shower score
     */
    static float GetTrackShowerScore(const recob::PFParticle &pfParticle, const PFParticlesToMetadata &pfParticleToMetadata);
};

} // namespace larpandora

#endif //  WORKSHOP_TRACK_SHOWER_HELPER_H

