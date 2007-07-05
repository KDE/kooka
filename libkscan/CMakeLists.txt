
add_subdirectory( pics ) 

include_directories( ${CMAKE_SOURCE_DIR}/libkscan ${SANE_INCLUDE_DIR}  )

add_definitions (-DQT3_SUPPORT -DQT3_SUPPORT_WARNINGS)

########### next target ###############

set(kscan_LIB_SRCS 
   kscandevice.cpp 
   kscanslider.cpp 
   kgammatable.cpp 
   kscanoption.cpp 
   kscanoptset.cpp 
   gammadialog.cpp 
   dispgamma.cpp 
   scansourcedialog.cpp 
   scanparams.cpp 
   massscandialog.cpp 
   devselector.cpp 
   scandialog.cpp 
   img_canvas.cpp 
   previewer.cpp 
   imgscaledialog.cpp 
   sizeindicator.cpp 
   imgscaninfo.cpp )

kde4_automoc(${kscan_LIB_SRCS})

kde4_add_library(kscan SHARED ${kscan_LIB_SRCS})

target_link_libraries(kscan   ${SANE_LIBRARIES} ${KDE4_KDE3SUPPORT_LIBS})

set_target_properties(kscan PROPERTIES VERSION 1.0.0 SOVERSION 1 )
install(TARGETS kscan  DESTINATION ${LIB_INSTALL_DIR} )


########### install files ###############

install( FILES scanservice.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

