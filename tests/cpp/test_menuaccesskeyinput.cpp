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

    QCOMPARE(kiriview::menuAccessKeyInputKind(alt), kiriview::MenuAccessKeyInputKind::AltKey);
    QVERIFY(!kiriview::menuAccessKeyIsMnemonicKeyPress(alt));
    QVERIFY(!kiriview::menuAccessKeyIsAltMnemonicKeyPress(alt));
}

void TestMenuAccessKeyInput::inputKindClassifiesAltModifiedMnemonic()
{
    const QKeyEvent event = keyPress(Qt::Key_O, Qt::AltModifier);

    QCOMPARE(
        kiriview::menuAccessKeyInputKind(event), kiriview::MenuAccessKeyInputKind::AltMnemonic);
    QVERIFY(kiriview::menuAccessKeyIsMnemonicKeyPress(event));
    QVERIFY(kiriview::menuAccessKeyIsAltMnemonicKeyPress(event));
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
            kiriview::menuAccessKeyInputKind(event), kiriview::MenuAccessKeyInputKind::Mnemonic);
        QVERIFY(kiriview::menuAccessKeyIsMnemonicKeyPress(event));
        QVERIFY(!kiriview::menuAccessKeyIsAltMnemonicKeyPress(event));
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

        QCOMPARE(kiriview::menuAccessKeyInputKind(event), kiriview::MenuAccessKeyInputKind::Other);
        QVERIFY(!kiriview::menuAccessKeyIsMnemonicKeyPress(event));
        QVERIFY(!kiriview::menuAccessKeyIsAltMnemonicKeyPress(event));
    }
}

void TestMenuAccessKeyInput::mnemonicHelpersExcludeAltKeyAndTrackAltModifier()
{
    const QKeyEvent plain = keyPress(Qt::Key_F);
    const QKeyEvent shiftedAlt = keyPress(Qt::Key_F, Qt::AltModifier | Qt::ShiftModifier);

    QVERIFY(kiriview::menuAccessKeyIsMnemonicKeyPress(plain));
    QVERIFY(!kiriview::menuAccessKeyIsAltMnemonicKeyPress(plain));
    QVERIFY(kiriview::menuAccessKeyIsMnemonicKeyPress(shiftedAlt));
    QVERIFY(kiriview::menuAccessKeyIsAltMnemonicKeyPress(shiftedAlt));
}

QTEST_GUILESS_MAIN(TestMenuAccessKeyInput)

#include "test_menuaccesskeyinput.moc"
