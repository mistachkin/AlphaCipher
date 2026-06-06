
/*****************************************************************************\
 * WARNING: These values are optimized for performance per D. Richard Hipp's *
 *          suggestions.                                                     *
 *                                                                           *
 *          PLEASE DO NOT CHANGE THEM WITHOUT FIRST CONSULTING AN EXPERT.    *
\*****************************************************************************/

PRAGMA page_size = 16384;
PRAGMA auto_vacuum = INCREMENTAL;
PRAGMA foreign_keys = ON;
PRAGMA journal_mode = WAL;

CREATE TABLE Chunks(
  Id INTEGER PRIMARY KEY NOT NULL,
  Data BLOB NOT NULL
);
