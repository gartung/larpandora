# ======================================================================
#  larpandora main build file
#
#  cd .../path/to/build/directory
#  source .../path/to/larpandora/ups/setup_for_development <-d|-p>
#  cmake [-DCMAKE_INSTALL_PREFIX=/install/path]
#        -DCMAKE_BUILD_TYPE=$CETPKG_TYPE
#        $CETPKG_SOURCE
#  make
#  make test
#  make install
#  make package (builds distribution tarfile)
# ======================================================================


# use cmake 3.11 or later
cmake_minimum_required (VERSION 3.11)

project(larpandora VERSION 08.00.00 LANGUAGES CXX)

set( fcl_dir job )
set( fw_dir scripts )
find_package( cetmodules )
include(CetCMakeEnv)
cet_cmake_env()

cet_set_compiler_flags(DIAGS CAUTIOUS
  WERROR
  NO_UNDEFINED
  EXTRA_FLAGS -pedantic -Wno-unused-local-typedefs
  )

cet_report_compiler_flags()

# these are minimum required versions, not the actual product versions
find_package( larreco REQUIRED )
find_package( LArPandoraContent REQUIRED )

# macros for dictionary and simple_plugin

include(ArtDictionary)
include(ArtMake)
include(BuildPlugins)

#cet_find_library( PANDORASDK NAMES PandoraSDK )
#cet_find_library( PANDORAMONITORING NAMES PandoraMonitoring )
#
## find larpandoracontent headers if building at the same time
##message(STATUS "larpandora: checking for MRB_SOURCE")
#set( mrb_source $ENV{MRB_SOURCE} )
#if( LARPANDORACONTENT_INC )
#  #message(STATUS "larpandora: found LARPANDORACONTENT_INC: $ENV{LARPANDORACONTENT_INC}")
#  # note that mrb v1_07_00 or later will define LARPANDORACONTENT_INC
#  if( mrb_source )
#    include_directories( ${LARPANDORACONTENT_INC} )
#  endif()
#else()
#  if( mrb_source )
#    include_directories( ${mrb_source}/larpandoracontent )
#  else()
#    message(FATAL_ERROR "LARPANDORACONTENT_INC is undefined, please setup larpandoracontent")
#  endif()
#endif()

# source
add_subdirectory(larpandora)
add_subdirectory(cmakeConfig)
# ups - table and config files
#add_subdirectory(ups)

# packaging utility
#include(UseCPack)

