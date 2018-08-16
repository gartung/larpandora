/**
 *  @file   larpandora/LArPandoraEventBuilding/LArPandoraSimpleNeutrinoId_tool.cxx
 *
 *  @brief  implementation of the lar pandora simple neutrino id tool
 */

#include "art/Utilities/ToolMacros.h"
#include "fhiclcpp/ParameterSet.h"

#include "larpandora/LArPandoraEventBuilding/NeutrinoIdBaseTool.h"
#include "larpandora/LArPandoraEventBuilding/Slice.h"

namespace lar_pandora
{

/**
 *  @brief  Simple neutrino ID tool that selects the most likely neutrino slice using the scores from Pandora
 */
class SimpleNeutrinoId : NeutrinoIdBaseTool
{
public:
    /**
     *  @brief  Default constructor
     *
     *  @param  pset FHiCL parameter set
     */
    SimpleNeutrinoId(fhicl::ParameterSet const &pset);

    /**
     *  @brief  Classify slices as neutrino or cosmic
     *
     *  @param  slices the input vector of slices to classify
     *  @param  evt the art event
     */
    void ClassifySlices(SliceVector &slices, const art::Event &evt) override;
};

DEFINE_ART_CLASS_TOOL(SimpleNeutrinoId)

} // namespace lar_pandora

//------------------------------------------------------------------------------------------------------------------------------------------
// implementation follows

namespace lar_pandora
{
    
SimpleNeutrinoId::SimpleNeutrinoId(fhicl::ParameterSet const &/*pset*/)
{
}

//------------------------------------------------------------------------------------------------------------------------------------------

void SimpleNeutrinoId::ClassifySlices(SliceVector &slices, const art::Event &evt) 
{
    if (slices.empty()) return;

    // Find the most probable slice
    float highestNuScore(-std::numeric_limits<float>::max());
    unsigned int mostProbableSliceIndex(std::numeric_limits<unsigned int>::max());

    for (unsigned int sliceIndex = 0; sliceIndex < slices.size(); ++sliceIndex)
    {
        const float nuScore(slices.at(sliceIndex).GetNeutrinoScore());
        std::cout << "Slice " << sliceIndex << " - " << nuScore << std::endl;
        if (nuScore > highestNuScore)
        {
            highestNuScore = nuScore;
            mostProbableSliceIndex = sliceIndex;
        }
    }

    std::cout << "Tagging slice " << mostProbableSliceIndex << std::endl;

    // Tag the most probable slice as a neutrino
    slices.at(mostProbableSliceIndex).TagAsNeutrino();

    this->GetSliceMetadata(slices, evt);
}

} // namespace lar_pandora
