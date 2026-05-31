// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <QDir>
#include <QFile>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTest>

class TestMainWindowVideoIntegration : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void mainWindowRoutesExternalInputsThroughDocumentSession();
    void imageViewportUsesExternallyOwnedImageDocument();
    void mainWindowUsesSessionModeAndMediaDispatch();
    void mediaViewportDelegatesImplementSharedContract();
    void videoModeExposesReadOnlyZoomReadout();
    void toolbarPageNavigationUsesSessionActiveProjection();
    void thumbnailPanelUsesSessionThumbnailModel();
    void infoPanelUsesSessionMediaInformationAndResponsiveLayouts();
    void activeNavigationActionsUseSessionSnapshotAndBoundaryScope();
    void shortcutsRouteSharedActiveNavigationThroughSessionRequests();
    void imageActionAvailabilityDoesNotDriveSharedActiveNavigation();
    void pageNavigationComponentDoesNotChooseBetweenRawNavigationSources();
    void documentSessionFacadeDoesNotExposeRawDirectMediaNavigation();
    void documentSessionRuntimePublicApiDoesNotExposeRawDirectMediaNavigation();
    void videoPlaybackControlsUseResponsiveFloatingAndFixedModes();
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

void verifySourceOmits(const QString &source, const QStringList &needles)
{
    for (const QString &needle : needles) {
        QVERIFY2(!source.contains(needle),
            qPrintable(QStringLiteral("unexpected source text: %1").arg(needle)));
    }
}

QString firstPublicSection(const QString &source)
{
    const qsizetype publicIndex = source.indexOf(QStringLiteral("public:"));
    const qsizetype privateIndex = source.indexOf(QStringLiteral("private:"), publicIndex);
    if (publicIndex < 0 || privateIndex < 0 || privateIndex <= publicIndex) {
        return {};
    }

    return source.mid(publicIndex, privateIndex - publicIndex);
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
    QVERIFY(mainQml.contains(QStringLiteral("title: documentSession.windowTitleSubject")));
}

void TestMainWindowVideoIntegration::imageViewportUsesExternallyOwnedImageDocument()
{
    const QString imageViewportQml = readSource(QStringLiteral("src/qml/ImageViewport.qml"));
    QVERIFY2(!imageViewportQml.isEmpty(), "ImageViewport.qml should be readable");

    QVERIFY(imageViewportQml.contains(QStringLiteral("MediaViewportDelegate {")));
    QVERIFY(imageViewportQml.contains(
        QStringLiteral("readonly property var imageDocument: root.documentSession.imageDocument")));
    QVERIFY(imageViewportQml.contains(
        QStringLiteral("imageInteractionSurface: ImageViewportInteractionSurface")));
    QVERIFY(!imageViewportQml.contains(QStringLiteral("property url initialSourceUrl")));
    QVERIFY(!imageViewportQml.contains(QStringLiteral("KiriImageDocument {")));
    QVERIFY(!imageViewportQml.contains(
        QStringLiteral("required property KiriImageDocument imageDocument")));
    QVERIFY(!imageViewportQml.contains(QStringLiteral("sourceUrl = root.initialSourceUrl")));
}

