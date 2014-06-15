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
#include "PYPhoneticEditor.h"
#include "PYConfig.h"
#include "PYPinyinProperties.h"
#include "PYSimpTradConverter.h"

#include <algorithm> 
#include <memory.h>

// for debug
//#include <stdio.h>
#include <stdlib.h>
//#include <sys/time.h>
//#include "log.h"
//#include "timelog.h"

using namespace std;

namespace PY {

const guint ORIG_WORDS_AREA    = 0;
const guint REC_WORDS_AREA     = 1;
const guint REC_SENTENCES_AREA = 2;

const guint UPDATE_TABLES_ORGWORD_RECWORD_RECSEN = 3;
const guint UPDATE_TABLES_RECWORD_RECSEN = 2;
const guint UPDATE_TABLES_RECSEN = 1;

struct WordInfoCmp {

    bool operator() (const WordInfo& a,const WordInfo& b) 
    {   
        return a.sim*a.posScore*(1.0+log(1.0 + a.freq)) > b.sim*b.posScore*(1.0+log(1.0 + b.freq));
    }   
};

struct SentenceInfoCmp {

    bool operator() (const SentenceInfo& a,const SentenceInfo& b) 
    {   
        return a.fTotalScore*(1.0+log(1.0 + a.freq)) > b.fTotalScore*(1.0+log(1.0 + b.freq)); 
    }   
};


/* init static members */
PhoneticEditor::PhoneticEditor (PinyinProperties & props, Config & config)
    : Editor (props, config),
      m_pinyin (MAX_PHRASE_LEN),
      m_pinyin_len (0),
      m_buffer (64),
      m_lookup_table (m_config.pageSize ()),
      m_lookup_table_rec_words (5),  // David need update
      m_lookup_table_rec_sentences (5),  // David need update
      m_phrase_editor (props, config),
      m_recSentence ("/usr/share/ibus-pinyin/resources/sentences_index",
                     "/usr/share/ibus-pinyin/resources/sentencesCorpusMain.bin", 
                     "/usr/share/ibus-pinyin/resources/sentencesCorpusOffset.bin", 
                     "/usr/share/ibus-pinyin/resources/wordsInfo4LDAInfer.bin", 
                     "/usr/share/ibus-pinyin/resources/wordsMapping4LDA.bin"), 
      m_recWord ("/usr/share/ibus-pinyin/resources/word2vec_dict"), //POS + Length=1 discount
      m_chSeg (DictionaryNew::GetInstance ("/usr/share/ibus-pinyin/resources/word2vec_dict")),
      m_need_reset_indicator (1),
      m_focused_area_indicator (0) // David
{

}

// David 
PhoneticEditor::~PhoneticEditor (void)
{

}

gboolean
PhoneticEditor::processSpace (guint keyval, guint keycode, guint modifiers)
{
    if (!m_text)
        return FALSE;
    if (cmshm_filter (modifiers) != 0)
        return TRUE;
   
    guint cursor_pos = 0;

    switch(m_focused_area_indicator)
    {
        case ORIG_WORDS_AREA:
            if (m_lookup_table.size () != 0) {
                cursor_pos = m_lookup_table.cursorPos ();
                selectCandidate (cursor_pos);
                return TRUE;
            }
            break;

        case REC_WORDS_AREA:
            if (m_lookup_table_rec_words.size () != 0) {
                cursor_pos = m_lookup_table_rec_words.cursorPos ();
                selectCandidateRecWords (cursor_pos);
                return TRUE;
            }
            break;
        
        case REC_SENTENCES_AREA:
            if (m_lookup_table_rec_sentences.size () != 0) {
                cursor_pos = m_lookup_table_rec_sentences.cursorPos ();
                selectCandidateRecSentences  (cursor_pos);
                return TRUE;
            }
            break;

        default:
            break;
    }

    /*
    if (m_lookup_table.size () != 0) {
        selectCandidate (m_lookup_table.cursorPos ());
    }
    else {
        commit ();
    }
    */

    commit ();
    return TRUE;
}

gboolean
PhoneticEditor::processFunctionKey (guint keyval, guint keycode, guint modifiers)
{
    if (m_text.empty ())
        return FALSE;

    /* ignore numlock */
    modifiers = cmshm_filter (modifiers);

    if (modifiers != 0 && modifiers != IBUS_CONTROL_MASK)
        return TRUE;

    /* process some cursor control keys */
    if (modifiers == 0) {
        switch (keyval) {
        case IBUS_Return:
        case IBUS_KP_Enter:
            commit ();
            return TRUE;

        case IBUS_BackSpace:
            if (m_phrase_editor.unselectCandidates ()) {
                update ();
            }
            else {
                removeCharBefore ();
            }
            return TRUE;

        case IBUS_Delete:
        case IBUS_KP_Delete:
            removeCharAfter ();
            return TRUE;

        case IBUS_Left:
        case IBUS_KP_Left:
            if (m_phrase_editor.unselectCandidates ()) {
                update ();
            }
            else {
                moveCursorLeft ();
            }
            return TRUE;

        case IBUS_Right:
        case IBUS_KP_Right:
            if (m_phrase_editor.unselectCandidates ()) {
                update ();
            }
            else {
                moveCursorRight ();
            }
            return TRUE;

        case IBUS_Home:
        case IBUS_KP_Home:
            if (m_phrase_editor.unselectCandidates ()) {
                update ();
            }
            else {
                moveCursorToBegin ();
            }
            return TRUE;

        case IBUS_End:
        case IBUS_KP_End:
            if (m_phrase_editor.unselectCandidates ()) {
                update ();
            }
            else {
                moveCursorToEnd ();
            }
            return TRUE;

        case IBUS_Up:
        case IBUS_KP_Up:
            cursorUp ();
            return TRUE;

        case IBUS_Down:
        case IBUS_KP_Down:
            cursorDown ();
            return TRUE;

        case IBUS_Page_Up:
        case IBUS_KP_Page_Up:
            pageUp ();
            return TRUE;

        case IBUS_Page_Down:
        case IBUS_KP_Page_Down:
        //case IBUS_Tab:
            pageDown ();
            return TRUE;

        // David 
        // add for update the focused area indicator
        case IBUS_Tab:
            updateFocusedAreaIndicator ();
            return TRUE;

        case IBUS_Escape:
            reset ();
            return TRUE;
        default:
            return TRUE;
        }
    }
    else {
        switch (keyval) {
        case IBUS_BackSpace:
            if (m_phrase_editor.unselectCandidates ()) {
                update ();
            }
            else {
                removeWordBefore ();
            }
            return TRUE;

        case IBUS_Delete:
        case IBUS_KP_Delete:
            removeWordAfter ();
            return TRUE;

        case IBUS_Left:
        case IBUS_KP_Left:
            if (m_phrase_editor.unselectCandidates ()) {
                update ();
            }
            else {
                moveCursorLeftByWord ();
            }
            return TRUE;

        case IBUS_Right:
        case IBUS_KP_Right:
            if (m_phrase_editor.unselectCandidates ()) {
                update ();
            }
            else {
                moveCursorToEnd ();
            }
            return TRUE;

        default:
            return TRUE;
        };
    }
    return TRUE;
}

gboolean
PhoneticEditor::processKeyEvent (guint keyval, guint keycode, guint modifiers)
{
    return FALSE;
}

gboolean
PhoneticEditor::updateSpecialPhrases (void)
{
    guint size = m_special_phrases.size ();
    m_special_phrases.clear ();

    if (!m_config.specialPhrases ())
        return FALSE;

    if (!m_selected_special_phrase.empty ())
        return FALSE;

    guint begin = m_phrase_editor.cursorInChar ();
    guint end = m_cursor;

    if (begin < end) {
        SpecialPhraseTable::instance ().lookup (
            m_text.substr (begin, m_cursor - begin),
            m_special_phrases);
    }

    return size != m_special_phrases.size () || size != 0;
}

gboolean
PhoneticEditor::updateLookupTableFastRecWords (void)
{
    //m_wordsAfterSegInFoucesd.clear ();
    Editor::updateLookupTableFastRecWords (m_lookup_table_rec_words, TRUE);
}

gboolean
PhoneticEditor::updateLookupTableRecWords (void)
{
    /* 
    TimeLog timeLog;
    timeLog.start (); 
    */

    guint cursor_pos = m_lookup_table.cursorPos ();
    IBusText * focusedText = m_lookup_table.getCandidate (cursor_pos);
    String focusedWord = Text(focusedText).text();
    
    m_wordsAfterSegInFoucesd.clear ();
    m_chSeg.segUTF8Str (focusedWord.c_str(), m_wordsAfterSegInFoucesd);

    //m_log.logInfo ("[%s][%s]OrgWords[%s] Seg:[", 
    //__FILE__, __FUNCTION__,focusedWord.c_str ());
   
    /* 
    for (int i=0;i<m_wordsAfterSegInFoucesd.size ();++i) {

        m_log.logInfo (" \"%s\"", m_wordsAfterSegInFoucesd[i].c_str ());
    
    }
    */

    //m_log.logInfo ("%s\n", "]SegEnd");

    if (m_wordsAfterSegInFoucesd.size ()<=0) {
        return FALSE;
    }

    m_recWordInfos.clear ();
    // no rec words
    //if (m_recWord.searchWords (focusedWord.c_str() , m_recWordInfos)   ==  -1)
    m_wordId4SearchRecWords = m_recWord.searchWords (m_wordsAfterSegInFoucesd.back().c_str() , m_recWordInfos);
    if (-1L==m_wordId4SearchRecWords)
    {
        return FALSE;
    }

    // Get feedback data from SQLite
    std::map<long,int> recWordFreqs;
    m_phrase_editor.qryFeedback(m_wordId4SearchRecWords, recWordFreqs, REC_TYPE_WORD);
   
    std::map<long,int>::iterator it; 
    for (int i=0;i<m_recWordInfos.size();++i) {

        it = recWordFreqs.find (m_recWordInfos[i].id);
        if (it == recWordFreqs.end () ) {
            continue;
        }

        m_recWordInfos[i].freq = it->second;
    }

    sort (m_recWordInfos.begin (), m_recWordInfos.end (), WordInfoCmp ());

    m_lookup_table_rec_words.clear ();

    //Log mylog ("/home/david/David/tablog");
    //mylog.logInfo ("Orig word:%s\n",m_wordsAfterSegInFoucesd.back().c_str ());
    
    for (guint i = 0;i < m_recWordInfos.size (); i++) {
        // m_rec_words.words[i]
        // m_rec_words.words_dis[i]
        
        Text text (m_recWordInfos[i].word);
       
        /*  
        if (i <20) {
            mylog.logInfo ("%s:", m_recWordInfos[i].word);
        }*/

        // 0x0000ef00  GREEN
        // text.appendAttribute (IBUS_ATTR_TYPE_FOREGROUND, 0x0000ef00, 0, -1);
        m_lookup_table_rec_words.appendCandidate (text);
    }

    //mylog.logInfo ("\n");
    Editor::updateLookupTableRecWords (m_lookup_table_rec_words, TRUE);

    //m_log.logInfo ("[%s][%s] word[%s] recwords:", 
    //__FILE__, __FUNCTION__,m_wordsAfterSegInFoucesd.back().c_str ());
 
    /* 
    for(int i=0;i<5;i++){

        m_log.logInfo ("%s %f\n",m_recWordInfos[i].word, m_recWordInfos[i].sim);
    } 
    */  

    /*
    char szMSG[1024] = {0};
    sprintf (szMSG, "WordRec[%s],Words cnt=[%d];",
            m_wordsAfterSegInFoucesd.back().c_str(), m_recWordInfos.size ());
    timeLog.end (szMSG);
    */
    return TRUE;
}

gboolean
PhoneticEditor::updateLookupTableFastRecSentences (void)
{
    Editor::updateLookupTableFastRecSentences (m_lookup_table_rec_sentences, TRUE);
}

gboolean
PhoneticEditor::updateLookupTableRecSentences (void)
{
    /* 
    TimeLog timeLog;
    timeLog.start (); 
    */ 
    guint cursor_pos = 0;
    IBusText * focusedText;

    /*
    cursor_pos = m_lookup_table.cursorPos ();
    focusedText = m_lookup_table.getCandidate (cursor_pos);
    String focusedOrigWord = Text(focusedText).text();
    */

    string wordsUsed2GetSentences ("");
    char szWord[256] = {0};
    m_wordId4SearchRecSentences = -1L;
    
    switch(m_focused_area_indicator)
    {
        case ORIG_WORDS_AREA:
            if (m_wordsAfterSegInFoucesd.size () == 0) {
                return FALSE;
            }

            for (int i=0;i<m_wordsAfterSegInFoucesd.size ();++i) {
                sprintf (szWord, "\"%s\" ", m_wordsAfterSegInFoucesd[i].c_str ());
                wordsUsed2GetSentences += szWord;
                memset (szWord, 0, sizeof (szWord));
            }
   
            if (m_wordsAfterSegInFoucesd.size () == 1) {
                m_wordId4SearchRecSentences = m_wordId4SearchRecWords;
            }        
            /* 
            m_log.logInfo ("[%s][%s]Use Orig Words to get sentences[%s]\n", 
                    __FILE__, 
                    __FUNCTION__, 
                    wordsUsed2GetSentences.c_str ());
            */
            break;
        case REC_WORDS_AREA:
            if (m_lookup_table_rec_words.size () == 0) {
                return FALSE;
            }

            cursor_pos = m_lookup_table_rec_words.cursorPos ();
            focusedText = m_lookup_table_rec_words.getCandidate (cursor_pos);
            sprintf (szWord, "\"%s\" ",Text(focusedText).text());
            wordsUsed2GetSentences += szWord;

            m_wordId4SearchRecSentences = m_recWordInfos[cursor_pos].id;
           
            /*
            m_log.logInfo ("[%s][%s]Use Rec Word to get sentences[%s]\n", 
                    __FILE__, 
                    __FUNCTION__, 
                    wordsUsed2GetSentences.c_str ());
            */ 
            break;
        case REC_SENTENCES_AREA:
            // Not need update
            return FALSE;
            break;
        default:
            break;
    }

    /*
    cursor_pos = m_lookup_table_rec_words.cursorPos ();
    focusedText = m_lookup_table_rec_words.getCandidate (cursor_pos);
    String focusedRecWord = Text(focusedText).text();
    */
    
    m_recSentenceInfos.clear();

    //if ( ! m_recSentence.searchSentencesAdvanced (focusedOrigWord.c_str(), focusedRecWord.c_str(),
    //m_recSentenceInfos) )
    if ( ! m_recSentence.searchSentencesAdvanced (wordsUsed2GetSentences.c_str (), " ", m_recSentenceInfos) )
    //if ( ! m_recSentence.searchSentencesAdvanced (focusedOrigWord.c_str(), "", m_recSentenceInfos) )
    {
        return FALSE;
    }

    if (m_recSentenceInfos.size () <=0) {
        return FALSE;
    }

    // Start
    // Get feedback data from SQLite
    std::map<long,int> recSentenceFreqs;
    m_phrase_editor.qryFeedback(m_wordId4SearchRecSentences, recSentenceFreqs, REC_TYPE_SENTENCE);
    
    std::map<long,int>::iterator it; 
    for (int i=0;i<m_recSentenceInfos.size();++i) {
        it = recSentenceFreqs.find (m_recSentenceInfos[i].id);
        if (it == recSentenceFreqs.end () ) {
            continue;
        }
        m_recSentenceInfos[i].freq = it->second;
    }
    sort (m_recSentenceInfos.begin (), m_recSentenceInfos.end (), SentenceInfoCmp ());

    // End
    m_lookup_table_rec_sentences.clear ();

    for (guint i = 0; i < m_recSentenceInfos.size() ; i++) {
        // m_rec_words.words[i]
        // m_rec_words.words_dis[i]
        Text text ( m_recSentenceInfos[i].szSentence );
        // 0x0000ef00  GREEN
        // text.appendAttribute (IBUS_ATTR_TYPE_FOREGROUND, 0x0000ef00, 0, -1);
        m_lookup_table_rec_sentences.appendCandidate (text);
    }

    Editor::updateLookupTableRecSentences (m_lookup_table_rec_sentences , TRUE);
    /*
    Log log ("/home/david/David/tablog");
    for(int i=0;i<200 && i < m_recSentenceInfos.size();i++){

        log.logInfo ("TotalScore[%f] LdaScore[%f] Qscore[%f] luceneScore[%f] sentence[%s] \n",
                m_recSentenceInfos[i].fTotalScore,
                m_recSentenceInfos[i].fLdaScore,
                m_recSentenceInfos[i].fQScore,
                m_recSentenceInfos[i].fLuceneScore,
                m_recSentenceInfos[i].szSentence);
    } 
    */
    /* 
    char szMSG[1024] = {0};
    sprintf (szMSG,"SentenceRec [%s],Sentences cnt=[%d]\n",
    wordsUsed2GetSentences.c_str (), m_recSentenceInfos.size ());
    timeLog.end (szMSG); 
    */
    return TRUE;
}

// David
// for Cursor up/down
// for Page up/down
void
PhoneticEditor::updateLookupTableFast (void)
{
    // David
    Editor::updateLookupTableFast (m_lookup_table, TRUE);
}

gboolean 
PhoneticEditor::updateLookupTableALL(void)
{

    Editor::updateLookupTable (m_lookup_table, TRUE);
    Editor::updateLookupTableRecWords (m_lookup_table_rec_words , TRUE);
    Editor::updateLookupTableRecSentences (m_lookup_table_rec_sentences , TRUE);

}

// David
// Update
void
PhoneticEditor::updateLookupTable (void)
{
    m_lookup_table.clear ();

    fillLookupTableByPage ();

    if (m_lookup_table.size ()) {
    
        Editor::updateLookupTable (m_lookup_table, TRUE);

        // move the following to update () function    
        //updateLookupTableRecWords ();
        //updateLookupTableRecSentences ();
    }
    else {
        hideLookupTable ();
        hideLookupTableRecWords ();
        hideLookupTableRecSentences ();
    }
}

gboolean
PhoneticEditor::fillLookupTableByPage (void)
{
    if (!m_selected_special_phrase.empty ()) {
        return FALSE;
    }

    guint filled_nr = m_lookup_table.size ();
    guint page_size = m_lookup_table.pageSize ();

    if (m_special_phrases.size () + m_phrase_editor.candidates ().size () < filled_nr + page_size)
        m_phrase_editor.fillCandidates ();

    guint need_nr = MIN (page_size, m_special_phrases.size () + m_phrase_editor.candidates ().size () - filled_nr);
    g_assert (need_nr >= 0);
    if (need_nr == 0) {
        return FALSE;
    }

    for (guint i = filled_nr; i < filled_nr + need_nr; i++) {
        if (i < m_special_phrases.size ()) {
            Text text (m_special_phrases[i].c_str ());
            text.appendAttribute (IBUS_ATTR_TYPE_FOREGROUND, 0x0000ef00, 0, -1);
            m_lookup_table.appendCandidate (text);
        }
        else {
            // if candidate is user phrase, set the color to blue
            if (G_LIKELY (m_props.modeSimp ())) {
                Text text (m_phrase_editor.candidate (i - m_special_phrases.size ()));
                if (m_phrase_editor.candidateIsUserPhease (i - m_special_phrases.size ()))
                    text.appendAttribute (IBUS_ATTR_TYPE_FOREGROUND, 0x000000ef, 0, -1);
                m_lookup_table.appendCandidate (text);
            }
            else {
                m_buffer.truncate (0);
                SimpTradConverter::simpToTrad (m_phrase_editor.candidate (i - m_special_phrases.size ()), m_buffer);
                Text text (m_buffer);
                if (m_phrase_editor.candidateIsUserPhease (i - m_special_phrases.size ()))
                    text.appendAttribute (IBUS_ATTR_TYPE_FOREGROUND, 0x000000ef, 0, -1);
                m_lookup_table.appendCandidate (text);
            }
        }
    }

    return TRUE;
}

// David
// for Cursor up/down
// for Page up/down
void
PhoneticEditor::updateMultiLookupTables (guint update_type)
{
    switch (update_type){
        case UPDATE_TABLES_ORGWORD_RECWORD_RECSEN:
            updateLookupTableFast ();
            updatePreeditText ();
            updateAuxiliaryText ();
             
            updateLookupTableRecWords ();
            updateLookupTableRecSentences ();
            break;
        case UPDATE_TABLES_RECWORD_RECSEN:
            updateLookupTableFastRecWords ();
            updateLookupTableRecSentences ();
            //updatePreeditTextRecWords ();
            //updateAuxiliaryTextRecWords ();
            break;
        case UPDATE_TABLES_RECSEN:
            updateLookupTableFastRecSentences ();
            //updatePreeditTextRecSentences ();
            //updateAuxiliaryTextRecSentences ();
            break;
        default:
            break;
    }
}

void
PhoneticEditor::pageUp (void)
{
    switch(m_focused_area_indicator)
    {
        case ORIG_WORDS_AREA:
            /*
            m_log.logInfo ("[%s][%s]:pageUp ORIG_WORDS_AREA[%d]\n", 
                    __FILE__, 
                    __FUNCTION__, 
                    m_focused_area_indicator);
            */
            if (G_LIKELY (m_lookup_table.pageUp ())) {

                updateMultiLookupTables (UPDATE_TABLES_ORGWORD_RECWORD_RECSEN);

            }
            break;
        case REC_WORDS_AREA:
            if (G_LIKELY (m_lookup_table_rec_words.pageUp ())) {

                updateMultiLookupTables (UPDATE_TABLES_RECWORD_RECSEN);

            }
            /* 
            m_log.logInfo ("[%s][%s]:pageUp REC_WORDS_AREA[%d]\n", 
                    __FILE__, 
                    __FUNCTION__, 
                    m_focused_area_indicator);
            */
            break;
        case REC_SENTENCES_AREA:
            if (G_LIKELY (m_lookup_table_rec_sentences.pageUp ())) {

                updateMultiLookupTables (UPDATE_TABLES_RECSEN);

            }
            /*
            m_log.logInfo ("[%s][%s]:pageUp REC_SENTENCES_AREA[%d]\n", 
                    __FILE__, 
                    __FUNCTION__, 
                    m_focused_area_indicator);
            */
            break;
        default:
            break;
    }

}

void
PhoneticEditor::pageDown (void)
{
    switch(m_focused_area_indicator)
    {
        case ORIG_WORDS_AREA:
            if (G_LIKELY(
                (m_lookup_table.pageDown ()) ||
                (fillLookupTableByPage () && m_lookup_table.pageDown ()))) {
        
                updateMultiLookupTables (UPDATE_TABLES_ORGWORD_RECWORD_RECSEN);
            }
            /*
            m_log.logInfo ("[%s][%s]:pageDown ORIG_WORDS_AREA[%d]\n", 
                    __FILE__, 
                    __FUNCTION__, 
                    m_focused_area_indicator);
            */
            break;
        case REC_WORDS_AREA:
            if (G_LIKELY (m_lookup_table_rec_words.pageDown ())) {

                updateMultiLookupTables (UPDATE_TABLES_RECWORD_RECSEN);

            }
            /* 
            m_log.logInfo ("[%s][%s]:pageDown REC_WORDS_AREA[%d]\n", 
                    __FILE__, 
                    __FUNCTION__, 
                    m_focused_area_indicator);
            */
            break;
        case REC_SENTENCES_AREA:
            if (G_LIKELY (m_lookup_table_rec_sentences.pageDown ())) {

                updateMultiLookupTables (UPDATE_TABLES_RECSEN);

            }
            /* 
            m_log.logInfo ("[%s][%s]:pageDown REC_SENTENCES_AREA[%d]\n", 
                    __FILE__, 
                    __FUNCTION__, 
                    m_focused_area_indicator);
            */
            break;
        default:
            break;
    }
}

// David
// for updating the focused area
void 
PhoneticEditor::updateFocusedAreaIndicator(void)
{
    m_focused_area_indicator = (m_focused_area_indicator + 1) % 3;
    Editor::updateFocusedAreaIndicator (m_focused_area_indicator);

    // David need update here.
    // switch case
    // if indicator == 0, updatelookuptable
    // if indicator == 1, updatelookuptable recwords
    // if indicator == 2, updatelookuptabel recsentences
    // otherwise. after using tab too change the indicator, ui show the result of last indicator
   
    switch(m_focused_area_indicator)
    {
        case ORIG_WORDS_AREA:
            m_lookup_table.setCursorPos (0);
            updateMultiLookupTables (UPDATE_TABLES_ORGWORD_RECWORD_RECSEN);
            /*
            m_log.logInfo ("[%s][%s]:ORIG_WORDS_AREA[%d]\n", 
                    __FILE__, 
                    __FUNCTION__, 
                    m_focused_area_indicator);
            */
            break;
        case REC_WORDS_AREA:
            m_lookup_table_rec_words.setCursorPos (0);
            updateMultiLookupTables (UPDATE_TABLES_RECWORD_RECSEN);
            /*
            m_log.logInfo ("[%s][%s]:REC_WORDS_AREA[%d]\n", 
                    __FILE__, 
                    __FUNCTION__, 
                    m_focused_area_indicator);
            */
            break;
        case REC_SENTENCES_AREA:
            m_lookup_table_rec_sentences.setCursorPos (0);
            updateMultiLookupTables (UPDATE_TABLES_RECSEN);
            /*
            m_log.logInfo ("[%s][%s]:REC_SENTENCES_AREA[%d]\n", 
                    __FILE__, 
                    __FUNCTION__, 
                    m_focused_area_indicator);
            */
            break;
        default:
            break;
    }
}

void 
PhoneticEditor::pageUpRecWords  (void) 
{
    if (G_LIKELY (m_lookup_table_rec_words.pageUp ())) {

        updateMultiLookupTables (UPDATE_TABLES_RECWORD_RECSEN);
    }
}

void
PhoneticEditor::pageDownRecWords  (void)
{
    if (G_LIKELY (m_lookup_table_rec_words.pageDown ())) {

        updateMultiLookupTables (UPDATE_TABLES_RECWORD_RECSEN);
    }
}

void
PhoneticEditor::cursorUpRecWords  (void)
{
    if (G_LIKELY (m_lookup_table_rec_words.cursorUp ())) {

        updateMultiLookupTables (UPDATE_TABLES_RECWORD_RECSEN);
    }
}

void
PhoneticEditor::cursorDownRecWords  (void)
{
    if (G_LIKELY (m_lookup_table_rec_words.cursorDown ())) {

        updateMultiLookupTables (UPDATE_TABLES_RECWORD_RECSEN);
    }
}

//Sentences
void 
PhoneticEditor::pageUpRecSentences (void)
{
    if (G_LIKELY (m_lookup_table_rec_sentences.pageUp ())) {

        updateMultiLookupTables (UPDATE_TABLES_RECSEN);
    }
}

void 
PhoneticEditor::pageDownRecSentences (void)
{
    if (G_LIKELY (m_lookup_table_rec_sentences.pageDown ())) {

        updateMultiLookupTables (UPDATE_TABLES_RECSEN);
    }
}

void 
PhoneticEditor::cursorUpRecSentences (void)
{
    if (G_LIKELY (m_lookup_table_rec_sentences.cursorUp ())) {

        updateMultiLookupTables (UPDATE_TABLES_RECSEN);
    }    
}

void 
PhoneticEditor::cursorDownRecSentences (void)
{
    if (G_LIKELY (m_lookup_table_rec_sentences.cursorDown())) {

        updateMultiLookupTables (UPDATE_TABLES_RECSEN);
    }

}

void
PhoneticEditor::cursorUp (void)
{
    switch(m_focused_area_indicator)
    {
        case ORIG_WORDS_AREA:
            if (G_LIKELY (m_lookup_table.cursorUp ())) {
                
                updateMultiLookupTables (UPDATE_TABLES_ORGWORD_RECWORD_RECSEN);

            }
            /*
            m_log.logInfo ("[%s][%s]:cursorUp ORIG_WORDS_AREA[%d]\n", 
                    __FILE__, 
                    __FUNCTION__, 
                    m_focused_area_indicator);
            */
            break;

        case REC_WORDS_AREA:
            if (G_LIKELY (m_lookup_table_rec_words.cursorUp ())) {

                updateMultiLookupTables (UPDATE_TABLES_RECWORD_RECSEN);

            }
            /* 
            m_log.logInfo ("[%s][%s]:cursorUp REC_WORDS_AREA[%d]\n", 
                    __FILE__, 
                    __FUNCTION__, 
                    m_focused_area_indicator);
            */
            break;
        case REC_SENTENCES_AREA:
            if (G_LIKELY (m_lookup_table_rec_sentences.cursorUp ())) {

                updateMultiLookupTables (UPDATE_TABLES_RECSEN);

            }    
            /* 
            m_log.logInfo ("[%s][%s]:cursorUp REC_SENTENCES_AREA[%d]\n", 
                    __FILE__, 
                    __FUNCTION__, 
                    m_focused_area_indicator);
            */
            break;
        default:
            break;
    }

}

void
PhoneticEditor::cursorDown (void)
{
    switch(m_focused_area_indicator)
    {
        case ORIG_WORDS_AREA:
            /* 
            m_log.logInfo ("[%s][%s]:cursorDown ORIG_WORDS_AREA[%d]\n", 
                    __FILE__, 
                    __FUNCTION__, 
                    m_focused_area_indicator);
            */

            if (G_LIKELY (
                (m_lookup_table.cursorPos () == m_lookup_table.size () - 1) &&
                (fillLookupTableByPage () == FALSE))) {
                return;
            }
            if (G_LIKELY (m_lookup_table.cursorDown ())) {

                updateMultiLookupTables (UPDATE_TABLES_ORGWORD_RECWORD_RECSEN);

            }
            break;
        case REC_WORDS_AREA:
            if (G_LIKELY (m_lookup_table_rec_words.cursorDown())) {

                updateMultiLookupTables (UPDATE_TABLES_RECWORD_RECSEN);

            }
    
            /* 
            m_log.logInfo ("[%s][%s]:cursorDown REC_WORDS_AREA[%d]\n", 
                    __FILE__, 
                    __FUNCTION__, 
                    m_focused_area_indicator);
            */
            break;
        case REC_SENTENCES_AREA:
            if (G_LIKELY (m_lookup_table_rec_sentences.cursorDown())) {

                updateMultiLookupTables (UPDATE_TABLES_RECSEN);
            }
    
            /* 
            m_log.logInfo ("[%s][%s]:cursorDown REC_SENTENCES_AREA[%d]\n", 
                    __FILE__, 
                    __FUNCTION__, 
                    m_focused_area_indicator);
            */
   
            break;
        default:
            break;
    }
}

void
PhoneticEditor::candidateClicked (guint index, guint button, guint state)
{
   selectCandidateInPage (index);
}

void
PhoneticEditor::candidateClickedRecWords (guint index, guint button, guint state)
{
    // David
    // Need do something here
    // selectCandidateInPage (index);
    // guint cursor_pos = m_lookup_table_rec_words.cursorPos ();

    guint page_size = m_lookup_table_rec_words.pageSize ();
    guint cursor_pos = m_lookup_table_rec_words.cursorPos ();

    if (G_UNLIKELY (index >= page_size))
        return ;
    index += (cursor_pos / page_size) * page_size;

    selectCandidateRecWords (index); // David add 2014.01.06
    IBusText * focusedText = m_lookup_table_rec_words.getCandidate ( index );
    String focusedWord = Text(focusedText).text();
 
}

void
PhoneticEditor::candidateClickedRecSentences (guint index, guint button, guint state)
{
    // David
    // Need do something here
    // selectCandidateInPage (index);
    guint page_size = m_lookup_table_rec_sentences.pageSize ();
    guint cursor_pos = m_lookup_table_rec_sentences.cursorPos ();

    if (G_UNLIKELY (index >= page_size))
        return ;
    index += (cursor_pos / page_size) * page_size;

    selectCandidateRecSentences (index); // David add 2014.01.06

}

void
PhoneticEditor::reset (void)
{
    m_pinyin.clear ();
    m_pinyin_len = 0;
    m_lookup_table.clear ();  // need clear lookup_table for recwords and recsentences ??
    m_phrase_editor.reset ();
    m_special_phrases.clear ();
    m_selected_special_phrase.clear ();

    Editor::reset ();
}

void
PhoneticEditor::update (void)
{
    
    if (m_focused_area_indicator && m_need_reset_indicator ) {

        //m_log.logInfo ("[%s][%s]indicator changed from[%d]to 0\n", 
        //__FILE__, __FUNCTION__, m_focused_area_indicator);
        m_focused_area_indicator = 0;
        Editor::updateFocusedAreaIndicator (m_focused_area_indicator);

    }
  
    /* 
    TimeLog timeLog;
    timeLog.start (); 
    */
    // cal time for one update
    updateLookupTable ();
    updatePreeditText ();
    updateAuxiliaryText ();

    if (m_lookup_table.size ()) {
    
        // moved from updateLookupTable ()
        updateLookupTableRecWords ();
        updateLookupTableRecSentences ();
    }
    
    //timeLog.end ("A recommendation Finish!");
    // David
    // or in reset the flag to 0 in the commit function
   
    /* 
    if (0 == m_focused_area_indicator || 0 == m_need_reset_indicator) {
        return;
    } else {
        m_log.logInfo ("[%s][%s]indicator changed from[%d]to 0\n", 
        __FILE__, __FUNCTION__, m_focused_area_indicator);
        m_focused_area_indicator = 0;
        Editor::updateFocusedAreaIndicator (m_focused_area_indicator);
    }
    */
    // updateFocusedAreaIndicator ();
}

void
PhoneticEditor::commit (const gchar *str)
{
    // David 
    // I am going to update commit rec word here
    StaticText text(str);
    commitText (text);

    // Todo
    /*
    switch(m_focused_area_indicator)
    {
        case ORIG_WORDS_AREA:
            break;
        case REC_WORDS_AREA:
            break;
        case REC_SENTENCES_AREA:
            break;
        default:
            break;
    }
    */

    m_recSentence.addWord2LocalContext (str);
    m_recWord.addWord2LocalContext (str);

    //m_wordsAfterSegInFoucesd.clear ();
    std::vector<std::string> words; 
    m_chSeg.segUTF8Str (str, words);

    //m_log.logInfo ("[%s][%s]indicator[%d]commit[%s]\n",
    //__FILE__, __FUNCTION__, m_focused_area_indicator, str);
    //m_focused_area_indicator = 0;
    //m_log.logInfo ("[%s][%s]commit[%s]\n", 
    //__FILE__, __FUNCTION__, str);
}

// David
void
PhoneticEditor::focusedRecWord (IBusText *sel_rec_word)
{
    // David
    // can do as the ibusengine.c ibus_engine_set_surrounding_text do
    // define a IBusText * m_focused_rec_word  in the class
    // and here do: 
    if (m_focused_rec_word) 
    {
        g_object_unref (m_focused_rec_word);
    }  
    
    m_focused_rec_word = (IBusText *) g_object_ref_sink (sel_rec_word ? sel_rec_word : NULL );
    // to replace the following
    // m_focused_rec_word = sel_rec_word->text();
}

void
PhoneticEditor::focusedRecSentence (IBusText *sel_rec_sentence)
{
    if (m_focused_rec_sentence) 
    {
        g_object_unref (m_focused_rec_sentence);
    }  
    
    m_focused_rec_sentence = (IBusText *) g_object_ref_sink (sel_rec_sentence ? sel_rec_sentence : NULL );
    // m_focused_rec_sentence = sel_rec_sentence->text();
}
gboolean 
PhoneticEditor::selectCandidateRecWords (guint i) 
{
    // guint cursor_pos = m_lookup_table_rec_words.cursorPos ();
    IBusText * focusedText = m_lookup_table_rec_words.getCandidate (i);
    std::string focusedRecWord = Text(focusedText).text();

    long selectedRecWordId = m_recWordInfos[i].id;

    // m_text is a String
    if (m_cursor == m_text.size ()) {  
        // all the pinyin get the responding phrase, need commit to wigdet and DB
        m_buffer = m_phrase_editor.selectedString ();
        m_buffer << focusedRecWord;
        //m_phrase_editor.commit ();  // David: add the new phrase to the DB

        // Feedback
        if (m_wordId4SearchRecWords !=-1) {
            m_phrase_editor.addFeedback (m_wordId4SearchRecWords, selectedRecWordId, REC_TYPE_WORD);
        }

        // Not good code
        m_need_reset_indicator = 0;
        reset (); // will set m_focused_area_indicator = 0 in PYEditor.cc
        m_need_reset_indicator = 1;
        commit ((const gchar *)m_buffer);

    }
    else { 
        // part pinyin get the phrase
        //updateSpecialPhrases ();  // use the left pinyin to get Special Phrases
        update ();
    }

    return TRUE;

}

gboolean 
PhoneticEditor::selectCandidateRecSentences (guint i)
{
    // guint cursor_pos = m_lookup_table_rec_words.cursorPos ();
    IBusText * focusedText = m_lookup_table_rec_sentences.getCandidate (i);
    std::string focusedRecSentence = Text(focusedText).text();

    long selectedRecSentenceId = m_recSentenceInfos[i].id;

    // m_text is a String
    if (m_cursor == m_text.size ()) {  
        // all the pinyin get the responding phrase, need commit to wigdet and DB
        m_buffer = m_phrase_editor.selectedString ();
        m_buffer << focusedRecSentence;
        //m_phrase_editor.commit ();  // David: add the new phrase to the DB

        // Feedback
        if (m_wordId4SearchRecSentences !=-1) {
            m_phrase_editor.addFeedback (m_wordId4SearchRecSentences, selectedRecSentenceId, REC_TYPE_SENTENCE);
        }

        // Not good code
        m_need_reset_indicator = 0;
        reset (); // will set m_focused_area_indicator = 0 in PYEditor.cc
        m_need_reset_indicator = 1;
        commit ((const gchar *)m_buffer);

    }
    else { 
        // part pinyin get the phrase
        //updateSpecialPhrases ();  // use the left pinyin to get Special Phrases
        update ();
    }

    return TRUE;
}

gboolean
PhoneticEditor::selectCandidate (guint i)
{
    // check if the i is in the range of m_special_phrases.size
    if (i < m_special_phrases.size ()) {
        /* select a special phrase */
        /*
        m_log.logInfo ("[%s][%s]PATH 1, select a special phrase:[%s], m_cursor=[%d] m_text=[%s,size=[%d]]\n", 
                __FILE__, 
                __FUNCTION__,
                m_special_phrases[i].c_str (),
                m_cursor,
                m_text.c_str (),
                m_text.size () );
        */

        m_selected_special_phrase = m_special_phrases[i];

        // m_text is a String
        if (m_cursor == m_text.size ()) {  
            // all the pinyin get the responding phrase, need commit to wigdet and DB
            m_buffer = m_phrase_editor.selectedString ();
            m_buffer << m_selected_special_phrase;
            m_phrase_editor.commit ();  // David: add the new phrase to the DB
            
            // Not good code
            m_need_reset_indicator = 0;
            reset (); // will set m_focused_area_indicator = 0 in PYEditor.cc
            m_need_reset_indicator = 1;
            commit ((const gchar *)m_buffer);

        }
        else { 
            // part pinyin get the phrase
            updateSpecialPhrases ();  // use the left pinyin to get Special Phrases
            update ();
        }
        return TRUE;
    }

    // Selected candidate is not A special phrase
    // calculate its index in the m_phrase_editor
    i -= m_special_phrases.size ();

    if (m_phrase_editor.selectCandidate (i)) {


        if (m_phrase_editor.pinyinExistsAfterCursor () ||
            *textAfterPinyin () != '\0') {
            /* 
            m_log.logInfo ("[%s][%s]PATH 2.1, selectCandidate\n", 
                    __FILE__, 
                    __FUNCTION__);
            */

            // More pinyin exist after cursor
            updateSpecialPhrases ();
            update ();
        }
        else {
            commit ();  // David: a virtual fun. reloaded in the sub class PinyinEditor
            /*
            m_log.logInfo ("[%s][%s]PATH 2.1, selectCandidate\n", 
                    __FILE__, 
                    __FUNCTION__);
            */

        }
        
        return TRUE;
    }
    
    //m_log.logInfo ("[%s][%s]PATH 3\n", __FILE__, __FUNCTION__);
    return FALSE;
}

// When click the lable(i) in the UI.
// here will select the clicked candidates
gboolean
PhoneticEditor::selectCandidateInPage (guint i)
{
    //guint page_size = m_lookup_table.pageSize ();
    //guint cursor_pos = m_lookup_table.cursorPos ();
    guint page_size = 0;
    guint cursor_pos = 0;

    switch(m_focused_area_indicator)
    {
        case ORIG_WORDS_AREA:
            page_size = m_lookup_table.pageSize ();
            cursor_pos = m_lookup_table.cursorPos ();
            
            if (G_UNLIKELY (i >= page_size))
                return FALSE;

            i += (cursor_pos / page_size) * page_size;
            return selectCandidate (i);
        case REC_WORDS_AREA:
            page_size = m_lookup_table_rec_words.pageSize ();
            cursor_pos = m_lookup_table_rec_words.cursorPos ();

            if (G_UNLIKELY (i >= page_size))
                return FALSE;

            i += (cursor_pos / page_size) * page_size;
            return selectCandidateRecWords (i);
        case REC_SENTENCES_AREA:
            page_size = m_lookup_table_rec_sentences.pageSize ();
            cursor_pos = m_lookup_table_rec_sentences.cursorPos ();

            if (G_UNLIKELY (i >= page_size))
                return FALSE;

            i += (cursor_pos / page_size) * page_size;
            return selectCandidateRecSentences (i);
        default:
            page_size = 0; 
            cursor_pos = 0; 
            break;
    }
    return FALSE;
    /*
    if (G_UNLIKELY (i >= page_size))
        return FALSE;

    i += (cursor_pos / page_size) * page_size;

    return selectCandidate (i);
    */
}

gboolean
PhoneticEditor::resetCandidate (guint i)
{
    i -= m_special_phrases.size ();
    if (m_phrase_editor.resetCandidate (i)) {
        update ();
    }
    return TRUE;
}

gboolean
PhoneticEditor::resetCandidateInPage (guint i)
{
    guint page_size = m_lookup_table.pageSize ();
    guint cursor_pos = m_lookup_table.cursorPos ();
    i += (cursor_pos / page_size) * page_size;

    return resetCandidate (i);
}

};

