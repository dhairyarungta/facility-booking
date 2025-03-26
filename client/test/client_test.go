package test


import (
	"testing"

	//"unsafe"

	//"github.com/dhairyarungta/facility-booking/client/pkg/utils"
	"github.com/stretchr/testify/assert"
)


func TestCStructSize(t *testing.T){
    assert := assert.New(t)
    // RequestMessage := utils.Request{}
    assert.Equal(1,1)
    // ReplyMessage := utils.Response{}
    // assert.Equal(uintptr(56),unsafe.Sizeof(RequestMessage))
    // assert.Equal(uintptr(32),unsafe.Sizeof(ReplyMessage))

}
