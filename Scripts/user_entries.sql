---------------------------------------------------------------

\connect shift_brokeragecenter;

---------------------------------------------------------------

-- Table: public.client_information

DROP TABLE IF EXISTS public.client_information CASCADE;

CREATE TABLE public.client_information
(
  trader_id_1 character varying(15) NOT NULL,
  trader_fname character varying(25),
  trader_lname character varying(25),
  trader_username character varying(15),
  trader_password character varying(100),
  trader_buyingp real,
  trader_email character varying(50),
  trader_gender integer,
  trader_role character varying(10),
  CONSTRAINT client_information_pkey PRIMARY KEY (trader_id_1)
)
WITH (
  OIDS=FALSE
);
ALTER TABLE public.client_information
  OWNER TO hanlonpgsql4;

INSERT INTO public.client_information
  VALUES ('0000', 'Demo', 'Client', 'democlient', 'password', NULL, NULL, NULL, NULL);

INSERT INTO public.client_information
  VALUES ('0001', 'Web', 'Client', 'webclient', 'password', NULL, NULL, NULL, NULL);

INSERT INTO public.client_information
  VALUES ('0002', 'Qt', 'Client', 'qtclient', 'password', NULL, NULL, NULL, NULL);

DO $$

DECLARE
  i integer := 0; 

BEGIN

  WHILE i < 10 LOOP
    i := i + 1;
    INSERT INTO public.client_information
      VALUES (1000 + i, 'Test', LPAD(i::text, 3, '0'), 'test' || LPAD(i::text, 3, '0'), 'password', NULL, NULL, NULL, NULL);
  END LOOP;

END $$;

---------------------------------------------------------------

-- Table: public.trade_record

DROP TABLE IF EXISTS public.trade_record;

CREATE TABLE public.trade_record
(
  real_time timestamp without time zone,
  execute_time timestamp without time zone,
  symbol character varying(15),
  price real,
  size integer,
  trader_id_1 character varying(15),
  trader_id_2 character varying(15),
  order_id_1 character varying(20) NOT NULL,
  order_id_2 character varying(20),
  order_type_1 character varying(2),
  order_type_2 character varying(2),
  time1 time without time zone,
  time2 time without time zone,
  decision character varying(10),
  destination character varying(10),
  CONSTRAINT trade_record_pkey PRIMARY KEY (order_id_1),
  CONSTRAINT trade_record_fkey FOREIGN KEY (trader_id_1)
      REFERENCES public.client_information (trader_id_1) MATCH SIMPLE
      ON UPDATE NO ACTION ON DELETE NO ACTION
)
WITH (
  OIDS=FALSE
);
ALTER TABLE public.trade_record
  OWNER TO hanlonpgsql4;

---------------------------------------------------------------
