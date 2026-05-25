// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTest>

class TestFlatpakPermissions : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void manifestKeepsKioFuseRuntimeWritable();
    void developmentRunKeepsKioFuseRuntimeWritable();
};

namespace {
QString projectFile(const QString &relativePath)
{
    return QStringLiteral(KIRIVIEW_TEST_SOURCE_DIR "/../../") + relativePath;
}

QByteArray readProjectFile(const QString &relativePath)
{
    QFile file(projectFile(relativePath));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }

    return file.readAll();
}

QStringList manifestFinishArgs()
{
    const QByteArray data = readProjectFile(QStringLiteral("io.github.hnjae.KiriView.json"));
    if (data.isEmpty()) {
        return {};
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        return {};
    }

    QStringList args;
    const QJsonArray finishArgs = document.object().value(QStringLiteral("finish-args")).toArray();
    for (const QJsonValue &value : finishArgs) {
        args.append(value.toString());
    }
    return args;
}
}

void TestFlatpakPermissions::manifestKeepsKioFuseRuntimeWritable()
{
    const QStringList finishArgs = manifestFinishArgs();
    QVERIFY2(!finishArgs.isEmpty(), "Flatpak manifest finish-args should be readable");

    QVERIFY(finishArgs.contains(QStringLiteral("--filesystem=/run/user")));
    QVERIFY(finishArgs.contains(QStringLiteral("--talk-name=org.kde.KIOFuse")));
    QVERIFY(!finishArgs.contains(QStringLiteral("--filesystem=/run/user:ro")));
    QVERIFY(!finishArgs.contains(QStringLiteral("--filesystem=xdg-run")));
}

void TestFlatpakPermissions::developmentRunKeepsKioFuseRuntimeWritable()
{
    const QString justfile = QString::fromUtf8(readProjectFile(QStringLiteral("justfile")));
    QVERIFY2(!justfile.isEmpty(), "justfile should be readable");

    QVERIFY(justfile.contains(
        QStringLiteral("--filesystem=\"${XDG_RUNTIME_DIR:-/run/user/$(id -u)}\"")));
    QVERIFY(justfile.contains(QStringLiteral("--talk-name=org.kde.KIOFuse")));
    QVERIFY(!justfile.contains(QStringLiteral("--filesystem=/run/user:ro")));
    QVERIFY(!justfile.contains(QStringLiteral("--filesystem=xdg-run \\")));
    QVERIFY(!justfile.contains(QStringLiteral("XDG_RUNTIME_DIR:-/run/user/$(id -u)}:ro")));
}

QTEST_GUILESS_MAIN(TestFlatpakPermissions)

#include "test_flatpakpermissions.moc"
