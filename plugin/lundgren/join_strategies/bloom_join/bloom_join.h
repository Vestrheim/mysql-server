#include <string.h>
#include "plugin/lundgren/distributed_query.h"
#include "plugin/lundgren/helpers.h"
#include "plugin/lundgren/partitions/partition.h"
#include "plugin/lundgren/join_strategies/common.h"
#include "plugin/lundgren/join_strategies/semi_join.h"
#include "plugin/lundgren/constants.h"
#include "plugin/lundgren/join_strategies/bloom_join/bloom_join_executor.h"

#ifndef LUNDGREN_BLOOM_JOIN
#define LUNDGREN_BLOOM_JOIN

#define BLOOM_SLAVE_FLAG "bloom_slave"

// Bloom join

// n = 1
static Distributed_query *make_one_sided_bloom_join_distributed_query(L_Parser_info *parser_info MY_ATTRIBUTE((unused)), L_Table* stationary_table, L_Table* remote_table, std::vector<Partition>* remote_partitions) {

  //std::vector<L_Table> *tables = &(parser_info->tables);
  std::string where_clause = parser_info->where_clause;

  std::vector<Stage> stages;

  stationary_table->interim_name = generate_interim_name();
  remote_table->interim_name = generate_interim_name();

  std::string stationary_join_column = stationary_table->where_transitive_projections[0];
  std::string remote_join_column = remote_table->where_transitive_projections[0];


  std::string join_column_projection_query = "SELECT " + stationary_join_column + " FROM " + stationary_table->name;

  std::string bloom_filter_base64 = generate_bloom_filter_from_query(join_column_projection_query);

  // // // STAGE 1
  // Stage stage1;

  // std::string join_column_projection_query_string = "SELECT DISTINCT " + stationary_join_column + " FROM " + stationary_table->name;

  // std::vector<Node> target_nodes;

  // for (auto &p : *remote_partitions) {
  //   target_nodes.push_back(p.node);
  // }
  
  // Interim_target interim_target = {stationary_table->interim_name, target_nodes};
  // Partition_query pq = {join_column_projection_query_string, Node(true), interim_target};

  // stage1.partition_queries.push_back(pq);
  // stages.push_back(stage1);

  // STAGE 2
  Stage stage2;

  std::string bloom_join_query_string = "/*distributed<join_strategy=bloom,";
  
  bloom_join_query_string += BLOOM_SLAVE_FLAG "=true,";
  bloom_join_query_string += "interim_name=" + stationary_table->interim_name + ",";
  bloom_join_query_string += "bloom_filter=" + bloom_filter_base64 + ">*/";

  bloom_join_query_string += "SELECT ";

  std::vector<std::string>::iterator p = remote_table->projections.begin();

  while (p != remote_table->projections.end()) {
    bloom_join_query_string += remote_table->name + "." + *p;
    ++p;
    if (p != remote_table->projections.end()) bloom_join_query_string += ", ";
  }
  if (remote_table->where_transitive_projections.size() > 0)
    bloom_join_query_string += ", ";
  p = remote_table->where_transitive_projections.begin();
  while (p != remote_table->where_transitive_projections.end()) {
    bloom_join_query_string += remote_table->name + "." + *p;
    ++p;
    if (p != remote_table->where_transitive_projections.end())
      bloom_join_query_string += ", ";
  }

  bloom_join_query_string += " FROM " + remote_table->name +
                            " JOIN " + stationary_table->interim_name +
                            " ON " + stationary_table->interim_name + "." + stationary_join_column +
                            " = " + remote_table->name + "." + remote_join_column;

  std::vector<Node> stage2_target_nodes{Node(true)}; //vector of self-node
  Interim_target stage2_interim_target = {remote_table->interim_name, stage2_target_nodes};

  for (auto &p : *remote_partitions) {
    Partition_query pq = {bloom_join_query_string, p.node, stage2_interim_target};
    stage2.partition_queries.push_back(pq);
  }

  stages.push_back(stage2);

  // Construct distributed query object
  Distributed_query *dq = new Distributed_query{semi_join_generate_final_join_query_string(stationary_table, remote_table, where_clause), stages};

  delete remote_partitions;

  return dq;

  // TODO: remember delete remote_partitions
}

// n > 1
static Distributed_query *make_recursive_bloom_join_distributed_query(L_Parser_info *parser_info MY_ATTRIBUTE((unused)), L_Table* remote_table, std::vector<Partition> *remote_partitions) {

  std::vector<Stage> stages;

  Stage stage1;

  std::string join_union_interim_table_name = generate_interim_name();

  //Distributed Partition queries
  std::string recursive_distributed_join_query_string = "/*distributed<join_strategy=bloom," IGNORE_TABLE_PARTITIONS_FLAG "=";
  recursive_distributed_join_query_string += remote_table->name + ">*/";

  recursive_distributed_join_query_string += generate_join_query_string(parser_info->tables, parser_info->where_clause, false);

  std::vector<Node> target_nodes;
  target_nodes.push_back(Node(true));
  Interim_target interim_target = {join_union_interim_table_name , target_nodes};

  for (auto &p : *remote_partitions) {
    Partition_query pq = {recursive_distributed_join_query_string, p.node, interim_target};
    stage1.partition_queries.push_back(pq);
  }
  
  stages.push_back(stage1);

  // Final query

  std::string final_query_string = "SELECT * FROM " + join_union_interim_table_name;

  // Construct distributed query object

  Distributed_query *dq = new Distributed_query{final_query_string, stages};

  delete remote_partitions;

  return dq;
  // remember delete remote_partitions
}

static bool is_bloom_slave(L_parsed_comment_args parsed_args) {
  return parsed_args.comment_args_lookup_table[BLOOM_SLAVE_FLAG] == "true";
}


static Distributed_query *make_bloom_join_distributed_query(L_Parser_info *parser_info, L_parsed_comment_args parsed_args) {

  if (!is_bloom_slave(parsed_args)) {

    // -----------------------------------------------------
    std::vector<L_Table> *tables = &(parser_info->tables);
    std::string where_clause = parser_info->where_clause;
    // ------------------------------------------------------

    std::vector<Stage> stages;

    L_Table* stationary_table;
    L_Table* remote_table;
    std::vector<Partition>* remote_partitions;
    bool has_stationary_table = false;
    unsigned int biggest_partition_count = 0;

    for (auto &table : *tables) {

      std::vector<Partition> *partitions =
          get_partitions_by_table_name(table.name);

      if (partitions == NULL) {
        return NULL;
      }

      // Choose table with one partition, or ignore flag
      if (!has_stationary_table && (partitions->size() == 1 || has_ignore_partitions_arg_for_table(table, parsed_args))) {
        has_stationary_table = true;
        stationary_table = &table;
        delete partitions;
      }
      else {
        if (partitions->size() > biggest_partition_count || biggest_partition_count == 0) {
          // Choose the most partitioned table, and its partitions
          remote_partitions = partitions;
          remote_table = &table;
          biggest_partition_count = partitions->size();
        }
      }
    }

    if (has_stationary_table) {
      // n=1
      return make_one_sided_bloom_join_distributed_query(parser_info, stationary_table, remote_table, remote_partitions);
      
    }
    else {
      // n=2
      return make_recursive_bloom_join_distributed_query(parser_info, remote_table, remote_partitions);
    }
  }
  else {
    return NULL;
  }
}

#endif  // LUNDGREN_BLOOM_JOIN
