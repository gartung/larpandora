/**
 *  @file   LArPandora/LArPandora.cxx
 * 
 *  @brief  Implementation of the LArPandora producer.
 * 
 *  $Log: $
 */

// Framework includes
#include "art/Framework/Services/Optional/TFileService.h"
#include "cetlib/search_path.h"

// LArSoft includes 
#include "Geometry/Geometry.h"
#include "Utilities/LArProperties.h"
#include "Utilities/DetectorProperties.h"
#include "Utilities/AssociationUtil.h"
#include "SimulationBase/MCTruth.h"
#include "RecoBase/Hit.h"
#include "RecoBase/Cluster.h"

// ROOT includes
#include "TTree.h"

// Pandora includes
#include "Api/PandoraApi.h"
#include "Objects/ParticleFlowObject.h"

// Pandora LArContent includes
#include "LArContent.h"

// Local includes
#include "LArPandora.h"
#include "MicroBooNEPseudoLayerCalculator.h"
#include "MicroBooNETransformationCalculator.h"

// System includes
#include <sys/time.h>
#include <iostream>

namespace lar_pandora
{

LArPandora::LArPandora(fhicl::ParameterSet const &pset)
{
    this->reconfigure(pset);
    m_pPandora = new pandora::Pandora();

    produces< std::vector<recob::Cluster> >();
    produces< art::Assns<recob::Cluster, recob::Hit> >();
}

//------------------------------------------------------------------------------------------------------------------------------------------

LArPandora::~LArPandora()
{
    delete m_pPandora;
}

//------------------------------------------------------------------------------------------------------------------------------------------

void LArPandora::reconfigure(fhicl::ParameterSet const &pset)
{
    m_enableProduction = pset.get<bool>("EnableProduction",true);
    m_enableMCParticles = pset.get<bool>("EnableMCParticles",false);
    m_enableMonitoring = pset.get<bool>("EnableMonitoring",false);

    m_configFile = pset.get<std::string>("ConfigFile");
    m_geantModuleLabel = pset.get<std::string>("GeantModuleLabel","largeant");
    m_hitfinderModuleLabel = pset.get<std::string>("HitFinderModuleLabel","ffthit");
}

//------------------------------------------------------------------------------------------------------------------------------------------

void LArPandora::beginJob()
{
    this->InitializePandora();  

    if (m_enableMonitoring)
        this->InitializeMonitoring();
}

//------------------------------------------------------------------------------------------------------------------------------------------

void LArPandora::endJob()
{
}

//------------------------------------------------------------------------------------------------------------------------------------------

void LArPandora::produce(art::Event &evt)
{ 
    mf::LogInfo("LArPandora") << " *** LArPandora::produce(...)  [Run=" << evt.run() << ", Event=" << evt.id().event() << "] *** " << std::endl;
    std::cout << " *** LArPandora::produce(...)  [Run=" << evt.run() << ", Event=" << evt.id().event() << "] *** " << std::endl;

    HitVector theArtHits;
    HitToParticleMap theHitToParticleMap;
    TruthToParticleMap theTruthToParticleMap;
    ParticleMap theParticleMap;
    HitMap thePandoraHits;

    this->CollectArtHits(evt, theArtHits, theHitToParticleMap);
    this->CreatePandoraHits(theArtHits, thePandoraHits);

    if (m_enableMCParticles && !evt.isRealData())
    {
        this->CollectArtParticles(evt, theParticleMap, theTruthToParticleMap);
        this->CreatePandoraParticles(theParticleMap, theTruthToParticleMap);
        this->CreatePandoraLinks(thePandoraHits, theHitToParticleMap);
    }

    struct timeval startTime, endTime;

    if (m_enableMonitoring)
        (void) gettimeofday(&startTime, NULL);

    this->RunPandora();

    if (m_enableMonitoring)
        (void) gettimeofday(&endTime, NULL);

    if (m_enableProduction)
        this->ProduceArtClusters(evt, thePandoraHits);

    this->ResetPandora();
  
    if (m_enableMonitoring)
    {
        m_run   = evt.run();
        m_event = evt.id().event();
        m_hits  = static_cast<int>(theArtHits.size());
        m_time  = (endTime.tv_sec + (endTime.tv_usec / 1.e6)) - (startTime.tv_sec + (startTime.tv_usec / 1.e6));
        mf::LogDebug("LArPandora") << "   Summary: Run=" << m_run << ", Event=" << m_event << ", Hits=" << m_hits << ", Time=" << m_time << std::endl;
        m_pRecoTree->Fill();
    }

    mf::LogDebug("LArPandora") << " *** LArPandora::produce(...)  [Run=" << evt.run() << ", Event=" << evt.id().event() << "]  Done! *** " << std::endl;
    std::cout << " *** LArPandora::produce(...)  [Run=" << evt.run() << ", Event=" << evt.id().event() << "]  Done! *** " << std::endl;
}

//------------------------------------------------------------------------------------------------------------------------------------------

void LArPandora::InitializePandora() const
{ 
    mf::LogDebug("LArPandora") << " *** LArPandora::InitializePandora(...) *** " << std::endl;

    // Find the Pandora settings file (must be within 'FW_SEARCH_PATH')
    cet::search_path sp("FW_SEARCH_PATH");
    std::string configFileName("");

    mf::LogDebug("LArPandora") << "   Load Pandora settings: " << m_configFile << std::endl;
    mf::LogDebug("LArPandora") << "   Search path: " << sp.to_string() << std::endl;

    if (!sp.find_file(m_configFile, configFileName))
    {
        mf::LogError("LArPandora") << "   Failed to find: " << m_configFile << std::endl;
        throw pandora::StatusCodeException(pandora::STATUS_CODE_NOT_FOUND);
    }
    else
    {
        mf::LogDebug("LArPandora") << "   Found it: " <<  configFileName << std::endl;
    }
    
    // Identify the Geometry and load the calculators
    art::ServiceHandle<geo::Geometry> theGeometry;

    if (theGeometry->DetId() == geo::kMicroBooNE)
    {
        PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=, LArContent::SetLArBFieldCalculator(*m_pPandora, new lar::LArBFieldCalculator));
        PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=, LArContent::SetLArPseudoLayerCalculator(*m_pPandora, new MicroBooNEPseudoLayerCalculator));
        PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=, LArContent::SetLArTransformationCalculator(*m_pPandora, new MicroBooNETransformationCalculator));
    }
    else
    {
        mf::LogError("LArPandora") << " Geometry helpers not yet available for detector: " << theGeometry->GetDetectorName() << std::endl;
        throw pandora::StatusCodeException(pandora::STATUS_CODE_INVALID_PARAMETER);   
    }

    PandoraApi::GeometryParameters geometryParameters;
    PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=, PandoraApi::Geometry::Create(*m_pPandora, geometryParameters));

    // Register the algorithms and read the settings
    PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=, LArContent::RegisterAlgorithms(*m_pPandora));
    PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=, LArContent::RegisterHelperFunctions(*m_pPandora));
    PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=, LArContent::RegisterResetFunctions(*m_pPandora));

    PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=, PandoraApi::ReadSettings(*m_pPandora, configFileName));

    mf::LogDebug("LArPandora") << " *** LArPandora::InitializePandora(...)  Done! *** " << std::endl;
}

