// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "document/imagedocumentstate.h"
#include "document/imageopentransitionapplier.h"
#include "location/imagedocumentlocation.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <algorithm>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

namespace {
QUrl localUrl(const QString& path) { return QUrl::fromLocalFile(path); }

QUrl archivePageUrl(const QUrl& archiveRootUrl, const QString& path)
{
    QUrl url = archiveRootUrl;
    url.setPath(archiveRootUrl.path() + path);
    return url;
}

kiriview::ImageOpenStateDelta stateDelta(kiriview::ImageOpenUrlTarget sourceUrl,
    kiriview::ImageOpenDisplayedLocationTarget displayedLocation,
    kiriview::ImageOpenUrlTarget containerNavigationUrl, kiriview::ImageOpenBoolTarget loading,
    kiriview::ImageOpenStatusTarget status, kiriview::ImageOpenErrorStringTarget errorString,
    bool clearLoadingContainerNavigationUrl)
{
    return kiriview::ImageOpenStateDelta { sourceUrl,
        kiriview::ImageOpenSourceKindTarget::Unchanged, displayedLocation, containerNavigationUrl,
        loading, status, errorString, kiriview::ImageOpenBoolTarget::Unchanged,
        kiriview::ImageOpenEmbeddedMetadataTarget::Unchanged, clearLoadingContainerNavigationUrl };
}

template <typename Operation>
const Operation* findOperation(const kiriview::ImageDocumentRuntimePlan& plan)
{
    for (const kiriview::ImageDocumentRuntimeOperation& operation : plan) {
        if (const auto* payload = std::get_if<Operation>(&operation)) {
            return payload;
        }
    }
    return nullptr;
}

bool containsChange(
    const std::vector<kiriview::ImageDocumentChange>& changes, kiriview::ImageDocumentChange change)
{
    return std::find(changes.cbegin(), changes.cend(), change) != changes.cend();
}
}

class TestImageOpenTransitionApplier : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void applicationPlanResolvesRuntimeTargetsBeforeStateMutation();
    void successfulTransitionAppliesSessionStateAndEffects();
    void successfulTransitionPublishesEmbeddedMetadataFromContext();
    void errorTransitionUsesDisplayedFallbackAndProvidedError();
    void invalidReadyWithErrorPlanDoesNotMutateStateOrRunEffects();
    void invalidReadyWithEmptySourcePlanDoesNotMutateStateOrRunEffects();
    void invalidReadyVideoSourceWithoutUnsupportedFlagDoesNotMutateStateOrRunEffects();
    void invalidReadyWithUnrelatedContainerNavigationPlanDoesNotMutateStateOrRunEffects();
    void invalidNullWithContainerNavigationPlanDoesNotMutateStateOrRunEffects();
    void missingRuntimeContextDoesNotClearResolvedTargets();
};

void TestImageOpenTransitionApplier::applicationPlanResolvesRuntimeTargetsBeforeStateMutation()
{
    const QUrl containerUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(containerUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl imageUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("01.png"));
    const kiriview::DisplayedImageLocation location
        = kiriview::DisplayedImageLocation::fromOpenedCollectionScope(imageUrl, *archiveCollection);
    const kiriview::ImageLoadSession session {
        5,
        kiriview::ImageLoadRequest::fromUrl(imageUrl, containerUrl),
        location,
    };
    kiriview::ImageOpenTransition transition;
    transition.stateDelta = stateDelta(kiriview::ImageOpenUrlTarget::SessionImage,
        kiriview::ImageOpenDisplayedLocationTarget::Session,
        kiriview::ImageOpenUrlTarget::SessionContainerNavigation,
        kiriview::ImageOpenBoolTarget::False, kiriview::ImageOpenStatusTarget::Ready,
        kiriview::ImageOpenErrorStringTarget::Clear, true);
    transition.effects.push_back(kiriview::ImageOpenEffect::UpdatePageNavigation);

    const kiriview::ImageOpenApplicationPlan plan = kiriview::imageOpenApplicationPlan(
        transition, kiriview::ImageOpenTransitionContext::successfulImageLoad(session));

    QVERIFY(plan.stateDelta.sourceUrl.has_value());
    QCOMPARE(*plan.stateDelta.sourceUrl, imageUrl);
    QVERIFY(plan.stateDelta.displayedLocation.has_value());
    QCOMPARE(*plan.stateDelta.displayedLocation, location);
    QVERIFY(plan.stateDelta.containerNavigationUrl.has_value());
    QCOMPARE(*plan.stateDelta.containerNavigationUrl, containerUrl);
    QVERIFY(plan.stateDelta.loading.has_value());
    QCOMPARE(*plan.stateDelta.loading, false);
    QVERIFY(plan.stateDelta.status.has_value());
    QCOMPARE(*plan.stateDelta.status, kiriview::ImageDocumentStatus::Ready);
    QVERIFY(plan.stateDelta.errorString.has_value());
    QVERIFY(plan.stateDelta.errorString->isEmpty());
    QVERIFY(plan.stateDelta.clearLoadingContainerNavigationUrl);
    QVERIFY(findOperation<kiriview::UpdatePageNavigationOperation>(plan.runtimePlan) != nullptr);
}

