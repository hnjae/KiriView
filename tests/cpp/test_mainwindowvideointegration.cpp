// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "facade/kiridocumentsession.h"
#include "facade/kiriviewapplication.h"
#include "localization/localization.h"
#include "session/documentsessionruntime.h"

#include "qml_component_test_support.h"

#include <KLocalizedQmlContext>
#include <QCoreApplication>
#include <QDir>
#include <QMetaMethod>
#include <QMetaProperty>
#include <QObject>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickStyle>
#include <QQuickView>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QString>
#include <QTest>
#include <QUrl>
#include <QVariant>
#include <QtQml/qqml.h>
#include <memory>
#include <type_traits>
#include <utility>

class TestMainWindowVideoIntegration : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void init();
    void pageNavigationButtonsTriggerActiveNavigationActions();
    void pageNavigationEditingDispatchesActiveNavigationCallback();
    void pageNavigationUnavailableStateBlocksEditing();
    void documentSessionFacadeExposesOnlySharedActiveNavigationSurface();
    void documentSessionRuntimePublicApiExposesOnlySharedActiveNavigationSurface();
};

namespace {
struct PageNavigationFixture
{
    std::unique_ptr<QQmlComponent> component;
    std::unique_ptr<QQuickView> view;
    QQuickItem* root = nullptr;
    QQuickItem* pageNavigation = nullptr;
    QQuickItem* pageNumberField = nullptr;
    QQuickItem* leftButton = nullptr;
    QQuickItem* rightButton = nullptr;
    QString errorString;

