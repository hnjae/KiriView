// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "application/menuaccesskeyinput.h"

#include <QKeyEvent>
#include <QObject>
#include <QTest>
#include <vector>

class TestMenuAccessKeyInput : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void inputKindClassifiesAltKeySeparatelyFromMnemonics();
    void inputKindClassifiesAltModifiedMnemonic();
    void inputKindClassifiesPlainMnemonicWithIgnoredModifiers();
    void inputKindRejectsNonMnemonicModifiers();
    void mnemonicHelpersExcludeAltKeyAndTrackAltModifier();
};

namespace {
QKeyEvent keyPress(int key, Qt::KeyboardModifiers modifiers = Qt::NoModifier)
{
    return QKeyEvent(QEvent::KeyPress, key, modifiers);
}
}

void TestMenuAccessKeyInput::inputKindClassifiesAltKeySeparatelyFromMnemonics()
{
    const QKeyEvent alt = keyPress(Qt::Key_Alt, Qt::AltModifier);

    QCOMPARE(KiriView::menuAccessKeyInputKind(alt), KiriView::MenuAccessKeyInputKind::AltKey);
    QVERIFY(!KiriView::menuAccessKeyIsMnemonicKeyPress(alt));
    QVERIFY(!KiriView::menuAccessKeyIsAltMnemonicKeyPress(alt));
}

void TestMenuAccessKeyInput::inputKindClassifiesAltModifiedMnemonic()
{
    const QKeyEvent event = keyPress(Qt::Key_O, Qt::AltModifier);

    QCOMPARE(
        KiriView::menuAccessKeyInputKind(event), KiriView::MenuAccessKeyInputKind::AltMnemonic);
    QVERIFY(KiriView::menuAccessKeyIsMnemonicKeyPress(event));
    QVERIFY(KiriView::menuAccessKeyIsAltMnemonicKeyPress(event));
}

void TestMenuAccessKeyInput::inputKindClassifiesPlainMnemonicWithIgnoredModifiers()
{
    const std::vector<Qt::KeyboardModifiers> modifiers {
        Qt::NoModifier,
        Qt::ShiftModifier,
        Qt::KeypadModifier,
        Qt::GroupSwitchModifier,
        Qt::ShiftModifier | Qt::KeypadModifier | Qt::GroupSwitchModifier,
    };

    for (Qt::KeyboardModifiers modifier : modifiers) {
        const QKeyEvent event = keyPress(Qt::Key_O, modifier);

        QCOMPARE(
            KiriView::menuAccessKeyInputKind(event), KiriView::MenuAccessKeyInputKind::Mnemonic);
        QVERIFY(KiriView::menuAccessKeyIsMnemonicKeyPress(event));
        QVERIFY(!KiriView::menuAccessKeyIsAltMnemonicKeyPress(event));
    }
}

void TestMenuAccessKeyInput::inputKindRejectsNonMnemonicModifiers()
{
    const std::vector<Qt::KeyboardModifiers> modifiers {
        Qt::ControlModifier,
        Qt::MetaModifier,
        Qt::ControlModifier | Qt::AltModifier,
        Qt::MetaModifier | Qt::ShiftModifier,
    };

    for (Qt::KeyboardModifiers modifier : modifiers) {
        const QKeyEvent event = keyPress(Qt::Key_O, modifier);

        QCOMPARE(KiriView::menuAccessKeyInputKind(event), KiriView::MenuAccessKeyInputKind::Other);
        QVERIFY(!KiriView::menuAccessKeyIsMnemonicKeyPress(event));
        QVERIFY(!KiriView::menuAccessKeyIsAltMnemonicKeyPress(event));
    }
}

void TestMenuAccessKeyInput::mnemonicHelpersExcludeAltKeyAndTrackAltModifier()
{
    const QKeyEvent plain = keyPress(Qt::Key_F);
    const QKeyEvent shiftedAlt = keyPress(Qt::Key_F, Qt::AltModifier | Qt::ShiftModifier);

    QVERIFY(KiriView::menuAccessKeyIsMnemonicKeyPress(plain));
    QVERIFY(!KiriView::menuAccessKeyIsAltMnemonicKeyPress(plain));
    QVERIFY(KiriView::menuAccessKeyIsMnemonicKeyPress(shiftedAlt));
    QVERIFY(KiriView::menuAccessKeyIsAltMnemonicKeyPress(shiftedAlt));
}

QTEST_GUILESS_MAIN(TestMenuAccessKeyInput)

#include "test_menuaccesskeyinput.moc"
