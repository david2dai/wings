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
#ifndef __PY_PHONETIC_EDITOR_H_
#define __PY_PHONETIC_EDITOR_H_

#include "PYLookupTable.h"
#include "PYEditor.h"
#include "PYPinyinParser.h"
#include "PYPhraseEditor.h"
#include "PYSpecialPhraseTable.h"
#include "PYDatabase.h"

// David 
// for recommend words
//#include "chseg.h"
#include "dictionary_new.h"
#include "recword.h"
#include "recsentence.h"
//#include "log.h"

#include <vector>
#include <map>

namespace PY {

class SpecialPhraseTable;

class PhoneticEditor : public Editor {
public:
    PhoneticEditor (PinyinProperties & props, Config & config);

    // David
    // add for release resource of the rec word
    virtual ~PhoneticEditor (void);

public:
    /* virtual functions */
    virtual void pageUp (void);
    virtual void pageDown (void);
    virtual void cursorUp (void);
    virtual void cursorDown (void);
     
    virtual void pageUpRecWords  (void);
    virtual void pageDownRecWords  (void);
    virtual void cursorUpRecWords  (void);
    virtual void cursorDownRecWords  (void);

    virtual void pageUpRecSentences (void);
    virtual void pageDownRecSentences (void);
    virtual void cursorUpRecSentences (void);
    virtual void cursorDownRecSentences (void);
    
    virtual void update (void);
    virtual void reset (void);
    virtual void candidateClicked (guint index, guint button, guint state);
    // David
    virtual void candidateClickedRecWords (guint index, guint button, guint state);
    virtual void candidateClickedRecSentences (guint index, guint button, guint state);
    // End 
    virtual gboolean processKeyEvent (guint keyval, guint keycode, guint modifiers);
    virtual gboolean processSpace (guint keyval, guint keycode, guint modifiers);
    virtual gboolean processFunctionKey (guint keyval, guint keycode, guint modifiers);
    virtual void updateLookupTable ();
    virtual void updateLookupTableFast ();
    virtual gboolean fillLookupTableByPage ();
    
    // David 
    // update the focused area indicator
    virtual void updateFocusedAreaIndicator(void);
    
    // David
    // get the rec word / sentence from engine (which is from the engineservice)
    virtual void focusedRecWord (IBusText *sel_rec_word);
    virtual void focusedRecSentence (IBusText *sel_rec_sentence);

protected:

    gboolean updateSpecialPhrases ();
    gboolean selectCandidate (guint i);
    gboolean selectCandidateInPage (guint i);
    gboolean resetCandidate (guint i);
    gboolean resetCandidateInPage (guint i);

    // David 
    gboolean updateLookupTableALL(void);
    
    gboolean updateLookupTableRecWords(void);
    gboolean updateLookupTableFastRecWords(void);
    
    gboolean updateLookupTableRecSentences(void);
    gboolean updateLookupTableFastRecSentences(void);
    
    void updateMultiLookupTables (guint update_type);
    
    gboolean selectCandidateRecWords (guint i);
    gboolean selectCandidateRecSentences (guint i);


    void commit (const gchar *str);

    /* inline functions */
    void updatePhraseEditor ()
    {
        m_phrase_editor.update (m_pinyin);
    }

    const gchar * textAfterPinyin () const
    {
        return (const gchar *)m_text + m_pinyin_len;
    }

    const gchar * textAfterPinyin (guint i) const
    {
        g_assert (i <= m_pinyin.size ());
        if ( G_UNLIKELY (i == 0))
            return m_text;
        i--;
        return (const gchar *)m_text + m_pinyin[i].begin + m_pinyin[i].len;
    }

    const gchar * textAfterCursor () const
    {
        return (const gchar *)m_text + m_cursor;
    }

    /* pure virtual functions */
    virtual gboolean insert (gint ch) = 0;
    virtual gboolean removeCharBefore (void) = 0;
    virtual gboolean removeCharAfter (void) = 0;
    virtual gboolean removeWordBefore (void) = 0;
    virtual gboolean removeWordAfter (void) = 0;
    virtual gboolean moveCursorLeft (void) = 0;
    virtual gboolean moveCursorRight (void) = 0;
    virtual gboolean moveCursorLeftByWord (void) = 0;
    virtual gboolean moveCursorRightByWord (void) = 0;
    virtual gboolean moveCursorToBegin (void) = 0;
    virtual gboolean moveCursorToEnd (void) = 0;
    virtual void commit (void) = 0;
    virtual void updateAuxiliaryText (void) = 0;
    virtual void updatePreeditText (void) = 0;

    /* varibles */
    PinyinArray                 m_pinyin;
    guint                       m_pinyin_len;
    String                      m_buffer;
    LookupTable                 m_lookup_table;
    LookupTable                 m_lookup_table_rec_words;
    LookupTable                 m_lookup_table_rec_sentences;
    PhraseEditor                m_phrase_editor;
    std::vector<std::string>    m_special_phrases;
    std::string                 m_selected_special_phrase;
   
    // David
    // use preedit text to get rec sentences. 
    // seg the preedit first 
    std::string                 m_preedit_text;

    //WordsInfo                   m_rec_words;

    RecSentence                 m_recSentence; //("./index_sentences");
    RecWord                     m_recWord; //("./index_sentences");
    DictionaryNew&              m_chSeg;
    std::vector <SentenceInfo>  m_recSentenceInfos;
    std::vector <WordInfo>      m_recWordInfos;
    std::vector <string>        m_wordsAfterSegInFoucesd;

    // David 
    // the indicator for focused area
    guint                       m_focused_area_indicator;
    gboolean                    m_need_reset_indicator;
    IBusText                    *m_focused_rec_word;
    IBusText                    *m_focused_rec_sentence;

    // For feedback
    long                        m_wordId4SearchRecWords;
    long                        m_wordId4SearchRecSentences;
    //Log                         m_log;

};
};

#endif