void TestMainWindowVideoIntegration::mainWindowUsesSessionModeAndMediaDispatch()
{
    const QString mainQml = readSource(QStringLiteral("src/qml/Main.qml"));
    const QString mediaWorkspaceHostQml
        = readSource(QStringLiteral("src/qml/MediaWorkspaceHost.qml"));
    const QString mediaViewportHostQml
        = readSource(QStringLiteral("src/qml/MediaViewportHost.qml"));
    const QString imageActionsQml = readSource(QStringLiteral("src/qml/ImageActions.qml"));
    QVERIFY2(!mainQml.isEmpty(), "Main.qml should be readable");
    QVERIFY2(!mediaWorkspaceHostQml.isEmpty(), "MediaWorkspaceHost.qml should be readable");
    QVERIFY2(!mediaViewportHostQml.isEmpty(), "MediaViewportHost.qml should be readable");
    QVERIFY2(!imageActionsQml.isEmpty(), "ImageActions.qml should be readable");

    QVERIFY(mainQml.contains(
        QStringLiteral("documentSession.documentKind === KiriDocumentSession.Image")));
    QVERIFY(mainQml.contains(
        QStringLiteral("documentSession.documentKind === KiriDocumentSession.Video")));
    QVERIFY(mainQml.contains(QStringLiteral("MediaWorkspaceHost {")));
    QVERIFY(mainQml.contains(QStringLiteral("mediaWorkspaceHost.forceActiveViewportFocus()")));
    QVERIFY(mainQml.contains(
        QStringLiteral("imageInteractionSurface: mediaWorkspaceHost.imageInteractionSurface")));
    QVERIFY(
        mainQml.contains(QStringLiteral("infoPanelVisible: mediaWorkspaceHost.infoPanelVisible")));
    QVERIFY(mainQml.contains(
        QStringLiteral("thumbnailPanelVisible: mediaWorkspaceHost.thumbnailPanelVisible")));
    QVERIFY(mainQml.contains(QStringLiteral("mediaWorkspaceHost.toggleInfoPanel()")));
    QVERIFY(mainQml.contains(QStringLiteral("mediaWorkspaceHost.toggleThumbnailPanel()")));
    QVERIFY(!mainQml.contains(QStringLiteral("MediaViewportHost {")));
    QVERIFY(!mainQml.contains(QStringLiteral("InfoPanel {")));
    QVERIFY(!mainQml.contains(QStringLiteral("ThumbnailPanel {")));
    QVERIFY(!mainQml.contains(QStringLiteral("id: contentSplitView")));
    QVERIFY(!mainQml.contains(QStringLiteral("id: mediaPanelSplitView")));
    QVERIFY(!mainQml.contains(QStringLiteral("setSource(Qt.resolvedUrl(\"VideoViewport.qml\")")));
    QVERIFY(!mainQml.contains(QStringLiteral("id: videoViewportLoader")));
    QVERIFY(!mainQml.contains(QStringLiteral("ImageViewport {")));
    QVERIFY(mediaWorkspaceHostQml.contains(QStringLiteral("MediaViewportHost {")));
    QVERIFY(mediaWorkspaceHostQml.contains(QStringLiteral("InfoPanel {")));
    QVERIFY(mediaWorkspaceHostQml.contains(QStringLiteral("ThumbnailPanel {")));
    QVERIFY(
        mediaWorkspaceHostQml.contains(QStringLiteral("documentSession: root.documentSession")));
    QVERIFY(mediaWorkspaceHostQml.contains(QStringLiteral("id: contentSplitView")));
    QVERIFY(mediaWorkspaceHostQml.contains(QStringLiteral("id: mediaPanelSplitView")));
    QVERIFY(mediaWorkspaceHostQml.contains(QStringLiteral("function toggleInfoPanel()")));
    QVERIFY(mediaWorkspaceHostQml.contains(QStringLiteral("function toggleThumbnailPanel()")));
    QVERIFY(mediaWorkspaceHostQml.contains(
        QStringLiteral("mediaViewportHost.forceActiveViewportFocus()")));
    QVERIFY(mediaWorkspaceHostQml.contains(
        QStringLiteral("imageInteractionSurface: mediaViewportHost.imageInteractionSurface")));
    QVERIFY(mediaWorkspaceHostQml.contains(QStringLiteral("viewerContextMenuRequested")));
    QVERIFY(mediaViewportHostQml.contains(
        QStringLiteral("documentSession.documentKind === KiriDocumentSession.Image")));
    QVERIFY(mediaViewportHostQml.contains(
        QStringLiteral("documentSession.documentKind === KiriDocumentSession.Video")));
    QVERIFY(mediaViewportHostQml.contains(QStringLiteral("id: mediaViewportDelegateLoader")));
    QVERIFY(
        mediaViewportHostQml.contains(QStringLiteral("active: root.imageMode || root.videoMode")));
    QVERIFY(mediaViewportHostQml.contains(QStringLiteral("activeDelegateSource")));
    QVERIFY(mediaViewportHostQml.contains(QStringLiteral("setSource(root.activeDelegateSource")));
    QVERIFY(mediaViewportHostQml.contains(QStringLiteral("imageInteractionSurface")));
    QVERIFY(mediaViewportHostQml.contains(QStringLiteral("inactiveImageInteractionSurface")));
    QVERIFY(!mediaViewportHostQml.contains(QStringLiteral("activeInteractionSurface")));
    QVERIFY(!mediaViewportHostQml.contains(QStringLiteral("id: imageViewportLoader")));
    QVERIFY(!mediaViewportHostQml.contains(QStringLiteral("id: videoViewportLoader")));
    QVERIFY(!mediaViewportHostQml.contains(QStringLiteral("callImageViewport")));
    QVERIFY(!mediaViewportHostQml.contains(QStringLiteral("imageViewportProperty")));
    QVERIFY(!mediaViewportHostQml.contains(QStringLiteral("sourceComponent: VideoViewport")));
    QVERIFY(!mediaViewportHostQml.contains(QStringLiteral("property VideoViewport")));
    QVERIFY(!mediaViewportHostQml.contains(QStringLiteral("ImageViewport {")));
    QVERIFY(mediaViewportHostQml.contains(QStringLiteral("\"presentationActive\": true")));
    QVERIFY(!mediaViewportHostQml.contains(QStringLiteral("function panBy")));
    QVERIFY(!mediaViewportHostQml.contains(QStringLiteral("function zoomByStepAtCenter")));
    QVERIFY(mediaViewportHostQml.contains(QStringLiteral("ImageStateOverlay {")));
    QVERIFY(mediaViewportHostQml.contains(QStringLiteral("viewerContextMenuRequested")));
    QVERIFY(mainQml.contains(QStringLiteral("sourceComponent: ImageShortcuts")));
    QVERIFY(mainQml.contains(QStringLiteral("active: page.imageMode || page.videoMode")));
    QVERIFY(mainQml.contains(QStringLiteral(
        "activeNavigationCurrentNumber: documentSession.activeNavigationCurrentNumber")));
    QVERIFY(mainQml.contains(
        QStringLiteral("activeNavigationCount: documentSession.activeNavigationCount")));
    QVERIFY(mainQml.contains(
        QStringLiteral("activeNavigationKnown: documentSession.activeNavigationKnown")));
    QVERIFY(
        mainQml.contains(QStringLiteral("documentSession.openActiveNavigationAtNumber(number)")));
    QVERIFY(mainQml.contains(QStringLiteral("videoMode: page.videoMode")));
    QVERIFY(!mainQml.contains(QStringLiteral("showVideoZoomReadout")));
    QVERIFY(!mainQml.contains(QStringLiteral("showImageControls")));
    QVERIFY(!mainQml.contains(QStringLiteral("enabled: !page.imageMode")));
    QVERIFY(
        imageActionsQml.contains(QStringLiteral("root.documentSession.activeNavigationAvailable")));
    QVERIFY(imageActionsQml.contains(
        QStringLiteral("root.documentSession.requestPreviousActiveNavigationBoundaryText()")));
    QVERIFY(imageActionsQml.contains(
        QStringLiteral("root.documentSession.requestNextActiveNavigationBoundaryText()")));
    QVERIFY(imageActionsQml.contains(
        QStringLiteral("root.documentSession.openFirstActiveNavigation()")));
    QVERIFY(imageActionsQml.contains(
        QStringLiteral("root.documentSession.openLastActiveNavigation()")));
    QVERIFY(imageActionsQml.contains(QStringLiteral("root.documentSession.deleteDisplayedFile")));
    QVERIFY(!mainQml.contains(QStringLiteral("videoApplicationMenuHost")));
    QVERIFY(!mainQml.contains(QStringLiteral("ordinaryDirectMediaScopeActive")));
    QVERIFY(!imageActionsQml.contains(QStringLiteral("ordinaryDirectMediaScopeActive")));
}

