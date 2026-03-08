# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/SKSE-MCP
    REF f6010de4e0971bcd7a0cb4dbadc9f2ec2e69d75c
    SHA512 4f636aa97e08ba0db85cb84a5bf9809fafec1a79a2e463f8cd74b4485c19af0336e84b515b59bd0f2e13bf31de2d01e5ff740f361bbc500efa8941b347cc898b
    HEAD_REF main
)

# Install codes
set(SKSEMCP_SOURCE	${SOURCE_PATH}/include/SKSEMCP)
file(INSTALL ${SKSEMCP_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")