void TestImageOpenTransitionApplier::successfulTransitionAppliesSessionStateAndEffects()
{
    std::vector<kiriview::ImageDocumentChange> changes;
    kiriview::ImageDocumentState state(
        [&changes](kiriview::ImageDocumentChange change) { changes.push_back(change); });

    state.setLoading(true);
    state.setErrorString(QStringLiteral("previous error"));
    changes.clear();

    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl imageUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("01.png"));
    const kiriview::DisplayedImageLocation location
        = kiriview::DisplayedImageLocation::fromOpenedCollectionScope(imageUrl, *archiveCollection);
    const kiriview::ImageLoadSession session {
        7,
        kiriview::ImageLoadRequest::fromUrl(archiveUrl),
        location,
    };
    kiriview::ImageOpenTransition transition;
    transition.stateDelta = stateDelta(kiriview::ImageOpenUrlTarget::SessionImage,
        kiriview::ImageOpenDisplayedLocationTarget::Session,
        kiriview::ImageOpenUrlTarget::DerivedContainerNavigation,
        kiriview::ImageOpenBoolTarget::False, kiriview::ImageOpenStatusTarget::Ready,
        kiriview::ImageOpenErrorStringTarget::Clear, true);
    transition.effects.push_back(kiriview::ImageOpenEffect::UpdatePageNavigation);
    transition.effects.push_back(kiriview::ImageOpenEffect::ScheduleAdjacentImagePredecode);

    const kiriview::ImageOpenApplicationPlan applicationPlan = kiriview::imageOpenApplicationPlan(
        transition, kiriview::ImageOpenTransitionContext::successfulImageLoad(session));
    const kiriview::ImageDocumentRuntimePlan plan
        = kiriview::applyImageOpenApplicationPlan(state, applicationPlan);

    QCOMPARE(state.sourceUrl(), imageUrl);
    QCOMPARE(state.displayedImageLocation(), location);
    QCOMPARE(state.containerNavigationUrl(), kiriview::containerNavigationUrlForLocation(location));
    QVERIFY(!state.loading());
    QCOMPARE(state.status(), kiriview::ImageDocumentStatus::Ready);
    QVERIFY(state.errorString().isEmpty());
    QVERIFY(findOperation<kiriview::UpdatePageNavigationOperation>(plan) != nullptr);
    QVERIFY(findOperation<kiriview::ScheduleAdjacentImagePredecodeOperation>(plan) != nullptr);
    QVERIFY(containsChange(changes, kiriview::ImageDocumentChange::SourceUrl));
    QVERIFY(containsChange(changes, kiriview::ImageDocumentChange::DisplayedUrl));
    QVERIFY(containsChange(changes, kiriview::ImageDocumentChange::ContainerNavigation));
    QVERIFY(containsChange(changes, kiriview::ImageDocumentChange::Loading));
    QVERIFY(containsChange(changes, kiriview::ImageDocumentChange::Status));
}

