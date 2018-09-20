#include "larpandora/LArPandoraEventBuilding/WorkshopTrackShowerHelperComplete.h"

#include "larpandora/LArPandoraObjects/PFParticleMetadata.h"

namespace lar_pandora
{

bool WorkshopTrackShowerHelperComplete::IsTrack(const art::Ptr<recob::PFParticle> &pfParticle, const PFParticlesToMetadata &pfParticleToMetadata, const float minTrackScore)
{
    // Only neutrino induced particles will have a track/shower score available
    // If we can't use the track/shower score, then let's default to Pandora's score
    // I.e. a track for primary CR muons, and a shower for delta-rays
    bool isTrack = LArPandoraHelper::IsTrack(pfParticle);

    try
    {
        const float trackScore = WorkshopTrackShowerHelperComplete::GetTrackShowerScore(pfParticle, pfParticleToMetadata);

        // When the track score can't be calculated, it will be saved as a negative value
        if (trackScore >= 0)
            isTrack = trackScore >= minTrackScore;
    }
    catch (const cet::exception &)
    {
        // Couldn't get the track score, so we just use the default from Pandora
    }

    return isTrack;
}

// -----------------------------------------------------------------------------------------------------------------------------------------

float WorkshopTrackShowerHelperComplete::GetTrackShowerScore(const art::Ptr<recob::PFParticle> &pfParticle, const PFParticlesToMetadata &pfParticleToMetadata)
{
    // Find the PFParticle in the metadata map
    const auto itParticle = pfParticleToMetadata.find(pfParticle);

    // Check the PFParticle was found
    if (itParticle == pfParticleToMetadata.end())
        throw cet::exception("WorkshopTrackShowerHelperComplete") << "PFParticle has no metadata" << std::endl;

    // There should only be one metadata for each PFParticle
    if (itParticle->second.size() != 1)
        throw cet::exception("WorkshopTrackShowerHelperComplete") << "PFParticle has mutiple metadata" << std::endl;

    // The metadata vector has size one as required, so just take the first element
    const auto metadata = itParticle->second.front();

    // Now get the properties map - this is a map from a string (the property name) to a float (the property value)
    const auto propertiesMap = metadata->GetPropertiesMap();

    // Look for the track shower ID score
    const auto itScore = propertiesMap.find("TrackScore");

    // Check the track score was available
    if (itScore == propertiesMap.end())
        throw cet::exception("WorkshopTrackShowerHelperComplete") << "PFParticle has no track score" << std::endl;

    return itScore->second;
}

} // namespace larpandora