//------------------------------------------------------------------------------------------------------------------------------------------

void LArPandora::InitializeMonitoring()
{
    art::ServiceHandle<art::TFileService> tfs;
    m_pRecoTree = tfs->make<TTree>("pandora", "LAr Reco");
    m_pRecoTree->Branch("run", &m_run, "run/I");
    m_pRecoTree->Branch("event", &m_event, "event/I");
    m_pRecoTree->Branch("hits", &m_hits, "hits/I");
    m_pRecoTree->Branch("time", &m_time, "time/F");
}

//------------------------------------------------------------------------------------------------------------------------------------------

void LArPandora::CollectArtHits(const art::Event &evt, HitVector &hitVector, HitToParticleMap &hitToParticleMap) const
{
    mf::LogDebug("LArPandora") << " *** LArPandora::CollectArtHits(...) *** " << std::endl;

    art::Handle< std::vector<recob::Hit> > hitHandle;
    evt.getByLabel(m_hitfinderModuleLabel, hitHandle);

    mf::LogDebug("LArPandora") << "   Number of ART hits: " << hitHandle->size() << std::endl;

    art::ServiceHandle<cheat::BackTracker> theBackTracker; 

    for (unsigned int iHit = 0, iHitEnd = hitHandle->size(); iHit < iHitEnd; ++iHit)
    {
        art::Ptr<recob::Hit> hit(hitHandle, iHit);
        hitVector.push_back(hit);

        const std::vector<cheat::TrackIDE> trackCollection(theBackTracker->HitToTrackID(hit));

        for (unsigned int iTrack = 0, iTrackEnd = trackCollection.size(); iTrack < iTrackEnd; ++iTrack)
        {
            cheat::TrackIDE trackIDE = trackCollection.at(iTrack);
            hitToParticleMap[hit].push_back(trackIDE);
        }
    }
}

