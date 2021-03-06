/* vim:set et ts=4 sts=4:
 *
 * ibus-pinyin - The Chinese PinYin engine for IBus
 *
 * Copyright (c) 2008-2010 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright (c) 2010 BYVoid <byvoid1@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include "PYBopomofoEngine.h"
#include <string>
#include "PYRawEditor.h"
#include "PYPunctEditor.h"
#ifdef IBUS_BUILD_LUA_EXTENSION
#include "PYExtEditor.h"
#endif
#include "PYBopomofoEditor.h"
#include "PYFallbackEditor.h"
#include "PYConfig.h"

namespace PY {

/* constructor */
BopomofoEngine::BopomofoEngine (IBusEngine *engine)
    : Engine (engine),
      m_props (BopomofoConfig::instance ()),
      m_prev_pressed_key (IBUS_VoidSymbol),
      m_input_mode (MODE_INIT),
      m_fallback_editor (new FallbackEditor (m_props, BopomofoConfig::instance ()))
{
    gint i;

    /* create editors */
    m_editors[MODE_INIT].reset (new BopomofoEditor (m_props, BopomofoConfig::instance ()));
    m_editors[MODE_PUNCT].reset (new PunctEditor (m_props, BopomofoConfig::instance ()));

    m_editors[MODE_RAW].reset (new RawEditor (m_props, BopomofoConfig::instance ()));
#ifdef IBUS_BUILD_LUA_EXTENSION
    m_editors[MODE_EXTENSION].reset (new ExtEditor (m_props, BopomofoConfig::instance ()));
#else
    m_editors[MODE_EXTENSION].reset (new Editor (m_props, BopomofoConfig::instance ()));
#endif

    m_props.signalUpdateProperty ().connect (std::bind (&BopomofoEngine::updateProperty, this, _1));

    for (i = MODE_INIT; i < MODE_LAST; i++) {
        connectEditorSignals (m_editors[i]);
    }

    connectEditorSignals (m_fallback_editor);
}

/* destructor */
BopomofoEngine::~BopomofoEngine (void)
{
}

gboolean
BopomofoEngine::processKeyEvent (guint keyval, guint keycode, guint modifiers)
{
    gboolean retval = FALSE;

    /* check Shift + Release hotkey,
     * and then ignore other Release key event */
    if (modifiers & IBUS_RELEASE_MASK) {
        /* press and release keyval are same,
         * and no other key event between the press and release ket event*/
        if (m_prev_pressed_key == keyval) {
            if (keyval == IBUS_Shift_L || keyval == IBUS_Shift_R) {
                if (!m_editors[MODE_INIT]->text ().empty ())
                    m_editors[MODE_INIT]->reset ();
                m_props.toggleModeChinese ();
                return TRUE;
            }
        }

        if (m_input_mode == MODE_INIT &&
            m_editors[MODE_INIT]->text ().empty ()) {
            /* If it is init mode, and no any previouse input text,
             * we will let client applications to handle release key event */
            return FALSE;
        }
        else {
            return TRUE;
        }
    }

    /* Toggle simp/trad Chinese Mode when hotkey Ctrl + Shift + F pressed */
    if (keyval == IBUS_F && scmshm_test (modifiers, (IBUS_SHIFT_MASK | IBUS_CONTROL_MASK))) {
        m_props.toggleModeSimp();
        m_prev_pressed_key = IBUS_F;
        return TRUE;
    }

    if (m_props.modeChinese ()) {
        if (G_UNLIKELY (m_input_mode == MODE_INIT &&
                        m_editors[MODE_INIT]->text ().empty () &&
                        (cmshm_filter (modifiers)) == 0) &&
                        keyval == IBUS_grave) {
            /* if BopomofoEditor is empty and get a grave key,
             * switch current editor to PunctEditor */
            m_input_mode = MODE_PUNCT;
        }

        retval = m_editors[m_input_mode]->processKeyEvent (keyval, keycode, modifiers);
        if (G_UNLIKELY (retval &&
                        m_input_mode != MODE_INIT &&
                        m_editors[m_input_mode]->text ().empty ()))
            m_input_mode = MODE_INIT;
    }

    if (G_UNLIKELY (!retval))
        retval = m_fallback_editor->processKeyEvent (keyval, keycode, modifiers);

    /* store ignored key event by editors */
    m_prev_pressed_key = retval ? IBUS_VoidSymbol : keyval;

    return retval;
}

void
BopomofoEngine::focusIn (void)
{
    registerProperties (m_props.properties ());
}

void
BopomofoEngine::focusOut (void)
{
    reset ();
}

void
BopomofoEngine::reset (void)
{
    m_prev_pressed_key = IBUS_VoidSymbol;
    m_input_mode = MODE_INIT;
    for (gint i = 0; i < MODE_LAST; i++) {
        m_editors[i]->reset ();
    }
    m_fallback_editor->reset ();
}

void
BopomofoEngine::enable (void)
{
    m_props.reset ();
}

void
BopomofoEngine::disable (void)
{
}

