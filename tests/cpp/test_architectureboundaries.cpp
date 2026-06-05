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
    void qmlDoesNotRecomputeSharedMediaReadiness();
    void qmlDoesNotWriteDurablePresentationState();
    void imageDocumentHasNoPublicPresentationBackdoorSetters();
    void qmlViewportUsesRevisionedCommandAcknowledgement();
    void qmlViewportUsesOpaqueRevisionTokens();
    void leafDocumentsAreNotProductionQmlCreatable();
    void actionUiGatesAreRevisionedSnapshots();
    void imageActionAvailabilityFacadeIsNotWritableQmlBackdoor();
    void fixedViewerShortcutsDoNotBypassRuntimeRouting();
    void sessionPublicProjectionHasNoPartialUpdateBackdoor();
    void sessionPublicProjectionDoesNotSampleLeafFacadesWhileApplying();
    void qmlDoesNotWriteSharedVideoOutputAttachment();
    void videoOutputAttachmentIsNotWritablePublicVideoDocumentState();
    void qmlDoesNotDeriveSharedControlPolicyFromLeafDocuments();
    void qmlViewportUsesFullCommandLifecycle();
    void viewportContextBridgeIsNonRenderingPublicQtFacade();
    void qmlViewportUsesContextBridgeForRenderContextDiscovery();
    void oldImageRendererArtifactsAreAbsent();
    void oldImageRendererBuildWiringIsAbsent();
    void productionImageDisplayUsesProviderPathOnly();
    void sourceKeysExposeTypedExtensionFamilies();
    void sourceKeysExposeOperationalExtensionContracts();
    void imagePageSurfaceOwnerTypeExists();
    void imagePageSurfaceOwnersExposeNoPresentationState();
    void activePresentationDoesNotWritePageSurfacePresentationState();
    void productionFacadesDoNotExposePresentationBackdoorSetters();
    void mediaInformationFacadeExposesSnapshotRevision();
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

QTEST_GUILESS_MAIN(TestArchitectureBoundaries)

#include "test_architectureboundaries.moc"
