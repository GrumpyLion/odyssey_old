cmake_minimum_required(VERSION 3.0.0)

project(vkbootstrap)

set(SOURCES
    "VkBootstrap.cpp"
    )

set(HEADERS
    "VkBootstrap.h"
    "VkBootstrapDispatch.h"
    )

assign_source_group(${SOURCES})
assign_source_group(${HEADERS})

add_library (vkbootstrap ${SOURCES} ${HEADERS})