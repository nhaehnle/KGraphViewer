set(simple_SRCS simple.cpp)

kde4_add_executable(simple ${simple_SRCS})

target_link_libraries(simple kgraphviewerlib ${KDE4_KDEUI_LIBS})