void TestMainWindowVideoIntegration::mediaViewportDelegatesImplementSharedContract()
{
    const QString mediaDelegateQml
        = readSource(QStringLiteral("src/qml/MediaViewportDelegate.qml"));
    const QString interactionSurfaceQml
        = readSource(QStringLiteral("src/qml/ImageViewportInteractionSurface.qml"));
    const QString imageViewportQml = readSource(QStringLiteral("src/qml/ImageViewport.qml"));
    const QString videoViewportQml = readSource(QStringLiteral("src/qml/VideoViewport.qml"));
    QVERIFY2(!mediaDelegateQml.isEmpty(), "MediaViewportDelegate.qml should be readable");
    QVERIFY2(
        !interactionSurfaceQml.isEmpty(), "ImageViewportInteractionSurface.qml should be readable");
    QVERIFY2(!imageViewportQml.isEmpty(), "ImageViewport.qml should be readable");
    QVERIFY2(!videoViewportQml.isEmpty(), "VideoViewport.qml should be readable");

    QVERIFY(mediaDelegateQml.contains(QStringLiteral("required property var documentSession")));
    QVERIFY(mediaDelegateQml.contains(
        QStringLiteral("property ImageViewportInteractionSurface imageInteractionSurface")));
    QVERIFY(mediaDelegateQml.contains(QStringLiteral("signal viewerClicked")));
    QVERIFY(mediaDelegateQml.contains(QStringLiteral("signal viewerContextMenuRequested")));
    QVERIFY(mediaDelegateQml.contains(QStringLiteral("function requestViewportFocus()")));
    QVERIFY(mediaDelegateQml.contains(QStringLiteral("acceptedButtons: Qt.RightButton")));

    QVERIFY(interactionSurfaceQml.contains(QStringLiteral("function panBy")));
    QVERIFY(interactionSurfaceQml.contains(QStringLiteral("function scanForward")));
    QVERIFY(interactionSurfaceQml.contains(QStringLiteral("function zoomByStepAtCenter")));

    QVERIFY(imageViewportQml.contains(QStringLiteral("MediaViewportDelegate {")));
    QVERIFY(imageViewportQml.contains(
        QStringLiteral("imageInteractionSurface: ImageViewportInteractionSurface")));
    QVERIFY(videoViewportQml.contains(QStringLiteral("MediaViewportDelegate {")));
    QVERIFY(videoViewportQml.contains(
        QStringLiteral("readonly property var videoDocument: root.documentSession.videoDocument")));
    QVERIFY(!videoViewportQml.contains(QStringLiteral("property bool active: true")));
    QVERIFY(videoViewportQml.contains(QStringLiteral("root.presentationActive && root.visible")));
}

void TestMainWindowVideoIntegration::toolbarPageNavigationUsesSessionActiveProjection()
{
    const QString mainQml = readSource(QStringLiteral("src/qml/Main.qml"));
    const QString imageToolBarQml = readSource(QStringLiteral("src/qml/ImageToolBar.qml"));
    QVERIFY2(!mainQml.isEmpty(), "Main.qml should be readable");
    QVERIFY2(!imageToolBarQml.isEmpty(), "ImageToolBar.qml should be readable");

    QVERIFY(mainQml.contains(
        QStringLiteral("activeNavigationAvailable: documentSession.activeNavigationAvailable")));
    QVERIFY(mainQml.contains(
        QStringLiteral("activeNavigationKnown: documentSession.activeNavigationKnown")));
    QVERIFY(mainQml.contains(
        QStringLiteral("activeNavigationEditable: documentSession.activeNavigationEditable")));
    QVERIFY(mainQml.contains(QStringLiteral(
        "activeNavigationCurrentNumber: documentSession.activeNavigationCurrentNumber")));
    QVERIFY(mainQml.contains(
        QStringLiteral("activeNavigationCount: documentSession.activeNavigationCount")));
    QVERIFY(
        mainQml.contains(QStringLiteral("documentSession.openActiveNavigationAtNumber(number)")));
    QVERIFY(!mainQml.contains(QStringLiteral("imageDocument.openImageAtPage(number)")));
    QVERIFY(!mainQml.contains(QStringLiteral("page.imageDocument.openImageAtPage(number)")));

    verifySourceOmits(mainQml,
        {
            QStringLiteral("currentMediaNumber: documentSession.currentMediaNumber"),
            QStringLiteral("mediaCount: documentSession.mediaCount"),
            QStringLiteral(
                "directMediaNavigationKnown: documentSession.directMediaNavigationKnown"),
            QStringLiteral("activeNavigationAvailable: actionAvailability"),
            QStringLiteral("activeNavigationKnown: actionAvailability"),
            QStringLiteral("activeNavigationEditable: actionAvailability"),
            QStringLiteral("activeNavigationCurrentNumber: page.imageDocument.currentPageNumber"),
            QStringLiteral("activeNavigationCount: page.imageDocument.pageCount"),
        });

    verifySourceOmits(imageToolBarQml,
        {
            QStringLiteral("property bool directMediaNavigationActive"),
            QStringLiteral("property bool directMediaNavigationKnown"),
            QStringLiteral("property int currentMediaNumber"),
            QStringLiteral("property int mediaCount"),
            QStringLiteral("ImageActionAvailability"),
            QStringLiteral("imageDocument.currentPageNumber"),
            QStringLiteral("imageDocument.pageCount"),
            QStringLiteral("imageDocument.openImageAtPage"),
        });
    QVERIFY(imageToolBarQml.contains(
        QStringLiteral("activeNavigationCurrentNumber: root.activeNavigationCurrentNumber")));
    QVERIFY(imageToolBarQml.contains(
        QStringLiteral("activeNavigationCount: root.activeNavigationCount")));
    QVERIFY(imageToolBarQml.contains(
        QStringLiteral("openActiveNavigationAtNumber: root.openActiveNavigationAtNumber")));
}

