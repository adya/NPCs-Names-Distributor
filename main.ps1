../pack-skse-mod/pack.ps1 -CMAKE_SE_BUILD_PRESET vs2022-windows-vcpkg-se `
                          -CMAKE_AE_BUILD_PRESET vs2022-windows-vcpkg-ae `
                          -CMAKE_VR_CONFIG_PRESET '' `
                          -FOMOD_INCLUDE_PDB $true `
                          -FOMOD_REQUIRED_INSTALLATION_DIR 'FOMOD/Required Files' `
                          -FOMOD_MOD_NAME 'NPCs Names Distributor' `
                          -FOMOD_MOD_AUTHOR sasnikol `
                          -FOMOD_MOD_NEXUS_ID 73081 `
                          -FOMOD_DEFAULT_IMAGE FOMOD/images/cover.png

../pack-skse-mod/pack.ps1 -CMAKE_SE_BUILD_PRESET vs2022-windows-vcpkg-se-debug `
                          -CMAKE_AE_BUILD_PRESET vs2022-windows-vcpkg-ae-debug `
                          -CMAKE_VR_CONFIG_PRESET '' `
                          -CMAKE_BUILD_CONFIGURATION: Debug `
                          -FOMOD_INCLUDE_PDB $true `
                          -FOMOD_REQUIRED_INSTALLATION_DIR 'FOMOD/Required Files' `
                          -FOMOD_MOD_NAME 'NPCs Names Distributor Debug' `
                          -FOMOD_MOD_AUTHOR sasnikol `
                          -FOMOD_MOD_NEXUS_ID 73081 `
                          -FOMOD_DEFAULT_IMAGE FOMOD/images/cover_debug.png