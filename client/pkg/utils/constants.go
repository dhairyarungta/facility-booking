package utils

type Day uint32

const (
	Monday Day = iota
	Tuesday
	Wednesday
	Thursday
	Friday
	Saturday
	Sunday
)

type Request struct {
	Uid [8]byte
	Days [7]Day
	Op byte
	FacilityName [4]byte
	StartTIme [4]byte
	EndTime [4]byte
	Offset [4]byte
}

type Response struct {
	Availabiltiies [8]byte
	Uid [8]byte
	NumAvail uint32
	ErrorCode uint32
	Capacity uint32
	Day Day
}