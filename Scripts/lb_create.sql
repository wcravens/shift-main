\connect shift_brokeragecenter;

DROP TABLE IF EXISTS public.leaderboard;

CREATE TABLE public.leaderboard
(
  id UUID DEFAULT uuid_generate_v4(),
  start_date timestamp without time zone,
  end_date timestamp without time zone,
  rank integer,
  user_id UUID NOT NULL,
  eod_buying_power real,
  eod_traded_shares integer,
  eod_pl real,
  eod_earnings real
)
