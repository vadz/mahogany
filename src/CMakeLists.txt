# CMakeLists.txt for Mahogany application itself

# Create the main executable
add_executable(mahogany)

# Add sources used under all platforms
target_sources(mahogany PRIVATE
  # Classes
  classes/CacheFile.cpp
  classes/ComposeTemplate.cpp
  classes/ConfigSource.cpp
  classes/ConfigSourcesAll.cpp
  classes/FolderMonitor.cpp
  classes/FolderView.cpp
  classes/ListReceiver.cpp
  classes/MApplication.cpp
  classes/MEvent.cpp
  classes/MFilter.cpp
  classes/MFolder.cpp
  classes/MModule.cpp
  classes/MObject.cpp
  classes/MessageTemplate.cpp
  classes/MessageView.cpp
  classes/Moptions.cpp
  classes/Mpers.cpp
  classes/NewMailNotifier.cpp
  classes/PGPClickInfo.cpp
  classes/PathFinder.cpp
  classes/Profile.cpp
  classes/QuotedText.cpp
  classes/Sequence.cpp
  classes/XFace.cpp
  classes/kbList.cpp

  # GUI components
  gui/AddressExpander.cpp
  gui/ClickAtt.cpp
  gui/ClickURL.cpp
  gui/ConfigSourceChoice.cpp
  gui/CreateFolderWizard.cpp
  gui/ImportFoldersWizard.cpp
  gui/MImport.cpp
  gui/Mdnd.cpp
  gui/wxAttachDialog.cpp
  gui/wxBrowseButton.cpp
  gui/wxColumnsDlg.cpp
  gui/wxComposeView.cpp
  gui/wxDialogLayout.cpp
  gui/wxFiltersDialog.cpp
  gui/wxFolderMenu.cpp
  gui/wxFolderTree.cpp
  gui/wxFolderView.cpp
  gui/wxHeadersDialogs.cpp
  gui/wxIconManager.cpp
  gui/wxMApp.cpp
  gui/wxMDialogs.cpp
  gui/wxMFolderDialogs.cpp
  gui/wxMFrame.cpp
  gui/wxMGuiUtils.cpp
  gui/wxMIMETreeDialog.cpp
  gui/wxMLog.cpp
  gui/wxMSplash.cpp
  gui/wxMainFrame.cpp
  gui/wxMenuDefs.cpp
  gui/wxMessageView.cpp
  gui/wxMimeDialog.cpp
  gui/wxModulesDlg.cpp
  gui/wxMsgCmdProc.cpp
  gui/wxOptionsDlg.cpp
  gui/wxRenameDialog.cpp
  gui/wxSearchDialog.cpp
  gui/wxSortDialog.cpp
  gui/wxSubfoldersDialog.cpp
  gui/wxTemplateDialog.cpp
  gui/wxTextDialog.cpp
  gui/wxThrDialog.cpp
  gui/wxllist.cpp
  gui/wxlparser.cpp
  gui/wxlwindow.cpp

  # Mail handling
  mail/ASMailFolder.cpp
  mail/Address.cpp
  mail/AddressCC.cpp
  mail/FolderType.cpp
  mail/HeaderInfoImpl.cpp
  mail/HeaderIterator.cpp
  mail/LogCircle.cpp
  mail/MFCache.cpp
  mail/MFDriver.cpp
  mail/MFPool.cpp
  mail/MFui.cpp
  mail/MailFolder.cpp
  mail/MailFolderCC.cpp
  mail/MailFolderCmn.cpp
  mail/MailMH.cpp
  mail/Message.cpp
  mail/MessageCC.cpp
  mail/MimeDecode.cpp
  mail/MimePartCC.cpp
  mail/MimePartCCBase.cpp
  mail/MimePartVirtual.cpp
  mail/MimeType.cpp
  mail/Pop3.cpp
  mail/SendMessageCC.cpp
  mail/Sorting.cpp
  mail/SpamFilter.cpp
  mail/ThreadJWZ.cpp
  mail/Threading.cpp
  mail/VFolder.cpp
  mail/VMessage.cpp

  # Address book
  adb/AdbDialogs.cpp
  adb/AdbEntry.cpp
  adb/AdbExport.cpp
  adb/AdbFrame.cpp
  adb/AdbImport.cpp
  adb/AdbManager.cpp
  adb/AdbModule.cpp
  adb/AdbProvider.cpp
  adb/Collect.cpp
  adb/ExportText.cpp
  adb/ExportVCard.cpp
  adb/ImportEudora.cpp
  adb/ImportPine.cpp
  adb/ImportText.cpp
  adb/ImportVCard.cpp
  adb/ImportXFMail.cpp
  adb/ProvDummy.cpp
  adb/ProvFC.cpp

  # Modules
  modules/BareBonesEditor.cpp
  modules/Filters.cpp
  modules/HtmlViewer.cpp
  modules/LayoutEditor.cpp
  modules/LayoutViewer.cpp
  modules/Migrate.cpp
  modules/NetscapeImporter.cpp
  modules/PineImport.cpp
  modules/TextViewer.cpp
  modules/crypt/PGPEngine.cpp
  modules/spam/HeadersFilter.cpp
  modules/spam/ServerSideFilter.cpp
  modules/viewflt/PGP.cpp
  modules/viewflt/QuoteURL.cpp
  modules/viewflt/Rot13.cpp
  modules/viewflt/Signature.cpp
  modules/viewflt/TextMarkup.cpp
  modules/viewflt/Trailer.cpp
  modules/viewflt/UUDecode.cpp

  # Utilities
  util/ColourNames.cpp
  util/matchurl.cpp
  util/ssl.cpp
  util/strutil.cpp
  util/sysutil.cpp
  util/twofish2.c
  util/upgrade.cpp

  # wxWidgets extensions
  wx/common/vcard.cpp
  wx/generic/persctrl.cpp
  wx/generic/vcarddlg.cpp
)

