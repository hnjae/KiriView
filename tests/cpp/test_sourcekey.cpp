// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "location/sourcekey.h"

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QObject>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>

class TestSourceKey : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void emptyAndInvalidUrlsHaveInvalidKeys();
    void pathSegmentsAreNormalizedButQueryAndFragmentArePreserved();
    void identityUsesFullyEncodedUrlText();
    void relativeLocalFilePathsUseAbsoluteIdentity();
    void localFileKeysCleanTrailingSlashSyntax();
    void pathIdentityIsCaseSensitive();
    void localFileKeysDoNotResolveSymlinks();
    void directMediaKeysUseNavigationSourceIdentity();
    void typedSourceKeyFamiliesKeepIdentityAndFreshnessSeparate();
};

void TestSourceKey::emptyAndInvalidUrlsHaveInvalidKeys()
{
    const KiriView::SourceKey emptyKey = KiriView::sourceKeyForUrl(QUrl());
    QVERIFY(!emptyKey.valid);
    QVERIFY(emptyKey.normalizedUrl.isEmpty());
    QVERIFY(emptyKey.identity.isEmpty());

    const QUrl invalidUrl
        = QUrl::fromEncoded(QByteArrayLiteral("http://example.test/%zz"), QUrl::StrictMode);
    QVERIFY(!invalidUrl.isValid());
    const KiriView::SourceKey invalidKey = KiriView::sourceKeyForUrl(invalidUrl);
    QVERIFY(!invalidKey.valid);
    QVERIFY(invalidKey.identity.isEmpty());

    QVERIFY(!KiriView::sameSourceUrlKey(QUrl(), QUrl()));
    QVERIFY(KiriView::sameSourceUrlKeyOrEmpty(QUrl(), QUrl()));
    QVERIFY(
        !KiriView::sameSourceUrlKeyOrEmpty(QUrl(), QUrl(QStringLiteral("file:///media/01.png"))));
}

void TestSourceKey::pathSegmentsAreNormalizedButQueryAndFragmentArePreserved()
{
    const QUrl requested(QStringLiteral(
        "https://example.test/library/../image%20folder/./cover.png?name=A%20B#page%201"));
    const QUrl normalized(
        QStringLiteral("https://example.test/image%20folder/cover.png?name=A%20B#page%201"));

    const KiriView::SourceKey key = KiriView::sourceKeyForUrl(requested);
    QVERIFY(key.valid);
    QCOMPARE(key.normalizedUrl, normalized);
    QCOMPARE(key.normalizedUrl.query(QUrl::FullyEncoded), QStringLiteral("name=A%20B"));
    QCOMPARE(key.normalizedUrl.fragment(QUrl::FullyEncoded), QStringLiteral("page%201"));
    QVERIFY(KiriView::sameSourceUrlKey(requested, normalized));
}

void TestSourceKey::identityUsesFullyEncodedUrlText()
{
    QUrl url;
    url.setScheme(QStringLiteral("https"));
    url.setHost(QStringLiteral("example.test"));
    url.setPath(QStringLiteral("/media/image 01.png"));
    url.setQuery(QStringLiteral("caption=one two"));
    url.setFragment(QStringLiteral("section one"));

    const KiriView::SourceKey key = KiriView::sourceKeyForUrl(url);
    QVERIFY(key.valid);
    QCOMPARE(key.identity, key.normalizedUrl.toString(QUrl::FullyEncoded));
    QVERIFY(key.identity.contains(QStringLiteral("%20")));
    QVERIFY(!key.identity.contains(QLatin1Char(' ')));
}

void TestSourceKey::relativeLocalFilePathsUseAbsoluteIdentity()
{
    const QUrl requested = QUrl::fromLocalFile(QStringLiteral("chapter/../cover.png"));
    const QString expectedPath
        = QDir::cleanPath(QDir::current().absoluteFilePath(QStringLiteral("cover.png")));
    const QUrl expected = QUrl::fromLocalFile(expectedPath);

    const KiriView::SourceKey key = KiriView::sourceKeyForUrl(requested);
    QVERIFY(key.valid);
    QCOMPARE(key.normalizedUrl, expected);
    QCOMPARE(key.identity, expected.toString(QUrl::FullyEncoded));
}

void TestSourceKey::localFileKeysCleanTrailingSlashSyntax()
{
    const QUrl trailingSlash = QUrl::fromLocalFile(QStringLiteral("/media/folder/../image.png/"));
    const QUrl clean = QUrl::fromLocalFile(QStringLiteral("/media/image.png"));

    const KiriView::SourceKey key = KiriView::sourceKeyForUrl(trailingSlash);
    QVERIFY(key.valid);
    QCOMPARE(key.normalizedUrl, clean);
    QVERIFY(KiriView::sameSourceUrlKey(trailingSlash, clean));
}

