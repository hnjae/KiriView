// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageviewportgeometry.h"

#include <QCoreApplication>
#include <QDir>
#include <QObject>
#include <QPoint>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickView>
#include <QRectF>
#include <QSize>
#include <QSizeF>
#include <QTest>
#include <QUrl>
#include <QVariant>
#include <QWheelEvent>
#include <QtQml/qqml.h>
#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>

class TestImageViewport : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void ctrlWheelZoomsWhileImageIsPannable();
    void plainWheelStillPansWhileImageIsPannable();
};

namespace {
class FakeKiriImageDocument : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QUrl sourceUrl READ sourceUrl WRITE setSourceUrl NOTIFY sourceUrlChanged)
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(
        QSizeF viewportSize READ viewportSize WRITE setViewportSize NOTIFY viewportSizeChanged)
    Q_PROPERTY(QRectF visibleItemRect READ visibleItemRect WRITE setVisibleItemRect NOTIFY
            visibleItemRectChanged)
    Q_PROPERTY(QSizeF displaySize READ displaySize NOTIFY displaySizeChanged)
    Q_PROPERTY(QSizeF primaryDisplaySize READ primaryDisplaySize NOTIFY displaySizeChanged)
    Q_PROPERTY(QSizeF secondaryDisplaySize READ secondaryDisplaySize CONSTANT)
    Q_PROPERTY(double zoomPercent READ zoomPercent WRITE setZoomPercent NOTIFY zoomPercentChanged)
    Q_PROPERTY(int minimumManualZoomPercent READ minimumManualZoomPercent CONSTANT)
    Q_PROPERTY(int maximumManualZoomPercent READ maximumManualZoomPercent CONSTANT)
    Q_PROPERTY(bool secondaryPageVisible READ secondaryPageVisible CONSTANT)
    Q_PROPERTY(bool rightToLeftReadingEnabled READ rightToLeftReadingEnabled CONSTANT)
    Q_PROPERTY(bool rightToLeftReadingAvailable READ rightToLeftReadingAvailable CONSTANT)

public:
    enum class Status {
        Null,
        Loading,
        Ready,
        Error,
    };
    Q_ENUM(Status)

    explicit FakeKiriImageDocument(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

    QUrl sourceUrl() const { return m_sourceUrl; }
    void setSourceUrl(const QUrl &sourceUrl)
    {
        if (m_sourceUrl == sourceUrl) {
            return;
        }

        m_sourceUrl = sourceUrl;
        Q_EMIT sourceUrlChanged();
    }

    Status status() const { return Status::Ready; }

    QSizeF viewportSize() const { return m_viewportSize; }
    void setViewportSize(const QSizeF &viewportSize)
    {
        if (m_viewportSize == viewportSize) {
            return;
        }

        m_viewportSize = viewportSize;
        Q_EMIT viewportSizeChanged();
    }

    QRectF visibleItemRect() const { return m_visibleItemRect; }
    void setVisibleItemRect(const QRectF &visibleItemRect)
    {
        if (m_visibleItemRect == visibleItemRect) {
            return;
        }

        m_visibleItemRect = visibleItemRect;
        Q_EMIT visibleItemRectChanged();
    }

    QSizeF displaySize() const
    {
        return KiriView::imageViewportDisplaySizeForZoom(imageSize(), m_zoomPercent, 1.0);
    }

    QSizeF primaryDisplaySize() const { return displaySize(); }
    QSizeF secondaryDisplaySize() const { return QSizeF(); }

    double zoomPercent() const { return m_zoomPercent; }
    void setZoomPercent(double zoomPercent)
    {
        if (std::abs(m_zoomPercent - zoomPercent) < 0.001) {
            return;
        }

        m_zoomPercent = zoomPercent;
        Q_EMIT zoomPercentChanged();
        Q_EMIT displaySizeChanged();
    }

    int minimumManualZoomPercent() const { return 10; }
    int maximumManualZoomPercent() const { return 1000; }
    bool secondaryPageVisible() const { return false; }
    bool rightToLeftReadingEnabled() const { return false; }
    bool rightToLeftReadingAvailable() const { return false; }

    Q_INVOKABLE double steppedManualZoomPercent(double stepCount) const
    {
        return std::clamp(m_zoomPercent * std::pow(1.1, stepCount),
            static_cast<double>(minimumManualZoomPercent()),
            static_cast<double>(maximumManualZoomPercent()));
    }

    static QSize imageSize() { return QSize(1000, 1000); }

Q_SIGNALS:
    void sourceUrlChanged();
    void statusChanged();
    void viewportSizeChanged();
    void visibleItemRectChanged();
    void displaySizeChanged();
    void zoomPercentChanged();

private:
    QUrl m_sourceUrl;
    QSizeF m_viewportSize;
    QRectF m_visibleItemRect;
    double m_zoomPercent = 100.0;
};

