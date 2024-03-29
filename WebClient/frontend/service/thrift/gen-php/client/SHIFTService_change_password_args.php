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

class SHIFTService_change_password_args
{
    static public $isValidate = false;

    static public $_TSPEC = array(
        1 => array(
            'var' => 'cur_password',
            'isRequired' => false,
            'type' => TType::STRING,
        ),
        2 => array(
            'var' => 'new_password',
            'isRequired' => false,
            'type' => TType::STRING,
        ),
        3 => array(
            'var' => 'username',
            'isRequired' => false,
            'type' => TType::STRING,
        ),
    );

    /**
     * @var string
     */
    public $cur_password = null;
    /**
     * @var string
     */
    public $new_password = null;
    /**
     * @var string
     */
    public $username = null;

    public function __construct($vals = null)
    {
        if (is_array($vals)) {
            if (isset($vals['cur_password'])) {
                $this->cur_password = $vals['cur_password'];
            }
            if (isset($vals['new_password'])) {
                $this->new_password = $vals['new_password'];
            }
            if (isset($vals['username'])) {
                $this->username = $vals['username'];
            }
        }
    }

    public function getName()
    {
        return 'SHIFTService_change_password_args';
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
                        $xfer += $input->readString($this->cur_password);
                    } else {
                        $xfer += $input->skip($ftype);
                    }
                    break;
                case 2:
                    if ($ftype == TType::STRING) {
                        $xfer += $input->readString($this->new_password);
                    } else {
                        $xfer += $input->skip($ftype);
                    }
                    break;
                case 3:
                    if ($ftype == TType::STRING) {
                        $xfer += $input->readString($this->username);
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
        $xfer += $output->writeStructBegin('SHIFTService_change_password_args');
        if ($this->cur_password !== null) {
            $xfer += $output->writeFieldBegin('cur_password', TType::STRING, 1);
            $xfer += $output->writeString($this->cur_password);
            $xfer += $output->writeFieldEnd();
        }
        if ($this->new_password !== null) {
            $xfer += $output->writeFieldBegin('new_password', TType::STRING, 2);
            $xfer += $output->writeString($this->new_password);
            $xfer += $output->writeFieldEnd();
        }
        if ($this->username !== null) {
            $xfer += $output->writeFieldBegin('username', TType::STRING, 3);
            $xfer += $output->writeString($this->username);
            $xfer += $output->writeFieldEnd();
        }
        $xfer += $output->writeFieldStop();
        $xfer += $output->writeStructEnd();
        return $xfer;
    }
}
