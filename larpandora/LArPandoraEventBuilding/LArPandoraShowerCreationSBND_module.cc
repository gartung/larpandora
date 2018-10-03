/**
 *  @file   larpandora/LArPandoraEventBuilding/LArPandoraShowerCreationSBND_module.cc
 *
 *  @brief  module for lar pandora shower creation
 */

#include "art/Framework/Core/EDProducer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"

#include "fhiclcpp/ParameterSet.h"

#include "lardataobj/RecoBase/Shower.h"
#include "lardataobj/RecoBase/PCAxis.h"
#include "lardataobj/RecoBase/SpacePoint.h"
#include "lardataobj/RecoBase/Hit.h"

#include "larreco/Calorimetry/LinearEnergyAlg.h"

#include "larpandoracontent/LArObjects/LArPfoObjects.h"

#include <memory>

#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Principal/SubRun.h"


#include "canvas/Utilities/InputTag.h"

#include "larcore/Geometry/Geometry.h"

#include "lardata/Utilities/AssociationUtil.h"

#include "lardataobj/RecoBase/PCAxis.h"
#include "lardataobj/RecoBase/PFParticle.h"
#include "lardataobj/RecoBase/Shower.h"
#include "lardataobj/RecoBase/SpacePoint.h"
#include "lardataobj/RecoBase/Vertex.h"

#include "messagefacility/MessageLogger/MessageLogger.h"

#include "larpandoracontent/LArHelpers/LArPfoHelper.h"

#include "larpandora/LArPandoraInterface/LArPandoraHelper.h"

#include "larreco/RecoAlg/EMShowerAlg.h"
#include "larreco/RecoAlg/ShowerEnergyAlg.h"
#include "larreco/Calorimetry/CalorimetryAlg.h"

#include <iostream>


namespace lar_pandora
{

class LArPandoraShowerCreationSBND : public art::EDProducer
{
public:
    explicit LArPandoraShowerCreationSBND(fhicl::ParameterSet const &pset);

    LArPandoraShowerCreationSBND(LArPandoraShowerCreationSBND const &) = delete;
    LArPandoraShowerCreationSBND(LArPandoraShowerCreationSBND &&) = delete;
    LArPandoraShowerCreationSBND & operator = (LArPandoraShowerCreationSBND const &) = delete;
    LArPandoraShowerCreationSBND & operator = (LArPandoraShowerCreationSBND &&) = delete;

    void produce(art::Event &evt) override;

private:
    /**
     *  @brief  Build a recob::Shower object
     *
     *  @param  larShowerPCA the lar shower pca parameters extracted from pandora
     *  @param  vertexPosition the shower vertex position
     */
  recob::Shower BuildShower(const lar_content::LArShowerPCA &larShowerPCA, const pandora::CartesianVector &vertexPosition, HitVector &hits,const art::Ptr<recob::Vertex>& larvertex ) ;

    /**
     *  @brief  Build a recob::PCAxis object
     *
     *  @param  larShowerPCA the lar shower pca parameters extracted from pandora
     */
    recob::PCAxis BuildPCAxis(const lar_content::LArShowerPCA &larShowerPCA) const;

    std::string     m_pfParticleLabel;              ///< The pf particle label
    bool            m_useAllParticles;              ///< Build a recob::Track for every recob::PFParticle

    shower::ShowerEnergyAlg fShowerEnergyAlg;
    calo::CalorimetryAlg fCalorimetryAlg;
    shower::EMShowerAlg fEMShowerAlg;

    // TODO When implementation lived in LArPandoraOutput, it contained key building blocks for calculation of shower energies per plane.
    // Now functionality has moved to separate module, will require reimplementation (was deeply embedded in LArPandoraOutput structure).
    // const calo::LinearEnergyAlg    *m_pShowerEnergyAlg;       ///< The address of the shower energy algorithm
};

DEFINE_ART_MODULE(LArPandoraShowerCreationSBND)

} // namespace lar_pandora

//------------------------------------------------------------------------------------------------------------------------------------------
// implementation follows

#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Principal/SubRun.h"

#include "art/Persistency/Common/PtrMaker.h"

#include "canvas/Utilities/InputTag.h"
#include "canvas/Persistency/Common/PtrVector.h"

#include "larcore/Geometry/Geometry.h"

#include "lardata/Utilities/AssociationUtil.h"

#include "lardataobj/RecoBase/PCAxis.h"
#include "lardataobj/RecoBase/PFParticle.h"
#include "lardataobj/RecoBase/Shower.h"
#include "lardataobj/RecoBase/SpacePoint.h"
#include "lardataobj/RecoBase/Vertex.h"
#include "lardataobj/RecoBase/Track.h"

#include "lardata/ArtDataHelper/TrackUtils.h"

