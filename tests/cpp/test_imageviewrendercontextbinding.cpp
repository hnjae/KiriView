// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imageviewrendercontextbinding.h"

#include <QObject>
#include <QTest>

class TestImageViewRenderContextBinding : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void primaryViewInstallsAndClearsProvider();
    void secondaryViewDoesNotInstallProvider();
    void changingPageRoleTransfersProviderOwnership();
    void componentDeferralDelaysProviderOwnership();
    void secondaryViewStaysSecondaryAcrossDocumentSwitch();
};

namespace {
kiriview::ImageViewRenderContextBindingInput bindingInput(
    bool documentAttached, bool secondaryPage = false, bool componentComplete = true)
{
    return kiriview::ImageViewRenderContextBindingInput {
        documentAttached,
        secondaryPage,
        componentComplete,
    };
}
}

void TestImageViewRenderContextBinding::primaryViewInstallsAndClearsProvider()
{
    kiriview::ImageViewRenderContextBinding binding;

    QCOMPARE(binding.synchronize(bindingInput(true)),
        kiriview::ImageViewRenderContextBindingAction::InstallProvider);
    QVERIFY(binding.providerInstalled());
    QCOMPARE(binding.synchronize(bindingInput(true)),
        kiriview::ImageViewRenderContextBindingAction::None);

    QCOMPARE(binding.synchronize(bindingInput(false)),
        kiriview::ImageViewRenderContextBindingAction::ClearProvider);
    QVERIFY(!binding.providerInstalled());
    QCOMPARE(binding.synchronize(bindingInput(false)),
        kiriview::ImageViewRenderContextBindingAction::None);
}

void TestImageViewRenderContextBinding::secondaryViewDoesNotInstallProvider()
{
    kiriview::ImageViewRenderContextBinding binding;

    QCOMPARE(binding.synchronize(bindingInput(false, true)),
        kiriview::ImageViewRenderContextBindingAction::None);
    QCOMPARE(binding.synchronize(bindingInput(true, true)),
        kiriview::ImageViewRenderContextBindingAction::None);
    QVERIFY(!binding.providerInstalled());

    QCOMPARE(binding.synchronize(bindingInput(false, true)),
        kiriview::ImageViewRenderContextBindingAction::None);
}

void TestImageViewRenderContextBinding::changingPageRoleTransfersProviderOwnership()
{
    kiriview::ImageViewRenderContextBinding binding;

    QCOMPARE(binding.synchronize(bindingInput(true)),
        kiriview::ImageViewRenderContextBindingAction::InstallProvider);
    QCOMPARE(binding.synchronize(bindingInput(true, true)),
        kiriview::ImageViewRenderContextBindingAction::ClearProvider);
    QVERIFY(!binding.providerInstalled());

    QCOMPARE(binding.synchronize(bindingInput(true, false)),
        kiriview::ImageViewRenderContextBindingAction::InstallProvider);
    QVERIFY(binding.providerInstalled());
}

void TestImageViewRenderContextBinding::componentDeferralDelaysProviderOwnership()
{
    kiriview::ImageViewRenderContextBinding binding;

    QCOMPARE(binding.synchronize(bindingInput(false, false, false)),
        kiriview::ImageViewRenderContextBindingAction::None);
    QCOMPARE(binding.synchronize(bindingInput(true, false, false)),
        kiriview::ImageViewRenderContextBindingAction::None);
    QVERIFY(!binding.providerInstalled());

    QCOMPARE(binding.synchronize(bindingInput(true, false, true)),
        kiriview::ImageViewRenderContextBindingAction::InstallProvider);
    QVERIFY(binding.providerInstalled());

    QCOMPARE(binding.synchronize(bindingInput(true, false, false)),
        kiriview::ImageViewRenderContextBindingAction::ClearProvider);
    QVERIFY(!binding.providerInstalled());
    QCOMPARE(binding.synchronize(bindingInput(false, false, false)),
        kiriview::ImageViewRenderContextBindingAction::None);
    QCOMPARE(binding.synchronize(bindingInput(true, false, false)),
        kiriview::ImageViewRenderContextBindingAction::None);
}

void TestImageViewRenderContextBinding::secondaryViewStaysSecondaryAcrossDocumentSwitch()
{
    kiriview::ImageViewRenderContextBinding binding;

    QCOMPARE(binding.synchronize(bindingInput(true, true)),
        kiriview::ImageViewRenderContextBindingAction::None);
    QVERIFY(!binding.providerInstalled());

    QCOMPARE(binding.synchronize(bindingInput(false, true)),
        kiriview::ImageViewRenderContextBindingAction::None);
    QCOMPARE(binding.synchronize(bindingInput(true, true)),
        kiriview::ImageViewRenderContextBindingAction::None);
    QVERIFY(!binding.providerInstalled());
}

QTEST_GUILESS_MAIN(TestImageViewRenderContextBinding)

#include "test_imageviewrendercontextbinding.moc"
