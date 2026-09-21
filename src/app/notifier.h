// JasmineKICQ - an ICQ (OSCAR) client for Symbian Anna/Belle.
// Copyright (C) 2026 - GPL-2.0-or-later, see LICENSE.
//
// Tells the user about a message while another application is in front: on Symbian a
// global "discreet popup" at the top of the screen (tapping it brings the app forward),
// plus vibration. The app itself keeps running in the background with its socket open,
// so this is all a "service" needs to be here. Elsewhere it just logs.
#ifndef NOTIFIER_H
#define NOTIFIER_H

#include <QObject>
#include <QString>

class Notifier : public QObject
{
    Q_OBJECT
public:
    explicit Notifier(QObject *parent = 0);

    void setVibrate(bool on) { m_vibrate = on; }
    void setPopups(bool on) { m_popups = on; }

    /// Shows the popup (title = who, text = what) and vibrates, according to the settings.
    void notify(const QString &title, const QString &text);
    /// Just the vibration, e.g. for an authorization request.
    void vibrate(int ms = 400);

private:
    bool m_vibrate;
    bool m_popups;
};

#endif // NOTIFIER_H
