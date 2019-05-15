#include <string.h>
#include <mysql/service_parser.h>
#include "plugin/histogram_updater/constants.h"

#ifndef LUNDGREN_QUERY_ACCEPTANCE
#define LUNDGREN_QUERY_ACCEPTANCE

static bool should_query_be_distributed(const char *query) {

    const std::string plugin_flag(PLUGIN_FLAG);
    //const std::string query_string(query);
    //return (query_string.find(plugin_flag) != std::string::npos);
    return (strncmp(query, plugin_flag.c_str(), plugin_flag.length()) == 0);
}

static bool accept_query(MYSQL_THD thd, const char *query) {

    if (!should_query_be_distributed(query)) {
        return false;
   }

    int type = mysql_parser_get_statement_type(thd);

    //We will accept all types of query's
    return true; // type == STATEMENT_TYPE_SELECT);
}

#endif  // LUNDGREN_QUERY_ACCEPTANCE
