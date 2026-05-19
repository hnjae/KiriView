// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageviewrendercontextbinding.h"

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
};

void TestImageViewRenderContextBinding::primaryViewInstallsAndClearsProvider()
{
    KiriView::ImageViewRenderContextBinding binding;

    QCOMPARE(binding.setDocumentAttached(true),
        KiriView::ImageViewRenderContextBindingAction::InstallProvider);
    QVERIFY(binding.providerInstalled());
    QCOMPARE(
        binding.setDocumentAttached(true), KiriView::ImageViewRenderContextBindingAction::None);

    QCOMPARE(binding.reset(), KiriView::ImageViewRenderContextBindingAction::ClearProvider);
    QVERIFY(!binding.providerInstalled());
    QCOMPARE(binding.reset(), KiriView::ImageViewRenderContextBindingAction::None);
}

void TestImageViewRenderContextBinding::secondaryViewDoesNotInstallProvider()
{
    KiriView::ImageViewRenderContextBinding binding;

    QCOMPARE(binding.setSecondaryPage(true), KiriView::ImageViewRenderContextBindingAction::None);
    QCOMPARE(
        binding.setDocumentAttached(true), KiriView::ImageViewRenderContextBindingAction::None);
    QVERIFY(!binding.providerInstalled());

    QCOMPARE(binding.reset(), KiriView::ImageViewRenderContextBindingAction::None);
}

void TestImageViewRenderContextBinding::changingPageRoleTransfersProviderOwnership()
{
    KiriView::ImageViewRenderContextBinding binding;

    QCOMPARE(binding.setDocumentAttached(true),
        KiriView::ImageViewRenderContextBindingAction::InstallProvider);
    QCOMPARE(binding.setSecondaryPage(true),
        KiriView::ImageViewRenderContextBindingAction::ClearProvider);
    QVERIFY(!binding.providerInstalled());

    QCOMPARE(binding.setSecondaryPage(false),
        KiriView::ImageViewRenderContextBindingAction::InstallProvider);
    QVERIFY(binding.providerInstalled());
}

void TestImageViewRenderContextBinding::componentDeferralDelaysProviderOwnership()
{
    KiriView::ImageViewRenderContextBinding binding;

    QCOMPARE(
        binding.setComponentComplete(false), KiriView::ImageViewRenderContextBindingAction::None);
    QCOMPARE(
        binding.setDocumentAttached(true), KiriView::ImageViewRenderContextBindingAction::None);
    QVERIFY(!binding.providerInstalled());

    QCOMPARE(binding.setComponentComplete(true),
        KiriView::ImageViewRenderContextBindingAction::InstallProvider);
    QVERIFY(binding.providerInstalled());

    QCOMPARE(binding.setComponentComplete(false),
        KiriView::ImageViewRenderContextBindingAction::ClearProvider);
    QVERIFY(!binding.providerInstalled());
    QCOMPARE(binding.reset(), KiriView::ImageViewRenderContextBindingAction::None);
    QCOMPARE(
        binding.setDocumentAttached(true), KiriView::ImageViewRenderContextBindingAction::None);
}

QTEST_GUILESS_MAIN(TestImageViewRenderContextBinding)

#include "test_imageviewrendercontextbinding.moc"
