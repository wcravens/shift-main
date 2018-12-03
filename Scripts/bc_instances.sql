---------------------------------------------------------------

DROP DATABASE IF EXISTS "shift_brokeragecenter";
CREATE DATABASE "shift_brokeragecenter" WITH OWNER "hanlonpgsql4" TEMPLATE template1 ENCODING UTF8;

---------------------------------------------------------------

\connect shift_brokeragecenter;

-- http://www.postgresqltutorial.com/postgresql-uuid/
<<<<<<< HEAD
CREATE EXTENSION IF NOT EXISTS "uuid-ossp"; -- This must come after the connection
=======
CREATE EXTENSION IF NOT EXISTS "uuid-ossp";
>>>>>>> bae017d9de608e0169d8e91fe701e6d8e8a6b5c4

---------------------------------------------------------------

-- Table: PUBLIC.buying_power

-- DROP TABLE PUBLIC.buying_power;

CREATE TABLE PUBLIC.buying_power
(
  tb_clientid VARCHAR NOT NULL,
  tb_bp DOUBLE PRECISION,
  CONSTRAINT buying_power_pkey PRIMARY KEY (tb_clientid)
)
WITH (
  OIDS=FALSE
);
ALTER TABLE PUBLIC.buying_power
  OWNER TO hanlonpgsql4;

---------------------------------------------------------------

-- Table: PUBLIC.holdings_xyz

-- DROP TABLE PUBLIC.holdings_xyz;

CREATE TABLE PUBLIC.holdings_xyz
(
  ht_clientid VARCHAR NOT NULL,
  ht_shares DOUBLE PRECISION,
  ht_avgprice DOUBLE PRECISION,
  CONSTRAINT holdings_xyz_pkey PRIMARY KEY (ht_clientid)
)
WITH (
  OIDS=FALSE
);
ALTER TABLE PUBLIC.holdings_xyz
  OWNER TO hanlonpgsql4;

---------------------------------------------------------------

-- Table: PUBLIC.web_traders

-- DROP TABLE PUBLIC.web_traders;

CREATE TABLE PUBLIC.web_traders
(
  id SERIAL,
  portfolio_id INTEGER DEFAULT -1,
  username VARCHAR(40) NOT NULL UNIQUE,
  password VARCHAR(40) NOT NULL,
  role VARCHAR(20) NOT NULL DEFAULT 'student'::VARCHAR,
  email VARCHAR(40) NOT NULL,
  sessionid VARCHAR(40),

  CONSTRAINT web_traders_pkey PRIMARY KEY (id)
  -- CONSTRAINT web_traders_fkey FOREIGN KEY (portfolio_id)
  --   REFERENCES PUBLIC.portfolio_summary (portfolio_id) MATCH SIMPLE
  --   ON UPDATE NO ACTION ON DELETE NO ACTION
)
WITH (
  OIDS=FALSE
);
ALTER TABLE PUBLIC.web_traders
  OWNER TO hanlonpgsql4;

---------------------------------------------------------------

-- Table: public.new_traders

-- DROP TABLE public.new_traders;

CREATE TABLE public.new_traders
(
  id UUID DEFAULT uuid_generate_v4(),
  username VARCHAR(40) NOT NULL,
  password VARCHAR(40) NOT NULL,
  firstname VARCHAR(40) NOT NULL,
  lastname VARCHAR(40) NOT NULL,
  email VARCHAR(40) NOT NULL,
  role VARCHAR(20) NOT NULL DEFAULT 'student'::VARCHAR,
  sessionid VARCHAR(40),
  target BOOLEAN NOT NULL DEFAULT FALSE,

  CONSTRAINT new_traders_pkey PRIMARY KEY (id)
)
WITH (
  OIDS=FALSE
);
ALTER TABLE public.new_traders
  OWNER TO hanlonpgsql4;

---------------------------------------------------------------
