---------------------------------------------------------------

\connect shift_brokeragecenter;

---------------------------------------------------------------

-- Table: public.client_information

DO $$

DECLARE
  i integer := -1; -- agent000 

BEGIN

  WHILE i < 220 LOOP
    i := i + 1;
    INSERT INTO public.client_information
      VALUES (2000 + i, 'Agent', LPAD(i::text, 3, '0'), 'agent' || LPAD(i::text, 3, '0'), 'password', NULL, NULL, NULL, NULL);
  END LOOP;

END $$;

---------------------------------------------------------------
