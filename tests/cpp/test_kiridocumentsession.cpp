// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "facade/kiridocumentsession.h"

#include "candidate_test_support.h"
#include "facade/kiriimagedocument.h"
#include "facade/kirimediainformation.h"
#include "facade/kirivideodocument.h"
#include "image_async_test_support.h"
#include "image_test_support.h"
#include "location/imagedocumentlocation.h"
#include "metadata/embeddedmetadata.h"
#include "navigation/directmedianavigationmodel.h"
#include "navigation/imagedocumentpagecandidateprovider.h"
#include "session/activenavigationthumbnailmodel.h"
#include "session/thumbnailimagestore.h"

#include <QAbstractItemModel>
#include <QFile>
#include <QImage>
#include <QObject>
#include <QSignalSpy>
#include <QSizeF>
#include <QTemporaryDir>
#include <QTest>
#include <cstddef>
#include <map>
#include <memory>
#include <utility>

namespace {
QString keyForUrl(const QUrl &url) { return url.adjusted(QUrl::NormalizePathSegments).toString(); }

QUrl localUrl(const QString &path) { return QUrl::fromLocalFile(path); }

KiriView::DirectMediaNavigationCandidate directMediaNavigationCandidate(const QUrl &url)
{
    return KiriView::DirectMediaNavigationCandidate { url, url.fileName(QUrl::PrettyDecoded) };
}

QVariant thumbnailData(const KiriDocumentSession &session, int row, int role)
{
    QAbstractItemModel *model = session.activeNavigationThumbnailModel();
    return model->data(model->index(row, 0), role);
}

int roleForName(const QAbstractItemModel &model, const QByteArray &name)
{
    const QHash<int, QByteArray> roles = model.roleNames();
    for (auto iterator = roles.cbegin(); iterator != roles.cend(); ++iterator) {
        if (iterator.value() == name) {
            return iterator.key();
        }
    }

    return -1;
}

QVariant mediaInformationRowData(
    const QAbstractItemModel &model, int row, const QByteArray &roleName)
{
    const int role = roleForName(model, roleName);
    if (role < 0) {
        return {};
    }

    return model.data(model.index(row, 0), role);
}

QString mediaInformationValueForLabel(const QAbstractItemModel &model, const QString &label)
{
    for (int row = 0; row < model.rowCount(); ++row) {
        if (mediaInformationRowData(model, row, QByteArrayLiteral("label")).toString() == label) {
            return mediaInformationRowData(model, row, QByteArrayLiteral("value")).toString();
        }
    }

    return {};
}

void compareThumbnailRow(const KiriDocumentSession &session, int row, int number, const QUrl &url,
    const QString &label, const QString &iconName, bool current)
{
    QCOMPARE(
        thumbnailData(session, row, KiriView::ActiveNavigationThumbnailModel::NumberRole).toInt(),
        number);
    QCOMPARE(thumbnailData(session, row, KiriView::ActiveNavigationThumbnailModel::UrlRole).toUrl(),
        url);
    QCOMPARE(
        thumbnailData(session, row, KiriView::ActiveNavigationThumbnailModel::LabelRole).toString(),
        label);
    QCOMPARE(thumbnailData(session, row, KiriView::ActiveNavigationThumbnailModel::IconNameRole)
                 .toString(),
        iconName);
    QCOMPARE(
        thumbnailData(session, row, KiriView::ActiveNavigationThumbnailModel::CurrentRole).toBool(),
        current);
}

QVariant thumbnailDataForRoleName(
    const KiriDocumentSession &session, int row, const QByteArray &roleName)
{
    QAbstractItemModel *model = session.activeNavigationThumbnailModel();
    const int role = roleForName(*model, roleName);
    if (role < 0) {
        return {};
    }

    return model->data(model->index(row, 0), role);
}

class FakeDirectMediaNavigationCandidateProvider
{
public:
    void setMedia(
        const QUrl &parentUrl, std::vector<KiriView::DirectMediaNavigationCandidate> candidates)
    {
        m_candidates[keyForUrl(parentUrl)] = std::move(candidates);
    }

    KiriView::DirectMediaNavigationCandidateProvider provider()
    {
        return KiriView::DirectMediaNavigationCandidateProvider {
            [this](QObject *, QUrl parentUrl,
                KiriView::DirectMediaNavigationCandidatesCallback callback,
                KiriView::ErrorCallback errorCallback) {
                const auto candidates = m_candidates.find(keyForUrl(parentUrl));
                if (candidates == m_candidates.cend()) {
                    if (errorCallback) {
                        errorCallback(QStringLiteral("missing direct media candidates"));
                    }
                    return KiriView::ImageIoJob();
                }

                if (callback) {
                    callback(candidates->second);
                }
                return KiriView::ImageIoJob();
            },
        };
    }

private:
    std::map<QString, std::vector<KiriView::DirectMediaNavigationCandidate>> m_candidates;
};

struct ManualDirectMediaNavigationCandidateLoad {
    QObject *object = nullptr;
    QUrl parentUrl;
    KiriView::DirectMediaNavigationCandidatesCallback callback;
    KiriView::ErrorCallback errorCallback;
    KiriView::ImageIoJobCompletion completion;
    bool canceled = false;
};

class ManualDirectMediaNavigationCandidateProvider
{
public:
    KiriView::DirectMediaNavigationCandidateProvider provider()
    {
        return KiriView::DirectMediaNavigationCandidateProvider {
            [this](QObject *receiver, QUrl parentUrl,
                KiriView::DirectMediaNavigationCandidatesCallback callback,
                KiriView::ErrorCallback errorCallback) {
                auto load = std::make_shared<ManualDirectMediaNavigationCandidateLoad>();
                load->parentUrl = std::move(parentUrl);
                load->callback = std::move(callback);
                load->errorCallback = std::move(errorCallback);

                KiriView::ImageIoJob job
                    = KiriView::TestSupport::Detail::startManualIoJob(receiver, load);
                m_loads.push_back(load);
                return job;
            },
        };
    }

    std::size_t loadCount() const { return m_loads.size(); }

    ManualDirectMediaNavigationCandidateLoad &loadAt(std::size_t index)
    {
        return *m_loads.at(index);
    }

    void finishLoad(
        std::size_t index, std::vector<KiriView::DirectMediaNavigationCandidate> candidates)
    {
        KiriView::TestSupport::Detail::finishManualIoJob(m_loads.at(index),
            [candidates = std::move(candidates)](
                ManualDirectMediaNavigationCandidateLoad &load) mutable {
                if (load.callback) {
                    load.callback(std::move(candidates));
                }
            });
    }

    void deliverIgnoringCancellation(
        std::size_t index, std::vector<KiriView::DirectMediaNavigationCandidate> candidates)
    {
        ManualDirectMediaNavigationCandidateLoad &load = loadAt(index);
        if (load.callback) {
            load.callback(std::move(candidates));
        }
    }

private:
    std::vector<std::shared_ptr<ManualDirectMediaNavigationCandidateLoad>> m_loads;
};

class FakeMediaOpenWithProvider
{
public:
    KiriView::MediaOpenWithProvider provider()
    {
        return [this](QObject *, KiriView::MediaOpenWithRequest request,
                   KiriView::MediaOpenWithCallback callback) {
            requests.push_back(std::move(request));
            if (callback) {
                callback(result, errorString);
            }
            return KiriView::ImageIoJob();
        };
    }

    std::vector<KiriView::MediaOpenWithRequest> requests;
    KiriView::MediaOpenWithResult result = KiriView::MediaOpenWithResult::Succeeded;
    QString errorString;
};

class FakeThumbnailLookupProvider
{
public:
    KiriView::ThumbnailCacheLookupProvider provider()
    {
        return [this](QObject *, KiriView::ThumbnailCacheLookupRequest request,
                   KiriView::ThumbnailCacheLookupCallback callback) {
            requests.push_back(std::move(request));
            if (callback) {
                callback(result);
            }
            return KiriView::ImageIoJob();
        };
    }

    std::vector<KiriView::ThumbnailCacheLookupRequest> requests;
    KiriView::ThumbnailCacheLookupResult result;
};

class FakeThumbnailGenerationProvider
{
public:
    KiriView::ThumbnailGenerationProvider provider()
    {
        return [this](QObject *, KiriView::ThumbnailGenerationRequest request,
                   KiriView::ThumbnailGenerationCallback callback) {
            requests.push_back(std::move(request));
            if (callback) {
                callback(result);
            }
            return KiriView::ImageIoJob();
        };
    }

    std::vector<KiriView::ThumbnailGenerationRequest> requests;
    KiriView::ThumbnailGenerationResult result;
};

std::unique_ptr<KiriDocumentSession> createSessionWithProvider(
    KiriView::DirectMediaNavigationCandidateProvider directMediaNavigationCandidateProvider,
    KiriView::TestSupport::ManualFileDeletionProvider *fileDeletion = nullptr,
    KiriView::TestSupport::ManualImageDataLoader *imageDataLoader = nullptr,
    KiriView::ImageDocumentPageCandidateProvider imageDocumentPageCandidateProvider = {},
    KiriView::ImageDataDecoder imageDataDecoder = KiriView::TestSupport::staticImageDataDecoder(),
    KiriView::MediaOpenWithProvider mediaOpenWithProvider = {},
    KiriView::ThumbnailCacheLookupProvider thumbnailLookupProvider = {},
    KiriView::ThumbnailGenerationProvider thumbnailGenerationProvider = {},
    std::shared_ptr<KiriView::ThumbnailImageStore> thumbnailImageStore = {})
{
    KiriView::KiriDocumentSessionDependencies dependencies;
    dependencies.sessionRuntime.directMediaNavigationCandidateProvider
        = std::move(directMediaNavigationCandidateProvider);
    dependencies.sessionRuntime.mediaOpenWithProvider = std::move(mediaOpenWithProvider);
    dependencies.sessionRuntime.activeNavigationThumbnailLookupProvider
        = std::move(thumbnailLookupProvider);
    dependencies.sessionRuntime.activeNavigationThumbnailGenerationProvider
        = std::move(thumbnailGenerationProvider);
    dependencies.sessionRuntime.activeNavigationThumbnailImageStore
        = std::move(thumbnailImageStore);
    dependencies.imageDocument.candidateProvider = std::move(imageDocumentPageCandidateProvider);
    if (fileDeletion != nullptr) {
        dependencies.sessionRuntime.fileDeletionProvider
            = KiriView::TestSupport::fileDeletionProviderFor(*fileDeletion);
        dependencies.imageDocument.fileDeletionProvider
            = KiriView::TestSupport::fileDeletionProviderFor(*fileDeletion);
    }
    if (imageDataLoader != nullptr) {
        dependencies.imageDocument.imageDecode = KiriView::TestSupport::imageDecodeDependenciesFor(
            *imageDataLoader, std::move(imageDataDecoder));
    }
    return std::make_unique<KiriDocumentSession>(std::move(dependencies));
}

std::unique_ptr<KiriDocumentSession> createSession(
    FakeDirectMediaNavigationCandidateProvider &directMediaNavigationProvider,
    KiriView::TestSupport::ManualFileDeletionProvider *fileDeletion = nullptr,
    KiriView::TestSupport::ManualImageDataLoader *imageDataLoader = nullptr)
{
    return createSessionWithProvider(
        directMediaNavigationProvider.provider(), fileDeletion, imageDataLoader);
}

void compareUnavailableActiveNavigation(const KiriDocumentSession &session)
{
    QVERIFY(!session.activeNavigationAvailable());
    QVERIFY(!session.activeNavigationKnown());
    QVERIFY(!session.activeNavigationEditable());
    QCOMPARE(session.activeNavigationCurrentNumber(), 0);
    QCOMPARE(session.activeNavigationCount(), 0);
    QVERIFY(!session.canOpenPreviousActiveNavigation());
    QVERIFY(!session.canOpenNextActiveNavigation());
    QVERIFY(!session.atKnownFirstActiveNavigation());
    QVERIFY(!session.atKnownLastActiveNavigation());
}

bool writeTestImage(const QString &path)
{
    QImage image(QSize(2, 2), QImage::Format_RGBA8888);
    image.fill(Qt::red);
    return image.save(path, "PNG");
}

bool writeEmptyFile(const QString &path)
{
    QFile file(path);
    return file.open(QIODevice::WriteOnly);
}

KiriView::EmbeddedMetadata testCameraMetadata()
{
    KiriView::EmbeddedMetadata metadata;
    metadata.cameraMake = QStringLiteral("Kiri Camera Co.");
    metadata.cameraModel = QStringLiteral("KiriCam 1");
    metadata.taken = QStringLiteral("2026-05-31 12:34:56");
    metadata.location = QStringLiteral("37.7749, -122.4194");
    metadata.lens = QStringLiteral("Kiri Prime 35mm");
    metadata.exposure = QStringLiteral("1/125 s at f/5.6");
    metadata.iso = QStringLiteral("400");
    metadata.focalLength = QStringLiteral("35 mm");
    metadata.software = QStringLiteral("KiriOS Camera");
    metadata.advancedRows = {
        KiriView::EmbeddedMetadataRow {
            QStringLiteral("Artist"),
            QStringLiteral("Kiri Tester"),
        },
    };
    return metadata;
}

KiriView::ImageDataDecoder staticImageDataDecoderWithMetadata(
    KiriView::EmbeddedMetadata metadata, QImage image = KiriView::TestSupport::testImage(2, 2))
{
    return [metadata = std::move(metadata), image = std::move(image)](
               const QByteArray &, const KiriView::ImageDecodeRequest &) mutable {
        KiriView::StaticDecodedImage decoded = KiriView::TestSupport::staticDecodedTestImage(image);
        decoded.embeddedMetadata = metadata;
        return KiriView::successfulDecodedImageResult(std::move(decoded));
    };
}

bool modelValuesContainCaseInsensitive(const QAbstractItemModel &model, const QString &needle)
{
    for (int row = 0; row < model.rowCount(); ++row) {
        const QString value
            = mediaInformationRowData(model, row, QByteArrayLiteral("value")).toString();
        if (value.contains(needle, Qt::CaseInsensitive)) {
            return true;
        }
    }

    return false;
}
}

class TestKiriDocumentSession : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void emptySessionProjectsUnavailableActiveNavigation();
    void emptySessionProjectsUnavailableMediaInformation();
    void imageMediaInformationUsesEmbeddedMetadataAndKnownDimensions();
    void imageMediaInformationClearsStaleEmbeddedMetadataOnReplacement();
    void videoMediaInformationUsesVideoSectionAndNoCameraRows();
    void mediaInformationDerivesFilenameAndPathFromTargetUrl();
    void mediaInformationRowModelsExposeLabelAndValueRoles();
    void directVideoRoutesToVideoDocumentWithOriginalSource();
    void activeZoomReadoutFollowsSessionDocumentKind();
    void archiveAndDirectoryInputsRouteToImageDocument();
    void directImageAfterVideoRestoresImageDocument();
    void kioArchiveImageAfterKioArchiveVideoUsesOriginalImageUrl();
    void directImageDirectMediaNavigationIncludesSiblingVideos();
    void directImageActiveNavigationIgnoresImageDocumentDirectoryPageCandidates();
    void directArchiveEntryImageUsesDirectMediaNavigationWithoutImageDocumentPages();
    void directMediaThumbnailModelTracksSiblingCandidates();
    void directMediaThumbnailModelStaysEmptyUntilCandidatesAreKnown();
    void activeNavigationThumbnailModelExposesSourceNeutralResultRoles();
    void activeNavigationThumbnailDemandSurfaceValidatesIdentityAndGeneration();
    void activeNavigationThumbnailDemandProjectsPendingAndUnsupportedResults();
    void directImageThumbnailDemandProjectsReadyCacheHitSource();
    void directImageThumbnailDemandProjectsReadyGeneratedSource();
    void directImageThumbnailDemandKeepsFallbackForFailedLookup();
    void directImageThumbnailDemandKeepsFallbackForFailedGeneration();
    void defaultMediaProviderListsLocalDirectImageSiblings();
    void defaultMediaProviderListsLocalDirectVideoSiblings();
    void freshDirectImageReadoutUsesRequestedCursorBeforeDisplayedUrl();
    void directImageDocumentPageCandidateCompletionSurvivesCursorConfirmation();
    void directImageReplacementFailureRestoresPreviousMediaCursor();
    void stalePendingDirectImageDocumentPageCandidateCompletionCannotPublishForNewCursor();
    void freshDirectImageFailureLeavesNavigationUnknown();
    void archiveImageDocumentProjectsActiveNavigationFromPages();
    void imageDocumentPageNavigationChangesEmitActiveNavigationWhenRelevant();
    void activeNavigationNumberDispatchRoutesDirectMedia();
    void activeNavigationNumberDispatchRoutesImageDocumentPages();
    void archiveCollectionThumbnailModelUsesPageCandidateNames();
    void activeNavigationRequestReportsDispatchAndBoundaryResults();
    void activeNavigationAdjacentRequestsSetRevealIntent();
    void activeNavigationLargeJumpRequestsSetRevealIntent();
    void activeNavigationThumbnailDispatchSetsRevealIntent();
    void sourceOpenReplacesStaleRevealIntent();
    void unavailableActiveNavigationRequestsClearRevealDirection();
    void activeNavigationBoundaryTextFollowsSessionSource();
    void activeNavigationNumberDispatchIgnoresUnknownNavigation();
    void activeNavigationClearsWhenSwitchingFromKnownDirectMedia();
    void activeNavigationAvailabilityUsesSameSnapshotAsCurrentAndCount();
    void activeNavigationBoundaryScopeFollowsSessionSource();
    void openWithIsUnavailableInEmptySession();
    void openWithUsesCurrentDirectImageUrl();
    void openWithFailureEmitsToastSignal();
    void twoPageSpreadLastBoundaryProjectsThroughActiveNavigation();
    void videoNavigationKeepsStillImagePredecodeCache();
    void videoActiveNavigationExposesCurrentNumberAndCount();
    void initialDirectImagePredecodeUsesRequestedMediaCursor();
    void directImagePredecodeUsesSessionDependencyOverrides();
    void directImagePredecodeDoesNotUseImageDocumentPageCandidates();
    void staleDirectMediaNavigationCandidateCompletionCannotPublishForNewSource();
    void nextMediaFromVideoCanRouteToImageWithoutUsingImageDocumentPageNavigation();
    void nonMediaImageDeletionProgressIsMirroredThroughSessionState();
    void directMediaDeletionInProgressDisablesActiveNavigationDispatch();
    void directImageDeletionCanOpenVideoFallback();
    void pendingDirectImageReplacementDoesNotDeletePreviousDisplayedFile();
    void pendingDirectMediaDeletionCandidateLoadIsCanceledBySourceChange();
    void videoDeletionUsesOriginalUrlAndOpensMediaFallback();
    void canceledVideoDeletionKeepsCurrentVideo();
    void staleVideoDeletionCompletionAfterSourceChangeIsIgnored();
};

