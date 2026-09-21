// JasmineKICQ - an ICQ (OSCAR) client for Symbian Anna/Belle.
// Copyright (C) 2026 - GPL-2.0-or-later, see LICENSE.
#include "notifier.h"

#include <QDebug>

#ifdef Q_OS_SYMBIAN
#include <akndiscreetpopup.h>
#include <AknSoftNotifier.h>
#include <AknSoftNotificationParameters.h>
#include <avkon.hrh>
#include <avkon.rsg>
#include <coemain.h>
#include <e32std.h>
#include <f32file.h>
#include <hwrmvibra.h>
#include <kicqnotes.rsg>

#ifndef KICQ_UID3
#define KICQ_UID3 0xE2C9A7D1
#endif

namespace
{
    _LIT(KNotesFile, "kicqnotes.rsc");
    _LIT(KNotesDir, "\resource\apps\\");

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

    // The soft notification parameters point at the note resource installed with the app;
    // the file is looked up on the drives, as the user may have installed to E: or F:.
    CAknSoftNotificationParameters *notificationParamsL()
    {
        RFs &fs = CCoeEnv::Static()->FsSession();
        TFindFile finder(fs);
        User::LeaveIfError(finder.FindByDir(KNotesFile, KNotesDir));
        CAknSoftNotificationParameters *p = CAknSoftNotificationParameters::NewL(
            finder.File(), R_KICQ_NOTE_MESSAGE, 0, R_AVKON_SOFTKEYS_SHOW_EXIT, CAknNoteDialog::ENoTone,
            TVwsViewId(TUid::Uid(KICQ_UID3), TUid::Uid(0)), TUid::Uid(0), EAknSoftkeyShow, KNullDesC8);
        p->SetGroupedTexts(R_KICQ_GROUPED_TEXTS);
        return p;
    }

    void setSoftNotificationL(int count)
    {
        CAknSoftNotificationParameters *params = notificationParamsL();
        CleanupStack::PushL(params);
        CAknSoftNotifier *notifier = CAknSoftNotifier::NewLC();
        if (count > 0) notifier->SetCustomNotificationCountL(*params, count);
        else notifier->CancelCustomSoftNotificationL(*params);
        CleanupStack::PopAndDestroy(2, params);
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
    TRAP(err, setSoftNotificationL(count));
    if (err != KErrNone) qWarning() << "soft notification failed:" << err;
#else
    qDebug() << "SOFT-NOTIFICATION count" << count;
#endif
}