void TestMainWindowVideoIntegration::thumbnailPanelUsesSessionThumbnailModel()
{
    const QString mainQml = readSource(QStringLiteral("src/qml/Main.qml"));
    const QString mediaWorkspaceHostQml
        = readSource(QStringLiteral("src/qml/MediaWorkspaceHost.qml"));
    const QString thumbnailPanelQml = readSource(QStringLiteral("src/qml/ThumbnailPanel.qml"));
    QVERIFY2(!mainQml.isEmpty(), "Main.qml should be readable");
    QVERIFY2(!mediaWorkspaceHostQml.isEmpty(), "MediaWorkspaceHost.qml should be readable");
    QVERIFY2(!thumbnailPanelQml.isEmpty(), "ThumbnailPanel.qml should be readable");

    QVERIFY(mainQml.contains(QStringLiteral("readonly property color darkForegroundColor")));
    QVERIFY(
        mainQml.contains(QStringLiteral("viewerSurfaceColor: imageViewTheme.darkBackgroundColor")));
    QVERIFY(mainQml.contains(
        QStringLiteral("viewerForegroundColor: imageViewTheme.darkForegroundColor")));
    QVERIFY(mediaWorkspaceHostQml.contains(
        QStringLiteral("required property color viewerSurfaceColor")));
    QVERIFY(mediaWorkspaceHostQml.contains(
        QStringLiteral("required property color viewerForegroundColor")));
    QVERIFY(mediaWorkspaceHostQml.contains(
        QStringLiteral("viewerSurfaceColor: root.viewerSurfaceColor")));
    QVERIFY(mediaWorkspaceHostQml.contains(
        QStringLiteral("viewerForegroundColor: root.viewerForegroundColor")));
    QVERIFY(mediaWorkspaceHostQml.contains(
        QStringLiteral("Controls.SplitView.maximumHeight: Kirigami.Units.gridUnit * 7.5")));
    QVERIFY(mediaWorkspaceHostQml.contains(
        QStringLiteral("Controls.SplitView.minimumHeight: Kirigami.Units.gridUnit * 6")));
    QVERIFY(mediaWorkspaceHostQml.contains(QStringLiteral("Math.max(Kirigami.Units.gridUnit * 6")));
    QVERIFY(thumbnailPanelQml.contains(
        QStringLiteral("required property KiriDocumentSession documentSession")));
    QVERIFY(
        thumbnailPanelQml.contains(QStringLiteral("required property color viewerSurfaceColor")));
    QVERIFY(thumbnailPanelQml.contains(
        QStringLiteral("required property color viewerForegroundColor")));
    QVERIFY(thumbnailPanelQml.contains(QStringLiteral("color: root.viewerSurfaceColor")));
    QVERIFY(thumbnailPanelQml.contains(QStringLiteral("color: root.viewerForegroundColor")));
    QVERIFY(thumbnailPanelQml.contains(QStringLiteral("opacity: 0.18")));
    QVERIFY(thumbnailPanelQml.contains(
        QStringLiteral("model: root.documentSession.activeNavigationThumbnailModel")));
    QVERIFY(thumbnailPanelQml.contains(
        QStringLiteral("currentIndex: root.documentSession.activeNavigationCurrentNumber - 1")));
    QVERIFY(thumbnailPanelQml.contains(
        QStringLiteral("positionViewAtIndex(currentIndex, ListView.Contain)")));
    QVERIFY(thumbnailPanelQml.contains(QStringLiteral("onCountChanged: containCurrentItem(true)")));
    QVERIFY(thumbnailPanelQml.contains(
        QStringLiteral("onCurrentIndexChanged: "
                       "containCurrentItemForNavigationIntent(currentIndexChangedRapidly())")));
    QVERIFY(thumbnailPanelQml.contains(
        QStringLiteral("root.documentSession.activeNavigationRevealIntent")));
    QVERIFY(thumbnailPanelQml.contains(QStringLiteral("KiriDocumentSession.AdjacentNavigation")));
    QVERIFY(thumbnailPanelQml.contains(QStringLiteral("KiriDocumentSession.LargeJump")));
    QVERIFY(thumbnailPanelQml.contains(QStringLiteral("KiriDocumentSession.LoadOrOpen")));
    QVERIFY(thumbnailPanelQml.contains(QStringLiteral("onVisibleChanged:")));
    QVERIFY(thumbnailPanelQml.contains(QStringLiteral("Behavior on contentX")));
    QVERIFY(thumbnailPanelQml.contains(QStringLiteral("NumberAnimation")));
    QVERIFY(thumbnailPanelQml.contains(QStringLiteral("duration: 140")));
    QVERIFY(thumbnailPanelQml.contains(QStringLiteral("easing.type: Easing.OutCubic")));
    QVERIFY(thumbnailPanelQml.contains(QStringLiteral("rapidCurrentIndexIntervalMs: 180")));
    QVERIFY(thumbnailPanelQml.contains(QStringLiteral("Math.abs(delta) <= nearThreshold")));
    QVERIFY(thumbnailPanelQml.contains(QStringLiteral("orientation: ListView.Horizontal")));
    QVERIFY(thumbnailPanelQml.contains(
        QStringLiteral("Controls.ScrollBar.horizontal: Controls.ScrollBar")));
    QVERIFY(thumbnailPanelQml.contains(QStringLiteral("objectName: \"thumbnailStrip\"")));
    QVERIFY(thumbnailPanelQml.contains(QStringLiteral("objectName: \"thumbnailStripItem\"")));
    QVERIFY(thumbnailPanelQml.contains(
        QStringLiteral("readonly property real delegateWidth: Kirigami.Units.gridUnit * 6")));
    QVERIFY(thumbnailPanelQml.contains(
        QStringLiteral("readonly property real itemPitch: delegateWidth + spacing")));
    QVERIFY(thumbnailPanelQml.contains(QStringLiteral("width: thumbnailStrip.delegateWidth")));
    QVERIFY(thumbnailPanelQml.contains(QStringLiteral("Kirigami.Units.iconSizes.large")));
    QVERIFY(thumbnailPanelQml.contains(QStringLiteral("font: Kirigami.Theme.fixedWidthFont")));
    QVERIFY(thumbnailPanelQml.contains(QStringLiteral("maximumLineCount: 1")));
    QVERIFY(thumbnailPanelQml.contains(QStringLiteral("wrapMode: Text.NoWrap")));
    QVERIFY(thumbnailPanelQml.contains(QStringLiteral("elide: Text.ElideRight")));
    QVERIFY(thumbnailPanelQml.contains(QStringLiteral("required property int number")));
    QVERIFY(thumbnailPanelQml.contains(QStringLiteral("required property string label")));
    QVERIFY(thumbnailPanelQml.contains(QStringLiteral("required property string iconName")));
    QVERIFY(thumbnailPanelQml.contains(QStringLiteral("required property bool current")));
    QVERIFY(thumbnailPanelQml.contains(
        QStringLiteral("border.color: thumbnailDelegate.current ? Kirigami.Theme.highlightColor")));
    QVERIFY(thumbnailPanelQml.contains(
        QStringLiteral("border.width: thumbnailDelegate.current ? 2 : 0")));
    QVERIFY(thumbnailPanelQml.contains(
        QStringLiteral("root.documentSession.openActiveNavigationThumbnailAtNumber(number)")));
    QVERIFY(!thumbnailPanelQml.contains(QStringLiteral("imageDocument.openImageAtPage")));
    QVERIFY(!thumbnailPanelQml.contains(QStringLiteral("openNextMedia")));
    QVERIFY(!thumbnailPanelQml.contains(QStringLiteral("openPreviousMedia")));
    QVERIFY(!thumbnailPanelQml.contains(QStringLiteral("Kirigami.Theme.backgroundColor")));
    QVERIFY(!thumbnailPanelQml.contains(QStringLiteral("Kirigami.ShadowedRectangle")));
    QVERIFY(!thumbnailPanelQml.contains(QStringLiteral("SmoothedAnimation")));
    QVERIFY(!thumbnailPanelQml.contains(QStringLiteral("onPositionChanged")));
    QVERIFY(!thumbnailPanelQml.contains(QStringLiteral("contentX =")));
    QVERIFY(!thumbnailPanelQml.contains(QStringLiteral("WheelHandler")));
}

