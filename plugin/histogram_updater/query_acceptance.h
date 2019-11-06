#include <string.h>
#include <mysql/service_parser.h>
#include "plugin/histogram_updater/constants.h"
#include "plugin/histogram_updater/histogram_updater.h"
//#include "plugin/histogram_updater/histogram_updater.cc"

#ifndef LUNDGREN_QUERY_ACCEPTANCE
#define LUNDGREN_QUERY_ACCEPTANCE

static bool should_query_be_distributed(const char *query) {

    const std::string plugin_flag(PLUGIN_FLAG);
    //const std::string query_string(query);
    //return (query_string.find(plugin_flag) != std::string::npos);
    return (strncmp(query, plugin_flag.c_str(), plugin_flag.length()) == 0);
}

static bool update_histograms(MYSQL_THD thd, const char *query,const int sys_var_update_rule) {

  //  if (!should_query_be_distributed(query)) {
  //      return false;
  // }
    //RULE 1
    int type = mysql_parser_get_statement_type(thd);

    if (sys_var_update_rule == 1 && type == STATEMENT_TYPE_INSERT){     //Rule 1 means update for every insert.
        return true;
    }

    //RULE 2
    if (sys_var_update_rule == 2 && type == STATEMENT_TYPE_INSERT){ //|| type == STATEMENT_TYPE_DELETE || type == STATEMENT_TYPE_UPDATE)){     //Rule 2 has a set number of runs between each update, defined in histogram_updater.h
        rule_2_counter++;
        if (rule_2_counter % rule_2_no_between_updates == 0){
            return true;
        }
        else {
            return false;
        }
    }

    //RULE 3
    if (sys_var_update_rule == 3 && type == STATEMENT_TYPE_INSERT){ //|| type == STATEMENT_TYPE_DELETE || type == STATEMENT_TYPE_UPDATE)){
        if (type == STATEMENT_TYPE_INSERT){
            rule_3_counter += 1*3;
        }
        if (type == STATEMENT_TYPE_DELETE){
            rule_3_counter += 1*2;
        }
        if (type == STATEMENT_TYPE_UPDATE){
            rule_3_counter += 1*1;
        }
        if (rule_3_counter % rule_3_no_between_updates == 0){
            return true;
        }
        else {
            return false;
        }
    }

    //RULE 4


    else {      //Rule is not handled, don't update
        return false;
    }

}

#endif  // LUNDGREN_QUERY_ACCEPTANCE