# Some files are only compiled under Unix because they're useless under
# Windows (and wouldn't compile there).
if(UNIX)
  target_sources(mahogany PRIVATE
    adb/ImportMailrc.cpp
    adb/ProvBbdb.cpp
    adb/ProvLine.cpp
    adb/ProvPalm.cpp
    adb/ProvPasswd.cpp

    modules/XFMailImport.cpp
  )
endif()

if(WIN32)
  target_sources(mahogany PRIVATE ${CMAKE_SOURCE_DIR}/res/M.rc)
endif()

# Generated MInterface.* must be built before MModule.cpp can be compiled.
set_source_files_properties(
  classes/MModule.cpp
  PROPERTIES
    OBJECT_DEPENDS
      generate_interface_files
)

# Add Python sources if enabled
if(USE_PYTHON)
  # Python interface files.
  set(PYTHON_INTERFACES
    HeaderInfo
    MDialogs
    MailFolder
    Message
    MimePart
    MimeType
    SendMessage
  )

  set(PYTHON_SOURCES
    Python/InitPython.cpp
    Python/PythonHelp.cpp
  )

  # Directly containing SWIG-generated Python files.
  set(swig_python_outdir ${CMAKE_BINARY_DIR}/src/Python)

  if(SWIG_FOUND)
    add_custom_command(
      OUTPUT
        ${swig_python_outdir}/Mswigpyrun.h
      COMMAND
        ${SWIG_EXECUTABLE} -python -external-runtime
          ${swig_python_outdir}/Mswigpyrun.h
    )

    foreach(interface IN LISTS PYTHON_INTERFACES)
      set(source "${CMAKE_SOURCE_DIR}/include/interface/${interface}.i")
      list(APPEND PYTHON_SWIG_SOURCES ${source})

      set_source_files_properties(${source} PROPERTIES
        CPLUSPLUS TRUE
      )
    endforeach()

    set(SWIG_USE_SWIG_DEPENDENCIES TRUE)
    swig_add_library(mahogany_python
      TYPE
        STATIC
      LANGUAGE
        python
      SOURCES
        ${PYTHON_SWIG_SOURCES}
      OUTPUT_DIR
        ${swig_python_outdir}
    )

    set_target_properties(mahogany_python PROPERTIES
      SWIG_INCLUDE_DIRECTORIES
          "${CMAKE_SOURCE_DIR}/include;${CMAKE_SOURCE_DIR}/include/interface"
      SWIG_GENERATED_COMPILE_OPTIONS
          -w
    )

    # Create custom target copying back SWIG-generated files to source tree.
    set(swig_generated_files)

    # Define custom commands for updating files for all interfaces.
    foreach(interface IN LISTS PYTHON_INTERFACES)
      add_custom_command(
        OUTPUT
          ${CMAKE_CURRENT_SOURCE_DIR}/Python/${interface}.cpp-swig
          ${CMAKE_CURRENT_SOURCE_DIR}/Python/${interface}.py-swig
        COMMAND
          ${CMAKE_COMMAND} -E copy_if_different
            ${swig_python_outdir}/${interface}_wrap.cxx
            ${CMAKE_CURRENT_SOURCE_DIR}/Python/${interface}.cpp-swig
        COMMAND
          ${CMAKE_COMMAND} -E copy_if_different
            ${swig_python_outdir}/${interface}.py
            ${CMAKE_CURRENT_SOURCE_DIR}/Python/${interface}.py-swig
      )

      list(APPEND swig_generated_files
        ${CMAKE_CURRENT_SOURCE_DIR}/Python/${interface}.cpp-swig
        ${CMAKE_CURRENT_SOURCE_DIR}/Python/${interface}.py-swig
      )
    endforeach()

    # And also a separate one for updating Mswigpyrun.h in source directory.
    add_custom_command(
      OUTPUT
        ${CMAKE_CURRENT_SOURCE_DIR}/Python/Mswigpyrun.h-swig
      COMMAND
        ${CMAKE_COMMAND} -E copy_if_different
          ${swig_python_outdir}/Mswigpyrun.h
          ${CMAKE_CURRENT_SOURCE_DIR}/Python/Mswigpyrun.h-swig
    )

    list(APPEND swig_generated_files Python/Mswigpyrun.h-swig)

    add_custom_target(update_swig_output DEPENDS ${swig_generated_files})
  else()
    add_library(mahogany_python STATIC)

    # Use pre-generated SWIG files from the repository.
    foreach(interface IN LISTS PYTHON_INTERFACES)
      add_custom_command(
        OUTPUT
          ${swig_python_outdir}/${interface}_wrap.cxx
          ${swig_python_outdir}/${interface}.py
        COMMAND
          ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/Python/${interface}.cpp-swig
            ${swig_python_outdir}/${interface}_wrap.cxx
        COMMAND
          ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/Python/${interface}.py-swig
            ${swig_python_outdir}/${interface}.py
      )

      target_sources(mahogany_python
        PRIVATE
          ${swig_python_outdir}/${interface}_wrap.cxx
      )
    endforeach()

    add_custom_command(
      OUTPUT
        ${swig_python_outdir}/Mswigpyrun.h
      COMMAND
        ${CMAKE_COMMAND} -E copy_if_different
          ${CMAKE_CURRENT_SOURCE_DIR}/Python/Mswigpyrun.h-swig
          ${swig_python_outdir}/Mswigpyrun.h
    )
  endif()

  # Adding it to the sources ensures that it is generated before compiling
  # the files that use it.
  target_sources(mahogany_python
    PRIVATE
      ${swig_python_outdir}/Mswigpyrun.h
  )

  target_sources(mahogany_python
    PRIVATE
      ${PYTHON_SOURCES}
  )

  target_include_directories(mahogany_python
    PRIVATE
      ${CMAKE_BINARY_DIR}/include
      ${CMAKE_SOURCE_DIR}/include
      ${swig_python_outdir}
      ${Python3_INCLUDE_DIRS}
  )

  target_link_libraries(mahogany_python
    PRIVATE
      Python3::Python
      ${wxWidgets_LIBRARIES}
  )

  target_link_libraries(mahogany PRIVATE mahogany_python)