void TestKiriDocumentSession::emptySessionProjectsUnavailableActiveNavigation()
{
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    std::unique_ptr<KiriDocumentSession> session = createSession(directMediaNavigationProvider);

    compareUnavailableActiveNavigation(*session);
}

void TestKiriDocumentSession::emptySessionProjectsUnavailableMediaInformation()
{
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    std::unique_ptr<KiriDocumentSession> session = createSession(directMediaNavigationProvider);

    KiriMediaInformation *mediaInformation = session->mediaInformation();
    QVERIFY(mediaInformation != nullptr);
    QVERIFY(!mediaInformation->available());
    QVERIFY(!mediaInformation->canCopyFilePath());
    QVERIFY(!mediaInformation->canOpenContainingFolder());
    QCOMPARE(mediaInformation->title(), QString());
    QCOMPARE(mediaInformation->summary(), QStringLiteral("No media selected"));
    QCOMPARE(mediaInformation->mediaSectionTitle(), QStringLiteral("Media"));
    QVERIFY(!mediaInformation->hasCameraSection());
    QCOMPARE(mediaInformation->generalRows()->rowCount(), 0);
    QCOMPARE(mediaInformation->mediaRows()->rowCount(), 0);
    QCOMPARE(mediaInformation->cameraRows()->rowCount(), 0);
}

void TestKiriDocumentSession::imageMediaInformationUsesEmbeddedMetadataAndKnownDimensions()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString imagePath = directory.filePath(QStringLiteral("photo.png"));
    QVERIFY(writeTestImage(imagePath));

    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    const QUrl imageUrl = localUrl(imagePath);
    directMediaNavigationProvider.setMedia(localUrl(directory.path() + QStringLiteral("/")),
        { directMediaNavigationCandidate(imageUrl) });
    KiriView::TestSupport::ManualImageDataLoader dataLoader;
    std::unique_ptr<KiriDocumentSession> session
        = createSessionWithProvider(directMediaNavigationProvider.provider(), nullptr, &dataLoader,
            {}, staticImageDataDecoderWithMetadata(testCameraMetadata()));

    session->setSourceUrl(imageUrl);
    QVERIFY(dataLoader.finishOldestActiveLoadForUrl(imageUrl, QByteArrayLiteral("image bytes")));

    QTRY_COMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Image);
    QTRY_COMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Ready);
    KiriMediaInformation *mediaInformation = session->mediaInformation();
    QVERIFY(mediaInformation->available());
    QCOMPARE(mediaInformation->title(), QStringLiteral("photo.png"));
    QCOMPARE(mediaInformation->summary(), QStringLiteral("Image, 2×2 px"));
    QCOMPARE(mediaInformation->mediaSectionTitle(), QStringLiteral("Image"));
    QVERIFY(mediaInformation->hasCameraSection());
    QVERIFY(mediaInformation->hasAdvancedSection());
    QVERIFY(mediaInformation->canCopyFilePath());
    QVERIFY(mediaInformation->canOpenContainingFolder());
    QCOMPARE(mediaInformation->generalRows()->rowCount(), 2);
    QCOMPARE(mediaInformation->mediaRows()->rowCount(), 1);
    QCOMPARE(mediaInformation->cameraRows()->rowCount(), 8);
    QCOMPARE(mediaInformation->advancedRows()->rowCount(), 1);
    QCOMPARE(
        mediaInformationValueForLabel(*mediaInformation->generalRows(), QStringLiteral("Type")),
        QStringLiteral("Image"));
    QCOMPARE(
        mediaInformationValueForLabel(*mediaInformation->generalRows(), QStringLiteral("Path")),
        imagePath);
    QCOMPARE(
        mediaInformationValueForLabel(*mediaInformation->mediaRows(), QStringLiteral("Dimensions")),
        QStringLiteral("2×2 px"));
    QCOMPARE(
        mediaInformationValueForLabel(*mediaInformation->cameraRows(), QStringLiteral("Camera")),
        QStringLiteral("Kiri Camera Co. KiriCam 1"));
    QCOMPARE(
        mediaInformationValueForLabel(*mediaInformation->cameraRows(), QStringLiteral("Taken")),
        QStringLiteral("2026-05-31 12:34:56"));
    QCOMPARE(
        mediaInformationValueForLabel(*mediaInformation->cameraRows(), QStringLiteral("Location")),
        QStringLiteral("37.7749, -122.4194"));
    QCOMPARE(
        mediaInformationValueForLabel(*mediaInformation->advancedRows(), QStringLiteral("Artist")),
        QStringLiteral("Kiri Tester"));
    QVERIFY(!modelValuesContainCaseInsensitive(
        *mediaInformation->generalRows(), QStringLiteral("placeholder")));
    QVERIFY(!modelValuesContainCaseInsensitive(
        *mediaInformation->mediaRows(), QStringLiteral("placeholder")));
    QVERIFY(!modelValuesContainCaseInsensitive(
        *mediaInformation->cameraRows(), QStringLiteral("placeholder")));
}

void TestKiriDocumentSession::imageMediaInformationClearsStaleEmbeddedMetadataOnReplacement()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString firstPath = directory.filePath(QStringLiteral("first.png"));
    const QString secondPath = directory.filePath(QStringLiteral("second.png"));
    QVERIFY(writeTestImage(firstPath));
    QVERIFY(writeTestImage(secondPath));

    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    const QUrl firstUrl = localUrl(firstPath);
    const QUrl secondUrl = localUrl(secondPath);
    directMediaNavigationProvider.setMedia(localUrl(directory.path() + QStringLiteral("/")),
        { directMediaNavigationCandidate(firstUrl), directMediaNavigationCandidate(secondUrl) });
    KiriView::EmbeddedMetadata metadata = testCameraMetadata();
    KiriView::ImageDataDecoder decoder = [metadata = std::move(metadata)](const QByteArray &data,
                                             const KiriView::ImageDecodeRequest &request) {
        KiriView::StaticDecodedImage decoded
            = KiriView::TestSupport::staticDecodedTestImage(KiriView::TestSupport::testImage(
                request.imageUrl().fileName() == QStringLiteral("first.png") ? 2 : 3, 2));
        if (data == QByteArrayLiteral("first")) {
            decoded.embeddedMetadata = metadata;
        }
        return KiriView::successfulDecodedImageResult(std::move(decoded));
    };
    KiriView::TestSupport::ManualImageDataLoader dataLoader;
    std::unique_ptr<KiriDocumentSession> session = createSessionWithProvider(
        directMediaNavigationProvider.provider(), nullptr, &dataLoader, {}, std::move(decoder));

    session->setSourceUrl(firstUrl);
    QVERIFY(dataLoader.finishOldestActiveLoadForUrl(firstUrl, QByteArrayLiteral("first")));
    QTRY_COMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Ready);
    QVERIFY(session->mediaInformation()->hasCameraSection());

    session->setSourceUrl(secondUrl);
    QVERIFY(dataLoader.finishOldestActiveLoadForUrl(secondUrl, QByteArrayLiteral("second")));
    QTRY_COMPARE(session->imageDocument()->primaryImageSize(), QSize(3, 2));
    QVERIFY(!session->mediaInformation()->hasCameraSection());
    QVERIFY(!session->mediaInformation()->hasAdvancedSection());
}

void TestKiriDocumentSession::videoMediaInformationUsesVideoSectionAndNoCameraRows()
{
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    const QUrl clip = localUrl(QStringLiteral("/media/clip.mp4"));
    directMediaNavigationProvider.setMedia(
        localUrl(QStringLiteral("/media/")), { directMediaNavigationCandidate(clip) });
    std::unique_ptr<KiriDocumentSession> session = createSession(directMediaNavigationProvider);

    session->setSourceUrl(clip);

    QCOMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Video);
    KiriMediaInformation *mediaInformation = session->mediaInformation();
    QVERIFY(mediaInformation->available());
    QCOMPARE(mediaInformation->title(), QStringLiteral("clip.mp4"));
    QCOMPARE(mediaInformation->summary(), QStringLiteral("Video"));
    QCOMPARE(mediaInformation->mediaSectionTitle(), QStringLiteral("Video"));
    QVERIFY(!mediaInformation->hasCameraSection());
    QVERIFY(!mediaInformation->hasAdvancedSection());
    QCOMPARE(mediaInformation->cameraRows()->rowCount(), 0);
    QCOMPARE(
        mediaInformationValueForLabel(*mediaInformation->generalRows(), QStringLiteral("Type")),
        QStringLiteral("Video"));
    QCOMPARE(mediaInformation->mediaRows()->rowCount(), 0);
}

void TestKiriDocumentSession::mediaInformationDerivesFilenameAndPathFromTargetUrl()
{
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    const QUrl clip(QStringLiteral("zip:///books/book.zip!/chapter/clip.mp4"));
    directMediaNavigationProvider.setMedia(QUrl(QStringLiteral("zip:///books/book.zip!/chapter/")),
        { directMediaNavigationCandidate(clip) });
    std::unique_ptr<KiriDocumentSession> session = createSession(directMediaNavigationProvider);

    session->setSourceUrl(clip);

    KiriMediaInformation *mediaInformation = session->mediaInformation();
    QCOMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Video);
    QVERIFY(mediaInformation->available());
    QCOMPARE(mediaInformation->title(), QStringLiteral("clip.mp4"));
    QCOMPARE(
        mediaInformationValueForLabel(*mediaInformation->generalRows(), QStringLiteral("Path")),
        QStringLiteral("zip:///books/book.zip!/chapter/clip.mp4"));
    QVERIFY(mediaInformation->canCopyFilePath());
    QVERIFY(mediaInformation->canOpenContainingFolder());
}

void TestKiriDocumentSession::mediaInformationRowModelsExposeLabelAndValueRoles()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString imagePath = directory.filePath(QStringLiteral("roles.png"));
    QVERIFY(writeTestImage(imagePath));

    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    const QUrl imageUrl = localUrl(imagePath);
    directMediaNavigationProvider.setMedia(localUrl(directory.path() + QStringLiteral("/")),
        { directMediaNavigationCandidate(imageUrl) });
    std::unique_ptr<KiriDocumentSession> session = createSession(directMediaNavigationProvider);

    session->setSourceUrl(imageUrl);

    QTRY_COMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Ready);
    QAbstractItemModel *model = session->mediaInformation()->generalRows();
    QVERIFY(model != nullptr);
    QCOMPARE(roleForName(*model, QByteArrayLiteral("label")),
        static_cast<int>(KiriMediaInformationRowModel::LabelRole));
    QCOMPARE(roleForName(*model, QByteArrayLiteral("value")),
        static_cast<int>(KiriMediaInformationRowModel::ValueRole));
    QCOMPARE(mediaInformationRowData(*model, 0, QByteArrayLiteral("label")).toString(),
        QStringLiteral("Type"));
    QCOMPARE(mediaInformationRowData(*model, 0, QByteArrayLiteral("value")).toString(),
        QStringLiteral("Image"));
}

void TestKiriDocumentSession::directVideoRoutesToVideoDocumentWithOriginalSource()
{
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    const QUrl clip = localUrl(QStringLiteral("/media/clip.mp4"));
    directMediaNavigationProvider.setMedia(
        localUrl(QStringLiteral("/media/")), { directMediaNavigationCandidate(clip) });
    std::unique_ptr<KiriDocumentSession> session = createSession(directMediaNavigationProvider);

    session->setSourceUrl(clip);

    QCOMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Video);
    QCOMPARE(session->sourceUrl(), clip);
    QCOMPARE(session->videoDocument()->sourceUrl(), clip);
    QCOMPARE(session->windowTitleSubject(), QStringLiteral("clip.mp4"));
    QCOMPARE(session->imageDocument()->sourceUrl(), QUrl());
    QVERIFY(session->activeNavigationAvailable());
    QVERIFY(session->activeNavigationKnown());
    QVERIFY(session->activeNavigationEditable());
    QCOMPARE(session->activeNavigationCurrentNumber(), 1);
    QCOMPARE(session->activeNavigationCount(), 1);
    QVERIFY(!session->canOpenPreviousActiveNavigation());
    QVERIFY(!session->canOpenNextActiveNavigation());
    QVERIFY(session->atKnownFirstActiveNavigation());
    QVERIFY(session->atKnownLastActiveNavigation());
}

void TestKiriDocumentSession::activeZoomReadoutFollowsSessionDocumentKind()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString imagePath = directory.filePath(QStringLiteral("01.png"));
    QVERIFY(writeTestImage(imagePath));

    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    const QUrl imageUrl = localUrl(imagePath);
    const QUrl videoUrl = localUrl(directory.filePath(QStringLiteral("02.mp4")));
    directMediaNavigationProvider.setMedia(localUrl(directory.path() + QStringLiteral("/")),
        { directMediaNavigationCandidate(imageUrl), directMediaNavigationCandidate(videoUrl) });
    std::unique_ptr<KiriDocumentSession> session = createSession(directMediaNavigationProvider);

    QCOMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Empty);
    QVERIFY(!session->activeZoomPercentAvailable());
    QVERIFY(!session->activeZoomPercentKnown());
    QCOMPARE(session->activeZoomPercent(), 0.0);
    QVERIFY(!session->activeZoomEditable());

    session->setSourceUrl(videoUrl);

    QCOMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Video);
    QVERIFY(session->activeZoomPercentAvailable());
    QVERIFY(!session->activeZoomPercentKnown());
    QCOMPARE(session->activeZoomPercent(), 0.0);
    QVERIFY(!session->activeZoomEditable());

    session->imageDocument()->setViewportSize(QSizeF(400.0, 300.0));
    session->setSourceUrl(imageUrl);

    QTRY_COMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Image);
    QTRY_COMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Ready);
    QVERIFY(session->activeZoomPercentAvailable());
    QVERIFY(session->activeZoomPercentKnown());
    QVERIFY(session->activeZoomPercent() > 0.0);
    QVERIFY(session->activeZoomEditable());
}

void TestKiriDocumentSession::archiveAndDirectoryInputsRouteToImageDocument()
{
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    std::unique_ptr<KiriDocumentSession> session = createSession(directMediaNavigationProvider);

    const QUrl archive = localUrl(QStringLiteral("/books/book.zip"));
    session->setSourceUrl(archive);

    QCOMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Image);
    QCOMPARE(session->imageDocument()->sourceUrl(), archive);
    QCOMPARE(session->videoDocument()->sourceUrl(), QUrl());

    const QUrl directory = localUrl(QStringLiteral("/books/"));
    session->setSourceUrl(directory);
    QCOMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Image);
    QCOMPARE(session->imageDocument()->sourceUrl(), directory);
}

void TestKiriDocumentSession::directImageAfterVideoRestoresImageDocument()
{
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    const QUrl clip = localUrl(QStringLiteral("/media/01.mp4"));
    const QUrl image = localUrl(QStringLiteral("/media/02.svg"));
    directMediaNavigationProvider.setMedia(localUrl(QStringLiteral("/media/")),
        { directMediaNavigationCandidate(clip), directMediaNavigationCandidate(image) });
    std::unique_ptr<KiriDocumentSession> session = createSession(directMediaNavigationProvider);

    session->setSourceUrl(clip);
    QCOMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Video);
    QCOMPARE(session->videoDocument()->sourceUrl(), clip);

    session->setSourceUrl(image);

    QCOMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Image);
    QCOMPARE(session->sourceUrl(), image);
    QCOMPARE(session->imageDocument()->sourceUrl(), image);
    QCOMPARE(session->videoDocument()->sourceUrl(), QUrl());
}

