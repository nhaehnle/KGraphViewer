#TODO: remove dependencies on ../ and ../part
include_directories( ../part ../ ${Boost_INCLUDE_DIRS} ${graphviz_INCLUDE_DIRECTORIES} )

set( kgraphviewerlib_LIB_SRCS
   abstractgraphmodel.cpp
   graphmodel.cpp
   graphscene.cpp
   dotgraphmodel.cpp
   loadandlayoutjob.cpp
   graphelement.cpp
   graphsubgraph.cpp
   graphnode.cpp
   graphedge.cpp
   graphexporter.cpp
   pannerview.cpp
   canvassubgraph.cpp
   canvasnode.cpp
   canvasedge.cpp
   canvaselement.cpp
   dotgraph.cpp
   dotgraphview.cpp
   dot2qtconsts.cpp
   dotgrammar.cpp
   DotGraphParsingHelper.cpp
   FontsCache.cpp
   simpleprintingsettings.cpp
   simpleprintingengine.cpp
   simpleprintingcommand.cpp
   simpleprintingpagesetup.cpp
   simpleprintpreviewwindow_p.cpp
   simpleprintpreviewwindow.cpp
   KgvGlobal.cpp
   KgvUnit.cpp
   KgvUnitWidgets.cpp
   KgvPageLayoutColumns.cpp
   KgvPageLayoutDia.cpp
   KgvPageLayout.cpp
   KgvPageLayoutHeader.cpp
   KgvPageLayoutSize.cpp)

set( kgraphviewerprinting_UI simpleprintingpagesetupbase.ui KgvPageLayoutColumnsBase.ui KgvPageLayoutHeaderBase.ui )

#TODO: move this to the part where it belongs
kde4_add_kcfg_files(kgraphviewerlib_LIB_SRCS kgraphviewer_partsettings.kcfgc)

kde4_add_ui_files(kgraphviewerlib_LIB_SRCS ${kgraphviewerprinting_UI} )

kde4_add_library(kgraphviewerlib SHARED ${kgraphviewerlib_LIB_SRCS})

add_definitions(-DQT_STL)
target_link_libraries(kgraphviewerlib ${QT_QTSVG_LIBRARY} ${KDE4_KIO_LIBS} ${KDE4_THREADWEAVER_LIBRARY} ${graphviz_LIBRARIES})

set_target_properties(kgraphviewerlib PROPERTIES VERSION 2.1 SOVERSION 2 OUTPUT_NAME kgraphviewer)

install(TARGETS kgraphviewerlib ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES dotgraphview.h DESTINATION ${INCLUDE_INSTALL_DIR}/kgraphviewer/ COMPONENT Devel)
install(FILES kgraphviewer_export.h DESTINATION ${INCLUDE_INSTALL_DIR}/kgraphviewer/ COMPONENT Devel)

install( FILES kgraphviewer_partsettings.kcfg DESTINATION ${KCFG_INSTALL_DIR} )
