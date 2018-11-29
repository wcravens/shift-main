---------------------------------------------------------------

DROP DATABASE IF EXISTS "shift_brokeragecenter";
CREATE DATABASE "shift_brokeragecenter" WITH OWNER "hanlonpgsql4" TEMPLATE template1 ENCODING UTF8;

-- http://www.postgresqltutorial.com/postgresql-uuid/
CREATE EXTENSION IF NOT EXISTS "uuid-ossp";

---------------------------------------------------------------

\connect shift_brokeragecenter;

---------------------------------------------------------------

-- Table: public.buying_power

-- DROP TABLE public.buying_power;

CREATE TABLE public.buying_power
(
  tb_clientid character varying NOT NULL,
  tb_bp double precision,
  CONSTRAINT buying_power_pkey PRIMARY KEY (tb_clientid)
)
WITH (
  OIDS=FALSE
);
ALTER TABLE public.buying_power
  OWNER TO hanlonpgsql4;

---------------------------------------------------------------

-- Table: public.holdings_xyz

-- DROP TABLE public.holdings_xyz;

CREATE TABLE public.holdings_xyz
(
  ht_clientid character varying NOT NULL,
  ht_shares double precision,
  ht_avgprice double precision,
  CONSTRAINT holdings_xyz_pkey PRIMARY KEY (ht_clientid)
)
WITH (
  OIDS=FALSE
);
ALTER TABLE public.holdings_xyz
  OWNER TO hanlonpgsql4;

---------------------------------------------------------------

-- Table: public.web_traders

-- DROP TABLE public.web_traders;

CREATE TABLE public.web_traders
(
  id SERIAL,
  username character varying(40) NOT NULL,
  password character varying(40) NOT NULL,
  role character varying(20) NOT NULL DEFAULT 'student'::character varying,
  email character varying(40) NOT NULL,
  sessionid character varying(40),
  CONSTRAINT web_traders_pkey PRIMARY KEY (id)
)
WITH (
  OIDS=FALSE
);
ALTER TABLE public.web_traders
  OWNER TO hanlonpgsql4;

---------------------------------------------------------------

-- Table: public.new_traders

-- DROP TABLE public.new_traders;

CREATE TABLE public.new_traders
(
  id uuid DEFAULT uuid_generate_v4(),
  username character varying(40) NOT NULL,
  password character varying(40) NOT NULL,
  firstname character varying(40) NOT NULL,
  lastname character varying(40) NOT NULL,
  email character varying(40) NOT NULL,
  role character varying(20) NOT NULL DEFAULT 'student'::character varying,
  sessionid character varying(40),
  CONSTRAINT new_traders_pkey PRIMARY KEY (id)
)
WITH (
  OIDS=FALSE
);
ALTER TABLE public.new_traders
  OWNER TO hanlonpgsql4;

---------------------------------------------------------------
