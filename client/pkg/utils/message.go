package utils

import (
	"bytes"
	"encoding/binary"
	"errors"
	"fmt"
)

type Day byte

//Hour minutes each byte is a char
type HourMinutes [4]byte

func(hm *HourMinutes) ToString() string{
	return fmt.Sprintf("%c%c:%c%c", hm[0], hm[1], hm[2], hm[3])
}

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

var CharToDay = map[byte]string{
	byte(Monday):    "Monday",
	byte(Tuesday):   "Tuesday",
	byte(Wednesday): "Wednesday",
	byte(Thursday):  "Thursday",
	byte(Friday):    "Friday",
	byte(Saturday):  "Saturday",
	byte(Sunday):    "Sunday",
}

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
	Port	     uint16
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

func FormatReplyMessage(reply *UnMarshalledReplyMessage){
	switch reply.Op {
	case 101:
		for _,availability := range reply.Availabilities{
			fmt.Printf("Availabilities for %v\n",CharToDay[byte(availability.Day)])
			fmt.Printf("---------------------------\n")
			for i,timeSlot := range availability.TimeSlots{
				startTime := timeSlot.StartTime.ToString()
				endTime := timeSlot.EndTime.ToString()
				fmt.Printf("%v - StartTime: %v - EndTime %v\n", i+1,startTime,endTime)
			}
		}
	case 102:
		fmt.Printf("Booking %v successfully created\n",reply.Uid)
	case 103:
		fmt.Printf("Update successful %v OK\n",reply.Op)
	case 104:
		fmt.Printf("Callback registered successfully %v OK\n",reply.Op)
	case 105:
		fmt.Printf("Total Capacity: %v\n",reply.Capacity)
	case 106:
		fmt.Printf("Update successful %v OK\n",reply.Op)
	case 107:
		fmt.Printf("Available facilities for booking\n")
		fmt.Printf("--------------------------------\n")
		for i,facilityName := range reply.FacilityNames{
			fmt.Printf("%v - %v\n",i+1,facilityName)
		}
	case 200:
		fmt.Printf("Error %v: Invalid facility name\n",reply.Op)
	case 300:
		fmt.Printf("Error %v: Facility unavailable during requested timeslot",reply.Op)
	case 400:
		fmt.Printf("Error %v: Invalid booking confirmation id\n",reply.Op)
	default:
		fmt.Print("Invalid Op Code")
	}
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
		byte(tensHour + '0'),
		byte(onesHour + '0'),
		byte(tensMinute + '0'),
		byte(onesMinute + '0'),
	}
	return hourMinutes
}

func HourMinutesToMinutes( hourMinutes HourMinutes) [4]byte{
	tensHour := uint32(hourMinutes[0] - '0') * 10
	onesHour := uint32(hourMinutes[1] - '0')
	hour := tensHour + onesHour

	tensMinute := uint32(hourMinutes[2] -'0') * 10
	onesMinute := uint32(hourMinutes[3] - '0')
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
		port := req.Port
		for _, data := range []interface{}{
			facilityNameLen,
			[]byte(facilityName),
			offset,
			port,
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
		offset := req.Offset
		err := binary.Write(&buf,binary.BigEndian,offset)
		if err!=nil{
			return nil,err
		}
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
