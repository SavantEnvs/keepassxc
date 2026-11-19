# libFuzzer harnesses for KeePassXC, included into upstream's top-level directory scope by
# inject.cmake (deferred to the end of upstream's CMakeLists.txt). Each harness is
# linked twice: with the fuzzing engine (fuzz_<name>) and with LLVM's standalone run-once driver
# (fuzz_<name>-standalone). Linked like upstream's unit tests (tests/CMakeLists.txt).

set(MAYHEM_FUZZERS
    kdbx_xml kdbx kdb csv bitwarden protonpass opux openssh_key keeagent_settings
    browser_host totp base32 filekey merge passkeys)

# keepassxc_gui and the feature libraries it links (browser, sshagent, keeshare, autotype) reference
# each other; link them as one rescanned group so every cross-reference resolves.
set(_libs "$<LINK_GROUP:RESCAN,keepassxc_gui,keepassxc_core,browser,sshagent,keeshare,autotype>")

separate_arguments(_engine UNIX_COMMAND "$ENV{LIB_FUZZING_ENGINE}")
add_library(mayhem_standalone_main OBJECT "$ENV{STANDALONE_FUZZ_MAIN}")
add_library(mayhem_lsan_off OBJECT "${MAYHEM_LSAN_OFF_SRC}")  # mayhem/lsan_off.cc, passed by build.sh

foreach(f ${MAYHEM_FUZZERS})
    set(_inc "${CMAKE_SOURCE_DIR}/src" "${CMAKE_BINARY_DIR}/src")
    add_executable(fuzz_${f} "${KPXC_MAYHEM_FUZZ_DIR}/fuzz_${f}.cpp" $<TARGET_OBJECTS:mayhem_lsan_off>)
    target_link_libraries(fuzz_${f} ${_libs})
    target_include_directories(fuzz_${f} PRIVATE ${_inc})
    target_link_options(fuzz_${f} PRIVATE ${_engine})

    add_executable(fuzz_${f}-standalone "${KPXC_MAYHEM_FUZZ_DIR}/fuzz_${f}.cpp" $<TARGET_OBJECTS:mayhem_lsan_off>
                   $<TARGET_OBJECTS:mayhem_standalone_main>)
    target_link_libraries(fuzz_${f}-standalone ${_libs})
    target_include_directories(fuzz_${f}-standalone PRIVATE ${_inc})

    set_target_properties(fuzz_${f} fuzz_${f}-standalone PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/mayhem-bin")
    list(APPEND _all fuzz_${f} fuzz_${f}-standalone)
endforeach()
add_custom_target(mayhem_fuzzers DEPENDS ${_all})
