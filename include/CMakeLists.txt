find_program(M4 m4 DOC "Macro processor (m4)")

set(interface_outdir ${CMAKE_BINARY_DIR}/include)
if(M4)
  set(interface_file ${CMAKE_CURRENT_SOURCE_DIR}/MInterface.mid)

  # Function used to generate custom commands for generating all files created
  # from MInterface.h.m4 defining the API interface.
  function(add_interface_command ext)
    add_custom_command(
      OUTPUT
        ${interface_outdir}/MInterface.${ext}
      COMMAND
        ${M4} -DM4FILE=${CMAKE_CURRENT_SOURCE_DIR}/mid2${ext}.m4
          ${interface_file} > ${interface_outdir}/MInterface.${ext}
      DEPENDS
        ${interface_file}
        ${CMAKE_CURRENT_SOURCE_DIR}/mid2${ext}.m4
      COMMENT
        "Generating MInterface.${ext}"
    )

    # Also define a custom command to update the files in the source directory.
    add_custom_command(
      OUTPUT
        ${CMAKE_CURRENT_SOURCE_DIR}/MInterface.${ext}.m4
      COMMAND
        ${CMAKE_COMMAND} -E copy
          ${interface_outdir}/MInterface.${ext}
          ${CMAKE_CURRENT_SOURCE_DIR}/MInterface.${ext}.m4
      DEPENDS
        ${interface_outdir}/MInterface.${ext}
      COMMENT
        "Updating source MInterface.${ext}.m4"
    )
  endfunction()

  add_interface_command(h)
  add_interface_command(cpp)
  add_interface_command(idl)

  add_custom_target(generate_interface_files
    DEPENDS
      ${interface_outdir}/MInterface.h
      ${interface_outdir}/MInterface.cpp
  )

  add_custom_target(update_m4_output
    DEPENDS
      ${CMAKE_CURRENT_SOURCE_DIR}/MInterface.h.m4
      ${CMAKE_CURRENT_SOURCE_DIR}/MInterface.cpp.m4
      ${CMAKE_CURRENT_SOURCE_DIR}/MInterface.idl.m4
  )
else()
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/MInterface.h.m4
    ${interface_outdir}/MInterface.h
    COPYONLY
  )
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/MInterface.cpp.m4
    ${interface_outdir}/MInterface.cpp
    COPYONLY
  )
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/MInterface.idl.m4
    ${interface_outdir}/MInterface.idl
    COPYONLY
  )
endif()

# Generate config.h.
configure_file(
  ${CMAKE_SOURCE_DIR}/build/cmake/config.h.in
  config.h
)
