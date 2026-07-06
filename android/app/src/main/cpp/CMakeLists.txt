cmake_minimum_required(VERSION 3.22.1)
project(roguetiles_android C)

set(REPO_ROOT ${CMAKE_CURRENT_LIST_DIR}/../../../../..)

add_library(roguetiles SHARED
    ${REPO_ROOT}/vers.c
    ${REPO_ROOT}/extern.c
    ${REPO_ROOT}/armor.c
    ${REPO_ROOT}/chase.c
    ${REPO_ROOT}/command.c
    ${REPO_ROOT}/daemon.c
    ${REPO_ROOT}/daemons.c
    ${REPO_ROOT}/fight.c
    ${REPO_ROOT}/init.c
    ${REPO_ROOT}/io.c
    ${REPO_ROOT}/list.c
    ${REPO_ROOT}/mach_dep.c
    ${REPO_ROOT}/main.c
    ${REPO_ROOT}/mdport.c
    ${REPO_ROOT}/misc.c
    ${REPO_ROOT}/monsters.c
    ${REPO_ROOT}/move.c
    ${REPO_ROOT}/new_level.c
    ${REPO_ROOT}/options.c
    ${REPO_ROOT}/pack.c
    ${REPO_ROOT}/passages.c
    ${REPO_ROOT}/potions.c
    ${REPO_ROOT}/rings.c
    ${REPO_ROOT}/rip.c
    ${REPO_ROOT}/rooms.c
    ${REPO_ROOT}/save.c
    ${REPO_ROOT}/scrolls.c
    ${REPO_ROOT}/state.c
    ${REPO_ROOT}/sticks.c
    ${REPO_ROOT}/things.c
    ${REPO_ROOT}/tiles.c
    ${REPO_ROOT}/tilepack.c
    ${REPO_ROOT}/frontend.c
    ${REPO_ROOT}/overlay_picker.c
    ${REPO_ROOT}/variant.c
    ${REPO_ROOT}/rogue_platform.c
    ${REPO_ROOT}/mobile_controls.c
    ${REPO_ROOT}/generated/rogue_tile_mapping.c
    ${REPO_ROOT}/allegro_frontend.c
    ${REPO_ROOT}/weapons.c
    ${REPO_ROOT}/wizard.c
    ${REPO_ROOT}/xcrypt.c
)

target_include_directories(roguetiles PRIVATE
    ${REPO_ROOT}
    ${REPO_ROOT}/generated
    ${REPO_ROOT}/android/vendor/allegro/include
)

target_compile_definitions(roguetiles PRIVATE
    ROGUE_ANDROID
    ROGUE_ANDROID_DEFAULT_ONLY
    ROGUE_ENABLE_ALLEGRO
    ALLSCORES
    SCOREFILE="rogue54.scr"
    LOCKFILE="rogue54.lck"
)

target_compile_options(roguetiles PRIVATE -std=gnu89 -fcommon)

find_library(log-lib log)
find_library(android-lib android)
find_library(gles-lib GLESv2)

add_library(allegro SHARED IMPORTED)
set_target_properties(allegro PROPERTIES IMPORTED_LOCATION
    ${REPO_ROOT}/android/vendor/allegro/jni/${ANDROID_ABI}/liballegro.so)
add_library(allegro_image SHARED IMPORTED)
set_target_properties(allegro_image PROPERTIES IMPORTED_LOCATION
    ${REPO_ROOT}/android/vendor/allegro/jni/${ANDROID_ABI}/liballegro_image.so)
add_library(allegro_font SHARED IMPORTED)
set_target_properties(allegro_font PROPERTIES IMPORTED_LOCATION
    ${REPO_ROOT}/android/vendor/allegro/jni/${ANDROID_ABI}/liballegro_font.so)
add_library(allegro_ttf SHARED IMPORTED)
set_target_properties(allegro_ttf PROPERTIES IMPORTED_LOCATION
    ${REPO_ROOT}/android/vendor/allegro/jni/${ANDROID_ABI}/liballegro_ttf.so)
add_library(allegro_primitives SHARED IMPORTED)
set_target_properties(allegro_primitives PROPERTIES IMPORTED_LOCATION
    ${REPO_ROOT}/android/vendor/allegro/jni/${ANDROID_ABI}/liballegro_primitives.so)

target_link_libraries(roguetiles
    allegro_image
    allegro_ttf
    allegro_font
    allegro_primitives
    allegro
    ${log-lib}
    ${android-lib}
    ${gles-lib}
)