void TestKiriDocumentSession::kioArchiveImageAfterKioArchiveVideoUsesOriginalImageUrl()
{
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    KiriView::TestSupport::ManualImageDataLoader dataLoader;
    const QUrl parentUrl(QStringLiteral("zip:///books/book.zip!/chapter/"));
    const QUrl videoUrl(QStringLiteral("zip:///books/book.zip!/chapter/clip.mp4"));
    const QUrl imageUrl(QStringLiteral("zip:///books/book.zip!/chapter/page.png"));
    directMediaNavigationProvider.setMedia(parentUrl, { directMediaNavigationCandidate(videoUrl) });
    std::unique_ptr<KiriDocumentSession> session
        = createSessionWithProvider(directMediaNavigationProvider.provider(), nullptr, &dataLoader);

    session->setSourceUrl(videoUrl);
    QCOMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Video);
    QCOMPARE(session->sourceUrl(), videoUrl);
    QCOMPARE(session->videoDocument()->sourceUrl(), videoUrl);

    directMediaNavigationProvider.setMedia(parentUrl, { directMediaNavigationCandidate(imageUrl) });
    session->setSourceUrl(imageUrl);

    QCOMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Image);
    QCOMPARE(session->sourceUrl(), imageUrl);
    QCOMPARE(session->imageDocument()->sourceUrl(), imageUrl);
    QCOMPARE(session->videoDocument()->sourceUrl(), QUrl());
    QTRY_COMPARE(dataLoader.loadCount(), std::size_t(1));
    QCOMPARE(dataLoader.backLoad().url, imageUrl);
    QVERIFY(dataLoader.backLoad().openedCollectionScope.isEmpty());
    QVERIFY(!dataLoader.backLoad().url.isLocalFile());
    QVERIFY(!dataLoader.backLoad().url.toString().contains(QStringLiteral("kio-fuse")));
}

void TestKiriDocumentSession::directImageDirectMediaNavigationIncludesSiblingVideos()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString imagePath = directory.filePath(QStringLiteral("01.png"));
    QVERIFY(writeTestImage(imagePath));

    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    const QUrl imageUrl = localUrl(imagePath);
    const QUrl videoUrl = localUrl(directory.filePath(QStringLiteral("02.mp4")));
    directMediaNavigationProvider.setMedia(localUrl(directory.path() + QStringLiteral("/")),
        { directMediaNavigationCandidate(imageUrl), directMediaNavigationCandidate(videoUrl) });
    std::unique_ptr<KiriDocumentSession> session = createSession(directMediaNavigationProvider);

    session->setSourceUrl(imageUrl);

    QTRY_COMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Image);
    QTRY_COMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Ready);
    QCOMPARE(session->windowTitleSubject(), QStringLiteral("01.png – 2×2"));
    QVERIFY(session->activeNavigationAvailable());
    QVERIFY(session->activeNavigationKnown());
    QCOMPARE(session->activeNavigationCurrentNumber(), 1);
    QCOMPARE(session->activeNavigationCount(), 2);
    QVERIFY(!session->canOpenPreviousActiveNavigation());
    QVERIFY(session->canOpenNextActiveNavigation());
    QVERIFY(session->atKnownFirstActiveNavigation());
    QVERIFY(!session->atKnownLastActiveNavigation());

    session->openActiveNavigationAtNumber(2);

    QCOMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Video);
    QCOMPARE(session->sourceUrl(), videoUrl);
    QCOMPARE(session->videoDocument()->sourceUrl(), videoUrl);
    QCOMPARE(session->windowTitleSubject(), QStringLiteral("02.mp4"));
    QCOMPARE(session->activeNavigationCurrentNumber(), 2);
    QCOMPARE(session->activeNavigationCount(), 2);
    QVERIFY(session->atKnownLastActiveNavigation());
}

void TestKiriDocumentSession::
    directImageActiveNavigationIgnoresImageDocumentDirectoryPageCandidates()
{
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    KiriView::TestSupport::FakeImageDocumentPageCandidateProvider imageDocumentPageCandidates;
    KiriView::TestSupport::ManualImageDataLoader imageDataLoader;
    const QUrl imageUrl = localUrl(QStringLiteral("/media/01.png"));
    const QUrl videoUrl = localUrl(QStringLiteral("/media/02.mp4"));
    const QUrl extraImageUrl = localUrl(QStringLiteral("/media/03.png"));
    directMediaNavigationProvider.setMedia(localUrl(QStringLiteral("/media/")),
        { directMediaNavigationCandidate(imageUrl), directMediaNavigationCandidate(videoUrl) });
    imageDocumentPageCandidates.setDirectoryImages(localUrl(QStringLiteral("/media/")),
        { KiriView::TestSupport::imageDocumentPageCandidate(imageUrl),
            KiriView::ImageDocumentPageCandidate {
                videoUrl, QStringLiteral("02.mp4"), KiriView::ImageDocumentPageKind::Video },
            KiriView::TestSupport::imageDocumentPageCandidate(extraImageUrl) });
    std::unique_ptr<KiriDocumentSession> session
        = createSessionWithProvider(directMediaNavigationProvider.provider(), nullptr,
            &imageDataLoader, imageDocumentPageCandidates.provider());

    session->setSourceUrl(imageUrl);
    QCOMPARE(imageDataLoader.loadCount(), std::size_t(1));
    imageDataLoader.finishBackLoad(QByteArrayLiteral("image"));

    QTRY_COMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Image);
    QTRY_COMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Ready);
    QCOMPARE(session->activeNavigationBoundaryScope(),
        KiriDocumentSession::ActiveNavigationBoundaryScope::DirectMediaNavigationBoundary);
    QVERIFY(session->activeNavigationKnown());
    QCOMPARE(session->activeNavigationCurrentNumber(), 1);
    QCOMPARE(session->activeNavigationCount(), 2);
    QCOMPARE(session->imageDocument()->currentPageNumber(), 0);
    QCOMPARE(session->imageDocument()->pageCount(), 0);
}

void TestKiriDocumentSession::
    directArchiveEntryImageUsesDirectMediaNavigationWithoutImageDocumentPages()
{
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    KiriView::TestSupport::FakeImageDocumentPageCandidateProvider imageDocumentPageCandidates;
    KiriView::TestSupport::ManualImageDataLoader imageDataLoader;
    const QUrl directoryUrl(QStringLiteral("zip:///path/archive.zip!/chapter/"));
    const QUrl imageUrl(QStringLiteral("zip:///path/archive.zip!/chapter/01.png"));
    const QUrl videoUrl(QStringLiteral("zip:///path/archive.zip!/chapter/02.mp4"));
    directMediaNavigationProvider.setMedia(directoryUrl,
        { directMediaNavigationCandidate(imageUrl), directMediaNavigationCandidate(videoUrl) });
    imageDocumentPageCandidates.setDirectoryImages(directoryUrl,
        { KiriView::TestSupport::imageDocumentPageCandidate(imageUrl),
            KiriView::ImageDocumentPageCandidate {
                videoUrl, QStringLiteral("02.mp4"), KiriView::ImageDocumentPageKind::Video } });
    std::unique_ptr<KiriDocumentSession> session
        = createSessionWithProvider(directMediaNavigationProvider.provider(), nullptr,
            &imageDataLoader, imageDocumentPageCandidates.provider());

    session->setSourceUrl(imageUrl);
    QCOMPARE(imageDataLoader.loadCount(), std::size_t(1));
    imageDataLoader.finishBackLoad(QByteArrayLiteral("image"));

    QTRY_COMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Image);
    QTRY_COMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Ready);
    QCOMPARE(session->activeNavigationBoundaryScope(),
        KiriDocumentSession::ActiveNavigationBoundaryScope::DirectMediaNavigationBoundary);
    QVERIFY(session->activeNavigationKnown());
    QCOMPARE(session->activeNavigationCurrentNumber(), 1);
    QCOMPARE(session->activeNavigationCount(), 2);
    QCOMPARE(session->imageDocument()->currentPageNumber(), 0);
    QCOMPARE(session->imageDocument()->pageCount(), 0);
}

void TestKiriDocumentSession::directMediaThumbnailModelTracksSiblingCandidates()
{
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    const QUrl imageUrl = localUrl(QStringLiteral("/media/01.png"));
    const QUrl videoUrl = localUrl(QStringLiteral("/media/02.mp4"));
    directMediaNavigationProvider.setMedia(localUrl(QStringLiteral("/media/")),
        { directMediaNavigationCandidate(imageUrl), directMediaNavigationCandidate(videoUrl) });
    std::unique_ptr<KiriDocumentSession> session = createSession(directMediaNavigationProvider);

    session->setSourceUrl(videoUrl);

    QAbstractItemModel *model = session->activeNavigationThumbnailModel();
    QVERIFY(model != nullptr);
    QVERIFY(session->activeNavigationKnown());
    QCOMPARE(session->activeNavigationCurrentNumber(), 2);
    QCOMPARE(session->activeNavigationCount(), 2);
    QCOMPARE(model->rowCount(), 2);
    compareThumbnailRow(*session, 0, 1, imageUrl, QStringLiteral("01.png"),
        QStringLiteral("image-x-generic-symbolic"), false);
    compareThumbnailRow(*session, 1, 2, videoUrl, QStringLiteral("02.mp4"),
        QStringLiteral("video-x-generic-symbolic"), true);
}

void TestKiriDocumentSession::directMediaThumbnailModelStaysEmptyUntilCandidatesAreKnown()
{
    ManualDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    const QUrl imageUrl = localUrl(QStringLiteral("/media/01.png"));
    const QUrl videoUrl = localUrl(QStringLiteral("/media/02.mp4"));
    std::unique_ptr<KiriDocumentSession> session
        = createSessionWithProvider(directMediaNavigationProvider.provider());

    session->setSourceUrl(videoUrl);

    QAbstractItemModel *model = session->activeNavigationThumbnailModel();
    QVERIFY(model != nullptr);
    QCOMPARE(directMediaNavigationProvider.loadCount(), std::size_t(1));
    QVERIFY(session->activeNavigationAvailable());
    QVERIFY(!session->activeNavigationKnown());
    QCOMPARE(model->rowCount(), 0);

    directMediaNavigationProvider.finishLoad(
        0, { directMediaNavigationCandidate(imageUrl), directMediaNavigationCandidate(videoUrl) });

    QVERIFY(session->activeNavigationKnown());
    QCOMPARE(session->activeNavigationCurrentNumber(), 2);
    QCOMPARE(session->activeNavigationCount(), 2);
    QCOMPARE(model->rowCount(), 2);
    compareThumbnailRow(*session, 1, 2, videoUrl, QStringLiteral("02.mp4"),
        QStringLiteral("video-x-generic-symbolic"), true);
}

void TestKiriDocumentSession::activeNavigationThumbnailModelExposesSourceNeutralResultRoles()
{
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    const QUrl imageUrl = localUrl(QStringLiteral("/media/01.png"));
    directMediaNavigationProvider.setMedia(
        localUrl(QStringLiteral("/media/")), { directMediaNavigationCandidate(imageUrl) });
    std::unique_ptr<KiriDocumentSession> session = createSession(directMediaNavigationProvider);

    session->setSourceUrl(imageUrl);

    QAbstractItemModel *model = session->activeNavigationThumbnailModel();
    QVERIFY(model != nullptr);
    QVERIFY(session->activeNavigationKnown());
    QCOMPARE(model->rowCount(), 1);

    QVERIFY(roleForName(*model, QByteArrayLiteral("thumbnailStatus")) >= 0);
    QVERIFY(roleForName(*model, QByteArrayLiteral("thumbnailImageSource")) >= 0);
    QCOMPARE(thumbnailDataForRoleName(*session, 0, QByteArrayLiteral("thumbnailStatus")).toInt(),
        static_cast<int>(KiriDocumentSession::ThumbnailResultStatus::NoThumbnailResult));
    QCOMPARE(
        thumbnailDataForRoleName(*session, 0, QByteArrayLiteral("thumbnailImageSource")).toUrl(),
        QUrl());
}

void TestKiriDocumentSession::activeNavigationThumbnailDemandSurfaceValidatesIdentityAndGeneration()
{
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    const QUrl imageUrl = localUrl(QStringLiteral("/media/01.png"));
    const QUrl videoUrl = localUrl(QStringLiteral("/media/02.mp4"));
    directMediaNavigationProvider.setMedia(localUrl(QStringLiteral("/media/")),
        { directMediaNavigationCandidate(imageUrl), directMediaNavigationCandidate(videoUrl) });
    std::unique_ptr<KiriDocumentSession> session = createSession(directMediaNavigationProvider);

    session->setSourceUrl(videoUrl);

    QAbstractItemModel *model = session->activeNavigationThumbnailModel();
    QVERIFY(model != nullptr);
    QVERIFY(session->activeNavigationKnown());
    QCOMPARE(model->rowCount(), 2);

    const int generationRole = roleForName(*model, QByteArrayLiteral("navigationGeneration"));
    QVERIFY(generationRole >= 0);
    const quint64 generation = model->data(model->index(1, 0), generationRole).toULongLong();
    QVERIFY(generation > 0);

    QCOMPARE(session->activeNavigationThumbnailDemandBucket(0),
        KiriDocumentSession::ThumbnailDemandBucket::NoThumbnailDemandBucket);
    QCOMPARE(session->activeNavigationThumbnailDemandBucket(128),
        KiriDocumentSession::ThumbnailDemandBucket::NormalThumbnailDemandBucket);
    QCOMPARE(session->activeNavigationThumbnailDemandBucket(129),
        KiriDocumentSession::ThumbnailDemandBucket::LargeThumbnailDemandBucket);
    QCOMPARE(session->activeNavigationThumbnailDemandBucket(257),
        KiriDocumentSession::ThumbnailDemandBucket::XLargeThumbnailDemandBucket);
    QCOMPARE(session->activeNavigationThumbnailDemandBucket(513),
        KiriDocumentSession::ThumbnailDemandBucket::XXLargeThumbnailDemandBucket);

    QVERIFY(session->reportActiveNavigationThumbnailDemand(2, videoUrl, 96,
        KiriDocumentSession::ThumbnailDemandPriority::VisibleThumbnailDemand, generation));
    QVERIFY(!session->reportActiveNavigationThumbnailDemand(2, videoUrl, 127,
        KiriDocumentSession::ThumbnailDemandPriority::VisibleThumbnailDemand, generation));
    QVERIFY(session->reportActiveNavigationThumbnailDemand(2, videoUrl, 129,
        KiriDocumentSession::ThumbnailDemandPriority::VisibleThumbnailDemand, generation));
    QVERIFY(!session->reportActiveNavigationThumbnailDemand(2, videoUrl, 130,
        KiriDocumentSession::ThumbnailDemandPriority::VisibleThumbnailDemand, generation));
    QVERIFY(session->reportActiveNavigationThumbnailDemand(2, videoUrl, 130,
        KiriDocumentSession::ThumbnailDemandPriority::NearbyThumbnailDemand, generation));

    QVERIFY(!session->reportActiveNavigationThumbnailDemand(2, imageUrl, 256,
        KiriDocumentSession::ThumbnailDemandPriority::NearbyThumbnailDemand, generation));
    QVERIFY(!session->reportActiveNavigationThumbnailDemand(3, videoUrl, 256,
        KiriDocumentSession::ThumbnailDemandPriority::NearbyThumbnailDemand, generation));
    QVERIFY(!session->reportActiveNavigationThumbnailDemand(2, videoUrl, 256,
        KiriDocumentSession::ThumbnailDemandPriority::NearbyThumbnailDemand, generation + 1));
    QVERIFY(!session->reportActiveNavigationThumbnailDemand(2, videoUrl, 0,
        KiriDocumentSession::ThumbnailDemandPriority::NearbyThumbnailDemand, generation));
}

