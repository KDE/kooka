#########################################################################
#  Project:		Kooka						#
#  Edit:		09-Dec-18					#
#########################################################################

project(kooka)

#########################################################################
#									#
#  Additional dependencies for OCR					#
#									#
#########################################################################

find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS Sonnet)

#########################################################################
#									#
#  Common definitions for plugins					#
#									#
#########################################################################

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/..)

# see https://gcc.gnu.org/wiki/Visibility
add_definitions("-DPLUGIN_EXPORT=__attribute__((visibility(\"default\")))")

#########################################################################
#									#
#  OCR plugin base library						#
#									#
#########################################################################

add_library(kookaocr SHARED abstractocrengine.cpp abstractocrdialogue.cpp)
set_target_properties(kookaocr PROPERTIES SOVERSION "1.0.0")
target_link_libraries(kookaocr Qt5::Core Qt5::Widgets Qt5::PrintSupport)
target_link_libraries(kookaocr KF5::I18n KF5::WidgetsAddons KF5::ConfigWidgets KF5::SonnetCore KF5::SonnetUi)
target_link_libraries(kookaocr kookascan kookacore)

##install(TARGETS kookaocr ${INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES kookaocrplugin.desktop DESTINATION ${SERVICETYPES_INSTALL_DIR})

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
target_link_libraries(kookaocrgocr KF5::I18n KF5::WidgetsAddons KF5::Service)
target_link_libraries(kookaocrgocr kookaocr kookascan)

#install(TARGETS kookaocrgocr DESTINATION ${PLUGIN_INSTALL_DIR}/kooka)
install(FILES kookaocr-gocr.desktop DESTINATION ${SERVICES_INSTALL_DIR})

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
target_link_libraries(kookaocrocrad KF5::I18n KF5::WidgetsAddons KF5::Service)
target_link_libraries(kookaocrocrad kookaocr kookascan)

#install(TARGETS kookaocrocrad DESTINATION ${PLUGIN_INSTALL_DIR}/kooka)
install(FILES kookaocr-ocrad.desktop DESTINATION ${SERVICES_INSTALL_DIR})