void TestMainWindowVideoIntegration::infoPanelUsesSessionMediaInformationAndResponsiveLayouts()
{
    const QString mainQml = readSource(QStringLiteral("src/qml/Main.qml"));
    const QString mediaWorkspaceHostQml
        = readSource(QStringLiteral("src/qml/MediaWorkspaceHost.qml"));
    const QString infoPanelQml = readSource(QStringLiteral("src/qml/InfoPanel.qml"));
    QVERIFY2(!mainQml.isEmpty(), "Main.qml should be readable");
    QVERIFY2(!mediaWorkspaceHostQml.isEmpty(), "MediaWorkspaceHost.qml should be readable");
    QVERIFY2(!infoPanelQml.isEmpty(), "InfoPanel.qml should be readable");

    QVERIFY(mediaWorkspaceHostQml.contains(QStringLiteral("property bool infoPanelOpen")));
    QVERIFY(mediaWorkspaceHostQml.contains(
        QStringLiteral("readonly property real infoPanelWideBreakpoint")));
    QVERIFY(mediaWorkspaceHostQml.contains(QStringLiteral("Kirigami.Units.gridUnit * 42")));
    QVERIFY(mediaWorkspaceHostQml.contains(QStringLiteral("Kirigami.Units.gridUnit * 16")));
    QVERIFY(mediaWorkspaceHostQml.contains(QStringLiteral("Kirigami.Units.gridUnit * 18")));
    QVERIFY(mediaWorkspaceHostQml.contains(QStringLiteral("Kirigami.Units.gridUnit * 20")));
    QVERIFY(mediaWorkspaceHostQml.contains(
        QStringLiteral("root.infoPanelOpen && root.infoPanelInlineMode")));
    QVERIFY(mediaWorkspaceHostQml.contains(QStringLiteral("Kirigami.OverlayDrawer")));
    QVERIFY(
        mediaWorkspaceHostQml.contains(QStringLiteral("objectName: \"infoPanelOverlayDrawer\"")));
    QVERIFY(mediaWorkspaceHostQml.contains(QStringLiteral("edge: Qt.RightEdge")));
    QVERIFY(mediaWorkspaceHostQml.contains(QStringLiteral("Controls.Popup.CloseOnEscape")));
    QVERIFY(mediaWorkspaceHostQml.contains(QStringLiteral("Controls.Popup.CloseOnReleaseOutside")));
    QVERIFY(
        mediaWorkspaceHostQml.contains(QStringLiteral("documentSession: root.documentSession")));
    QVERIFY(mediaWorkspaceHostQml.contains(QStringLiteral("function closeInfoPanel()")));

    QVERIFY(infoPanelQml.contains(QStringLiteral(
        "readonly property var mediaInformation: documentSession.mediaInformation")));
    QVERIFY(infoPanelQml.contains(QStringLiteral("Controls.ScrollView")));
    QVERIFY(infoPanelQml.contains(QStringLiteral("objectName: \"infoPanelScrollView\"")));
    QVERIFY(infoPanelQml.contains(QStringLiteral("model: sectionRoot.rowModel")));
    QVERIFY(infoPanelQml.contains(QStringLiteral("root.mediaInformation.generalRows")));
    QVERIFY(infoPanelQml.contains(QStringLiteral("root.mediaInformation.mediaRows")));
    QVERIFY(infoPanelQml.contains(QStringLiteral("root.mediaInformation.cameraRows")));
    QVERIFY(infoPanelQml.contains(QStringLiteral("root.mediaInformation.advancedRows")));
    QVERIFY(infoPanelQml.contains(QStringLiteral("root.mediaInformation.hasAdvancedSection")));
    QVERIFY(infoPanelQml.contains(QStringLiteral("initiallyExpanded: false")));
    QVERIFY(infoPanelQml.contains(QStringLiteral("root.mediaInformation.copyFilePath()")));
    QVERIFY(infoPanelQml.contains(QStringLiteral("root.mediaInformation.openContainingFolder()")));
    QVERIFY(!infoPanelQml.contains(QStringLiteral("placeholder metadata")));
    QVERIFY(infoPanelQml.contains(QStringLiteral("elide: Text.ElideRight")));
    QVERIFY(infoPanelQml.contains(QStringLiteral("elide: Text.ElideMiddle")));
    QVERIFY(!infoPanelQml.contains(QStringLiteral("#")));
    QVERIFY(!infoPanelQml.contains(QStringLiteral("Qt.rgba")));

    QVERIFY(mainQml.contains(QStringLiteral("mediaWorkspaceHost.closeInfoPanel()")));
    QVERIFY(mainQml.contains(QStringLiteral("!mediaWorkspaceHost.infoPanelVisible")));
}

void TestMainWindowVideoIntegration::activeNavigationActionsUseSessionSnapshotAndBoundaryScope()
{
    const QString mainQml = readSource(QStringLiteral("src/qml/Main.qml"));
    const QString imageActionsQml = readSource(QStringLiteral("src/qml/ImageActions.qml"));
    QVERIFY2(!mainQml.isEmpty(), "Main.qml should be readable");
    QVERIFY2(!imageActionsQml.isEmpty(), "ImageActions.qml should be readable");

    QVERIFY(imageActionsQml.contains(QStringLiteral("canOpenPreviousActiveNavigation")));
    QVERIFY(imageActionsQml.contains(QStringLiteral("canOpenNextActiveNavigation")));
    QVERIFY(
        imageActionsQml.contains(QStringLiteral("requestPreviousActiveNavigationBoundaryText")));
    QVERIFY(imageActionsQml.contains(QStringLiteral("requestNextActiveNavigationBoundaryText")));
    QVERIFY(imageActionsQml.contains(QStringLiteral("activeNavigationBoundaryScope")));
    QVERIFY(imageActionsQml.contains(
        QStringLiteral("KiriDocumentSession.DirectMediaNavigationBoundary")));

    verifySourceOmits(imageActionsQml,
        {
            QStringLiteral("FirstActiveNavigationBoundary"),
            QStringLiteral("LastActiveNavigationBoundary"),
            QStringLiteral("First media item"),
            QStringLiteral("Last media item"),
            QStringLiteral("First image"),
            QStringLiteral("Last image"),
            QStringLiteral("directMediaNavigationActive"),
            QStringLiteral("directMediaNavigationKnown"),
            QStringLiteral("currentMediaNumber"),
            QStringLiteral("mediaCount"),
            QStringLiteral("openMediaAtNumber"),
            QStringLiteral("openNextMedia"),
            QStringLiteral("openPreviousMedia"),
            QStringLiteral("currentPageNumber"),
            QStringLiteral("currentLastPageNumber"),
            QStringLiteral("pageCount"),
            QStringLiteral("canOpenPreviousImage"),
            QStringLiteral("canOpenNextImage"),
            QStringLiteral("atKnownFirstImage"),
            QStringLiteral("atKnownLastImage"),
            QStringLiteral("atKnownFirstActiveNavigation"),
            QStringLiteral("atKnownLastActiveNavigation"),
            QStringLiteral("canUsePageActions"),
            QStringLiteral("pageShortcutsEnabled"),
            QStringLiteral("openImageAtPage"),
        });
    QVERIFY(!mainQml.contains(QStringLiteral(
        "directMediaNavigationActive: documentSession.directMediaNavigationActive")));
}