void TestImageOpenTransitionApplier::successfulTransitionPublishesEmbeddedMetadataFromContext()
{
    kiriview::ImageDocumentState state;
    kiriview::EmbeddedMetadata previousMetadata;
    previousMetadata.cameraMake = QStringLiteral("Previous Camera");
    state.setEmbeddedMetadata(previousMetadata);

    const QUrl imageUrl = localUrl(QStringLiteral("/images/page.png"));
    const kiriview::ImageLoadSession session {
        11,
        kiriview::ImageLoadRequest::fromUrl(imageUrl),
        kiriview::DisplayedImageLocation::fromUrl(imageUrl),
    };
    kiriview::EmbeddedMetadata metadata;
    metadata.cameraMake = QStringLiteral("Kiri Camera");
    kiriview::ImageOpenTransition transition;
    transition.stateDelta = stateDelta(kiriview::ImageOpenUrlTarget::SessionImage,
        kiriview::ImageOpenDisplayedLocationTarget::Session,
        kiriview::ImageOpenUrlTarget::DerivedContainerNavigation,
        kiriview::ImageOpenBoolTarget::False, kiriview::ImageOpenStatusTarget::Ready,
        kiriview::ImageOpenErrorStringTarget::Clear, true);
    transition.stateDelta.embeddedMetadata = kiriview::ImageOpenEmbeddedMetadataTarget::Provided;

    const kiriview::ImageOpenApplicationPlan applicationPlan = kiriview::imageOpenApplicationPlan(
        transition, kiriview::ImageOpenTransitionContext::successfulImageLoad(session, metadata));
    const kiriview::ImageDocumentRuntimePlan plan
        = kiriview::applyImageOpenApplicationPlan(state, applicationPlan);

    QVERIFY(plan.empty());
    QCOMPARE(state.sourceUrl(), imageUrl);
    QCOMPARE(state.embeddedMetadata().cameraMake, QStringLiteral("Kiri Camera"));
}

void TestImageOpenTransitionApplier::errorTransitionUsesDisplayedFallbackAndProvidedError()
{
    std::vector<kiriview::ImageDocumentChange> changes;
    kiriview::ImageDocumentState state(
        [&changes](kiriview::ImageDocumentChange change) { changes.push_back(change); });

    const QUrl previousImageUrl = localUrl(QStringLiteral("/images/previous.png"));
    const QUrl failedImageUrl = localUrl(QStringLiteral("/images/missing.png"));
    state.setSourceUrl(failedImageUrl);
    state.setDisplayedImageLocation(kiriview::DisplayedImageLocation::fromUrl(previousImageUrl));
    state.setLoading(true);
    changes.clear();

    const kiriview::ImageLoadSession session {
        9,
        kiriview::ImageLoadRequest::fromUrl(failedImageUrl),
        kiriview::DisplayedImageLocation::fromUrl(failedImageUrl),
    };
    kiriview::ImageOpenTransition transition;
    transition.stateDelta = stateDelta(kiriview::ImageOpenUrlTarget::Displayed,
        kiriview::ImageOpenDisplayedLocationTarget::Unchanged,
        kiriview::ImageOpenUrlTarget::Unchanged, kiriview::ImageOpenBoolTarget::False,
        kiriview::ImageOpenStatusTarget::Error, kiriview::ImageOpenErrorStringTarget::Provided,
        true);
    transition.effects.push_back(kiriview::ImageOpenEffect::UpdatePageNavigation);
    transition.effects.push_back(kiriview::ImageOpenEffect::ScheduleAdjacentImagePredecode);

    const kiriview::ImageOpenApplicationPlan applicationPlan
        = kiriview::imageOpenApplicationPlan(transition,
            kiriview::ImageOpenTransitionContext::sourceLoadError(session, previousImageUrl,
                kiriview::ImageLoadFailure {
                    session.imageUrl(),
                    session.id(),
                    kiriview::ImageLoadFailureKind::DataLoad,
                    QStringLiteral("missing"),
                    QStringLiteral("missing"),
                    kiriview::ImageLoadFailureSeverity::Error,
                    false,
                }));
    const kiriview::ImageDocumentRuntimePlan plan
        = kiriview::applyImageOpenApplicationPlan(state, applicationPlan);

    QCOMPARE(state.sourceUrl(), previousImageUrl);
    QCOMPARE(state.displayedUrl(), previousImageUrl);
    QVERIFY(!state.loading());
    QCOMPARE(state.status(), kiriview::ImageDocumentStatus::Error);
    QCOMPARE(state.errorString(), QStringLiteral("missing"));
    QVERIFY(findOperation<kiriview::UpdatePageNavigationOperation>(plan) != nullptr);
    QVERIFY(findOperation<kiriview::ScheduleAdjacentImagePredecodeOperation>(plan) != nullptr);
    QVERIFY(!changes.empty());
    QCOMPARE(changes.front(), kiriview::ImageDocumentChange::Loading);
}