//------------------------------------------------------------------------------------------------------------------------------------------

void LArPandora::CollectArtParticles(const art::Event &evt, ParticleMap &particleMap, TruthToParticleMap &truthToParticleMap) const
{
    mf::LogDebug("LArPandora") << " *** LArPandora::CollectArtParticles(...) *** " << std::endl;

    art::Handle< std::vector<simb::MCParticle> > mcParticleHandle;
    evt.getByLabel(m_geantModuleLabel, mcParticleHandle);

    mf::LogDebug("LArPandora") << "   Number of ART particles: " << mcParticleHandle->size() << std::endl;

    art::ServiceHandle<cheat::BackTracker> theBackTracker; 

    for (unsigned int i = 0, iEnd = mcParticleHandle->size(); i < iEnd; ++i)
    {
        art::Ptr<simb::MCParticle> particle(mcParticleHandle, i);
        particleMap[particle->TrackId()] = particle;

        art::Ptr<simb::MCTruth> truth(theBackTracker->TrackIDToMCTruth(particle->TrackId()));
        truthToParticleMap[truth].push_back(particle->TrackId());
    }
}

//------------------------------------------------------------------------------------------------------------------------------------------

void LArPandora::CreatePandoraHits(const HitVector &hitVector, HitMap &hitMap) const
{
    mf::LogDebug("LArPandora") << " *** LArPandora::CreatePandoraHits(...) *** " << std::endl;

    static const double dx_cm(0.5);          // cm
    static const double int_cm(84.0);        // cm
    static const double rad_cm(14.0);        // cm
    static const double dEdX_max(25.0);      // MeV/cm
    static const double dEdX_mip(2.0);       // MeV/cm (for now)
    static const double mips_to_gev(3.5e-4); // from 100 single-electrons

    art::ServiceHandle<util::DetectorProperties> theDetector;
    static const double us_per_tdc(1.0e-3 * theDetector->SamplingRate()); // ns->us
    static const double tdc_offset(theDetector->TriggerOffset());

    art::ServiceHandle<geo::Geometry> theGeometry;
    art::ServiceHandle<util::LArProperties> theLiquidArgon;

    // Calculate offsets in coordinate system
    // (until we find the wireID->Upos and wireID->Vpos methods in LArSoft!)
    double y0(0.f); double z0(0.f);
    theGeometry->IntersectionPoint(0, 0, geo::kU,geo::kV, 0, 0, y0, z0);

    const double wire_pitch_cm(lar::LArGeometryHelper::GetLArPseudoLayerCalculator()->GetZPitch());
    const double u0(lar::LArGeometryHelper::GetLArTransformationCalculator()->YZtoU(y0, z0));
    const double v0(lar::LArGeometryHelper::GetLArTransformationCalculator()->YZtoV(y0, z0));
    const double w0(wire_pitch_cm);
    const double x0(0.f); // if necessary, make the hits line up with the MC particles

    // Loop over hits
    int hitCounter(0);

    for (HitVector::const_iterator iter = hitVector.begin(), iterEnd = hitVector.end(); iter != iterEnd; ++iter)
    {
        const art::Ptr<const recob::Hit> hit = *iter;

        const int hit_View(hit->View());
        const double hit_Time(hit->PeakTime());
        const double hit_Charge(hit->Charge(true));
        const geo::WireID hit_WireID(hit->WireID());

        const double wpos_cm(hit_WireID.Wire * wire_pitch_cm);
        const double xpos_cm(theDetector->ConvertTicksToX(hit_Time, hit_WireID.Plane, hit_WireID.TPC, hit_WireID.Cryostat));

        const double t_us((hit_Time - tdc_offset) * us_per_tdc);
        const double dQdX(hit_Charge / wire_pitch_cm); // ADC/cm
        const double dQdX_e(dQdX / (theDetector->ElectronsToADC() * exp(-t_us / theLiquidArgon->ElectronLifetime()))); // e/cm

        double dEdX(theLiquidArgon->BirksCorrection(dQdX_e));

        if ((dEdX < 0) || (dEdX > dEdX_max))
            dEdX = dEdX_max;

        const double mips(dEdX / dEdX_mip); // TODO: Check if calibration procedure is correct

        hitMap[++hitCounter] = hit;

        // Create Pandora CaloHit
        PandoraApi::CaloHit::Parameters caloHitParameters;
        caloHitParameters.m_expectedDirection = pandora::CartesianVector(0., 0., 1.);
        caloHitParameters.m_cellNormalVector = pandora::CartesianVector(0., 0., 1.);
        caloHitParameters.m_cellSizeU = dx_cm;
        caloHitParameters.m_cellSizeV = dx_cm;
        caloHitParameters.m_cellThickness = wire_pitch_cm;
        caloHitParameters.m_time = 0.;
        caloHitParameters.m_nCellRadiationLengths = dx_cm / rad_cm;
        caloHitParameters.m_nCellInteractionLengths = dx_cm / int_cm;
        caloHitParameters.m_isDigital = false;
        caloHitParameters.m_detectorRegion = pandora::ENDCAP;
        caloHitParameters.m_layer = 0;
        caloHitParameters.m_isInOuterSamplingLayer = false;
        caloHitParameters.m_inputEnergy = hit_Charge;
        caloHitParameters.m_mipEquivalentEnergy = mips;
        caloHitParameters.m_electromagneticEnergy = mips * mips_to_gev; 
        caloHitParameters.m_hadronicEnergy = mips * mips_to_gev; 
        caloHitParameters.m_pParentAddress = (void*)((intptr_t)hitCounter); 

        if (hit_View == geo::kW)
        {
            caloHitParameters.m_hitType = pandora::VIEW_W;
            caloHitParameters.m_positionVector = pandora::CartesianVector(xpos_cm + x0, 0., wpos_cm + w0);
        }
        else if(hit_View == geo::kU)
        {
            caloHitParameters.m_hitType = pandora::VIEW_U;
            caloHitParameters.m_positionVector = pandora::CartesianVector(xpos_cm + x0, 0., wpos_cm + u0);
        }
        else if(hit_View == geo::kV)
        {
            caloHitParameters.m_hitType = pandora::VIEW_V;
            caloHitParameters.m_positionVector = pandora::CartesianVector(xpos_cm + x0, 0., wpos_cm + v0);
        }
        else
        {
            mf::LogError("LArPandora") << " --- WARNING: UNKNOWN VIEW !!!  (View=" << hit_View << ")" << std::endl;
            throw pandora::StatusCodeException(pandora::STATUS_CODE_FAILURE);
        }

        if (std::isnan(mips))
        {
            mf::LogError("LArPandora") << " --- WARNING: UNPHYSICAL PULSEHEIGHT !!! (MIPs=" << mips << ")" << std::endl;
            throw pandora::StatusCodeException(pandora::STATUS_CODE_FAILURE);
        }

        PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=, PandoraApi::CaloHit::Create(*m_pPandora, caloHitParameters)); 
    }

    mf::LogDebug("LArPandora") << "   Number of Pandora hits: " << hitCounter << std::endl;
}