void TestMainWindowVideoIntegration::shortcutsRouteSharedActiveNavigationThroughSessionRequests()
{
    const QString mainQml = readSource(QStringLiteral("src/qml/Main.qml"));
    const QString imageShortcutsQml = readSource(QStringLiteral("src/qml/ImageShortcuts.qml"));
    QVERIFY2(!mainQml.isEmpty(), "Main.qml should be readable");
    QVERIFY2(!imageShortcutsQml.isEmpty(), "ImageShortcuts.qml should be readable");

    QVERIFY(mainQml.contains(QStringLiteral("documentSession: documentSession")));
    QVERIFY(!mainQml.contains(QStringLiteral("videoDirectMediaNavigationActive:")));

    QVERIFY(imageShortcutsQml.contains(
        QStringLiteral("required property KiriDocumentSession documentSession")));
    QVERIFY(imageShortcutsQml.contains(
        QStringLiteral("root.documentSession.requestPreviousActiveNavigationBoundaryText()")));
    QVERIFY(imageShortcutsQml.contains(
        QStringLiteral("root.documentSession.requestNextActiveNavigationBoundaryText()")));
    QVERIFY(imageShortcutsQml.contains(QStringLiteral("mediaShortcutsEnabledForScope")));
    verifySourceOmits(imageShortcutsQml,
        {
            QStringLiteral("activeNavigationShortcutsEnabledForScope"),
            QStringLiteral("videoShortcutsEnabledForScope"),
            QStringLiteral("videoDirectMediaNavigationActive"),
            QStringLiteral("directMediaNavigationActive"),
            QStringLiteral("directMediaNavigationKnown"),
            QStringLiteral("currentMediaNumber"),
            QStringLiteral("mediaCount"),
            QStringLiteral("openMediaAtNumber"),
            QStringLiteral("openNextMedia"),
            QStringLiteral("openPreviousMedia"),
            QStringLiteral("openPreviousPage()"),
            QStringLiteral("openNextPage()"),
            QStringLiteral("openImageAtPage"),
            QStringLiteral("imageDocument.currentPageNumber"),
            QStringLiteral("actionAvailability.scanBackwardAtFirstImageBoundary"),
        });
    QVERIFY(imageShortcutsQml.contains(
        QStringLiteral("root.documentSession.activeNavigationBoundaryScope")));
    QVERIFY(imageShortcutsQml.contains(
        QStringLiteral("KiriDocumentSession.ImageDocumentPageNavigationBoundary")));
    QVERIFY(imageShortcutsQml.contains(
        QStringLiteral("root.documentSession.atKnownFirstActiveNavigation")));
    QVERIFY(imageShortcutsQml.contains(
        QStringLiteral("root.documentSession.canOpenPreviousActiveNavigation")));
}

void TestMainWindowVideoIntegration::imageActionAvailabilityDoesNotDriveSharedActiveNavigation()
{
    const QString mainQml = readSource(QStringLiteral("src/qml/Main.qml"));
    const QString imageActionsQml = readSource(QStringLiteral("src/qml/ImageActions.qml"));
    const QString imageShortcutsQml = readSource(QStringLiteral("src/qml/ImageShortcuts.qml"));
    QVERIFY2(!mainQml.isEmpty(), "Main.qml should be readable");
    QVERIFY2(!imageActionsQml.isEmpty(), "ImageActions.qml should be readable");
    QVERIFY2(!imageShortcutsQml.isEmpty(), "ImageShortcuts.qml should be readable");

    verifySourceOmits(mainQml,
        {
            QStringLiteral("activeNavigationAvailable: actionAvailability"),
            QStringLiteral("activeNavigationKnown: actionAvailability"),
            QStringLiteral("activeNavigationEditable: actionAvailability"),
            QStringLiteral("activeNavigationCurrentNumber: actionAvailability"),
            QStringLiteral("activeNavigationCount: actionAvailability"),
            QStringLiteral("currentLastPageNumber: page.imageDocument"),
            QStringLiteral("currentPageNumber: page.imageDocument"),
            QStringLiteral("pageCount: page.imageDocument"),
            QStringLiteral("imageHorizontallyPannable:"),
            QStringLiteral("scanBackwardAtFirstImageBoundary:"),
        });
    verifySourceOmits(imageActionsQml,
        {
            QStringLiteral("actionAvailability.canOpenPreviousImage"),
            QStringLiteral("actionAvailability.canOpenNextImage"),
            QStringLiteral("actionAvailability.atKnownFirstImage"),
            QStringLiteral("actionAvailability.atKnownLastImage"),
            QStringLiteral("actionAvailability.canUsePageActions"),
            QStringLiteral("actionAvailability.pageShortcutsEnabled"),
            QStringLiteral("actionAvailability.imageHorizontallyPannable"),
        });
    QVERIFY(imageShortcutsQml.contains(QStringLiteral("mediaShortcutsEnabledForScope")));
    QVERIFY(
        !imageShortcutsQml.contains(QStringLiteral("activeNavigationShortcutsEnabledForScope")));
    QVERIFY(!imageShortcutsQml.contains(
        QStringLiteral("actionAvailability.scanBackwardAtFirstImageBoundary")));
    QVERIFY(!imageShortcutsQml.contains(
        QStringLiteral("actionAvailability.imageHorizontallyPannable")));
    QVERIFY(imageShortcutsQml.contains(
        QStringLiteral("root.imageInteractionSurface.imageHorizontallyPannable")));
    QVERIFY(!imageShortcutsQml.contains(QStringLiteral("imageDocument.currentPageNumber")));
}

