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
    void manifestScopesKioFuseRuntimeAccess();
    void developmentRunScopesKioFuseRuntimeAccess();
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
    const QByteArray data = readProjectFile(QStringLiteral("org.hnjae.kiriview.json"));
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

void TestFlatpakPermissions::manifestScopesKioFuseRuntimeAccess()
{
    const QStringList finishArgs = manifestFinishArgs();
    QVERIFY2(!finishArgs.isEmpty(), "Flatpak manifest finish-args should be readable");

    QVERIFY(finishArgs.contains(QStringLiteral("--nofilesystem=/run/user")));
    QVERIFY(finishArgs.contains(QStringLiteral("--filesystem=xdg-run/pipewire-0")));
    QVERIFY(finishArgs.contains(QStringLiteral("--filesystem=xdg-run/gvfs")));
    QVERIFY(finishArgs.contains(QStringLiteral("--talk-name=org.kde.KIOFuse")));
    QVERIFY(!finishArgs.contains(QStringLiteral("--filesystem=/run/user")));
    QVERIFY(!finishArgs.contains(QStringLiteral("--filesystem=/run/user:ro")));
    QVERIFY(!finishArgs.contains(QStringLiteral("--filesystem=xdg-run")));
}

void TestFlatpakPermissions::developmentRunScopesKioFuseRuntimeAccess()
{
    const QString justfile = QString::fromUtf8(readProjectFile(QStringLiteral("justfile")));
    QVERIFY2(!justfile.isEmpty(), "justfile should be readable");

    QVERIFY(justfile.contains(
        QStringLiteral("runtime_dir=\"${XDG_RUNTIME_DIR:-/run/user/$(id -u)}\"")));
    QVERIFY(justfile.contains(QStringLiteral("--nofilesystem=/run/user")));
    QVERIFY(justfile.contains(QStringLiteral("--filesystem=xdg-run/pipewire-0")));
    QVERIFY(justfile.contains(QStringLiteral("--filesystem=xdg-run/gvfs")));
    QVERIFY(justfile.contains(QStringLiteral("--talk-name=org.kde.KIOFuse")));
    QVERIFY(justfile.contains(QStringLiteral("if [ -d \"$runtime_dir/doc\" ]; then")));
    QVERIFY(
        justfile.contains(QStringLiteral("--bind-mount=\"$runtime_dir/doc=$runtime_dir/doc\"")));
    QVERIFY(
        justfile.contains(QStringLiteral("for kio_fuse_path in \"$runtime_dir\"/kio-fuse-*; do")));
    QVERIFY(justfile.contains(
        QStringLiteral("\"--filesystem=xdg-run/$(basename \"$kio_fuse_path\")\"")));
    QVERIFY(!justfile.contains(
        QStringLiteral("--filesystem=\"${XDG_RUNTIME_DIR:-/run/user/$(id -u)}\"")));
    QVERIFY(!justfile.contains(QStringLiteral("--filesystem=/run/user \\")));
    QVERIFY(!justfile.contains(QStringLiteral("--filesystem=/run/user:ro")));
    QVERIFY(!justfile.contains(QStringLiteral("--filesystem=xdg-run \\")));
    QVERIFY(!justfile.contains(QStringLiteral("XDG_RUNTIME_DIR:-/run/user/$(id -u)}:ro")));
}

QTEST_GUILESS_MAIN(TestFlatpakPermissions)

#include "test_flatpakpermissions.moc"