endif()

if(USE_DSPAM)
  target_sources(mahogany PRIVATE modules/spam/DspamFilter.cpp)
endif()

# Set executable properties
set_target_properties(mahogany PROPERTIES
  OUTPUT_NAME
    M
  WIN32_EXECUTABLE
    TRUE
)

# Include directories
target_include_directories(mahogany
  PRIVATE
    ${CMAKE_BINARY_DIR}/include
    ${CMAKE_SOURCE_DIR}/include
)

# Compiler/platform-specific compilation options
if(WIN32)
  target_compile_definitions(mahogany PRIVATE
    WIN32
    _WINDOWS
    _UNICODE
    UNICODE
  )
endif()

if(MSVC)
  target_compile_definitions(mahogany PRIVATE
    wxNO_AUI_LIB
    wxNO_GL_LIB
    wxNO_MEDIA_LIB
    wxNO_RIBBON_LIB
    wxNO_RICHTEXT_LIB
    wxNO_STC_LIB
    wxNO_WEBVIEW_LIB
    wxNO_XRC_LIB
    USE_PCH
  )

  target_compile_options(mahogany PRIVATE
    /Zm110   # Increase compiler memory limit
    /W4      # Maximum warning level
    /wd4100  # Disable unused parameter warnings
    /wd4244  # Disable conversion warnings
    /wd4267  # Disable size_t conversion warnings
  )
  # Use precompiled headers
  target_precompile_headers(mahogany PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:${CMAKE_SOURCE_DIR}/include/Mpch.h>
  )

  # Except for some files that don't include Mpch.h
  set_source_files_properties(
    wx/common/vcard.cpp
    wx/generic/persctrl.cpp
    wx/generic/vcarddlg.cpp
    PROPERTIES
      SKIP_PRECOMPILE_HEADERS ON
  )
elseif(IS_GCC_LIKE)
  target_compile_options(mahogany PRIVATE
    -Wall
    -Wextra
    -Wno-unused-parameter
    -Wno-missing-field-initializers
    -Wno-implicit-fallthrough # TODO: fix fall-through warnings and remove this
    $<$<COMPILE_LANGUAGE:CXX>:-fno-operator-names>
  )
endif()


# Platform-specific libraries
if(WIN32)
  set(PLATFORM_LIBS winmm comctl32 rpcrt4 ws2_32)
elseif(UNIX AND NOT APPLE)
  set(PLATFORM_LIBS dl pthread)
endif()

# Link libraries
target_link_libraries(mahogany PRIVATE
  mahogany_imap
  mahogany_compface
  mahogany_versit
  ${wxWidgets_LIBRARIES}
  ${PLATFORM_LIBS}
)

# Python support
if(USE_PYTHON)
  target_link_libraries(mahogany PRIVATE Python3::Python)
endif()

# SSL support via OpenSSL?
if(USE_OPENSSL)
  target_link_libraries(mahogany PRIVATE OpenSSL::SSL OpenSSL::Crypto)
endif()

if(USE_DSPAM)
  target_link_libraries(mahogany PRIVATE dspam)
endif()
