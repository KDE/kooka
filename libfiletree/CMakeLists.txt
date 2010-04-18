
project(kooka4)

########### libfiletree ###############

add_definitions(-DKDE_DEFAULT_DEBUG_AREA=250)

set(libfiletree_SRCS
  filetreeview.cpp
  filetreebranch.cpp
  filetreeviewitem.cpp
)

kde4_add_library(libfiletree STATIC ${libfiletree_SRCS})
target_link_libraries(libfiletree
  ${KDE4_KIO_LIBS}
  ${KDE4_KDEUI_LIBS}
  ${KDE4_KDECORE_LIBS}
)

########### clobbering ###############

add_custom_target(clobber
  COMMENT "Clobber in ${CMAKE_CURRENT_BINARY_DIR}..."
  VERBATIM
  COMMAND sh -c "rm -f *.moc"
  COMMAND sh -c "rm -f *_automoc.cpp *_automoc.cpp.files"
  COMMAND sh -c "rm -r -f CMakeFiles *.cmake"
  COMMAND sh -c "rm -f config-*.h"
)

########### install files ###############
