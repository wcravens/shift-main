BrokerageCenter -u democlient -p password -i Demo Client democlient@shift -s
BrokerageCenter -u webclient -p password -i Web Client webclient@shift -s
BrokerageCenter -u qtclient -p password -i Qt Client qtclient@shift -s

for i in $(seq -f "%03g" 1 10)
do
    BrokerageCenter -u test$i -p password -i Test $i test@shift -s
done