void TestKiriDocumentSession::activeNavigationThumbnailDemandProjectsPendingAndUnsupportedResults()
{
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    const QUrl imageUrl = localUrl(QStringLiteral("/media/01.png"));
    const QUrl videoUrl = localUrl(QStringLiteral("/media/02.mp4"));
    directMediaNavigationProvider.setMedia(localUrl(QStringLiteral("/media/")),
        { directMediaNavigationCandidate(imageUrl), directMediaNavigationCandidate(videoUrl) });
    std::unique_ptr<KiriDocumentSession> session = createSession(directMediaNavigationProvider);

    session->setSourceUrl(imageUrl);

    QAbstractItemModel *model = session->activeNavigationThumbnailModel();
    QVERIFY(model != nullptr);
    QVERIFY(session->activeNavigationKnown());
    QCOMPARE(model->rowCount(), 2);

    const quint64 generation = thumbnailData(
        *session, 0, KiriView::ActiveNavigationThumbnailModel::NavigationGenerationRole)
                                   .toULongLong();
    QVERIFY(generation > 0);

    QVERIFY(session->reportActiveNavigationThumbnailDemand(1, imageUrl, 96,
        KiriDocumentSession::ThumbnailDemandPriority::VisibleThumbnailDemand, generation));
    QCOMPARE(thumbnailDataForRoleName(*session, 0, QByteArrayLiteral("thumbnailStatus")).toInt(),
        static_cast<int>(KiriDocumentSession::ThumbnailResultStatus::PendingThumbnailResult));
    QCOMPARE(
        thumbnailDataForRoleName(*session, 0, QByteArrayLiteral("thumbnailImageSource")).toUrl(),
        QUrl());

    QVERIFY(session->reportActiveNavigationThumbnailDemand(2, videoUrl, 96,
        KiriDocumentSession::ThumbnailDemandPriority::VisibleThumbnailDemand, generation));
    QCOMPARE(thumbnailDataForRoleName(*session, 1, QByteArrayLiteral("thumbnailStatus")).toInt(),
        static_cast<int>(KiriDocumentSession::ThumbnailResultStatus::UnsupportedThumbnailResult));
    QCOMPARE(
        thumbnailDataForRoleName(*session, 1, QByteArrayLiteral("thumbnailImageSource")).toUrl(),
        QUrl());
}

void TestKiriDocumentSession::directImageThumbnailDemandProjectsReadyCacheHitSource()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString imagePath = directory.filePath(QStringLiteral("01.png"));
    QVERIFY(writeTestImage(imagePath));

    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    const QUrl imageUrl = localUrl(imagePath);
    directMediaNavigationProvider.setMedia(localUrl(directory.path() + QStringLiteral("/")),
        { directMediaNavigationCandidate(imageUrl) });
    FakeThumbnailLookupProvider thumbnailLookup;
    QImage thumbnail(QSize(2, 1), QImage::Format_RGBA8888);
    thumbnail.fill(Qt::blue);
    thumbnailLookup.result = KiriView::ThumbnailCacheLookupResult {
        KiriView::ThumbnailCacheLookupStatus::Ready,
        thumbnail,
        KiriView::ActiveNavigationThumbnailDemandBucket::Normal,
        KiriView::ActiveNavigationThumbnailDemandBucket::Normal,
        QStringLiteral("/cache/thumbnail.png"),
        {},
    };
    auto store = std::make_shared<KiriView::ThumbnailImageStore>();
    std::unique_ptr<KiriDocumentSession> session = createSessionWithProvider(
        directMediaNavigationProvider.provider(), nullptr, nullptr, {},
        KiriView::TestSupport::staticImageDataDecoder(), {}, thumbnailLookup.provider(), {}, store);

    session->setSourceUrl(imageUrl);

    QAbstractItemModel *model = session->activeNavigationThumbnailModel();
    QVERIFY(model != nullptr);
    QTRY_COMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Ready);
    QCOMPARE(model->rowCount(), 1);

    const quint64 generation = thumbnailData(
        *session, 0, KiriView::ActiveNavigationThumbnailModel::NavigationGenerationRole)
                                   .toULongLong();
    QVERIFY(session->reportActiveNavigationThumbnailDemand(1, imageUrl, 96,
        KiriDocumentSession::ThumbnailDemandPriority::VisibleThumbnailDemand, generation));

    QCOMPARE(thumbnailLookup.requests.size(), std::size_t(1));
    QCOMPARE(thumbnailLookup.requests.front().localPathBytes, QFile::encodeName(imagePath));
    QCOMPARE(thumbnailDataForRoleName(*session, 0, QByteArrayLiteral("thumbnailStatus")).toInt(),
        static_cast<int>(KiriDocumentSession::ThumbnailResultStatus::ReadyThumbnailResult));
    const QUrl source
        = thumbnailDataForRoleName(*session, 0, QByteArrayLiteral("thumbnailImageSource")).toUrl();
    QVERIFY(source.toString().startsWith(QStringLiteral("image://kiriview-thumbnails/")));
    QCOMPARE(store->size(), qsizetype(1));
}

void TestKiriDocumentSession::directImageThumbnailDemandProjectsReadyGeneratedSource()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString imagePath = directory.filePath(QStringLiteral("01.png"));
    QVERIFY(writeTestImage(imagePath));

    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    const QUrl imageUrl = localUrl(imagePath);
    directMediaNavigationProvider.setMedia(localUrl(directory.path() + QStringLiteral("/")),
        { directMediaNavigationCandidate(imageUrl) });
    FakeThumbnailLookupProvider thumbnailLookup;
    thumbnailLookup.result.status = KiriView::ThumbnailCacheLookupStatus::Missing;
    FakeThumbnailGenerationProvider thumbnailGeneration;
    QImage thumbnail(QSize(2, 1), QImage::Format_RGBA8888);
    thumbnail.fill(Qt::green);
    thumbnailGeneration.result = KiriView::ThumbnailGenerationResult {
        KiriView::ThumbnailGenerationStatus::Ready,
        thumbnail,
        KiriView::ActiveNavigationThumbnailDemandBucket::Normal,
        QStringLiteral("/cache/generated.png"),
        {},
    };
    auto store = std::make_shared<KiriView::ThumbnailImageStore>();
    std::unique_ptr<KiriDocumentSession> session
        = createSessionWithProvider(directMediaNavigationProvider.provider(), nullptr, nullptr, {},
            KiriView::TestSupport::staticImageDataDecoder(), {}, thumbnailLookup.provider(),
            thumbnailGeneration.provider(), store);

    session->setSourceUrl(imageUrl);

    QAbstractItemModel *model = session->activeNavigationThumbnailModel();
    QVERIFY(model != nullptr);
    QTRY_COMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Ready);
    const quint64 generation = thumbnailData(
        *session, 0, KiriView::ActiveNavigationThumbnailModel::NavigationGenerationRole)
                                   .toULongLong();
    QVERIFY(session->reportActiveNavigationThumbnailDemand(1, imageUrl, 96,
        KiriDocumentSession::ThumbnailDemandPriority::VisibleThumbnailDemand, generation));

    QCOMPARE(thumbnailLookup.requests.size(), std::size_t(1));
    QCOMPARE(thumbnailGeneration.requests.size(), std::size_t(1));
    QCOMPARE(thumbnailGeneration.requests.front().localPathBytes, QFile::encodeName(imagePath));
    QCOMPARE(thumbnailDataForRoleName(*session, 0, QByteArrayLiteral("thumbnailStatus")).toInt(),
        static_cast<int>(KiriDocumentSession::ThumbnailResultStatus::ReadyThumbnailResult));
    const QUrl source
        = thumbnailDataForRoleName(*session, 0, QByteArrayLiteral("thumbnailImageSource")).toUrl();
    QVERIFY(source.toString().startsWith(QStringLiteral("image://kiriview-thumbnails/")));
    QCOMPARE(store->size(), qsizetype(1));
}

void TestKiriDocumentSession::directImageThumbnailDemandKeepsFallbackForFailedLookup()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString imagePath = directory.filePath(QStringLiteral("01.png"));
    QVERIFY(writeTestImage(imagePath));

    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    const QUrl imageUrl = localUrl(imagePath);
    directMediaNavigationProvider.setMedia(localUrl(directory.path() + QStringLiteral("/")),
        { directMediaNavigationCandidate(imageUrl) });
    FakeThumbnailLookupProvider thumbnailLookup;
    thumbnailLookup.result.status = KiriView::ThumbnailCacheLookupStatus::Failed;
    std::unique_ptr<KiriDocumentSession> session
        = createSessionWithProvider(directMediaNavigationProvider.provider(), nullptr, nullptr, {},
            KiriView::TestSupport::staticImageDataDecoder(), {}, thumbnailLookup.provider());

    session->setSourceUrl(imageUrl);

    QAbstractItemModel *model = session->activeNavigationThumbnailModel();
    QVERIFY(model != nullptr);
    QTRY_COMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Ready);
    const quint64 generation = thumbnailData(
        *session, 0, KiriView::ActiveNavigationThumbnailModel::NavigationGenerationRole)
                                   .toULongLong();
    QVERIFY(session->reportActiveNavigationThumbnailDemand(1, imageUrl, 96,
        KiriDocumentSession::ThumbnailDemandPriority::VisibleThumbnailDemand, generation));

    QCOMPARE(thumbnailDataForRoleName(*session, 0, QByteArrayLiteral("thumbnailStatus")).toInt(),
        static_cast<int>(KiriDocumentSession::ThumbnailResultStatus::FailedThumbnailResult));
    QCOMPARE(
        thumbnailDataForRoleName(*session, 0, QByteArrayLiteral("thumbnailImageSource")).toUrl(),
        QUrl());
}

void TestKiriDocumentSession::directImageThumbnailDemandKeepsFallbackForFailedGeneration()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString imagePath = directory.filePath(QStringLiteral("01.png"));
    QVERIFY(writeTestImage(imagePath));

    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    const QUrl imageUrl = localUrl(imagePath);
    directMediaNavigationProvider.setMedia(localUrl(directory.path() + QStringLiteral("/")),
        { directMediaNavigationCandidate(imageUrl) });
    FakeThumbnailLookupProvider thumbnailLookup;
    thumbnailLookup.result.status = KiriView::ThumbnailCacheLookupStatus::Missing;
    FakeThumbnailGenerationProvider thumbnailGeneration;
    thumbnailGeneration.result.status = KiriView::ThumbnailGenerationStatus::Failed;
    std::unique_ptr<KiriDocumentSession> session
        = createSessionWithProvider(directMediaNavigationProvider.provider(), nullptr, nullptr, {},
            KiriView::TestSupport::staticImageDataDecoder(), {}, thumbnailLookup.provider(),
            thumbnailGeneration.provider());

    session->setSourceUrl(imageUrl);

    QAbstractItemModel *model = session->activeNavigationThumbnailModel();
    QVERIFY(model != nullptr);
    QTRY_COMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Ready);
    const quint64 generation = thumbnailData(
        *session, 0, KiriView::ActiveNavigationThumbnailModel::NavigationGenerationRole)
                                   .toULongLong();
    QVERIFY(session->reportActiveNavigationThumbnailDemand(1, imageUrl, 96,
        KiriDocumentSession::ThumbnailDemandPriority::VisibleThumbnailDemand, generation));

    QCOMPARE(thumbnailLookup.requests.size(), std::size_t(1));
    QCOMPARE(thumbnailGeneration.requests.size(), std::size_t(1));
    QCOMPARE(thumbnailDataForRoleName(*session, 0, QByteArrayLiteral("thumbnailStatus")).toInt(),
        static_cast<int>(KiriDocumentSession::ThumbnailResultStatus::FailedThumbnailResult));
    QCOMPARE(
        thumbnailDataForRoleName(*session, 0, QByteArrayLiteral("thumbnailImageSource")).toUrl(),
        QUrl());
}

void TestKiriDocumentSession::defaultMediaProviderListsLocalDirectImageSiblings()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString firstImagePath = directory.filePath(QStringLiteral("01.png"));
    const QString videoPath = directory.filePath(QStringLiteral("02.mp4"));
    const QString currentImagePath = directory.filePath(QStringLiteral("03.png"));
    QVERIFY(writeTestImage(firstImagePath));
    QVERIFY(writeEmptyFile(videoPath));
    QVERIFY(writeTestImage(currentImagePath));

    std::unique_ptr<KiriDocumentSession> session
        = createSessionWithProvider(KiriView::DirectMediaNavigationCandidateProvider {});

    session->setSourceUrl(localUrl(currentImagePath));

    QTRY_COMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Image);
    QTRY_COMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Ready);
    QTRY_VERIFY(session->activeNavigationKnown());
    QVERIFY(session->activeNavigationEditable());
    QCOMPARE(session->activeNavigationCurrentNumber(), 3);
    QCOMPARE(session->activeNavigationCount(), 3);
    QVERIFY(session->canOpenPreviousActiveNavigation());
    QVERIFY(!session->canOpenNextActiveNavigation());
    QVERIFY(!session->atKnownFirstActiveNavigation());
    QVERIFY(session->atKnownLastActiveNavigation());
}

void TestKiriDocumentSession::defaultMediaProviderListsLocalDirectVideoSiblings()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString firstImagePath = directory.filePath(QStringLiteral("01.png"));
    const QString currentVideoPath = directory.filePath(QStringLiteral("02.mp4"));
    const QString nextVideoPath = directory.filePath(QStringLiteral("03.mov"));
    QVERIFY(writeTestImage(firstImagePath));
    QVERIFY(writeEmptyFile(currentVideoPath));
    QVERIFY(writeEmptyFile(nextVideoPath));

    std::unique_ptr<KiriDocumentSession> session
        = createSessionWithProvider(KiriView::DirectMediaNavigationCandidateProvider {});

    session->setSourceUrl(localUrl(currentVideoPath));

    QCOMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Video);
    QCOMPARE(session->videoDocument()->sourceUrl(), localUrl(currentVideoPath));
    QVERIFY(session->activeNavigationAvailable());
    QTRY_VERIFY(session->activeNavigationKnown());
    QVERIFY(session->activeNavigationEditable());
    QCOMPARE(session->activeNavigationCurrentNumber(), 2);
    QCOMPARE(session->activeNavigationCount(), 3);
    QVERIFY(session->canOpenPreviousActiveNavigation());
    QVERIFY(session->canOpenNextActiveNavigation());
    QVERIFY(!session->atKnownFirstActiveNavigation());
    QVERIFY(!session->atKnownLastActiveNavigation());
}

void TestKiriDocumentSession::freshDirectImageReadoutUsesRequestedCursorBeforeDisplayedUrl()
{
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    KiriView::TestSupport::ManualImageDataLoader imageDataLoader;
    const QUrl imageUrl = localUrl(QStringLiteral("/media/01.png"));
    const QUrl videoUrl = localUrl(QStringLiteral("/media/02.mp4"));
    directMediaNavigationProvider.setMedia(localUrl(QStringLiteral("/media/")),
        { directMediaNavigationCandidate(imageUrl), directMediaNavigationCandidate(videoUrl) });
    std::unique_ptr<KiriDocumentSession> session
        = createSession(directMediaNavigationProvider, nullptr, &imageDataLoader);

    session->setSourceUrl(imageUrl);

    QCOMPARE(imageDataLoader.loadCount(), std::size_t(1));
    QCOMPARE(session->imageDocument()->displayedUrl(), QUrl());
    QVERIFY(session->activeNavigationAvailable());
    QCOMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Loading);
    QVERIFY(session->activeNavigationKnown());
    QVERIFY(session->activeNavigationEditable());
    QCOMPARE(session->activeNavigationCurrentNumber(), 1);
    QCOMPARE(session->activeNavigationCount(), 2);
}

void TestKiriDocumentSession::directImageDocumentPageCandidateCompletionSurvivesCursorConfirmation()
{
    ManualDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    KiriView::TestSupport::ManualImageDataLoader imageDataLoader;
    const QUrl imageUrl = localUrl(QStringLiteral("/media/01.png"));
    const QUrl siblingUrl = localUrl(QStringLiteral("/media/02.mp4"));
    std::unique_ptr<KiriDocumentSession> session = createSessionWithProvider(
        directMediaNavigationProvider.provider(), nullptr, &imageDataLoader);

    session->setSourceUrl(imageUrl);

    QCOMPARE(imageDataLoader.loadCount(), std::size_t(1));
    QCOMPARE(directMediaNavigationProvider.loadCount(), std::size_t(1));
    QCOMPARE(
        directMediaNavigationProvider.loadAt(0).parentUrl, localUrl(QStringLiteral("/media/")));
    QVERIFY(session->activeNavigationAvailable());
    QVERIFY(!session->activeNavigationKnown());
    QVERIFY(!session->activeNavigationEditable());

    imageDataLoader.finishBackLoad(QByteArrayLiteral("image"));

    QTRY_COMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Ready);
    QCOMPARE(session->imageDocument()->displayedUrl(), imageUrl);
    QCOMPARE(directMediaNavigationProvider.loadCount(), std::size_t(1));
    QVERIFY(!directMediaNavigationProvider.loadAt(0).canceled);
    QVERIFY(session->activeNavigationAvailable());
    QVERIFY(!session->activeNavigationKnown());
    QVERIFY(!session->activeNavigationEditable());

    directMediaNavigationProvider.finishLoad(0,
        { directMediaNavigationCandidate(imageUrl), directMediaNavigationCandidate(siblingUrl) });

    QVERIFY(session->activeNavigationKnown());
    QVERIFY(session->activeNavigationEditable());
    QCOMPARE(session->activeNavigationCurrentNumber(), 1);
    QCOMPARE(session->activeNavigationCount(), 2);
    QVERIFY(session->canOpenNextActiveNavigation());
}

