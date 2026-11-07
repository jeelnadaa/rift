// Loads the bundled fonts (ui/fonts → :/fonts) and sets the design's body font as the app default:
// Bricolage Grotesque 14 px, weight 450, width axis 96 (DESIGN.md §3). Every QML Text inherits what it
// doesn't override, so views only set family/size/weight where a role differs.
#pragma once
#include <QDirIterator>
#include <QFont>
#include <QFontDatabase>
#include <QGuiApplication>

inline void installAppFonts() {
    QDirIterator it(QStringLiteral(":/fonts"), {QStringLiteral("*.ttf"), QStringLiteral("*.otf")}, QDir::Files);
    bool haveUi = false;
    while (it.hasNext()) {
        const int id = QFontDatabase::addApplicationFont(it.next());
        if (id >= 0 && QFontDatabase::applicationFontFamilies(id).contains(QStringLiteral("Bricolage Grotesque"))) haveUi = true;
    }
    QFont f = QGuiApplication::font();
    if (haveUi) f.setFamilies({QStringLiteral("Bricolage Grotesque")});
    f.setPixelSize(14);
    f.setWeight(static_cast<QFont::Weight>(450));
    f.setVariableAxis(QFont::Tag("wdth"), 96);
    f.setHintingPreference(QFont::PreferVerticalHinting);
    f.setStyleStrategy(QFont::PreferAntialias);
    QGuiApplication::setFont(f);
}
