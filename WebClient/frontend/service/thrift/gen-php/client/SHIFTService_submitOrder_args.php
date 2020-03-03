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

class SHIFTService_submitOrder_args
{
    static public $isValidate = false;

    static public $_TSPEC = array(
        1 => array(
            'var' => 'username',
            'isRequired' => false,
            'type' => TType::STRING,
        ),
        2 => array(
            'var' => 'orderType',
            'isRequired' => false,
            'type' => TType::STRING,
        ),
        3 => array(
            'var' => 'orderSymbol',
            'isRequired' => false,
            'type' => TType::STRING,
        ),
        4 => array(
            'var' => 'orderSize',
            'isRequired' => false,
            'type' => TType::I32,
        ),
        5 => array(
            'var' => 'orderPrice',
            'isRequired' => false,
            'type' => TType::DOUBLE,
        ),
        6 => array(
            'var' => 'orderID',
            'isRequired' => false,
            'type' => TType::STRING,
        ),
    );

    /**
     * @var string
     */
    public $username = null;
    /**
     * @var string
     */
    public $orderType = null;
    /**
     * @var string
     */
    public $orderSymbol = null;
    /**
     * @var int
     */
    public $orderSize = null;
    /**
     * @var double
     */
    public $orderPrice = null;
    /**
     * @var string
     */
    public $orderID = null;

    public function __construct($vals = null)
    {
        if (is_array($vals)) {
            if (isset($vals['username'])) {
                $this->username = $vals['username'];
            }
            if (isset($vals['orderType'])) {
                $this->orderType = $vals['orderType'];
            }
            if (isset($vals['orderSymbol'])) {
                $this->orderSymbol = $vals['orderSymbol'];
            }
            if (isset($vals['orderSize'])) {
                $this->orderSize = $vals['orderSize'];
            }
            if (isset($vals['orderPrice'])) {
                $this->orderPrice = $vals['orderPrice'];
            }
            if (isset($vals['orderID'])) {
                $this->orderID = $vals['orderID'];
            }
        }
    }

    public function getName()
    {
        return 'SHIFTService_submitOrder_args';
    }


    public function read($input)
    {
        $xfer = 0;
        $fname = null;
        $ftype = 0;
        $fid = 0;
        $xfer += $input->readStructBegin($fname);
        while (true) {
            $xfer += $input->readFieldBegin($fname, $ftype, $fid);
            if ($ftype == TType::STOP) {
                break;
            }
            switch ($fid) {
                case 1:
                    if ($ftype == TType::STRING) {
                        $xfer += $input->readString($this->username);
                    } else {
                        $xfer += $input->skip($ftype);
                    }
                    break;
                case 2:
                    if ($ftype == TType::STRING) {
                        $xfer += $input->readString($this->orderType);
                    } else {
                        $xfer += $input->skip($ftype);
                    }
                    break;
                case 3:
                    if ($ftype == TType::STRING) {
                        $xfer += $input->readString($this->orderSymbol);
                    } else {
                        $xfer += $input->skip($ftype);
                    }
                    break;
                case 4:
                    if ($ftype == TType::I32) {
                        $xfer += $input->readI32($this->orderSize);
                    } else {
                        $xfer += $input->skip($ftype);
                    }
                    break;
                case 5:
                    if ($ftype == TType::DOUBLE) {
                        $xfer += $input->readDouble($this->orderPrice);
                    } else {
                        $xfer += $input->skip($ftype);
                    }
                    break;
                case 6:
                    if ($ftype == TType::STRING) {
                        $xfer += $input->readString($this->orderID);
                    } else {
                        $xfer += $input->skip($ftype);
                    }
                    break;
                default:
                    $xfer += $input->skip($ftype);
                    break;
            }
            $xfer += $input->readFieldEnd();
        }
        $xfer += $input->readStructEnd();
        return $xfer;
    }

    public function write($output)
    {
        $xfer = 0;
        $xfer += $output->writeStructBegin('SHIFTService_submitOrder_args');
        if ($this->username !== null) {
            $xfer += $output->writeFieldBegin('username', TType::STRING, 1);
            $xfer += $output->writeString($this->username);
            $xfer += $output->writeFieldEnd();
        }
        if ($this->orderType !== null) {
            $xfer += $output->writeFieldBegin('orderType', TType::STRING, 2);
            $xfer += $output->writeString($this->orderType);
            $xfer += $output->writeFieldEnd();
        }
        if ($this->orderSymbol !== null) {
            $xfer += $output->writeFieldBegin('orderSymbol', TType::STRING, 3);
            $xfer += $output->writeString($this->orderSymbol);
            $xfer += $output->writeFieldEnd();
        }
        if ($this->orderSize !== null) {
            $xfer += $output->writeFieldBegin('orderSize', TType::I32, 4);
            $xfer += $output->writeI32($this->orderSize);
            $xfer += $output->writeFieldEnd();
        }
        if ($this->orderPrice !== null) {
            $xfer += $output->writeFieldBegin('orderPrice', TType::DOUBLE, 5);
            $xfer += $output->writeDouble($this->orderPrice);
            $xfer += $output->writeFieldEnd();
        }
        if ($this->orderID !== null) {
            $xfer += $output->writeFieldBegin('orderID', TType::STRING, 6);
            $xfer += $output->writeString($this->orderID);
            $xfer += $output->writeFieldEnd();
        }
        $xfer += $output->writeFieldStop();
        $xfer += $output->writeStructEnd();
        return $xfer;
    }
}
