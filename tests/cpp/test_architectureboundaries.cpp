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
    void qmlDoesNotOwnSharedActionPolicy();
    void qmlDoesNotRecomputeSharedMediaReadiness();
    void qmlDoesNotWriteDurablePresentationState();
    void imageDocumentHasNoPublicPresentationBackdoorSetters();
    void qmlViewportUsesRevisionedCommandAcknowledgement();
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
    void sourceKeysExposeTypedExtensionFamilies();
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

void TestArchitectureBoundaries::mediaInformationFacadeExposesSnapshotRevision()
{
    const QString header = readProjectFile(QStringLiteral("src/facade/kirimediainformation.h"));
    QVERIFY(header.contains(
        QStringLiteral("Q_PROPERTY(quint64 revision READ revision NOTIFY changed)")));
}

QTEST_GUILESS_MAIN(TestArchitectureBoundaries)

#include "test_architectureboundaries.moc"