void TestImageOpenTransitionApplier::invalidReadyWithErrorPlanDoesNotMutateStateOrRunEffects()
{
    std::vector<kiriview::ImageDocumentChange> changes;
    kiriview::ImageDocumentState state(
        [&changes](kiriview::ImageDocumentChange change) { changes.push_back(change); });

    state.setSourceUrl(localUrl(QStringLiteral("/images/current.png")));
    state.setLoading(true);
    state.setStatus(kiriview::ImageDocumentStatus::Loading);
    changes.clear();

    kiriview::ImageOpenTransition transition;
    transition.stateDelta = stateDelta(kiriview::ImageOpenUrlTarget::Unchanged,
        kiriview::ImageOpenDisplayedLocationTarget::Unchanged,
        kiriview::ImageOpenUrlTarget::Unchanged, kiriview::ImageOpenBoolTarget::False,
        kiriview::ImageOpenStatusTarget::Ready, kiriview::ImageOpenErrorStringTarget::Provided,
        false);
    transition.effects.push_back(kiriview::ImageOpenEffect::UpdatePageNavigation);

    const kiriview::ImageDocumentRuntimePlan plan
        = kiriview::applyImageOpenTransition(state, transition,
            kiriview::ImageOpenTransitionContext::animationError(QStringLiteral("late error")));

    QCOMPARE(state.sourceUrl(), localUrl(QStringLiteral("/images/current.png")));
    QVERIFY(state.loading());
    QCOMPARE(state.status(), kiriview::ImageDocumentStatus::Loading);
    QVERIFY(state.errorString().isEmpty());
    QVERIFY(plan.empty());
    QVERIFY(changes.empty());
}

void TestImageOpenTransitionApplier::invalidReadyWithEmptySourcePlanDoesNotMutateStateOrRunEffects()
{
    std::vector<kiriview::ImageDocumentChange> changes;
    kiriview::ImageDocumentState state(
        [&changes](kiriview::ImageDocumentChange change) { changes.push_back(change); });

    const QUrl sourceUrl = localUrl(QStringLiteral("/images/current.png"));
    const QUrl displayedUrl = localUrl(QStringLiteral("/images/displayed.png"));
    state.setSourceUrl(sourceUrl);
    state.setDisplayedImageLocation(kiriview::DisplayedImageLocation::fromUrl(displayedUrl));
    state.setStatus(kiriview::ImageDocumentStatus::Loading);
    state.setLoading(true);
    changes.clear();

    kiriview::ImageOpenApplicationPlan applicationPlan;
    applicationPlan.stateDelta.sourceUrl = QUrl();
    applicationPlan.stateDelta.loading = false;
    applicationPlan.stateDelta.status = kiriview::ImageDocumentStatus::Ready;
    applicationPlan.stateDelta.errorString = QString();
    applicationPlan.runtimePlan.push_back(kiriview::UpdatePageNavigationOperation {});

    const kiriview::ImageDocumentRuntimePlan plan
        = kiriview::applyImageOpenApplicationPlan(state, std::move(applicationPlan));

    QCOMPARE(state.sourceUrl(), sourceUrl);
    QCOMPARE(state.displayedUrl(), displayedUrl);
    QVERIFY(state.loading());
    QCOMPARE(state.status(), kiriview::ImageDocumentStatus::Loading);
    QVERIFY(plan.empty());
    QVERIFY(changes.empty());
}

