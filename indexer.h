#ifndef INDEXER_H
#define INDEXER_H

#include "common.h"

/* Indexing loop: reads every document from load_collection(), splits it
   into words with next_token() and adds each word to the index.
   idx must already be set up with index_init().
   Fills in total_terms and unique_terms of docs[i] as well, so that
   docs[i] and idx->docs[i] describe the same document (id == position).
   Returns the number of documents indexed, or -1 if memory ran out. */
int build_index(Index *idx, Document *docs, int count);

#endif
