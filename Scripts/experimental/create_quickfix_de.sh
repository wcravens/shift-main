#!/bin/bash

# Ubuntu: sudo -u postgres ./create_quickfix_de.sh postgres
# macOS: ./create_quickfix_de.sh $USER

SCRIPTS_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

echo "Dropping database"
dropdb -U $1 quickfix_datafeedengine

echo "Creating database"
createdb -U $1 -O hanlonpgsql4 quickfix_datafeedengine

echo "Creating database tables"
cd ${SCRIPTS_DIR}/quickfix
psql -U $1 -d quickfix_datafeedengine -f postgresql.sql
cd ${SCRIPTS_DIR}

echo "Changing database tables ownership"
# sessions_table.sql
psql -U $1 -d quickfix_datafeedengine -c 'ALTER TABLE public.sessions OWNER TO hanlonpgsql4;'
# messages_table.sql
psql -U $1 -d quickfix_datafeedengine -c 'ALTER TABLE public.messages OWNER TO hanlonpgsql4;'
# messages_log_table.sql
psql -U $1 -d quickfix_datafeedengine -c 'ALTER TABLE public.messages_log OWNER TO hanlonpgsql4;'
# event_log_table.sql
psql -U $1 -d quickfix_datafeedengine -c 'ALTER TABLE public.event_log OWNER TO hanlonpgsql4;'
psql -U $1 -d quickfix_datafeedengine -c 'ALTER TABLE public.event_log OWNER TO hanlonpgsql4;'
