from scorekeeper import ScoreKeeper
import argparse
from getpass import getpass

def main(keeper):
  print(keeper.isActive())

if __name__ == "__main__":

  parser = argparse.ArgumentParser(description="LeaderboardGenerator - Generates a day's trader rankings")
  parser.add_argument('-u', type=str, help='DB User', dest='user')
  parser.add_argument('-p', '--p', action='store_true', help='DB User password', dest='password')

  args = parser.parse_args()

  if args.password:
    password = getpass()
  
  user = args.user

  main(ScoreKeeper(user, password))