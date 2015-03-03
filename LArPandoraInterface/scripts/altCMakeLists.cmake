set(LArPandoraInterface_scripts
     PandoraSettings_MicroBooNE_Cosmic.xml
     PandoraSettings_MicroBooNE_Neutrino.xml
     PandoraSettings_Write.xml
     )

install(FILES ${LArPandoraInterface_scripts} DESTINATION scripts COMPONENT Runtime)
