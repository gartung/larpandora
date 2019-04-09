include_directories( ${PandoraSDK_INCLUDE_DIRS} )
include_directories( ${PandoraMonitoring_INCLUDE_DIRS} )
include_directories( ${EIGEN3_INCLUDE_DIRS} )
include_directories( ${LArPandoraContent_INCLUDE_DIRS} )

art_make( 
          LIB_LIBRARIES larcorealg_Geometry
                        larcore_Geometry_Geometry_service
                        larsim_Simulation lardataobj_Simulation
                        lardataobj_RawData
                        lardataobj_RecoBase
                        lardataobj_AnalysisBase
                        lardata_ArtDataHelper
                        lardata_Utilities
                        larreco_RecoAlg_ClusterRecoUtil
                        larreco_RecoAlg
                        larreco_ClusterFinder
                        larreco_Calorimetry
                        ${PandoraSDK_LIBRARIES}
                        ${PandoraMonitoring_LIBRARIES}
                        LArPandoraContent
                        larpandora_LArPandoraInterface
                        nusimdata_SimulationBase
                        ${ART_FRAMEWORK_CORE}
                        ${ART_FRAMEWORK_PRINCIPAL}
                        ${ART_FRAMEWORK_SERVICES_REGISTRY}
                        ${ART_FRAMEWORK_SERVICES_BASIC}
                        ${ART_ROOT_IO_TFILE_SUPPORT} ${ROOT_CORE}
                        ${ART_ROOT_IO_TFILESERVICE_SERVICE}
                        art_Persistency_Common
                        art_Persistency_Provenance
                        art_Utilities
                        canvas
                        ${MF_MESSAGELOGGER}
                        ${FHICLCPP}
                        cetlib cetlib_except
                        ${Boost_SYSTEM_LIBRARY}
                        ${Boost_FILESYSTEM_LIBRARY}
                        ${ROOT_Geom_Library}
                        ${ROOT_BASIC_LIB_LIST}
                        ${ROOT_GenVector_LIBRARY}
                        MODULE_LIBRARIES larpandora_LArPandoraEventBuilding
          )

      simple_plugin(SimpleNeutrinoId "tool" larpandora_LArPandoraEventBuilding)

install_headers()
install_fhicl()
install_source()
