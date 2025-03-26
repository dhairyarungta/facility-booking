package utils

import (
	"bytes"
	"encoding/binary"
	"errors"
	"fmt"
)

type Day byte

type Hourminute [4]byte

type TimeSlot struct {
	StartTime Hourminute
	EndTime Hourminute
}

type Availability struct {
	Day Day
	TimeSlots []TimeSlot
}

const (
	Monday    Day = iota + '0'
	Tuesday   
	Wednesday 
	Thursday  
	Friday    
	Saturday  
	Sunday    
)

type marshalledMessage struct {
	reqId uint32
	uid uint32
	op uint32
	payLoadLen uint32
	payload []byte
}




//~ note that not all fields will be populated. only those relevant to op code, handled by marshall and rest will be zeroed
type UnMarshalledRequestMessage struct {
	ReqId uint32
	Uid uint32
	Op uint32
	Days []Day
	FacilityName string
	StartTime Hourminute
	EndTime Hourminute
	Offset int32
}


type UnMarshalledReplyMessage struct{
	Uid uint32
	Op uint32
	// ErrorCode uint32 Op code is error code on client side
	Capacity uint32
	FacilityNames []string
	Availabilities []Availability

}

func Marshal(req *UnMarshalledRequestMessage) ([]byte,error){
	var buf bytes.Buffer
	switch req.Op {
	case 101:
		facilityNameLen := uint32(len(req.FacilityName))

		for _,data := range []interface{}{
			facilityNameLen,
			[]byte(req.FacilityName),
		}{
			err := binary.Write(&buf,binary.BigEndian,data)
			if err!=nil{
				return nil,err
			}
		}

		for _,data := range req.Days {
			err := binary.Write(&buf,binary.BigEndian,data)
			if err!=nil{
				return nil,err
			}
		}

	case 102:
		facilityNameLen := uint32(len(req.FacilityName))
		facilityName := req.FacilityName

		startTime := req.StartTime
		endTime := req.EndTime

		for _,data := range []interface{}{
			facilityNameLen,
			facilityName,
			startTime,
			endTime,
		}{
			err := binary.Write(&buf,binary.BigEndian,data)
			if err!=nil{
				return nil,err
			}

		}
		
		
	//TODO
	case 103:
	//TODO
	case 104:
	case 105:
		facilityNameLen := uint32(len(req.FacilityName))
		facilityName := req.FacilityName

		for _, data := range []interface{}{
			facilityNameLen,
			[]byte(facilityName),
		}{
			err := binary.Write(&buf,binary.BigEndian,data)
			if err!=nil{
				return nil,err
			}
		}

	case 106:
		break
	case 107:
		break
	default:
		return nil, errors.New("invalid op code")
		
	}

	payloadLen := uint32(buf.Len())
	var networkBuf bytes.Buffer

	for _,data := range []interface{}{
		req.ReqId,	
		req.Uid,
		req.Op,
		payloadLen,
		buf.Bytes(),
	} {
		if err:= binary.Write(&networkBuf,binary.BigEndian,data); err!=nil{
			return nil, err

		}
	}
	return networkBuf.Bytes(),nil
}


func UnMarshal(incomingPacket []byte) (*UnMarshalledReplyMessage, error){
	buf := bytes.NewBuffer(incomingPacket)

	var networkMessage marshalledMessage

	for _, data := range []interface{}{
		&networkMessage.reqId,
		&networkMessage.uid,
		&networkMessage.op,
		&networkMessage.payLoadLen,
	}{
		if err:= binary.Read(buf,binary.BigEndian,data); err!=nil{
			return nil, err
		}

	}
	var newReply UnMarshalledReplyMessage 
	newReply.Op = networkMessage.op
	newReply.Uid = networkMessage.uid
	fmt.Printf("Payload Len %v\n", networkMessage.payLoadLen)


	if(networkMessage.payLoadLen <= 0){
		return &newReply,nil
	}
	switch networkMessage.op {
	case 101:
		var numDays uint32
		err := binary.Read(buf,binary.BigEndian,&numDays)
		if err!=nil{
			fmt.Println("unable to read byte")
			return nil,err
		}

		availabilities := make([]Availability,numDays)

		for i := range numDays{
			availability := &availabilities[i]
			if err = binary.Read(buf,binary.BigEndian,&availability.Day); err!=nil{
				return nil, err
			}
			var numOfAvailTimeSlots uint32
			if err = binary.Read(buf,binary.BigEndian,&numOfAvailTimeSlots); err!=nil{
				return nil, err
			}
			for _ = range numOfAvailTimeSlots{
				var startTime [4]byte
				if err = binary.Read(buf,binary.LittleEndian,&startTime); err!=nil{
					return nil, err
				}
				var endTime [4]byte
				if err = binary.Read(buf,binary.LittleEndian,&endTime); err !=nil{

				}

				timeSlot := TimeSlot{
					StartTime: startTime,
					EndTime: endTime,
				}

				availability.TimeSlots = append(availability.TimeSlots, timeSlot)
			}

		}

		newReply.Availabilities = availabilities
	case 102:
		break
	case 103:
		break
	case 104:
		break
	case 105:
		var capacity uint32
		if err := binary.Read(buf,binary.BigEndian,&capacity); err!=nil{
			return nil,err
		}
		newReply.Capacity = capacity
	case 106:
		break

		
	}

	return &newReply,nil
	
}

