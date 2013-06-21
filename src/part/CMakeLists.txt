include_directories( ../ ../lib ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR}/../lib ${Boost_INCLUDE_DIRS} ${graphviz_INCLUDE_DIRECTORIES} )

########### next target ###############

set( kgraphviewerpart_PART_SRCS kgraphviewer_part.cpp  )

kde4_add_plugin(kgraphviewerpart ${kgraphviewerpart_PART_SRCS})

add_definitions(-DQT_STL)
target_link_libraries(kgraphviewerpart ${KDE4_KPARTS_LIBS} gvc graph pathplan cdt kgraphviewerlib)

install( TARGETS kgraphviewerpart DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES kgraphviewer_part.rc DESTINATION ${DATA_INSTALL_DIR}/kgraphviewerpart )

install( FILES kgraphviewer_part.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
install( FILES kgraphviewer-bev.png DESTINATION ${DATA_INSTALL_DIR}/kgraphviewerpart/pics )
install( FILES kgraphviewer-newnode.png DESTINATION ${DATA_INSTALL_DIR}/kgraphviewerpart/pics )
install( FILES kgraphviewer-newedge.png DESTINATION ${DATA_INSTALL_DIR}/kgraphviewerpart/pics )

install(FILES kgraphviewer_interface.h DESTINATION ${INCLUDE_INSTALL_DIR}/kgraphviewer/ COMPONENT Devel)
