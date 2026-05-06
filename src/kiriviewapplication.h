// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_KIRIVIEWAPPLICATION_H
#define KIRIVIEW_KIRIVIEWAPPLICATION_H

#include <AbstractKirigamiApplication>

#include <QAction>
#include <QKeySequence>
#include <QList>
#include <QString>
#include <QtQml/qqmlregistration.h>

class KiriViewApplication : public AbstractKirigamiApplication
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(MenuPresentation menuPresentation READ menuPresentation WRITE setMenuPresentation
            NOTIFY menuPresentationChanged)
    Q_PROPERTY(int shortcutRevision READ shortcutRevision NOTIFY shortcutRevisionChanged)

public:
    enum MenuPresentation {
        HamburgerMenu = 0,
        MenuBar = 1,
    };
    Q_ENUM(MenuPresentation)

    explicit KiriViewApplication(QObject *parent = nullptr);
    ~KiriViewApplication() override;

    MenuPresentation menuPresentation() const;
    void setMenuPresentation(MenuPresentation presentation);
    int shortcutRevision() const;

    Q_INVOKABLE QAction *action(const QString &actionName);
    Q_INVOKABLE QString shortcutText(const QString &actionName) const;

Q_SIGNALS:
    void menuPresentationChanged();
    void shortcutRevisionChanged();

protected:
    void setupActions() override;

private:
    QAction *addRegisteredAction(const QString &name, const QString &text, const QString &iconName,
        const QList<QKeySequence> &defaultShortcuts = {});
    void handleActionChanged();
    void updateShowMenuBarAction();

    QAction *m_showMenuBarAction = nullptr;
    int m_shortcutRevision = 0;
};

#endif