//------------------------------------------------------------------------------------------------------------------------------------------

void LArPandora::CreatePandoraParticles(const ParticleMap& particleMap, const TruthToParticleMap &truthToParticleMap) const
{
    mf::LogDebug("LArPandora") << " *** LArPandora::CreatePandoraParticles(...) *** " << std::endl;
    static const int id_offset = 100000000;

    // Vector for neutrino-induced particles
    ParticleVector particleVector;

    // Loop over incident particles
    int neutrinoCounter(0);

    for (TruthToParticleMap::const_iterator iter = truthToParticleMap.begin(), iterEnd = truthToParticleMap.end(); iter != iterEnd; ++iter)
    {
        const art::Ptr<const simb::MCTruth> truth = iter->first;

        if (truth->NeutrinoSet())
        {
            const simb::MCNeutrino neutrino(truth->GetNeutrino());
            ++neutrinoCounter;

            if (neutrinoCounter > id_offset)
                throw pandora::StatusCodeException(pandora::STATUS_CODE_FAILURE);

            const int neutrinoID(neutrinoCounter + 4 * id_offset);

            // Create Pandora 3D MC Particle
            PandoraApi::MCParticle::Parameters mcParticleParameters;
            mcParticleParameters.m_energy = neutrino.Nu().E();
            mcParticleParameters.m_momentum = pandora::CartesianVector(neutrino.Nu().Px(), neutrino.Nu().Py(), neutrino.Nu().Pz());
            mcParticleParameters.m_vertex = pandora::CartesianVector(neutrino.Nu().Vx(), neutrino.Nu().Vy(), neutrino.Nu().Vz());
            mcParticleParameters.m_endpoint = pandora::CartesianVector(neutrino.Nu().Vx(), neutrino.Nu().Vy(), neutrino.Nu().Vz());
            mcParticleParameters.m_particleId = neutrino.Nu().PdgCode();
            mcParticleParameters.m_mcParticleType = pandora::MC_STANDARD;
            mcParticleParameters.m_pParentAddress = (void*)((intptr_t)neutrinoID);
            PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=, PandoraApi::MCParticle::Create(*m_pPandora, mcParticleParameters));

            // Loop over associated particles
            const std::vector<int> particleCollection = iter->second;

            for (unsigned int j = 0; j < particleCollection.size(); ++j)
            {
                const int trackID = particleCollection.at(j);

                ParticleMap::const_iterator iterJ = particleMap.find(trackID);

                if( iterJ == particleMap.end() )
                    throw pandora::StatusCodeException(pandora::STATUS_CODE_FAILURE);

                const art::Ptr<const simb::MCParticle> particle = iterJ->second;

                // Mother/Daughter Links
                if (particle->Mother() == 0) 
                    PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=, PandoraApi::SetMCParentDaughterRelationship(*m_pPandora,
                        (void*)((intptr_t)neutrinoID), (void*)((intptr_t)trackID)));

                // Select these particles (TODO: deal with cosmic-ray particles)
                particleVector.push_back(particle);
            }
        }
    }

    mf::LogDebug("LArPandora") << "   Number of Pandora neutrinos: " << neutrinoCounter << std::endl;


    // Loop over G4 particles
    int particleCounter(0);
    
    for (ParticleMap::const_iterator iterI = particleMap.begin(), iterEndI = particleMap.end(); iterI != iterEndI; ++iterI )
    {
        const art::Ptr<const simb::MCParticle> particle = iterI->second;

        if (particle->TrackId() != iterI->first)
            throw pandora::StatusCodeException(pandora::STATUS_CODE_FAILURE);

        if (particle->TrackId() > id_offset)
            throw pandora::StatusCodeException(pandora::STATUS_CODE_FAILURE);

        ++particleCounter;

        // Create Pandora MC Particle
        PandoraApi::MCParticle::Parameters mcParticleParameters;
        mcParticleParameters.m_energy = particle->E();
        mcParticleParameters.m_momentum = pandora::CartesianVector(particle->Px(), particle->Py(), particle->Pz());
        mcParticleParameters.m_vertex = pandora::CartesianVector(particle->Vx(), particle->Vy(), particle->Vz());
        mcParticleParameters.m_endpoint = pandora::CartesianVector(particle->EndX(), particle->EndY(), particle->EndZ());
        mcParticleParameters.m_particleId = particle->PdgCode();
        mcParticleParameters.m_mcParticleType = pandora::MC_STANDARD;
        mcParticleParameters.m_pParentAddress = (void*)((intptr_t)particle->TrackId());
        PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=, PandoraApi::MCParticle::Create(*m_pPandora, mcParticleParameters));

        // Mother/Daughter Links
        const int id_mother(particle->Mother());
        ParticleMap::const_iterator iterJ = particleMap.find(id_mother);

        if (iterJ != particleMap.end())
            PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=, PandoraApi::SetMCParentDaughterRelationship(*m_pPandora,
                (void*)((intptr_t)id_mother), (void*)((intptr_t)particle->TrackId())));

    }


    // Create 2D projections for Pandora event display 
    for( unsigned int i =0; i < particleVector.size(); ++i )
    {
        const art::Ptr<const simb::MCParticle> particle = particleVector.at(i);

        const float dx(particle->EndX() - particle->Vx());
        const float dy(particle->EndY() - particle->Vy());
        const float dz(particle->EndZ() - particle->Vz());
        const float dw(lar::LArGeometryHelper::GetLArPseudoLayerCalculator()->GetZPitch());

        if (dx * dx + dy * dy + dz * dz < 0.5 * dw * dw)
            continue;

        if (particle->TrackId() > id_offset)
            throw pandora::StatusCodeException(pandora::STATUS_CODE_FAILURE);

        // Common parameters
        PandoraApi::MCParticle::Parameters mcParticleParameters;
        mcParticleParameters.m_energy = particle->E();
        mcParticleParameters.m_particleId = particle->PdgCode();

        // Create U projection
        mcParticleParameters.m_momentum = pandora::CartesianVector(particle->Px(), 0.f, 
            lar::LArGeometryHelper::GetLArTransformationCalculator()->PYPZtoPU(particle->Py(), particle->Pz()));
        mcParticleParameters.m_vertex = pandora::CartesianVector(particle->Vx(),  0.f, 
            lar::LArGeometryHelper::GetLArTransformationCalculator()->YZtoU(particle->Vy(), particle->Vz()));
        mcParticleParameters.m_endpoint = pandora::CartesianVector(particle->EndX(),  0.f, 
            lar::LArGeometryHelper::GetLArTransformationCalculator()->YZtoU(particle->EndY(), particle->EndZ()));
        mcParticleParameters.m_mcParticleType = pandora::MC_VIEW_U;
        mcParticleParameters.m_pParentAddress = (void*)((intptr_t)(particle->TrackId() + 1 * id_offset)); 
        PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=, PandoraApi::MCParticle::Create(*m_pPandora, mcParticleParameters));

        // Create V projection
        mcParticleParameters.m_momentum = pandora::CartesianVector(particle->Px(), 0.f, 
            lar::LArGeometryHelper::GetLArTransformationCalculator()->PYPZtoPV(particle->Py(), particle->Pz()));
        mcParticleParameters.m_vertex = pandora::CartesianVector(particle->Vx(),  0.f, 
            lar::LArGeometryHelper::GetLArTransformationCalculator()->YZtoV(particle->Vy(), particle->Vz()));
        mcParticleParameters.m_endpoint = pandora::CartesianVector(particle->EndX(),  0.f, 
            lar::LArGeometryHelper::GetLArTransformationCalculator()->YZtoV(particle->EndY(), particle->EndZ()));
        mcParticleParameters.m_mcParticleType = pandora::MC_VIEW_V;
        mcParticleParameters.m_pParentAddress = (void*)((intptr_t)(particle->TrackId() + 2 * id_offset)); 
        PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=, PandoraApi::MCParticle::Create(*m_pPandora, mcParticleParameters));

        // Create W projection
        mcParticleParameters.m_momentum = pandora::CartesianVector(particle->Px(), 0.f, particle->Pz());
        mcParticleParameters.m_vertex = pandora::CartesianVector(particle->Vx(),  0.f, particle->Vz());
        mcParticleParameters.m_endpoint = pandora::CartesianVector(particle->EndX(),  0.f, particle->EndZ());
        mcParticleParameters.m_mcParticleType = pandora::MC_VIEW_W;
        mcParticleParameters.m_pParentAddress = (void*)((intptr_t)(particle->TrackId() + 3 * id_offset)); 
        PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=, PandoraApi::MCParticle::Create(*m_pPandora, mcParticleParameters));
    }

    mf::LogDebug("LArPandora") << "   Number of Pandora particles: " << particleCounter << std::endl;
}

