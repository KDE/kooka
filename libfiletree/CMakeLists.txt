
project(libfiletree)

########### libfiletree ###############

add_definitions(-DKDE_DEFAULT_DEBUG_AREA=250)

set(libfiletree_SRCS
  filetreeview.cpp
  filetreebranch.cpp
  filetreeviewitem.cpp
)

add_library(libfiletree STATIC ${libfiletree_SRCS})
target_link_libraries(libfiletree
  Qt5::Widgets
  KF5::KIOCore
  KF5::KIOWidgets
)
