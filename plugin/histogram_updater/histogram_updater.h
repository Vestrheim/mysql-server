//
// Created by svestrhe on 10/21/19.
//

#ifndef MYSQL_HISTOGRAM_UPDATER_H
#define MYSQL_HISTOGRAM_UPDATER_H

//System variables
int sys_var_update_rule;    //Rule variable
int sys_var_statements_between_updates;  //rule 2,3 between updates
double sys_var_insert_weight;
double sys_var_delete_weight;
double sys_var_ratio_for_update;  //Ratio for updates to use for rule six and nine
double sys_var_outside_boundary_weight;
int sys_var_inverse_sensitivity_to_change;

//Rule 2 variables
int rule_2_counter;

//Rule 3 variables
double rule_3_counter;


//Rule 6 variables
int rule_6_counter;
double rule_6_ratio;

//static int rule_6_base_no_between_updates = 200000;
int table_size;


//Rule 7 variables
double rule_7_counter;
double upper_bound;
double lower_bound;
double insert_value;

//Rule 9 variables
double rule_9_counter;
double rule_9_ratio;
#endif //MYSQL_HISTOGRAM_UPDATER_H
