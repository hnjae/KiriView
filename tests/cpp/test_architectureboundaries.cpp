// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "facade/kiriimagedocument.h"
#include "facade/kirivideodocument.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QMetaProperty>
#include <QRegularExpression>
#include <QStringList>
#include <QTest>

class TestArchitectureBoundaries : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void leafDocumentSourceRoutesAreReadOnlyPublicObservations();
    void leafDocumentRouteSettersStayPrivateToTheSession();
    void qmlCannotSetLeafDocumentRoutes();
    void qmlDoesNotWriteSharedActionState();
    void qmlActionProxiesDoNotOverrideRuntimeActionState();
    void qmlDoesNotOwnSharedActionPolicy();
    void qmlDoesNotComputePannabilityActionGate();
    void qmlDoesNotRecomputeSharedMediaReadiness();
    void qmlDoesNotWriteDurablePresentationState();
    void imageDocumentHasNoPublicPresentationBackdoorSetters();
    void qmlViewportUsesRevisionedCommandAcknowledgement();
    void qmlViewportCommandHandlersDoNotApplyProjectionDirectly();
    void qmlViewportUsesOpaqueRevisionTokens();
    void leafDocumentsAreNotProductionQmlCreatable();
    void actionUiGatesAreRevisionedSnapshots();
    void qmlDoesNotManufactureStaleSensitiveRevisions();
    void imageActionAvailabilityFacadeIsNotWritableQmlBackdoor();
    void fixedViewerShortcutsDoNotBypassRuntimeRouting();
    void qmlDoesNotExposeFixedViewerScanCommandRoutes();
    void videoSeekShortcutsRouteThroughApplicationRuntime();
    void applicationFacadeDoesNotOwnFixedViewerCommandRouting();
    void applicationCommandRouterPortsAreGroupedByOwner();
    void applicationFacadeDoesNotOwnActionCommandSwitch();
    void sessionPublicProjectionHasNoPartialUpdateBackdoor();
    void sessionPublicProjectionDoesNotSampleLeafFacadesWhileApplying();
    void qmlDoesNotWriteSharedVideoOutputAttachment();
    void videoOutputAttachmentIsNotWritablePublicVideoDocumentState();
    void qmlDoesNotDeriveSharedControlPolicyFromLeafDocuments();
    void qmlUsesCentralNavigationPresentationOrder();
    void qmlViewportUsesFullCommandLifecycle();
    void viewportContextBridgeIsNonRenderingPublicQtFacade();
    void qmlViewportUsesContextBridgeForRenderContextDiscovery();
    void oldImageRendererArtifactsAreAbsent();
    void oldImageRendererBuildWiringIsAbsent();
    void cppTestBuildConsumesCargoAppLibraryOnly();
    void productionImageDisplayUsesProviderPathOnly();
    void sourceKeysExposeTypedExtensionFamilies();
    void sourceKeysExposeOperationalExtensionContracts();
    void openedCollectionThumbnailEligibilityUsesSharedPolicy();
    void decodingUsesNeutralThumbnailContracts();
    void cppNamespaceIsLowercaseKiriview();
    void thumbnailGenerationContractsLiveInThumbnailModule();
    void documentSessionUsesThumbnailStripDependencyPort();
    void documentSessionUsesOpenWithRuntime();
    void activeNavigationThumbnailRuntimeUsesCanonicalThumbnailSourceKey();
    void liveDirectoryWatchUsesProviderBoundary();
    void mediaFormatRegistryDoesNotOwnLocalizedDialogLabels();
    void asyncImageIoJobsDoNotOwnDecodeDataLoading();
    void asyncImageIoJobsDoNotOwnDirectoryCandidateLoading();
    void asyncImageIoJobsDoNotOwnOpenedCollectionCandidateLoading();
    void mediaEntrySourceStoreDoesNotDependOnDocumentPlanning();
    void imagePageSurfaceOwnerTypeExists();
    void imagePageSurfaceOwnersExposeNoPresentationState();
    void activePresentationDoesNotWritePageSurfacePresentationState();
    void productionFacadesDoNotExposePresentationBackdoorSetters();
    void mediaInformationFacadeExposesSnapshotRevision();
    void sessionLeafSnapshotPortsAreSeparateFromCommandPorts();
};

namespace {
QString projectPath(const QString &relativePath)
{
    return QDir(QStringLiteral(KIRIVIEW_TEST_SOURCE_DIR "/../..")).filePath(relativePath);
}

QString readProjectFile(const QString &relativePath)
{
    QFile file(projectPath(relativePath));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qFatal("Cannot open %s", qPrintable(relativePath));
    }
    return QString::fromUtf8(file.readAll());
}

QStringList productionQmlFiles()
{
    QStringList files;
    QDirIterator iterator(projectPath(QStringLiteral("src/qml")),
        QStringList { QStringLiteral("*.qml") }, QDir::Files, QDirIterator::Subdirectories);
    while (iterator.hasNext()) {
        files.push_back(iterator.next());
    }
    files.sort();
    return files;
}

QStringList projectFilesUnder(const QStringList &relativeRoots, const QStringList &nameFilters)
{
    QStringList files;
    for (const QString &relativeRoot : relativeRoots) {
        QDirIterator iterator(
            projectPath(relativeRoot), nameFilters, QDir::Files, QDirIterator::Subdirectories);
        while (iterator.hasNext()) {
            files.push_back(iterator.next());
        }
    }
    files.sort();
    return files;
}

QString relativeProjectPath(const QString &absolutePath)
{
    return QDir(projectPath(QString())).relativeFilePath(absolutePath);
}

QString matchingLines(const QString &filePath, const QList<QRegularExpression> &patterns)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QStringLiteral("%1: cannot open file").arg(relativeProjectPath(filePath));
    }

    QStringList matches;
    int lineNumber = 0;
    while (!file.atEnd()) {
        ++lineNumber;
        const QString line = QString::fromUtf8(file.readLine()).trimmed();
        for (const QRegularExpression &pattern : patterns) {
            if (pattern.match(line).hasMatch()) {
                matches.push_back(QStringLiteral("%1:%2: %3")
                        .arg(relativeProjectPath(filePath))
                        .arg(lineNumber)
                        .arg(line));
                break;
            }
        }
    }
    return matches.join(QLatin1Char('\n'));
}

void verifySourceUrlReadOnly(const QMetaObject &metaObject)
{
    const int sourceUrlIndex = metaObject.indexOfProperty("sourceUrl");
    QVERIFY(sourceUrlIndex >= 0);

    const QMetaProperty sourceUrlProperty = metaObject.property(sourceUrlIndex);
    QVERIFY(sourceUrlProperty.hasNotifySignal());
    QVERIFY(!sourceUrlProperty.isWritable());
}

void verifyPrivateRouteSetter(const QString &relativePath)
{
    const QString header = readProjectFile(relativePath);
    const QRegularExpression propertyPattern(QStringLiteral(
        R"(Q_PROPERTY\(\s*QUrl\s+sourceUrl\s+READ\s+sourceUrl\s+NOTIFY\s+sourceUrlChanged\s*\))"));
    QVERIFY2(propertyPattern.match(header).hasMatch(),
        qPrintable(
            QStringLiteral("%1 sourceUrl property must remain read-only").arg(relativePath)));
    QVERIFY2(!header.contains(QStringLiteral("Q_INVOKABLE void setSourceUrl")),
        qPrintable(QStringLiteral("%1 must not expose setSourceUrl to QML").arg(relativePath)));

    const qsizetype privateIndex = header.indexOf(QStringLiteral("\nprivate:"));
    const qsizetype friendIndex
        = header.indexOf(QStringLiteral("friend class KiriDocumentSession;"));
    const qsizetype setterIndex = header.indexOf(QStringLiteral("void setSourceUrl"));
    QVERIFY2(privateIndex >= 0,
        qPrintable(QStringLiteral("%1 must have a private section").arg(relativePath)));
    QVERIFY2(friendIndex > privateIndex,
        qPrintable(QStringLiteral("%1 route setter friend must stay private").arg(relativePath)));
    QVERIFY2(setterIndex > friendIndex,
        qPrintable(QStringLiteral("%1 setSourceUrl must stay behind KiriDocumentSession")
                .arg(relativePath)));
}

QStringList existingProjectFiles(const QList<QString> &relativePaths)
{
    QStringList existing;
    for (const QString &relativePath : relativePaths) {
        if (QFileInfo::exists(projectPath(relativePath))) {
            existing.push_back(relativePath);
        }
    }
    return existing;
}
}

void TestArchitectureBoundaries::leafDocumentSourceRoutesAreReadOnlyPublicObservations()
{
    verifySourceUrlReadOnly(KiriImageDocument::staticMetaObject);
    verifySourceUrlReadOnly(KiriVideoDocument::staticMetaObject);
}

void TestArchitectureBoundaries::leafDocumentRouteSettersStayPrivateToTheSession()
{
    verifyPrivateRouteSetter(QStringLiteral("src/facade/kiriimagedocument.h"));
    verifyPrivateRouteSetter(QStringLiteral("src/facade/kirivideodocument.h"));
}

void TestArchitectureBoundaries::qmlCannotSetLeafDocumentRoutes()
{
    const QList<QRegularExpression> forbiddenPatterns {
        QRegularExpression(
            QStringLiteral(R"(\b(?:imageDocument|videoDocument)\s*\.\s*sourceUrl\s*=)")),
        QRegularExpression(
            QStringLiteral(R"(\b(?:imageDocument|videoDocument)\s*\.\s*setSourceUrl\s*\()")),
        QRegularExpression(QStringLiteral(
            R"(\bdocumentSession\s*\.\s*(?:imageDocument|videoDocument)\s*\.\s*sourceUrl\s*=)")),
        QRegularExpression(QStringLiteral(
            R"(\bdocumentSession\s*\.\s*(?:imageDocument|videoDocument)\s*\.\s*setSourceUrl\s*\()")),
        QRegularExpression(QStringLiteral(R"(^\s*sourceUrl\s*:)")),
    };

    QStringList violations;
    for (const QString &filePath : productionQmlFiles()) {
        const QString matches = matchingLines(filePath, forbiddenPatterns);
        if (!matches.isEmpty()) {
            violations.push_back(matches);
        }
    }

    QVERIFY2(violations.isEmpty(), qPrintable(violations.join(QLatin1Char('\n'))));
}

