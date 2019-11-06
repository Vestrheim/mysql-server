#!/bin/bash

cd /export/home/tmp/bygging/runtime_output_directory

./mysql -u root --socket /export/home/tmp/bygging/mysql-test/var/tmp/mysqld.1.sock -c

use test;
	
CREATE TABLE employee(
   emp_id INT AUTO_INCREMENT PRIMARY KEY,
   gender int(1) not null,
   salary decimal(12,2),
   department_id int
);

Create table department(
	dept_id int auto_increment primary key,
	dept_name int,
	dept_location_id int,
	dept_manager INT
);

create table location(
	location_id int auto_increment primary key,
	location_name int,
	country_id int
);

create table country(
	country_id int auto_increment primary key,
	country_name int
);

use test;

Load data infile '../../std_data/employee.csv'
into table employee
fields terminated by ','
enclosed by '"'
lines terminated by '\n'
IGNORE 1 ROWS;

Load data infile '../../std_data/department.csv'
into table department
fields terminated by ','
enclosed by '"'
lines terminated by '\n'
IGNORE 1 ROWS;

Load data infile '../../std_data/location.csv'
into table location
fields terminated by ','
enclosed by '"'
lines terminated by '\n'
IGNORE 1 ROWS;

Load data infile '../../std_data/country.csv'
into table country
fields terminated by ','
enclosed by '"'
lines terminated by '\n'
IGNORE 1 ROWS;

alter table department
add foreign key (dept_manager) references employee(emp_id);

alter table department
add foreign key (dept_location_id) references location(location_id);

alter table employee
add foreign key (department_id) references department(dept_id);

alter table location
add foreign key (country_id) references country(country_id);

analyze table department;
analyze table employee;
analyze table location;
analyze table country;

exit;