#########################################################################
#  Project:		Kooka						#
#  Edit:		14-Dec-18					#
#########################################################################

project(kooka5)

#########################################################################
#									#
#  OCR plugin for OCRAD							#
#									#
#########################################################################

set(kookaocrocrad_SRCS
  ocrocradengine.cpp
  ocrocraddialog.cpp
)

add_library(kookaocrocrad MODULE ${kookaocrocrad_SRCS})
kcoreaddons_desktop_to_json(kookaocrocrad kookaocr-ocrad.desktop)

target_link_libraries(kookaocrocrad Qt5::Core Qt5::Gui)
target_link_libraries(kookaocrocrad KF5::I18n KF5::WidgetsAddons)
target_link_libraries(kookaocrocrad kookaocr kookascan)

if (INSTALL_BINARIES)
  install(TARGETS kookaocrocrad DESTINATION ${PLUGIN_INSTALL_DIR}/kooka)
endif (INSTALL_BINARIES)
install(FILES kookaocr-ocrad.desktop DESTINATION ${SERVICES_INSTALL_DIR})