void TestArchitectureBoundaries::qmlDoesNotWriteSharedActionState()
{
    const QList<QRegularExpression> forbiddenPatterns {
        QRegularExpression(QStringLiteral(
            R"(\bsourceAction\s*\.\s*(?:enabled|checked|checkable|text|shortcut)\s*=)")),
        QRegularExpression(QStringLiteral(
            R"(\bsourceAction\s*\.\s*(?:setEnabled|setChecked|setText|setShortcut)\s*\()")),
        QRegularExpression(QStringLiteral(
            R"(\bactionForId\s*\([^)]*\)\s*\.\s*(?:enabled|checked|checkable|text|shortcut)\s*=)")),
        QRegularExpression(QStringLiteral(
            R"(\bactionForId\s*\([^)]*\)\s*\.\s*(?:setEnabled|setChecked|setText|setShortcut)\s*\()")),
        QRegularExpression(QStringLiteral(
            R"(\bapplication\s*\.\s*action\s*\([^)]*\)\s*\.\s*(?:enabled|checked|checkable|text|shortcut)\s*=)")),
        QRegularExpression(QStringLiteral(
            R"(\bapplication\s*\.\s*action\s*\([^)]*\)\s*\.\s*(?:setEnabled|setChecked|setText|setShortcut)\s*\()")),
    };

    QStringList violations;
    for (const QString &filePath : productionQmlFiles()) {
        const QString matches = matchingLines(filePath, forbiddenPatterns);
        if (!matches.isEmpty()) {
            violations.push_back(matches);
        }
    }

    QVERIFY2(violations.isEmpty(), qPrintable(violations.join(QLatin1Char('\n'))));
}

void TestArchitectureBoundaries::qmlActionProxiesDoNotOverrideRuntimeActionState()
{
    const QString managedAction = readProjectFile(QStringLiteral("src/qml/ManagedAction.qml"));
    const QString actionProxy = readProjectFile(QStringLiteral("src/qml/ActionProxy.qml"));
    const QString combined = managedAction + QLatin1Char('\n') + actionProxy;
    const QList<QRegularExpression> forbiddenPatterns {
        QRegularExpression(QStringLiteral(R"(\bproxy(?:Enabled|Checked|Checkable)\b)")),
        QRegularExpression(QStringLiteral(R"(\b(?:enabled|checked|checkable)Override\b)")),
    };

    QStringList violations;
    for (const QRegularExpression &pattern : forbiddenPatterns) {
        QRegularExpressionMatchIterator iterator = pattern.globalMatch(combined);
        while (iterator.hasNext()) {
            violations.push_back(iterator.next().captured(0));
        }
    }

    QVERIFY2(violations.isEmpty(), qPrintable(violations.join(QLatin1Char('\n'))));
}

void TestArchitectureBoundaries::qmlDoesNotOwnSharedActionPolicy()
{
    const QList<QRegularExpression> forbiddenPatterns {
        QRegularExpression(QStringLiteral(R"(\bupdateActionState\s*\()")),
        QRegularExpression(QStringLiteral(R"(\bfunction\s+dispatchAction\s*\()")),
        QRegularExpression(QStringLiteral(R"(\bonActionTriggered\s*:)")),
        QRegularExpression(QStringLiteral(R"(\bonActionTriggered\s*\()")),
        QRegularExpression(QStringLiteral(R"(^\s*Shortcut\s*\{)")),
    };

    QStringList violations;
    for (const QString &relativePath : { QStringLiteral("src/qml/ImageActions.qml"),
             QStringLiteral("src/qml/ImageShortcuts.qml") }) {
        const QString matches = matchingLines(projectPath(relativePath), forbiddenPatterns);
        if (!matches.isEmpty()) {
            violations.push_back(matches);
        }
    }

    QVERIFY2(violations.isEmpty(), qPrintable(violations.join(QLatin1Char('\n'))));
}

void TestArchitectureBoundaries::qmlDoesNotComputePannabilityActionGate()
{
    const QString mainQml = readProjectFile(QStringLiteral("src/qml/Main.qml"));
    const QString applicationHeader
        = readProjectFile(QStringLiteral("src/facade/kiriviewapplication.h"));
    const QString applicationImplementation
        = readProjectFile(QStringLiteral("src/facade/kiriviewapplication.cpp"));
    const QList<QRegularExpression> forbiddenPatterns {
        QRegularExpression(QStringLiteral(R"(\bimageMode\s*&&\s*[\w.]+imagePannable\b)")),
        QRegularExpression(QStringLiteral(R"(\bimagePannable\s*&&\s*\w+\.imageMode\b)")),
        QRegularExpression(QStringLiteral(R"(\bimageInteractionSurface\s*\.\s*imagePannable\b)")),
    };
    QStringList violations;
    for (const QRegularExpression &pattern : forbiddenPatterns) {
        QRegularExpressionMatchIterator iterator = pattern.globalMatch(mainQml);
        while (iterator.hasNext()) {
            violations.push_back(iterator.next().captured(0));
        }
    }

    QVERIFY2(violations.isEmpty(), qPrintable(violations.join(QLatin1Char('\n'))));
    QVERIFY(!applicationHeader.contains(QStringLiteral("bool imagePannable")));
    QVERIFY(!applicationHeader.contains(QStringLiteral("m_imagePannable")));
    QVERIFY(!applicationImplementation.contains(QStringLiteral("m_imagePannable")));
    QVERIFY(applicationImplementation.contains(QStringLiteral("viewportPannable()")));
}

void TestArchitectureBoundaries::qmlDoesNotRecomputeSharedMediaReadiness()
{
    const QList<QRegularExpression> forbiddenPatterns {
        QRegularExpression(QStringLiteral(
            R"(\bimageDocument\s*\.\s*status\s*===\s*KiriImageDocument\s*\.\s*Ready)")),
        QRegularExpression(
            QStringLiteral(R"(\bimageDocument\s*\.\s*unsupportedOpenedCollectionVideo\b)")),
    };

    QStringList violations;
    for (const QString &filePath : productionQmlFiles()) {
        const QString matches = matchingLines(filePath, forbiddenPatterns);
        if (!matches.isEmpty()) {
            violations.push_back(matches);
        }
    }

    QVERIFY2(violations.isEmpty(), qPrintable(violations.join(QLatin1Char('\n'))));
}

void TestArchitectureBoundaries::qmlDoesNotWriteDurablePresentationState()
{
    const QList<QRegularExpression> forbiddenPatterns {
        QRegularExpression(QStringLiteral(R"(\bimageDocument\s*\.\s*zoomPercent\s*=)")),
        QRegularExpression(QStringLiteral(R"(\bimageDocument\s*\.\s*setZoomPercent\s*\()")),
        QRegularExpression(QStringLiteral(R"(\bimageDocument\s*\.\s*setFitMode\s*\()")),
        QRegularExpression(QStringLiteral(R"(\bimageDocument\s*\.\s*resetZoom\s*\()")),
        QRegularExpression(QStringLiteral(R"(\bimageDocument\s*\.\s*twoPageModeEnabled\s*=)")),
        QRegularExpression(
            QStringLiteral(R"(\bimageDocument\s*\.\s*rightToLeftReadingEnabled\s*=)")),
        QRegularExpression(QStringLiteral(R"(\bimageDocument\s*\.\s*viewportContentPosition\s*=)")),
        QRegularExpression(QStringLiteral(R"(\bimageDocument\s*\.\s*visibleItemRect\s*=)")),
        QRegularExpression(
            QStringLiteral(R"(\bimageDocument\s*\.\s*setViewportContentPosition\s*\()")),
        QRegularExpression(QStringLiteral(R"(\bimageDocument\s*\.\s*setVisibleItemRect\s*\()")),
    };

    QStringList violations;
    for (const QString &filePath : productionQmlFiles()) {
        const QString matches = matchingLines(filePath, forbiddenPatterns);
        if (!matches.isEmpty()) {
            violations.push_back(matches);
        }
    }

    QVERIFY2(violations.isEmpty(), qPrintable(violations.join(QLatin1Char('\n'))));
}

void TestArchitectureBoundaries::imageDocumentHasNoPublicPresentationBackdoorSetters()
{
    const QString header = readProjectFile(QStringLiteral("src/facade/kiriimagedocument.h"));
    const qsizetype privateIndex = header.indexOf(QStringLiteral("\nprivate:"));
    QVERIFY(privateIndex > 0);
    const QString publicHeader = header.left(privateIndex);

    const QList<QRegularExpression> forbiddenPatterns {
        QRegularExpression(QStringLiteral(R"(\bvoid\s+setViewportContentPosition\s*\()")),
        QRegularExpression(QStringLiteral(R"(\bvoid\s+setVisibleItemRect\s*\()")),
        QRegularExpression(QStringLiteral(R"(\bvoid\s+setZoomPercent\s*\()")),
        QRegularExpression(QStringLiteral(R"(\bvoid\s+setTwoPageModeEnabled\s*\()")),
        QRegularExpression(QStringLiteral(R"(\bvoid\s+setRightToLeftReadingEnabled\s*\()")),
    };
    QStringList violations;
    for (const QRegularExpression &pattern : forbiddenPatterns) {
        const QRegularExpressionMatch match = pattern.match(publicHeader);
        if (match.hasMatch()) {
            violations.push_back(match.captured(0));
        }
    }

    QVERIFY2(violations.isEmpty(), qPrintable(violations.join(QLatin1Char('\n'))));
}

void TestArchitectureBoundaries::qmlViewportUsesRevisionedCommandAcknowledgement()
{
    const QString viewport = readProjectFile(QStringLiteral("src/qml/ImageViewport.qml"));
    QVERIFY(viewport.contains(QStringLiteral("requestViewportContentPosition(")));
    QVERIFY(viewport.contains(QStringLiteral("acknowledgeViewportCommand(")));
    QVERIFY(viewport.contains(QStringLiteral("observeViewportContentPosition(")));
    QVERIFY(!viewport.contains(QStringLiteral("Qt.callLater")));
    QVERIFY(
        !QRegularExpression(QStringLiteral(R"(\bimageDocument\s*\.\s*viewportContentPosition\s*=)"))
            .match(viewport)
            .hasMatch());
    QVERIFY(!QRegularExpression(QStringLiteral(R"(\bimageDocument\s*\.\s*visibleItemRect\s*=)"))
            .match(viewport)
            .hasMatch());
    QVERIFY(!QRegularExpression(
        QStringLiteral(R"(\bimageDocument\s*\.\s*setViewportContentPosition\s*\()"))
            .match(viewport)
            .hasMatch());
    QVERIFY(!QRegularExpression(QStringLiteral(R"(\bimageDocument\s*\.\s*setVisibleItemRect\s*\()"))
            .match(viewport)
            .hasMatch());
}

void TestArchitectureBoundaries::qmlViewportCommandHandlersDoNotApplyProjectionDirectly()
{
    const QString viewport = readProjectFile(QStringLiteral("src/qml/ImageViewport.qml"));
    const QStringList commandHandlers {
        QStringLiteral("moveContentPosition"),
        QStringLiteral("applyDisplayedImageInitialContentPosition"),
        QStringLiteral("panBy"),
        QStringLiteral("panToBottomRight"),
        QStringLiteral("panToTopLeft"),
        QStringLiteral("zoomByStep"),
        QStringLiteral("toggleFitOrActualSize"),
    };

    QStringList violations;
    for (const QString &handler : commandHandlers) {
        const QRegularExpression functionPattern(
            QStringLiteral(R"(function\s+%1\s*\([^)]*\)\s*\{([\s\S]*?)\n    \})")
                .arg(QRegularExpression::escape(handler)));
        QRegularExpressionMatchIterator matches = functionPattern.globalMatch(viewport);
        QVERIFY2(matches.hasNext(),
            qPrintable(QStringLiteral("ImageViewport.qml must define %1()").arg(handler)));

        while (matches.hasNext()) {
            if (matches.next().captured(1).contains(QStringLiteral("applyViewportProjection("))) {
                violations.push_back(QStringLiteral("%1()").arg(handler));
            }
        }
    }

    QVERIFY2(violations.isEmpty(),
        qPrintable(QStringLiteral("Viewport command handlers must rely on projection changes: %1")
                .arg(violations.join(QStringLiteral(", ")))));
}

