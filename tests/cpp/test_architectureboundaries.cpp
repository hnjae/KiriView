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
    void qmlViewportUsesRevisionedCommandAcknowledgement();
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

QTEST_GUILESS_MAIN(TestArchitectureBoundaries)

#include "test_architectureboundaries.moc"
