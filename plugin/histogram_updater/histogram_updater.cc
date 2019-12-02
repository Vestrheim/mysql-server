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

/// Updater function for the status variable ...statements_between_updates.
static void update_statements_between_updates(MYSQL_THD, SYS_VAR *, void *, const void *save) {
    sys_var_statements_between_updates = *static_cast<const int *>(save);
}

/// Updater function for the status variable ...insert_weight.
static void update_insert_weight(MYSQL_THD, SYS_VAR *, void *, const void *save) {
    sys_var_insert_weight = *static_cast<const double *>(save);
}

/// Updater function for the status variable ...delete_weight.
static void update_delete_weight(MYSQL_THD, SYS_VAR *, void *, const void *save) {
    sys_var_delete_weight = *static_cast<const double *>(save);
}

/// Updater function for the status variable ...ratio_for_update.
static void update_ratio_for_updates(MYSQL_THD, SYS_VAR *, void *, const void *save) {
    sys_var_ratio_for_update = *static_cast<const double *>(save);
}

/// Updater function for the status variable ...outside_boundary_weight.
static void update_outside_boundary_weight(MYSQL_THD, SYS_VAR *, void *, const void *save) {
    sys_var_outside_boundary_weight = *static_cast<const double *>(save);
}

/// Updater function for the status variable ...inverse_sensitivity_to_change.
static void update_inverse_sensitivity_to_change(MYSQL_THD, SYS_VAR *, void *, const void *save) {
    sys_var_inverse_sensitivity_to_change = *static_cast<const int *>(save);
}


static MYSQL_SYSVAR_INT(rule,              // Name.
                        sys_var_update_rule,      // Variable.
                        PLUGIN_VAR_NOCMDARG,  // Not a command-line argument.
                        "Tells " PLUGIN_NAME " what updating rule should be followed",
                        NULL,            // Check function.
                        update_rule,  // Update function.
                        0,               // Default value.
                        0,               // Min value.
                        10,               // Max value.
                        1                // Block size.
                        );

static MYSQL_SYSVAR_INT(statements_between_updates,              // Name.
                        sys_var_statements_between_updates,      // Variable.
                        PLUGIN_VAR_NOCMDARG,  // Not a command-line argument.
                        "Tells " PLUGIN_NAME " how many statements there should be between updates in rule 2, 3 and 7",
                        NULL,            // Check function.
                        update_statements_between_updates,  // Update function.
                        1000,               // Default value.
                        0,               // Min value.
                        INT_MAX,               // Max value.
                        1                // Block size.
                        );

static MYSQL_SYSVAR_DOUBLE(insert_weight,              // Name.
                        sys_var_insert_weight,      // Variable.
                        PLUGIN_VAR_NOCMDARG,  // Not a command-line argument.
                        "Tells " PLUGIN_NAME " how important we think insert statements are",
                        NULL,            // Check function.
                        update_insert_weight,  // Update function.
                        5,               // Default value.
                        0,               // Min value.
                        INT_MAX,               // Max value.
                        1                // Block size.
                        );

static MYSQL_SYSVAR_DOUBLE(delete_weight,              // Name.
                        sys_var_delete_weight,      // Variable.
                        PLUGIN_VAR_NOCMDARG,  // Not a command-line argument.
                        "Tells " PLUGIN_NAME " how important we think delete statements are",
                        NULL,            // Check function.
                        update_delete_weight,  // Update function.
                        3,               // Default value.
                        0,               // Min value.
                        INT_MAX,               // Max value.
                        1                // Block size.
                        );

static MYSQL_SYSVAR_DOUBLE(ratio_for_update,              // Name.
                        sys_var_ratio_for_update,      // Variable.
                        PLUGIN_VAR_NOCMDARG,  // Not a command-line argument.
                        "Tells " PLUGIN_NAME " what the ratio of updated rows in a table must be to force an update",
                        NULL,            // Check function.
                        update_ratio_for_updates,  // Update function.
                        0.1,               // Default value.
                        0.00001,               // Min value.
                        1,               // Max value.
                        1                // Block size.
                        );

static MYSQL_SYSVAR_DOUBLE(outside_boundary_weight,              // Name.
                        sys_var_outside_boundary_weight,      // Variable.
                        PLUGIN_VAR_NOCMDARG,  // Not a command-line argument.
                        "Tells " PLUGIN_NAME " how important updates that are outside the boundary of a histogram are regarded",
                        NULL,            // Check function.
                        update_outside_boundary_weight,  // Update function.
                        5,               // Default value.
                        0,               // Min value.
                        INT_MAX,               // Max value.
                        1                // Block size.
                        );

static MYSQL_SYSVAR_INT(inverse_sensitivity_to_change,              // Name.
                        sys_var_inverse_sensitivity_to_change,      // Variable.
                        PLUGIN_VAR_NOCMDARG,  // Not a command-line argument.
                        "Tells " PLUGIN_NAME " how important updates that are outside the boundary of a histogram are regarded",
                        NULL,            // Check function.
                        update_inverse_sensitivity_to_change,  // Update function.
                        10000,               // Default value.
                        0,               // Min value.
                        INT_MAX,               // Max value.
                        1                // Block size.
                        );

SYS_VAR *histogram_rewriter_plugin_sys_vars[] = {MYSQL_SYSVAR(rule),
                                                 MYSQL_SYSVAR(statements_between_updates),
                                                 MYSQL_SYSVAR(ratio_for_update),
                                                 MYSQL_SYSVAR(outside_boundary_weight),
                                                 MYSQL_SYSVAR(inverse_sensitivity_to_change),NULL};


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
  table_size  = -1;
  lower_bound = -1;
  upper_bound = -1;
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
    // connect_and_run(table_name); we used to do this, but we only want to update historams on one table, so let's simplify this for ourselves
    connect_and_run("measurement");

  }

  return 0;
}





