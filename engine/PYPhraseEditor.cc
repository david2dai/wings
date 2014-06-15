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
#include "PYPhraseEditor.h"
#include "PYConfig.h"
#include "PYDatabase.h"
#include "PYPinyinProperties.h"
#include "PYSimpTradConverter.h"


namespace PY {

PhraseEditor::PhraseEditor (PinyinProperties & props, Config & config)
    : m_candidates (32),
      m_selected_phrases (8),
      m_selected_string (32),
      m_candidate_0_phrases (8),
      m_pinyin (16),
      m_cursor (0),
      m_props (props),
      m_config (config)
      //m_log ("/home/david/David/tablog")
{
}

PhraseEditor::~PhraseEditor (void)
{
}

gboolean
PhraseEditor::update (const PinyinArray &pinyin)
{
    /* the size of pinyin must not bigger than MAX_PHRASE_LEN */
    g_assert (pinyin.size () <= MAX_PHRASE_LEN);

    m_pinyin = pinyin;
    m_cursor = 0;

    /* FIXME, should not remove all phrases1 */
    m_selected_phrases.clear ();
    m_selected_string.truncate (0);
    updateCandidates ();
    return TRUE;
}

gboolean
PhraseEditor::resetCandidate (guint i)
{
    Database::instance ().remove (m_candidates[i]);

    updateCandidates ();
    return TRUE;
}

void
PhraseEditor::commit (void)
{
    Database::instance ().commit (m_selected_phrases);
    reset ();
}

    // Add feedback data to db
void 
PhraseEditor::addFeedback (long origWordId, long recId, int type)
{

    Database::instance ().addFeedback (origWordId, recId, type);

}

// Query feedback data from db
void 
PhraseEditor::qryFeedback (long origWordId, std::map<long,int> & idFreqs, int type)
{

    Database::instance ().qryFeedback (origWordId, idFreqs, type);

}

gboolean
PhraseEditor::selectCandidate (guint i)
{
    /*
    PhraseArray m_candidates;           // candidates phrase array
    PhraseArray m_selected_phrases;     // selected phrases, before cursor
    String      m_selected_string;      // selected phrases, in string format
    PhraseArray m_candidate_0_phrases;  // the first candidate in phrase array format
    
    PinyinArray m_pinyin;   //seg pinyin array
    */ 

    /*
     Pinyin
     {  
        text        : "ang",
        bopomofo    : L"ㄤ",
        sheng       : "",
        yun         : "ang",
        pinyin_id   : {{ PINYIN_ID_ZERO, PINYIN_ID_ANG }, { PINYIN_ID_ZERO, PINYIN_ID_AN }, { PINYIN_ID_ZERO, PINYIN_ID_ZERO }},
        len         : 3,
        flags       : 0
    }
    */

    if (G_UNLIKELY (i >= m_candidates.size ()))
        return FALSE;

    // Select the first candidate of the UI
    if (G_LIKELY (i == 0)) {
        m_selected_phrases.insert (m_selected_phrases.end (),
                                   m_candidate_0_phrases.begin (),
                                   m_candidate_0_phrases.end ());

        if (G_LIKELY (m_props.modeSimp ()))
            m_selected_string << m_candidates[0].phrase;
        else
            SimpTradConverter::simpToTrad (m_candidates[0].phrase, m_selected_string);
        //m_log.logInfo ("[%s][%s]Selected is the FIRST: total select[%s]\n", __FILE__, __FUNCTION__, m_selected_string.c_str ());
        m_cursor = m_pinyin.size (); // complete pinyin counts 
    }
    else { // the selected candidate is not the first in the UI
        m_selected_phrases.push_back (m_candidates[i]);
        if (G_LIKELY (m_props.modeSimp ()))
            m_selected_string << m_candidates[i].phrase;
        else
            SimpTradConverter::simpToTrad (m_candidates[i].phrase, m_selected_string);
        
        //m_log.logInfo ("[%s][%s]Selected is not the FIRST: total select[%s]\n", __FILE__, __FUNCTION__, m_selected_string.c_str ());
        m_cursor += m_candidates[i].len;
    }

    updateCandidates ();
    return TRUE;
}

void
PhraseEditor::updateCandidates (void)
{
    m_candidates.clear ();
    m_query.reset ();
    updateTheFirstCandidate (); // use the left pinyin to get the fisrt candidate.

    if (G_UNLIKELY (m_pinyin.size () == 0))
        return;

    // the first candidate including several words
    if (G_LIKELY (m_candidate_0_phrases.size () > 1)) {
        Phrase phrase;
        phrase.reset ();
        for (guint i = 0; i < m_candidate_0_phrases.size (); i++)
            phrase += m_candidate_0_phrases[i];
        m_candidates.push_back (phrase); // put the first candidate at the front of the m_candidates
    }

    m_query.reset (new Query (m_pinyin,
                              m_cursor,
                              m_pinyin.size () - m_cursor,
                              m_config.option ()));
    fillCandidates ();
}

void
PhraseEditor::updateTheFirstCandidate (void)
{
    guint begin;
    guint end;

    m_candidate_0_phrases.clear ();

    if (G_UNLIKELY (m_pinyin.size () == 0))
        return;

    begin = m_cursor;
    end = m_pinyin.size ();

    while (begin != end) {
        gint ret;
        Query query (m_pinyin,
                     begin,
                     end - begin,
                     m_config.option ());
        ret = query.fill (m_candidate_0_phrases, 1);
        g_assert (ret == 1);
        begin += m_candidate_0_phrases.back ().len;
    }
}

gboolean
PhraseEditor::fillCandidates (void)
{
    if (G_UNLIKELY (m_query.get () == NULL)) {
        return FALSE;
    }

    gint ret = m_query->fill (m_candidates, FILL_GRAN);

    if (G_UNLIKELY (ret < FILL_GRAN)) {
        /* got all candidates from query */
        m_query.reset ();
    }

    return ret > 0 ? TRUE : FALSE;
}

};


