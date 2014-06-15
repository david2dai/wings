/* vim:set et ts=4 sts=4:
 *
 * ibus-pinyin - The Chinese PinYin engine for IBus
 *
 * Copyright (c) 2008-2010 Peng Huang <shawn.p.huang@gmail.com>
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
#ifndef __PY_ENGINE_H_
#define __PY_ENGINE_H_

#include <ibus.h>

#include "PYPointer.h"
#include "PYLookupTable.h"
#include "PYProperty.h"
#include "PYEditor.h"

namespace PY {

#define IBUS_TYPE_PINYIN_ENGINE	\
	(PY::ibus_pinyin_engine_get_type ())

GType   ibus_pinyin_engine_get_type    (void);

class Engine {
public:
    Engine (IBusEngine *engine) : m_engine (engine) { }
    virtual ~Engine (void);

    // virtual functions
    virtual gboolean processKeyEvent (guint keyval, guint keycode, guint modifiers) = 0;
    virtual void focusIn (void) = 0;
    virtual void focusOut (void) = 0;
    virtual void reset (void) = 0;
    virtual void enable (void) = 0;
    virtual void disable (void) = 0;
    virtual void pageUp (void) = 0;
    virtual void pageDown (void) = 0;
    virtual void cursorUp (void) = 0;
    virtual void cursorDown (void) = 0;
    virtual gboolean propertyActivate (const gchar *prop_name, guint prop_state) = 0;
    virtual void candidateClicked (guint index, guint button, guint state) = 0;
    
    // David
    // Process the glib signal from the ibusengine service
    virtual void focusedRecWord (IBusText *sel_rec_word) = 0;
    virtual void focusedRecSentence (IBusText *sel_rec_sentence) = 0;
     
    virtual void pageUpRecWords (void) = 0;
    virtual void pageDownRecWords  (void) = 0;
    virtual void cursorUpRecWords  (void) = 0;
    virtual void cursorDownRecWords  (void) = 0;
    virtual void candidateClickedRecWords  (guint index, guint button, guint state) = 0;
     
    virtual void pageUpRecSentences (void) = 0;
    virtual void pageDownRecSentences (void) = 0;
    virtual void cursorUpRecSentences (void) = 0;
    virtual void cursorDownRecSentences (void) = 0;
    virtual void candidateClickedRecSentences (guint index, guint button, guint state) = 0;

protected:
    void commitText (Text & text) const
    {
        ibus_engine_commit_text (m_engine, text);
    }

    void updatePreeditText (Text & text, guint cursor, gboolean visible) const
    {
        ibus_engine_update_preedit_text (m_engine, text, cursor, visible);
    }

    // David 
    // call ibus engine to update the focused area indicator
    void updateFocusedAreaIndicator (guint indicator) const
    {
        ibus_engine_update_focused_area_indicator (m_engine, indicator);
    }

    void showPreeditText (void) const
    {
        ibus_engine_show_preedit_text (m_engine);
    }

    void hidePreeditText (void) const
    {
        ibus_engine_hide_preedit_text (m_engine);
    }

    void updateAuxiliaryText (Text & text, gboolean visible) const
    {
        ibus_engine_update_auxiliary_text (m_engine, text, visible);
    }

    void showAuxiliaryText (void) const
    {
        ibus_engine_show_auxiliary_text (m_engine);
    }

    void hideAuxiliaryText (void) const
    {
        ibus_engine_hide_auxiliary_text (m_engine);
    }

    void updateLookupTable (LookupTable &table, gboolean visible) const
    {
        ibus_engine_update_lookup_table (m_engine, table, visible);
    }

    void updateLookupTableFast (LookupTable &table, gboolean visible) const
    {
        ibus_engine_update_lookup_table_fast (m_engine, table, visible);
    }

    void showLookupTable (void) const
    {
        ibus_engine_show_lookup_table (m_engine);
    }

    void hideLookupTable (void) const
    {
        ibus_engine_hide_lookup_table (m_engine);
    }


    // David
    void updateLookupTableRecWords (LookupTable &table, gboolean visible) const
    {
        ibus_engine_update_lookup_table_rec_words (m_engine, table, visible);
    }

    void updateLookupTableFastRecWords (LookupTable &table, gboolean visible) const
    {
        ibus_engine_update_lookup_table_fast_rec_words (m_engine, table, visible);
    }

    void showLookupTableRecWords (void) const
    {
        ibus_engine_show_lookup_table_rec_words (m_engine);
    }

    void hideLookupTableRecWords (void) const
    {
        ibus_engine_hide_lookup_table_rec_words (m_engine);
    }

    // David
    void updateLookupTableRecSentences (LookupTable &table, gboolean visible) const
    {
        ibus_engine_update_lookup_table_rec_sentences (m_engine, table, visible);
    }

    void updateLookupTableFastRecSentences (LookupTable &table, gboolean visible) const
    {
        ibus_engine_update_lookup_table_fast_rec_sentences (m_engine, table, visible);
    }

    void showLookupTableRecSentences (void) const
    {
        ibus_engine_show_lookup_table_rec_sentences (m_engine);
    }

    void hideLookupTableRecSentences (void) const
    {
        ibus_engine_hide_lookup_table_rec_sentences (m_engine);
    }

    //
    
    void registerProperties (PropList & props) const
    {
        ibus_engine_register_properties (m_engine, props);
    }

    void updateProperty (Property & prop) const
    {
        ibus_engine_update_property (m_engine, prop);
    }

protected:
    Pointer<IBusEngine>  m_engine;      // engine pointer
    // David
    //guint  m_focused_area_indicator;

};

};
#endif