void TestMainWindowVideoIntegration::
    pageNavigationComponentDoesNotChooseBetweenRawNavigationSources()
{
    const QString pageNavigationQml
        = readSource(QStringLiteral("src/qml/ImageDocumentPageNavigation.qml"));
    QVERIFY2(!pageNavigationQml.isEmpty(), "ImageDocumentPageNavigation.qml should be readable");

    verifySourceOmits(pageNavigationQml,
        {
            QStringLiteral("required property KiriImageDocument"),
            QStringLiteral("directMediaNavigationActive"),
            QStringLiteral("directMediaNavigationKnown"),
            QStringLiteral("currentMediaNumber"),
            QStringLiteral("mediaCount"),
            QStringLiteral("openMediaAtNumber"),
            QStringLiteral("openNextMedia"),
            QStringLiteral("openPreviousMedia"),
            QStringLiteral("imageDocument.currentPageNumber"),
            QStringLiteral("imageDocument.pageCount"),
            QStringLiteral("openImageAtPage"),
        });
    QVERIFY(pageNavigationQml.contains(QStringLiteral("activeNavigationCurrentNumber")));
    QVERIFY(pageNavigationQml.contains(QStringLiteral("activeNavigationCount")));
    QVERIFY(pageNavigationQml.contains(QStringLiteral("activeNavigationEditable")));
    QVERIFY(pageNavigationQml.contains(QStringLiteral("openActiveNavigationAtNumber")));
}

void TestMainWindowVideoIntegration::documentSessionFacadeDoesNotExposeRawDirectMediaNavigation()
{
    const QString documentSessionHeader
        = readSource(QStringLiteral("src/facade/kiridocumentsession.h"));
    QVERIFY2(!documentSessionHeader.isEmpty(), "kiridocumentsession.h should be readable");

    verifySourceOmits(documentSessionHeader,
        {
            QStringLiteral("directMediaNavigationActive"),
            QStringLiteral("directMediaNavigationKnown"),
            QStringLiteral("currentMediaNumber"),
            QStringLiteral("mediaCount"),
            QStringLiteral("canOpenPreviousMedia"),
            QStringLiteral("canOpenNextMedia"),
            QStringLiteral("atKnownFirstMedia"),
            QStringLiteral("atKnownLastMedia"),
            QStringLiteral("openPreviousMedia"),
            QStringLiteral("openNextMedia"),
            QStringLiteral("openMediaAtNumber"),
            QStringLiteral("directMediaNavigationAvailabilityChanged"),
        });
}

void TestMainWindowVideoIntegration::
    documentSessionRuntimePublicApiDoesNotExposeRawDirectMediaNavigation()
{
    const QString documentSessionRuntimeHeader
        = readSource(QStringLiteral("src/session/documentsessionruntime.h"));
    QVERIFY2(
        !documentSessionRuntimeHeader.isEmpty(), "documentsessionruntime.h should be readable");

    const QString publicSection = firstPublicSection(documentSessionRuntimeHeader);
    QVERIFY2(!publicSection.isEmpty(), "DocumentSessionRuntime public API should be detectable");
    QVERIFY(publicSection.contains(QStringLiteral("activeNavigationAvailable")));
    QVERIFY(publicSection.contains(QStringLiteral("openPreviousActiveNavigation")));

    verifySourceOmits(publicSection,
        {
            QStringLiteral("directMediaNavigationActive"),
            QStringLiteral("directMediaNavigationKnown"),
            QStringLiteral("currentMediaNumber"),
            QStringLiteral("mediaCount"),
            QStringLiteral("canOpenPreviousMedia"),
            QStringLiteral("canOpenNextMedia"),
            QStringLiteral("atKnownFirstMedia"),
            QStringLiteral("atKnownLastMedia"),
            QStringLiteral("openPreviousMedia"),
            QStringLiteral("openNextMedia"),
            QStringLiteral("openMediaAtNumber"),
        });
}

void TestMainWindowVideoIntegration::videoModeExposesReadOnlyZoomReadout()
{
    const QString mainQml = readSource(QStringLiteral("src/qml/Main.qml"));
    const QString videoViewportQml = readSource(QStringLiteral("src/qml/VideoViewport.qml"));
    const QString imageToolBarQml = readSource(QStringLiteral("src/qml/ImageToolBar.qml"));
    QVERIFY2(!mainQml.isEmpty(), "Main.qml should be readable");
    QVERIFY2(!videoViewportQml.isEmpty(), "VideoViewport.qml should be readable");
    QVERIFY2(!imageToolBarQml.isEmpty(), "ImageToolBar.qml should be readable");

    QVERIFY(!videoViewportQml.contains(QStringLiteral("videoZoomPercentForRects")));
    QVERIFY(
        videoViewportQml.contains(QStringLiteral("endOfStreamPolicy: VideoOutput.KeepLastFrame")));
    QVERIFY(videoViewportQml.contains(QStringLiteral("videoOutput.contentRect")));
    QVERIFY(videoViewportQml.contains(QStringLiteral("videoOutput.sourceRect")));
    QVERIFY(videoViewportQml.contains(QStringLiteral("setVideoOutputGeometry")));
    QVERIFY(!videoViewportQml.contains(QStringLiteral("displayDevicePixelRatio")));
    QVERIFY(!videoViewportQml.contains(QStringLiteral("devicePixelRatio")));
    QVERIFY(!mainQml.contains(QStringLiteral("videoDocument.zoomPercentKnown")));
    QVERIFY(!mainQml.contains(QStringLiteral("videoDocument.zoomPercent")));
    QVERIFY(mainQml.contains(QStringLiteral("documentSession.activeZoomPercentAvailable")));
    QVERIFY(mainQml.contains(QStringLiteral("documentSession.activeZoomPercentKnown")));
    QVERIFY(mainQml.contains(QStringLiteral("documentSession.activeZoomPercent")));
    QVERIFY(mainQml.contains(QStringLiteral("documentSession.activeZoomEditable")));
    QVERIFY(!mainQml.contains(QStringLiteral("videoViewportLoader.zoomPercentKnown")));
    QVERIFY(!mainQml.contains(QStringLiteral("videoViewportLoader.zoomPercent")));
    QVERIFY(!imageToolBarQml.contains(QStringLiteral("videoZoomLevelAction")));
    QVERIFY(!imageToolBarQml.contains(QStringLiteral("videoToolbarControls")));
    QVERIFY(!imageToolBarQml.contains(QStringLiteral("showVideoZoomReadout")));
    QVERIFY(!imageToolBarQml.contains(QStringLiteral("showImageControls")));
    QVERIFY(imageToolBarQml.contains(
        QStringLiteral("readonly property var toolbarControls: imageToolbarControls")));
    QVERIFY(imageToolBarQml.contains(QStringLiteral("readOnlyDisplayMode: root.videoMode")));
    QVERIFY(
        imageToolBarQml.contains(QStringLiteral("readOnlyPercent: Math.round(root.zoomPercent)")));
    QVERIFY(
        imageToolBarQml.contains(QStringLiteral("readOnlyPercentKnown: root.zoomPercentKnown")));
    QVERIFY(imageToolBarQml.contains(QStringLiteral("zoomPercent: root.zoomPercent")));
    QVERIFY(imageToolBarQml.contains(
        QStringLiteral("zoomPercentAvailable: root.zoomPercentAvailable")));
    QVERIFY(imageToolBarQml.contains(QStringLiteral("zoomPercentKnown: root.zoomPercentKnown")));
    QVERIFY(mainQml.contains(QStringLiteral("zoomPercent: documentSession.activeZoomPercent")));
    QVERIFY(mainQml.contains(
        QStringLiteral("zoomPercentAvailable: documentSession.activeZoomPercentAvailable")));
    QVERIFY(mainQml.contains(
        QStringLiteral("zoomPercentKnown: documentSession.activeZoomPercentKnown")));
    QVERIFY(!mainQml.contains(QStringLiteral("videoZoomPercent:")));
    QVERIFY(!mainQml.contains(QStringLiteral("videoZoomReady:")));
}

