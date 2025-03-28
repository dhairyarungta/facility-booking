package utils

import (
	"bytes"
	"encoding/binary"
	"errors"
	"fmt"

	"github.com/davecgh/go-spew/spew"
)

type Day byte

type HourMinutes [4]byte

type TimeSlot struct {
	StartTime HourMinutes
	EndTime   HourMinutes
}

type Availability struct {
	Day       Day
	TimeSlots []TimeSlot
}

const (
	Monday Day = iota + '0'
	Tuesday
	Wednesday
	Thursday
	Friday
	Saturday
	Sunday
)

type marshalledMessage struct {
	reqId      uint32
	uid        uint32
	op         uint32
	payLoadLen uint32
	payload    []byte
}

// ~ note that not all fields will be populated. only those relevant to op code, handled by marshall and rest will be zeroed
type UnMarshalledRequestMessage struct {
	ReqId        uint32
	Uid          uint32
	Op           uint32
	Days         []Day
	FacilityName string
	StartTime    HourMinutes
	EndTime      HourMinutes
	Offset       int32
}

type UnMarshalledReplyMessage struct {
	Uid uint32
	Op  uint32
	// ErrorCode uint32 Op code is error code on client side
	Capacity       uint32
	FacilityNames  []string
	Availabilities []Availability
}

//function will be used for converting server representation to client side for easy format
func MinutesToHourMinutes(dayMinutes [4]byte, incomingFromNetwork bool) HourMinutes{
	var parsedDayMinutes uint32

	if(incomingFromNetwork){
		parsedDayMinutes = binary.BigEndian.Uint32(dayMinutes[:])
	} else {
		parsedDayMinutes = binary.NativeEndian.Uint32(dayMinutes[:])
	}

	hours := parsedDayMinutes / 60
	minutes := parsedDayMinutes % 60

	tensHour := hours / 10
	onesHour := hours % 10

	tensMinute := minutes / 10
	onesMinute := minutes % 10

	hourMinutes := HourMinutes{
		byte(tensHour),
		byte(onesHour),
		byte(tensMinute),
		byte(onesMinute),
	}
	return hourMinutes
}

func HourMinutesToMinutes( hourMinutes HourMinutes) [4]byte{
	tensHour := uint32(hourMinutes[0]) * 10
	onesHour := uint32(hourMinutes[1])
	hour := tensHour + onesHour

	tensMinute := uint32(hourMinutes[2]) * 10
	onesMinute := uint32(hourMinutes[3])
	minutes := tensMinute + onesMinute

	time := hour * 60 + minutes
	var buf [4]byte
	binary.NativeEndian.PutUint32(buf[:],time)
	return buf
}

func Marshal(req *UnMarshalledRequestMessage) ([]byte, error) {
	var buf bytes.Buffer
	switch req.Op {
	case 101:
		facilityNameLen := uint32(len(req.FacilityName))

		for _, data := range []interface{}{
			facilityNameLen,
			[]byte(req.FacilityName),
		} {
			err := binary.Write(&buf, binary.BigEndian, data)
			if err != nil {
				return nil, err
			}
		}

		for _, data := range req.Days {
			err := binary.Write(&buf, binary.BigEndian, data)
			if err != nil {
				return nil, err
			}
		}
		break
	case 102:
		facilityNameLen := uint32(len(req.FacilityName))
		facilityName := req.FacilityName

		startTime := req.StartTime
		endTime := req.EndTime

		day := req.Days[0]

		for _, data := range []interface{}{
			facilityNameLen,
			[]byte(facilityName),
			day,
			startTime,
			endTime,
		} {
			err := binary.Write(&buf, binary.BigEndian, data)
			if err != nil {
				return nil, err
			}

		}

		break
	case 103:
		offset := req.Offset
		err := binary.Write(&buf, binary.BigEndian, offset)
		if err != nil {
			return nil, err
		}
		break
	case 104:
		facilityNameLen := uint32(len(req.FacilityName))
		facilityName := req.FacilityName
		offset := req.Offset
		for _, data := range []interface{}{
			facilityNameLen,
			[]byte(facilityName),
			offset,
		} {
			err := binary.Write(&buf, binary.BigEndian, data)
			if err != nil {
				return nil, err
			}

		}
		break
	case 105:
		facilityNameLen := uint32(len(req.FacilityName))
		facilityName := req.FacilityName

		for _, data := range []interface{}{
			facilityNameLen,
			[]byte(facilityName),
		} {
			err := binary.Write(&buf, binary.BigEndian, data)
			if err != nil {
				return nil, err
			}
		}
		break
	case 106:
		break
	case 107:
		break
	default:
		return nil, errors.New("invalid op code")

	}

	payloadLen := uint32(buf.Len())
	var networkBuf bytes.Buffer

	for _, data := range []interface{}{
		req.ReqId,
		req.Uid,
		req.Op,
		payloadLen,
		buf.Bytes(),
	} {
		if err := binary.Write(&networkBuf, binary.BigEndian, data); err != nil {
			return nil, err

		}
	}
	fmt.Println(payloadLen)
	spew.Dump(networkBuf.Bytes())
	return networkBuf.Bytes(), nil
}