    bool isValid() const
    {
        return component != nullptr && view != nullptr && root != nullptr
            && pageNavigation != nullptr && pageNumberField != nullptr && leftButton != nullptr
            && rightButton != nullptr;
    }
};

template <typename, typename = void> struct HasOpenPreviousActiveNavigation : std::false_type
{
};

template <typename T>
struct HasOpenPreviousActiveNavigation<T,
    std::void_t<decltype(std::declval<T&>().openPreviousActiveNavigation())>> : std::true_type
{
};

template <typename, typename = void> struct HasOpenNextActiveNavigation : std::false_type
{
};

template <typename T>
struct HasOpenNextActiveNavigation<T,
    std::void_t<decltype(std::declval<T&>().openNextActiveNavigation())>> : std::true_type
{
};

template <typename, typename = void> struct HasOpenActiveNavigationAtNumber : std::false_type
{
};

template <typename T>
struct HasOpenActiveNavigationAtNumber<T,
    std::void_t<decltype(std::declval<T&>().openActiveNavigationAtNumber(1))>> : std::true_type
{
};

template <typename, typename = void> struct HasOpenPreviousMedia : std::false_type
{
};

template <typename T>
struct HasOpenPreviousMedia<T, std::void_t<decltype(std::declval<T&>().openPreviousMedia())>>
    : std::true_type
{
};

template <typename, typename = void> struct HasOpenNextMedia : std::false_type
{
};

template <typename T>
struct HasOpenNextMedia<T, std::void_t<decltype(std::declval<T&>().openNextMedia())>>
    : std::true_type
{
};

template <typename, typename = void> struct HasOpenMediaAtNumber : std::false_type
{
};

template <typename T>
struct HasOpenMediaAtNumber<T, std::void_t<decltype(std::declval<T&>().openMediaAtNumber(1))>>
    : std::true_type
{
};

template <typename, typename = void> struct HasCurrentMediaNumber : std::false_type
{
};

template <typename T>
struct HasCurrentMediaNumber<T,
    std::void_t<decltype(std::declval<const T&>().currentMediaNumber())>> : std::true_type
{
};

template <typename, typename = void> struct HasMediaCount : std::false_type
{
};

template <typename T>
struct HasMediaCount<T, std::void_t<decltype(std::declval<const T&>().mediaCount())>>
    : std::true_type
{
};

template <typename, typename = void> struct HasDirectMediaNavigationKnown : std::false_type
{
};

template <typename T>
struct HasDirectMediaNavigationKnown<T,
    std::void_t<decltype(std::declval<const T&>().directMediaNavigationKnown())>> : std::true_type
{
};

void addEnvironmentImportPaths(QQmlEngine& engine)
{
    const QString paths = qEnvironmentVariable("NIXPKGS_QML_SEARCH_PATHS");
    for (const QString& path : paths.split(QLatin1Char(':'), Qt::SkipEmptyParts)) {
        engine.addImportPath(path);
    }
}

QString qmlSourceImport()
{
    const QString qmlPath = QDir(QStringLiteral(KIRIVIEW_TEST_SOURCE_DIR))
                                .absoluteFilePath(QStringLiteral("../../src/qml"));
    return QUrl::fromLocalFile(qmlPath).toString();
}

QString pageNavigationFixtureQml()
{
    return QStringLiteral(R"(
import QtQuick
import QtQuick.Controls as Controls
import org.hnjae.kiriview
import "%1" as KiriViewQml

Item {
    id: root

    objectName: "pageNavigationFixture"
    focus: true
    width: 360
    height: 96

    property bool activeNavigationAvailable: true
    property int activeNavigationCount: 9
    property int activeNavigationCurrentNumber: 3
    property bool activeNavigationEditable: true
    property bool activeNavigationKnown: true
    property bool nextEnabled: true
    property bool previousEnabled: true
    property bool editingReturnedFocus: false
    property int nextTriggerCount: 0
    property int openedNumber: -1
    property int previousTriggerCount: 0
    property alias rightToLeftPresentationActive: navigationPresentationProvider.rightToLeftReadingActive

    function openActiveNavigationAtNumber(number) {
        openedNumber = number;
        activeNavigationCurrentNumber = number;
    }

    Controls.Action {
        id: previousAction

        enabled: root.previousEnabled
        text: "Previous"

        onTriggered: root.previousTriggerCount += 1
    }

    Controls.Action {
        id: nextAction

        enabled: root.nextEnabled
        text: "Next"

        onTriggered: root.nextTriggerCount += 1
    }

    QtObject {
        id: actions

        property var nextImageAction: nextAction
        property var previousImageAction: previousAction
    }

    QtObject {
        id: navigationPresentationProvider

        property bool rightToLeftReadingActive: false
        property int actionStateRevision: rightToLeftReadingActive ? 1 : 0

        function navigationPresentationActionId(slot) {
            switch (slot) {
            case KiriViewApplication.LeadingImageActionSlot:
                return rightToLeftReadingActive ? KiriViewApplication.GoNextImageAction : KiriViewApplication.GoPreviousImageAction;
            case KiriViewApplication.TrailingImageActionSlot:
                return rightToLeftReadingActive ? KiriViewApplication.GoPreviousImageAction : KiriViewApplication.GoNextImageAction;
            default:
                return KiriViewApplication.ActionCount;
            }
        }

        function navigationPresentationIconActionId(slot) {
            switch (slot) {
            case KiriViewApplication.LeadingImageActionSlot:
                return KiriViewApplication.GoPreviousImageAction;
            case KiriViewApplication.TrailingImageActionSlot:
                return KiriViewApplication.GoNextImageAction;
            default:
                return KiriViewApplication.ActionCount;
            }
        }

        function navigationApplicationMenuActionIds() {
            return [];
        }
    }

    KiriViewQml.ImageDocumentPageNavigation {
        id: pageNavigation

        objectName: "pageNavigation"
        anchors.centerIn: parent
        activeNavigationAvailable: root.activeNavigationAvailable
        activeNavigationCount: root.activeNavigationCount
        activeNavigationCurrentNumber: root.activeNavigationCurrentNumber
        activeNavigationEditable: root.activeNavigationEditable
        activeNavigationKnown: root.activeNavigationKnown
        actions: actions
        navigationPresentationProvider: navigationPresentationProvider
        openActiveNavigationAtNumber: root.openActiveNavigationAtNumber

        onEditingCompleted: function (returnViewerFocus) {
            root.editingReturnedFocus = returnViewerFocus;
        }
    }
}
)")
        .arg(qmlSourceImport());
}

PageNavigationFixture createPageNavigationFixture()
{
    PageNavigationFixture fixture;
    fixture.view = std::make_unique<QQuickView>();
    fixture.view->resize(360, 96);
    fixture.view->setResizeMode(QQuickView::SizeRootObjectToView);
    addEnvironmentImportPaths(*fixture.view->engine());
    fixture.view->engine()->addImportPath(QDir(QStringLiteral(KIRIVIEW_TEST_SOURCE_DIR))
            .absoluteFilePath(QStringLiteral("../../src/qml")));
    KLocalization::setupLocalizedContext(fixture.view->engine());

    fixture.component = std::make_unique<QQmlComponent>(fixture.view->engine());
    fixture.component->setData(
        pageNavigationFixtureQml().toUtf8(), QUrl(QStringLiteral("memory:page_navigation.qml")));
    if (!waitForQmlComponentReady(*fixture.component)) {
        fixture.errorString = QStringLiteral("QML component did not become ready");
        return fixture;
    }
    if (fixture.component->isError()) {
        fixture.errorString = fixture.component->errorString();
        return fixture;
    }

    QObject* createdRoot = fixture.component->create();
    if (createdRoot == nullptr) {
        fixture.errorString = fixture.component->errorString();
        return fixture;
    }

    fixture.root = qobject_cast<QQuickItem*>(createdRoot);
    if (fixture.root == nullptr) {
        fixture.errorString = QStringLiteral("fixture root is not a QQuickItem");
        delete createdRoot;
        return fixture;
    }

    fixture.view->setContent(
        QUrl(QStringLiteral("memory:page_navigation.qml")), fixture.component.get(), createdRoot);
    fixture.view->show();
    if (!QTest::qWaitForWindowExposed(fixture.view.get())) {
        fixture.errorString = QStringLiteral("test window was not exposed");
        return fixture;
    }

    fixture.pageNavigation = fixture.root->findChild<QQuickItem*>(QStringLiteral("pageNavigation"));
    fixture.pageNumberField
        = fixture.root->findChild<QQuickItem*>(QStringLiteral("pageNumberField"));
    fixture.leftButton
        = fixture.root->findChild<QQuickItem*>(QStringLiteral("leftPageNavigationButton"));
    fixture.rightButton
        = fixture.root->findChild<QQuickItem*>(QStringLiteral("rightPageNavigationButton"));
    if (!fixture.isValid()) {
        fixture.errorString = QStringLiteral("fixture did not create required objects");
    }
    return fixture;
}

QPoint itemCenter(QQuickItem* item)
{
    if (item == nullptr || item->width() <= 0 || item->height() <= 0) {
        return QPoint(-1, -1);
    }
    return item->mapToScene(QPointF(item->width() / 2.0, item->height() / 2.0)).toPoint();
}

void clickItem(QQuickView* view, QQuickItem* item)
{
    const QPoint point = itemCenter(item);
    QVERIFY(point.x() >= 0);
    QVERIFY(point.y() >= 0);
    QTest::mouseClick(view, Qt::LeftButton, Qt::NoModifier, point);
    QCoreApplication::processEvents();
}

bool invokeCommitEditing(QQuickItem* pageNavigation, bool returnViewerFocus)
{
    return QMetaObject::invokeMethod(pageNavigation, "commitEditing", Qt::DirectConnection,
        Q_ARG(QVariant, QVariant(returnViewerFocus)));
}

void focusPageNumberField(QQuickItem* pageNumberField)
{
    pageNumberField->forceActiveFocus();
    QTRY_VERIFY(pageNumberField->hasActiveFocus());
}

bool metaObjectHasProperty(const QMetaObject& metaObject, const char* name)
{
    return metaObject.indexOfProperty(name) >= 0;
}

bool metaObjectHasMethod(const QMetaObject& metaObject, const char* signature)
{
    return metaObject.indexOfMethod(QMetaObject::normalizedSignature(signature)) >= 0;
}

bool metaObjectExposesName(const QMetaObject& metaObject, const QByteArray& name)
{
    for (int index = metaObject.propertyOffset(); index < metaObject.propertyCount(); ++index) {
        if (metaObject.property(index).name() == name) {
            return true;
        }
    }
    for (int index = metaObject.methodOffset(); index < metaObject.methodCount(); ++index) {
        if (metaObject.method(index).name() == name) {
            return true;
        }
    }
    return false;
}
}

