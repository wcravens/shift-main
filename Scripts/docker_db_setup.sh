#!/bin/bash
psql -d postgres -v username=hanlonpgsql4 -v password=XIfyqPqM446M -f `pwd`db_setup.sql