void TestArchitectureBoundaries::qmlViewportUsesOpaqueRevisionTokens()
{
    const QString viewport = readProjectFile(QStringLiteral("src/qml/ImageViewport.qml"));
    const QString imageDocumentHeader
        = readProjectFile(QStringLiteral("src/facade/kiriimagedocument.h"));
    const QList<QRegularExpression> qmlForbiddenPatterns {
        QRegularExpression(QStringLiteral(
            R"(\bproperty\s+(?:int|real|double|var)\s+appliedViewport(?:Command|Observation)Revision\b)")),
        QRegularExpression(QStringLiteral(R"(\bviewport(?:Command|Observation)Revision\b)")),
    };
    const QList<QRegularExpression> headerForbiddenPatterns {
        QRegularExpression(QStringLiteral(
            R"(Q_PROPERTY\s*\(\s*quint64\s+viewport(?:Command|AppliedCommand|Observation)Revision\b)")),
        QRegularExpression(
            QStringLiteral(R"(Q_INVOKABLE\s+quint64\s+requestViewportContentPosition\s*\()")),
        QRegularExpression(QStringLiteral(
            R"(Q_INVOKABLE\s+bool\s+(?:beginViewportCommandApplication|completeViewportCommandApplication|acknowledgeViewportCommand)\s*\(\s*quint64\b)")),
    };
    QStringList violations;
    for (const QRegularExpression &pattern : qmlForbiddenPatterns) {
        QRegularExpressionMatchIterator iterator = pattern.globalMatch(viewport);
        while (iterator.hasNext()) {
            violations.push_back(iterator.next().captured(0));
        }
    }
    for (const QRegularExpression &pattern : headerForbiddenPatterns) {
        QRegularExpressionMatchIterator iterator = pattern.globalMatch(imageDocumentHeader);
        while (iterator.hasNext()) {
            violations.push_back(iterator.next().captured(0));
        }
    }

    QVERIFY2(violations.isEmpty(), qPrintable(violations.join(QLatin1Char('\n'))));
    QVERIFY(viewport.contains(QStringLiteral("viewportCommandRevisionToken")));
    QVERIFY(viewport.contains(QStringLiteral("viewportObservationRevisionToken")));
    QVERIFY(imageDocumentHeader.contains(QStringLiteral("viewportCommandRevisionToken")));
    QVERIFY(imageDocumentHeader.contains(QStringLiteral("viewportObservationRevisionToken")));
}

void TestArchitectureBoundaries::leafDocumentsAreNotProductionQmlCreatable()
{
    const QString imageHeader = readProjectFile(QStringLiteral("src/facade/kiriimagedocument.h"));
    const QString videoHeader = readProjectFile(QStringLiteral("src/facade/kirivideodocument.h"));
    QVERIFY(!imageHeader.contains(QStringLiteral("QML_ELEMENT")));
    QVERIFY(!videoHeader.contains(QStringLiteral("QML_ELEMENT")));
    QVERIFY(imageHeader.contains(QStringLiteral("QML_UNCREATABLE")));
    QVERIFY(videoHeader.contains(QStringLiteral("QML_UNCREATABLE")));
}

void TestArchitectureBoundaries::actionUiGatesAreRevisionedSnapshots()
{
    const QString header = readProjectFile(QStringLiteral("src/facade/kiriviewapplication.h"));
    const QString implementation
        = readProjectFile(QStringLiteral("src/facade/kiriviewapplication.cpp"));
    QVERIFY(header.contains(QStringLiteral("ActionUiGateSnapshot")));
    QVERIFY(header.contains(QStringLiteral("updateActionUiGateSnapshot")));
    QVERIFY(!header.contains(QStringLiteral("updateActionUiState(bool")));
    QVERIFY(!implementation.contains(QStringLiteral("updateActionUiState(bool")));
}

void TestArchitectureBoundaries::qmlDoesNotManufactureStaleSensitiveRevisions()
{
    const QString mainQml = readProjectFile(QStringLiteral("src/qml/Main.qml"));
    const QString videoViewport = readProjectFile(QStringLiteral("src/qml/VideoViewport.qml"));
    const QString applicationHeader
        = readProjectFile(QStringLiteral("src/facade/kiriviewapplication.h"));
    const QString sessionHeader
        = readProjectFile(QStringLiteral("src/facade/kiridocumentsession.h"));
    const QString sessionRuntimeHeader
        = readProjectFile(QStringLiteral("src/session/documentsessionruntime.h"));
    const QList<QRegularExpression> forbiddenPatterns {
        QRegularExpression(
            QStringLiteral(R"(\bproperty\s+(?:int|real|double|var)\s+\w*Revision\b)")),
        QRegularExpression(QStringLiteral(R"(\b\w+Revision\s*\+=)")),
        QRegularExpression(QStringLiteral(R"(\bnext\w*Revision\s*\()")),
        QRegularExpression(QStringLiteral(
            R"(Q_INVOKABLE\s+void\s+updateActionUiGateSnapshot\s*\(\s*quint64\s+revision\b)")),
        QRegularExpression(
            QStringLiteral(R"(reportVideoOutputSurfaceClaim\s*\(\s*quint64\s+claimRevision\b)")),
    };
    QStringList violations;
    for (const QString &source :
        { mainQml, videoViewport, applicationHeader, sessionHeader, sessionRuntimeHeader }) {
        for (const QRegularExpression &pattern : forbiddenPatterns) {
            QRegularExpressionMatchIterator iterator = pattern.globalMatch(source);
            while (iterator.hasNext()) {
                violations.push_back(iterator.next().captured(0));
            }
        }
    }

    QVERIFY2(violations.isEmpty(), qPrintable(violations.join(QLatin1Char('\n'))));
    QVERIFY(videoViewport.contains(QStringLiteral("nextVideoOutputSurfaceClaimToken")));
    QVERIFY(sessionHeader.contains(QStringLiteral("nextVideoOutputSurfaceClaimToken")));
    QVERIFY(sessionRuntimeHeader.contains(QStringLiteral("nextVideoOutputSurfaceClaimToken")));
    QVERIFY(applicationHeader.contains(QStringLiteral("updateActionUiGateSnapshot(bool")));
}

void TestArchitectureBoundaries::imageActionAvailabilityFacadeIsNotWritableQmlBackdoor()
{
    const QString header = readProjectFile(QStringLiteral("src/facade/imageactionavailability.h"));
    QVERIFY(!header.contains(QStringLiteral("QML_ELEMENT")));
    QVERIFY(!header.contains(QStringLiteral("WRITE setImageReady")));
    QVERIFY(!header.contains(QStringLiteral("WRITE setTwoPageModeEnabled")));
    QVERIFY(!header.contains(QStringLiteral("WRITE setRightToLeftReadingEnabled")));
}

void TestArchitectureBoundaries::fixedViewerShortcutsDoNotBypassRuntimeRouting()
{
    const QString header = readProjectFile(QStringLiteral("src/facade/kiriviewapplication.h"));
    const QString implementation
        = readProjectFile(QStringLiteral("src/facade/kiriviewapplication.cpp"));
    QVERIFY(!header.contains(QStringLiteral("handleFixedShortcutEvent")));
    QVERIFY(!header.contains(QStringLiteral("handleHorizontalArrowShortcut")));
    QVERIFY(!header.contains(QStringLiteral("handleSinglePageArrowShortcut")));
    QVERIFY(!header.contains(QStringLiteral("handleVerticalPanShortcut")));
    QVERIFY(!implementation.contains(QStringLiteral("class FixedShortcutEventFilter")));
    QVERIFY(!implementation.contains(QStringLiteral("handleFixedShortcutEvent")));
}

void TestArchitectureBoundaries::qmlDoesNotExposeFixedViewerScanCommandRoutes()
{
    const QString imageViewport = readProjectFile(QStringLiteral("src/qml/ImageViewport.qml"));
    const QString imageInteractionSurface
        = readProjectFile(QStringLiteral("src/qml/ImageViewportInteractionSurface.qml"));
    const QString coreSources = readProjectFile(QStringLiteral("src/cpp_core_sources.txt"));
    const QString qmlHeaders = readProjectFile(QStringLiteral("src/cpp_cxxqt_header_sources.txt"));
    const QStringList stalePolicyFacadeFiles = existingProjectFiles({
        QStringLiteral("src/facade/imageshortcutnavigationpolicy.h"),
        QStringLiteral("src/facade/imageshortcutnavigationpolicy.cpp"),
    });
    const QList<QRegularExpression> forbiddenQmlPatterns {
        QRegularExpression(QStringLiteral(R"(\brequestViewportScan(?:Forward|Backward)\s*\()")),
        QRegularExpression(QStringLiteral(R"(\bfunction\s+scan(?:Forward|Backward)\s*\()")),
    };

    QStringList violations;
    for (const QString &source : { imageViewport, imageInteractionSurface }) {
        for (const QRegularExpression &pattern : forbiddenQmlPatterns) {
            QRegularExpressionMatchIterator iterator = pattern.globalMatch(source);
            while (iterator.hasNext()) {
                violations.push_back(iterator.next().captured(0));
            }
        }
    }

    QVERIFY2(violations.isEmpty(), qPrintable(violations.join(QLatin1Char('\n'))));
    QVERIFY2(stalePolicyFacadeFiles.isEmpty(),
        qPrintable(stalePolicyFacadeFiles.join(QLatin1Char('\n'))));
    QVERIFY(!coreSources.contains(QStringLiteral("facade/imageshortcutnavigationpolicy")));
    QVERIFY(!qmlHeaders.contains(QStringLiteral("facade/imageshortcutnavigationpolicy")));
}

