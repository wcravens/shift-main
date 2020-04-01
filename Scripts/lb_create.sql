\connect shift_brokeragecenter;

DROP TABLE IF EXISTS public.leaderboard;

CREATE TABLE public.leaderboard
(
  id UUID DEFAULT uuid_generate_v4(), 
  start_date timestamp without time zone,
  end_date timestamp without time zone,
  contest_day integer,
  rank integer,
  trader_id UUID NOT NULL,
  eod_buying_power real,
  eod_traded_shares integer,
  eod_pl real,
  fees real,
  total_orders integer,
  penalty_total real,
  pl_2 real
)