class FakeKiriImageView : public QQuickItem
{
    Q_OBJECT

    Q_PROPERTY(
        FakeKiriImageDocument *document READ document WRITE setDocument NOTIFY documentChanged)
    Q_PROPERTY(
        bool secondaryPage READ secondaryPage WRITE setSecondaryPage NOTIFY secondaryPageChanged)

public:
    explicit FakeKiriImageView(QQuickItem *parent = nullptr)
        : QQuickItem(parent)
    {
    }

    FakeKiriImageDocument *document() const { return m_document; }
    void setDocument(FakeKiriImageDocument *document)
    {
        if (m_document == document) {
            return;
        }

        m_document = document;
        Q_EMIT documentChanged();
    }

    bool secondaryPage() const { return m_secondaryPage; }
    void setSecondaryPage(bool secondaryPage)
    {
        if (m_secondaryPage == secondaryPage) {
            return;
        }

        m_secondaryPage = secondaryPage;
        Q_EMIT secondaryPageChanged();
    }

    Q_INVOKABLE QPointF panContentPosition(
        const QPointF &contentPosition, const QPointF &delta) const
    {
        return KiriView::imageViewportPanPosition(
            viewportSize(), viewportImageRect(), contentPosition, delta);
    }

    Q_INVOKABLE QPointF nextScanContentPosition(const QPointF &contentPosition) const
    {
        return KiriView::imageViewportNextZScanPosition(
            viewportSize(), viewportImageRect(), contentPosition);
    }

    Q_INVOKABLE QPointF previousScanContentPosition(const QPointF &contentPosition) const
    {
        return KiriView::imageViewportPreviousZScanPosition(
            viewportSize(), viewportImageRect(), contentPosition);
    }

    Q_INVOKABLE QPointF initialScanContentPosition() const
    {
        return KiriView::imageViewportInitialZScanPosition(viewportSize(), viewportImageRect());
    }

    Q_INVOKABLE QPointF finalScanContentPosition() const
    {
        return KiriView::imageViewportFinalZScanPosition(viewportSize(), viewportImageRect());
    }

    Q_INVOKABLE void setNextDisplayedImageStartToFinalScanPosition() { }

    Q_INVOKABLE QPointF displayedImageInitialContentPosition() const
    {
        return initialScanContentPosition();
    }

    Q_INVOKABLE bool viewportPointInsideImage(
        const QPointF &contentPosition, const QPointF &viewportPoint) const
    {
        return KiriView::imageViewportPointInsideImage(
            contentPosition, viewportPoint, viewportImageRect());
    }

    Q_INVOKABLE QPointF zoomContentPosition(const QPointF &contentPosition,
        const QPointF &viewportAnchorPoint, double nextZoomPercent) const
    {
        const QSizeF nextDisplaySize = KiriView::imageViewportDisplaySizeForZoom(
            FakeKiriImageDocument::imageSize(), nextZoomPercent, 1.0);
        return KiriView::imageViewportContentPositionForZoom(
            viewportSize(), displaySize(), nextDisplaySize, contentPosition, viewportAnchorPoint);
    }

Q_SIGNALS:
    void documentChanged();
    void secondaryPageChanged();
    void displayedImageInitialContentPositionRequested();

private:
    QSizeF viewportSize() const
    {
        return m_document == nullptr ? QSizeF() : m_document->viewportSize();
    }

    QSizeF displaySize() const
    {
        return m_document == nullptr ? QSizeF() : m_document->displaySize();
    }

    QRectF viewportImageRect() const
    {
        return KiriView::imageViewportImageRect(viewportSize(), displaySize());
    }

    FakeKiriImageDocument *m_document = nullptr;
    bool m_secondaryPage = false;
};

struct ImageViewportFixture {
    std::unique_ptr<QQuickView> view;
    QObject *root = nullptr;
    QString errorString;

    bool isValid() const { return view != nullptr && root != nullptr; }
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

    qmlRegisterType<FakeKiriImageDocument>("io.github.hnjae.kiriview", 1, 0, "KiriImageDocument");
    qmlRegisterType<FakeKiriImageView>("io.github.hnjae.kiriview", 1, 0, "KiriImageView");
    registered = true;
}

QString qmlSourceImport()
{
    const QString qmlPath = QDir(QStringLiteral(KIRIVIEW_TEST_SOURCE_DIR))
                                .absoluteFilePath(QStringLiteral("../../src/qml"));
    return QUrl::fromLocalFile(qmlPath).toString();
}

