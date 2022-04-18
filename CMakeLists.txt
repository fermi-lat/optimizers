##### Library ######
add_library(
  optimizers SHARED
  src/AbsEdge.cxx
  src/Amoeba.cxx
  src/BrokenPowerLaw.cxx
  src/ChiSq.cxx
  src/CompositeFunction.cxx
  src/dgaus8.c
  src/Dom.cxx
  src/dpptri-blas.c
  src/dpptri.c
  src/Drmnfb.cxx
  src/drmnfb_routines.c
  src/Drmngb.cxx
  src/drmngb_routines.c
  src/Function.cxx
  src/FunctionFactory.cxx
  src/FunctionTest.cxx
  src/Gaussian.cxx
  src/intrac.c
  src/Lbfgs.cxx
  src/lbfgs_routines.c
  src/Mcmc.cxx
  src/Minuit.cxx
  src/minuit_routines.c
  src/ModNewton.cxx
  src/MyFun.cxx
  src/NewMinuit.cxx
  src/Optimizer.cxx
  src/OptimizerFactory.cxx
  src/OptPP.cxx
  src/Parameter.cxx
  src/Powell.cxx
  src/PowerLaw.cxx
  src/ProductFunction.cxx
  src/Rosen.cxx
  src/RosenBounded.cxx
  src/RosenND.cxx
  src/StMnMinos.cxx
  src/SumFunction.cxx
  src/Util.cxx
)

target_link_libraries(
  optimizers
  PUBLIC PkgConfig::cfitsio CLHEP::CLHEP PkgConfig::f2c Minuit2::Minuit2 st_facilities xmlBase
)

target_include_directories(
  optimizers PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:>
)


###### Tests ######

add_executable(test_optimizers src/test/main.cxx)
target_link_libraries(test_optimizers PRIVATE optimizers)
target_include_directories(
  test_optimizers PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/src
  $<INSTALL_INTERFACE:>
)

if(${CMAKE_SYSTEM_NAME} MATCHES "Posix")
  target_compile_definitions(test_optimizers PRIVATE "TRAP_FPE")
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  target_compile_definitions(test_optimizers PRIVATE "DARWIN")
endif()
