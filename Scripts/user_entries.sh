BrokerageCenter -u democlient -p password -i Demo Client democlient@shift -t
BrokerageCenter -u webclient -p password -i Web Client webclient@shift -t
BrokerageCenter -u qtclient -p password -i Qt Client qtclient@shift -t

for i in $(seq -f "%03g" 1 10)
do
    BrokerageCenter -u test$i -p password -i Test $i test@shift -t
done
