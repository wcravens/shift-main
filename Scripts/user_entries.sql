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