void TestKiriDocumentSession::directImageReplacementFailureRestoresPreviousMediaCursor()
{
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    KiriView::TestSupport::ManualImageDataLoader imageDataLoader;
    const QUrl firstImage = localUrl(QStringLiteral("/media/01.png"));
    const QUrl secondImage = localUrl(QStringLiteral("/media/02.png"));
    directMediaNavigationProvider.setMedia(localUrl(QStringLiteral("/media/")),
        { directMediaNavigationCandidate(firstImage),
            directMediaNavigationCandidate(secondImage) });
    std::unique_ptr<KiriDocumentSession> session
        = createSession(directMediaNavigationProvider, nullptr, &imageDataLoader);

    session->setSourceUrl(firstImage);
    QCOMPARE(imageDataLoader.loadCount(), std::size_t(1));
    imageDataLoader.finishBackLoad(QByteArrayLiteral("first"));
    QTRY_COMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Ready);
    QCOMPARE(session->imageDocument()->displayedUrl(), firstImage);
    QCOMPARE(session->activeNavigationCurrentNumber(), 1);
    QCOMPARE(session->activeNavigationCount(), 2);
    QVERIFY(!session->canOpenPreviousActiveNavigation());
    QVERIFY(session->canOpenNextActiveNavigation());

    session->setSourceUrl(secondImage);

    QCOMPARE(imageDataLoader.loadCount(), std::size_t(2));
    QCOMPARE(session->imageDocument()->displayedUrl(), firstImage);
    QCOMPARE(session->sourceUrl(), secondImage);
    QVERIFY(session->activeNavigationKnown());
    QVERIFY(session->activeNavigationEditable());
    QCOMPARE(session->activeNavigationCurrentNumber(), 2);
    QCOMPARE(session->activeNavigationCount(), 2);

    imageDataLoader.failBackLoad(QStringLiteral("replacement failed"));

    QTRY_COMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Ready);
    QCOMPARE(session->sourceUrl(), firstImage);
    QCOMPARE(session->imageDocument()->displayedUrl(), firstImage);
    QVERIFY(session->activeNavigationAvailable());
    QVERIFY(session->activeNavigationKnown());
    QVERIFY(session->activeNavigationEditable());
    QCOMPARE(session->activeNavigationCurrentNumber(), 1);
    QCOMPARE(session->activeNavigationCount(), 2);
    QVERIFY(!session->canOpenPreviousActiveNavigation());
    QVERIFY(session->canOpenNextActiveNavigation());
    QVERIFY(session->atKnownFirstActiveNavigation());
    QVERIFY(!session->atKnownLastActiveNavigation());
}

void TestKiriDocumentSession::
    stalePendingDirectImageDocumentPageCandidateCompletionCannotPublishForNewCursor()
{
    ManualDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    KiriView::TestSupport::ManualImageDataLoader imageDataLoader;
    std::unique_ptr<KiriDocumentSession> session = createSessionWithProvider(
        directMediaNavigationProvider.provider(), nullptr, &imageDataLoader);

    const QUrl firstImage = localUrl(QStringLiteral("/first/01.png"));
    const QUrl staleSibling = localUrl(QStringLiteral("/first/02.mp4"));
    const QUrl secondImage = localUrl(QStringLiteral("/second/01.png"));

    session->setSourceUrl(firstImage);
    QCOMPARE(directMediaNavigationProvider.loadCount(), std::size_t(1));
    QCOMPARE(
        directMediaNavigationProvider.loadAt(0).parentUrl, localUrl(QStringLiteral("/first/")));

    session->setSourceUrl(secondImage);
    QCOMPARE(directMediaNavigationProvider.loadCount(), std::size_t(2));
    QCOMPARE(
        directMediaNavigationProvider.loadAt(1).parentUrl, localUrl(QStringLiteral("/second/")));
    QVERIFY(directMediaNavigationProvider.loadAt(0).canceled);
    QVERIFY(!session->activeNavigationKnown());

    directMediaNavigationProvider.deliverIgnoringCancellation(0,
        { directMediaNavigationCandidate(firstImage),
            directMediaNavigationCandidate(staleSibling) });

    QVERIFY(!session->activeNavigationKnown());
    QCOMPARE(session->activeNavigationCount(), 0);

    directMediaNavigationProvider.finishLoad(1, { directMediaNavigationCandidate(secondImage) });

    QVERIFY(session->activeNavigationKnown());
    QCOMPARE(session->activeNavigationCurrentNumber(), 1);
    QCOMPARE(session->activeNavigationCount(), 1);
}

void TestKiriDocumentSession::freshDirectImageFailureLeavesNavigationUnknown()
{
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    KiriView::TestSupport::ManualImageDataLoader imageDataLoader;
    const QUrl imageUrl = localUrl(QStringLiteral("/media/01.png"));
    const QUrl siblingUrl = localUrl(QStringLiteral("/media/02.mp4"));
    directMediaNavigationProvider.setMedia(localUrl(QStringLiteral("/media/")),
        { directMediaNavigationCandidate(imageUrl), directMediaNavigationCandidate(siblingUrl) });
    std::unique_ptr<KiriDocumentSession> session
        = createSession(directMediaNavigationProvider, nullptr, &imageDataLoader);

    session->setSourceUrl(imageUrl);
    QCOMPARE(imageDataLoader.loadCount(), std::size_t(1));
    QVERIFY(session->activeNavigationKnown());
    QCOMPARE(session->activeNavigationCurrentNumber(), 1);
    QCOMPARE(session->activeNavigationCount(), 2);

    imageDataLoader.failBackLoad(QStringLiteral("initial load failed"));

    QTRY_COMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Error);
    QVERIFY(!session->activeNavigationKnown());
    QCOMPARE(session->activeNavigationCurrentNumber(), 0);
    QCOMPARE(session->activeNavigationCount(), 0);
    QVERIFY(!session->canOpenPreviousActiveNavigation());
    QVERIFY(!session->canOpenNextActiveNavigation());
}

void TestKiriDocumentSession::archiveImageDocumentProjectsActiveNavigationFromPages()
{
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    KiriView::TestSupport::FakeImageDocumentPageCandidateProvider imageDocumentPageCandidates;
    KiriView::TestSupport::ManualImageDataLoader dataLoader;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollection
        = KiriView::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl firstPage = KiriView::TestSupport::archivePageUrl(
        archiveCollection->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondPage = KiriView::TestSupport::archivePageUrl(
        archiveCollection->rootUrl(), QStringLiteral("02.png"));
    imageDocumentPageCandidates.setOpenedCollectionCandidates(archiveCollection->rootUrl(),
        { KiriView::TestSupport::imageDocumentPageCandidate(firstPage),
            KiriView::TestSupport::imageDocumentPageCandidate(secondPage) });
    std::unique_ptr<KiriDocumentSession> session
        = createSessionWithProvider(directMediaNavigationProvider.provider(), nullptr, &dataLoader,
            imageDocumentPageCandidates.provider());

    session->setSourceUrl(archiveUrl);
    QTRY_COMPARE(dataLoader.loadCount(), std::size_t(1));
    dataLoader.finishBackLoad(QByteArrayLiteral("first"));

    QTRY_COMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Image);
    QTRY_COMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Ready);
    QCOMPARE(session->activeNavigationBoundaryScope(),
        KiriDocumentSession::ActiveNavigationBoundaryScope::ImageDocumentPageNavigationBoundary);
    QVERIFY(session->activeNavigationAvailable());
    QTRY_COMPARE(session->imageDocument()->currentPageNumber(), 1);
    QTRY_COMPARE(session->imageDocument()->currentLastPageNumber(), 1);
    QTRY_COMPARE(session->imageDocument()->pageCount(), 2);
    QTRY_VERIFY(session->activeNavigationKnown());
    QVERIFY(session->activeNavigationEditable());
    QCOMPARE(session->activeNavigationCurrentNumber(), 1);
    QCOMPARE(session->activeNavigationCount(), 2);
    QCOMPARE(session->windowTitleSubject(), QStringLiteral("book.cbz – 1/2"));
    QVERIFY(!session->canOpenPreviousActiveNavigation());
    QVERIFY(session->canOpenNextActiveNavigation());
    QVERIFY(session->atKnownFirstActiveNavigation());
    QVERIFY(!session->atKnownLastActiveNavigation());
}

void TestKiriDocumentSession::imageDocumentPageNavigationChangesEmitActiveNavigationWhenRelevant()
{
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    KiriView::TestSupport::FakeImageDocumentPageCandidateProvider imageDocumentPageCandidates;
    KiriView::TestSupport::ManualImageDataLoader dataLoader;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/signals.cbz"));
    const std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollection
        = KiriView::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl firstPage = KiriView::TestSupport::archivePageUrl(
        archiveCollection->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondPage = KiriView::TestSupport::archivePageUrl(
        archiveCollection->rootUrl(), QStringLiteral("02.png"));
    imageDocumentPageCandidates.setOpenedCollectionCandidates(archiveCollection->rootUrl(),
        { KiriView::TestSupport::imageDocumentPageCandidate(firstPage),
            KiriView::TestSupport::imageDocumentPageCandidate(secondPage) });
    std::unique_ptr<KiriDocumentSession> session
        = createSessionWithProvider(directMediaNavigationProvider.provider(), nullptr, &dataLoader,
            imageDocumentPageCandidates.provider());
    QSignalSpy activeNavigationSpy(session.get(), &KiriDocumentSession::activeNavigationChanged);

    session->setSourceUrl(archiveUrl);
    QTRY_COMPARE(dataLoader.loadCount(), std::size_t(1));
    dataLoader.finishBackLoad(QByteArrayLiteral("first"));
    QTRY_COMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Ready);
    QTRY_COMPARE(session->activeNavigationCount(), 2);
    const int baselineSignalCount = activeNavigationSpy.count();

    session->imageDocument()->openImageAtPage(2);

    QTRY_COMPARE(session->activeNavigationCurrentNumber(), 2);
    QVERIFY(activeNavigationSpy.count() > baselineSignalCount);
    QVERIFY(!session->canOpenNextActiveNavigation());
    QVERIFY(session->atKnownLastActiveNavigation());
}

void TestKiriDocumentSession::activeNavigationNumberDispatchRoutesDirectMedia()
{
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    const QUrl imageUrl = localUrl(QStringLiteral("/media/01.png"));
    const QUrl videoUrl = localUrl(QStringLiteral("/media/02.mp4"));
    directMediaNavigationProvider.setMedia(localUrl(QStringLiteral("/media/")),
        { directMediaNavigationCandidate(imageUrl), directMediaNavigationCandidate(videoUrl) });
    std::unique_ptr<KiriDocumentSession> session = createSession(directMediaNavigationProvider);

    session->setSourceUrl(videoUrl);

    QCOMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Video);
    QVERIFY(session->activeNavigationKnown());
    QCOMPARE(session->activeNavigationCurrentNumber(), 2);

    session->openActiveNavigationAtNumber(1);

    QCOMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Image);
    QCOMPARE(session->sourceUrl(), imageUrl);
    QCOMPARE(session->imageDocument()->sourceUrl(), imageUrl);
}

void TestKiriDocumentSession::activeNavigationNumberDispatchRoutesImageDocumentPages()
{
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    KiriView::TestSupport::FakeImageDocumentPageCandidateProvider imageDocumentPageCandidates;
    KiriView::TestSupport::ManualImageDataLoader dataLoader;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/number-dispatch.cbz"));
    const std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollection
        = KiriView::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl firstPage = KiriView::TestSupport::archivePageUrl(
        archiveCollection->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondPage = KiriView::TestSupport::archivePageUrl(
        archiveCollection->rootUrl(), QStringLiteral("02.png"));
    imageDocumentPageCandidates.setOpenedCollectionCandidates(archiveCollection->rootUrl(),
        { KiriView::TestSupport::imageDocumentPageCandidate(firstPage),
            KiriView::TestSupport::imageDocumentPageCandidate(secondPage) });
    std::unique_ptr<KiriDocumentSession> session
        = createSessionWithProvider(directMediaNavigationProvider.provider(), nullptr, &dataLoader,
            imageDocumentPageCandidates.provider());

    session->setSourceUrl(archiveUrl);
    QTRY_COMPARE(dataLoader.loadCount(), std::size_t(1));
    dataLoader.finishBackLoad(QByteArrayLiteral("first"));
    QTRY_COMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Ready);
    QCOMPARE(session->activeNavigationCurrentNumber(), 1);
    QCOMPARE(session->activeNavigationCount(), 2);

    session->openActiveNavigationAtNumber(2);

    QTRY_COMPARE(dataLoader.backLoad().url, secondPage);
    dataLoader.finishBackLoad(QByteArrayLiteral("second"));
    QTRY_COMPARE(session->imageDocument()->currentPageNumber(), 2);
    QCOMPARE(session->activeNavigationCurrentNumber(), 2);
}

void TestKiriDocumentSession::archiveCollectionThumbnailModelUsesPageCandidateNames()
{
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    KiriView::TestSupport::FakeImageDocumentPageCandidateProvider imageDocumentPageCandidates;
    KiriView::TestSupport::ManualImageDataLoader dataLoader;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/thumbnails.cbz"));
    const std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollection
        = KiriView::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl firstPage = KiriView::TestSupport::archivePageUrl(
        archiveCollection->rootUrl(), QStringLiteral("chapter/01.png"));
    const QUrl secondPage = KiriView::TestSupport::archivePageUrl(
        archiveCollection->rootUrl(), QStringLiteral("extras/clip.mp4"));
    imageDocumentPageCandidates.setOpenedCollectionCandidates(archiveCollection->rootUrl(),
        {
            KiriView::ImageDocumentPageCandidate { firstPage, QStringLiteral("chapter/01.png"),
                KiriView::ImageDocumentPageKind::Image },
            KiriView::ImageDocumentPageCandidate { secondPage, QStringLiteral("extras/clip.mp4"),
                KiriView::ImageDocumentPageKind::Video },
        });
    std::unique_ptr<KiriDocumentSession> session
        = createSessionWithProvider(directMediaNavigationProvider.provider(), nullptr, &dataLoader,
            imageDocumentPageCandidates.provider());

    session->setSourceUrl(archiveUrl);
    QTRY_COMPARE(dataLoader.loadCount(), std::size_t(1));
    dataLoader.finishBackLoad(QByteArrayLiteral("first"));

    QAbstractItemModel *model = session->activeNavigationThumbnailModel();
    QVERIFY(model != nullptr);
    QTRY_COMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Ready);
    QCOMPARE(session->activeNavigationCurrentNumber(), 1);
    QCOMPARE(session->activeNavigationCount(), 2);
    QCOMPARE(model->rowCount(), 2);
    compareThumbnailRow(*session, 0, 1, firstPage, QStringLiteral("chapter/01.png"),
        QStringLiteral("image-x-generic-symbolic"), true);
    compareThumbnailRow(*session, 1, 2, secondPage, QStringLiteral("extras/clip.mp4"),
        QStringLiteral("video-x-generic-symbolic"), false);
}

void TestKiriDocumentSession::activeNavigationRequestReportsDispatchAndBoundaryResults()
{
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    KiriView::TestSupport::FakeImageDocumentPageCandidateProvider imageDocumentPageCandidates;
    KiriView::TestSupport::ManualImageDataLoader dataLoader;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/request-results.cbz"));
    const std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollection
        = KiriView::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl firstPage = KiriView::TestSupport::archivePageUrl(
        archiveCollection->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondPage = KiriView::TestSupport::archivePageUrl(
        archiveCollection->rootUrl(), QStringLiteral("02.png"));
    imageDocumentPageCandidates.setOpenedCollectionCandidates(archiveCollection->rootUrl(),
        { KiriView::TestSupport::imageDocumentPageCandidate(firstPage),
            KiriView::TestSupport::imageDocumentPageCandidate(secondPage) });
    std::unique_ptr<KiriDocumentSession> session
        = createSessionWithProvider(directMediaNavigationProvider.provider(), nullptr, &dataLoader,
            imageDocumentPageCandidates.provider());

    session->setSourceUrl(archiveUrl);
    QTRY_COMPARE(dataLoader.loadCount(), std::size_t(1));
    dataLoader.finishBackLoad(QByteArrayLiteral("first"));
    QTRY_COMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Ready);
    QVERIFY(session->activeNavigationKnown());
    QVERIFY(session->atKnownFirstActiveNavigation());

    QCOMPARE(session->requestPreviousActiveNavigation(),
        KiriDocumentSession::ActiveNavigationRequestResult::FirstActiveNavigationBoundary);

    QCOMPARE(session->requestNextActiveNavigation(),
        KiriDocumentSession::ActiveNavigationRequestResult::ActiveNavigationRequestDispatched);
    QTRY_COMPARE(dataLoader.backLoad().url, secondPage);
    dataLoader.finishBackLoad(QByteArrayLiteral("second"));
    QTRY_COMPARE(session->activeNavigationCurrentNumber(), 2);
    QVERIFY(session->atKnownLastActiveNavigation());

    QCOMPARE(session->requestNextActiveNavigation(),
        KiriDocumentSession::ActiveNavigationRequestResult::LastActiveNavigationBoundary);
}

