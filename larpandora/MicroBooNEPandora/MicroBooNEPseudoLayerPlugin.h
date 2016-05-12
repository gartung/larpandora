/**
 *  @file   larpandora/MicroBooNEPandora/MicroBooNEPseudoLayerPlugin.h
 *
 *  @brief  Header file for the MicroBooNE pseudo layer plugin class.
 *
 *  $Log: $
 */
#ifndef MICRO_BOONE_PSEUDO_LAYER_PLUGIN_H
#define MICRO_BOONE_PSEUDO_LAYER_PLUGIN_H 1

// Pandora includes
#include "larpandoracontent/LArPlugins/LArPseudoLayerPlugin.h"

namespace lar_pandora
{

/**
 *  @brief  MicroBooNEPseudoLayerPlugin class
 */
class MicroBooNEPseudoLayerPlugin : public lar_content::LArPseudoLayerPlugin
{
public:
    /**
     *  @brief  Default constructor
     */
    MicroBooNEPseudoLayerPlugin();
};

} // namespace lar

#endif // #ifndef MICRO_BOONE_PSEUDO_LAYER_PLUGIN_H
