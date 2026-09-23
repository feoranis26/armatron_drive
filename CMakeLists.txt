cmake_minimum_required(VERSION 3.8)
project(armatron_drive)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()


# find dependencies
find_package(ament_cmake REQUIRED)

add_executable(main src/main.cpp src/stepper.cpp src/whl_driver.cpp src/udp_control.cpp)
target_include_directories(main PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include/armatron_drive/")

target_link_libraries(main pigpio)


install(TARGETS
  main
  DESTINATION lib/${PROJECT_NAME})

install(PROGRAMS
  scripts/run-armatron-drive
  DESTINATION lib/${PROJECT_NAME}
)

# uncomment the following section in order to fill in
# further dependencies manually.
# find_package(<dependency> REQUIRED)

if(BUILD_TESTING)
  enable_testing()
  add_executable(test_control_packet test/test_control_packet.cpp)
  target_include_directories(test_control_packet PRIVATE include/armatron_drive)
  add_test(NAME control_packet COMMAND test_control_packet)
  find_package(ament_lint_auto REQUIRED)
  # the following line skips the linter which checks for copyrights
  # comment the line when a copyright and license is added to all source files
  set(ament_cmake_copyright_FOUND TRUE)
  # the following line skips cpplint (only works in a git repo)
  # comment the line when this package is in a git repo and when
  # a copyright and license is added to all source files
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
