---------------------------------------------------------------

\connect shift_brokeragecenter;

---------------------------------------------------------------

-- Table: PUBLIC.traders

DROP TABLE IF EXISTS PUBLIC.traders CASCADE;

CREATE TABLE PUBLIC.traders
(
  id SERIAL,
  portfolio_id INTEGER DEFAULT -1,
  firstname VARCHAR(40) NOT NULL,
  lastname VARCHAR(40) NOT NULL,
  target_id VARCHAR(40) NOT NULL UNIQUE,
  password VARCHAR(40) NOT NULL,
  role VARCHAR(20) NOT NULL DEFAULT 'student'::VARCHAR,
  email VARCHAR(40) NOT NULL,

  CONSTRAINT traders_pkey PRIMARY KEY (id)
  -- CONSTRAINT traders_fkey FOREIGN KEY (portfolio_id)
  --   REFERENCES PUBLIC.portfolio_summary (portfolio_id) MATCH SIMPLE
  --   ON UPDATE NO ACTION ON DELETE NO ACTION
)
WITH (
  OIDS=FALSE
);
ALTER TABLE PUBLIC.traders
  OWNER TO hanlonpgsql4;

INSERT INTO PUBLIC.traders (firstname, lastname, target_id, password, email)
  VALUES ('Demo', 'Client', 'democlient', 'password', 'democlient@shift');

INSERT INTO PUBLIC.traders (firstname, lastname, target_id, password, email)
  VALUES ('Web', 'Client', 'webclient', 'password', 'webclient@shift');

INSERT INTO PUBLIC.traders (firstname, lastname, target_id, password, email)
  VALUES ('Qt', 'Client', 'qtclient', 'password', 'qtclient@shift');

DO $$

DECLARE
  i INTEGER := 0;

BEGIN

  WHILE i < 10 LOOP
    i := i + 1;
    INSERT INTO PUBLIC.traders (firstname, lastname, target_id, password, email)
      VALUES ('Test', LPAD(i::TEXT, 3, '0'), 'test' || LPAD(i::TEXT, 3, '0'), 'password', 'test@shift');
  END LOOP;

END $$;


---------------------------------------------------------------
