##### Library ######
add_library(
  optimizers_c_objects OBJECT
  src/dgaus8.c src/dpptri-blas.c src/dpptri.c
  src/drmnfb_routines.c src/drmngb_routines.c
  src/intrac.c src/lbfgs_routines.c
  src/minuit_routines.c
)
target_include_directories(
  optimizers_c_objects
  PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> $<INSTALL_INTERFACE:>
)
target_link_libraries(optimizers_c_objects PRIVATE f2c::f2c)


add_library(
  optimizers STATIC
  src/AbsEdge.cxx src/Amoeba.cxx src/BrokenPowerLaw.cxx src/ChiSq.cxx
  src/CompositeFunction.cxx src/Dom.cxx src/Drmnfb.cxx src/Drmngb.cxx src/Function.cxx
  src/FunctionFactory.cxx src/FunctionTest.cxx src/Gaussian.cxx src/Lbfgs.cxx
  src/Mcmc.cxx src/Minuit.cxx src/ModNewton.cxx src/MyFun.cxx src/NewMinuit.cxx
  src/Optimizer.cxx src/OptimizerFactory.cxx src/OptPP.cxx src/Parameter.cxx
  src/Powell.cxx src/PowerLaw.cxx src/ProductFunction.cxx src/Rosen.cxx
  src/RosenBounded.cxx src/RosenND.cxx src/StMnMinos.cxx src/SumFunction.cxx
  src/Util.cxx
)

target_link_libraries(
  optimizers
  PUBLIC xmlBase PkgConfig::xerces-c Minuit2::Minuit2
  PRIVATE optimizers_c_objects PkgConfig::cfitsio CLHEP::CLHEP st_facilities
)

target_include_directories(
  optimizers PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:>
)

target_compile_definitions(optimizers PRIVATE BUILD_WITHOUT_ROOT)


###### Tests ######
add_executable(test_optimizers src/test/main.cxx)
target_link_libraries(test_optimizers PRIVATE optimizers st_facilities)
target_include_directories(
  test_optimizers PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/src
  $<INSTALL_INTERFACE:>
)

if(APPLE)
  target_compile_definitions(test_optimizers PRIVATE DARWIN DARWIN_F2C_FAILURE)
else()
  target_compile_definitions(test_optimizers PRIVATE TRAP_FPE)
endif()

###############################################################
# Installation
###############################################################
install(DIRECTORY optimizers DESTINATION ${FERMI_INSTALL_INCLUDEDIR})
install(DIRECTORY xml/ DESTINATION ${FERMI_INSTALL_XMLDIR}/optimizers)

install(
  TARGETS optimizers test_optimizers
  EXPORT fermiTargets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
