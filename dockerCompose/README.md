# Docker Compose

There are two DockerCompose files used to run the docker images created from `dockerFiles/`

1. shift.yml -  which pertains to the base system, and runs the 3 main modules as 3 separate containers
2. psql.yml - which sets up and initializes a PGSQL database for use with the SHiFT system. Default permission values are used for now. 

# Using the docker compose files

## shift-base
```
docker-compose -f dockerCompose/shift.yml up -d
```

## postgres:shift
```
docker-compose -f dockerCompose/psql.yml up -d
```

## User setup
After the shift containers go up, run the following script from Scripts:

`docker_db_setup.sh`, to initialize and restart the shift_broker container with the default trader profiles.