void TestKiriDocumentSession::activeNavigationAdjacentRequestsSetRevealIntent()
{
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    const QUrl first = localUrl(QStringLiteral("/media/01.mp4"));
    const QUrl second = localUrl(QStringLiteral("/media/02.mp4"));
    directMediaNavigationProvider.setMedia(localUrl(QStringLiteral("/media/")),
        { directMediaNavigationCandidate(first), directMediaNavigationCandidate(second) });
    std::unique_ptr<KiriDocumentSession> session = createSession(directMediaNavigationProvider);

    session->setSourceUrl(first);
    QVERIFY(session->activeNavigationKnown());
    QCOMPARE(session->activeNavigationRevealIntent(),
        KiriDocumentSession::ActiveNavigationRevealIntent::LoadOrOpen);
    QCOMPARE(session->activeNavigationRevealDirection(),
        KiriDocumentSession::ActiveNavigationRevealDirection::None);

    session->requestNextActiveNavigation();

    QCOMPARE(session->sourceUrl(), second);
    QCOMPARE(session->activeNavigationCurrentNumber(), 2);
    QCOMPARE(session->activeNavigationRevealIntent(),
        KiriDocumentSession::ActiveNavigationRevealIntent::AdjacentNavigation);
    QCOMPARE(session->activeNavigationRevealDirection(),
        KiriDocumentSession::ActiveNavigationRevealDirection::Next);

    session->openPreviousActiveNavigation();

    QCOMPARE(session->sourceUrl(), first);
    QCOMPARE(session->activeNavigationCurrentNumber(), 1);
    QCOMPARE(session->activeNavigationRevealIntent(),
        KiriDocumentSession::ActiveNavigationRevealIntent::AdjacentNavigation);
    QCOMPARE(session->activeNavigationRevealDirection(),
        KiriDocumentSession::ActiveNavigationRevealDirection::Previous);

    QCOMPARE(session->requestPreviousActiveNavigation(),
        KiriDocumentSession::ActiveNavigationRequestResult::FirstActiveNavigationBoundary);
    QCOMPARE(session->activeNavigationRevealIntent(),
        KiriDocumentSession::ActiveNavigationRevealIntent::None);
    QCOMPARE(session->activeNavigationRevealDirection(),
        KiriDocumentSession::ActiveNavigationRevealDirection::None);
}

void TestKiriDocumentSession::activeNavigationLargeJumpRequestsSetRevealIntent()
{
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    const QUrl first = localUrl(QStringLiteral("/media/01.mp4"));
    const QUrl second = localUrl(QStringLiteral("/media/02.mp4"));
    const QUrl third = localUrl(QStringLiteral("/media/03.mp4"));
    directMediaNavigationProvider.setMedia(localUrl(QStringLiteral("/media/")),
        { directMediaNavigationCandidate(first), directMediaNavigationCandidate(second),
            directMediaNavigationCandidate(third) });
    std::unique_ptr<KiriDocumentSession> session = createSession(directMediaNavigationProvider);

    session->setSourceUrl(second);
    QVERIFY(session->activeNavigationKnown());

    session->openFirstActiveNavigation();

    QCOMPARE(session->sourceUrl(), first);
    QCOMPARE(session->activeNavigationRevealIntent(),
        KiriDocumentSession::ActiveNavigationRevealIntent::LargeJump);
    QCOMPARE(session->activeNavigationRevealDirection(),
        KiriDocumentSession::ActiveNavigationRevealDirection::None);

    session->openLastActiveNavigation();

    QCOMPARE(session->sourceUrl(), third);
    QCOMPARE(session->activeNavigationRevealIntent(),
        KiriDocumentSession::ActiveNavigationRevealIntent::LargeJump);
    QCOMPARE(session->activeNavigationRevealDirection(),
        KiriDocumentSession::ActiveNavigationRevealDirection::None);

    session->openActiveNavigationAtNumber(2);

    QCOMPARE(session->sourceUrl(), second);
    QCOMPARE(session->activeNavigationRevealIntent(),
        KiriDocumentSession::ActiveNavigationRevealIntent::LargeJump);
    QCOMPARE(session->activeNavigationRevealDirection(),
        KiriDocumentSession::ActiveNavigationRevealDirection::None);
}

void TestKiriDocumentSession::activeNavigationThumbnailDispatchSetsRevealIntent()
{
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    const QUrl first = localUrl(QStringLiteral("/media/01.mp4"));
    const QUrl second = localUrl(QStringLiteral("/media/02.mp4"));
    directMediaNavigationProvider.setMedia(localUrl(QStringLiteral("/media/")),
        { directMediaNavigationCandidate(first), directMediaNavigationCandidate(second) });
    std::unique_ptr<KiriDocumentSession> session = createSession(directMediaNavigationProvider);

    session->setSourceUrl(first);
    QVERIFY(session->activeNavigationKnown());

    session->openActiveNavigationThumbnailAtNumber(2);

    QCOMPARE(session->sourceUrl(), second);
    QCOMPARE(session->activeNavigationCurrentNumber(), 2);
    QCOMPARE(session->activeNavigationRevealIntent(),
        KiriDocumentSession::ActiveNavigationRevealIntent::ThumbnailActivation);
    QCOMPARE(session->activeNavigationRevealDirection(),
        KiriDocumentSession::ActiveNavigationRevealDirection::None);
}

void TestKiriDocumentSession::sourceOpenReplacesStaleRevealIntent()
{
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    const QUrl first = localUrl(QStringLiteral("/media/01.mp4"));
    const QUrl second = localUrl(QStringLiteral("/media/02.mp4"));
    const QUrl other = localUrl(QStringLiteral("/other/01.mp4"));
    directMediaNavigationProvider.setMedia(localUrl(QStringLiteral("/media/")),
        { directMediaNavigationCandidate(first), directMediaNavigationCandidate(second) });
    directMediaNavigationProvider.setMedia(
        localUrl(QStringLiteral("/other/")), { directMediaNavigationCandidate(other) });
    std::unique_ptr<KiriDocumentSession> session = createSession(directMediaNavigationProvider);

    session->setSourceUrl(first);
    QVERIFY(session->activeNavigationKnown());

    session->requestNextActiveNavigation();
    QCOMPARE(session->activeNavigationRevealIntent(),
        KiriDocumentSession::ActiveNavigationRevealIntent::AdjacentNavigation);
    QCOMPARE(session->activeNavigationRevealDirection(),
        KiriDocumentSession::ActiveNavigationRevealDirection::Next);

    session->setSourceUrl(other);

    QCOMPARE(session->sourceUrl(), other);
    QCOMPARE(session->activeNavigationCurrentNumber(), 1);
    QCOMPARE(session->activeNavigationRevealIntent(),
        KiriDocumentSession::ActiveNavigationRevealIntent::LoadOrOpen);
    QCOMPARE(session->activeNavigationRevealDirection(),
        KiriDocumentSession::ActiveNavigationRevealDirection::None);

    session->openActiveNavigationAtNumber(42);

    QCOMPARE(session->sourceUrl(), other);
    QCOMPARE(session->activeNavigationRevealIntent(),
        KiriDocumentSession::ActiveNavigationRevealIntent::None);
    QCOMPARE(session->activeNavigationRevealDirection(),
        KiriDocumentSession::ActiveNavigationRevealDirection::None);
}

void TestKiriDocumentSession::unavailableActiveNavigationRequestsClearRevealDirection()
{
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    std::unique_ptr<KiriDocumentSession> session = createSession(directMediaNavigationProvider);

    QCOMPARE(session->requestNextActiveNavigation(),
        KiriDocumentSession::ActiveNavigationRequestResult::NoActiveNavigationRequestResult);
    QCOMPARE(session->activeNavigationRevealIntent(),
        KiriDocumentSession::ActiveNavigationRevealIntent::None);
    QCOMPARE(session->activeNavigationRevealDirection(),
        KiriDocumentSession::ActiveNavigationRevealDirection::None);
}

void TestKiriDocumentSession::activeNavigationBoundaryTextFollowsSessionSource()
{
    KiriView::TestSupport::FakeImageDocumentPageCandidateProvider imageDocumentPageCandidates;
    KiriView::TestSupport::ManualImageDataLoader dataLoader;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/request-boundary-text.cbz"));
    const std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollection
        = KiriView::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl firstPage = KiriView::TestSupport::archivePageUrl(
        archiveCollection->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondPage = KiriView::TestSupport::archivePageUrl(
        archiveCollection->rootUrl(), QStringLiteral("02.png"));
    imageDocumentPageCandidates.setOpenedCollectionCandidates(archiveCollection->rootUrl(),
        { KiriView::TestSupport::imageDocumentPageCandidate(firstPage),
            KiriView::TestSupport::imageDocumentPageCandidate(secondPage) });
    FakeDirectMediaNavigationCandidateProvider unusedDirectMediaNavigationProvider;
    std::unique_ptr<KiriDocumentSession> imageSession
        = createSessionWithProvider(unusedDirectMediaNavigationProvider.provider(), nullptr,
            &dataLoader, imageDocumentPageCandidates.provider());

    imageSession->setSourceUrl(archiveUrl);
    QTRY_COMPARE(dataLoader.loadCount(), std::size_t(1));
    dataLoader.finishBackLoad(QByteArrayLiteral("first"));
    QTRY_COMPARE(imageSession->imageDocument()->status(), KiriImageDocument::Status::Ready);
    QVERIFY(imageSession->atKnownFirstActiveNavigation());

    QCOMPARE(
        imageSession->requestPreviousActiveNavigationBoundaryText(), QStringLiteral("First image"));
    QCOMPARE(imageSession->requestNextActiveNavigationBoundaryText(), QString());

    QTRY_COMPARE(dataLoader.backLoad().url, secondPage);
    dataLoader.finishBackLoad(QByteArrayLiteral("second"));
    QTRY_COMPARE(imageSession->activeNavigationCurrentNumber(), 2);
    QVERIFY(imageSession->atKnownLastActiveNavigation());

    QCOMPARE(imageSession->requestNextActiveNavigationBoundaryText(), QStringLiteral("Last image"));
}

void TestKiriDocumentSession::activeNavigationNumberDispatchIgnoresUnknownNavigation()
{
    ManualDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    const QUrl first = localUrl(QStringLiteral("/media/01.mp4"));
    const QUrl second = localUrl(QStringLiteral("/media/02.mp4"));
    std::unique_ptr<KiriDocumentSession> session
        = createSessionWithProvider(directMediaNavigationProvider.provider());

    session->setSourceUrl(first);

    QCOMPARE(directMediaNavigationProvider.loadCount(), std::size_t(1));
    QVERIFY(session->activeNavigationAvailable());
    QVERIFY(!session->activeNavigationKnown());
    QVERIFY(!session->activeNavigationEditable());
    QCOMPARE(session->activeNavigationCurrentNumber(), 0);
    QCOMPARE(session->activeNavigationCount(), 0);
    QVERIFY(!session->canOpenPreviousActiveNavigation());
    QVERIFY(!session->canOpenNextActiveNavigation());
    QCOMPARE(session->requestPreviousActiveNavigation(),
        KiriDocumentSession::ActiveNavigationRequestResult::NoActiveNavigationRequestResult);
    QCOMPARE(session->requestNextActiveNavigation(),
        KiriDocumentSession::ActiveNavigationRequestResult::NoActiveNavigationRequestResult);

    session->openActiveNavigationAtNumber(2);

    QCOMPARE(directMediaNavigationProvider.loadCount(), std::size_t(1));
    QCOMPARE(session->sourceUrl(), first);

    directMediaNavigationProvider.finishLoad(
        0, { directMediaNavigationCandidate(first), directMediaNavigationCandidate(second) });
    QVERIFY(session->activeNavigationKnown());
    session->openActiveNavigationAtNumber(2);
    QCOMPARE(directMediaNavigationProvider.loadCount(), std::size_t(2));
    directMediaNavigationProvider.finishLoad(
        1, { directMediaNavigationCandidate(first), directMediaNavigationCandidate(second) });
    QCOMPARE(session->sourceUrl(), second);
}

void TestKiriDocumentSession::activeNavigationClearsWhenSwitchingFromKnownDirectMedia()
{
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    KiriView::TestSupport::FakeImageDocumentPageCandidateProvider imageDocumentPageCandidates;
    KiriView::TestSupport::ManualImageDataLoader dataLoader;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/clear.cbz"));
    const std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollection
        = KiriView::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl page = KiriView::TestSupport::archivePageUrl(
        archiveCollection->rootUrl(), QStringLiteral("01.png"));
    imageDocumentPageCandidates.setOpenedCollectionCandidates(
        archiveCollection->rootUrl(), { KiriView::TestSupport::imageDocumentPageCandidate(page) });
    const QUrl clip = localUrl(QStringLiteral("/media/01.mp4"));
    const QUrl nextClip = localUrl(QStringLiteral("/media/02.mp4"));
    const QUrl lastClip = localUrl(QStringLiteral("/media/03.mp4"));
    directMediaNavigationProvider.setMedia(localUrl(QStringLiteral("/media/")),
        { directMediaNavigationCandidate(clip), directMediaNavigationCandidate(nextClip),
            directMediaNavigationCandidate(lastClip) });
    std::unique_ptr<KiriDocumentSession> session
        = createSessionWithProvider(directMediaNavigationProvider.provider(), nullptr, &dataLoader,
            imageDocumentPageCandidates.provider());

    session->setSourceUrl(nextClip);
    QVERIFY(session->activeNavigationKnown());
    QCOMPARE(session->activeNavigationCurrentNumber(), 2);
    QCOMPARE(session->activeNavigationCount(), 3);
    QCOMPARE(session->windowTitleSubject(), QStringLiteral("02.mp4"));

    session->setSourceUrl(archiveUrl);

    QVERIFY(session->activeNavigationCount() != 3);
    QTRY_COMPARE(dataLoader.loadCount(), std::size_t(1));
    dataLoader.finishBackLoad(QByteArrayLiteral("page"));
    QTRY_COMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Image);
    QTRY_COMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Ready);
    QCOMPARE(session->activeNavigationCurrentNumber(), 1);
    QCOMPARE(session->activeNavigationCount(), 1);
    QCOMPARE(session->windowTitleSubject(), QStringLiteral("clear.cbz – 1/1"));

    session->setSourceUrl(nextClip);
    QVERIFY(session->activeNavigationKnown());
    QCOMPARE(session->activeNavigationCurrentNumber(), 2);
    QCOMPARE(session->activeNavigationCount(), 3);

    session->setSourceUrl(QUrl());

    QCOMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Empty);
    QCOMPARE(session->windowTitleSubject(), QString());
    compareUnavailableActiveNavigation(*session);
}

void TestKiriDocumentSession::activeNavigationAvailabilityUsesSameSnapshotAsCurrentAndCount()
{
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    const QUrl first = localUrl(QStringLiteral("/media/01.mp4"));
    const QUrl second = localUrl(QStringLiteral("/media/02.mp4"));
    const QUrl third = localUrl(QStringLiteral("/media/03.mp4"));
    directMediaNavigationProvider.setMedia(localUrl(QStringLiteral("/media/")),
        { directMediaNavigationCandidate(first), directMediaNavigationCandidate(second),
            directMediaNavigationCandidate(third) });
    std::unique_ptr<KiriDocumentSession> session = createSession(directMediaNavigationProvider);

    session->setSourceUrl(second);

    QVERIFY(session->activeNavigationKnown());
    QCOMPARE(session->activeNavigationCurrentNumber(), 2);
    QCOMPARE(session->activeNavigationCount(), 3);
    QVERIFY(session->canOpenPreviousActiveNavigation());
    QVERIFY(session->canOpenNextActiveNavigation());
    QVERIFY(!session->atKnownFirstActiveNavigation());
    QVERIFY(!session->atKnownLastActiveNavigation());

    session->openLastActiveNavigation();

    QCOMPARE(session->sourceUrl(), third);
    QCOMPARE(session->activeNavigationCurrentNumber(), 3);
    QCOMPARE(session->activeNavigationCount(), 3);
    QVERIFY(session->canOpenPreviousActiveNavigation());
    QVERIFY(!session->canOpenNextActiveNavigation());
    QVERIFY(!session->atKnownFirstActiveNavigation());
    QVERIFY(session->atKnownLastActiveNavigation());

    session->openFirstActiveNavigation();

    QCOMPARE(session->sourceUrl(), first);
    QCOMPARE(session->activeNavigationCurrentNumber(), 1);
    QCOMPARE(session->activeNavigationCount(), 3);
    QVERIFY(!session->canOpenPreviousActiveNavigation());
    QVERIFY(session->canOpenNextActiveNavigation());
    QVERIFY(session->atKnownFirstActiveNavigation());
    QVERIFY(!session->atKnownLastActiveNavigation());
}