void TestArchitectureBoundaries::videoSeekShortcutsRouteThroughApplicationRuntime()
{
    const QString videoViewport = readProjectFile(QStringLiteral("src/qml/VideoViewport.qml"));
    const QString shortcutRuntime
        = readProjectFile(QStringLiteral("src/application/applicationshortcutruntime.cpp"));
    const QString applicationHeader
        = readProjectFile(QStringLiteral("src/facade/kiriviewapplication.h"));
    const QString applicationImplementation
        = readProjectFile(QStringLiteral("src/facade/kiriviewapplication.cpp"));
    const QList<QRegularExpression> forbiddenVideoViewportPatterns {
        QRegularExpression(QStringLiteral(R"(\bKeys\s*\.\s*onPressed\b)")),
        QRegularExpression(QStringLiteral(R"(\bhandleSeekShortcut\b)")),
        QRegularExpression(QStringLiteral(R"(\bseekByShortcut\b)")),
        QRegularExpression(QStringLiteral(R"(\bvideoDocument\s*\.\s*seekable\b)")),
        QRegularExpression(QStringLiteral(R"(\bvideoDocument\s*\.\s*seekBy\s*\()")),
        QRegularExpression(QStringLiteral(R"(\bQt\s*\.\s*Key_(?:Left|Right|Up|Down)\b)")),
    };

    QStringList violations;
    for (const QRegularExpression &pattern : forbiddenVideoViewportPatterns) {
        QRegularExpressionMatchIterator iterator = pattern.globalMatch(videoViewport);
        while (iterator.hasNext()) {
            violations.push_back(iterator.next().captured(0));
        }
    }

    QVERIFY2(violations.isEmpty(), qPrintable(violations.join(QLatin1Char('\n'))));
    const QString shortcutPolicy
        = readProjectFile(QStringLiteral("src/application/applicationshortcutpolicy.cpp"));
    QVERIFY(shortcutPolicy.contains(QStringLiteral("fixedVideoSeekShortcut")));
    QVERIFY(shortcutPolicy.contains(QStringLiteral("fixedShortcutDispatchOutcome")));
    QVERIFY(shortcutPolicy.contains(QStringLiteral("Alt+Left")));
    QVERIFY(shortcutPolicy.contains(QStringLiteral("Alt+Right")));
    QVERIFY(shortcutPolicy.contains(QStringLiteral("Alt+Up")));
    QVERIFY(shortcutPolicy.contains(QStringLiteral("Alt+Down")));
    QVERIFY(shortcutRuntime.contains(QStringLiteral("fixedShortcutDispatchOutcome")));
    QVERIFY(shortcutRuntime.contains(QStringLiteral("videoSeekShortcutTriggered")));
    QVERIFY(applicationHeader.contains(QStringLiteral("executeVideoSeekShortcut")));
    QVERIFY(applicationImplementation.contains(QStringLiteral("executeVideoSeekShortcut")));
    QVERIFY(applicationImplementation.contains(QStringLiteral("seekable()")));
    QVERIFY(applicationImplementation.contains(QStringLiteral("seekBy(")));
}

void TestArchitectureBoundaries::applicationFacadeDoesNotOwnFixedViewerCommandRouting()
{
    const QString header = readProjectFile(QStringLiteral("src/facade/kiriviewapplication.h"));
    const QString implementation
        = readProjectFile(QStringLiteral("src/facade/kiriviewapplication.cpp"));
    const QString coreSources = readProjectFile(QStringLiteral("src/cpp_core_sources.txt"));

    QVERIFY(!header.contains(QStringLiteral("navigation/imageshortcutnavigationpolicy.h")));
    QVERIFY(!header.contains(QStringLiteral("ImageShortcutNavigationPolicy m_navigationPolicy")));
    QVERIFY(!implementation.contains(QStringLiteral("m_navigationPolicy.")));
    QVERIFY(!implementation.contains(QStringLiteral("keyboardPanDistance")));
    QVERIFY(header.contains(QStringLiteral("ApplicationCommandRouter")));
    QVERIFY(implementation.contains(QStringLiteral("ApplicationCommandRouter")));
    QVERIFY(coreSources.contains(QStringLiteral("src/application/applicationcommandrouter.cpp")));
}

void TestArchitectureBoundaries::applicationCommandRouterPortsAreGroupedByOwner()
{
    const QString header
        = readProjectFile(QStringLiteral("src/application/applicationcommandrouter.h"));

    QVERIFY(header.contains(QStringLiteral("ApplicationCommandRouterShellPorts")));
    QVERIFY(header.contains(QStringLiteral("ApplicationCommandRouterSessionPorts")));
    QVERIFY(header.contains(QStringLiteral("ApplicationCommandRouterImageDocumentPorts")));
    QVERIFY(header.contains(QStringLiteral("ApplicationCommandRouterImagePresentationPorts")));
    QVERIFY(header.contains(QStringLiteral("ApplicationCommandRouterPanelPorts")));
    QVERIFY(header.contains(QStringLiteral("ApplicationCommandRouterWindowPorts")));
    QVERIFY(header.contains(QStringLiteral("ApplicationCommandRouterHelpPorts")));
    QVERIFY(header.contains(QStringLiteral("ApplicationCommandRouterVideoPorts")));

    QVERIFY(header.contains(QStringLiteral("ApplicationCommandRouterShellPorts shell")));
    QVERIFY(header.contains(QStringLiteral("ApplicationCommandRouterSessionPorts session")));
    QVERIFY(header.contains(
        QStringLiteral("ApplicationCommandRouterImageDocumentPorts imageDocument")));
    QVERIFY(header.contains(
        QStringLiteral("ApplicationCommandRouterImagePresentationPorts imagePresentation")));
    QVERIFY(header.contains(QStringLiteral("ApplicationCommandRouterPanelPorts panel")));
    QVERIFY(header.contains(QStringLiteral("ApplicationCommandRouterWindowPorts window")));
    QVERIFY(header.contains(QStringLiteral("ApplicationCommandRouterHelpPorts help")));
    QVERIFY(header.contains(QStringLiteral("ApplicationCommandRouterVideoPorts video")));
}

void TestArchitectureBoundaries::applicationFacadeDoesNotOwnActionCommandSwitch()
{
    const QString implementation
        = readProjectFile(QStringLiteral("src/facade/kiriviewapplication.cpp"));

    QVERIFY(!implementation.contains(QStringLiteral("switch (actionId)")));
    QVERIFY(!implementation.contains(QStringLiteral("requestFitMode(KiriImageDocument::ZoomMode")));
    QVERIFY(!implementation.contains(QStringLiteral("deleteDisplayedFile(KiriDocumentSession::")));
    QVERIFY(implementation.contains(QStringLiteral("handleActionTriggered(actionId")));
}

void TestArchitectureBoundaries::sessionPublicProjectionHasNoPartialUpdateBackdoor()
{
    const QString header = readProjectFile(QStringLiteral("src/session/documentsessionstate.h"));
    const QString implementation
        = readProjectFile(QStringLiteral("src/session/documentsessionstate.cpp"));
    QVERIFY(!header.contains(QStringLiteral("updatePublicProjection")));
    QVERIFY(!implementation.contains(QStringLiteral("updatePublicProjection")));
    QVERIFY(!implementation.contains(QStringLiteral("applyPublicProjection")));
}

void TestArchitectureBoundaries::sessionPublicProjectionDoesNotSampleLeafFacadesWhileApplying()
{
    const QString implementation
        = readProjectFile(QStringLiteral("src/session/documentsessionruntime.cpp"));
    const qsizetype functionIndex
        = implementation.indexOf(QStringLiteral("DocumentSessionRuntime::publicSnapshotInput("));
    QVERIFY(functionIndex >= 0);
    const qsizetype nextFunctionIndex = implementation.indexOf(
        QStringLiteral("\nDirectMediaActiveNavigationInput"), functionIndex);
    QVERIFY(nextFunctionIndex > functionIndex);
    const QString body = implementation.mid(functionIndex, nextFunctionIndex - functionIndex);

    const QList<QRegularExpression> forbiddenPatterns {
        QRegularExpression(QStringLiteral(R"(\bm_imageDocument\s*\.)")),
        QRegularExpression(QStringLiteral(R"(\bm_videoDocument\s*\.)")),
    };
    QStringList violations;
    for (const QRegularExpression &pattern : forbiddenPatterns) {
        QRegularExpressionMatchIterator iterator = pattern.globalMatch(body);
        while (iterator.hasNext()) {
            violations.push_back(iterator.next().captured(0));
        }
    }
    QVERIFY2(violations.isEmpty(), qPrintable(violations.join(QLatin1Char('\n'))));
}

void TestArchitectureBoundaries::qmlDoesNotWriteSharedVideoOutputAttachment()
{
    const QList<QRegularExpression> forbiddenPatterns {
        QRegularExpression(QStringLiteral(R"(\b\w+\s*\.\s*videoOutput\s*=)")),
        QRegularExpression(QStringLiteral(R"(\bvideoDocument\s*\.\s*videoOutput\s*=)")),
        QRegularExpression(
            QStringLiteral(R"(\bdocumentSession\s*\.\s*videoDocument\s*\.\s*videoOutput\s*=)")),
    };

    QStringList violations;
    for (const QString &filePath : productionQmlFiles()) {
        const QString matches = matchingLines(filePath, forbiddenPatterns);
        if (!matches.isEmpty()) {
            violations.push_back(matches);
        }
    }

    QVERIFY2(violations.isEmpty(), qPrintable(violations.join(QLatin1Char('\n'))));
}

void TestArchitectureBoundaries::videoOutputAttachmentIsNotWritablePublicVideoDocumentState()
{
    const QString header = readProjectFile(QStringLiteral("src/facade/kirivideodocument.h"));
    QVERIFY(!header.contains(QStringLiteral("WRITE setVideoOutput")));
    QVERIFY(!header.contains(QStringLiteral("Q_INVOKABLE void setVideoOutput")));
}

void TestArchitectureBoundaries::qmlDoesNotDeriveSharedControlPolicyFromLeafDocuments()
{
    const QList<QRegularExpression> forbiddenPatterns {
        QRegularExpression(
            QStringLiteral(R"(\bimageDocument\s*\.\s*openedCollectionScopeActive\b)")),
        QRegularExpression(QStringLiteral(R"(\bimageDocument\s*\.\s*rightToLeftReadingEnabled\b)")),
        QRegularExpression(
            QStringLiteral(R"(\bimageDocument\s*\.\s*rightToLeftReadingAvailable\b)")),
        QRegularExpression(QStringLiteral(
            R"(\bvideoDocument\s*\.\s*status\s*===\s*KiriVideoDocument\s*\.\s*Ready)")),
    };

    QStringList violations;
    for (const QString &filePath : productionQmlFiles()) {
        const QString matches = matchingLines(filePath, forbiddenPatterns);
        if (!matches.isEmpty()) {
            violations.push_back(matches);
        }
    }

    QVERIFY2(violations.isEmpty(), qPrintable(violations.join(QLatin1Char('\n'))));
}

