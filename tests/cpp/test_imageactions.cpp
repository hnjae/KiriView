// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "facade/kiridocumentsession.h"
#include "facade/kiriimagedocument.h"
#include "facade/kirimediainformation.h"
#include "facade/kirivideodocument.h"
#include "facade/kiriviewapplication.h"
#include "localization/localization.h"

#include <KLocalizedQmlContext>
#include <KZip>
#include <QAction>
#include <QBuffer>
#include <QCoreApplication>
#include <QDir>
#include <QImage>
#include <QObject>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QString>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>
#include <QVariant>
#include <QtQml/qqml.h>
#include <memory>

class TestImageActions : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void previousNextAvailabilityFollowsSessionActiveNavigation();
    void firstLastDispatchUsesSessionActiveNavigation();
    void boundaryFeedbackUsesSessionBoundaryScope();
};

namespace {
struct ImageActionsFixture {
    std::unique_ptr<QQmlEngine> engine;
    std::unique_ptr<QObject> root;
    std::unique_ptr<QTemporaryDir> temporaryDirectory;
    KiriDocumentSession *documentSession = nullptr;
    KiriViewApplication *application = nullptr;
    QString errorString;

    bool isValid() const
    {
        return engine != nullptr && root != nullptr && documentSession != nullptr
            && application != nullptr;
    }
};

void addEnvironmentImportPaths(QQmlEngine &engine)
{
    const QString paths = qEnvironmentVariable("NIXPKGS_QML_SEARCH_PATHS");
    for (const QString &path : paths.split(QLatin1Char(':'), Qt::SkipEmptyParts)) {
        engine.addImportPath(path);
    }
}

void registerKiriViewQmlTypes()
{
    static bool registered = false;
    if (registered) {
        return;
    }

    KiriView::initializeLocalization();
    qmlRegisterType<KiriViewApplication>("io.github.hnjae.kiriview", 1, 0, "KiriViewApplication");
    qmlRegisterType<KiriDocumentSession>("io.github.hnjae.kiriview", 1, 0, "KiriDocumentSession");
    qmlRegisterType<KiriImageDocument>("io.github.hnjae.kiriview", 1, 0, "KiriImageDocument");
    qmlRegisterType<KiriMediaInformation>("io.github.hnjae.kiriview", 1, 0, "KiriMediaInformation");
    qmlRegisterType<KiriVideoDocument>("io.github.hnjae.kiriview", 1, 0, "KiriVideoDocument");
    registered = true;
}

QString qmlSourceImport()
{
    const QString qmlPath = QDir(QStringLiteral(KIRIVIEW_TEST_SOURCE_DIR))
                                .absoluteFilePath(QStringLiteral("../../src/qml"));
    return QUrl::fromLocalFile(qmlPath).toString();
}

std::unique_ptr<QTemporaryDir> createComicBookArchive(QString *sourcePath, QString *errorString)
{
    auto directory = std::make_unique<QTemporaryDir>();
    if (!directory->isValid()) {
        *errorString = QStringLiteral("temporary archive directory was not created");
        return nullptr;
    }

    QImage image(QSize(2, 4), QImage::Format_RGBA8888);
    image.fill(Qt::blue);
    QByteArray imageData;
    QBuffer imageBuffer(&imageData);
    if (!imageBuffer.open(QIODevice::WriteOnly) || !image.save(&imageBuffer, "PNG")) {
        *errorString = QStringLiteral("failed to encode test archive image");
        return nullptr;
    }

    *sourcePath = directory->filePath(QStringLiteral("book.cbz"));
    KZip archive(*sourcePath);
    if (!archive.open(QIODevice::WriteOnly)) {
        *errorString = QStringLiteral("failed to open test archive");
        return nullptr;
    }

    for (int index = 1; index <= 3; ++index) {
        const QString entryName = QStringLiteral("%1.png").arg(index, 2, 10, QLatin1Char('0'));
        if (!archive.writeFile(entryName, imageData)) {
            *errorString = QStringLiteral("failed to write archive entry");
            archive.close();
            return nullptr;
        }
    }
    archive.close();
    return directory;
}

QString fixtureQml(const QString &sourceUrl)
{
    return QStringLiteral(R"(
import QtQuick
import io.github.hnjae.kiriview
import "%1" as KiriViewQml

Item {
    id: root

    property string lastBoundaryMessage: ""
    property bool previousProxyEnabled: imageActions.previousImageAction.enabled
    property bool nextProxyEnabled: imageActions.nextImageAction.enabled
    property bool firstProxyEnabled: imageActions.firstImageAction.enabled
    property bool lastProxyEnabled: imageActions.lastImageAction.enabled
    property bool previousProxyVisible: imageActions.previousImageAction.visible
    property bool nextProxyVisible: imageActions.nextImageAction.visible
    property bool previousQActionEnabled: application.actionForId(KiriViewApplication.GoPreviousImageAction).enabled
    property bool nextQActionEnabled: application.actionForId(KiriViewApplication.GoNextImageAction).enabled
    property bool firstQActionEnabled: application.actionForId(KiriViewApplication.GoFirstImageAction).enabled
    property bool lastQActionEnabled: application.actionForId(KiriViewApplication.GoLastImageAction).enabled
    property string firstMenuText: imageActions.firstImageMenuAction.text
    property string lastMenuText: imageActions.lastImageMenuAction.text

    function triggerPrevious() {
        application.actionForId(KiriViewApplication.GoPreviousImageAction).trigger();
    }

    function triggerNext() {
        application.actionForId(KiriViewApplication.GoNextImageAction).trigger();
    }

    function triggerFirst() {
        application.actionForId(KiriViewApplication.GoFirstImageAction).trigger();
    }

    function triggerLast() {
        application.actionForId(KiriViewApplication.GoLastImageAction).trigger();
    }

    function publishActionUiState() {
        application.updateActionUiGateSnapshot(1, false, false, false, false, false, false, true, true);
    }

    Component.onCompleted: {
        application.setDocumentSession(documentSession);
        publishActionUiState();
    }

    KiriViewApplication {
        id: application
        objectName: "application"
    }

    KiriDocumentSession {
        id: documentSession
        objectName: "documentSession"
        sourceUrl: "%2"
    }

    KiriViewQml.ImageActions {
        id: imageActions

        application: application
        documentSession: documentSession
        imageDocument: documentSession.imageDocument
    }

    Connections {
        target: application

        function onImageBoundaryReached(message) {
            root.lastBoundaryMessage = message;
        }
    }
}
)")
        .arg(qmlSourceImport(), sourceUrl);
}

ImageActionsFixture createFixture(const QString &sourceUrl = QString())
{
    ImageActionsFixture fixture;
    registerKiriViewQmlTypes();
    fixture.engine = std::make_unique<QQmlEngine>();
    addEnvironmentImportPaths(*fixture.engine);
    fixture.engine->addImportPath(QDir(QStringLiteral(KIRIVIEW_TEST_SOURCE_DIR))
            .absoluteFilePath(QStringLiteral("../../src/qml")));
    KLocalization::setupLocalizedContext(fixture.engine.get());

    QQmlComponent component(fixture.engine.get());
    component.setData(
        fixtureQml(sourceUrl).toUtf8(), QUrl(QStringLiteral("memory:test_imageactions.qml")));
    for (int attempt = 0; component.isLoading() && attempt < 100; ++attempt) {
        QCoreApplication::processEvents();
        QTest::qWait(10);
    }
    if (component.isLoading()) {
        fixture.errorString = QStringLiteral("QML component did not become ready");
        return fixture;
    }
    if (component.isError()) {
        fixture.errorString = component.errorString();
        return fixture;
    }

    QObject *root = component.create();
    if (root == nullptr) {
        fixture.errorString = component.errorString();
        return fixture;
    }

    fixture.root.reset(root);
    fixture.documentSession
        = root->findChild<KiriDocumentSession *>(QStringLiteral("documentSession"));
    fixture.application = root->findChild<KiriViewApplication *>(QStringLiteral("application"));
    if (!fixture.isValid()) {
        fixture.errorString = QStringLiteral("fixture did not create required objects");
    }
    return fixture;
}

bool invoke(QObject &object, const char *method)
{
    return QMetaObject::invokeMethod(&object, method, Qt::DirectConnection);
}
}