void
BopomofoEngine::pageUp (void)
{
    m_editors[m_input_mode]->pageUp ();
}

void
BopomofoEngine::pageDown (void)
{
    m_editors[m_input_mode]->pageDown ();
}

void
BopomofoEngine::cursorUp (void)
{
    m_editors[m_input_mode]->cursorUp ();
}

void
BopomofoEngine::cursorDown (void)
{
    m_editors[m_input_mode]->cursorDown ();
}

inline void
BopomofoEngine::showSetupDialog (void)
{
    g_spawn_command_line_async (LIBEXECDIR"/ibus-setup-pinyin bopomofo", NULL);
}

gboolean
BopomofoEngine::propertyActivate (const gchar *prop_name, guint prop_state)
{
    const static std::string setup ("setup");
    if (m_props.propertyActivate (prop_name, prop_state)) {
        return TRUE;
    }
    else if (setup == prop_name) {
        showSetupDialog ();
        return TRUE;
    }
    return FALSE;
}

void
BopomofoEngine::candidateClicked (guint index, guint button, guint state)
{
    m_editors[m_input_mode]->candidateClicked (index, button, state);
}

void
BopomofoEngine::candidateClickedRecWords (guint index, guint button, guint state)
{
    m_editors[m_input_mode]->candidateClickedRecWords (index, button, state);
}

void
BopomofoEngine::candidateClickedRecSentences (guint index, guint button, guint state)
{
    m_editors[m_input_mode]->candidateClickedRecSentences (index, button, state);
}


// David
void
BopomofoEngine::focusedRecWord (IBusText *sel_rec_word)
{
    /* 
    if (m_input_mode != MODE_INIT)
        return;
    m_editors[m_input_mode]->focusedRecWord (sel_rec_word);
    */
}

// David
// May be pageUp/Down cusorUp/Down is not need, 
// it can use the same with the original
// Words
void
BopomofoEngine::pageUpRecWords (void)
{
    m_editors[m_input_mode]->pageUp ();
}

void
BopomofoEngine::pageDownRecWords  (void)
{
    m_editors[m_input_mode]->pageDown ();
}

void
BopomofoEngine::cursorUpRecWords  (void)
{
    m_editors[m_input_mode]->cursorUp ();
}

void
BopomofoEngine::cursorDownRecWords  (void)
{
    m_editors[m_input_mode]->cursorDown ();
}

// Sentences 
void
BopomofoEngine::pageUpRecSentences (void)
{
    m_editors[m_input_mode]->pageUp ();
}

void
BopomofoEngine::pageDownRecSentences (void)
{
    m_editors[m_input_mode]->pageDown ();
}

void
BopomofoEngine::cursorUpRecSentences (void)
{
    m_editors[m_input_mode]->cursorUp ();
}

void
BopomofoEngine::cursorDownRecSentences (void)
{
    m_editors[m_input_mode]->cursorDown ();
}

void
BopomofoEngine::focusedRecSentence (IBusText *sel_rec_sentence)
{
    /*
      if (m_input_mode != MODE_INIT)
        return;
    m_editors[m_input_mode]->focusedRecSentence (sel_rec_sentence);
    */
}

void
BopomofoEngine::commitText (Text & text)
{
    Engine::commitText (text);
    if (m_input_mode != MODE_INIT)
        m_input_mode = MODE_INIT;
    if (text.text ())
        static_cast<FallbackEditor*> (m_fallback_editor.get ())->setPrevCommittedChar (*text.text ());
    else
        static_cast<FallbackEditor*> (m_fallback_editor.get ())->setPrevCommittedChar (0);
}

void
BopomofoEngine::connectEditorSignals (EditorPtr editor)
{
    editor->signalCommitText ().connect (
        std::bind (&BopomofoEngine::commitText, this, _1));

    editor->signalUpdatePreeditText ().connect (
        std::bind (&BopomofoEngine::updatePreeditText, this, _1, _2, _3));
    editor->signalShowPreeditText ().connect (
        std::bind (&BopomofoEngine::showPreeditText, this));
    editor->signalHidePreeditText ().connect (
        std::bind (&BopomofoEngine::hidePreeditText, this));

    editor->signalUpdateAuxiliaryText ().connect (
        std::bind (&BopomofoEngine::updateAuxiliaryText, this, _1, _2));
    editor->signalShowAuxiliaryText ().connect (
        std::bind (&BopomofoEngine::showAuxiliaryText, this));
    editor->signalHideAuxiliaryText ().connect (
        std::bind (&BopomofoEngine::hideAuxiliaryText, this));

    editor->signalUpdateLookupTable ().connect (
        std::bind (&BopomofoEngine::updateLookupTable, this, _1, _2));
    editor->signalUpdateLookupTableFast ().connect (
        std::bind (&BopomofoEngine::updateLookupTableFast, this, _1, _2));
    editor->signalShowLookupTable ().connect (
        std::bind (&BopomofoEngine::showLookupTable, this));
    editor->signalHideLookupTable ().connect (
        std::bind (&BopomofoEngine::hideLookupTable, this));
}

};