#include "messagefacility/MessageLogger/MessageLogger.h"

#include "larpandoracontent/LArHelpers/LArPfoHelper.h"

#include "larpandora/LArPandoraInterface/LArPandoraHelper.h"

#include "larreco/RecoAlg/EMShowerAlg.h"
#include "larreco/RecoAlg/ShowerEnergyAlg.h"

#include <iostream>

namespace lar_pandora
{
  
  LArPandoraShowerCreationSBND::LArPandoraShowerCreationSBND(fhicl::ParameterSet const &pset) :
  m_pfParticleLabel(pset.get<std::string>("PFParticleLabel")),
  m_useAllParticles(pset.get<bool>("UseAllParticles", false)),
  fShowerEnergyAlg(pset.get<fhicl::ParameterSet>("ShowerEnergyAlg")),
  fCalorimetryAlg(pset.get<fhicl::ParameterSet>("CalorimetryAlg")),
  fEMShowerAlg(pset.get<fhicl::ParameterSet>("EMShowerAlg"))
  {
    produces< std::vector<recob::Shower> >();
    produces< std::vector<recob::PCAxis> >();
    produces< art::Assns<recob::PFParticle, recob::Shower> >();
    produces< art::Assns<recob::PFParticle, recob::PCAxis> >();
    produces< art::Assns<recob::Shower, recob::Hit> >();
    produces< art::Assns<recob::Shower, recob::PCAxis> >();
}

//------------------------------------------------------------------------------------------------------------------------------------------

void LArPandoraShowerCreationSBND::produce(art::Event &evt)
{
    std::unique_ptr< std::vector<recob::Shower> > outputShowers( new std::vector<recob::Shower> );
    std::unique_ptr< std::vector<recob::PCAxis> > outputPCAxes( new std::vector<recob::PCAxis> );
    std::unique_ptr< art::Assns<recob::PFParticle, recob::Shower> > outputParticlesToShowers( new art::Assns<recob::PFParticle, recob::Shower> );
    std::unique_ptr< art::Assns<recob::PFParticle, recob::PCAxis> > outputParticlesToPCAxes( new art::Assns<recob::PFParticle, recob::PCAxis> );
    std::unique_ptr< art::Assns<recob::Shower, recob::Hit> > outputShowersToHits( new art::Assns<recob::Shower, recob::Hit> );
    std::unique_ptr< art::Assns<recob::Shower, recob::PCAxis> > outputShowersToPCAxes( new art::Assns<recob::Shower, recob::PCAxis> );

    const art::PtrMaker<recob::Shower> makeShowerPtr(evt, *this);
    const art::PtrMaker<recob::PCAxis> makePCAxisPtr(evt, *this);

    // Organise inputs
    PFParticleVector pfParticleVector;
    PFParticlesToSpacePoints pfParticlesToSpacePoints;
    LArPandoraHelper::CollectPFParticles(evt, m_pfParticleLabel, pfParticleVector, pfParticlesToSpacePoints);

    VertexVector vertexVector;
    PFParticlesToVertices pfParticlesToVertices;
    LArPandoraHelper::CollectVertices(evt, m_pfParticleLabel, vertexVector, pfParticlesToVertices);

    for (const art::Ptr<recob::PFParticle> pPFParticle : pfParticleVector)
    {

        // Select shower-like pfparticles
        if (!m_useAllParticles && !LArPandoraHelper::IsShower(pPFParticle))
            continue;

        // Obtain associated spacepoints
        PFParticlesToSpacePoints::const_iterator particleToSpacePointIter(pfParticlesToSpacePoints.find(pPFParticle));

        if (pfParticlesToSpacePoints.end() == particleToSpacePointIter)
        {
            mf::LogDebug("LArPandoraShowerCreationSBND") << "No spacepoints associated to particle ";
            continue;
        }

	HitVector hitsInParticle;
	LArPandoraHelper::GetAssociatedHits(evt, m_pfParticleLabel, particleToSpacePointIter->second, hitsInParticle);

        // Obtain associated vertex
        PFParticlesToVertices::const_iterator particleToVertexIter(pfParticlesToVertices.find(pPFParticle));

        if ((pfParticlesToVertices.end() == particleToVertexIter) || (1 != particleToVertexIter->second.size()))
        {
            mf::LogDebug("LArPandoraShowerCreationSBND") << "Unexpected number of vertices for particle ";
            continue;
        }

        // Copy information into expected pandora form
        pandora::CartesianPointVector cartesianPointVector;
        for (const art::Ptr<recob::SpacePoint> spacePoint : particleToSpacePointIter->second)
            cartesianPointVector.emplace_back(pandora::CartesianVector(spacePoint->XYZ()[0], spacePoint->XYZ()[1], spacePoint->XYZ()[2]));

        double vertexXYZ[3] = {0., 0., 0.};
        particleToVertexIter->second.front()->XYZ(vertexXYZ);
        const pandora::CartesianVector vertexPosition(vertexXYZ[0], vertexXYZ[1], vertexXYZ[2]);

        // Call pandora "fast" shower fitter
        try
        {
            // Ensure successful creation of all structures before placing results in output containers
            const lar_content::LArShowerPCA larShowerPCA(lar_content::LArPfoHelper::GetPrincipalComponents(cartesianPointVector, vertexPosition));
            recob::Shower shower(LArPandoraShowerCreationSBND::BuildShower(larShowerPCA, vertexPosition, hitsInParticle, particleToVertexIter->second.front()));
            const recob::PCAxis pcAxis(LArPandoraShowerCreationSBND::BuildPCAxis(larShowerPCA));
            outputShowers->emplace_back(shower);
            outputPCAxes->emplace_back(pcAxis);
        }
        catch (const pandora::StatusCodeException &)
        {
            mf::LogDebug("LArPandoraShowerCreationSBND") << "Unable to extract shower pca";
            continue;
        }

        // Output objects
        art::Ptr<recob::Shower> pShower(makeShowerPtr(outputShowers->size() - 1));
        art::Ptr<recob::PCAxis> pPCAxis(makePCAxisPtr(outputPCAxes->size() - 1));
   
        // Output associations, after output objects are in place
        util::CreateAssn(*this, evt, pShower, pPFParticle, *(outputParticlesToShowers.get()));
        util::CreateAssn(*this, evt, pPCAxis, pPFParticle, *(outputParticlesToPCAxes.get()));
        util::CreateAssn(*this, evt, *(outputShowers.get()), hitsInParticle, *(outputShowersToHits.get()));
        util::CreateAssn(*this, evt, pPCAxis, pShower, *(outputShowersToPCAxes.get()));
    }

    mf::LogDebug("LArPandora") << "   Number of new showers: " << outputShowers->size() << std::endl;
    mf::LogDebug("LArPandora") << "   Number of new pcaxes:  " << outputPCAxes->size() << std::endl;

    evt.put(std::move(outputShowers));
    evt.put(std::move(outputPCAxes));
    evt.put(std::move(outputParticlesToShowers));
    evt.put(std::move(outputParticlesToPCAxes));
    evt.put(std::move(outputShowersToHits));
    evt.put(std::move(outputShowersToPCAxes));
}

//------------------------------------------------------------------------------------------------------------------------------------------

