from sqlalchemy import create_engine
from sqlalchemy.orm import sessionmaker

class ScoreKeeper():

  def __init__(self, user,password, dbhost='localhost'):
    self._user = user
    self._dbhost = dbhost
    #self._dbname = dbname

    self._engine = create_engine(f"postgresql://{user}:{password}@dbhost")
    self._sessionmaker = sessionmaker(bind=self._engine)
    self.session = self._sessionmaker()


  def isActive(self):
    return True