void TestKiriDocumentSession::activeNavigationBoundaryScopeFollowsSessionSource()
{
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    KiriView::TestSupport::FakeImageDocumentPageCandidateProvider imageDocumentPageCandidates;
    KiriView::TestSupport::ManualImageDataLoader dataLoader;
    const QUrl clip = localUrl(QStringLiteral("/media/01.mp4"));
    directMediaNavigationProvider.setMedia(
        localUrl(QStringLiteral("/media/")), { directMediaNavigationCandidate(clip) });
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/boundary.cbz"));
    const std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollection
        = KiriView::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl page = KiriView::TestSupport::archivePageUrl(
        archiveCollection->rootUrl(), QStringLiteral("01.png"));
    imageDocumentPageCandidates.setOpenedCollectionCandidates(
        archiveCollection->rootUrl(), { KiriView::TestSupport::imageDocumentPageCandidate(page) });
    std::unique_ptr<KiriDocumentSession> session
        = createSessionWithProvider(directMediaNavigationProvider.provider(), nullptr, &dataLoader,
            imageDocumentPageCandidates.provider());

    QCOMPARE(session->activeNavigationBoundaryScope(),
        KiriDocumentSession::ActiveNavigationBoundaryScope::NoNavigationBoundary);

    session->setSourceUrl(clip);

    QVERIFY(session->activeNavigationKnown());
    QCOMPARE(session->activeNavigationBoundaryScope(),
        KiriDocumentSession::ActiveNavigationBoundaryScope::DirectMediaNavigationBoundary);

    session->setSourceUrl(archiveUrl);
    QTRY_COMPARE(dataLoader.loadCount(), std::size_t(1));
    dataLoader.finishBackLoad(QByteArrayLiteral("page"));
    QTRY_COMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Image);
    QTRY_VERIFY(session->activeNavigationKnown());
    QCOMPARE(session->activeNavigationBoundaryScope(),
        KiriDocumentSession::ActiveNavigationBoundaryScope::ImageDocumentPageNavigationBoundary);
}

void TestKiriDocumentSession::openWithIsUnavailableInEmptySession()
{
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    FakeMediaOpenWithProvider openWithProvider;
    std::unique_ptr<KiriDocumentSession> session
        = createSessionWithProvider(directMediaNavigationProvider.provider(), nullptr, nullptr, {},
            KiriView::TestSupport::staticImageDataDecoder(), openWithProvider.provider());

    QVERIFY(!session->displayedMediaOpenWithAvailable());

    session->openCurrentMediaWith();

    QVERIFY(openWithProvider.requests.empty());
}

void TestKiriDocumentSession::openWithUsesCurrentDirectImageUrl()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString imagePath = directory.filePath(QStringLiteral("01.png"));
    QVERIFY(writeTestImage(imagePath));
    const QUrl imageUrl = localUrl(imagePath);

    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    directMediaNavigationProvider.setMedia(localUrl(directory.path() + QStringLiteral("/")),
        { directMediaNavigationCandidate(imageUrl) });
    FakeMediaOpenWithProvider openWithProvider;
    std::unique_ptr<KiriDocumentSession> session
        = createSessionWithProvider(directMediaNavigationProvider.provider(), nullptr, nullptr, {},
            KiriView::TestSupport::staticImageDataDecoder(), openWithProvider.provider());

    QSignalSpy availabilitySpy(
        session.get(), &KiriDocumentSession::displayedMediaOpenWithAvailabilityChanged);
    session->setSourceUrl(imageUrl);

    QTRY_COMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Ready);
    QVERIFY(session->displayedMediaOpenWithAvailable());
    QVERIFY(availabilitySpy.count() > 0);

    session->openCurrentMediaWith();

    QCOMPARE(openWithProvider.requests.size(), std::size_t(1));
    QCOMPARE(openWithProvider.requests.at(0).targetUrl, imageUrl);
}

void TestKiriDocumentSession::openWithFailureEmitsToastSignal()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString imagePath = directory.filePath(QStringLiteral("01.png"));
    QVERIFY(writeTestImage(imagePath));
    const QUrl imageUrl = localUrl(imagePath);

    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    directMediaNavigationProvider.setMedia(localUrl(directory.path() + QStringLiteral("/")),
        { directMediaNavigationCandidate(imageUrl) });
    FakeMediaOpenWithProvider openWithProvider;
    openWithProvider.result = KiriView::MediaOpenWithResult::Failed;
    openWithProvider.errorString = QStringLiteral("launcher failed");
    std::unique_ptr<KiriDocumentSession> session
        = createSessionWithProvider(directMediaNavigationProvider.provider(), nullptr, nullptr, {},
            KiriView::TestSupport::staticImageDataDecoder(), openWithProvider.provider());
    QSignalSpy failureSpy(session.get(), &KiriDocumentSession::openWithFailed);

    session->setSourceUrl(imageUrl);
    QTRY_COMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Ready);
    session->openCurrentMediaWith();

    QCOMPARE(failureSpy.count(), 1);
    QCOMPARE(failureSpy.at(0).at(0).toString(), QStringLiteral("launcher failed"));
}

void TestKiriDocumentSession::twoPageSpreadLastBoundaryProjectsThroughActiveNavigation()
{
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    KiriView::TestSupport::FakeImageDocumentPageCandidateProvider imageDocumentPageCandidates;
    KiriView::TestSupport::ManualImageDataLoader dataLoader;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollection
        = KiriView::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl firstPage = KiriView::TestSupport::archivePageUrl(
        archiveCollection->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondPage = KiriView::TestSupport::archivePageUrl(
        archiveCollection->rootUrl(), QStringLiteral("02.png"));
    const QUrl thirdPage = KiriView::TestSupport::archivePageUrl(
        archiveCollection->rootUrl(), QStringLiteral("03.png"));
    imageDocumentPageCandidates.setOpenedCollectionCandidates(archiveCollection->rootUrl(),
        { KiriView::TestSupport::imageDocumentPageCandidate(firstPage),
            KiriView::TestSupport::imageDocumentPageCandidate(secondPage),
            KiriView::TestSupport::imageDocumentPageCandidate(thirdPage) });
    std::unique_ptr<KiriDocumentSession> session
        = createSessionWithProvider(directMediaNavigationProvider.provider(), nullptr, &dataLoader,
            imageDocumentPageCandidates.provider(),
            KiriView::TestSupport::staticImageDataDecoder(
                KiriView::TestSupport::testImage(QSize(100, 200))));
    session->imageDocument()->setViewportSize(QSizeF(400.0, 300.0));

    session->setSourceUrl(archiveUrl);
    QTRY_COMPARE(dataLoader.loadCount(), std::size_t(1));
    dataLoader.finishBackLoad(QByteArrayLiteral("first"));
    QTRY_COMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Ready);

    session->imageDocument()->setTwoPageModeEnabled(true);
    session->imageDocument()->openNextPage();
    QTRY_COMPARE(dataLoader.backLoad().url, secondPage);
    dataLoader.finishBackLoad(QByteArrayLiteral("second"));
    QTRY_COMPARE(dataLoader.backLoad().url, thirdPage);
    dataLoader.finishBackLoad(QByteArrayLiteral("third"));

    QTRY_VERIFY(session->imageDocument()->secondaryPageVisible());
    QCOMPARE(session->imageDocument()->currentPageNumber(), 2);
    QCOMPARE(session->imageDocument()->currentLastPageNumber(), 3);
    QCOMPARE(session->activeNavigationCurrentNumber(), 2);
    QCOMPARE(session->activeNavigationCount(), 3);
    QVERIFY(session->activeNavigationKnown());
    QVERIFY(session->canOpenPreviousActiveNavigation());
    QVERIFY(!session->canOpenNextActiveNavigation());
    QVERIFY(!session->atKnownFirstActiveNavigation());
    QVERIFY(session->atKnownLastActiveNavigation());
}

void TestKiriDocumentSession::videoNavigationKeepsStillImagePredecodeCache()
{
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    KiriView::TestSupport::ManualImageDataLoader imageDataLoader;
    const QUrl firstImage = localUrl(QStringLiteral("/media/00.png"));
    const QUrl video = localUrl(QStringLiteral("/media/01.mp4"));
    const QUrl nextImage = localUrl(QStringLiteral("/media/02.png"));
    directMediaNavigationProvider.setMedia(localUrl(QStringLiteral("/media/")),
        { directMediaNavigationCandidate(firstImage), directMediaNavigationCandidate(video),
            directMediaNavigationCandidate(nextImage) });
    std::unique_ptr<KiriDocumentSession> session
        = createSession(directMediaNavigationProvider, nullptr, &imageDataLoader);

    session->setSourceUrl(firstImage);
    QTRY_COMPARE(imageDataLoader.loadCount(), std::size_t(1));
    QCOMPARE(imageDataLoader.frontLoad().url, firstImage);
    imageDataLoader.finishFrontLoad(QByteArrayLiteral("first"));
    QTRY_COMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Image);
    QTRY_COMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Ready);

    QTRY_COMPARE(imageDataLoader.loadCount(), std::size_t(2));
    QCOMPARE(imageDataLoader.backLoad().url, nextImage);
    imageDataLoader.finishBackLoad(QByteArrayLiteral("next"));

    session->openNextActiveNavigation();
    QCOMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Video);
    QCOMPARE(session->sourceUrl(), video);

    const std::size_t loadCountBeforeReturn = imageDataLoader.loadCount();
    session->openPreviousActiveNavigation();

    QCOMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Image);
    QCOMPARE(session->sourceUrl(), firstImage);
    QCOMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Ready);
    QCOMPARE(imageDataLoader.loadCount(), loadCountBeforeReturn);
}

void TestKiriDocumentSession::videoActiveNavigationExposesCurrentNumberAndCount()
{
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    const QUrl imageUrl = localUrl(QStringLiteral("/media/01.png"));
    const QUrl videoUrl = localUrl(QStringLiteral("/media/02.mp4"));
    directMediaNavigationProvider.setMedia(localUrl(QStringLiteral("/media/")),
        { directMediaNavigationCandidate(imageUrl), directMediaNavigationCandidate(videoUrl) });
    std::unique_ptr<KiriDocumentSession> session = createSession(directMediaNavigationProvider);

    session->setSourceUrl(videoUrl);

    QCOMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Video);
    QVERIFY(session->activeNavigationAvailable());
    QVERIFY(session->activeNavigationKnown());
    QCOMPARE(session->activeNavigationCurrentNumber(), 2);
    QCOMPARE(session->activeNavigationCount(), 2);
    QVERIFY(session->canOpenPreviousActiveNavigation());
    QVERIFY(!session->canOpenNextActiveNavigation());

    session->openActiveNavigationAtNumber(1);

    QCOMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Image);
    QCOMPARE(session->sourceUrl(), imageUrl);
    QCOMPARE(session->imageDocument()->sourceUrl(), imageUrl);
    QCOMPARE(session->videoDocument()->sourceUrl(), QUrl());
}

void TestKiriDocumentSession::initialDirectImagePredecodeUsesRequestedMediaCursor()
{
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    KiriView::TestSupport::ManualImageDataLoader imageDataLoader;
    const QUrl firstImage = localUrl(QStringLiteral("/media/01.png"));
    const QUrl video = localUrl(QStringLiteral("/media/02.mp4"));
    const QUrl nextImage = localUrl(QStringLiteral("/media/03.png"));
    directMediaNavigationProvider.setMedia(localUrl(QStringLiteral("/media/")),
        { directMediaNavigationCandidate(firstImage), directMediaNavigationCandidate(video),
            directMediaNavigationCandidate(nextImage) });
    std::unique_ptr<KiriDocumentSession> session
        = createSession(directMediaNavigationProvider, nullptr, &imageDataLoader);

    session->setSourceUrl(firstImage);

    QCOMPARE(imageDataLoader.loadCount(), std::size_t(1));
    QCOMPARE(imageDataLoader.frontLoad().url, firstImage);
    QCOMPARE(session->imageDocument()->displayedUrl(), QUrl());
    QVERIFY(session->activeNavigationKnown());
    QCOMPARE(session->activeNavigationCurrentNumber(), 1);
    QTRY_COMPARE(imageDataLoader.loadCount(), std::size_t(2));
    QCOMPARE(imageDataLoader.backLoad().url, nextImage);
}

void TestKiriDocumentSession::directImagePredecodeUsesSessionDependencyOverrides()
{
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    KiriView::TestSupport::ManualImageDataLoader imageDataLoader;
    KiriView::TestSupport::ManualImageDataLoader directMediaPredecodeDataLoader;
    const QUrl currentImage = localUrl(QStringLiteral("/media/current.png"));
    const QUrl nextImage = localUrl(QStringLiteral("/media/next.png"));
    directMediaNavigationProvider.setMedia(localUrl(QStringLiteral("/media/")),
        { directMediaNavigationCandidate(currentImage),
            directMediaNavigationCandidate(nextImage) });

    KiriView::KiriDocumentSessionDependencies dependencies;
    dependencies.sessionRuntime.directMediaNavigationCandidateProvider
        = directMediaNavigationProvider.provider();
    dependencies.imageDocument.imageDecode = KiriView::TestSupport::imageDecodeDependenciesFor(
        imageDataLoader, KiriView::TestSupport::staticImageDataDecoder());
    dependencies.sessionRuntime.directMediaPredecodeDependencies.imageDecode
        = KiriView::TestSupport::imageDecodeDependenciesFor(
            directMediaPredecodeDataLoader, KiriView::TestSupport::staticImageDataDecoder());
    std::unique_ptr<KiriDocumentSession> session
        = std::make_unique<KiriDocumentSession>(std::move(dependencies));

    session->setSourceUrl(currentImage);

    QCOMPARE(imageDataLoader.loadCount(), std::size_t(1));
    QCOMPARE(imageDataLoader.frontLoad().url, currentImage);
    QTRY_COMPARE(directMediaPredecodeDataLoader.loadCount(), std::size_t(1));
    QCOMPARE(directMediaPredecodeDataLoader.frontLoad().url, nextImage);
    QCOMPARE(imageDataLoader.loadCount(), std::size_t(1));
}

void TestKiriDocumentSession::directImagePredecodeDoesNotUseImageDocumentPageCandidates()
{
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    KiriView::TestSupport::FakeImageDocumentPageCandidateProvider imageDocumentPageCandidates;
    KiriView::TestSupport::ManualImageDataLoader imageDataLoader;
    const QUrl currentImage = localUrl(QStringLiteral("/media/current.png"));
    const QUrl directMediaNextImage = localUrl(QStringLiteral("/media/next.png"));
    const QUrl imageDocumentOnlyImage = localUrl(QStringLiteral("/media/page-only.png"));
    directMediaNavigationProvider.setMedia(localUrl(QStringLiteral("/media/")),
        { directMediaNavigationCandidate(currentImage),
            directMediaNavigationCandidate(directMediaNextImage) });
    imageDocumentPageCandidates.setDirectoryImages(localUrl(QStringLiteral("/media/")),
        { KiriView::TestSupport::imageDocumentPageCandidate(currentImage),
            KiriView::TestSupport::imageDocumentPageCandidate(imageDocumentOnlyImage) });
    std::unique_ptr<KiriDocumentSession> session
        = createSessionWithProvider(directMediaNavigationProvider.provider(), nullptr,
            &imageDataLoader, imageDocumentPageCandidates.provider());

    session->setSourceUrl(currentImage);

    QCOMPARE(imageDataLoader.loadCount(), std::size_t(1));
    QCOMPARE(imageDataLoader.frontLoad().url, currentImage);
    QTRY_COMPARE(imageDataLoader.loadCount(), std::size_t(2));
    QCOMPARE(imageDataLoader.backLoad().url, directMediaNextImage);
    QVERIFY(imageDataLoader.finishOldestActiveLoadForUrl(
        directMediaNextImage, QByteArrayLiteral("direct-next")));
    QVERIFY(
        imageDataLoader.finishOldestActiveLoadForUrl(currentImage, QByteArrayLiteral("current")));
    QTRY_COMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Ready);

    QTest::qWait(250);

    QCOMPARE(imageDataLoader.loadCount(), std::size_t(2));
}

void TestKiriDocumentSession::
    staleDirectMediaNavigationCandidateCompletionCannotPublishForNewSource()
{
    ManualDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    std::unique_ptr<KiriDocumentSession> session
        = createSessionWithProvider(directMediaNavigationProvider.provider());

    const QUrl firstClip = localUrl(QStringLiteral("/media/01.mp4"));
    const QUrl secondClip = localUrl(QStringLiteral("/media/02.mp4"));
    const QUrl secondSibling = localUrl(QStringLiteral("/media/03.mp4"));

    session->setSourceUrl(firstClip);
    QCOMPARE(directMediaNavigationProvider.loadCount(), std::size_t(1));
    QCOMPARE(
        directMediaNavigationProvider.loadAt(0).parentUrl, localUrl(QStringLiteral("/media/")));

    session->setSourceUrl(secondClip);
    QCOMPARE(directMediaNavigationProvider.loadCount(), std::size_t(2));
    QCOMPARE(
        directMediaNavigationProvider.loadAt(1).parentUrl, localUrl(QStringLiteral("/media/")));
    QVERIFY(directMediaNavigationProvider.loadAt(0).canceled);
    QVERIFY(!session->activeNavigationKnown());

    directMediaNavigationProvider.deliverIgnoringCancellation(0,
        { directMediaNavigationCandidate(secondClip),
            directMediaNavigationCandidate(secondSibling) });

    QVERIFY(!session->activeNavigationKnown());
    QCOMPARE(session->activeNavigationCurrentNumber(), 0);
    QCOMPARE(session->activeNavigationCount(), 0);

    directMediaNavigationProvider.finishLoad(1,
        { directMediaNavigationCandidate(firstClip), directMediaNavigationCandidate(secondClip) });

    QVERIFY(session->activeNavigationKnown());
    QCOMPARE(session->activeNavigationCurrentNumber(), 2);
    QCOMPARE(session->activeNavigationCount(), 2);
}

