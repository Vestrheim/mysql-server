/*  Copyright (c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License, version 2.0,
    as published by the Free Software Foundation.

    This program is also distributed with certain software (including
    but not limited to OpenSSL) that is licensed under separate terms,
    as designated in a particular file or component or in included license
    documentation.  The authors of MySQL hereby grant you an additional
    permission to link the program and your derivative works with the
    separately licensed software that they have included with MySQL.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License, version 2.0, for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA */

#include <ctype.h>
#include <mysql/components/services/log_builtins.h>
#include <mysql/plugin.h>
#include <mysql/plugin_audit.h>
#include <mysql/psi/mysql_memory.h>
#include <mysql/service_mysql_alloc.h>
#include <string.h>
#include <thread>

//#include <mysql/service_command.h>
//#include <mysql/service_parser.h>
//#include <mysql/service_srv_session.h>

#include "my_inttypes.h"
#include "my_psi_config.h"
#include "my_thread.h"  // my_thread_handle needed by mysql_memory.h

#include <iostream>

//#include "plugin/histogram_updater/distributed_query_manager.h"
#include "plugin/histogram_updater/distributed_query_rewriter.h"
#include "plugin/histogram_updater/distributed_query.h"
#include "plugin/histogram_updater/query_acceptance.h"
#include "plugin/histogram_updater/internal_query/internal_query_session.h"
#include "plugin/histogram_updater/internal_query/sql_resultset.h"
#include "plugin/histogram_updater/helpers.h"

/* instrument the memory allocation */
#ifdef HAVE_PSI_INTERFACE
static PSI_memory_key key_memory_lundgren;

static PSI_memory_info all_rewrite_memory[] = {
    {&key_memory_lundgren, "histogram_updater", 0, 0, PSI_DOCUMENT_ME}};

static int plugin_init(MYSQL_PLUGIN) {
  const char *category = "sql";
  int count;
  count = static_cast<int>(array_elements(all_rewrite_memory));

  /*
  MYSQL_THD thd;
  mysql_memory_register(category, all_rewrite_memory, count);
  std::string create_table = "CREATE TABLE IF NOT EXISTS tester (\n"
                               "    test_id INT AUTO_INCREMENT,\n"
                               "    text VARCHAR(255) NOT NULL,\n";
  char *init_query;
  strncpy(init_query, create_table.c_str(), sizeof(create_table));
  MYSQL_LEX_STRING new_query = {init_query, sizeof(init_query)};
  mysql_parser_parse(thd, new_query, false, NULL, NULL);

   */
  return 0; /* success */
}
#else
#define plugin_init NULL
#define key_memory_lundgren PSI_NOT_INSTRUMENTED
#endif /* HAVE_PSI_INTERFACE */

void connect_and_run()
{
    Internal_query_session *session = new Internal_query_session();
    int fail = session->execute_resultless_query("USE histogram_updater");
    //fail = session->execute_resultless_query("Create database if not exists hei_hei");
    fail = session->execute_resultless_query("Insert into 'tester' ('text') VALUES ('1')");
    if (fail == 1) {
    }
    else{
        delete session;
    }
}

static int lundgren_start(MYSQL_THD thd, mysql_event_class_t event_class,
                          const void *event) {
  if (event_class == MYSQL_AUDIT_PARSE_CLASS) {
    const struct mysql_event_parse *event_parse =
        static_cast<const struct mysql_event_parse *>(event);
    if (event_parse->event_subclass == MYSQL_AUDIT_PARSE_POSTPARSE) {


      if (!accept_query(thd, event_parse->query.str)) {
        return 0;
      }

      std::thread plugin_executor (connect_and_run);

      plugin_executor.join();

      //bool is_join = detect_join(event_parse->query.str);

      //L_Parser_info *parser_info = get_tables_from_parse_tree(thd);

      //Distributed_query* distributed_query;

      /*if (is_join) {
        if (parser_info != NULL) {
          parser_info->tables.pop_back(); //hack
        }

        L_parsed_comment_args parsed_args = parse_query_comments(event_parse->query.str);

        switch(parsed_args.join_strategy) {
        case SEMI:
          distributed_query = make_semi_join_distributed_query(parser_info, parsed_args);
          break;
        case BLOOM:
          distributed_query = make_bloom_join_distributed_query(parser_info, parsed_args);
          break;
        case SORT_MERGE:
          distributed_query = execute_sort_merge_distributed_query(parser_info);
          break;
        case HASH_REDIST:
          distributed_query = make_hash_redist_join_distributed_query(parser_info, parsed_args, event_parse->query.str);
          break;
        case DATA_TO_QUERY:
        default:
          distributed_query = make_data_to_query_distributed_query(parser_info, true);
          break;
        }

      } else {
        distributed_query = make_data_to_query_distributed_query(parser_info, false);
      } */


     // std::string incoming_query;
     // incoming_query = event_parse->query.str;

     // std::string temp = "Select '"+incoming_query+"';";



     // std::string temp = "insert into tester (text) values('test');";
     //   fail = session->execute_resultless_query("insert into tester (text) values('test');");
     // Sql_resultset *result = session->execute_query(temp.c_str());


      // std::vector<L_Table> table = parser_info->tables;

     /* int type = mysql_parser_get_statement_type(thd);
      int number_of_querys = 0;
      if (type != 1) {
          number_of_querys +=1;
      }
      temp ="Insert into 'tester' ('text') VALUES ('"+std::to_string(number_of_querys)+"')";
*/
      /*
      // std::string query = "UPDATE HISTOGRAMS;";


      std::string query = "Insert into 'tester' ('text') VALUES ('"+std::to_string(number_of_querys)+"')";
*/

      /*char *query_to_run;
      strncpy(query_to_run, temp.c_str(), sizeof(temp));
      MYSQL_LEX_STRING new_query = {query_to_run, sizeof(query_to_run)};
      mysql_parser_parse(thd, new_query, false, NULL, NULL);
        */
       /*

      if (number_of_querys % 10 == 0){

          MYSQL_LEX_STRING new_query = {query_to_run, sizeof(query_to_run)};
          mysql_parser_parse(thd, new_query, false, NULL, NULL);
      }
        */


      *((int *)event_parse->flags) |=
          (int)MYSQL_AUDIT_PARSE_REWRITE_PLUGIN_QUERY_REWRITTEN;
    }
  }

  return 0;
}



/* Audit plugin descriptor */
static struct st_mysql_audit lundgren_descriptor = {
    MYSQL_AUDIT_INTERFACE_VERSION, /* interface version */
    NULL,                          /* release_thd()     */
    lundgren_start,                /* event_notify()    */
    {
        0,
        0,
        (unsigned long)MYSQL_AUDIT_PARSE_ALL,
    } /* class mask        */
};

/* Plugin descriptor */
mysql_declare_plugin(audit_log){
    MYSQL_AUDIT_PLUGIN,   /* plugin type                   */
    &lundgren_descriptor, /* type specific descriptor      */
    "histogram_updater",           /* plugin name                   */
    "Sevre Vestrheim", /* author */
    "Histogram updater plugin", /* description                   */
    PLUGIN_LICENSE_GPL,         /* license                       */
    plugin_init,                /* plugin initializer            */
    NULL,                       /* plugin check uninstall        */
    NULL,                       /* plugin deinitializer          */
    0x0001,                     /* version                       */
    NULL,                       /* status variables              */
    NULL,                       /* system variables              */
    NULL,                       /* reserverd                     */
    0                           /* flags                         */
} mysql_declare_plugin_end;
