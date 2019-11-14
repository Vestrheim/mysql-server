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
#include "plugin/histogram_updater/histogram_updater.h"

#define PLUGIN_NAME "Histrogram Updater"

//Declare internal variables


/// Updater function for the status variable ..._rule.
static void update_rule(MYSQL_THD, SYS_VAR *, void *, const void *save) {
    sys_var_update_rule = *static_cast<const int *>(save);
}


static MYSQL_SYSVAR_INT(rule,              // Name.
                        sys_var_update_rule,      // Variable.
                        PLUGIN_VAR_NOCMDARG,  // Not a command-line argument.
                        "Tells " PLUGIN_NAME " what updating rule should be followed",
                        NULL,            // Check function.
                        update_rule,  // Update function.
                        0,               // Default value.
                        0,               // Min value.
                        20,               // Max value.
                        1                // Block size.
                        );


SYS_VAR *histogram_rewriter_plugin_sys_vars[] = {MYSQL_SYSVAR(rule), NULL};


static int histogram_updater_notify(MYSQL_THD thd, mysql_event_class_t event_class,
                                const void *event);

/* instrument the memory allocation */
#ifdef HAVE_PSI_INTERFACE
static PSI_memory_key key_memory_lundgren;

static PSI_memory_info all_rewrite_memory[] = {
    {&key_memory_lundgren, "histogram_updater", 0, 0, PSI_DOCUMENT_ME}};

static int plugin_init(MYSQL_PLUGIN) {
  const char *category = "sql";
  int count;
  count = static_cast<int>(array_elements(all_rewrite_memory));
  rule_2_counter = 0;       //Init counters to 0
  rule_3_counter = 0;

  sys_var_update_rule = 0;      // Intialise with the rule set to don't update histograms.

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



/* Audit plugin descriptor */
static struct st_mysql_audit lundgren_descriptor = {
        MYSQL_AUDIT_INTERFACE_VERSION, /* interface version */
        NULL,                          /* release_thd()     */
        histogram_updater_notify,      /* event_notify()    */
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
        histogram_rewriter_plugin_sys_vars, /* system variables              */
        NULL,                       /* reserverd                     */
        0                           /* flags                         */
} mysql_declare_plugin_end;


void connect_and_run(const char* table)
{
    Internal_query_session *session = new Internal_query_session();
    int fail =session->execute_resultless_query("USE test");
    char query[200];
    strcpy(query,"Analyze table ");
    strcat(query,table);
    strcat(query," Update histogram on msm_value");
    // std::string builder = "Analyze table ";
    // builder += table;
    // builder += " Update histograms on text";
    // const char* query = builder.c_str();
    Sql_resultset *resultset = session->execute_query(query);   //Analyze table measurement Update histograms on msm_value
    delete session;

}

/**
  Entry point to the plugin. The server calls this function after each parsed
  query when the plugin is active.
*/

static int histogram_updater_notify(MYSQL_THD thd, mysql_event_class_t event_class,
                                   const void *event) {
  if (event_class == MYSQL_AUDIT_PARSE_CLASS) {
    const struct mysql_event_parse *event_parse =
        static_cast<const struct mysql_event_parse *>(event);
    if (event_parse->event_subclass != MYSQL_AUDIT_PARSE_POSTPARSE || sys_var_update_rule == 0) {
        return 0;       //if we don't match our event class or the update rule is 0 then don't do anything.
    }

    if (!update_histograms(thd, event_parse->query.str)) {
        return 0;
    }

    L_Parser_info *parser_info = get_tables_from_parse_tree(thd);

    const char* table_name = parser_info->tables[0].name.c_str();

    // connect_and_run(table_name); we used to do this, but we only want to update historams on one table, so let's simplify this for ourselves
    connect_and_run("measurement");

  }

  return 0;
}





