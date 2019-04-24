#include <string.h>

#ifndef LUNDGREN_DISTRIBUTED_QUERY
#define LUNDGREN_DISTRIBUTED_QUERY

// Flytt til egen fil:

struct L_Table {
  std::string name;
  std::string interim_name;
  std::vector<std::string> projections;
  std::vector<std::string> where_transitive_projections;
  std::vector<std::string> join_columns;
  std::vector<std::string> aliases;
};


struct L_Parser_info {
  std::vector<L_Table> tables;
  std::string where_clause;
};

//------------------------

#endif  // LUNDGREN_DISTRIBUTED_QUERY