void TestArchitectureBoundaries::qmlUsesCentralNavigationPresentationOrder()
{
    const QString projectionRelativePath
        = QStringLiteral("src/qml/NavigationPresentationOrder.qml");
    QVERIFY2(QFileInfo::exists(projectPath(projectionRelativePath)),
        qPrintable(QStringLiteral("%1 must own RTL-aware navigation presentation ordering")
                .arg(projectionRelativePath)));

    const QList<QRegularExpression> forbiddenPatterns {
        QRegularExpression(QStringLiteral(
            R"(\brightToLeftReadingActive\b[^\n]*\?[^\n]*(?:previous|next|first|last)(?:Image|Container)(?:Managed|Menu)?Action)")),
        QRegularExpression(QStringLiteral(
            R"((?:previous|next|first|last)(?:Image|Container)(?:Managed|Menu)?Action[^\n]*\brightToLeftReadingActive\b[^\n]*\?)")),
    };

    QStringList violations;
    for (const QString &filePath : productionQmlFiles()) {
        if (relativeProjectPath(filePath) == projectionRelativePath) {
            continue;
        }

        const QString matches = matchingLines(filePath, forbiddenPatterns);
        if (!matches.isEmpty()) {
            violations.push_back(matches);
        }
    }

    QVERIFY2(violations.isEmpty(), qPrintable(violations.join(QLatin1Char('\n'))));

    const QString projection = readProjectFile(projectionRelativePath);
    QVERIFY(projection.contains(QStringLiteral("rightToLeftReadingActive")));
    QVERIFY(projection.contains(QStringLiteral("leading")));
    QVERIFY(projection.contains(QStringLiteral("trailing")));
}

void TestArchitectureBoundaries::qmlViewportUsesFullCommandLifecycle()
{
    const QString viewport = readProjectFile(QStringLiteral("src/qml/ImageViewport.qml"));
    QVERIFY(viewport.contains(QStringLiteral("beginViewportCommandApplication(")));
    QVERIFY(viewport.contains(QStringLiteral("completeViewportCommandApplication(")));
    QVERIFY(viewport.contains(QStringLiteral("acknowledgeViewportCommand(")));
    QVERIFY(!viewport.contains(QStringLiteral("rejectedViewportCommandStatus = 6")));
}

void TestArchitectureBoundaries::viewportContextBridgeIsNonRenderingPublicQtFacade()
{
    const QString headerPath
        = projectPath(QStringLiteral("src/facade/kiriimageviewportcontextbridge.h"));
    const QString implementationPath
        = projectPath(QStringLiteral("src/facade/kiriimageviewportcontextbridge.cpp"));
    QVERIFY2(QFileInfo::exists(headerPath),
        qPrintable(QStringLiteral("%1 must define the non-rendering viewport context bridge")
                .arg(relativeProjectPath(headerPath))));
    QVERIFY2(QFileInfo::exists(implementationPath),
        qPrintable(QStringLiteral("%1 must implement the non-rendering viewport context bridge")
                .arg(relativeProjectPath(implementationPath))));

    const QString header
        = readProjectFile(QStringLiteral("src/facade/kiriimageviewportcontextbridge.h"));
    const QString implementation
        = readProjectFile(QStringLiteral("src/facade/kiriimageviewportcontextbridge.cpp"));
    const QString combined = header + QLatin1Char('\n') + implementation;
    QVERIFY(header.contains(QStringLiteral("class KiriImageViewportContextBridge")));
    QVERIFY(header.contains(QStringLiteral("QML_ELEMENT")));
    QVERIFY(header.contains(QStringLiteral("public QQuickItem")));
    QVERIFY(header.contains(QStringLiteral("KiriImageDocument *document")));
    QVERIFY(header.contains(QStringLiteral("bool secondaryPage")));
    QVERIFY(header.contains(QStringLiteral("renderContextProviderInstalled")));

    const QList<QString> forbiddenTokens {
        QStringLiteral("ItemHasContents"),
        QStringLiteral("updatePaintNode"),
        QStringLiteral("QSGNode"),
        QStringLiteral("QSGTexture"),
        QStringLiteral("QRhi"),
        QStringLiteral("<rhi/qrhi.h>"),
        QStringLiteral("->rhi("),
        QStringLiteral(".rhi("),
    };
    QStringList violations;
    for (const QString &token : forbiddenTokens) {
        if (combined.contains(token)) {
            violations.push_back(token);
        }
    }
    QVERIFY2(violations.isEmpty(), qPrintable(violations.join(QLatin1Char('\n'))));

    const QString cxxqtSources = readProjectFile(QStringLiteral("src/cpp_cxxqt_sources.txt"));
    const QString cxxqtHeaders
        = readProjectFile(QStringLiteral("src/cpp_cxxqt_header_sources.txt"));
    QVERIFY(cxxqtSources.contains(QStringLiteral("src/facade/kiriimageviewportcontextbridge.cpp")));
    QVERIFY(cxxqtHeaders.contains(QStringLiteral("src/facade/kiriimageviewportcontextbridge.h")));
}

void TestArchitectureBoundaries::qmlViewportUsesContextBridgeForRenderContextDiscovery()
{
    const QString viewport = readProjectFile(QStringLiteral("src/qml/ImageViewport.qml"));
    QVERIFY(viewport.contains(QStringLiteral("KiriImageViewportContextBridge")));
    QVERIFY(viewport.contains(QStringLiteral("DisplayImagePage")));
    QVERIFY(viewport.contains(QStringLiteral("objectName: \"primaryContextBridge\"")));
    QVERIFY(viewport.contains(QStringLiteral("objectName: \"secondaryContextBridge\"")));
    QVERIFY(viewport.contains(QStringLiteral("acknowledgeDisplayImageLoad(")));
    QVERIFY(!viewport.contains(QStringLiteral("KiriImageView {")));
    QVERIFY(!viewport.contains(QStringLiteral("primaryImageView")));
    QVERIFY(!viewport.contains(QStringLiteral("secondaryImageView")));
    QVERIFY(!viewport.contains(QStringLiteral("renderContextProviderEnabled")));
    QVERIFY(viewport.contains(QStringLiteral("imageDocument.viewportPointInsideImage(")));
    QVERIFY(viewport.contains(QStringLiteral("imageDocument.nearestImageViewportPoint(")));
    QVERIFY(!viewport.contains(QStringLiteral("imageView.viewportPointInsideImage(")));
    QVERIFY(!viewport.contains(QStringLiteral("imageView.nearestImageViewportPoint(")));
    QVERIFY(!viewport.contains(QStringLiteral("onDisplayedImageInitialContentPositionRequested")));
}

void TestArchitectureBoundaries::oldImageRendererArtifactsAreAbsent()
{
    const QList<QString> forbiddenFiles {
        QStringLiteral("src/facade/kiriimageview.cpp"),
        QStringLiteral("src/facade/kiriimageview.h"),
        QStringLiteral("src/rendering/decodedtilecache.cpp"),
        QStringLiteral("src/rendering/decodedtilecache.h"),
        QStringLiteral("src/rendering/displayedimagesurfacestate.cpp"),
        QStringLiteral("src/rendering/displayedimagesurfacestate.h"),
        QStringLiteral("src/rendering/imagerenderframe.cpp"),
        QStringLiteral("src/rendering/imagerenderframe.h"),
        QStringLiteral("src/rendering/imagerendernodestate.cpp"),
        QStringLiteral("src/rendering/imagerendernodestate.h"),
        QStringLiteral("src/rendering/imagetiledecoderuntime.cpp"),
        QStringLiteral("src/rendering/imagetiledecoderuntime.h"),
        QStringLiteral("src/rendering/imagetiledecodescheduler.cpp"),
        QStringLiteral("src/rendering/imagetiledecodescheduler.h"),
        QStringLiteral("src/rendering/imagetiledecodestate.cpp"),
        QStringLiteral("src/rendering/imagetiledecodestate.h"),
        QStringLiteral("src/rendering/imagetilerequestplan.cpp"),
        QStringLiteral("src/rendering/imagetilerequestplan.h"),
        QStringLiteral("src/rendering/imagesurface.cpp"),
        QStringLiteral("src/rendering/imagesurface.h"),
        QStringLiteral("src/rendering/kiriimagerendernode.cpp"),
        QStringLiteral("src/rendering/kiriimagerendernode.h"),
        QStringLiteral("src/shaders/kiriimageview.frag"),
        QStringLiteral("src/shaders/kiriimageview.vert"),
        QStringLiteral("src/shaders/kiriimageview_shaders.h"),
    };

    const QStringList existing = existingProjectFiles(forbiddenFiles);
    QVERIFY2(existing.isEmpty(), qPrintable(existing.join(QLatin1Char('\n'))));

    const QString coreSources = readProjectFile(QStringLiteral("src/cpp_core_sources.txt"));
    const QString cxxqtSources = readProjectFile(QStringLiteral("src/cpp_cxxqt_sources.txt"));
    const QString cxxqtHeaders
        = readProjectFile(QStringLiteral("src/cpp_cxxqt_header_sources.txt"));
    const QString manifests
        = coreSources + QLatin1Char('\n') + cxxqtSources + QLatin1Char('\n') + cxxqtHeaders;
    QStringList manifestViolations;
    for (const QString &relativePath : forbiddenFiles) {
        if (manifests.contains(relativePath)) {
            manifestViolations.push_back(relativePath);
        }
    }
    QVERIFY2(manifestViolations.isEmpty(), qPrintable(manifestViolations.join(QLatin1Char('\n'))));
}

void TestArchitectureBoundaries::oldImageRendererBuildWiringIsAbsent()
{
    const QString buildScript = readProjectFile(QStringLiteral("build.rs"));
    const QList<QString> forbiddenBuildTokens {
        QStringLiteral("QT_RHI"),
        QStringLiteral("add_qt_rhi_include_dirs"),
        QStringLiteral("bake_shaders"),
        QStringLiteral("run_qsb"),
        QStringLiteral("qsb"),
        QStringLiteral("qshader"),
        QStringLiteral("kiriimageview.vert"),
        QStringLiteral("kiriimageview.frag"),
        QStringLiteral("kiriimageview_shaders"),
    };

    QStringList buildViolations;
    for (const QString &token : forbiddenBuildTokens) {
        if (buildScript.contains(token)) {
            buildViolations.push_back(QStringLiteral("build.rs: %1").arg(token));
        }
    }

    const QString testCMake = readProjectFile(QStringLiteral("tests/cpp/CMakeLists.txt"));
    const QList<QString> forbiddenTestCMakeTokens {
        QStringLiteral("kiriview_qt_rhi_include_dirs"),
        QStringLiteral("KIRIVIEW_QT_RHI_INCLUDE_DIRS"),
        QStringLiteral("rhi/qrhi.h"),
        QStringLiteral("rhi/qshader.h"),
    };
    for (const QString &token : forbiddenTestCMakeTokens) {
        if (testCMake.contains(token)) {
            buildViolations.push_back(QStringLiteral("tests/cpp/CMakeLists.txt: %1").arg(token));
        }
    }

    QVERIFY2(buildViolations.isEmpty(), qPrintable(buildViolations.join(QLatin1Char('\n'))));
}

