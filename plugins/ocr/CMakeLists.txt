#########################################################################
#  Project:		Kooka						#
#  Edit:		14-Dec-18					#
#########################################################################

project(kooka5)

#########################################################################
#									#
#  Options								#
#									#
#########################################################################

option(WITH_KADMOS "Enable the Kadmos OCR engine" false)

#########################################################################
#									#
#  Additional dependencies for OCR					#
#									#
#########################################################################

find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS I18n ConfigWidgets TextWidgets IconThemes Sonnet WidgetsAddons)

#########################################################################
#									#
#  OCR plugin base library						#
#									#
#########################################################################

set(kookaocr_SRCS
  abstractocrengine.cpp
  abstractocrdialogue.cpp
  executablepathdialogue.cpp
)
add_library(kookaocr SHARED ${kookaocr_SRCS})
set_target_properties(kookaocr PROPERTIES SOVERSION "1.0.0")
target_link_libraries(kookaocr Qt5::Core Qt5::Widgets)
target_link_libraries(kookaocr KF5::I18n KF5::WidgetsAddons KF5::ConfigWidgets KF5::SonnetCore KF5::SonnetUi)
target_link_libraries(kookaocr kookascan kookacore)
target_link_libraries(kookaocr dialogutil)

if (INSTALL_BINARIES)
  install(TARGETS kookaocr ${INSTALL_TARGETS_DEFAULT_ARGS})
endif (INSTALL_BINARIES)
install(FILES kookaocrplugin.desktop DESTINATION ${SERVICETYPES_INSTALL_DIR})

#########################################################################
#									#
#  Subdirectories							#
#									#
#########################################################################

add_subdirectory(gocr)
add_subdirectory(ocrad)
if(WITH_KADMOS)
  add_subdirectory(kadmos)
endif(WITH_KADMOS)