void TestMainWindowVideoIntegration::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    kiriview::initializeLocalization();
    qmlRegisterType<KiriViewApplication>("org.hnjae.kiriview", 1, 0, "KiriViewApplication");
    if (!qEnvironmentVariableIsSet("QT_QUICK_CONTROLS_STYLE")) {
        QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));
    }
}

void TestMainWindowVideoIntegration::init()
{
    QTest::failOnWarning(QRegularExpression(
        QStringLiteral(".*Created graphical object was not placed in the graphics scene.*")));
}

void TestMainWindowVideoIntegration::pageNavigationButtonsTriggerActiveNavigationActions()
{
    PageNavigationFixture fixture = createPageNavigationFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));

    clickItem(fixture.view.get(), fixture.leftButton);
    QCOMPARE(fixture.root->property("previousTriggerCount").toInt(), 1);
    QCOMPARE(fixture.root->property("nextTriggerCount").toInt(), 0);

    clickItem(fixture.view.get(), fixture.rightButton);
    QCOMPARE(fixture.root->property("previousTriggerCount").toInt(), 1);
    QCOMPARE(fixture.root->property("nextTriggerCount").toInt(), 1);

    QVERIFY(fixture.root->setProperty("rightToLeftPresentationActive", true));
    QCoreApplication::processEvents();
    clickItem(fixture.view.get(), fixture.leftButton);
    QCOMPARE(fixture.root->property("previousTriggerCount").toInt(), 1);
    QCOMPARE(fixture.root->property("nextTriggerCount").toInt(), 2);

    clickItem(fixture.view.get(), fixture.rightButton);
    QCOMPARE(fixture.root->property("previousTriggerCount").toInt(), 2);
    QCOMPARE(fixture.root->property("nextTriggerCount").toInt(), 2);
}