void TestArchitectureBoundaries::cppTestBuildConsumesCargoAppLibraryOnly()
{
    const QString testCMake = readProjectFile(QStringLiteral("tests/cpp/CMakeLists.txt"));
    const QList<QString> forbiddenTokens {
        QStringLiteral("kiriview_manifest_sources"),
        QStringLiteral("cpp_core_sources.txt"),
        QStringLiteral("cpp_cxxqt_sources.txt"),
        QStringLiteral("KIRIVIEW_CORE_SOURCE_PATHS"),
        QStringLiteral("KIRIVIEW_CXXQT_SOURCE_PATHS"),
        QStringLiteral("kconfig_add_kcfg_files"),
    };
    const QList<QString> requiredTokens {
        QStringLiteral("KiriViewCargoStatic"),
        QStringLiteral("add_library(kiriview_test_core INTERFACE"),
        QStringLiteral("LINK_LIBRARY:WHOLE_ARCHIVE"),
        QStringLiteral("libkiriview.a"),
    };

    QStringList violations;
    for (const QString &token : forbiddenTokens) {
        if (testCMake.contains(token)) {
            violations.push_back(
                QStringLiteral("tests/cpp/CMakeLists.txt must not contain %1").arg(token));
        }
    }
    for (const QString &token : requiredTokens) {
        if (!testCMake.contains(token)) {
            violations.push_back(
                QStringLiteral("tests/cpp/CMakeLists.txt must contain %1").arg(token));
        }
    }

    QVERIFY2(violations.isEmpty(), qPrintable(violations.join(QLatin1Char('\n'))));
}

void TestArchitectureBoundaries::productionImageDisplayUsesProviderPathOnly()
{
    const QList<QRegularExpression> forbiddenPatterns {
        QRegularExpression(QStringLiteral(R"(<rhi/qrhi\.h>)")),
        QRegularExpression(QStringLiteral(R"(\bQQuickWindow::rhi\b)")),
        QRegularExpression(QStringLiteral(R"((?:->|\.)rhi\s*\()")),
        QRegularExpression(QStringLiteral(R"(\bQSGRenderNode\b)")),
        QRegularExpression(QStringLiteral(R"(\bQSGTexture\b)")),
        QRegularExpression(QStringLiteral(R"(\bQSGTextureProvider\b)")),
        QRegularExpression(QStringLiteral(R"(\bQQuickFramebufferObject\b)")),
        QRegularExpression(QStringLiteral(R"(\bupdatePaintNode\s*\()")),
        QRegularExpression(QStringLiteral(R"(\bItemHasContents\b)")),
        QRegularExpression(QStringLiteral(R"(\bKiriImageView\b)")),
        QRegularExpression(QStringLiteral(R"(\bKiriImageRenderNode\b)")),
        QRegularExpression(QStringLiteral(R"(\bImageRenderFrame\b)")),
        QRegularExpression(QStringLiteral(R"(\bDisplayedImageRenderSnapshot\b)")),
        QRegularExpression(QStringLiteral(R"(\bDisplayedImageSurface\b)")),
        QRegularExpression(QStringLiteral(R"(\bStaticTileSurface\b)")),
        QRegularExpression(QStringLiteral(R"(\bStaticImagePayload\b)")),
        QRegularExpression(QStringLiteral(R"(\bcompatibilityStaticImage\b)")),
        QRegularExpression(QStringLiteral(R"(\bsetStaticImage\s*\()")),
        QRegularExpression(QStringLiteral(R"(\bstaticTileCache\w*)")),
        QRegularExpression(QStringLiteral(R"(\bStaticTileCache\w*)")),
        QRegularExpression(QStringLiteral(R"(\bImageTileDecode(?:Scheduler|Runtime|State)\b)")),
        QRegularExpression(QStringLiteral(R"(\bDecodedTileCache\b)")),
        QRegularExpression(QStringLiteral(R"(\bscheduleVisibleTileDecode\s*\()")),
        QRegularExpression(QStringLiteral(R"(\bImageDocumentChange::RenderFrame\b)")),
        QRegularExpression(QStringLiteral(R"(\bImageDocumentChange::Repaint\b)")),
        QRegularExpression(QStringLiteral(R"(\bImageDocumentPublicSignal::Repaint\b)")),
        QRegularExpression(QStringLiteral(R"(\brepaintRequested\b)")),
    };

    QStringList violations;
    const QStringList files = projectFilesUnder({ QStringLiteral("src") },
        { QStringLiteral("*.cpp"), QStringLiteral("*.h"), QStringLiteral("*.qml"),
            QStringLiteral("*.txt") });
    for (const QString &filePath : files) {
        const QString matches = matchingLines(filePath, forbiddenPatterns);
        if (!matches.isEmpty()) {
            violations.push_back(matches);
        }
    }

    QVERIFY2(violations.isEmpty(), qPrintable(violations.join(QLatin1Char('\n'))));
}

void TestArchitectureBoundaries::sourceKeysExposeTypedExtensionFamilies()
{
    const QString header = readProjectFile(QStringLiteral("src/location/sourcekey.h"));
    for (const QString &typeName : {
             QStringLiteral("OrdinaryFileSourceKey"),
             QStringLiteral("DirectMediaSourceKey"),
             QStringLiteral("DirectMediaScopeKey"),
             QStringLiteral("ImageDocumentPageSourceKey"),
             QStringLiteral("OpenedCollectionEntrySourceKey"),
             QStringLiteral("ThumbnailSourceKey"),
             QStringLiteral("PredecodeCandidateKey"),
             QStringLiteral("RenderSurfaceKey"),
         }) {
        QVERIFY2(header.contains(typeName), qPrintable(typeName));
    }
}

void TestArchitectureBoundaries::sourceKeysExposeOperationalExtensionContracts()
{
    const QString header = readProjectFile(QStringLiteral("src/location/sourcekey.h"));
    for (const QString &symbolName : {
             QStringLiteral("openedCollectionEntrySourceKey"),
             QStringLiteral("thumbnailSourceKey"),
             QStringLiteral("predecodeCandidateKey"),
             QStringLiteral("renderSurfaceKey"),
             QStringLiteral("sameOpenedCollectionEntrySourceKey"),
             QStringLiteral("sameThumbnailSourceKey"),
             QStringLiteral("samePredecodeCandidateKey"),
             QStringLiteral("sameRenderSurfaceKey"),
             QStringLiteral("qHash(const RenderSurfaceKey"),
         }) {
        QVERIFY2(header.contains(symbolName), qPrintable(symbolName));
    }
}

void TestArchitectureBoundaries::openedCollectionThumbnailEligibilityUsesSharedPolicy()
{
    const QString policyHeader
        = readProjectFile(QStringLiteral("src/archive/openedcollectionthumbnailpolicy.h"));
    const QString karchiveBackend
        = readProjectFile(QStringLiteral("src/archive/mediaentrysourcebackend_karchive.cpp"));

    QVERIFY(policyHeader.contains(
        QStringLiteral("openedCollectionEntrySupportsThumbnailContentIdentity")));
    QVERIFY(policyHeader.contains(
        QStringLiteral("openedCollectionEntryPathSupportsThumbnailContentIdentity")));
    QVERIFY(!policyHeader.contains(
        QStringLiteral("openedCollectionRootSchemeSupportsThumbnailContentIdentity")));
    QVERIFY(karchiveBackend.contains(
        QStringLiteral("openedCollectionEntryPathSupportsThumbnailContentIdentity")));

    const QList<QRegularExpression> forbiddenPatterns {
        QRegularExpression(
            QStringLiteral(R"(\bopenedCollectionRootSchemeSupportsThumbnailContentIdentity\b)")),
        QRegularExpression(QStringLiteral(R"(\bisSupportedImageFileName\s*\()")),
        QRegularExpression(QStringLiteral(R"(\bisComicBook\s*\()")),
    };

    QStringList violations;
    for (const QRegularExpression &pattern : forbiddenPatterns) {
        QRegularExpressionMatchIterator iterator = pattern.globalMatch(karchiveBackend);
        while (iterator.hasNext()) {
            violations.push_back(iterator.next().captured(0));
        }
    }

    QVERIFY2(violations.isEmpty(), qPrintable(violations.join(QLatin1Char('\n'))));
}

