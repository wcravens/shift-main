# Docker Deployment

There are two Dockerfiles used in the dockerization of SHiFT.
The docker files are meant to be utilized with the configurations provided in the dockerCompose directory.

1. Dockerfile -  which pertains to the base system, and holds the executables for all 3 main modules
2. DockerPsql - which sets up and initializes a PGSQL database for use with the SHiFT system. Default permission values are used for now. 

# Building the docker images

## Building shift-base
```
docker build -f Docker/dockerFiles/Dockerfile . --tag shift-base
```

## Building postgres:shift
The build file for postgresql:shift heavily depends on several scripts from the `Scripts/` directory:

1. db_setup.sh
2. de_instances.sql
3. bc_instances.sql
4. db_setup.sql - Initializes some DB settings
5. docker_db_setup.sh - To launch the default users for demo purposes
```
docker build -f Docker/dockerFiles/DockerPsql . --tag postgresql:shift
```

### Setting up postgres:shift
By default, docker postgres handles persistence by mounting the database filesystem onto `/var/lib/postgresql/data`.

If the location is desired to be changed, then mapping a local directory to the volume location of `/var/lib/postgresql/data` on the container is necessary.

# Running a container from the images

The docker images can be run independently, or via docker-compose. To run them in the terminal, run as follows:

## postgresql:shift
```
docker run -it --network host --volume ~/desired/local/directory:/var/lib/postgresql/data \
[BrokerageCenter|DatafeedEngine|MatchingEngine] [Options]
```

## shift-base
```
docker run -it shift-base --network host \
[BrokerageCenter|DatafeedEngine|MatchingEngine] [Options]
```


