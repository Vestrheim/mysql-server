#include <string.h>
#include "plugin/lundgren/partitions/partition.h"
#include "plugin/lundgren/partitions/node.h"

#ifndef LUNDGREN_DISTRIBUTED_QUERY
#define LUNDGREN_DISTRIBUTED_QUERY

// Flytt til egen fil:

struct L_Table {
  std::string name;
  std::string interim_name;
  std::vector<std::string> projections;
  std::vector<std::string> where_transitive_projections;
};


struct L_Parser_info {
  std::vector<L_Table> tables;
  std::string where_clause;
};

//------------------------

// TODO:
// Ved å bruke generell interim target får vi nok fleksibilitet til å gjøre alle rene SQL strategier
// NB! for å bruke temp tables blir vi nødt til å holde connections i live, det er sikkert lurt uansett!
struct Interim_target {
    std::string interim_table_name;
    std::vector<Node> nodes;
    //bool is_temp;
};

struct Partition_query {
  std::string sql_statement;
  //std::string interim_table_name;
  Node node;
  Interim_target interim_target;
    // Usikker på om prep_statements blir nyttig
    //std::vector<std::string> prep_statements;
};

// TODO: bruk stages for å fikse multi-step strategier, slik som semi-join med 2 partisjonerte tabeller
struct Stage {
    std::vector<Partition_query> *partition_queries;
};

struct Distributed_query {
    std::string rewritten_query;
    //std::vector<Partition_query> *partition_queries;
    std::vector<Stage> *stages;
};

// TODO: Lag cleanup funksjon
// static void delete_distributed_query(Distributed_query *dq) {
  
// }

#endif  // LUNDGREN_DISTRIBUTED_QUERY
