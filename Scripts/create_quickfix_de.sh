#!/bin/bash

# Ubuntu: sudo -u postgres ./create_quickfix_de.sh postgres
# macOS: ./create_quickfix_de.sh $USER

SCRIPTS_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

echo "Dropping database"
dropdb -U $1 quickfix_datafeedengine

echo "Creating database"
createdb -U $1 quickfix_datafeedengine

echo "Creating database tables"
cd ${SCRIPTS_DIR}/quickfix
psql -U $1 -d quickfix_datafeedengine -f postgresql.sql
cd ${SCRIPTS_DIR}
