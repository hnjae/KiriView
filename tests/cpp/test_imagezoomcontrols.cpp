// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "facade/kiriimagedocument.h"
#include "localization/localization.h"

#include <KLocalizedQmlContext>
#include <QDir>
#include <QObject>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickStyle>
#include <QRegularExpression>
#include <QTest>
#include <QUrl>
#include <QtQml/qqml.h>
#include <cmath>
#include <limits>
#include <memory>

class TestImageZoomControls : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void init();
    void formattedZoomText_data();
    void formattedZoomText();
    void displayTextReflectsMissingAndUnknownZoomValues();
    void displayTextIsRightAligned();
    void percentSuffixTrailingSpacingFollowsControlSpacing();
};

namespace {
struct ZoomControlsFixture {
    std::unique_ptr<QQmlEngine> engine;
    std::unique_ptr<KiriImageDocument> imageDocument;
    std::unique_ptr<QObject> root;
    QString errorString;

    bool isValid() const { return engine != nullptr && root != nullptr; }
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

    kiriview::initializeLocalization();
    qmlRegisterType<KiriImageDocument>("org.hnjae.kiriview", 1, 0, "KiriImageDocument");
    registered = true;
}

QUrl imageZoomControlsQmlUrl()
{
    return QUrl::fromLocalFile(QDir(QStringLiteral(KIRIVIEW_TEST_SOURCE_DIR))
            .absoluteFilePath(QStringLiteral("../../src/qml/ImageZoomControls.qml")));
}

double binaryOctaveZoomStepFactor() { return std::pow(2.0, 1.0 / 8.0); }

ZoomControlsFixture createZoomControlsFixture()
{
    ZoomControlsFixture fixture;
    registerKiriViewQmlTypes();
    fixture.engine = std::make_unique<QQmlEngine>();
    addEnvironmentImportPaths(*fixture.engine);
    fixture.engine->addImportPath(QDir(QStringLiteral(KIRIVIEW_TEST_SOURCE_DIR))
            .absoluteFilePath(QStringLiteral("../../src/qml")));
    KLocalization::setupLocalizedContext(fixture.engine.get());

    QQmlComponent component(fixture.engine.get(), imageZoomControlsQmlUrl());
    fixture.imageDocument = std::make_unique<KiriImageDocument>();
    const QVariantMap properties {
        { QStringLiteral("imageDocument"),
            QVariant::fromValue(static_cast<QObject *>(fixture.imageDocument.get())) },
        { QStringLiteral("imageReady"), false },
        { QStringLiteral("minimumManualZoomPercent"), 10 },
        { QStringLiteral("maximumManualZoomPercent"), 1'000'000 },
        { QStringLiteral("zoomStepFactor"), binaryOctaveZoomStepFactor() },
    };
    fixture.root.reset(component.createWithInitialProperties(properties));
    if (fixture.root == nullptr) {
        fixture.errorString = component.errorString();
    }
    return fixture;
}

QObject *findObject(QObject *root, const QString &objectName)
{
    return root->findChild<QObject *>(objectName, Qt::FindChildrenRecursively);
}

void compareZoomValueText(QObject *zoomTextInput, const QString &expectedText)
{
    const QString actualText = zoomTextInput->property("text").toString();
    QCOMPARE(actualText, expectedText);
    QVERIFY2(!actualText.contains(QLatin1Char('%')),
        qPrintable(
            QStringLiteral("zoom value text must not contain percent suffix: %1").arg(actualText)));
}

QString invokeFormattedZoomText(
    QObject *zoomSpinBox, double rawPercent, bool valueAvailable, bool valueKnown)
{
    QVariant result;
    const bool invoked = QMetaObject::invokeMethod(zoomSpinBox, "formattedZoomText",
        Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, rawPercent),
        Q_ARG(QVariant, valueAvailable), Q_ARG(QVariant, valueKnown));
    if (!invoked) {
        return QStringLiteral("<invoke failed>");
    }
    return result.toString();
}
}

void TestImageZoomControls::initTestCase()
{
    if (!qEnvironmentVariableIsSet("QT_QUICK_CONTROLS_STYLE")) {
        QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));
    }
}

void TestImageZoomControls::init()
{
    QTest::failOnWarning(QRegularExpression(
        QStringLiteral(".*Created graphical object was not placed in the graphics scene.*")));
}

void TestImageZoomControls::formattedZoomText_data()
{
    QTest::addColumn<double>("rawPercent");
    QTest::addColumn<bool>("valueAvailable");
    QTest::addColumn<bool>("valueKnown");
    QTest::addColumn<QString>("expectedText");

    QTest::newRow("missing value") << 0.0 << false << false << QStringLiteral("    -");
    QTest::newRow("unknown value") << 0.0 << true << false << QStringLiteral("    ?");
    QTest::newRow("ordinary value") << 100.0 << true << true << QStringLiteral("  100");
    QTest::newRow("rounded below threshold") << 9998.4 << true << true << QStringLiteral(" 9998");
    QTest::newRow("rounded to numeric cap") << 9998.6 << true << true << QStringLiteral(" 9999");
    QTest::newRow("sub-threshold cap") << 9999.9 << true << true << QStringLiteral(" 9999");
    QTest::newRow("exact ten k") << 10000.0 << true << true << QStringLiteral("  10k");
    QTest::newRow("epsilon ten k") << 10000.0005 << true << true << QStringLiteral("  10k");
    QTest::newRow("ten k plus") << 10000.1 << true << true << QStringLiteral(" 10k+");
    QTest::newRow("bucket floor") << 10999.9 << true << true << QStringLiteral(" 10k+");
    QTest::newRow("exact eleven k") << 11000.0 << true << true << QStringLiteral("  11k");
    QTest::newRow("exact upper bucket") << 999000.0 << true << true << QStringLiteral(" 999k");
    QTest::newRow("upper bucket plus") << 999000.1 << true << true << QStringLiteral("999k+");
    QTest::newRow("epsilon million cap") << 999999.9995 << true << true << QStringLiteral("999k+");
    QTest::newRow("million cap") << 1000000.0 << true << true << QStringLiteral("999k+");
    QTest::newRow("non-finite") << std::numeric_limits<double>::quiet_NaN() << true << true
                                << QStringLiteral("    ?");
}

