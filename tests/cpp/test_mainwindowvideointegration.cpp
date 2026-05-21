// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <QDir>
#include <QFile>
#include <QObject>
#include <QString>
#include <QTest>

class TestMainWindowVideoIntegration : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void mainWindowRoutesExternalInputsThroughDocumentSession();
    void imageViewportUsesExternallyOwnedImageDocument();
    void mainWindowUsesSessionModeAndMediaDispatch();
};

namespace {
QString sourceFile(const QString &relativePath)
{
    return QDir(QStringLiteral(KIRIVIEW_TEST_SOURCE_DIR))
        .absoluteFilePath(QStringLiteral("../../") + relativePath);
}

QString readSource(const QString &relativePath)
{
    QFile file(sourceFile(relativePath));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }

    return QString::fromUtf8(file.readAll());
}
}

void TestMainWindowVideoIntegration::mainWindowRoutesExternalInputsThroughDocumentSession()
{
    const QString mainQml = readSource(QStringLiteral("src/qml/Main.qml"));
    QVERIFY2(!mainQml.isEmpty(), "Main.qml should be readable");

    QVERIFY(mainQml.contains(QStringLiteral("KiriDocumentSession")));
    QVERIFY(mainQml.contains(QStringLiteral("documentSession.sourceUrl = urls[0]")));
    QVERIFY(mainQml.contains(QStringLiteral("documentSession.sourceUrl = selectedFile")));
    QVERIFY(mainQml.contains(QStringLiteral("sourceUrl = root.initialSourceUrl")));
    QVERIFY(!mainQml.contains(QStringLiteral("imageDocument.sourceUrl =")));
    QVERIFY(mainQml.contains(QStringLiteral("nameFilters: documentSession.openDialogNameFilters")));
    QVERIFY(mainQml.contains(QStringLiteral("title: documentSession.windowTitleFileName")));
}

void TestMainWindowVideoIntegration::imageViewportUsesExternallyOwnedImageDocument()
{
    const QString imageViewportQml = readSource(QStringLiteral("src/qml/ImageViewport.qml"));
    QVERIFY2(!imageViewportQml.isEmpty(), "ImageViewport.qml should be readable");

    QVERIFY(imageViewportQml.contains(
        QStringLiteral("required property KiriImageDocument imageDocument")));
    QVERIFY(!imageViewportQml.contains(QStringLiteral("property url initialSourceUrl")));
    QVERIFY(!imageViewportQml.contains(QStringLiteral("KiriImageDocument {")));
    QVERIFY(!imageViewportQml.contains(QStringLiteral("sourceUrl = root.initialSourceUrl")));
}

void TestMainWindowVideoIntegration::mainWindowUsesSessionModeAndMediaDispatch()
{
    const QString mainQml = readSource(QStringLiteral("src/qml/Main.qml"));
    const QString imageActionsQml = readSource(QStringLiteral("src/qml/ImageActions.qml"));
    QVERIFY2(!mainQml.isEmpty(), "Main.qml should be readable");
    QVERIFY2(!imageActionsQml.isEmpty(), "ImageActions.qml should be readable");

    QVERIFY(mainQml.contains(
        QStringLiteral("documentSession.documentKind === KiriDocumentSession.Image")));
    QVERIFY(mainQml.contains(
        QStringLiteral("documentSession.documentKind === KiriDocumentSession.Video")));
    QVERIFY(mainQml.contains(QStringLiteral("active: page.videoMode")));
    QVERIFY(mainQml.contains(QStringLiteral("sourceComponent: VideoViewport")));
    QVERIFY(mainQml.contains(QStringLiteral("active: page.imageMode")));
    QVERIFY(mainQml.contains(QStringLiteral("sourceComponent: ImageShortcuts")));
    QVERIFY(imageActionsQml.contains(QStringLiteral("root.documentSession.mediaNavigationActive")));
    QVERIFY(imageActionsQml.contains(QStringLiteral("root.documentSession.openPreviousMedia()")));
    QVERIFY(imageActionsQml.contains(QStringLiteral("root.documentSession.openNextMedia()")));
    QVERIFY(imageActionsQml.contains(QStringLiteral("root.documentSession.deleteDisplayedFile")));
    QVERIFY(!mainQml.contains(QStringLiteral("ordinaryDirectMediaScopeActive")));
    QVERIFY(!imageActionsQml.contains(QStringLiteral("ordinaryDirectMediaScopeActive")));
}

QTEST_GUILESS_MAIN(TestMainWindowVideoIntegration)

#include "test_mainwindowvideointegration.moc"
