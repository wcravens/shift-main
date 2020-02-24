#!/bin/bash

RUNTYPE=$1

if [[ $RUNTYPE == "local" ]]
then
    SQL_SOURCE=""
    SH_SOURCE=""
else
    SQL_SOURCE="/docker-entrypoint-initdb.d/runsql/"
    SH_SOURCE="/docker-entrypoint-initdb.d/runsh/"
fi

psql -d postgres -v username="\"hanlonpgsql4\"" -v password="'XIfyqPqM446M'" -f "${SQL_SOURCE}db_setup.sql";

psql -d postgres -f "${SQL_SOURCE}de_instances.sql";
psql -d postgres -f "${SQL_SOURCE}bc_instances.sql";

#sh "${SH_SOURCE}user_entries.sh"
