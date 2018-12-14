#########################################################################
#  Project:		Kooka						#
#  Edit:		14-Dec-18					#
#########################################################################

project(kooka)

#########################################################################
#									#
#  OCR plugin for GOCR							#
#									#
#########################################################################

set(kookaocrgocr_SRCS
  ocrgocrengine.cpp
  ocrgocrdialog.cpp
)

add_library(kookaocrgocr MODULE ${kookaocrgocr_SRCS})
kcoreaddons_desktop_to_json(kookaocrgocr kookaocr-gocr.desktop)

target_link_libraries(kookaocrgocr Qt5::Core Qt5::Gui)
target_link_libraries(kookaocrgocr KF5::I18n KF5::WidgetsAddons)
target_link_libraries(kookaocrgocr kookaocr kookascan)

#install(TARGETS kookaocrgocr DESTINATION ${PLUGIN_INSTALL_DIR}/kooka)
install(FILES kookaocr-gocr.desktop DESTINATION ${SERVICES_INSTALL_DIR})
