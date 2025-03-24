package utils

import (
	"bytes"
	"encoding/binary"
	"unsafe"
)


type NetworkMessage interface {
	Marshal()([]byte,error)
	UnMarshal([]byte) (NetworkMessage,error)
}

type Day byte

const (
	Monday    Day = 0
	Tuesday   Day = 1
	Wednesday Day = 2
	Thursday  Day = 3
	Friday    Day = 4
	Saturday  Day = 5
	Sunday    Day = 6
)

type Request struct {
	Uid [8]byte
	Days [7]Day
	Op byte
	FacilityName [4]byte
	StartTime [4]byte
	EndTime [4]byte
	Offset [4]byte
}

//! Marshalling/Unmarshalling assumes network representation of fields is packed
func (req *Request) Marshal() ([]byte,error){
	buffer := make([]byte,unsafe.Sizeof(req))
	byteWriter := bytes.NewBuffer(buffer)

	for _,data := range []interface{}{
		req.Uid,
		req.Days,
		req.Op,
		req.FacilityName,
		req.StartTime,
		req.EndTime,
		
	} {
		err := binary.Write(byteWriter,binary.BigEndian,data)
		if err != nil{
			return nil, err
		}
	}

	return byteWriter.Bytes(), nil
}


func (req *Request) UnMarshal(data []byte) (NetworkMessage,error){
	byteReader := bytes.NewBuffer(data)

	for _,data := range[]interface{}{
		&req.Uid,
		&req.Days,
		&req.Op,
		&req.FacilityName,
		&req.StartTime,
		&req.EndTime,
	} {
		err := binary.Read(byteReader,binary.LittleEndian,data)
		if err!=nil{
			return nil,err
		}

	}

	return req,nil
}

type Response struct {
	Availabilties [8]byte
	Uid [8]byte
	NumAvail uint32
	ErrorCode uint32
	Capacity uint32
	Day Day
}

func (resp *Response) Marshal() ([]byte, error){
	buffer := make([]byte,unsafe.Sizeof(resp))
	byteWriter := bytes.NewBuffer(buffer)

	for _,data := range []interface{}{
		resp.Availabilties,
		resp.Uid,
		resp.NumAvail,
		resp.ErrorCode,
		resp.Capacity,
		resp.Day,
	} {

		err := binary.Write(byteWriter,binary.BigEndian,data)
		if err!=nil{
			return nil,err
		}

	}

	return byteWriter.Bytes(),nil
}

func (resp *Response) UnMarshal( data []byte) (NetworkMessage, error){
	byteReader := bytes.NewBuffer(data)
	for _,data := range []interface{}{
		&resp.Availabilties,
		&resp.Uid,
		&resp.NumAvail,
		&resp.ErrorCode,
		&resp.Capacity,
		&resp.Day,
	} {
		err := binary.Read(byteReader,binary.LittleEndian,data)
		if err!=nil{
			return nil, err
		}

	}

	return resp,nil

}