void TestKiriDocumentSession::
    nextMediaFromVideoCanRouteToImageWithoutUsingImageDocumentPageNavigation()
{
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    const QUrl clip = localUrl(QStringLiteral("/media/01.mp4"));
    const QUrl image = localUrl(QStringLiteral("/media/02.png"));
    directMediaNavigationProvider.setMedia(localUrl(QStringLiteral("/media/")),
        { directMediaNavigationCandidate(clip), directMediaNavigationCandidate(image) });
    std::unique_ptr<KiriDocumentSession> session = createSession(directMediaNavigationProvider);
    session->setSourceUrl(clip);

    QVERIFY(session->canOpenNextActiveNavigation());
    session->openNextActiveNavigation();

    QCOMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Image);
    QCOMPARE(session->sourceUrl(), image);
    QCOMPARE(session->imageDocument()->sourceUrl(), image);
    QCOMPARE(session->videoDocument()->sourceUrl(), QUrl());
}

void TestKiriDocumentSession::nonMediaImageDeletionProgressIsMirroredThroughSessionState()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString imagePath = directory.filePath(QStringLiteral("page.png"));
    QImage image(QSize(2, 2), QImage::Format_RGBA8888);
    image.fill(Qt::red);
    QVERIFY(image.save(imagePath, "PNG"));

    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    KiriView::TestSupport::ManualFileDeletionProvider fileDeletionProvider;
    const QUrl directoryUrl = localUrl(directory.path());
    std::unique_ptr<KiriDocumentSession> session
        = createSession(directMediaNavigationProvider, &fileDeletionProvider);
    QSignalSpy progressSpy(session.get(), &KiriDocumentSession::fileDeletionInProgressChanged);

    session->setSourceUrl(directoryUrl);
    QTRY_COMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Image);
    QTRY_COMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Ready);
    QCOMPARE(session->activeNavigationBoundaryScope(),
        KiriDocumentSession::ActiveNavigationBoundaryScope::ImageDocumentPageNavigationBoundary);

    session->deleteDisplayedFile(KiriDocumentSession::DeletionMode::MoveToTrash);

    QCOMPARE(fileDeletionProvider.operationCount(), std::size_t(1));
    QCOMPARE(fileDeletionProvider.backOperation().request.targetUrl, directoryUrl);
    QVERIFY(session->fileDeletionInProgress());
    QCOMPARE(progressSpy.count(), 1);

    fileDeletionProvider.finishBackOperation(KiriView::FileDeletionResult::Succeeded);

    QVERIFY(!session->fileDeletionInProgress());
    QCOMPARE(progressSpy.count(), 2);
}

void TestKiriDocumentSession::directMediaDeletionInProgressDisablesActiveNavigationDispatch()
{
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    KiriView::TestSupport::ManualFileDeletionProvider fileDeletionProvider;
    const QUrl firstClip = localUrl(QStringLiteral("/media/01.mp4"));
    const QUrl currentClip = localUrl(QStringLiteral("/media/02.mp4"));
    const QUrl lastClip = localUrl(QStringLiteral("/media/03.mp4"));
    directMediaNavigationProvider.setMedia(localUrl(QStringLiteral("/media/")),
        { directMediaNavigationCandidate(firstClip), directMediaNavigationCandidate(currentClip),
            directMediaNavigationCandidate(lastClip) });
    std::unique_ptr<KiriDocumentSession> session
        = createSession(directMediaNavigationProvider, &fileDeletionProvider);

    session->setSourceUrl(currentClip);

    QCOMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Video);
    QVERIFY(session->displayedFileDeletionAvailable());
    QVERIFY(session->activeNavigationAvailable());
    QVERIFY(session->activeNavigationKnown());
    QVERIFY(session->activeNavigationEditable());
    QCOMPARE(session->activeNavigationCurrentNumber(), 2);
    QCOMPARE(session->activeNavigationCount(), 3);
    QVERIFY(session->canOpenPreviousActiveNavigation());
    QVERIFY(session->canOpenNextActiveNavigation());

    session->deleteDisplayedFile(KiriDocumentSession::DeletionMode::MoveToTrash);

    QCOMPARE(fileDeletionProvider.operationCount(), std::size_t(1));
    QCOMPARE(fileDeletionProvider.backOperation().request.targetUrl, currentClip);
    QVERIFY(session->fileDeletionInProgress());
    QVERIFY(!session->displayedFileDeletionAvailable());
    QVERIFY(session->activeNavigationAvailable());
    QVERIFY(session->activeNavigationKnown());
    QVERIFY(!session->activeNavigationEditable());
    QCOMPARE(session->activeNavigationCurrentNumber(), 2);
    QCOMPARE(session->activeNavigationCount(), 3);
    QVERIFY(!session->canOpenPreviousActiveNavigation());
    QVERIFY(!session->canOpenNextActiveNavigation());
    QVERIFY(!session->atKnownFirstActiveNavigation());
    QVERIFY(!session->atKnownLastActiveNavigation());
    QCOMPARE(session->requestPreviousActiveNavigation(),
        KiriDocumentSession::ActiveNavigationRequestResult::NoActiveNavigationRequestResult);
    QCOMPARE(session->requestNextActiveNavigation(),
        KiriDocumentSession::ActiveNavigationRequestResult::NoActiveNavigationRequestResult);

    session->openPreviousActiveNavigation();
    session->openNextActiveNavigation();
    session->openFirstActiveNavigation();
    session->openLastActiveNavigation();
    session->openActiveNavigationAtNumber(1);
    session->deleteDisplayedFile(KiriDocumentSession::DeletionMode::MoveToTrash);

    QCOMPARE(session->sourceUrl(), currentClip);
    QCOMPARE(session->videoDocument()->sourceUrl(), currentClip);
    QCOMPARE(fileDeletionProvider.operationCount(), std::size_t(1));

    fileDeletionProvider.finishBackOperation(KiriView::FileDeletionResult::Canceled);

    QVERIFY(!session->fileDeletionInProgress());
    QCOMPARE(session->sourceUrl(), currentClip);
}

void TestKiriDocumentSession::directImageDeletionCanOpenVideoFallback()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString imagePath = directory.filePath(QStringLiteral("01.png"));
    QImage image(QSize(2, 2), QImage::Format_RGBA8888);
    image.fill(Qt::red);
    QVERIFY(image.save(imagePath, "PNG"));

    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    KiriView::TestSupport::ManualFileDeletionProvider fileDeletionProvider;
    const QUrl imageUrl = localUrl(imagePath);
    const QUrl videoUrl = localUrl(directory.filePath(QStringLiteral("02.mp4")));
    directMediaNavigationProvider.setMedia(localUrl(directory.path() + QStringLiteral("/")),
        { directMediaNavigationCandidate(imageUrl), directMediaNavigationCandidate(videoUrl) });
    std::unique_ptr<KiriDocumentSession> session
        = createSession(directMediaNavigationProvider, &fileDeletionProvider);
    session->setSourceUrl(imageUrl);
    QTRY_COMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Image);
    QTRY_COMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Ready);
    QCOMPARE(session->activeNavigationBoundaryScope(),
        KiriDocumentSession::ActiveNavigationBoundaryScope::DirectMediaNavigationBoundary);

    session->deleteDisplayedFile(KiriDocumentSession::DeletionMode::MoveToTrash);

    QCOMPARE(fileDeletionProvider.operationCount(), std::size_t(1));
    QCOMPARE(fileDeletionProvider.backOperation().request.targetUrl, imageUrl);

    fileDeletionProvider.finishBackOperation(KiriView::FileDeletionResult::Succeeded);

    QCOMPARE(session->sourceUrl(), videoUrl);
    QCOMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Video);
    QCOMPARE(session->videoDocument()->sourceUrl(), videoUrl);
}

void TestKiriDocumentSession::pendingDirectImageReplacementDoesNotDeletePreviousDisplayedFile()
{
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    KiriView::TestSupport::ManualFileDeletionProvider fileDeletionProvider;
    KiriView::TestSupport::ManualImageDataLoader imageDataLoader;
    const QUrl firstImage = localUrl(QStringLiteral("/media/01.png"));
    const QUrl secondImage = localUrl(QStringLiteral("/media/02.png"));
    directMediaNavigationProvider.setMedia(localUrl(QStringLiteral("/media/")),
        { directMediaNavigationCandidate(firstImage),
            directMediaNavigationCandidate(secondImage) });
    std::unique_ptr<KiriDocumentSession> session
        = createSession(directMediaNavigationProvider, &fileDeletionProvider, &imageDataLoader);

    session->setSourceUrl(firstImage);
    QCOMPARE(imageDataLoader.loadCount(), std::size_t(1));
    imageDataLoader.finishBackLoad(QByteArrayLiteral("first"));
    QTRY_COMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Ready);
    QCOMPARE(session->imageDocument()->displayedUrl(), firstImage);

    session->setSourceUrl(secondImage);

    QCOMPARE(imageDataLoader.loadCount(), std::size_t(2));
    QCOMPARE(session->imageDocument()->displayedUrl(), firstImage);
    QVERIFY(!session->displayedFileDeletionAvailable());
    session->deleteDisplayedFile(KiriDocumentSession::DeletionMode::MoveToTrash);
    QCOMPARE(fileDeletionProvider.operationCount(), std::size_t(0));

    imageDataLoader.failBackLoad(QStringLiteral("replacement failed"));
    QTRY_COMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Ready);
    QCOMPARE(session->sourceUrl(), firstImage);
    QCOMPARE(session->imageDocument()->displayedUrl(), firstImage);
    QVERIFY(session->displayedFileDeletionAvailable());
    QCOMPARE(session->activeNavigationCurrentNumber(), 1);

    session->deleteDisplayedFile(KiriDocumentSession::DeletionMode::MoveToTrash);

    QCOMPARE(fileDeletionProvider.operationCount(), std::size_t(1));
    QCOMPARE(fileDeletionProvider.backOperation().request.targetUrl, firstImage);
}

void TestKiriDocumentSession::pendingDirectMediaDeletionCandidateLoadIsCanceledBySourceChange()
{
    ManualDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    KiriView::TestSupport::ManualFileDeletionProvider fileDeletionProvider;
    const QUrl firstClip = localUrl(QStringLiteral("/first/01.mp4"));
    const QUrl firstFallback = localUrl(QStringLiteral("/first/02.png"));
    const QUrl secondClip = localUrl(QStringLiteral("/second/01.mp4"));
    std::unique_ptr<KiriDocumentSession> session = createSessionWithProvider(
        directMediaNavigationProvider.provider(), &fileDeletionProvider);

    session->setSourceUrl(firstClip);
    QCOMPARE(directMediaNavigationProvider.loadCount(), std::size_t(1));
    directMediaNavigationProvider.finishLoad(0,
        { directMediaNavigationCandidate(firstClip),
            directMediaNavigationCandidate(firstFallback) });

    session->deleteDisplayedFile(KiriDocumentSession::DeletionMode::MoveToTrash);

    QCOMPARE(directMediaNavigationProvider.loadCount(), std::size_t(2));
    QVERIFY(session->fileDeletionInProgress());
    QCOMPARE(fileDeletionProvider.operationCount(), std::size_t(0));

    session->setSourceUrl(secondClip);

    QCOMPARE(session->sourceUrl(), secondClip);
    QCOMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Video);
    QVERIFY(!session->fileDeletionInProgress());
    QVERIFY(directMediaNavigationProvider.loadAt(1).canceled);

    directMediaNavigationProvider.deliverIgnoringCancellation(1,
        { directMediaNavigationCandidate(firstClip),
            directMediaNavigationCandidate(firstFallback) });

    QCOMPARE(fileDeletionProvider.operationCount(), std::size_t(0));
    QCOMPARE(session->sourceUrl(), secondClip);
    QVERIFY(!session->fileDeletionInProgress());
}

void TestKiriDocumentSession::videoDeletionUsesOriginalUrlAndOpensMediaFallback()
{
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    KiriView::TestSupport::ManualFileDeletionProvider fileDeletionProvider;
    const QUrl clip = QUrl(QStringLiteral("zip:///path/archive.zip!/chapter/01.mp4"));
    const QUrl fallback = QUrl(QStringLiteral("zip:///path/archive.zip!/chapter/02.jpg"));
    directMediaNavigationProvider.setMedia(
        QUrl(QStringLiteral("zip:///path/archive.zip!/chapter/")),
        { directMediaNavigationCandidate(clip), directMediaNavigationCandidate(fallback) });
    std::unique_ptr<KiriDocumentSession> session
        = createSession(directMediaNavigationProvider, &fileDeletionProvider);
    session->setSourceUrl(clip);

    session->deleteDisplayedFile(KiriDocumentSession::DeletionMode::MoveToTrash);

    QCOMPARE(fileDeletionProvider.operationCount(), std::size_t(1));
    QCOMPARE(fileDeletionProvider.backOperation().request.targetUrl, clip);
    QVERIFY(session->fileDeletionInProgress());

    fileDeletionProvider.finishBackOperation(KiriView::FileDeletionResult::Succeeded);

    QVERIFY(!session->fileDeletionInProgress());
    QCOMPARE(session->sourceUrl(), fallback);
    QCOMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Image);
}

void TestKiriDocumentSession::canceledVideoDeletionKeepsCurrentVideo()
{
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    KiriView::TestSupport::ManualFileDeletionProvider fileDeletionProvider;
    const QUrl clip = localUrl(QStringLiteral("/media/01.mov"));
    directMediaNavigationProvider.setMedia(
        localUrl(QStringLiteral("/media/")), { directMediaNavigationCandidate(clip) });
    std::unique_ptr<KiriDocumentSession> session
        = createSession(directMediaNavigationProvider, &fileDeletionProvider);
    session->setSourceUrl(clip);

    session->deleteDisplayedFile(KiriDocumentSession::DeletionMode::DeletePermanently);
    fileDeletionProvider.finishBackOperation(KiriView::FileDeletionResult::Canceled);

    QCOMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Video);
    QCOMPARE(session->sourceUrl(), clip);
    QCOMPARE(session->videoDocument()->sourceUrl(), clip);
    QCOMPARE(session->errorString(), QString());
}

void TestKiriDocumentSession::staleVideoDeletionCompletionAfterSourceChangeIsIgnored()
{
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    KiriView::TestSupport::ManualFileDeletionProvider fileDeletionProvider;
    const QUrl firstClip = localUrl(QStringLiteral("/first/01.mov"));
    const QUrl staleFallback = localUrl(QStringLiteral("/first/02.png"));
    const QUrl secondClip = localUrl(QStringLiteral("/second/01.mov"));
    directMediaNavigationProvider.setMedia(localUrl(QStringLiteral("/first/")),
        { directMediaNavigationCandidate(firstClip),
            directMediaNavigationCandidate(staleFallback) });
    directMediaNavigationProvider.setMedia(
        localUrl(QStringLiteral("/second/")), { directMediaNavigationCandidate(secondClip) });
    std::unique_ptr<KiriDocumentSession> session
        = createSession(directMediaNavigationProvider, &fileDeletionProvider);
    session->setSourceUrl(firstClip);

    session->deleteDisplayedFile(KiriDocumentSession::DeletionMode::MoveToTrash);

    QCOMPARE(fileDeletionProvider.operationCount(), std::size_t(1));
    QCOMPARE(fileDeletionProvider.operationAt(0).request.targetUrl, firstClip);
    QVERIFY(session->fileDeletionInProgress());

    session->setSourceUrl(secondClip);

    QCOMPARE(session->sourceUrl(), secondClip);
    QCOMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Video);
    QVERIFY(!session->fileDeletionInProgress());
    QVERIFY(fileDeletionProvider.operationAt(0).canceled);

    fileDeletionProvider.deliverOperationAtIgnoringCancellation(
        0, KiriView::FileDeletionResult::Succeeded);

    QCOMPARE(session->sourceUrl(), secondClip);
    QCOMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Video);
    QCOMPARE(session->videoDocument()->sourceUrl(), secondClip);
    QVERIFY(!session->fileDeletionInProgress());
    QCOMPARE(session->errorString(), QString());
}

QTEST_GUILESS_MAIN(TestKiriDocumentSession)

#include "test_kiridocumentsession.moc"
