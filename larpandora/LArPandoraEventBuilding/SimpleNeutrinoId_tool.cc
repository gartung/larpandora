/**
 *  @file   larpandora/LArPandoraEventBuilding/LArPandoraSimpleNeutrinoId_tool.cxx
 *
 *  @brief  implementation of the lar pandora simple neutrino id tool
 */

#include "art/Utilities/ToolMacros.h"
#include "fhiclcpp/ParameterSet.h"

#include "TTree.h"

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
        
private:
    bool          m_enableTestMode;     ///< If test mode should be enabled - in test mode, output slice metadata to external root file

    TTree        *m_tree;               ///< The TTree to fill in test mode
    int           m_interactionType;    ///< The interaction type of the current event 
    float         m_nuEnergy;           ///< The energy of the neutrino in the current event
    SliceMetadata m_outputMetadata;     ///< The metadata object for the current slice to output
    float         m_topologicalScore;   ///< The topological score from pandora for the current slice to output
    bool          m_isTaggedAsNu;       ///< If the current slice has been tagged as a neutrino - to output
};

DEFINE_ART_CLASS_TOOL(SimpleNeutrinoId)

} // namespace lar_pandora

//------------------------------------------------------------------------------------------------------------------------------------------
// implementation follows

#include "art/Framework/Services/Optional/TFileService.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"

namespace lar_pandora
{
    
SimpleNeutrinoId::SimpleNeutrinoId(fhicl::ParameterSet const &/*pset*/) :
    m_enableTestMode(true), // TODO make this configurable
    m_interactionType(-std::numeric_limits<int>::max()),
    m_nuEnergy(-std::numeric_limits<float>::max()),
    m_topologicalScore(-std::numeric_limits<float>::max()),
    m_isTaggedAsNu(false)
{
    if (m_enableTestMode)
    {
        art::ServiceHandle<art::TFileService> fileService;
        m_tree = fileService->make<TTree>("sliceData","");

        m_tree->Branch("interactionType", &m_interactionType, "interactionType/I");
        m_tree->Branch("nuEnergy", &m_nuEnergy, "nuEnergy/F");

        m_tree->Branch("nHits", &m_outputMetadata.m_nHits, "nHits/i");
        m_tree->Branch("purity", &m_outputMetadata.m_purity, "purity/F");
        m_tree->Branch("completeness", &m_outputMetadata.m_completeness, "completeness/F");
        m_tree->Branch("isMostComplete", &m_outputMetadata.m_isMostComplete, "isMostComplete/O");
        
        m_tree->Branch("topologicalScore", &m_topologicalScore, "topologicalScore/F");
        m_tree->Branch("isTaggedAsNu", &m_isTaggedAsNu, "isTaggedAsNu/O");
    }
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
        if (nuScore > highestNuScore)
        {
            highestNuScore = nuScore;
            mostProbableSliceIndex = sliceIndex;
        }
    }

    // Tag the most probable slice as a neutrino
    slices.at(mostProbableSliceIndex).TagAsNeutrino();

    if (m_enableTestMode)
    {
        SliceMetadataVector sliceMetadata;
        this->GetSliceMetadata(slices, evt, sliceMetadata, m_interactionType, m_nuEnergy);

        for (unsigned int sliceIndex = 0; sliceIndex < slices.size(); ++sliceIndex)
        {
            const auto &slice(slices.at(sliceIndex));
            
            m_topologicalScore = slice.GetNeutrinoScore();
            m_isTaggedAsNu = slice.IsTaggedAsNeutrino();
            m_outputMetadata = sliceMetadata.at(sliceIndex);

            m_tree->Fill();
        }
    }
}

} // namespace lar_pandora
