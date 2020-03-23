import sys
sys.path.append("shiftpy")

from shift_service import SHIFTService
from thrift.transport import TSocket
from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol


trans = TSocket.TSocket('localhost', 9090)
trans = TTransport.TBufferedTransport(trans)

proto = TBinaryProtocol.TBinaryProtocol(trans)
client = SHIFTService.Client(proto)

trans.open()

msg = None
sDate = "2020-03-16"
eDate = "2020-03-17"

print("w/ params:")
msg = client.getThisLeaderboard('', '')
print(msg)

