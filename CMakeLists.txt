cmake_minimum_required(VERSION 3.20)
project(TrayAppSuite VERSION 1.0.0)

# Build both sub-projects from the repo root
add_subdirectory(tray-app)
add_subdirectory(svc)

# Combined install target puts both binaries in dist/bin
install(TARGETS TrayApp  RUNTIME DESTINATION bin)
install(TARGETS TrayService RUNTIME DESTINATION bin)
