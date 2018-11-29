BrokerageCenter -u democlient -p password -i Demo Client democlient@shift
BrokerageCenter -u webclient -p password -i Web Client webclient@shift
BrokerageCenter -u qtclient -p password -i Qt Client qtclient@shift

for i in $(seq -f "%03g" 1 10)
do
    BrokerageCenter -u test$i -p password -i Test $i test@shift
done
