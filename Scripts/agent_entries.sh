for i in $(seq -f "%03g" 1 220)
do
    BrokerageCenter -u agent$i -p password -i Agent $i agent@shift -t
done
