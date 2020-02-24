#!/bin/bash

docker exec -it shift_broker ./Scripts/user_entries.sh
docker restart shift_broker

