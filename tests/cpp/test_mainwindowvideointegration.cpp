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
    void videoModeExposesReadOnlyZoomReadout();
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
    QVERIFY(mainQml.contains(QStringLiteral("setSource(Qt.resolvedUrl(\"VideoViewport.qml\")")));
    QVERIFY(!mainQml.contains(QStringLiteral("sourceComponent: VideoViewport")));
    QVERIFY(!mainQml.contains(QStringLiteral("property VideoViewport")));
    QVERIFY(mainQml.contains(QStringLiteral("active: page.imageMode")));
    QVERIFY(mainQml.contains(QStringLiteral("sourceComponent: ImageShortcuts")));
    QVERIFY(
        mainQml.contains(QStringLiteral("currentMediaNumber: documentSession.currentMediaNumber")));
    QVERIFY(mainQml.contains(QStringLiteral("mediaCount: documentSession.mediaCount")));
    QVERIFY(mainQml.contains(
        QStringLiteral("mediaNavigationKnown: documentSession.mediaNavigationKnown")));
    QVERIFY(mainQml.contains(QStringLiteral("documentSession.openMediaAtNumber(mediaNumber)")));
    QVERIFY(mainQml.contains(QStringLiteral("showVideoZoomReadout: page.videoMode")));
    QVERIFY(imageActionsQml.contains(QStringLiteral("root.documentSession.mediaNavigationActive")));
    QVERIFY(imageActionsQml.contains(QStringLiteral("root.documentSession.openPreviousMedia()")));
    QVERIFY(imageActionsQml.contains(QStringLiteral("root.documentSession.openNextMedia()")));
    QVERIFY(imageActionsQml.contains(QStringLiteral("root.documentSession.openMediaAtNumber")));
    QVERIFY(imageActionsQml.contains(QStringLiteral("root.documentSession.deleteDisplayedFile")));
    QVERIFY(!mainQml.contains(QStringLiteral("videoApplicationMenuHost")));
    QVERIFY(!mainQml.contains(QStringLiteral("ordinaryDirectMediaScopeActive")));
    QVERIFY(!imageActionsQml.contains(QStringLiteral("ordinaryDirectMediaScopeActive")));
}

void TestMainWindowVideoIntegration::videoModeExposesReadOnlyZoomReadout()
{
    const QString mainQml = readSource(QStringLiteral("src/qml/Main.qml"));
    const QString videoViewportQml = readSource(QStringLiteral("src/qml/VideoViewport.qml"));
    const QString imageToolBarQml = readSource(QStringLiteral("src/qml/ImageToolBar.qml"));
    QVERIFY2(!mainQml.isEmpty(), "Main.qml should be readable");
    QVERIFY2(!videoViewportQml.isEmpty(), "VideoViewport.qml should be readable");
    QVERIFY2(!imageToolBarQml.isEmpty(), "ImageToolBar.qml should be readable");

    QVERIFY(videoViewportQml.contains(QStringLiteral("videoZoomPercentForRects")));
    QVERIFY(videoViewportQml.contains(QStringLiteral("videoOutput.contentRect")));
    QVERIFY(videoViewportQml.contains(QStringLiteral("videoOutput.sourceRect")));
    QVERIFY(videoViewportQml.contains(QStringLiteral("displayDevicePixelRatio")));
    QVERIFY(imageToolBarQml.contains(QStringLiteral("videoZoomLevelAction")));
    QVERIFY(imageToolBarQml.contains(QStringLiteral("showVideoZoomReadout")));
    QVERIFY(imageToolBarQml.contains(
        QStringLiteral("showImageControls ? imageToolbarControls : videoToolbarControls")));
    QVERIFY(mainQml.contains(QStringLiteral("videoZoomPercent: page.videoZoomPercent")));
    QVERIFY(mainQml.contains(QStringLiteral("videoZoomReady: page.videoZoomReady")));
}

QTEST_GUILESS_MAIN(TestMainWindowVideoIntegration)

#include "test_mainwindowvideointegration.moc"