void TestImageOpenTransitionApplier::
    invalidReadyVideoSourceWithoutUnsupportedFlagDoesNotMutateStateOrRunEffects()
{
    std::vector<kiriview::ImageDocumentChange> changes;
    kiriview::ImageDocumentState state(
        [&changes](kiriview::ImageDocumentChange change) { changes.push_back(change); });

    const QUrl sourceUrl = localUrl(QStringLiteral("/images/current.png"));
    const QUrl videoUrl = localUrl(QStringLiteral("/archive/video.mp4"));
    state.setSourceUrl(sourceUrl);
    state.setDisplayedImageLocation(kiriview::DisplayedImageLocation::fromUrl(sourceUrl));
    state.setStatus(kiriview::ImageDocumentStatus::Loading);
    state.setLoading(true);
    changes.clear();

    kiriview::ImageOpenApplicationPlan applicationPlan;
    applicationPlan.stateDelta.sourceUrl = videoUrl;
    applicationPlan.stateDelta.sourceKind = kiriview::ImageDocumentPageKind::Video;
    applicationPlan.stateDelta.displayedLocation
        = kiriview::DisplayedImageLocation::fromUrl(videoUrl);
    applicationPlan.stateDelta.loading = false;
    applicationPlan.stateDelta.status = kiriview::ImageDocumentStatus::Ready;
    applicationPlan.stateDelta.errorString = QString();
    applicationPlan.runtimePlan.push_back(kiriview::UpdatePageNavigationOperation {});

    const kiriview::ImageDocumentRuntimePlan plan
        = kiriview::applyImageOpenApplicationPlan(state, std::move(applicationPlan));

    QCOMPARE(state.sourceUrl(), sourceUrl);
    QCOMPARE(state.displayedUrl(), sourceUrl);
    QCOMPARE(state.sourceKind(), kiriview::ImageDocumentPageKind::Image);
    QVERIFY(!state.unsupportedOpenedCollectionVideo());
    QVERIFY(state.loading());
    QCOMPARE(state.status(), kiriview::ImageDocumentStatus::Loading);
    QVERIFY(plan.empty());
    QVERIFY(changes.empty());
}

void TestImageOpenTransitionApplier::
    invalidReadyWithUnrelatedContainerNavigationPlanDoesNotMutateStateOrRunEffects()
{
    std::vector<kiriview::ImageDocumentChange> changes;
    kiriview::ImageDocumentState state(
        [&changes](kiriview::ImageDocumentChange change) { changes.push_back(change); });

    const QUrl previousSourceUrl = localUrl(QStringLiteral("/images/current.png"));
    const QUrl directImageUrl = localUrl(QStringLiteral("/images/next.png"));
    const QUrl unrelatedContainerUrl = localUrl(QStringLiteral("/books/book.cbz"));
    state.setSourceUrl(previousSourceUrl);
    state.setDisplayedImageLocation(kiriview::DisplayedImageLocation::fromUrl(previousSourceUrl));
    state.setStatus(kiriview::ImageDocumentStatus::Loading);
    state.setLoading(true);
    changes.clear();

    kiriview::ImageOpenApplicationPlan applicationPlan;
    applicationPlan.stateDelta.sourceUrl = directImageUrl;
    applicationPlan.stateDelta.displayedLocation
        = kiriview::DisplayedImageLocation::fromUrl(directImageUrl);
    applicationPlan.stateDelta.containerNavigationUrl = unrelatedContainerUrl;
    applicationPlan.stateDelta.loading = false;
    applicationPlan.stateDelta.status = kiriview::ImageDocumentStatus::Ready;
    applicationPlan.stateDelta.errorString = QString();
    applicationPlan.runtimePlan.push_back(kiriview::UpdatePageNavigationOperation {});

    const kiriview::ImageDocumentRuntimePlan plan
        = kiriview::applyImageOpenApplicationPlan(state, std::move(applicationPlan));

    QCOMPARE(state.sourceUrl(), previousSourceUrl);
    QCOMPARE(state.displayedUrl(), previousSourceUrl);
    QVERIFY(state.containerNavigationUrl().isEmpty());
    QVERIFY(state.loading());
    QCOMPARE(state.status(), kiriview::ImageDocumentStatus::Loading);
    QVERIFY(plan.empty());
    QVERIFY(changes.empty());
}

