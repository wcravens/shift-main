#!/bin/bash

# Ubuntu: sudo -u postgres ./create_quickfix_me.sh postgres
# macOS: ./create_quickfix_me.sh $USER

SCRIPTS_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

echo "Dropping database"
dropdb -U $1 quickfix_matchingengine

echo "Creating database"
createdb -U $1 quickfix_matchingengine

echo "Creating database tables"
cd ${SCRIPTS_DIR}/quickfix
psql -U $1 -d quickfix_matchingengine -f postgresql.sql
cd ${SCRIPTS_DIR}

echo "Granting permissions to user hanlonpgsql4"
psql -U $1 -d quickfix_matchingengine -c 'GRANT CONNECT ON DATABASE quickfix_matchingengine TO hanlonpgsql4;'
psql -U $1 -d quickfix_matchingengine -c 'GRANT USAGE ON SCHEMA public TO hanlonpgsql4;'
psql -U $1 -d quickfix_matchingengine -c 'GRANT ALL PRIVILEGES ON ALL TABLES IN SCHEMA public TO hanlonpgsql4;'
psql -U $1 -d quickfix_matchingengine -c 'GRANT ALL PRIVILEGES ON ALL SEQUENCES IN SCHEMA public TO hanlonpgsql4;'
