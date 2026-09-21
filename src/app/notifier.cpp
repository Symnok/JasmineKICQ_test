// JasmineKICQ - an ICQ (OSCAR) client for Symbian Anna/Belle.
// Copyright (C) 2026 - GPL-2.0-or-later, see LICENSE.
#include "notifier.h"

#include <QApplication>
#include <QDebug>
#include <QWidget>

#ifdef Q_OS_SYMBIAN
#include <akndiscreetpopup.h>
#include <AknSmallIndicator.h>
#include <AknSoftNotifier.h>
#include <AknSoftNotificationParameters.h>
#include <avkon.hrh>
#include <avkon.rsg>
#include <coeaui.h>
#include <coemain.h>
#include <coeview.h>
#include <e32std.h>
#include <f32file.h>
#include <hwrmvibra.h>
#include <kicqnotes.rsg>

#ifndef KICQ_UID3
#define KICQ_UID3 0xE2C9A7D1
#endif

namespace
{
    const TInt KNotificationViewId = 1;

    _LIT(KNotesFile, "kicqnotes.rsc");
    _LIT(KNotesDir, "\\resource\\apps\\");

    TPtrC ptr(const QString &s)
    {
        return TPtrC(reinterpret_cast<const TUint16 *>(s.utf16()), s.length());
    }

    /// A Qt application has no Avkon views, and a soft notification's "Show" softkey works
    /// by activating a view. This registers one: activating it (from the notification, or
    /// from anything else that knows the UID) brings the application to the front.
    class NotificationView : public MCoeView
    {
    public:
        NotificationView() : m_registered(false)
        {
            CCoeAppUi *ui = CCoeEnv::Static() ? CCoeEnv::Static()->AppUi() : 0;
            if (!ui) return;
            TRAPD(err, ui->RegisterViewL(*this));
            m_registered = err == KErrNone;
            if (err != KErrNone) qWarning() << "view registration failed:" << err;
        }
        ~NotificationView()
        {
            CCoeAppUi *ui = CCoeEnv::Static() ? CCoeEnv::Static()->AppUi() : 0;
            if (ui && m_registered) ui->DeregisterView(*this);
        }
        TVwsViewId ViewId() const { return TVwsViewId(TUid::Uid(KICQ_UID3), TUid::Uid(KNotificationViewId)); }
        void ViewActivatedL(const TVwsViewId &, TUid, const TDesC8 &)
        {
            QWidget *w = QApplication::activeWindow();
            if (!w) {
                QWidgetList tops = QApplication::topLevelWidgets();
                if (!tops.isEmpty()) w = tops.first();
            }
            if (w) { w->showFullScreen(); w->raise(); w->activateWindow(); }
        }
        void ViewDeactivated() {}
    private:
        bool m_registered;
    };

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

    // The "new message" envelope in the status bar - the small indicator the messaging
    // application lights up.
    void setEnvelopeL(bool on)
    {
        CAknSmallIndicator *ind = CAknSmallIndicator::NewLC(TUid::Uid(EAknIndicatorEnvelope));
        ind->SetIndicatorStateL(on ? EAknIndicatorStateOn : EAknIndicatorStateOff);
        CleanupStack::PopAndDestroy(ind);
    }

    // The soft notification parameters point at the note resource installed with the app;
    // the file is looked up on the drives, as the user may have installed to E: or F:.
    // "Show" activates the view registered above, which brings the app forward.
    CAknSoftNotificationParameters *notificationParamsL()
    {
        RFs &fs = CCoeEnv::Static()->FsSession();
        TFindFile finder(fs);
        User::LeaveIfError(finder.FindByDir(KNotesFile, KNotesDir));
        CAknSoftNotificationParameters *p = CAknSoftNotificationParameters::NewL(
            finder.File(), R_KICQ_NOTE_MESSAGE, 0, R_AVKON_SOFTKEYS_SHOW_EXIT, CAknNoteDialog::ENoTone,
            TVwsViewId(TUid::Uid(KICQ_UID3), TUid::Uid(KNotificationViewId)), TUid::Uid(0), EAknSoftkeyShow, KNullDesC8);
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
    : QObject(parent), m_vibrate(true), m_popups(true), m_pending(0), m_view(0)
{
#ifdef Q_OS_SYMBIAN
    m_view = new NotificationView();
#endif
}

Notifier::~Notifier()
{
#ifdef Q_OS_SYMBIAN
    delete static_cast<NotificationView *>(m_view);
#endif
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
    TRAP(err, setEnvelopeL(count > 0));
    if (err != KErrNone) qWarning() << "envelope indicator failed:" << err;
#else
    qDebug() << "NOTIFICATION count" << count;
#endif
}