void TestImageActions::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    KiriView::initializeLocalization();
}

void TestImageActions::previousNextAvailabilityFollowsSessionActiveNavigation()
{
    ImageActionsFixture emptyFixture = createFixture();
    QVERIFY2(emptyFixture.isValid(), qPrintable(emptyFixture.errorString));
    QVERIFY(!emptyFixture.root->property("previousQActionEnabled").toBool());
    QVERIFY(!emptyFixture.root->property("nextQActionEnabled").toBool());
    QVERIFY(!emptyFixture.root->property("previousProxyEnabled").toBool());
    QVERIFY(!emptyFixture.root->property("nextProxyEnabled").toBool());
    QVERIFY(!emptyFixture.root->property("previousProxyVisible").toBool());
    QVERIFY(!emptyFixture.root->property("nextProxyVisible").toBool());

    QString errorString;
    QString archivePath;
    auto archiveDirectory = createComicBookArchive(&archivePath, &errorString);
    QVERIFY2(archiveDirectory != nullptr, qPrintable(errorString));
    ImageActionsFixture archiveFixture = createFixture(QUrl::fromLocalFile(archivePath).toString());
    archiveFixture.temporaryDirectory = std::move(archiveDirectory);
    QVERIFY2(archiveFixture.isValid(), qPrintable(archiveFixture.errorString));
    QTRY_VERIFY(archiveFixture.documentSession->activeNavigationKnown());
    QVERIFY(archiveFixture.root->property("previousQActionEnabled").toBool());
    QVERIFY(archiveFixture.root->property("nextQActionEnabled").toBool());
    QVERIFY(archiveFixture.root->property("previousProxyEnabled").toBool());
    QVERIFY(archiveFixture.root->property("nextProxyEnabled").toBool());
    QVERIFY(!archiveFixture.root->property("previousProxyVisible").toBool());
    QVERIFY(archiveFixture.root->property("nextProxyVisible").toBool());

    archiveFixture.documentSession->openActiveNavigationAtNumber(2);
    QTRY_COMPARE(archiveFixture.documentSession->activeNavigationCurrentNumber(), 2);
    QVERIFY(archiveFixture.root->property("previousQActionEnabled").toBool());
    QVERIFY(archiveFixture.root->property("nextQActionEnabled").toBool());
    QVERIFY(archiveFixture.root->property("previousProxyEnabled").toBool());
    QVERIFY(archiveFixture.root->property("nextProxyEnabled").toBool());
    QVERIFY(archiveFixture.root->property("previousProxyVisible").toBool());
    QVERIFY(archiveFixture.root->property("nextProxyVisible").toBool());
}

