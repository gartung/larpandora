/**
 *  @file   larpandora/LArPandoraInterface/LArPandora.h
 *
 *  @brief  Base producer module for reconstructing recob::PFParticles from recob::Hits
 *
 */

#ifndef LAR_PANDORA_H
#define LAR_PANDORA_H 1

#include "LArPandoraInterface/ILArPandora.h"

class TTree;

//------------------------------------------------------------------------------------------------------------------------------------------

namespace lar_pandora
{

/**
 *  @brief  LArPandora class
 */
class LArPandora : public ILArPandora
{
public:
    /**
     *  @brief  Constructor
     *
     *  @param  pset the parameter set
     */
    LArPandora(fhicl::ParameterSet const &pset);

    /**
     *  @brief  Destructor
     */
    ~LArPandora();

    void beginJob();
    void produce(art::Event &evt);

private:
    void DeletePandoraInstances();
    void CreatePandoraInput(art::Event &evt, IdToHitMap &idToHitMap);
    void ProcessPandoraOutput(art::Event &evt, const IdToHitMap &idToHitMap);
    void RunPandoraInstances();
    void ResetPandoraInstances();

    /**
     *  @brief  Create a new Pandora instance and register lar content algs and plugins
     *
     *  @return the address of the new Pandora instance
     */
    const pandora::Pandora *CreateNewPandora() const;

    /**
     *  @brief  Set x0 values for all pfos created by the specified pandora isntance
     *
     *  @param  pPandora the address of the relevant pandora instance
     */
    void SetParticleX0Values(const pandora::Pandora *const pPandora) const;

    /**
     *  @brief Initialize the internal monitoring
     */
    void InitializeMonitoring();

    bool         m_enableProduction;        ///<
    bool         m_enableMCParticles;       ///<
    bool         m_enableMonitoring;        ///<

    std::string  m_configFile;              ///<
    std::string  m_stitchingConfigFile;     ///<

    std::string  m_geantModuleLabel;        ///<
    std::string  m_hitfinderModuleLabel;    ///<
    std::string  m_spacepointModuleLabel;   ///<
    std::string  m_pandoraModuleLabel;      ///<

    bool         m_useHitWidths;            ///<
    int          m_uidOffset;               ///<

    double       m_dx_cm;                   ///<
    double       m_int_cm;                  ///<
    double       m_rad_cm;                  ///< 
    double       m_dEdX_max;                ///< 
    double       m_dEdX_mip;                ///< 
    double       m_mips_to_gev;             ///< 
    double       m_recombination_factor;    ///<

    bool         m_buildTracks;             ///<
    bool         m_buildShowers;            ///<

    TTree       *m_pRecoTree;               ///<
    int          m_run;                     ///<
    int          m_event;                   ///<
    int          m_hits;                    ///<
    int          m_pandoraHits;             ///<
    float        m_collectionTime;          ///<
    float        m_inputTime;               ///<
    float        m_processTime;             ///<
    float        m_outputTime;              ///<
};

} // namespace lar_pandora

#endif // #ifndef LAR_PANDORA_H
