---------------------------------------------------------------

\connect shift_brokeragecenter;

---------------------------------------------------------------

-- Table: public.traders

DROP TABLE IF EXISTS public.traders CASCADE;

CREATE TABLE public.traders
(
  id serial,
  firstname character varying(40) NOT NULL,
  lastname character varying(40) NOT NULL,
  username character varying(40) NOT NULL,
  password character varying(40) NOT NULL,
  role character varying(20) NOT NULL DEFAULT 'student'::character varying,
  email character varying(40) NOT NULL,
  CONSTRAINT traders_pkey PRIMARY KEY (id)
)
WITH (
  OIDS=FALSE
);
ALTER TABLE public.traders
  OWNER TO hanlonpgsql4;

INSERT INTO public.traders (firstname, lastname, username, password, email)
  VALUES ('Demo', 'Client', 'democlient', 'password', 'democlient@shift');

INSERT INTO public.traders (firstname, lastname, username, password, email)
  VALUES ('Web', 'Client', 'webclient', 'password', 'webclient@shift');

INSERT INTO public.traders (firstname, lastname, username, password, email)
  VALUES ('Qt', 'Client', 'qtclient', 'password', 'qtclient@shift');

DO $$

DECLARE
  i integer := 0;

BEGIN

  WHILE i < 10 LOOP
    i := i + 1;
    INSERT INTO public.traders (firstname, lastname, username, password, email)
      VALUES ('Test', LPAD(i::text, 3, '0'), 'test' || LPAD(i::text, 3, '0'), 'password', 'test@shift');
  END LOOP;

END $$;

---------------------------------------------------------------

-- Table: public.trade_record

-- DROP TABLE IF EXISTS public.trade_record;

-- CREATE TABLE public.trade_record
-- (
--   real_time timestamp without time zone,
--   execute_time timestamp without time zone,
--   symbol character varying(15),
--   price real,
--   size integer,
--   trader_id_1 bigint NOT NULL,
--   trader_id_2 bigint NOT NULL,
--   order_id_1 character varying(20) NOT NULL,
--   order_id_2 character varying(20),
--   order_type_1 character varying(2),
--   order_type_2 character varying(2),
--   time1 time without time zone,
--   time2 time without time zone,
--   decision character varying(10),
--   destination character varying(10),
--   CONSTRAINT trade_record_pkey PRIMARY KEY (order_id_1),
--   CONSTRAINT trade_record_fkey FOREIGN KEY (trader_id_1)
--       REFERENCES public.traders (id) MATCH SIMPLE
--       ON UPDATE NO ACTION ON DELETE NO ACTION
-- )
-- WITH (
--   OIDS=FALSE
-- );
-- ALTER TABLE public.trade_record
--   OWNER TO hanlonpgsql4;

---------------------------------------------------------------
