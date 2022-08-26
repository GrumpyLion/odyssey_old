cmake_minimum_required(VERSION 3.0.0)

project(imgui)

set(SOURCES
    "imgui_demo.cpp"
    "imgui_draw.cpp"
    "imgui_widgets.cpp"
    "imgui.cpp"
    "imgui_tables.cpp"
    )

set(HEADERS
    "imgui_internal.h"
    "imconfig.h"
    "imgui.h"
    "imstb_rectpack.h"
    "imstb_textedit.h"
    "imstb_truetype.h"
    )

assign_source_group(${SOURCES})
assign_source_group(${HEADERS})

add_library (imgui ${SOURCES} ${HEADERS})