void TestImageOpenTransitionApplier::
    invalidNullWithContainerNavigationPlanDoesNotMutateStateOrRunEffects()
{
    std::vector<kiriview::ImageDocumentChange> changes;
    kiriview::ImageDocumentState state(
        [&changes](kiriview::ImageDocumentChange change) { changes.push_back(change); });

    const QUrl sourceUrl = localUrl(QStringLiteral("/images/current.png"));
    const QUrl containerUrl = localUrl(QStringLiteral("/images/"));
    state.setSourceUrl(sourceUrl);
    state.setDisplayedImageLocation(kiriview::DisplayedImageLocation::fromUrl(sourceUrl));
    state.setStatus(kiriview::ImageDocumentStatus::Loading);
    state.setLoading(true);
    changes.clear();

    kiriview::ImageOpenApplicationPlan applicationPlan;
    applicationPlan.stateDelta.containerNavigationUrl = containerUrl;
    applicationPlan.stateDelta.loading = false;
    applicationPlan.stateDelta.status = kiriview::ImageDocumentStatus::Null;
    applicationPlan.stateDelta.errorString = QString();
    applicationPlan.runtimePlan.push_back(kiriview::ClearPresentationImageOperation {});

    const kiriview::ImageDocumentRuntimePlan plan
        = kiriview::applyImageOpenApplicationPlan(state, std::move(applicationPlan));

    QCOMPARE(state.sourceUrl(), sourceUrl);
    QCOMPARE(state.displayedUrl(), sourceUrl);
    QVERIFY(state.containerNavigationUrl().isEmpty());
    QVERIFY(state.loading());
    QCOMPARE(state.status(), kiriview::ImageDocumentStatus::Loading);
    QVERIFY(plan.empty());
    QVERIFY(changes.empty());
}

void TestImageOpenTransitionApplier::missingRuntimeContextDoesNotClearResolvedTargets()
{
    std::vector<kiriview::ImageDocumentChange> changes;
    kiriview::ImageDocumentState state(
        [&changes](kiriview::ImageDocumentChange change) { changes.push_back(change); });

    const QUrl sourceUrl = localUrl(QStringLiteral("/images/current.png"));
    const QUrl displayedUrl = localUrl(QStringLiteral("/images/displayed.png"));
    const QUrl containerUrl = localUrl(QStringLiteral("/images/"));
    state.setSourceUrl(sourceUrl);
    state.setDisplayedImageLocation(kiriview::DisplayedImageLocation::fromUrl(displayedUrl));
    state.setContainerNavigationUrl(containerUrl);
    state.setLoading(true);
    state.setErrorString(QStringLiteral("previous error"));
    changes.clear();

    kiriview::ImageOpenTransition transition;
    transition.stateDelta = stateDelta(kiriview::ImageOpenUrlTarget::SessionImage,
        kiriview::ImageOpenDisplayedLocationTarget::Session,
        kiriview::ImageOpenUrlTarget::DerivedContainerNavigation,
        kiriview::ImageOpenBoolTarget::False, kiriview::ImageOpenStatusTarget::Ready,
        kiriview::ImageOpenErrorStringTarget::Provided, true);
    transition.effects.push_back(kiriview::ImageOpenEffect::PrepareFailedContainer);

    const kiriview::ImageDocumentRuntimePlan plan
        = kiriview::applyImageOpenTransition(state, transition);

    QCOMPARE(state.sourceUrl(), sourceUrl);
    QCOMPARE(state.displayedUrl(), displayedUrl);
    QCOMPARE(state.containerNavigationUrl(), containerUrl);
    QCOMPARE(state.errorString(), QStringLiteral("previous error"));
    QVERIFY(state.loading());
    QCOMPARE(state.status(), kiriview::ImageDocumentStatus::Null);
    QVERIFY(plan.empty());
    QVERIFY(!containsChange(changes, kiriview::ImageDocumentChange::SourceUrl));
    QVERIFY(!containsChange(changes, kiriview::ImageDocumentChange::DisplayedUrl));
    QVERIFY(!containsChange(changes, kiriview::ImageDocumentChange::ContainerNavigation));
    QVERIFY(!containsChange(changes, kiriview::ImageDocumentChange::ErrorString));
    QVERIFY(!containsChange(changes, kiriview::ImageDocumentChange::Loading));
    QVERIFY(!containsChange(changes, kiriview::ImageDocumentChange::Status));
}

QTEST_GUILESS_MAIN(TestImageOpenTransitionApplier)

#include "test_imageopentransitionapplier.moc"
