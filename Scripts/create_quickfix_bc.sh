#!/bin/bash

SCRIPTS_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

echo "Dropping databases"
dropdb -U $1 quickfix_bcme
dropdb -U $1 quickfix_bclc

echo "Creating databases"
createdb -U $1 -O hanlonpgsql4 quickfix_bcme
createdb -U $1 -O hanlonpgsql4 quickfix_bclc

echo "Creating database tables"
cd ${SCRIPTS_DIR}/quickfix
psql -U $1 -d quickfix_bcme -f postgresql.sql
psql -U $1 -d quickfix_bclc -f postgresql.sql
cd ${SCRIPTS_DIR}

echo "Changing database tables ownership"
# sessions_table.sql
psql -U $1 -d quickfix_bcme -c 'ALTER TABLE public.sessions OWNER TO hanlonpgsql4;'
psql -U $1 -d quickfix_bclc -c 'ALTER TABLE public.sessions OWNER TO hanlonpgsql4;'
# messages_table.sql
psql -U $1 -d quickfix_bcme -c 'ALTER TABLE public.messages OWNER TO hanlonpgsql4;'
psql -U $1 -d quickfix_bclc -c 'ALTER TABLE public.messages OWNER TO hanlonpgsql4;'
# messages_log_table.sql
psql -U $1 -d quickfix_bcme -c 'ALTER TABLE public.messages_log OWNER TO hanlonpgsql4;'
psql -U $1 -d quickfix_bclc -c 'ALTER TABLE public.messages_log OWNER TO hanlonpgsql4;'
# event_log_table.sql
psql -U $1 -d quickfix_bcme -c 'ALTER TABLE public.event_log OWNER TO hanlonpgsql4;'
psql -U $1 -d quickfix_bcme -c 'ALTER TABLE public.event_log OWNER TO hanlonpgsql4;'
psql -U $1 -d quickfix_bclc -c 'ALTER TABLE public.event_backup_log OWNER TO hanlonpgsql4;'
psql -U $1 -d quickfix_bclc -c 'ALTER TABLE public.event_backup_log OWNER TO hanlonpgsql4;'
