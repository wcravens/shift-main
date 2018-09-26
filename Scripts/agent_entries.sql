---------------------------------------------------------------

\connect shift_brokeragecenter;

---------------------------------------------------------------

-- Table: public.traders

DO $$

DECLARE
  i integer := -1; -- agent000

BEGIN

  WHILE i < 220 LOOP
    i := i + 1;
    INSERT INTO public.traders (firstname, lastname, username, password, email)
      VALUES ('Agent', LPAD(i::text, 3, '0'), 'agent' || LPAD(i::text, 3, '0'), 'password', 'agent@shift');
  END LOOP;

END $$;

---------------------------------------------------------------
