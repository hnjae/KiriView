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
KiriView::ImageViewRenderContextBindingInput bindingInput(
    bool documentAttached, bool secondaryPage = false, bool componentComplete = true)
{
    return KiriView::ImageViewRenderContextBindingInput {
        documentAttached,
        secondaryPage,
        componentComplete,
    };
}
}

void TestImageViewRenderContextBinding::primaryViewInstallsAndClearsProvider()
{
    KiriView::ImageViewRenderContextBinding binding;

    QCOMPARE(binding.synchronize(bindingInput(true)),
        KiriView::ImageViewRenderContextBindingAction::InstallProvider);
    QVERIFY(binding.providerInstalled());
    QCOMPARE(binding.synchronize(bindingInput(true)),
        KiriView::ImageViewRenderContextBindingAction::None);

    QCOMPARE(binding.synchronize(bindingInput(false)),
        KiriView::ImageViewRenderContextBindingAction::ClearProvider);
    QVERIFY(!binding.providerInstalled());
    QCOMPARE(binding.synchronize(bindingInput(false)),
        KiriView::ImageViewRenderContextBindingAction::None);
}

void TestImageViewRenderContextBinding::secondaryViewDoesNotInstallProvider()
{
    KiriView::ImageViewRenderContextBinding binding;

    QCOMPARE(binding.synchronize(bindingInput(false, true)),
        KiriView::ImageViewRenderContextBindingAction::None);
    QCOMPARE(binding.synchronize(bindingInput(true, true)),
        KiriView::ImageViewRenderContextBindingAction::None);
    QVERIFY(!binding.providerInstalled());

    QCOMPARE(binding.synchronize(bindingInput(false, true)),
        KiriView::ImageViewRenderContextBindingAction::None);
}

void TestImageViewRenderContextBinding::changingPageRoleTransfersProviderOwnership()
{
    KiriView::ImageViewRenderContextBinding binding;

    QCOMPARE(binding.synchronize(bindingInput(true)),
        KiriView::ImageViewRenderContextBindingAction::InstallProvider);
    QCOMPARE(binding.synchronize(bindingInput(true, true)),
        KiriView::ImageViewRenderContextBindingAction::ClearProvider);
    QVERIFY(!binding.providerInstalled());

    QCOMPARE(binding.synchronize(bindingInput(true, false)),
        KiriView::ImageViewRenderContextBindingAction::InstallProvider);
    QVERIFY(binding.providerInstalled());
}

void TestImageViewRenderContextBinding::componentDeferralDelaysProviderOwnership()
{
    KiriView::ImageViewRenderContextBinding binding;

    QCOMPARE(binding.synchronize(bindingInput(false, false, false)),
        KiriView::ImageViewRenderContextBindingAction::None);
    QCOMPARE(binding.synchronize(bindingInput(true, false, false)),
        KiriView::ImageViewRenderContextBindingAction::None);
    QVERIFY(!binding.providerInstalled());

    QCOMPARE(binding.synchronize(bindingInput(true, false, true)),
        KiriView::ImageViewRenderContextBindingAction::InstallProvider);
    QVERIFY(binding.providerInstalled());

    QCOMPARE(binding.synchronize(bindingInput(true, false, false)),
        KiriView::ImageViewRenderContextBindingAction::ClearProvider);
    QVERIFY(!binding.providerInstalled());
    QCOMPARE(binding.synchronize(bindingInput(false, false, false)),
        KiriView::ImageViewRenderContextBindingAction::None);
    QCOMPARE(binding.synchronize(bindingInput(true, false, false)),
        KiriView::ImageViewRenderContextBindingAction::None);
}

void TestImageViewRenderContextBinding::secondaryViewStaysSecondaryAcrossDocumentSwitch()
{
    KiriView::ImageViewRenderContextBinding binding;

    QCOMPARE(binding.synchronize(bindingInput(true, true)),
        KiriView::ImageViewRenderContextBindingAction::None);
    QVERIFY(!binding.providerInstalled());

    QCOMPARE(binding.synchronize(bindingInput(false, true)),
        KiriView::ImageViewRenderContextBindingAction::None);
    QCOMPARE(binding.synchronize(bindingInput(true, true)),
        KiriView::ImageViewRenderContextBindingAction::None);
    QVERIFY(!binding.providerInstalled());
}

QTEST_GUILESS_MAIN(TestImageViewRenderContextBinding)

#include "test_imageviewrendercontextbinding.moc"