void TestMainWindowVideoIntegration::videoPlaybackControlsUseResponsiveFloatingAndFixedModes()
{
    const QString videoControlsQml
        = readSource(QStringLiteral("src/qml/VideoFloatingControls.qml"));
    const QString videoViewportQml = readSource(QStringLiteral("src/qml/VideoViewport.qml"));
    QVERIFY2(!videoControlsQml.isEmpty(), "VideoFloatingControls.qml should be readable");
    QVERIFY2(!videoViewportQml.isEmpty(), "VideoViewport.qml should be readable");

    QVERIFY(videoControlsQml.contains(QStringLiteral("objectName: \"videoPlaybackControls\"")));
    QVERIFY(
        videoControlsQml.contains(QStringLiteral("objectName: \"videoPlaybackPlayPauseButton\"")));
    QVERIFY(
        videoControlsQml.contains(QStringLiteral("objectName: \"videoPlaybackCurrentTimeLabel\"")));
    QVERIFY(videoControlsQml.contains(QStringLiteral("objectName: \"videoPlaybackSlider\"")));
    QVERIFY(
        videoControlsQml.contains(QStringLiteral("objectName: \"videoPlaybackDurationLabel\"")));
    QVERIFY(videoControlsQml.contains(QStringLiteral("objectName: \"videoPlaybackMuteButton\"")));
    QVERIFY(videoControlsQml.contains(QStringLiteral("root.videoDocument.toggleMuted()")));
    QVERIFY(videoControlsQml.contains(QStringLiteral("font: Kirigami.Theme.fixedWidthFont")));
    QVERIFY(videoControlsQml.contains(QStringLiteral("display: Controls.AbstractButton.IconOnly")));
    QVERIFY(videoControlsQml.contains(QStringLiteral("parent.width * 0.75")));
    QVERIFY(videoControlsQml.contains(QStringLiteral("parent.width - horizontalViewportMargin")));
    QVERIFY(
        videoControlsQml.contains(QStringLiteral("Math.min(availableResponsiveWidth, Math.max")));
    QVERIFY(videoControlsQml.contains(QStringLiteral("Kirigami.ShadowedRectangle")));
    QVERIFY(videoControlsQml.contains(QStringLiteral("Kirigami.Theme.backgroundColor")));
    QVERIFY(videoControlsQml.contains(QStringLiteral("Kirigami.Units.cornerRadius")));
    QVERIFY(videoControlsQml.contains(QStringLiteral("shadow.size: root.fixedMode ? 0")));
    QVERIFY(videoControlsQml.contains(QStringLiteral("interval: Kirigami.Units.humanMoment")));
    QVERIFY(videoControlsQml.contains(QStringLiteral("duration: Kirigami.Units.shortDuration")));
    QVERIFY(videoControlsQml.contains(QStringLiteral(
        "readonly property bool autoHideEligible: !fixedMode && videoDocument.playing")));
    QVERIFY(!videoControlsQml.contains(
        QStringLiteral("Math.min(parent.width - Kirigami.Units.largeSpacing * 2, implicitWidth)")));

    QVERIFY(videoViewportQml.contains(QStringLiteral("readonly property bool fixedControlsMode")));
    QVERIFY(videoViewportQml.contains(QStringLiteral("Kirigami.Settings.isMobile")));
    QVERIFY(videoViewportQml.contains(QStringLiteral("Kirigami.Settings.hasTransientTouchInput")));
    QVERIFY(videoViewportQml.contains(QStringLiteral("Kirigami.Units.longDuration <= 0")));
    QVERIFY(videoViewportQml.contains(QStringLiteral("width < Kirigami.Units.gridUnit * 32")));
    QVERIFY(videoViewportQml.contains(QStringLiteral("height < Kirigami.Units.gridUnit * 16")));
    QVERIFY(videoViewportQml.contains(QStringLiteral(
        "anchors.bottom: root.controlsReserveSpace ? floatingControls.top : parent.bottom")));
    QVERIFY(videoViewportQml.contains(
        QStringLiteral("anchors.bottomMargin: root.fixedControlsMode ? 0")));
    QVERIFY(videoViewportQml.contains(QStringLiteral("visible: root.videoControlsReady")));
    QVERIFY(videoViewportQml.contains(QStringLiteral("floatingControls.revealControls()")));
}

QTEST_GUILESS_MAIN(TestMainWindowVideoIntegration)

#include "test_mainwindowvideointegration.moc"