void TestMainWindowVideoIntegration::pageNavigationEditingDispatchesActiveNavigationCallback()
{
    PageNavigationFixture fixture = createPageNavigationFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    QTRY_COMPARE(fixture.pageNumberField->property("text").toString(), QStringLiteral("3"));

    focusPageNumberField(fixture.pageNumberField);
    QVERIFY(fixture.pageNumberField->setProperty("text", QStringLiteral("8")));
    QVERIFY(invokeCommitEditing(fixture.pageNavigation, true));
    QCOMPARE(fixture.root->property("openedNumber").toInt(), 8);
    QVERIFY(fixture.root->property("editingReturnedFocus").toBool());
    QTRY_COMPARE(fixture.pageNumberField->property("text").toString(), QStringLiteral("8"));

    focusPageNumberField(fixture.pageNumberField);
    QVERIFY(fixture.pageNumberField->setProperty("text", QStringLiteral("99")));
    QVERIFY(invokeCommitEditing(fixture.pageNavigation, false));
    QCOMPARE(fixture.root->property("openedNumber").toInt(), 9);
    QTRY_COMPARE(fixture.pageNumberField->property("text").toString(), QStringLiteral("9"));
}

void TestMainWindowVideoIntegration::pageNavigationUnavailableStateBlocksEditing()
{
    PageNavigationFixture fixture = createPageNavigationFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));

    QVERIFY(fixture.root->setProperty("activeNavigationKnown", false));
    QCoreApplication::processEvents();
    QTRY_COMPARE(fixture.pageNumberField->property("text").toString(), QStringLiteral("–"));
    QVERIFY(!fixture.pageNumberField->isEnabled());

    clickItem(fixture.view.get(), fixture.leftButton);
    clickItem(fixture.view.get(), fixture.rightButton);
    QCOMPARE(fixture.root->property("openedNumber").toInt(), -1);
    QCOMPARE(fixture.root->property("previousTriggerCount").toInt(), 0);
    QCOMPARE(fixture.root->property("nextTriggerCount").toInt(), 0);
}

