#include <string.h>
#include <mysql/service_parser.h>
#include "plugin/histogram_updater/constants.h"
#include "plugin/histogram_updater/histogram_updater.h"
#include "plugin/histogram_updater/internal_query/internal_query_session.h"
#include "plugin/histogram_updater/internal_query/sql_resultset.h"
#include "plugin/histogram_updater/internal_query/sql_service_context.h"
//#include "plugin/histogram_updater/histogram_updater.cc"

#ifndef LUNDGREN_QUERY_ACCEPTANCE
#define LUNDGREN_QUERY_ACCEPTANCE

int table_size;

int fetch_measurement_table_size()
{
    Internal_query_session *session = new Internal_query_session();
    int fail =session->execute_resultless_query("USE test");
    char query[200];
    strcpy(query,"select concat('',count(*)) from measurement;");
    // std::string builder = "Analyze table ";
    // builder += table;
    // builder += " Update histograms on text";
    // const char* query = builder.c_str();
    Sql_resultset *resultset = session->execute_query(query);   //Analyze table measurement Update histograms on msm_value
    resultset->first();
    int number_of_rows = atoi(resultset->getString(0));
    delete session;
    return number_of_rows;

}

double fetch_histogram_boundaries() {
    Internal_query_session *session = new Internal_query_session();
    int fail = session->execute_resultless_query("USE test");
    char query[2000];
    strcpy(query, "SELECT TABLE_NAME,COLUMN_NAME,CAST(JSON_EXTRACT(HISTOGRAM,'$.buckets[0][0]')AS CHAR) AS LOWER_BOUND, CAST(JSON_EXTRACT(HISTOGRAM,CONCAT(\"$.buckets[\",JSON_LENGTH(`HISTOGRAM` ->> '$.buckets')-1,\"][1]\"))AS CHAR) AS UPPER_BOUND FROM INFORMATION_SCHEMA.COLUMN_STATISTICS WHERE SCHEMA_NAME = \"test\";");
    Sql_resultset *resultset = session->execute_query(query);   //Analyze table measurement Update histograms on msm_value
    resultset->first();
    double lower_bound = atof(resultset->getString(2));
    double upper_bound = atof(resultset->getString(3));
    delete session;
    return lower_bound,upper_bound;
}

static bool should_query_be_distributed(const char *query) {

    const std::string plugin_flag(PLUGIN_FLAG);
    //const std::string query_string(query);
    //return (query_string.find(plugin_flag) != std::string::npos);
    return (strncmp(query, plugin_flag.c_str(), plugin_flag.length()) == 0);
}

static bool update_histograms(MYSQL_THD thd, const char *query) {

  //  if (!should_query_be_distributed(query)) {
  //      return false;
  // }
//Definitons used elsewhere
//#define STATEMENT_TYPE_SELECT 1
//#define STATEMENT_TYPE_UPDATE 2
//#define STATEMENT_TYPE_INSERT 3
//#define STATEMENT_TYPE_DELETE 4
//#define STATEMENT_TYPE_REPLACE 5
//#define STATEMENT_TYPE_OTHER 6

    int type = mysql_parser_get_statement_type(thd);

    //RULE 1
    if (sys_var_update_rule == 1 && type == STATEMENT_TYPE_INSERT) {     //Rule 1 means update for every insert.
        return true;
    }

    //RULE 2
    else if (sys_var_update_rule == 2 && (type == STATEMENT_TYPE_INSERT || type == STATEMENT_TYPE_DELETE || type == STATEMENT_TYPE_UPDATE)){     //Rule 2 has a set number of runs between each update, defined in histogram_updater.h
        rule_2_counter++;
        if (rule_2_counter % rule_2_no_between_updates == 0){
            return true;
        }
        else {
            return false;
        }
    }

    //RULE 3
    else if (sys_var_update_rule == 3 && (type == STATEMENT_TYPE_INSERT || type == STATEMENT_TYPE_DELETE || type == STATEMENT_TYPE_UPDATE)){
        if (type == STATEMENT_TYPE_INSERT){
            rule_3_counter += 1*3;
        }
        if (type == STATEMENT_TYPE_DELETE){
            rule_3_counter += 1*2;
        }
        if (type == STATEMENT_TYPE_UPDATE){
            rule_3_counter += 1*1;
        }
        if (rule_3_counter % rule_3_no_between_updates < 3){
            return true;
        }
        else {
            return false;
        }
    }

    //RULE 6
    else if (sys_var_update_rule == 6 && (type == STATEMENT_TYPE_INSERT || type == STATEMENT_TYPE_DELETE || type == STATEMENT_TYPE_UPDATE)){
        int table_size = fetch_measurement_table_size();
        rule_6_counter++;
        float ratio = (table_size!=0)? rule_6_counter/table_size : 0;
        if (ratio > 0.1){
            rule_6_counter = 0;
            return true;
        }
        else{
            return false;
        }
    }

    //RULE 7
    else if (sys_var_update_rule == 7 && (type == STATEMENT_TYPE_INSERT || type == STATEMENT_TYPE_DELETE || type == STATEMENT_TYPE_UPDATE)){
        float lower, upper = fetch_histogram_boundaries();
    }



    else {      //Rule is not handled, don't update
        return false;
        }

}


#endif  // LUNDGREN_QUERY_ACCEPTANCE