void TestImageZoomControls::formattedZoomText()
{
    ZoomControlsFixture fixture = createZoomControlsFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    QObject *zoomSpinBox = findObject(fixture.root.get(), QStringLiteral("zoomSpinBox"));
    QVERIFY(zoomSpinBox != nullptr);

    QFETCH(double, rawPercent);
    QFETCH(bool, valueAvailable);
    QFETCH(bool, valueKnown);
    QFETCH(QString, expectedText);
    QCOMPARE(
        invokeFormattedZoomText(zoomSpinBox, rawPercent, valueAvailable, valueKnown), expectedText);
}

void TestImageZoomControls::displayTextReflectsMissingAndUnknownZoomValues()
{
    ZoomControlsFixture fixture = createZoomControlsFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    QObject *zoomTextInput = findObject(fixture.root.get(), QStringLiteral("zoomTextInput"));
    QObject *zoomPercentSuffixLabel
        = findObject(fixture.root.get(), QStringLiteral("zoomPercentSuffixLabel"));
    QObject *zoomSpinBox = findObject(fixture.root.get(), QStringLiteral("zoomSpinBox"));
    QVERIFY(zoomTextInput != nullptr);
    QVERIFY(zoomPercentSuffixLabel != nullptr);
    QVERIFY(zoomSpinBox != nullptr);
    QCOMPARE(zoomPercentSuffixLabel->property("text").toString(), QStringLiteral("%"));
    compareZoomValueText(zoomTextInput, QStringLiteral("    -"));
    QCOMPARE(zoomSpinBox->property("value").toInt(), 0);

    QVERIFY(fixture.root->setProperty("zoomPercent", 208.0));
    QVERIFY(fixture.root->setProperty("zoomPercentAvailable", false));
    QVERIFY(fixture.root->setProperty("zoomPercentKnown", false));
    QCoreApplication::processEvents();
    compareZoomValueText(zoomTextInput, QStringLiteral("    -"));
    QCOMPARE(zoomSpinBox->property("value").toInt(), 0);

    QVERIFY(fixture.root->setProperty("readOnlyDisplayMode", true));
    QVERIFY(fixture.root->setProperty("zoomPercentAvailable", true));
    QVERIFY(fixture.root->setProperty("zoomPercentKnown", false));
    QVERIFY(fixture.root->setProperty("readOnlyPercentKnown", false));
    QCoreApplication::processEvents();
    compareZoomValueText(zoomTextInput, QStringLiteral("    ?"));
    QCOMPARE(zoomSpinBox->property("value").toInt(), 0);

    QVERIFY(fixture.root->setProperty("zoomPercentKnown", true));
    QVERIFY(fixture.root->setProperty("zoomPercent", 100.0));
    QVERIFY(fixture.root->setProperty("readOnlyPercentKnown", true));
    QVERIFY(fixture.root->setProperty("readOnlyPercent", 100));
    QCoreApplication::processEvents();
    compareZoomValueText(zoomTextInput, QStringLiteral("  100"));

    QVERIFY(fixture.root->setProperty("zoomPercent", 10000.0));
    QVERIFY(fixture.root->setProperty("readOnlyPercent", 10000));
    QCoreApplication::processEvents();
    compareZoomValueText(zoomTextInput, QStringLiteral("  10k"));
}

void TestImageZoomControls::displayTextIsRightAligned()
{
    ZoomControlsFixture fixture = createZoomControlsFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    QObject *zoomTextInput = findObject(fixture.root.get(), QStringLiteral("zoomTextInput"));
    QVERIFY(zoomTextInput != nullptr);
    QCOMPARE(zoomTextInput->property("horizontalAlignment").toInt(), int(Qt::AlignRight));
}

void TestImageZoomControls::percentSuffixTrailingSpacingFollowsControlSpacing()
{
    ZoomControlsFixture fixture = createZoomControlsFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    QObject *zoomPercentSuffixLabel
        = findObject(fixture.root.get(), QStringLiteral("zoomPercentSuffixLabel"));
    QVERIFY(zoomPercentSuffixLabel != nullptr);

    const int normalSpacing = fixture.root->property("controlSpacing").toInt();
    QCOMPARE(zoomPercentSuffixLabel->property("trailingSpacing").toInt(), normalSpacing);

    QVERIFY(fixture.root->setProperty("compact", true));
    QCoreApplication::processEvents();
    const int compactSpacing = fixture.root->property("controlSpacing").toInt();
    QCOMPARE(zoomPercentSuffixLabel->property("trailingSpacing").toInt(), compactSpacing);
    QVERIFY(compactSpacing <= normalSpacing);
}

QTEST_MAIN(TestImageZoomControls)

#include "test_imagezoomcontrols.moc"
