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
#ifndef __PY_DATABASE_H_
#define __PY_DATABASE_H_

#include "PYUtil.h"
#include "PYString.h"
#include "PYTypes.h"
#include "PYPhraseArray.h"
#include <map>

// David add
#define REC_TYPE_WORD       0
#define REC_TYPE_SENTENCE   1 

typedef struct sqlite3 sqlite3;

namespace PY {

class PinyinArray;
struct Phrase;

class SQLStmt;
typedef std::shared_ptr<SQLStmt> SQLStmtPtr;

class Database;

class Query {
public:
    Query (const PinyinArray    & pinyin,
           guint                  pinyin_begin,
           guint                  pinyin_len,
           guint                  option);
    ~Query (void);
    gint fill (PhraseArray &phrases, gint count);

private:
    const PinyinArray & m_pinyin;
    guint m_pinyin_begin;
    guint m_pinyin_len;
    guint m_option;
    SQLStmtPtr m_stmt;
};


class Database {
public:
    ~Database ();
protected:
    Database ();

public:
    SQLStmtPtr query (const PinyinArray   & pinyin,
                      guint                 pinyin_begin,
                      guint                 pinyin_len,
                      gint                  m,
                      guint                 option);
    void commit (const PhraseArray  & phrases);
    void remove (const Phrase & phrase);

    // Add feedback data to db
    void addFeedback (long origWordId, long recId, int type);
    // Query feedback data from db
    void qryFeedback (long origWordId, std::map<long,int> & idFreqs, int type);
    int qryFeedback (long origWordId, long recId, int type); // return freq

    void conditionsDouble (void);
    void conditionsTriple (void);

    static void init (void);
    static void finalize (void);
    static Database & instance (void) { return *m_instance; }

private:
    gboolean open (void);
    gboolean loadUserDB (void);
    gboolean saveUserDB (void);

    // For feedback
    gboolean loadFeedbackDB (void);
    gboolean saveFeedbackDB (void);

    void prefetch (void);
    void phraseSql (const Phrase & p, String & sql);
    void phraseWhereSql (const Phrase & p, String & sql);
    gboolean executeSQL (const gchar *sql, sqlite3 *db = NULL);
    void modified (void);
    void modified_feedback (void);
    static gboolean timeoutCallback (gpointer data);
    static gboolean timeoutCallback_feedback (gpointer data);

private:
    sqlite3 *m_db;              /* sqlite3 database */

    String m_sql;        /* sql stmt */
    String m_buffer;     /* temp buffer */
    guint m_timeout_id;
    guint m_timeout_id_feedback;
    GTimer *m_timer;
    GTimer *m_timer_feedback;

private:
    static std::unique_ptr<Database> m_instance;
};


};

#endif
