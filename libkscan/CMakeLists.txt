
add_subdirectory( pics ) 

include_directories( ${CMAKE_SOURCE_DIR}/libkscan ${SANE_INCLUDE_DIR} ${BLITZ_INCLUDES} )

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


kde4_add_plugin(kscanplugin ${kscan_LIB_SRCS})

target_link_libraries(kscanplugin   ${SANE_LIBRARY} ${KDE4_KDE3SUPPORT_LIBS} ${KDE4_KDEUI_LIBS} ${BLITZ_LIBRARIES})

install(TARGETS kscanplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES scanservice.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

