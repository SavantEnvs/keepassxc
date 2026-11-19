# Passed to upstream's configure as -DCMAKE_PROJECT_KeePassXC_INCLUDE=<this file> by
# mayhem/build.sh. It runs right after upstream's project(KeePassXC) call and defers adding
# including mayhem/fuzz/targets.cmake to the END of the top-level CMakeLists.txt, when every
# upstream library target (keepassxc_core, keepassxc_gui, browser, sshagent, ...) exists
# (deferred calls may not add subdirectories, so the harness targets live in the top-level scope). This keeps the
# integration additive: no upstream file is edited.
set(KPXC_MAYHEM_FUZZ_DIR "${CMAKE_CURRENT_LIST_DIR}")
cmake_language(DEFER DIRECTORY "${CMAKE_SOURCE_DIR}"
               CALL include "${KPXC_MAYHEM_FUZZ_DIR}/targets.cmake")