//------------------------------------------------------------------------------------------------------------------------------------------

void LArPandora::CreatePandoraLinks(const HitMap &hitMap, const HitToParticleMap &hitToParticleMap) const
{
    mf::LogDebug("LArPandora") << " *** LArPandora::CreatePandoraLinks(...) *** " << std::endl;

    for (HitMap::const_iterator iterI = hitMap.begin(), iterEndI = hitMap.end(); iterI != iterEndI ; ++iterI)
    {
        const int hitID(iterI->first);
        const art::Ptr<recob::Hit> hit(iterI->second);

        HitToParticleMap::const_iterator iterJ = hitToParticleMap.find(hit);

        if (iterJ == hitToParticleMap.end())
            continue;

        std::vector<cheat::TrackIDE> trackCollection = iterJ->second;

        if (trackCollection.size() == 0)
            throw pandora::StatusCodeException(pandora::STATUS_CODE_FAILURE);

        for (unsigned int k = 0; k < trackCollection.size(); ++k)
        {
            const cheat::TrackIDE trackIDE(trackCollection.at(k));
            const int trackID(std::abs(trackIDE.trackID)); // TODO: Find out why std::abs is needed
            const float energyFrac(trackIDE.energyFrac);

            PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=, PandoraApi::SetCaloHitToMCParticleRelationship(*m_pPandora,
                (void*)((intptr_t)hitID), (void*)((intptr_t)trackID), energyFrac));
        }
    }
}

