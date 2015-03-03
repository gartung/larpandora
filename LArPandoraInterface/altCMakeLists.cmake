include_directories( ${CMAKE_CURRENT_SOURCE_DIR} )

include_directories( ${PANDORA_INC} )
include_directories( ${PANDORA_FQ_DIR}/PandoraSDK/include )
include_directories( ${PANDORA_FQ_DIR}/PandoraMonitoring/include )

set(LArPandoraInterface_HEADERS
     LArPandoraBase.h
     LArPandoraCollector.h
     LArPandoraHelper.h
     LArPandoraParticleCreator.h
     LBNE35tGeometryHelper.h
     LBNE35tPseudoLayerPlugin.h
     LBNE35tTransformationPlugin.h
     LBNE4APAGeometryHelper.h
     LBNE4APAPseudoLayerPlugin.h
     LBNE4APATransformationPlugin.h
     MicroBooNEPseudoLayerPlugin.h
     MicroBooNETransformationPlugin.h
     PFParticleSeed.h
     PFParticleStitcher.h
     )

add_library(LArPandoraInterface SHARED
     ${LArPandoraInterface_HEADERS}
     LArPandoraBase.cxx
     LArPandoraCollector.cxx
     LArPandoraHelper.cxx
     LArPandoraParticleCreator.cxx
     LBNE35tGeometryHelper.cxx
     LBNE35tPseudoLayerPlugin.cxx
     LBNE35tTransformationPlugin.cxx
     LBNE4APAGeometryHelper.cxx
     LBNE4APAPseudoLayerPlugin.cxx
     LBNE4APATransformationPlugin.cxx
     MicroBooNEPseudoLayerPlugin.cxx
     MicroBooNETransformationPlugin.cxx
     PFParticleSeed.cxx
     PFParticleStitcher.cxx
     )

target_link_libraries(LArPandoraInterface
     art::art_Framework_Core
     art::art_Framework_Principal
     art::art_Persistency_Provenance
     art::art_Utilities
     art::art_Framework_Services_Registry
     FNALCore::FNALCore
     LArPandoraAlgorithms
     larsoft::Geometry
     larsoft::Simulation
     larsoft::RawData
     larsoft::RecoBase
     larsoft::Utilities
     ${PANDORASDK}
     ${PANDORAMONITORING}
     ${SIMULATIONBASE}
     ${ROOT_BASIC_LIB_LIST}
)

art_add_module(LBNE35tPandora_module LBNE35tPandora_module.cc)
art_add_module(LBNE35tParticleStitcher_module LBNE35tParticleStitcher_module.cc)
art_add_module(LBNE4APAPandora_module LBNE4APAPandora_module.cc)
art_add_module(MicroBooNEPandora_module MicroBooNEPandora_module.cc)
art_add_module(PFParticleAnalysis_module PFParticleAnalysis_module.cc)
art_add_module(PFParticleCosmicAna_module PFParticleCosmicAna_module.cc)
art_add_module(PFParticleHitDumper_module PFParticleHitDumper_module.cc)
art_add_module(PFParticleMonitoring_module PFParticleMonitoring_module.cc)
art_add_module(PFParticleTrackMaker_module PFParticleTrackMaker_module.cc)

install(TARGETS
     LArPandoraInterface
     MicroBooNEPandora_module
     EXPORT larpandoraLibraries
     RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
     LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
     ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
     COMPONENT Runtime
     )

install(FILES ${LArPandoraInterface_HEADERS} DESTINATION 
     ${CMAKE_INSTALL_INCLUDEDIR}/LArPandoraInterface COMPONENT Development)


file(GLOB FHICL_FILES 
     [^.]*.fcl
)

install(FILES ${FHICL_FILES} DESTINATION job COMPONENT Runtime)



add_subdirectory(scripts)

