package test

/*

typedef enum Day {
    Monday,
    Tuesday,
    Wednesday,
    Thursday,
    Friday,
    Saturday,
    Sunday,
} Day;

typedef struct RequestMessage {
    char uid[8];
    enum Day days[7];
    char op;
    char facilityName[4];
    char startTime[4];
    char endTime[4];
    char offset[4];
} RequestMessage;

typedef struct ReplyMessage {
    char availabilities[8];
    char uid[8];
    unsigned int numAvail;
    unsigned int errorCode;
    unsigned int capacity;
    enum Day day;
} ReplyMessage;

*/
import (
	"testing"

	"unsafe"

	"github.com/dhairyarungta/facility-booking/client/pkg/utils"
	"github.com/stretchr/testify/assert"
)


func TestCStructSize(t *testing.T){
    assert := assert.New(t)
    RequestMessage := utils.Request{}

    ReplyMessage := utils.Response{}

    assert.Equal(uintptr(56),unsafe.Sizeof(RequestMessage))
    assert.Equal(uintptr(32),unsafe.Sizeof(ReplyMessage))



}
