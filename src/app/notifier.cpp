// JasmineKICQ - an ICQ (OSCAR) client for Symbian Anna/Belle.
// Copyright (C) 2026 - GPL-2.0-or-later, see LICENSE.
#include "notifier.h"

#include <QDebug>

#ifdef Q_OS_SYMBIAN
#include <akndiscreetpopup.h>
#include <AknSmallIndicator.h>
#include <avkon.hrh>
#include <e32std.h>
#include <hwrmvibra.h>

#ifndef KICQ_UID3
#define KICQ_UID3 0xE2C9A7D1
#endif

namespace
{
    TPtrC ptr(const QString &s)
    {
        return TPtrC(reinterpret_cast<const TUint16 *>(s.utf16()), s.length());
    }

    void showPopupL(const QString &title, const QString &text)
    {
        // Long popup with the lights on and the confirmation tone; tapping it launches
        // (brings forward) this application through its UID.
        CAknDiscreetPopup::ShowGlobalPopupL(ptr(title), ptr(text), KAknsIIDNone, KNullDesC, 0, 0,
            KAknDiscreetPopupDurationLong | KAknDiscreetPopupLightsOn | KAknDiscreetPopupConfirmationTone,
            0, NULL, TUid::Uid(KICQ_UID3));
    }

    void vibrateL(int ms)
    {
        CHWRMVibra *v = CHWRMVibra::NewLC();
        v->StartVibraL(ms);
        CleanupStack::PopAndDestroy(v);
    }

    // The "new message" envelope in the status bar - the same small indicator the messaging
    // application lights up. It stays until the app comes to the foreground.
    void setEnvelopeL(bool on)
    {
        CAknSmallIndicator *ind = CAknSmallIndicator::NewLC(TUid::Uid(EAknIndicatorEnvelope));
        ind->SetIndicatorStateL(on ? EAknIndicatorStateOn : EAknIndicatorStateOff);
        CleanupStack::PopAndDestroy(ind);
    }
}
#endif

Notifier::Notifier(QObject *parent)
    : QObject(parent), m_vibrate(true), m_popups(true), m_pending(0)
{
}

void Notifier::notify(const QString &title, const QString &text)
{
#ifdef Q_OS_SYMBIAN
    if (m_popups) {
        QString t = title;
        QString b = text.simplified();
        if (b.size() > 120) b = b.left(117) + QLatin1String("...");
        TRAP_IGNORE(showPopupL(t, b));
    }
    if (m_vibrate) TRAP_IGNORE(vibrateL(400));
#else
    qDebug() << "NOTIFY" << title << ":" << text << (m_popups ? "" : "(popups off)") << (m_vibrate ? "" : "(vibrate off)");
#endif
}

void Notifier::vibrate(int ms)
{
#ifdef Q_OS_SYMBIAN
    if (m_vibrate) TRAP_IGNORE(vibrateL(ms));
#else
    qDebug() << "VIBRATE" << ms;
#endif
}

void Notifier::setPendingCount(int count)
{
    if (count < 0) count = 0;
    if (count == m_pending) return;
    m_pending = count;
#ifdef Q_OS_SYMBIAN
    TInt err = KErrNone;
    TRAP(err, setEnvelopeL(count > 0));
    if (err != KErrNone) qWarning() << "envelope indicator failed:" << err;
#else
    qDebug() << "ENVELOPE" << (count > 0 ? "on" : "off") << count;
#endif
}