void TestSourceKey::pathIdentityIsCaseSensitive()
{
    QVERIFY(!KiriView::sameSourceUrlKey(QUrl(QStringLiteral("file:///media/Image.png")),
        QUrl(QStringLiteral("file:///media/image.png"))));
}

void TestSourceKey::localFileKeysDoNotResolveSymlinks()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    QFile target(directory.filePath(QStringLiteral("target.png")));
    QVERIFY(target.open(QIODevice::WriteOnly));
    target.write("image");
    target.close();

    const QString linkPath = directory.filePath(QStringLiteral("link.png"));
    if (!QFile::link(target.fileName(), linkPath)) {
        QSKIP("Symlink creation is unavailable in this test environment");
    }

    QVERIFY(!KiriView::sameSourceUrlKey(
        QUrl::fromLocalFile(target.fileName()), QUrl::fromLocalFile(linkPath)));
}

void TestSourceKey::directMediaKeysUseNavigationSourceIdentity()
{
    const QUrl current(QStringLiteral("file:///media/chapter/../01.png"));
    const QUrl parent(QStringLiteral("file:///media/chapter/.."));
    const KiriView::SourceKey currentKey = KiriView::sourceKeyForDirectMediaCurrentUrl(current);
    const KiriView::SourceKey parentKey = KiriView::sourceKeyForDirectMediaParentUrl(parent);

    QVERIFY(KiriView::sameSourceKey(
        currentKey, KiriView::sourceKeyForUrl(QUrl(QStringLiteral("file:///media/01.png")))));
    QVERIFY(KiriView::sameSourceKey(
        parentKey, KiriView::sourceKeyForUrl(QUrl(QStringLiteral("file:///media")))));
}

void TestSourceKey::typedSourceKeyFamiliesKeepIdentityAndFreshnessSeparate()
{
    const QUrl current(QStringLiteral("file:///media/chapter/../01.png"));
    const QUrl parent(QStringLiteral("file:///media/chapter/.."));

    const KiriView::OrdinaryFileSourceKey ordinary = KiriView::ordinaryFileSourceKeyForUrl(current);
    const KiriView::DirectMediaSourceKey direct = KiriView::directMediaSourceKeyForUrl(current);
    const KiriView::DirectMediaScopeKey firstScope
        = KiriView::directMediaScopeKeyForUrls(current, parent, 1);
    const KiriView::DirectMediaScopeKey refreshedScope
        = KiriView::directMediaScopeKeyForUrls(current, parent, 2);
    const KiriView::RenderSurfaceKey firstRenderSurface = KiriView::renderSurfaceKey(
        QStringLiteral("surface-1"), 1, 10, QStringLiteral("primary"), QStringLiteral("raster"));
    const KiriView::RenderSurfaceKey repeatedRenderSurface = KiriView::renderSurfaceKey(
        QStringLiteral("surface-1"), 1, 10, QStringLiteral("primary"), QStringLiteral("raster"));
    const KiriView::RenderSurfaceKey refreshedRenderSurface = KiriView::renderSurfaceKey(
        QStringLiteral("surface-1"), 2, 10, QStringLiteral("primary"), QStringLiteral("raster"));

    QVERIFY(KiriView::sameOrdinaryFileSourceKey(ordinary,
        KiriView::ordinaryFileSourceKeyForUrl(QUrl(QStringLiteral("file:///media/01.png")))));
    QVERIFY(KiriView::sameDirectMediaSourceKey(direct,
        KiriView::directMediaSourceKeyForUrl(QUrl(QStringLiteral("file:///media/01.png")))));
    QVERIFY(KiriView::sameDirectMediaScopeKey(firstScope, refreshedScope));
    QCOMPARE(firstScope.generation, quint64(1));
    QCOMPARE(refreshedScope.generation, quint64(2));
    QVERIFY(!KiriView::sameDirectMediaScopeKey(firstScope,
        KiriView::directMediaScopeKeyForUrls(current, QUrl(QStringLiteral("file:///other")), 1)));
    QVERIFY(KiriView::sameRenderSurfaceKey(firstRenderSurface, repeatedRenderSurface));
    QVERIFY(!KiriView::sameRenderSurfaceKey(firstRenderSurface, refreshedRenderSurface));
    QCOMPARE(firstRenderSurface.surfaceGeneration, quint64(1));
    QCOMPARE(refreshedRenderSurface.surfaceGeneration, quint64(2));
}

QTEST_GUILESS_MAIN(TestSourceKey)

#include "test_sourcekey.moc"
