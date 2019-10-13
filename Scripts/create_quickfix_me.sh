#!/bin/bash

SCRIPTS_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

echo "Dropping databases"
dropdb -U $1 quickfix_mede
dropdb -U $1 quickfix_mebc

echo "Creating databases"
createdb -U $1 -O hanlonpgsql4 quickfix_mede
createdb -U $1 -O hanlonpgsql4 quickfix_mebc

echo "Creating database tables"
cd ${SCRIPTS_DIR}/quickfix
psql -U $1 -d quickfix_mede -f postgresql.sql
psql -U $1 -d quickfix_mebc -f postgresql.sql
cd ${SCRIPTS_DIR}

echo "Changing database tables ownership"
# sessions_table.sql
psql -U $1 -d quickfix_mede -c 'ALTER TABLE public.sessions OWNER TO hanlonpgsql4;'
psql -U $1 -d quickfix_mebc -c 'ALTER TABLE public.sessions OWNER TO hanlonpgsql4;'
# messages_table.sql
psql -U $1 -d quickfix_mede -c 'ALTER TABLE public.messages OWNER TO hanlonpgsql4;'
psql -U $1 -d quickfix_mebc -c 'ALTER TABLE public.messages OWNER TO hanlonpgsql4;'
# messages_log_table.sql
psql -U $1 -d quickfix_mede -c 'ALTER TABLE public.messages_log OWNER TO hanlonpgsql4;'
psql -U $1 -d quickfix_mebc -c 'ALTER TABLE public.messages_log OWNER TO hanlonpgsql4;'
# event_log_table.sql
psql -U $1 -d quickfix_mede -c 'ALTER TABLE public.event_log OWNER TO hanlonpgsql4;'
psql -U $1 -d quickfix_mede -c 'ALTER TABLE public.event_log OWNER TO hanlonpgsql4;'
psql -U $1 -d quickfix_mebc -c 'ALTER TABLE public.event_backup_log OWNER TO hanlonpgsql4;'
psql -U $1 -d quickfix_mebc -c 'ALTER TABLE public.event_backup_log OWNER TO hanlonpgsql4;'
