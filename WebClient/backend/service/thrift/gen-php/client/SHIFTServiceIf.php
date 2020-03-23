<?php
namespace client;

/**
 * Autogenerated by Thrift Compiler (0.12.0)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
use Thrift\Base\TBase;
use Thrift\Type\TType;
use Thrift\Type\TMessageType;
use Thrift\Exception\TException;
use Thrift\Exception\TProtocolException;
use Thrift\Protocol\TProtocol;
use Thrift\Protocol\TBinaryProtocolAccelerated;
use Thrift\Exception\TApplicationException;

interface SHIFTServiceIf
{
    /**
     * @param string $username
     * @param string $orderType
     * @param string $orderSymbol
     * @param int $orderSize
     * @param double $orderPrice
     * @param string $orderID
     */
    public function submitOrder($username, $orderType, $orderSymbol, $orderSize, $orderPrice, $orderID);
    /**
     * @param string $username
     */
    public function webClientSendUsername($username);
    /**
     * @param string $username
     * @param string $password
     * @return string
     */
    public function webUserLoginV2($username, $password);
    /**
     * @param string $username
     */
    public function webUserLogin($username);
    /**
     * @return string
     */
    public function getAllTraders();
    /**
     * @param string $startDate
     * @param string $endDate
     * @return string
     */
    public function getThisLeaderboard($startDate, $endDate);
}