func UnMarshal(incomingPacket []byte) (*UnMarshalledReplyMessage, error) {
	buf := bytes.NewBuffer(incomingPacket)

	var networkMessage marshalledMessage

	for _, data := range []interface{}{
		&networkMessage.reqId,
		&networkMessage.uid,
		&networkMessage.op,
		&networkMessage.payLoadLen,
	} {
		if err := binary.Read(buf, binary.BigEndian, data); err != nil {
			return nil, err
		}

	}
	var newReply UnMarshalledReplyMessage
	newReply.Op = networkMessage.op
	newReply.Uid = networkMessage.uid
	fmt.Printf("Payload Len %v\n", networkMessage.payLoadLen)

	if networkMessage.payLoadLen <= 0 {
		return &newReply, nil
	}
	switch networkMessage.op {
	case 101:
		var numDays uint32
		err := binary.Read(buf, binary.BigEndian, &numDays)
		if err != nil {
			fmt.Println("unable to read byte")
			return nil, err
		}

		availabilities := make([]Availability, numDays)

		for i := range numDays {
			availability := &availabilities[i]
			if err = binary.Read(buf, binary.BigEndian, &availability.Day); err != nil {
				return nil, err
			}
			var numOfAvailTimeSlots uint32
			if err = binary.Read(buf, binary.BigEndian, &numOfAvailTimeSlots); err != nil {
				return nil, err
			}
			for _ = range numOfAvailTimeSlots {
				var startTime [4]byte
				if err = binary.Read(buf, binary.BigEndian, &startTime); err != nil {
					return nil, err
				}
				var endTime [4]byte
				if err = binary.Read(buf, binary.BigEndian, &endTime); err != nil {
					return nil, err
				}

				timeSlot := TimeSlot{
					StartTime: MinutesToHourMinutes(startTime,true),
					EndTime:   MinutesToHourMinutes(endTime,true),
				}

				availability.TimeSlots = append(availability.TimeSlots, timeSlot)
			}

		}

		newReply.Availabilities = availabilities
		break
	case 102:
		break
	case 103:
		break
	case 104:
		break
	case 105:
		var capacity uint32
		if err := binary.Read(buf, binary.BigEndian, &capacity); err != nil {
			return nil, err
		}
		newReply.Capacity = capacity
		break
	case 106:
		break
	case 107:
		var numFacility uint32
		var facilities []string
		if err := binary.Read(buf, binary.BigEndian, &numFacility); err != nil {
			return nil, err
		}
		fmt.Printf("Number of Facilities %v",numFacility)
		for _ = range numFacility {
			var facilityNameLen uint32
			if err := binary.Read(buf, binary.BigEndian, &facilityNameLen); err != nil {
				return nil, err
			}
			facility := make([]byte, facilityNameLen)
			if err := binary.Read(buf, binary.BigEndian, &facility); err != nil {
				return nil, err
			}
			facilities = append(facilities, string(facility))
		}
		newReply.FacilityNames = facilities
		break
	}

	return &newReply, nil

}