void TestArchitectureBoundaries::decodingUsesNeutralThumbnailContracts()
{
    const QStringList files = projectFilesUnder(
        { QStringLiteral("src/decoding") }, { QStringLiteral("*.cpp"), QStringLiteral("*.h") });
    const QList<QRegularExpression> forbiddenPatterns {
        QRegularExpression(QStringLiteral(R"(#include\s+"session/thumbnail[^"]+")")),
        QRegularExpression(
            QStringLiteral(R"(#include\s+"session/activenavigationthumbnaildemand\.h")")),
    };

    QStringList violations;
    for (const QString &filePath : files) {
        const QString matches = matchingLines(filePath, forbiddenPatterns);
        if (!matches.isEmpty()) {
            violations.push_back(matches);
        }
    }

    QVERIFY2(violations.isEmpty(), qPrintable(violations.join(QLatin1Char('\n'))));

    const QString cacheLookupHeader
        = readProjectFile(QStringLiteral("src/thumbnail/thumbnailcachelookup.h"));
    QVERIFY(cacheLookupHeader.contains(QStringLiteral("ThumbnailCacheLookupRequest")));
}

void TestArchitectureBoundaries::cppNamespaceIsLowercaseKiriview()
{
    const QList<QRegularExpression> forbiddenCppPatterns {
        QRegularExpression(QStringLiteral(R"(\bnamespace\s+KiriView\b)")),
        QRegularExpression(QStringLiteral(R"(\bKiriView::)")),
        QRegularExpression(QStringLiteral(R"(\busing\s+namespace\s+KiriView\b)")),
    };
    const QList<QRegularExpression> forbiddenRustPatterns {
        QRegularExpression(QStringLiteral(R"(namespace\s*=\s*"KiriView")")),
    };

    QStringList violations;
    for (const QString &filePath : projectFilesUnder(QStringList { QStringLiteral("src") },
             QStringList {
                 QStringLiteral("*.cpp"),
                 QStringLiteral("*.h"),
             })) {
        const QString matches = matchingLines(filePath, forbiddenCppPatterns);
        if (!matches.isEmpty()) {
            violations.push_back(matches);
        }
    }
    for (const QString &filePath : projectFilesUnder(QStringList { QStringLiteral("src/policy") },
             QStringList { QStringLiteral("*.rs") })) {
        const QString matches = matchingLines(filePath, forbiddenRustPatterns);
        if (!matches.isEmpty()) {
            violations.push_back(matches);
        }
    }

    QVERIFY2(violations.isEmpty(), qPrintable(violations.join(QLatin1Char('\n'))));
}

void TestArchitectureBoundaries::thumbnailGenerationContractsLiveInThumbnailModule()
{
    const QString generationHeaderPath = QStringLiteral("src/thumbnail/thumbnailgeneration.h");
    const QString generationSourcePath = QStringLiteral("src/thumbnail/thumbnailgeneration.cpp");
    const QString legacyGenerationHeaderPath = QStringLiteral("src/session/thumbnailgeneration.h");
    const QString legacyGenerationSourcePath
        = QStringLiteral("src/session/thumbnailgeneration.cpp");

    QVERIFY2(QFileInfo::exists(projectPath(generationHeaderPath)),
        qPrintable(QStringLiteral("%1 must own thumbnail generation contracts")
                .arg(generationHeaderPath)));
    QVERIFY2(!QFileInfo::exists(projectPath(legacyGenerationHeaderPath)),
        qPrintable(QStringLiteral("%1 must move out of session").arg(legacyGenerationHeaderPath)));
    QVERIFY2(!QFileInfo::exists(projectPath(legacyGenerationSourcePath)),
        qPrintable(QStringLiteral("%1 must move out of session").arg(legacyGenerationSourcePath)));

    const QString generationHeader = readProjectFile(generationHeaderPath);
    const QString coreSources = readProjectFile(QStringLiteral("src/cpp_core_sources.txt"));
    const QString activeNavigationRuntimeHeader
        = readProjectFile(QStringLiteral("src/session/activenavigationthumbnailruntime.h"));

    QVERIFY(generationHeader.contains(QStringLiteral("ThumbnailSourceKind sourceKind")));
    QVERIFY(!generationHeader.contains(QStringLiteral("ActiveNavigationThumbnailSourceKind")));
    QVERIFY(!generationHeader.contains(
        QStringLiteral("#include \"session/activenavigationthumbnailprojection.h\"")));
    QVERIFY(
        generationHeader.contains(QStringLiteral("#include \"thumbnail/thumbnailsourcekind.h\"")));
    QVERIFY(coreSources.contains(generationSourcePath));
    QVERIFY(!coreSources.contains(legacyGenerationSourcePath));
    QVERIFY(activeNavigationRuntimeHeader.contains(
        QStringLiteral("#include \"thumbnail/thumbnailgeneration.h\"")));
    QVERIFY(!activeNavigationRuntimeHeader.contains(
        QStringLiteral("#include \"session/thumbnailgeneration.h\"")));
}

void TestArchitectureBoundaries::documentSessionUsesThumbnailStripDependencyPort()
{
    const QString documentSessionHeader
        = readProjectFile(QStringLiteral("src/session/documentsessionruntime.h"));
    const QString thumbnailRuntimeHeader
        = readProjectFile(QStringLiteral("src/session/activenavigationthumbnailruntime.h"));

    QVERIFY(thumbnailRuntimeHeader.contains(
        QStringLiteral("struct ActiveNavigationThumbnailRuntimeDependencies")));
    QVERIFY(documentSessionHeader.contains(
        QStringLiteral("ActiveNavigationThumbnailRuntimeDependencies activeNavigationThumbnails")));

    const QList<QRegularExpression> rawThumbnailProviderFields {
        QRegularExpression(QStringLiteral(R"(\bactiveNavigationThumbnailLookupProvider\b)")),
        QRegularExpression(QStringLiteral(R"(\bactiveNavigationThumbnailGenerationProvider\b)")),
        QRegularExpression(QStringLiteral(R"(\bactiveNavigationThumbnailSourceAdapter\b)")),
        QRegularExpression(QStringLiteral(R"(\bactiveNavigationThumbnailWorkerScheduler\b)")),
        QRegularExpression(QStringLiteral(R"(\bactiveNavigationThumbnailImageStore\b)")),
    };

    QStringList violations;
    for (const QRegularExpression &pattern : rawThumbnailProviderFields) {
        if (pattern.match(documentSessionHeader).hasMatch()) {
            violations.push_back(pattern.pattern());
        }
    }

    QVERIFY2(violations.isEmpty(), qPrintable(violations.join(QLatin1Char('\n'))));
}

void TestArchitectureBoundaries::documentSessionUsesOpenWithRuntime()
{
    const QString documentSessionHeader
        = readProjectFile(QStringLiteral("src/session/documentsessionruntime.h"));
    const QString openWithRuntimeHeader
        = readProjectFile(QStringLiteral("src/session/documentsessionmediaopenwithruntime.h"));

    QVERIFY(openWithRuntimeHeader.contains(
        QStringLiteral("class DocumentSessionMediaOpenWithRuntime")));
    QVERIFY(documentSessionHeader.contains(QStringLiteral("DocumentSessionMediaOpenWithRuntime")));

    const QList<QRegularExpression> rawOpenWithFields {
        QRegularExpression(QStringLiteral(R"(\bm_mediaOpenWithProvider\b)")),
        QRegularExpression(QStringLiteral(R"(\bm_mediaOpenWithJob\b)")),
        QRegularExpression(QStringLiteral(R"(\bm_mediaOpenWithOperation\b)")),
    };

    QStringList violations;
    for (const QRegularExpression &pattern : rawOpenWithFields) {
        if (pattern.match(documentSessionHeader).hasMatch()) {
            violations.push_back(pattern.pattern());
        }
    }

    QVERIFY2(violations.isEmpty(), qPrintable(violations.join(QLatin1Char('\n'))));
}

void TestArchitectureBoundaries::activeNavigationThumbnailRuntimeUsesCanonicalThumbnailSourceKey()
{
    const QString thumbnailRuntimeHeader
        = readProjectFile(QStringLiteral("src/session/activenavigationthumbnailruntime.h"));
    const QString thumbnailRuntimeSource
        = readProjectFile(QStringLiteral("src/session/activenavigationthumbnailruntime.cpp"));

    QVERIFY(thumbnailRuntimeHeader.contains(QStringLiteral("#include \"location/sourcekey.h\"")));
    QVERIFY(thumbnailRuntimeHeader.contains(QStringLiteral("ThumbnailSourceKey sourceKey")));
    QVERIFY(!thumbnailRuntimeHeader.contains(
        QStringLiteral("struct ActiveNavigationThumbnailSourceKey")));
    QVERIFY(!thumbnailRuntimeHeader.contains(QStringLiteral("static bool sameSourceKey")));
    QVERIFY(!thumbnailRuntimeSource.contains(
        QStringLiteral("ActiveNavigationThumbnailSourceKey sourceKeyForRow")));
}

void TestArchitectureBoundaries::liveDirectoryWatchUsesProviderBoundary()
{
    const QString entryHeader = readProjectFile(
        QStringLiteral("src/navigation/imagedocumentpagecandidatedirectoryentry.h"));
    const QString entryImplementation = readProjectFile(
        QStringLiteral("src/navigation/imagedocumentpagecandidatedirectoryentry.cpp"));
    const QString entryCombined = entryHeader + QLatin1Char('\n') + entryImplementation;
    QVERIFY2(!entryCombined.contains(QStringLiteral("KCoreDirLister")),
        "ImageDocumentPageCandidateDirectoryEntry must consume watch provider events instead of "
        "owning KDE listers");

    const QString providerHeaderPath
        = projectPath(QStringLiteral("src/navigation/imagedocumentpagecandidatewatchprovider.h"));
    const QString providerImplementationPath
        = projectPath(QStringLiteral("src/navigation/imagedocumentpagecandidatewatchprovider.cpp"));
    QVERIFY2(QFileInfo::exists(providerHeaderPath),
        qPrintable(QStringLiteral("%1 must define the live directory watch provider port")
                .arg(relativeProjectPath(providerHeaderPath))));
    QVERIFY2(QFileInfo::exists(providerImplementationPath),
        qPrintable(QStringLiteral("%1 must implement the production live directory watch provider")
                .arg(relativeProjectPath(providerImplementationPath))));

    const QString providerHeader = readProjectFile(
        QStringLiteral("src/navigation/imagedocumentpagecandidatewatchprovider.h"));
    const QString providerImplementation = readProjectFile(
        QStringLiteral("src/navigation/imagedocumentpagecandidatewatchprovider.cpp"));
    QVERIFY(providerHeader.contains(QStringLiteral("ImageDocumentPageCandidateWatchProvider")));
    QVERIFY(providerImplementation.contains(QStringLiteral("KCoreDirLister")));
}

void TestArchitectureBoundaries::mediaFormatRegistryDoesNotOwnLocalizedDialogLabels()
{
    const QString mediaRegistryHeader
        = readProjectFile(QStringLiteral("src/navigation/mediaformatregistry.h"));
    const QString mediaRegistrySource
        = readProjectFile(QStringLiteral("src/navigation/mediaformatregistry.cpp"));
    const QString combined = mediaRegistryHeader + QLatin1Char('\n') + mediaRegistrySource;

    const QList<QRegularExpression> forbiddenPatterns {
        QRegularExpression(QStringLiteral(R"(KLocalizedString)")),
        QRegularExpression(QStringLiteral(R"(\bki?18nc?\s*\()")),
        QRegularExpression(QStringLiteral(R"(\bordinaryMediaOpenDialogNameFilters\b)")),
    };

    QStringList violations;
    for (const QRegularExpression &pattern : forbiddenPatterns) {
        QRegularExpressionMatchIterator iterator = pattern.globalMatch(combined);
        while (iterator.hasNext()) {
            violations.push_back(iterator.next().captured(0));
        }
    }

    QVERIFY2(violations.isEmpty(), qPrintable(violations.join(QLatin1Char('\n'))));
}

void TestArchitectureBoundaries::asyncImageIoJobsDoNotOwnDecodeDataLoading()
{
    const QStringList relativePaths = existingProjectFiles({
        QStringLiteral("src/async/imageiojobs.h"),
        QStringLiteral("src/async/imageiojobs.cpp"),
    });
    const QList<QRegularExpression> forbiddenPatterns {
        QRegularExpression(QStringLiteral(R"(#include\s+"decoding/imagedecoderequest\.h")")),
        QRegularExpression(QStringLiteral(R"(\bImageDecodeRequest\b)")),
        QRegularExpression(QStringLiteral(R"(\bstartStoredImageDataLoad\b)")),
        QRegularExpression(QStringLiteral(R"(\bKIO::storedGet\b)")),
    };

    QStringList violations;
    for (const QString &relativePath : relativePaths) {
        const QString matches = matchingLines(projectPath(relativePath), forbiddenPatterns);
        if (!matches.isEmpty()) {
            violations.push_back(matches);
        }
    }

    QVERIFY2(violations.isEmpty(), qPrintable(violations.join(QLatin1Char('\n'))));
}

void TestArchitectureBoundaries::asyncImageIoJobsDoNotOwnDirectoryCandidateLoading()
{
    const QStringList relativePaths = existingProjectFiles({
        QStringLiteral("src/async/imageiojobs.h"),
        QStringLiteral("src/async/imageiojobs.cpp"),
    });
    const QList<QRegularExpression> forbiddenPatterns {
        QRegularExpression(QStringLiteral(R"(#include\s+"async/directorylistingjob\.h")")),
        QRegularExpression(QStringLiteral(R"(\bDirectoryItemListProvider\b)")),
        QRegularExpression(QStringLiteral(R"(\bstartDirectoryImageDocumentPageCandidateList\b)")),
        QRegularExpression(QStringLiteral(R"(\bstartDirectoryContainerCandidateList\b)")),
    };

    QStringList violations;
    for (const QString &relativePath : relativePaths) {
        const QString matches = matchingLines(projectPath(relativePath), forbiddenPatterns);
        if (!matches.isEmpty()) {
            violations.push_back(matches);
        }
    }

    QVERIFY2(violations.isEmpty(), qPrintable(violations.join(QLatin1Char('\n'))));
}

void TestArchitectureBoundaries::asyncImageIoJobsDoNotOwnOpenedCollectionCandidateLoading()
{
    const QStringList relativePaths = existingProjectFiles({
        QStringLiteral("src/async/imageiojobs.h"),
        QStringLiteral("src/async/imageiojobs.cpp"),
    });
    const QList<QRegularExpression> forbiddenPatterns {
        QRegularExpression(QStringLiteral(R"(#include\s+"archive/mediaentrysourcebackend\.h")")),
        QRegularExpression(QStringLiteral(R"(\bMediaEntrySourceCandidatesResult\b)")),
        QRegularExpression(QStringLiteral(R"(\bloadMediaEntrySourceCandidates\b)")),
        QRegularExpression(QStringLiteral(R"(\bstartOpenedCollectionCandidateList\b)")),
    };

    QStringList violations;
    for (const QString &relativePath : relativePaths) {
        const QString matches = matchingLines(projectPath(relativePath), forbiddenPatterns);
        if (!matches.isEmpty()) {
            violations.push_back(matches);
        }
    }

    QVERIFY2(violations.isEmpty(), qPrintable(violations.join(QLatin1Char('\n'))));
}

void TestArchitectureBoundaries::mediaEntrySourceStoreDoesNotDependOnDocumentPlanning()
{
    const QList<QString> relativePaths {
        QStringLiteral("src/archive/mediaentrysourcestore.h"),
        QStringLiteral("src/archive/mediaentrysourcestore.cpp"),
    };
    const QList<QRegularExpression> forbiddenPatterns {
        QRegularExpression(QStringLiteral(R"(#include\s+"document/)")),
        QRegularExpression(QStringLiteral(R"(\bImageDocumentSourceLoadRequest\b)")),
        QRegularExpression(QStringLiteral(R"(\bopenedCollectionScopeLoadPlan\s*\()")),
    };

    QStringList violations;
    for (const QString &relativePath : relativePaths) {
        const QString matches = matchingLines(projectPath(relativePath), forbiddenPatterns);
        if (!matches.isEmpty()) {
            violations.push_back(matches);
        }
    }

    QVERIFY2(violations.isEmpty(), qPrintable(violations.join(QLatin1Char('\n'))));
}

void TestArchitectureBoundaries::imagePageSurfaceOwnerTypeExists()
{
    const QString headerPath
        = projectPath(QStringLiteral("src/presentation/imagepagesurfacecontroller.h"));
    QFileInfo header(headerPath);

    QVERIFY2(header.exists(),
        qPrintable(QStringLiteral("%1 must define the page surface owner")
                .arg(relativeProjectPath(headerPath))));
    QVERIFY(readProjectFile(QStringLiteral("src/presentation/imagepagesurfacecontroller.h"))
            .contains(QStringLiteral("class ImagePageSurfaceController")));
}

void TestArchitectureBoundaries::imagePageSurfaceOwnersExposeNoPresentationState()
{
    const QString relativePath = QStringLiteral("src/presentation/imagepagesurfacecontroller.h");
    const QString headerPath = projectPath(relativePath);
    QVERIFY2(QFileInfo::exists(headerPath),
        qPrintable(QStringLiteral("%1 must define the page surface owner")
                .arg(relativeProjectPath(headerPath))));

    const QString header = readProjectFile(relativePath);
    const QList<QRegularExpression> forbiddenPatterns {
        QRegularExpression(QStringLiteral(R"(\bsetViewportSize\s*\()")),
        QRegularExpression(QStringLiteral(R"(\bviewportSize\s*\()")),
        QRegularExpression(QStringLiteral(R"(\bsetVisibleItemRect\s*\()")),
        QRegularExpression(QStringLiteral(R"(\bvisibleItemRect\s*\()")),
        QRegularExpression(QStringLiteral(R"(\bsetZoomPercent\s*\()")),
        QRegularExpression(QStringLiteral(R"(\bzoomPercent\s*\()")),
        QRegularExpression(QStringLiteral(R"(\bzoomMode\s*\()")),
        QRegularExpression(QStringLiteral(R"(\bsetFitMode\s*\()")),
        QRegularExpression(QStringLiteral(R"(\bresetZoom\s*\()")),
        QRegularExpression(QStringLiteral(R"(\brotateClockwise\s*\()")),
        QRegularExpression(QStringLiteral(R"(\brotateCounterclockwise\s*\()")),
        QRegularExpression(QStringLiteral(R"(\bresetRotation\s*\()")),
        QRegularExpression(QStringLiteral(R"(\brotationDegrees\s*\()")),
        QRegularExpression(QStringLiteral(R"(\brenderSnapshot\s*\()")),
    };

    QStringList violations;
    for (const QRegularExpression &pattern : forbiddenPatterns) {
        QRegularExpressionMatchIterator iterator = pattern.globalMatch(header);
        while (iterator.hasNext()) {
            violations.push_back(iterator.next().captured(0));
        }
    }

    QVERIFY2(violations.isEmpty(), qPrintable(violations.join(QLatin1Char('\n'))));
}

void TestArchitectureBoundaries::activePresentationDoesNotWritePageSurfacePresentationState()
{
    const QList<QString> relativePaths {
        QStringLiteral("src/presentation/imagepresentationruntime.cpp"),
        QStringLiteral("src/presentation/imagespreadpresentationcontroller.cpp"),
    };
    const QList<QRegularExpression> forbiddenPatterns {
        QRegularExpression(QStringLiteral(R"(\bsetVisibleItemRect\s*\()")),
        QRegularExpression(QStringLiteral(
            R"(\b(?:m_primaryPageSurface|primaryPageSurface|m_secondaryPageController|pageSurfaceController)\b[^\n]*(?:\.|->)\s*(?:setViewportSize|setZoomPercent|setFitMode|resetZoom|rotateClockwise|rotateCounterclockwise|resetRotation)\s*\()")),
    };

    QStringList violations;
    for (const QString &relativePath : relativePaths) {
        const QString path = projectPath(relativePath);
        if (!QFileInfo::exists(path)) {
            continue;
        }

        const QString matches = matchingLines(path, forbiddenPatterns);
        if (!matches.isEmpty()) {
            violations.push_back(matches);
        }
    }

    QVERIFY2(violations.isEmpty(), qPrintable(violations.join(QLatin1Char('\n'))));
}

void TestArchitectureBoundaries::productionFacadesDoNotExposePresentationBackdoorSetters()
{
    const QList<QString> relativePaths {
        QStringLiteral("src/facade/kiriimagedocument.h"),
        QStringLiteral("src/document/imagedocumentruntime.h"),
        QStringLiteral("src/presentation/imagespreadpresentationcontroller.h"),
    };
    const QList<QRegularExpression> forbiddenPatterns {
        QRegularExpression(QStringLiteral(R"(\bsetViewportContentPosition\s*\()")),
        QRegularExpression(QStringLiteral(R"(\bsetVisibleItemRect\s*\()")),
        QRegularExpression(QStringLiteral(R"(\bsetZoomPercent\s*\()")),
    };

    QStringList violations;
    for (const QString &relativePath : relativePaths) {
        const QString contents = readProjectFile(relativePath);
        for (const QRegularExpression &pattern : forbiddenPatterns) {
            QRegularExpressionMatchIterator iterator = pattern.globalMatch(contents);
            while (iterator.hasNext()) {
                violations.push_back(
                    QStringLiteral("%1: %2").arg(relativePath, iterator.next().captured(0)));
            }
        }
    }

    QVERIFY2(violations.isEmpty(), qPrintable(violations.join(QLatin1Char('\n'))));
}

void TestArchitectureBoundaries::mediaInformationFacadeExposesSnapshotRevision()
{
    const QString header = readProjectFile(QStringLiteral("src/facade/kirimediainformation.h"));
    QVERIFY(header.contains(
        QStringLiteral("Q_PROPERTY(quint64 revision READ revision NOTIFY changed)")));
}

void TestArchitectureBoundaries::sessionLeafSnapshotPortsAreSeparateFromCommandPorts()
{
    const QString header
        = readProjectFile(QStringLiteral("src/session/documentsessiondocumentports.h"));
    const QStringList snapshotPorts {
        QStringLiteral("DocumentSessionImageDocumentSnapshotPort"),
        QStringLiteral("DocumentSessionVideoDocumentSnapshotPort"),
    };
    const QStringList commandPorts {
        QStringLiteral("DocumentSessionImageDocumentCommandPort"),
        QStringLiteral("DocumentSessionVideoDocumentCommandPort"),
    };

    QVERIFY(!header.contains(QStringLiteral("struct DocumentSessionImageDocumentPort")));
    QVERIFY(!header.contains(QStringLiteral("struct DocumentSessionVideoDocumentPort")));

    for (const QString &port : snapshotPorts + commandPorts) {
        const QRegularExpression structPattern(QStringLiteral(R"(struct\s+%1\s*\{([\s\S]*?)\n\};)")
                .arg(QRegularExpression::escape(port)));
        const QRegularExpressionMatch match = structPattern.match(header);
        QVERIFY2(match.hasMatch(),
            qPrintable(QStringLiteral("documentsessiondocumentports.h must define %1").arg(port)));
    }

    const QStringList snapshotForbiddenTokens {
        QStringLiteral("setSourceUrl"),
        QStringLiteral("openPreviousPage"),
        QStringLiteral("openNextPage"),
        QStringLiteral("openImageAtPage"),
        QStringLiteral("deleteDisplayedFile"),
        QStringLiteral("videoOutput"),
        QStringLiteral("stop"),
        QStringLiteral("setVideoOutput"),
        QStringLiteral("setVideoOutputGeometry"),
    };
    const QStringList commandForbiddenTokens {
        QStringLiteral("snapshot"),
        QStringLiteral("snapshotChanged"),
    };

    QStringList violations;
    for (const QString &port : snapshotPorts + commandPorts) {
        const QRegularExpression structPattern(QStringLiteral(R"(struct\s+%1\s*\{([\s\S]*?)\n\};)")
                .arg(QRegularExpression::escape(port)));
        const QString body = structPattern.match(header).captured(1);
        const QStringList forbiddenTokens
            = snapshotPorts.contains(port) ? snapshotForbiddenTokens : commandForbiddenTokens;
        for (const QString &token : forbiddenTokens) {
            if (body.contains(token)) {
                violations.push_back(QStringLiteral("%1 contains %2").arg(port, token));
            }
        }
    }

    QVERIFY2(violations.isEmpty(), qPrintable(violations.join(QLatin1Char('\n'))));
}

QTEST_GUILESS_MAIN(TestArchitectureBoundaries)

#include "test_architectureboundaries.moc"
