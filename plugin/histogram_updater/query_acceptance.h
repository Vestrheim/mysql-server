#include <string.h>
#include <mysql/service_parser.h>
#include "plugin/histogram_updater/constants.h"
#include "plugin/histogram_updater/histogram_updater.h"
#include "plugin/histogram_updater/internal_query/internal_query_session.h"
#include "plugin/histogram_updater/internal_query/sql_resultset.h"
#include "plugin/histogram_updater/internal_query/sql_service_context.h"
#include "my_inttypes.h"
#include <regex>
//#include "plugin/histogram_updater/histogram_updater.cc"

#ifndef LUNDGREN_QUERY_ACCEPTANCE
#define LUNDGREN_QUERY_ACCEPTANCE

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
    if (resultset->get_rows()>0){
        resultset->first();
        int number_of_rows = atoi(resultset->getString(0));
        delete session;
        return number_of_rows;
    }
    else {
        delete session;
        return -1;
    }
}


double fetch_histogram_boundaries() {
    double lower;
    double upper;
    Internal_query_session *session = new Internal_query_session();
    int fail = session->execute_resultless_query("USE test");
    char query[2000];
    strcpy(query, "SELECT TABLE_NAME,COLUMN_NAME,CAST(JSON_EXTRACT(HISTOGRAM,'$.buckets[0][0]')AS CHAR) AS LOWER_BOUND, CAST(JSON_EXTRACT(HISTOGRAM,CONCAT(\"$.buckets[\",JSON_LENGTH(`HISTOGRAM` ->> '$.buckets')-1,\"][1]\"))AS CHAR) AS UPPER_BOUND FROM INFORMATION_SCHEMA.COLUMN_STATISTICS WHERE SCHEMA_NAME = \"test\";");
    Sql_resultset *resultset = session->execute_query(query);   //Analyze table measurement Update histograms on msm_value
    if (resultset->get_rows()>0) {
        resultset->first();
        lower = atof(resultset->getString(2));
        upper = atof(resultset->getString(3));
    }
    else{
        lower = 0;
        upper = 0;
    }
    delete session;
    return lower, upper;
}


static bool update_histograms(MYSQL_THD thd, const char *query) {

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
        if (std::fmod(rule_2_counter,sys_var_statements_between_updates) < 1){
            return true;
        }
        else {
            return false;
        }
    }

    //RULE 3
    else if (sys_var_update_rule == 3 && (type == STATEMENT_TYPE_INSERT || type == STATEMENT_TYPE_DELETE || type == STATEMENT_TYPE_UPDATE)){
        if (type == STATEMENT_TYPE_INSERT){
            rule_3_counter += 1*sys_var_insert_weight;
        }
        if (type == STATEMENT_TYPE_DELETE){
            rule_3_counter += 1*sys_var_delete_weight;
        }
        if (type == STATEMENT_TYPE_UPDATE){
            rule_3_counter += 1*1;
        }
        if (std::fmod(rule_3_counter,sys_var_statements_between_updates) < std::min(sys_var_delete_weight,sys_var_insert_weight) ){
            //printf("HEIHEI %d\n", rule_3_counter);
            rule_3_counter = 0;
            return true;
        }
        else {
            return false;
        }
    }

    //RULE 6
    else if (sys_var_update_rule == 6 && (type == STATEMENT_TYPE_INSERT || type == STATEMENT_TYPE_DELETE || type == STATEMENT_TYPE_UPDATE)){
        if (table_size == -1){
            table_size = fetch_measurement_table_size();
        }
        rule_6_counter++;
        rule_6_ratio = (table_size!=0&&table_size!=-1)? (double)rule_6_counter/double(table_size) : 1;
        if (rule_6_ratio > sys_var_ratio_for_update){
            rule_6_counter = 0;
            table_size  =-1;
            return true;
        }
        else{
            return false;
        }
    }

    //RULE 7
    else if (sys_var_update_rule == 7 && (type == STATEMENT_TYPE_INSERT || type == STATEMENT_TYPE_DELETE || type == STATEMENT_TYPE_UPDATE)){

        if (type == STATEMENT_TYPE_INSERT){//Get the inserted value
            std::string result;
            std::regex re("(\\d{2}\\.\\d{3,4})");
            std::cmatch match;
            if (std::regex_search(query, match, re) && match.size() > 1) {
                result = match.str(1);
            } else {
                result = std::string("");
                insert_value = -1;
            }
            if (result.length()>0){
                insert_value = atof(result.c_str());
            }
        }
        if (lower_bound==-1 && upper_bound == -1){ //Intialize/ fetch new data if required
            lower_bound, upper_bound = fetch_histogram_boundaries();
        }
        if (type == STATEMENT_TYPE_INSERT && (upper_bound <= insert_value || insert_value <= lower_bound)){ //statement outside range
            rule_7_counter += 1*sys_var_outside_boundary_weight;
        }
        else  {//Statment inside range or delete statement
            rule_7_counter++;
        }
        if (std::fmod(rule_7_counter,sys_var_statements_between_updates) < sys_var_outside_boundary_weight){//Do we need to udpate?
            rule_7_counter = 0;
            lower_bound = -1;
            upper_bound = -1;
            return true;
        }
        else{
            return false;
        }
    }

    //RULE 9
    else if (sys_var_update_rule == 9 && (type == STATEMENT_TYPE_INSERT || type == STATEMENT_TYPE_DELETE || type == STATEMENT_TYPE_UPDATE)){
        if (table_size == -1){
            table_size = fetch_measurement_table_size();
        }
        if (type == STATEMENT_TYPE_INSERT){//Get the inserted value
            std::string result;
            std::regex re("(\\d{2}\\.\\d{3,4})");
            std::cmatch match;
            if (std::regex_search(query, match, re) && match.size() > 1) {
                result = match.str(1);
            } else {
                result = std::string("");
                insert_value = -1;
            }
            if (result.length()>0){
                insert_value = atof(result.c_str());
            }
        }
        if (lower_bound==-1 && upper_bound == -1){ //Intialize/ fetch new data if required
            lower_bound, upper_bound = fetch_histogram_boundaries();
        }
        if (type == STATEMENT_TYPE_INSERT && (upper_bound <= insert_value || insert_value <= lower_bound)){ //statement outside range
            rule_9_counter += 1*sys_var_outside_boundary_weight;
        } else if (type == STATEMENT_TYPE_INSERT){
            rule_9_counter += 1*sys_var_insert_weight;
        } else if (type == STATEMENT_TYPE_DELETE){
            rule_9_counter += 1*sys_var_delete_weight;
        } else {
            rule_9_counter ++;
        }

        rule_9_ratio = (table_size!=0&&table_size!=-1)? rule_9_counter/table_size : 1;

        if (rule_9_counter*rule_9_ratio > sys_var_inverse_sensitivity_to_change){
            rule_9_counter = 0;
            return true;
        }
        else {
            return false;
        }
    }


    else {      //Rule is not handled, don't update
        return false;
        }

}


#endif  // LUNDGREN_QUERY_ACCEPTANCE