//------------------------------------------------------------------------------------------------------------------------------------------

void LArPandora::RunPandora() const
{
    mf::LogDebug("LArPandora") << " *** LArPandora::RunPandora() *** " << std::endl;
 
    PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=, PandoraApi::ProcessEvent(*m_pPandora));
}

//------------------------------------------------------------------------------------------------------------------------------------------

void LArPandora::ResetPandora() const
{
    mf::LogDebug("LArPandora") << " *** LArPandora::ResetPandora() *** " << std::endl;

    PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=, PandoraApi::Reset(*m_pPandora));
}

//------------------------------------------------------------------------------------------------------------------------------------------

void LArPandora::ProduceArtClusters(art::Event &evt, const HitMap &hitMap) const
{
    mf::LogDebug("LArPandora") << " *** LArPandora::ProduceArtClusters() *** " << std::endl;

    const pandora::PfoList *pPfoList = NULL;
    PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=, PandoraApi::GetCurrentPfoList(*m_pPandora, pPfoList));

    std::unique_ptr< std::vector<recob::Cluster> > artClusterVector( new std::vector<recob::Cluster> );
    std::unique_ptr< art::Assns<recob::Cluster, recob::Hit> > artClusterAssociations( new art::Assns<recob::Cluster, recob::Hit> );

    for (pandora::PfoList::iterator iter = pPfoList->begin(), iterEnd = pPfoList->end(); iter != iterEnd; ++iter)
    {
        const pandora::ParticleFlowObject *const pPfo = *iter;
        const pandora::ClusterAddressList clusterAddressList(pPfo->GetClusterAddressList()); 

        HitVector artClusterHits;

        for (pandora::ClusterAddressList::const_iterator cIter = clusterAddressList.begin(), cIterEnd = clusterAddressList.end();
            cIter != cIterEnd; ++cIter)
        {
            const pandora::CaloHitAddressList &caloHitAddressList(*cIter);

            for (pandora::CaloHitAddressList::const_iterator hIter = caloHitAddressList.begin(), hIterEnd = caloHitAddressList.end();
                hIter != hIterEnd; ++hIter)
            {
                const void *pHitAddress(*hIter);
                const intptr_t hitID_temp((intptr_t)(pHitAddress)); // TODO
                const int hitID((int)(hitID_temp));

                HitMap::const_iterator artIter = hitMap.find(hitID);

                if (artIter == hitMap.end())
                    throw pandora::StatusCodeException(pandora::STATUS_CODE_FAILURE);

                art::Ptr<recob::Hit> hit = artIter->second;
                artClusterHits.push_back(hit);
            }
        }

        if (!artClusterHits.empty())
        {
            recob::Cluster artCluster;
            artClusterVector->push_back(artCluster);
            util::CreateAssn(*this, evt, *(artClusterVector.get()), artClusterHits, *(artClusterAssociations.get()));
        }
    }

    evt.put(std::move(artClusterVector));
    evt.put(std::move(artClusterAssociations));
}

} // namespace lar_pandora
