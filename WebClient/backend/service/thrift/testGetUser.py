import sys

sys.path.append("shiftpy")

from shift_service import SHIFTService
from thrift.transport import TSocket
from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol


trans = TSocket.TSocket("localhost", 9090)
trans = TTransport.TBufferedTransport(trans)

proto = TBinaryProtocol.TBinaryProtocol(trans)
client = SHIFTService.Client(proto)

trans.open()

msg = None

# msg = client.is_login("e84cbe1b-300e-4b3e-8174-b7125297fa45")
msg = client.get_user_by_username("webclient")

print(msg)