  recob::Shower LArPandoraShowerCreationSBND::BuildShower(const lar_content::LArShowerPCA &larShowerPCA, const pandora::CartesianVector &vertexPosition, HitVector &hits, art::Ptr<recob::Vertex> const& larvertex ) 
{
    //Get the geometry                                                                                                                                                     
    art::ServiceHandle<geo::Geometry> fGeom;

    const pandora::CartesianVector &showerLength(larShowerPCA.GetAxisLengths());
    const pandora::CartesianVector &showerDirection(larShowerPCA.GetPrimaryAxis());

    const float length(showerLength.GetX());
    const float openingAngle(larShowerPCA.GetPrimaryLength() > 0.f ? std::atan(larShowerPCA.GetSecondaryLength() / larShowerPCA.GetPrimaryLength()) : 0.f);
    const TVector3 direction(showerDirection.GetX(), showerDirection.GetY(), showerDirection.GetZ());
    const TVector3 vertex(vertexPosition.GetX(), vertexPosition.GetY(), vertexPosition.GetZ());

    std::vector<double> dEdx;
    std::vector<double> totalEnergy;

    std::map< int, std::vector< art::Ptr< recob::Hit > > > initialTrackHits;
    std::map<int,std::vector<art::Ptr<recob::Hit> > > planeHitsMap;
    std::map<int,std::vector<art::Ptr<recob::Hit> > > showerHitsMap;

    std::unique_ptr<recob::Track> initialTrack = NULL;

    fEMShowerAlg.fDebug=0;
    
    art::PtrVector<recob::Hit> ShowerHitsVector;

    for (std::vector< art::Ptr<recob::Hit> >::const_iterator hit = hits.begin(); hit != hits.end(); ++hit){
      planeHitsMap[((*hit)->WireID()).Plane].push_back(*hit);
      ShowerHitsVector.push_back(*hit);
    }


    for (unsigned int plane = 0; plane < fGeom->MaxPlanes(); ++plane) {
      if (planeHitsMap.find(plane)!=planeHitsMap.end()){
	totalEnergy.push_back(fShowerEnergyAlg.ShowerEnergy(planeHitsMap[plane], plane));
	//	std::map<int,std::vector<art::Ptr<recob::Hit> > > showerHits = fEMShowerAlg.OrderShowerHits(ShowerHitsVector, plane);
	std::vector<art::Ptr<recob::Hit> > showerHits;
	fEMShowerAlg.OrderShowerHits(planeHitsMap[plane],showerHits ,larvertex); 
	fEMShowerAlg.FindInitialTrackHits(showerHits, larvertex, initialTrackHits[plane]);
	showerHitsMap[plane] = showerHits;
      }
      else{
	totalEnergy.push_back(0);
      }

    }


    //The plane doesn't seem to matter anymore just set as 0. 
    fEMShowerAlg.FindInitialTrack(showerHitsMap,initialTrack,initialTrackHits,0);
      for (std::map<int,std::vector<art::Ptr<recob::Hit > > >::iterator planeit = initialTrackHits.begin(); planeit !=initialTrackHits.end(); ++planeit){
	int trackhitsize = (planeit->second).size();
	if(initialTrack){
	if(planeHitsMap.find(planeit->first)!=planeHitsMap.end() || initialTrack==NULL ||trackhitsize<4){
	  // Get the pitch                                                                                                                                                     
	  double pitch = 0;
	  double totalCharge = 0, totalDistance = 0, avHitTime = 0;
	  unsigned int nHits = 0;
	  
	  try{initialTrack->Length();}
	  catch(...){std::cout << "Track failed to be born. Silly track." << std::endl;dEdx.push_back(0);continue;}
	  try { pitch = lar::util::TrackPitchInView(*initialTrack, (planeit->second).at(0)->View()); }
	  catch(...) { pitch = 0; }

	  for(std::vector<art::Ptr<recob::Hit > >::iterator trackHitIt = std::next(planeit->second.begin()); trackHitIt != planeit->second.end(); ++trackHitIt) {
	  totalDistance += pitch;
	  totalCharge += (*trackHitIt)->Integral();
	  avHitTime += (*trackHitIt)->PeakTime();
	  ++nHits;
	}

	  avHitTime /= (double)nHits;
	  
	  double dQdx = totalCharge / totalDistance;
	  double dEdxval = fCalorimetryAlg.dEdx_AREA(dQdx, avHitTime, planeit->second.at(0)->WireID().Plane);
	  dEdx.push_back(dEdxval);
	}
	else{
	  dEdx.push_back(0);
	}
	}
	else{
          dEdx.push_back(0);
        }
      }

    // TODO
    const TVector3 directionErr;
    const TVector3 vertexErr;
    const std::vector<double> totalEnergyErr;
    const std::vector<double> dEdxErr;
    const int bestplane(2);

    return recob::Shower(direction, directionErr, vertex, vertexErr, totalEnergy, totalEnergyErr, dEdx, dEdxErr, bestplane, util::kBogusI, length, openingAngle);
}

//------------------------------------------------------------------------------------------------------------------------------------------

recob::PCAxis LArPandoraShowerCreationSBND::BuildPCAxis(const lar_content::LArShowerPCA &larShowerPCA) const
{
    const pandora::CartesianVector &showerCentroid(larShowerPCA.GetCentroid());
    const pandora::CartesianVector &showerDirection(larShowerPCA.GetPrimaryAxis());
    const pandora::CartesianVector &showerSecondaryVector(larShowerPCA.GetSecondaryAxis());
    const pandora::CartesianVector &showerTertiaryVector(larShowerPCA.GetTertiaryAxis());
    const pandora::CartesianVector &showerEigenValues(larShowerPCA.GetEigenValues());

    const bool svdOK(true); ///< SVD Decomposition was successful
    const double eigenValues[3] = {showerEigenValues.GetX(), showerEigenValues.GetY(), showerEigenValues.GetZ()}; ///< Eigen values from SVD decomposition
    const double avePosition[3] = {showerCentroid.GetX(), showerCentroid.GetY(), showerCentroid.GetZ()}; ///< Average position of hits fed to PCA

    std::vector< std::vector<double> > eigenVecs = { /// The three principle axes
        { showerDirection.GetX(), showerDirection.GetY(), showerDirection.GetZ() },
        { showerSecondaryVector.GetX(), showerSecondaryVector.GetY(), showerSecondaryVector.GetZ() },
        { showerTertiaryVector.GetX(), showerTertiaryVector.GetY(), showerTertiaryVector.GetZ() }
    };

    // TODO
    const int numHitsUsed(100); ///< Number of hits in the decomposition, not yet ready
    const double aveHitDoca(0.); ///< Average doca of hits used in PCA, not ready yet
    const size_t iD(util::kBogusI); ///< Axis ID, not ready yet

    return recob::PCAxis(svdOK, numHitsUsed, eigenValues, eigenVecs, avePosition, aveHitDoca, iD);
}

} // namespace lar_pandora
