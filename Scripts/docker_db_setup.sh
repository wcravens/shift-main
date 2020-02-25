#!/bin/bash
psql -d postgres -v username="\"${BC_USER}\"" -v password="'${BC_PASS}'" -f `pwd`db_setup.sql

