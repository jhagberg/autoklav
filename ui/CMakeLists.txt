cmake_minimum_required(VERSION 3.16)
project(autoklav_ui)

# Auto-generated file list (populated by LVGL editor on export)
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/file_list_gen.cmake")
    include(${CMAKE_CURRENT_LIST_DIR}/file_list_gen.cmake)
endif()

if(LV_EDITOR_PREVIEW)
    # ── LVGL Editor Preview mode ──────────────────────────────
    # Used when opening this project in viewer.lvgl.io
    # or the desktop LVGL Pro Editor
    add_library(lib-ui ${PROJECT_SOURCES})

else()
    # ── ESP-IDF target build ──────────────────────────────────
    idf_component_register(
        SRCS ${PROJECT_SOURCES}
        INCLUDE_DIRS "."
        REQUIRES lvgl
    )
    target_compile_definitions(${COMPONENT_LIB} PUBLIC
        "-DLV_LVGL_H_INCLUDE_SIMPLE"
    )
endif()
