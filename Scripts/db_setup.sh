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

psql -d postgres -v username="\"${BC_USER}\"" -v password="'${BC_PASS}'" -f "${SQL_SOURCE}db_setup.sql";

psql -d postgres -f "${SQL_SOURCE}de_instances.sql";
psql -d postgres -f "${SQL_SOURCE}bc_instances.sql";

echo "~~${BC_USER} SETUP ESTABLISHED~~"
#sh "${SH_SOURCE}user_entries.sh"