void TestImageActions::firstLastDispatchUsesSessionActiveNavigation()
{
    QString errorString;
    QString archivePath;
    auto archiveDirectory = createComicBookArchive(&archivePath, &errorString);
    QVERIFY2(archiveDirectory != nullptr, qPrintable(errorString));
    ImageActionsFixture archiveFixture = createFixture(QUrl::fromLocalFile(archivePath).toString());
    archiveFixture.temporaryDirectory = std::move(archiveDirectory);
    QVERIFY2(archiveFixture.isValid(), qPrintable(archiveFixture.errorString));
    QTRY_VERIFY(archiveFixture.documentSession->activeNavigationKnown());

    QVERIFY(invoke(*archiveFixture.root, "triggerLast"));
    QTRY_COMPARE(archiveFixture.documentSession->activeNavigationCurrentNumber(), 3);
    QVERIFY(invoke(*archiveFixture.root, "triggerFirst"));
    QTRY_COMPARE(archiveFixture.documentSession->activeNavigationCurrentNumber(), 1);
}

void TestImageActions::boundaryFeedbackUsesSessionBoundaryScope()
{
    QString errorString;
    QString archivePath;
    auto archiveDirectory = createComicBookArchive(&archivePath, &errorString);
    QVERIFY2(archiveDirectory != nullptr, qPrintable(errorString));
    ImageActionsFixture archiveFixture = createFixture(QUrl::fromLocalFile(archivePath).toString());
    archiveFixture.temporaryDirectory = std::move(archiveDirectory);
    QVERIFY2(archiveFixture.isValid(), qPrintable(archiveFixture.errorString));
    QTRY_VERIFY(archiveFixture.documentSession->atKnownFirstActiveNavigation());

    QVERIFY(invoke(*archiveFixture.root, "triggerPrevious"));
    QCOMPARE(archiveFixture.root->property("lastBoundaryMessage").toString(),
        QStringLiteral("First image"));
}

QTEST_MAIN(TestImageActions)

#include "test_imageactions.moc"
