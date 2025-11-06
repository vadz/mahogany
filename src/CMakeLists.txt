#################################################################################
# CMakeLists.txt for Main Mahogany Application
#################################################################################
# This file builds the main Mahogany GUI email client application.

# Core application sources
set(MAHOGANY_SOURCES
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
    adb/ImportMailrc.cpp
    adb/ImportPine.cpp
    adb/ImportText.cpp
    adb/ImportVCard.cpp
    adb/ImportXFMail.cpp
    adb/ProvBbdb.cpp
    adb/ProvDummy.cpp
    adb/ProvFC.cpp
    adb/ProvLine.cpp
    adb/ProvPalm.cpp
    adb/ProvPasswd.cpp
    
    # Modules
    modules/BareBonesEditor.cpp
    modules/Calendar.cpp
    modules/Filters.cpp
    modules/HtmlViewer.cpp
    modules/LayoutEditor.cpp
    modules/LayoutViewer.cpp
    modules/Mdummy.cpp
    modules/Migrate.cpp
    modules/NetscapeImporter.cpp
    modules/PalmOS.cpp
    modules/PineImport.cpp
    modules/TextViewer.cpp
    modules/XFMailImport.cpp
    modules/crypt/PGPEngine.cpp
    modules/spam/DspamFilter.cpp
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

# Python interface sources (optional)
set(PYTHON_SOURCES
    Python/HeaderInfo.cpp-swig
    Python/InitPython.cpp
    Python/MDialogs.cpp-swig
    Python/MailFolder.cpp-swig
    Python/Message.cpp-swig
    Python/MimePart.cpp-swig
    Python/MimeType.cpp-swig
    Python/PythonHelp.cpp
    Python/SendMessage.cpp-swig
    Python/swiglib.cpp-swig
)

# Create the main executable
add_executable(mahogany ${MAHOGANY_SOURCES})

# Add Python sources if enabled
if(ENABLE_PYTHON AND Python3_FOUND)
    target_sources(mahogany PRIVATE ${PYTHON_SOURCES})
endif()

# Set executable properties
set_target_properties(mahogany PROPERTIES
    OUTPUT_NAME M
    WIN32_EXECUTABLE TRUE
)

# Include directories
target_include_directories(mahogany PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/include
)

# Preprocessor definitions
target_compile_definitions(mahogany PRIVATE
    wxNO_AUI_LIB
    wxNO_GL_LIB  
    wxNO_MEDIA_LIB
    wxNO_RIBBON_LIB
    wxNO_RICHTEXT_LIB
    wxNO_STC_LIB
    wxNO_WEBVIEW_LIB
    wxNO_XRC_LIB
)

if(WIN32)
    target_compile_definitions(mahogany PRIVATE 
        WIN32 
        _WINDOWS 
        _UNICODE 
        UNICODE
    )
    if(BUILD_SHARED_LIBS)
        target_compile_definitions(mahogany PRIVATE WXUSINGDLL)
    endif()
endif()

# Compiler-specific settings
if(MSVC)
    target_compile_options(mahogany PRIVATE
        /Zm110  # Increase compiler memory limit
        /wd4100 /wd4244 /wd4267  # Disable specific warnings
    )
    # Use precompiled headers
    target_precompile_headers(mahogany PRIVATE
        "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_SOURCE_DIR}/include/Mpch.h>"
    )
endif()

# Link libraries
target_link_libraries(mahogany PRIVATE
    mahogany_imap
    mahogany_compface  
    mahogany_dspam
    mahogany_versit
    ${wxWidgets_LIBRARIES}
    ${PLATFORM_LIBS}
)

# Python support
if(ENABLE_PYTHON AND Python3_FOUND)
    target_link_libraries(mahogany PRIVATE Python3::Python)
    target_include_directories(mahogany PRIVATE ${Python3_INCLUDE_DIRS})
endif()

# SSL support
if(ENABLE_SSL AND OpenSSL_FOUND)
    target_link_libraries(mahogany PRIVATE OpenSSL::SSL OpenSSL::Crypto)
endif()

