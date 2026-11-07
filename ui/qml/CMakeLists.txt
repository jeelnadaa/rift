# The QML module "Rift": every view and component of the app. Flat on purpose (one directory).
qt_add_library(rift_ui STATIC)
set_source_files_properties(Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
qt_add_qml_module(rift_ui
    URI Rift
    OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/qml/Rift
    VERSION 1.0
    QML_FILES
        Main.qml Theme.qml Icons.js
        Icon.qml Logo.qml Tip.qml Btn.qml Field.qml Check.qml Toggle.qml Seg.qml Select.qml NumField.qml
        RMenu.qml RMenuItem.qml Badge.qml Kind.qml Prog.qml SegBar.qml PieceStrip.qml Banner.qml Toasts.qml
        RDialog.qml SpeedGraph.qml Kbd.qml Cap.qml KV.qml
        Sidebar.qml SideItem.qml Toolbar.qml DownloadList.qml DownloadRow.qml BulkBar.qml DetailsPanel.qml
        StatusBar.qml EmptyState.qml RowMenu.qml
        AddLinkDialog.qml AddTorrentDialog.qml RemoveDialog.qml LimitsDialog.qml TextPrompt.qml
        SettingsDialog.qml SettingRow.qml CommandPalette.qml AboutDialog.qml ExtensionDialog.qml
        StatsView.qml HistoryView.qml ComingSoon.qml)
