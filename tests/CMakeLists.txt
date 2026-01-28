# LevelGuard Core - Test Configuration
cmake_minimum_required(VERSION 3.28)

include(FetchContent)

# Google Test をダウンロード
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.14.0
)

# Windows: gtest を共有ランタイムでビルドしない
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

enable_testing()

# テスト実行ファイル
add_executable(
  levelguard_core_test
  state_machine_test.cpp
  logger_test.cpp
  state_history_test.cpp
  dsp_test.cpp
  limiter_test.cpp
  compressor_test.cpp
  gain_controller_test.cpp
  dsp_chain_test.cpp
  human_operation_test.cpp
  core_interface_test.cpp
)

# インクルードパス
target_include_directories(
  levelguard_core_test
  PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

# ソースファイル（テスト対象）
target_sources(
  levelguard_core_test
  PRIVATE
    ${CMAKE_SOURCE_DIR}/src/core/state_machine.cpp
    ${CMAKE_SOURCE_DIR}/src/dsp/true_peak.cpp
    ${CMAKE_SOURCE_DIR}/src/dsp/rms.cpp
    ${CMAKE_SOURCE_DIR}/src/dsp/lufs.cpp
    ${CMAKE_SOURCE_DIR}/src/dsp/limiter.cpp
    ${CMAKE_SOURCE_DIR}/src/dsp/compressor.cpp
    ${CMAKE_SOURCE_DIR}/src/dsp/gain_controller.cpp
    ${CMAKE_SOURCE_DIR}/src/dsp/dsp_chain.cpp
    ${CMAKE_SOURCE_DIR}/src/detection/human_operation.cpp
    ${CMAKE_SOURCE_DIR}/src/core/core_interface.cpp
)

# Google Test とリンク
target_link_libraries(
  levelguard_core_test
  PRIVATE
    GTest::gtest_main
)

# C++17
target_compile_features(levelguard_core_test PRIVATE cxx_std_17)

# CTest に登録
include(GoogleTest)
gtest_discover_tests(levelguard_core_test)
