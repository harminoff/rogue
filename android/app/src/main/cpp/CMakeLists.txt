cmake_minimum_required(VERSION 3.22.1)
project(roguetiles_android C)

set(REPO_ROOT ${CMAKE_CURRENT_LIST_DIR}/../../../../..)

set(ROGUE52_CFILES
    vers.c extern.c armor.c chase.c command.c daemon.c
    daemons.c fight.c init.c io.c list.c main.c mach_dep.c misc.c
    monsters.c move.c new_level.c options.c pack.c passages.c
    potions.c rings.c rip.c rooms.c save.c scrolls.c state.c
    sticks.c things.c weapons.c wizard.c xcrypt.c rogue52_port.c
)
set(ROGUE36_CFILES
    vers.c armor.c chase.c command.c daemon.c
    daemons.c fight.c init.c io.c list.c main.c misc.c
    monsters.c move.c newlevel.c options.c pack.c passages.c
    potions.c rings.c rip.c rooms.c save.c scrolls.c state.c
    sticks.c things.c weapons.c wizard.c xcrypt.c rogue36_port.c
)
set(SROGUE90_CFILES
    vers.c armor.c chase.c command.c daemon.c daemons.c disply.c
    encumb.c fight.c global.c init.c io.c list.c main.c misc.c
    monsters.c move.c new_leve.c options.c pack.c passages.c
    potions.c pstats.c rings.c rip.c rooms.c save.c scrolls.c
    state.c sticks.c things.c trader.c weapons.c wizard.c xcrypt.c
    srogue90_port.c
)

set(ROGUE_VARIANT_TOOLCHAIN_FLAGS -fPIC)
if(CMAKE_C_COMPILER_TARGET)
    list(APPEND ROGUE_VARIANT_TOOLCHAIN_FLAGS --target=${CMAKE_C_COMPILER_TARGET})
endif()
if(CMAKE_SYSROOT)
    list(APPEND ROGUE_VARIANT_TOOLCHAIN_FLAGS --sysroot=${CMAKE_SYSROOT})
endif()

function(add_rogue_variant_objects variant score_file lock_file)
    string(TOUPPER ${variant} variant_upper)
    set(variant_dir ${REPO_ROOT}/variants/${variant})
    set(build_dir ${CMAKE_CURRENT_BINARY_DIR}/${variant})
    set(redef ${variant_dir}/${variant}_symbols.redef)
    set(expanded_redef ${build_dir}/${variant}_symbols.expanded.redef)
    set(config_header ${build_dir}/${variant}_android_config.h)

    file(MAKE_DIRECTORY ${build_dir})
    file(WRITE ${config_header}
        "#ifndef ${variant_upper}_ANDROID_CONFIG_H\n"
        "#define ${variant_upper}_ANDROID_CONFIG_H\n"
        "#define SCOREFILE \"${score_file}\"\n"
        "#define LOCKFILE \"${lock_file}\"\n"
        "#endif\n"
    )

    add_custom_command(
        OUTPUT ${expanded_redef}
        COMMAND ${CMAKE_COMMAND}
            -DINPUT=${redef}
            -DOUTPUT=${expanded_redef}
            -DPREFIX=${variant}
            -P ${CMAKE_CURRENT_LIST_DIR}/expand_variant_redef.cmake
        DEPENDS ${redef} ${CMAKE_CURRENT_LIST_DIR}/expand_variant_redef.cmake
        VERBATIM
    )

    set(objects)
    foreach(source_file IN LISTS ${variant_upper}_CFILES)
        set(source ${variant_dir}/${source_file})
        get_filename_component(source_name ${source_file} NAME)
        set(object ${build_dir}/${source_name}.o)

        add_custom_command(
            OUTPUT ${object}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${build_dir}
            COMMAND ${CMAKE_C_COMPILER}
                ${ROGUE_VARIANT_TOOLCHAIN_FLAGS}
                -std=gnu89
                -fcommon
                -Wno-return-type
                -Wno-deprecated-non-prototype
                -DROGUE_ANDROID
                -DROGUE_VARIANT_HIDE_UNISTD
                -I${CMAKE_CURRENT_LIST_DIR}
                -I${variant_dir}/port
                -I${variant_dir}
                -include ${config_header}
                -include ${variant_dir}/${variant}_port.h
                -c ${source}
                -o ${object}
            COMMAND ${CMAKE_OBJCOPY}
                --redefine-syms ${expanded_redef}
                ${object}
            DEPENDS
                ${source}
                ${expanded_redef}
                ${variant_dir}/${variant}_port.h
                ${variant_dir}/port/pwd.h
                ${variant_dir}/port/termios.h
            VERBATIM
        )

        set_source_files_properties(${object} PROPERTIES
            EXTERNAL_OBJECT TRUE
            GENERATED TRUE
        )
        list(APPEND objects ${object})
    endforeach()

    set(${variant}_objects ${objects} PARENT_SCOPE)
endfunction()

add_rogue_variant_objects(rogue52 rogue52.scr rogue52.lck)
add_rogue_variant_objects(rogue36 rogue36.scr rogue36.lck)
add_rogue_variant_objects(srogue90 srogue90.scr srogue90.lck)

add_library(roguetiles SHARED
    android_curses.c
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
    ${rogue52_objects}
    ${rogue36_objects}
    ${srogue90_objects}
)

target_include_directories(roguetiles PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${REPO_ROOT}
    ${REPO_ROOT}/generated
    ${REPO_ROOT}/android/vendor/allegro/include
)

target_compile_definitions(roguetiles PRIVATE
    ROGUE_ANDROID
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