QString fixtureQml()
{
    return QStringLiteral(R"(
import QtQuick
import io.github.hnjae.kiriview
import "%1" as KiriViewQml

Item {
    id: root

    width: 200
    height: 160

    function contentY() {
        return imageViewport.flickable.contentY;
    }

    function documentReady() {
        return imageViewport.imageDocument.status === KiriImageDocument.Ready;
    }

    function imagePannable() {
        return imageViewport.imagePannable;
    }

    function setContentY(contentY) {
        imageViewport.flickable.contentY = contentY;
    }

    function setManualZoom(zoomPercent) {
        imageViewport.imageDocument.zoomPercent = zoomPercent;
    }

    function zoomPercent() {
        return imageViewport.imageDocument.zoomPercent;
    }

    KiriViewQml.ImageViewport {
        id: imageViewport

        anchors.fill: parent
    }
}
)")
        .arg(qmlSourceImport());
}

ImageViewportFixture createFixture()
{
    ImageViewportFixture fixture;
    registerKiriViewQmlTypes();
    fixture.view = std::make_unique<QQuickView>();
    fixture.view->resize(200, 160);
    fixture.view->setResizeMode(QQuickView::SizeRootObjectToView);
    addEnvironmentImportPaths(*fixture.view->engine());

    QQmlComponent component(fixture.view->engine());
    component.setData(fixtureQml().toUtf8(), QUrl(QStringLiteral("memory:test_imageviewport.qml")));
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

    fixture.view->setContent(
        QUrl(QStringLiteral("memory:test_imageviewport.qml")), &component, root);
    fixture.view->show();
    if (!QTest::qWaitForWindowExposed(fixture.view.get())) {
        fixture.errorString = QStringLiteral("test window was not exposed");
        return fixture;
    }

    fixture.root = root;
    return fixture;
}

bool invokeBool(QObject *object, const char *method)
{
    QVariant result;
    const bool invoked = QMetaObject::invokeMethod(
        object, method, Qt::DirectConnection, Q_RETURN_ARG(QVariant, result));
    return invoked && result.toBool();
}

qreal invokeReal(QObject *object, const char *method)
{
    QVariant result;
    const bool invoked = QMetaObject::invokeMethod(
        object, method, Qt::DirectConnection, Q_RETURN_ARG(QVariant, result));
    return invoked ? result.toReal() : std::numeric_limits<qreal>::quiet_NaN();
}

void invokeSetReal(QObject *object, const char *method, qreal value)
{
    QVERIFY(QMetaObject::invokeMethod(object, method, Q_ARG(QVariant, QVariant::fromValue(value))));
}

void sendWheel(QQuickView *view, const QPointF &position, int angleDeltaY,
    Qt::KeyboardModifiers modifiers = Qt::NoModifier)
{
    QWheelEvent event(position, view->mapToGlobal(position.toPoint()), QPoint(),
        QPoint(0, angleDeltaY), Qt::NoButton, modifiers, Qt::ScrollUpdate, false);
    QCoreApplication::sendEvent(view, &event);
    QCoreApplication::processEvents();
}

void preparePannableImage(ImageViewportFixture &fixture)
{
    QTRY_VERIFY(invokeBool(fixture.root, "documentReady"));
    invokeSetReal(fixture.root, "setManualZoom", 100.0);
    QTRY_VERIFY(invokeBool(fixture.root, "imagePannable"));
}
}

void TestImageViewport::ctrlWheelZoomsWhileImageIsPannable()
{
    ImageViewportFixture fixture = createFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    preparePannableImage(fixture);

    const qreal initialZoomPercent = invokeReal(fixture.root, "zoomPercent");
    sendWheel(fixture.view.get(), QPointF(100.0, 80.0), 120, Qt::ControlModifier);

    QTRY_VERIFY(invokeReal(fixture.root, "zoomPercent") > initialZoomPercent);
}

void TestImageViewport::plainWheelStillPansWhileImageIsPannable()
{
    ImageViewportFixture fixture = createFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    preparePannableImage(fixture);

    const qreal initialZoomPercent = invokeReal(fixture.root, "zoomPercent");
    invokeSetReal(fixture.root, "setContentY", 400.0);
    QTRY_VERIFY(std::abs(invokeReal(fixture.root, "contentY") - 400.0) < 0.001);

    sendWheel(fixture.view.get(), QPointF(100.0, 80.0), -120);

    QTRY_VERIFY(std::abs(invokeReal(fixture.root, "contentY") - 400.0) > 0.001);
    QCOMPARE(invokeReal(fixture.root, "zoomPercent"), initialZoomPercent);
}

QTEST_MAIN(TestImageViewport)

#include "test_imageviewport.moc"