void TestMainWindowVideoIntegration::documentSessionFacadeExposesOnlySharedActiveNavigationSurface()
{
    const QMetaObject& metaObject = KiriDocumentSession::staticMetaObject;
    QVERIFY(metaObjectHasProperty(metaObject, "activeNavigationAvailable"));
    QVERIFY(metaObjectHasProperty(metaObject, "activeNavigationKnown"));
    QVERIFY(metaObjectHasProperty(metaObject, "activeNavigationCurrentNumber"));
    QVERIFY(metaObjectHasProperty(metaObject, "activeNavigationThumbnailModel"));
    QVERIFY(metaObjectHasMethod(metaObject, "openPreviousActiveNavigation()"));
    QVERIFY(metaObjectHasMethod(metaObject, "openNextActiveNavigation()"));
    QVERIFY(metaObjectHasMethod(metaObject, "openActiveNavigationAtNumber(int)"));
    QVERIFY(metaObjectHasMethod(metaObject, "requestPreviousActiveNavigationBoundaryText()"));
    QVERIFY(metaObjectHasMethod(metaObject,
        "reportActiveNavigationThumbnailDemand(int,QUrl,int,KiriDocumentSession::"
        "ThumbnailDemandPriority,quint64)"));

    const QList<QByteArray> rawDirectMediaNames {
        QByteArrayLiteral("directMediaNavigationActive"),
        QByteArrayLiteral("directMediaNavigationKnown"),
        QByteArrayLiteral("currentMediaNumber"),
        QByteArrayLiteral("mediaCount"),
        QByteArrayLiteral("canOpenPreviousMedia"),
        QByteArrayLiteral("canOpenNextMedia"),
        QByteArrayLiteral("atKnownFirstMedia"),
        QByteArrayLiteral("atKnownLastMedia"),
        QByteArrayLiteral("openPreviousMedia"),
        QByteArrayLiteral("openNextMedia"),
        QByteArrayLiteral("openMediaAtNumber"),
        QByteArrayLiteral("directMediaNavigationAvailabilityChanged"),
    };
    for (const QByteArray& name : rawDirectMediaNames) {
        QVERIFY2(!metaObjectExposesName(metaObject, name),
            qPrintable(
                QStringLiteral("unexpected QML API surface: %1").arg(QString::fromLatin1(name))));
    }
}

void TestMainWindowVideoIntegration::
    documentSessionRuntimePublicApiExposesOnlySharedActiveNavigationSurface()
{
    using Runtime = kiriview::DocumentSessionRuntime;

    QVERIFY(HasOpenPreviousActiveNavigation<Runtime>::value);
    QVERIFY(HasOpenNextActiveNavigation<Runtime>::value);
    QVERIFY(HasOpenActiveNavigationAtNumber<Runtime>::value);

    QVERIFY(!HasOpenPreviousMedia<Runtime>::value);
    QVERIFY(!HasOpenNextMedia<Runtime>::value);
    QVERIFY(!HasOpenMediaAtNumber<Runtime>::value);
    QVERIFY(!HasCurrentMediaNumber<Runtime>::value);
    QVERIFY(!HasMediaCount<Runtime>::value);
    QVERIFY(!HasDirectMediaNavigationKnown<Runtime>::value);
}

QTEST_MAIN(TestMainWindowVideoIntegration)

#include "test_mainwindowvideointegration.moc"
