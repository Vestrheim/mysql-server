#include <mysql/service_parser.h>
#include <string.h>
#include <algorithm>
#include <iostream>
#include "plugin/histogram_updater/constants.h"
#include "plugin/histogram_updater/distributed_query.h"
#include "sql/item.h"
#include "sql/table.h"

#ifndef LUNDGREN_DQR
#define LUNDGREN_DQR

struct L_Item {
  std::string sql;
  Item::Type type;
  std::string alias;
};


int catch_item(MYSQL_ITEM item, unsigned char *arg) {
  std::vector<L_Item> *fields = (std::vector<L_Item> *)arg;

  if (item != NULL) {
    /*
    String s;
    item->print(&s, QT_ORDINARY);

    std::string item_sql = std::string(s.ptr());
    // hack
    item_sql.erase(std::remove(item_sql.begin(), item_sql.end(), '`'),
                   item_sql.end());
    item_sql.erase(std::remove(item_sql.begin(), item_sql.end(), '('),
                   item_sql.end());
    item_sql.erase(std::remove(item_sql.begin(), item_sql.end(), ')'),
                   item_sql.end());

    L_Item fi = {item_sql, item->type()};


    // There is an alias provided
    if (!item->item_name.is_autogenerated()) {

      char *alias_buffer = new char[item->item_name.length()];
      item->item_name.strcpy(alias_buffer);
      fi.alias = std::string(alias_buffer);
      delete alias_buffer;
    }

    fields->push_back(fi);
     */
  }

  return 0;
}

int catch_table(TABLE_LIST *tl, unsigned char *arg) {
  // const char **result_string_ptr = (const char **)arg;

  std::vector<L_Table> *tables = (std::vector<L_Table> *)arg;

  if (tl != NULL) {
    L_Table t = {std::string(tl->table_name)};
    tables->push_back(t);
    return 0;
  }
  return 1;
}

static void place_projection_in_table(L_Item field_item,
                                      std::vector<L_Table> *tables,
                                      bool where_transitive_projection) {

  std::string projection = field_item.sql;

  std::string field =
      projection.substr(projection.find(".") + 1, projection.length());
  for (auto &table : *tables) {
    if (table.name == projection.substr(0, projection.find("."))) {
      if (where_transitive_projection) {

        // only add as where transitive if not in regular projection set
        if (std::find(table.projections.begin(), table.projections.end(), field) == table.projections.end()) {
          table.where_transitive_projections.push_back(field);
        }
        table.join_columns.push_back(field);
      } else {
        table.projections.push_back(field);
        table.aliases.push_back(field_item.alias);
      }
      break;
    }
  }
}

static L_Parser_info *get_tables_from_parse_tree(MYSQL_THD thd) {

  /*
   * Walk parse tree
   */

  std::vector<L_Item> fields = std::vector<L_Item>();
  mysql_parser_visit_tree(thd, catch_item, (unsigned char *)&fields);

  std::vector<L_Table> tables = std::vector<L_Table>();

  mysql_parser_visit_tables(thd, catch_table, (unsigned char *)&tables);

  if (tables.size() == 0) {
    return NULL;
  }

/*
  std::string where_clause = "";
  bool passed_where_clause = false;

  std::vector<L_Item>::iterator f = fields.begin();

  switch (f->type) {
    case Item::FIELD_ITEM:

      while (f != fields.end()) {
        if (f->sql.find("=") != std::string::npos) {
          where_clause += f->sql;
          passed_where_clause = true;

          f++;
          continue;
        }
        if (f->type != Item::FIELD_ITEM) {
          f++;
          continue;
        }

        place_projection_in_table(*f, &tables, passed_where_clause);
        f++;
      }
      break;
    case Item::SUM_FUNC_ITEM:

      // THIS IS NOW BROKEN
      // ++f;

      // partition_query_string += "SUM(" + f->sql + ") as " + f->sql +
      //                           "_sum, count(*) as " + f->sql + "_count ";
      // final_query_string +=
      //     "(SUM(" + f->sql + "_sum) / SUM(" + f->sql + "_count)) as average
      //     ";

      break;
    default:
      break;
  }
*/

  L_Parser_info *parser_info = new L_Parser_info();
  parser_info->tables = tables;
/*
  if (where_clause.length() > 0) {

   std::string clean_where_clause = tables[0].name + "." + tables[0].join_columns[0] + " = " + tables[1].name + "." + tables[1].join_columns[0];

    // for (auto &table : tables) {
    //   for (auto &join_column : table.join_columns) {
    //     clean_where_clause = table.name + "." + join_column;
    //   }
    //   clean_where_clause += " = ";
    // }

    parser_info->where_clause = clean_where_clause;

  }
*/
  return parser_info;
}


#endif  // LUNDGREN_DQR
