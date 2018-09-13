---------------------------------------------------------------

DROP DATABASE IF EXISTS "shift_brokeragecenter";
CREATE DATABASE "shift_brokeragecenter" WITH OWNER "hanlonpgsql4" TEMPLATE template1 ENCODING UTF8;

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

-- Table: public.forum

-- DROP TABLE public.forum;

CREATE TABLE public.forum
(
  user_name character varying,
  comments character varying,
  subject character varying
)
WITH (
  OIDS=FALSE
);
ALTER TABLE public.forum
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

-- Table: public.strategies

-- DROP TABLE public.strategies;

CREATE TABLE public.strategies
(
  user_name character varying,
  order_type character varying,
  stock_name character varying,
  "interval" character varying
)
WITH (
  OIDS=FALSE
);
ALTER TABLE public.strategies
  OWNER TO hanlonpgsql4;

---------------------------------------------------------------

-- Sequence: public.test_id_seq

-- DROP SEQUENCE public.test_id_seq;

CREATE SEQUENCE public.test_id_seq
  INCREMENT 1
  MINVALUE 1
  MAXVALUE 9223372036854775807
  START 31
  CACHE 1;
ALTER TABLE public.test_id_seq
  OWNER TO hanlonpgsql4;

---------------------------------------------------------------

-- Table: public.traders

-- DROP TABLE public.traders;

CREATE TABLE public.traders
(
  id bigint NOT NULL DEFAULT nextval('test_id_seq'::regclass),
  username character varying(40) NOT NULL,
  password character varying(40) NOT NULL,
  role character varying(20) NOT NULL DEFAULT 'student'::character varying,
  email character varying(40) NOT NULL,
  sessionid character varying(60),
  CONSTRAINT traders_pkey PRIMARY KEY (id)
)
WITH (
  OIDS=FALSE
);
ALTER TABLE public.traders
  OWNER TO hanlonpgsql4;

---------------------------------------------------------------
