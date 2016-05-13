
project(libdialogutil)

########### libdialogutil ###############

set(dialogutil_SRCS
  dialogbase.cpp
  dialogstatesaver.cpp
  recentsaver.cpp
)

set(dialogutil_HDRS
  dialogbase.h
  dialogstatesaver.h
  recentsaver.h
  ${CMAKE_CURRENT_BINARY_DIR}/libdialogutil_export.h
)

add_library(dialogutil SHARED ${dialogutil_SRCS})
generate_export_header(dialogutil BASE_NAME libdialogutil)
target_link_libraries(dialogutil
  Qt5::Widgets
  KF5::ConfigCore
  KF5::WidgetsAddons
  KF5::KIOCore
  KF5::KIOFileWidgets
)

set_target_properties(dialogutil PROPERTIES VERSION 1.0.0 SOVERSION 1)

########### installation ###############

install(TARGETS dialogutil ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES ${dialogutil_HDRS} DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF5})
