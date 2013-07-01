set(graphvizsimple_SRCS graphvizsimple.cpp)

kde4_add_executable(graphvizsimple ${graphvizsimple_SRCS})

target_link_libraries(graphvizsimple kgraphviewerlib ${KDE4_KDEUI_LIBS})
