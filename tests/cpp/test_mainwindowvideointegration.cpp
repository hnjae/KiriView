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
    void activeNavigationActionsUseSessionSnapshotAndBoundaryScope();
    void shortcutsRouteSharedActiveNavigationThroughSessionRequests();
    void imageActionAvailabilityDoesNotDriveSharedActiveNavigation();
    void pageNavigationComponentDoesNotChooseBetweenRawNavigationSources();
    void documentSessionFacadeDoesNotExposeRawMediaNavigation();
    void videoFloatingControlsUsesViewportResponsiveWidth();
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
            QStringLiteral("mediaNavigationKnown: documentSession.mediaNavigationKnown"),
            QStringLiteral("activeNavigationAvailable: actionAvailability"),
            QStringLiteral("activeNavigationKnown: actionAvailability"),
            QStringLiteral("activeNavigationEditable: actionAvailability"),
            QStringLiteral("activeNavigationCurrentNumber: page.imageDocument.currentPageNumber"),
            QStringLiteral("activeNavigationCount: page.imageDocument.imageCount"),
        });

    verifySourceOmits(imageToolBarQml,
        {
            QStringLiteral("property bool mediaNavigationActive"),
            QStringLiteral("property bool mediaNavigationKnown"),
            QStringLiteral("property int currentMediaNumber"),
            QStringLiteral("property int mediaCount"),
            QStringLiteral("ImageActionAvailability"),
            QStringLiteral("imageDocument.currentPageNumber"),
            QStringLiteral("imageDocument.imageCount"),
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
    const QString thumbnailPanelQml = readSource(QStringLiteral("src/qml/ThumbnailPanel.qml"));
    QVERIFY2(!thumbnailPanelQml.isEmpty(), "ThumbnailPanel.qml should be readable");

    QVERIFY(thumbnailPanelQml.contains(
        QStringLiteral("required property KiriDocumentSession documentSession")));
    QVERIFY(thumbnailPanelQml.contains(
        QStringLiteral("model: root.documentSession.activeNavigationThumbnailModel")));
    QVERIFY(thumbnailPanelQml.contains(QStringLiteral("orientation: ListView.Horizontal")));
    QVERIFY(thumbnailPanelQml.contains(QStringLiteral("objectName: \"thumbnailStrip\"")));
    QVERIFY(thumbnailPanelQml.contains(QStringLiteral("objectName: \"thumbnailStripItem\"")));
    QVERIFY(thumbnailPanelQml.contains(QStringLiteral("required property int number")));
    QVERIFY(thumbnailPanelQml.contains(QStringLiteral("required property string label")));
    QVERIFY(thumbnailPanelQml.contains(QStringLiteral("required property string iconName")));
    QVERIFY(thumbnailPanelQml.contains(QStringLiteral("required property bool current")));
    QVERIFY(thumbnailPanelQml.contains(
        QStringLiteral("root.documentSession.openActiveNavigationAtNumber(number)")));
    QVERIFY(!thumbnailPanelQml.contains(QStringLiteral("imageDocument.openImageAtPage")));
    QVERIFY(!thumbnailPanelQml.contains(QStringLiteral("openNextMedia")));
    QVERIFY(!thumbnailPanelQml.contains(QStringLiteral("openPreviousMedia")));
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
    QVERIFY(
        imageActionsQml.contains(QStringLiteral("KiriDocumentSession.MediaNavigationBoundary")));

    verifySourceOmits(imageActionsQml,
        {
            QStringLiteral("FirstActiveNavigationBoundary"),
            QStringLiteral("LastActiveNavigationBoundary"),
            QStringLiteral("First media item"),
            QStringLiteral("Last media item"),
            QStringLiteral("First image"),
            QStringLiteral("Last image"),
            QStringLiteral("mediaNavigationActive"),
            QStringLiteral("mediaNavigationKnown"),
            QStringLiteral("currentMediaNumber"),
            QStringLiteral("mediaCount"),
            QStringLiteral("openMediaAtNumber"),
            QStringLiteral("openNextMedia"),
            QStringLiteral("openPreviousMedia"),
            QStringLiteral("currentPageNumber"),
            QStringLiteral("currentLastPageNumber"),
            QStringLiteral("imageCount"),
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
    QVERIFY(!mainQml.contains(
        QStringLiteral("mediaNavigationActive: documentSession.mediaNavigationActive")));
}

void TestMainWindowVideoIntegration::shortcutsRouteSharedActiveNavigationThroughSessionRequests()
{
    const QString mainQml = readSource(QStringLiteral("src/qml/Main.qml"));
    const QString imageShortcutsQml = readSource(QStringLiteral("src/qml/ImageShortcuts.qml"));
    QVERIFY2(!mainQml.isEmpty(), "Main.qml should be readable");
    QVERIFY2(!imageShortcutsQml.isEmpty(), "ImageShortcuts.qml should be readable");

    QVERIFY(mainQml.contains(QStringLiteral("documentSession: documentSession")));
    QVERIFY(!mainQml.contains(QStringLiteral("videoMediaNavigationActive:")));

    QVERIFY(imageShortcutsQml.contains(
        QStringLiteral("required property KiriDocumentSession documentSession")));
    QVERIFY(imageShortcutsQml.contains(
        QStringLiteral("root.documentSession.requestPreviousActiveNavigationBoundaryText()")));
    QVERIFY(imageShortcutsQml.contains(
        QStringLiteral("root.documentSession.requestNextActiveNavigationBoundaryText()")));
    QVERIFY(imageShortcutsQml.contains(QStringLiteral("activeNavigationShortcutsEnabledForScope")));
    verifySourceOmits(imageShortcutsQml,
        {
            QStringLiteral("videoMediaNavigationActive"),
            QStringLiteral("mediaNavigationActive"),
            QStringLiteral("mediaNavigationKnown"),
            QStringLiteral("currentMediaNumber"),
            QStringLiteral("mediaCount"),
            QStringLiteral("openMediaAtNumber"),
            QStringLiteral("openNextMedia"),
            QStringLiteral("openPreviousMedia"),
            QStringLiteral("openPreviousImage()"),
            QStringLiteral("openNextImage()"),
        });

    const int activeNavigationOpenPageCalls
        = imageShortcutsQml.count(QStringLiteral("openImageAtPage"));
    QCOMPARE(activeNavigationOpenPageCalls, 1);
    QVERIFY(imageShortcutsQml.contains(QStringLiteral("Image-internal scan fallback")));
}

void TestMainWindowVideoIntegration::imageActionAvailabilityDoesNotDriveSharedActiveNavigation()
{
    const QString mainQml = readSource(QStringLiteral("src/qml/Main.qml"));
    const QString imageActionsQml = readSource(QStringLiteral("src/qml/ImageActions.qml"));
    const QString imageShortcutsQml = readSource(QStringLiteral("src/qml/ImageShortcuts.qml"));
    QVERIFY2(!mainQml.isEmpty(), "Main.qml should be readable");
    QVERIFY2(!imageActionsQml.isEmpty(), "ImageActions.qml should be readable");
    QVERIFY2(!imageShortcutsQml.isEmpty(), "ImageShortcuts.qml should be readable");

    QVERIFY(mainQml.contains(QStringLiteral("scanBackwardAtFirstImageBoundary")));
    verifySourceOmits(mainQml,
        {
            QStringLiteral("activeNavigationAvailable: actionAvailability"),
            QStringLiteral("activeNavigationKnown: actionAvailability"),
            QStringLiteral("activeNavigationEditable: actionAvailability"),
            QStringLiteral("activeNavigationCurrentNumber: actionAvailability"),
            QStringLiteral("activeNavigationCount: actionAvailability"),
            QStringLiteral("currentLastPageNumber: page.imageDocument"),
            QStringLiteral("currentPageNumber: page.imageDocument"),
            QStringLiteral("imageCount: page.imageDocument"),
        });
    verifySourceOmits(imageActionsQml,
        {
            QStringLiteral("actionAvailability.canOpenPreviousImage"),
            QStringLiteral("actionAvailability.canOpenNextImage"),
            QStringLiteral("actionAvailability.atKnownFirstImage"),
            QStringLiteral("actionAvailability.atKnownLastImage"),
            QStringLiteral("actionAvailability.canUsePageActions"),
            QStringLiteral("actionAvailability.pageShortcutsEnabled"),
        });
    QVERIFY(imageShortcutsQml.contains(QStringLiteral("activeNavigationShortcutsEnabledForScope")));
    QVERIFY(imageShortcutsQml.contains(QStringLiteral("Image-internal scan fallback")));
    QVERIFY(imageShortcutsQml.contains(
        QStringLiteral("actionAvailability.scanBackwardAtFirstImageBoundary")));
}

void TestMainWindowVideoIntegration::
    pageNavigationComponentDoesNotChooseBetweenRawNavigationSources()
{
    const QString pageNavigationQml = readSource(QStringLiteral("src/qml/ImagePageNavigation.qml"));
    QVERIFY2(!pageNavigationQml.isEmpty(), "ImagePageNavigation.qml should be readable");

    verifySourceOmits(pageNavigationQml,
        {
            QStringLiteral("required property KiriImageDocument"),
            QStringLiteral("mediaNavigationActive"),
            QStringLiteral("mediaNavigationKnown"),
            QStringLiteral("currentMediaNumber"),
            QStringLiteral("mediaCount"),
            QStringLiteral("openMediaAtNumber"),
            QStringLiteral("openNextMedia"),
            QStringLiteral("openPreviousMedia"),
            QStringLiteral("imageDocument.currentPageNumber"),
            QStringLiteral("imageDocument.imageCount"),
            QStringLiteral("openImageAtPage"),
        });
    QVERIFY(pageNavigationQml.contains(QStringLiteral("activeNavigationCurrentNumber")));
    QVERIFY(pageNavigationQml.contains(QStringLiteral("activeNavigationCount")));
    QVERIFY(pageNavigationQml.contains(QStringLiteral("activeNavigationEditable")));
    QVERIFY(pageNavigationQml.contains(QStringLiteral("openActiveNavigationAtNumber")));
}

void TestMainWindowVideoIntegration::documentSessionFacadeDoesNotExposeRawMediaNavigation()
{
    const QString documentSessionHeader
        = readSource(QStringLiteral("src/facade/kiridocumentsession.h"));
    QVERIFY2(!documentSessionHeader.isEmpty(), "kiridocumentsession.h should be readable");

    verifySourceOmits(documentSessionHeader,
        {
            QStringLiteral("mediaNavigationActive"),
            QStringLiteral("mediaNavigationKnown"),
            QStringLiteral("currentMediaNumber"),
            QStringLiteral("mediaCount"),
            QStringLiteral("canOpenPreviousMedia"),
            QStringLiteral("canOpenNextMedia"),
            QStringLiteral("atKnownFirstMedia"),
            QStringLiteral("atKnownLastMedia"),
            QStringLiteral("openPreviousMedia"),
            QStringLiteral("openNextMedia"),
            QStringLiteral("openMediaAtNumber"),
            QStringLiteral("mediaNavigationAvailabilityChanged"),
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

void TestMainWindowVideoIntegration::videoFloatingControlsUsesViewportResponsiveWidth()
{
    const QString videoControlsQml
        = readSource(QStringLiteral("src/qml/VideoFloatingControls.qml"));
    QVERIFY2(!videoControlsQml.isEmpty(), "VideoFloatingControls.qml should be readable");

    QVERIFY(videoControlsQml.contains(QStringLiteral("parent.width * 0.65")));
    QVERIFY(videoControlsQml.contains(QStringLiteral("Kirigami.Units.gridUnit * 24")));
    QVERIFY(videoControlsQml.contains(QStringLiteral("Kirigami.Units.gridUnit * 44")));
    QVERIFY(videoControlsQml.contains(QStringLiteral("parent.width - horizontalViewportMargin")));
    QVERIFY(videoControlsQml.contains(
        QStringLiteral("Math.min(availableResponsiveWidth, Math.max(minimumResponsiveWidth")));
    QVERIFY(!videoControlsQml.contains(
        QStringLiteral("Math.min(parent.width - Kirigami.Units.largeSpacing * 2, implicitWidth)")));
}

QTEST_GUILESS_MAIN(TestMainWindowVideoIntegration)

#include "test_mainwindowvideointegration.moc"
